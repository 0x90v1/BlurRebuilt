#include "StdAfx.h"
#include <system/io/async.h>
#include <system/time/simpletimer.h>
#include <framework/Framework.h>
#include <OverlayFs/OverlayFs.h>
#include <LogFs/LogFs.h>
#include <MemoryFs/MemoryFs.h>
#include <GameFs/GameHelpers.h>
#include <gamesource/GameSource.h> // [Blur1Multi] game source selection (see Blur1Multi/README.md)
#include <X360ConvertFs.h> // [Blur1Multi] Xbox 360 assets served in PC format

// [Blur1Multi] the converting layer over m_pFileSystem for an Xbox 360 source (NULL otherwise)
static X360Convert::C_ConvertFs* s_pConvertFs = NULL;

#include <GameSystems.h>
#include <SchedulerConfig.h>

#include <Framework/Application/LoadDebugVars.h>
#include <Framework/Application/CoreDialogs.h>

#include <ui/Game/UIScreenControl.h>
#include <ui/Graphics/Localisation/Loc.h>
#include <LuaDebugger.h>

#include <Ui/Game/Automation/UIAutomation.h>
#include <Automation/GameAutomationMessageHandler.h>
#include <Resource/Resource.h>
#include <SystemProfile/SystemProfileManager.h>

#include <fx/fx.h>

#include <GameApp.h>
#include <PakNames.h>

#include <system/scheduler.h>
#include "SchedulerConfig.h"
#include <PakFs/PakFs.h>

#include <Security/RSASecurity.h>
#define RENDERINGDATA_ALLOW_INCLUDE
#include <renderingdata/renderingdata.h>

#if defined(PLATFORM_PS3) || defined(PLATFORM_SPU)
#include <sys/paths.h>
#endif

#include <Game/PublicKey/Amax_Public_Key.h>
#include <AudioBufferSizes.h>

#ifdef CFILESYSTEM_CREATE_FUNC_ENABLED
Sys::C_FileSystem* CFileSystemCreateFunc()
{
	Sys::C_FileSystem* pFs = new LOGFS_WRAPPER(Sys::C_FileSystem, "CFileSystem");
	return pFs;
}
#endif

#ifdef CFG_GOLD
//#define ENABLE_MASTER_GAMEDATA_SECURITY
#else
//#define ENABLE_DEBUG_SHADERS
#endif

namespace Game
{
extern Debug::Bool g_bootRamPakEnable;

static Debug::Logger &gLog = Debug::GetLogger("amax.gamesystems");

// C_GameSystems

C_GameSystems* C_GameSystems::s_pInstance = NULL;

C_GameSystems::C_GameSystems()
{
	assert(s_pInstance == NULL);
	s_pInstance = this;
	m_pFileSystem = NULL;
	m_pMemoryFileSystem = NULL;
		
	LuaDebugger::InitialiseNetworkConnection();

	Automation::RegisterCallback(UIAutomation::Callback);
	Automation::RegisterCallback(GameAutomation::Callback);

#ifndef CFG_GOLD
	gGameLoadDebugVariables = Framework::LoadDebugVars;
	gGameSaveDebugVariables = Framework::SaveDebugVars;

	UI::gUiSetLuaHookRoutine = LuaDebugger::Update;
#endif

	m_pSchedulerConfig = new C_SchedulerConfig;
}

C_GameSystems::~C_GameSystems()
{
	SAFE_DELETE_PTR( m_pSchedulerConfig );

	assert(s_pInstance == this);
	s_pInstance = NULL;
}

C_GameSystems& C_GameSystems::Instance()
{
	assert(s_pInstance);
	return *s_pInstance;
}

S32 PrimaryPadQuery()
{
	return SysProfile::C_Manager::Instance().GetPrimaryPadNo();
}

HRESULT C_GameSystems::Initialise(const Framework::C_CoreSystemsParams& params)
{
	HRESULT hr = S_OK;

	const_cast<Framework::C_CoreSystemsParams&>(params).m_Name = "Amax";
		
	hr = C_CoreSystems::Initialise( params );
	if( FAILED(hr) )
		return hr;

	hr = m_pSchedulerConfig->Initialise();
	if( FAILED(hr) )
		return hr;

	LOGFS_INIT("./user/logfs");

	//Use default dirty disc callback.
	Framework::SetPrimaryPadQueryCallback(&PrimaryPadQuery);
	Sys::SetFileRwErrorCallback(&Framework::CriticalReadErrorDialog);

	m_pFileSystem = LogicalFs::GameHelpers::CreateGameFs();
	m_pMemoryFileSystem = new LogicalFs::C_MemoryFs();
	SetFileSystems(m_pFileSystem, m_pMemoryFileSystem);

	FX::SetFileSystem(m_pFileSystem);
	Pak::SetPacketCompressionPolicy(m_pSchedulerConfig->GetPakPolicy());
	return hr;
}

void C_GameSystems::Shutdown()
{
	RenderingData::Target::StreamManager::Instance().Destroy();

	SetFileSystems(&Sys::C_FileSystem::Instance(), NULL);
	SAFE_DELETE_PTR(s_pConvertFs);	// [Blur1Multi]
	SAFE_DELETE_PTR(m_pFileSystem);
	SAFE_DELETE_PTR(m_pMemoryFileSystem);
	m_pSchedulerConfig->Shutdown();
	C_CoreSystems::Shutdown();
}

void C_GameSystems::Update()
{
	if(!C_GameApp::GetInstance()->IsLoadingScreenEnabled())
	{
		C_CoreSystems::Update();
	}

	Resource::C_Manager::GetInstance()->Update();
}

#define USE_NEW_PAK_LAYOUT
void C_GameSystems::CreateFsLayers()
{
#ifdef PLATFORM_XENON
	IoSuspendFileCaching();
#endif

	Sys::I_FileSystem& nativeFs = Sys::C_FileSystem::Instance();

	// [Blur1Multi] Select the game source before any layer is mounted: --source=<dir> or
	// BLUR_SOURCE, default the working directory as retail. The source becomes the working
	// directory, which is layer 0 of the overlay file system. A PC source then gets the
	// retail PC mount sequence unchanged; an Xbox 360 source gets the Xenon build's mounts
	// (the non-PC branches below, selected at run time). Other platforms stop here.
	{
		GameSource::C_SourceInfo source;
		bool found = GameSource::Select(source);
		char text[1024];
		GameSource::Describe(source, text, sizeof text);
		LOGINFO(Framework::g_BootLog, "GameSource: %s", text);
		if (!found || (source.m_Platform != GameSource::E_Platform_PC && source.m_Platform != GameSource::E_Platform_Xbox360))
		{
			LOGWARN(Framework::g_BootLog, "GameSource: this build has no data layer for that source; stopping");
			GameSource::FailUnsupported(source, text);
		}
	}
	const bool xbox360Source = GameSource::Current().m_Platform == GameSource::E_Platform_Xbox360;

#ifdef PLATFORM_PS3
	LogicalFs::GameHelpers::SetTitleId(PlatformPS3::g_ps3_titleId);
#endif

	//Mount paks
	Sys::C_OpenParams defaultPakParams(
		Sys::C_OpenParams::kBufferSizeHintDefault,
		true, //iscritical
		true, //asserteof
		0 // use default # buffers;
	);

	//As close as we can get to true unbuffered io via cakey's stuff without refactoring the pak system.
	//We have to have one buffer which should match the buffer size being used to read out of the pak.
	//The reason for using a buffer is that otherwise cakey's io layer enforces 2k alignment of reads.
	Sys::C_OpenParams unalignedUbPakParams(
#ifdef PLATFORM_XENON
		AUDIO_XMA_BUFFER_SIZE_K*1024, // buffer size
#else
		//PC & PS3
		// [Blur1Multi] an Xbox 360 source streams XMA audio: the Xenon buffer size
		(xbox360Source ? AUDIO_XMA_BUFFER_SIZE_K : AUDIO_ADPCM_BUFFER_SIZE_K)*1024,
#endif
		true, //iscritical
		true, //asserteof
		1 // num buffers
	);

	//Not secure retail...
	{
		U32 defaultBufSize = Sys::C_OpenParams::kBufferSizeHintDefault;
#ifdef USE_NEW_PAK_LAYOUT
	//Spiral house still using old pak layout, probably okay for PC
	#ifdef PLATFORM_PC
		// [Blur2PC] retail PC also ships root_streaming.pak: the streamed audio banks (menumusic.baf,
		// ingamemusic.baf, env_*.baf) stored at its root and requested as audio/<name>.baf.
		char* pcPaks[] =			{ "cache/root.pak",	"cache/models.pak",			"cache/shaders-speed.pak",	"cache/root_streaming.pak"};
		char* pcDstPaths[] =		{ "/",				"/models/",					"/graphics/shaders/speed/",	"/audio/"};
		char* pcSrcPaths[] =		{ "/",				"/models/",					"/",						"/"};
		Sys::C_OpenParams* pcOpt[] ={ &defaultPakParams,&defaultPakParams,			&defaultPakParams,			&unalignedUbPakParams};
		// [Blur1Multi] Xbox 360 source: the Xenon list (the #else branch below), unchanged.
		char* x360Paks[] =			{ "cache/root.pak",	"cache/root_streaming.pak",	"cache/shaders-speed.pak"};
		char* x360DstPaths[] =		{ "/",				"/",						"/graphics/shaders/speed/"};
		char* x360SrcPaths[] =		{ "/",				"/",						"/"};
		Sys::C_OpenParams* x360Opt[] ={ &defaultPakParams,&unalignedUbPakParams,	&defaultPakParams};
		char** nonSecurePaks = xbox360Source ? x360Paks : pcPaks;
		char** nonSecureDstPaths = xbox360Source ? x360DstPaths : pcDstPaths;
		char** nonSecureSrcPaths = xbox360Source ? x360SrcPaths : pcSrcPaths;
		Sys::C_OpenParams** opt = xbox360Source ? x360Opt : pcOpt;
		const S32 nonSecureCount = xbox360Source ? lengthof(x360Paks) : lengthof(pcPaks);
	#else
		char* nonSecurePaks[] =		{ "cache/root.pak",	"cache/root_streaming.pak",	"cache/shaders-speed.pak"};
		char* nonSecureDstPaths[] =	{ "/",				"/",						"/graphics/shaders/speed/"};
		char* nonSecureSrcPaths[] =	{ "/",				"/",						"/"};
		Sys::C_OpenParams* opt[] =  { &defaultPakParams,&unalignedUbPakParams,		&defaultPakParams};
	#endif
#else
	#ifdef PLATFORM_PS3
			char* nonSecurePaks[] =		{ "cache/root.pak",	"cache/models.pak",	"cache/root_streaming.pak",	"cache/shaders-speed.pak"};
			char* nonSecureDstPaths[] =	{ "/",				"/models/",			"/",						"/graphics/shaders/speed/"};
			char* nonSecureSrcPaths[] =	{ "/",				"/models/",			"/",						"/"};
			Sys::C_OpenParams* opt[] =  { &defaultPakParams,&defaultPakParams,	&unalignedUbPakParams,		&defaultPakParams};
	#else
			char* nonSecurePaks[] =		{ "cache/root.pak",	"cache/models.pak", "cache/shaders-speed.pak"};
			char* nonSecureDstPaths[] =	{ "/",				"/models/",			"/graphics/shaders/speed/"};
			char* nonSecureSrcPaths[] =	{ "/",				"/models/",			"/"};
			Sys::C_OpenParams* opt[] =  { &defaultPakParams,&defaultPakParams,	&defaultPakParams};
	#endif
#endif

#ifndef PLATFORM_PC
		const S32 nonSecureCount = lengthof(nonSecurePaks);
#endif
		for (S32 i = 0; i < nonSecureCount; ++i)
		{
			Sys::C_FilePath pakPath = 
				LogicalFs::GameHelpers::GetPotentiallyInstalledFilePath(nonSecurePaks[i]);
			LogicalFs::GameHelpers::MountPakLayer(
#ifdef PLATFORM_PS3
				nativeFs,
#else
				*LogicalFs::GameHelpers::GetGameFs(),
#endif
				(LogicalFs::C_OverlayFs&)*m_pFileSystem,
				pakPath,
				nonSecureDstPaths[i], 
				nonSecureSrcPaths[i],
				NULL,
				*opt[i]);
		}
	}

	//Secure retail...
	{
		Security::RSASecurity rsa(1024);
		rsa.InitKey(g_amax_publicKey, lengthof(g_amax_publicKey));
		rsa.InitSignature("cache/gamedata.sig");

		Sys::C_FilePath pakPath = 
			LogicalFs::GameHelpers::GetPotentiallyInstalledFilePath("cache/gamedata.pak");

		LogicalFs::GameHelpers::MountPakLayer(
#ifdef PLATFORM_PS3
			nativeFs,
#else
			*LogicalFs::GameHelpers::GetGameFs(),
#endif
			(LogicalFs::C_OverlayFs&)*m_pFileSystem,
			pakPath,
			"/gamedata/",
			"/gamedata/",
			&rsa);
	}

	//Not secure debug
#ifdef ENABLE_DEBUG_SHADERS
	{
		char* nonSecurePaks[] =		{ "cache/shaders-debug.pak"};
		char* nonSecureDstPaths[] =	{ "/graphics/shaders/debug/" };
		char* nonSecureSrcPaths[] =	{ "/"};
		for (S32 i = 0; i < lengthof(nonSecurePaks); ++i)
			LogicalFs::GameHelpers::MountPakLayer(
#ifdef PLATFORM_PS3
				nativeFs,
#else
				*LogicalFs::GameHelpers::GetGameFs(),
#endif
				(LogicalFs::C_OverlayFs&)*m_pFileSystem,
				nonSecurePaks[i], 
				nonSecureDstPaths[i], 
				nonSecureSrcPaths[i]);
	}
#endif

#ifndef PLATFORM_PC
	if (g_bootRamPakEnable)
#else
	if (xbox360Source && g_bootRamPakEnable)	// [Blur1Multi] the Xenon boot RAM pak, for Xbox 360 sources
#endif
	{
		Security::RSASecurity rsa(1024);
		rsa.InitKey(g_amax_publicKey, lengthof(g_amax_publicKey));
		rsa.InitSignature("cache/gamedata_boot.sig");

		Sys::C_FilePath ramPak = LogicalFs::GameHelpers::GetPotentiallyInstalledFilePath(GAMEDATA_BOOT_FILEPATH);
		{
			Time::C_SimpleTimer simpleTimer;
			Sys::C_FileInfo info;
			LogicalFs::GameHelpers::GetGameFs()->FileInfo(ramPak, &info);
			LogicalFs::GameHelpers::MountRamPakLayer(
				nativeFs,
				*C_GameSystems::Instance().GetMemoryFs(),
				*LogicalFs::GameHelpers::GetGameFs(),
				ramPak,
				GAMEDATA_BOOT_FILEPATH,
				"/",
				"/",
				&rsa);
			F32 sizeMb = (F32)info.m_FileSize / (1024.f*1024.f);
			F32 deltaSecs = simpleTimer.Stop() / 1000.f;

			LOGINFO(Framework::g_BootLog, "%s read in %.2f secs @ %.2fmb/s", ramPak.Chars(), deltaSecs, sizeMb / deltaSecs);
		}
	}

#ifdef PLATFORM_XENON
	IoResumeFileCaching();
#endif

	// [Blur1Multi] Verification hook (BLUR1MULTI_PAKDUMP), then, for an Xbox 360 source, stop:
	// its paks are mounted and readable, but every asset in them is still in 360 format
	// (big-endian chunk files, Xenos textures and shaders) and the conversion layer is T072.
	// [Blur1Multi] An Xbox 360 source's assets are read through the converting layer:
	// every reader (resource manager, default file system, Display, FX) gets it.
	if (xbox360Source && !s_pConvertFs)
	{
		s_pConvertFs = new X360Convert::C_ConvertFs(m_pFileSystem);
		SetFileSystems(s_pConvertFs, m_pMemoryFileSystem);
		FX::SetFileSystem(s_pConvertFs);
		LOGINFO(Framework::g_BootLog, "GameSource: Xbox 360 assets are converted to PC format on load");
	}

	const int dumped = GameSource::DumpListedFiles(s_pConvertFs ? *(Sys::I_FileSystem*)s_pConvertFs : *(Sys::I_FileSystem*)m_pFileSystem);
	if (dumped >= 0)
		LOGINFO(Framework::g_BootLog, "GameSource: BLUR1MULTI_PAKDUMP wrote %d files", dumped);
	// BLUR1MULTI_X360_CONTINUE=1 (exploration only) boots on past this point to see which
	// asset fails first.
	if (xbox360Source && !getenv("BLUR1MULTI_X360_CONTINUE"))
	{
		char text[1024];
		GameSource::Describe(GameSource::Current(), text, sizeof text);
		LOGWARN(Framework::g_BootLog, "GameSource: Xbox 360 paks mounted; asset conversion (T072) not written yet; stopping");
		GameSource::FailUnsupported(GameSource::Current(), text);
	}
}

void C_GameSystems::UnloadGamedataRampak()
{
#ifndef PLATFORM_PC
	if (g_bootRamPakEnable)
#else
	if (GameSource::Current().m_Platform == GameSource::E_Platform_Xbox360 && g_bootRamPakEnable)	// [Blur1Multi]
#endif
	{
		LogicalFs::GameHelpers::UnmountRamPakLayer(
			*C_GameSystems::Instance().GetMemoryFs(),
			*LogicalFs::GameHelpers::GetGameFs(),
			GAMEDATA_BOOT_FILEPATH);
	}
}

#ifdef PLATFORM_PS3
void C_GameSystems::RemountPakLayers()
{
	Sys::I_FileSystem& nativeFs = Sys::C_FileSystem::Instance();

	for (U32 layerIdx = 0; layerIdx < m_pFileSystem->GetNumLayers(LogicalFs::E_MediaLayer_0); ++layerIdx)
	{
		LogicalFs::C_OverlayLayer layer;
		if (!m_pFileSystem->GetLayerByIndex(layer, LogicalFs::E_MediaLayer_0, layerIdx))
			continue;

		//Must be a better way of doing this.
		if (Sys::ExtractFileExtension(Sys::C_FilePath(layer.GetName())) != "pak")
			continue;

		Sys::C_FilePath sourcePath = layer.GetName();
		Sys::C_FilePath newPath = LogicalFs::GameHelpers::GetPotentiallyInstalledFilePath(Sys::C_FilePath(layer.GetName()));

		if (newPath == sourcePath)
			continue;

		//We've found a cache file that's being loaded from the disc. Remount if present on HDD.
		//If we're already on the hdd this won't do anything.
		LogicalFs::GameHelpers::RemountPakLayer(
			LogicalFs::E_MediaLayer_0,
			layerIdx,
			nativeFs,
			*m_pFileSystem,
			newPath.Chars());
	}
}
#endif

void C_GameSystems::MountPaksByLanguageCode(const char* pLanguageCode)
{
	if (!pLanguageCode)
		return;
	
	//Not mounting language specific paks.
}

LogicalFs::C_MemoryFs* C_GameSystems::GetMemoryFs()
{
	return m_pMemoryFileSystem;
}

C_SchedulerConfig& C_GameSystems::GetSchedulerConfig()
{
	return *m_pSchedulerConfig;
}

} // namespace Game
