#include "StdAfx.h"
#include <system/io/async.h>
#include <system/time/simpletimer.h>
#include <framework/Framework.h>
#include <OverlayFs/OverlayFs.h>
#include <LogFs/LogFs.h>
#include <MemoryFs/MemoryFs.h>
#include <GameFs/GameHelpers.h>
#include <gamesource/GameSource.h> // [Blur1Multi] game source selection (see Blur1Multi/README.md)
#include <X360ConvertFs.h> // [Blur1Multi] Xbox 360 and PS3 assets served in PC format
#include <x360convert/X360Convert.h>

// [Blur1Multi] the converting layer over m_pFileSystem for an Xbox 360 or PS3 source (NULL otherwise)
static X360Convert::C_ConvertFs* s_pConvertFs = NULL;
#include <AudioCacheFs.h> // [Blur1Multi] Xbox 360 audio banks through the audio cache (T075, D15)
// [Blur1Multi] over s_pConvertFs for an Xbox 360 source (NULL otherwise)
static X360Convert::C_AudioCacheFs* s_pAudioFs = NULL;
// [Blur1Multi] the game source chosen in Initialise (GameSource::Select)
static GameSource::C_SourceInfo s_source;
static bool s_sourceFound = false;
#include <CatalogFs.h> // [BlurX] several discs served through the asset catalog
// [BlurX] the asset catalog layer of m_pFileSystem in catalog mode (NULL otherwise)
static AssetCatalog::C_CatalogFs* s_pCatalogFs = NULL;

// [BlurX] Diagnostic (BLURX_OOM_STACK=1): turn the engine's out-of-memory panic into an
// exception, so the crash handler writes the stack of the failing allocation.
namespace Memory { void SetOutOfMemoryCallback(void (*handler)(void)); }
static void BlurXOutOfMemoryStack() { RaiseException(0xE0B10001, 0, 0, NULL); }

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

	// [Blur1Multi] Select the game source before the game file system exists: CreateGameFs
	// makes the working directory layer 0, so the source must be the working directory by
	// then (a PS3 source moves it into PS3_GAME/USRDIR). CreateFsLayers reports the result.
	// [BlurX] Catalog mode (--profile=<name> or BLURX_PROFILE) has no single source: the
	// working directory stays where it is (the writable layer 0) and CreateFsLayers mounts
	// the profile's discs through the asset catalog.
	if (!AssetCatalog::C_CatalogFs::Requested())
		s_sourceFound = GameSource::Select(s_source);
	if (getenv("BLURX_OOM_STACK"))
		Memory::SetOutOfMemoryCallback(BlurXOutOfMemoryStack);

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
	SAFE_DELETE_PTR(s_pAudioFs);	// [Blur1Multi]
	SAFE_DELETE_PTR(s_pConvertFs);	// [Blur1Multi]
	SAFE_DELETE_PTR(m_pFileSystem);
	SAFE_DELETE_PTR(s_pCatalogFs);	// [BlurX] after the overlay that holds it as a layer
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

	// [Blur1Multi] The game source was selected in Initialise, before CreateGameFs: --source=<dir>
	// or BLUR_SOURCE, default the working directory as retail. The source became the working
	// directory, which is layer 0 of the overlay file system. A PC source then gets the
	// retail PC mount sequence unchanged; an Xbox 360 or PS3 source gets the console
	// builds' mounts (the non-PC branches below, selected at run time; both consoles used
	// USE_NEW_PAK_LAYOUT). Other sources stop here.
	// [BlurX] Catalog mode: the data comes from several Xbox 360 discs at once through the
	// asset catalog (mounted below, after the retail sequence, which it skips).
	const bool catalogMode = AssetCatalog::C_CatalogFs::Requested();
	if (!catalogMode)
	{
		const GameSource::C_SourceInfo& source = s_source;
		const bool found = s_sourceFound;
		char text[1024];
		GameSource::Describe(source, text, sizeof text);
		LOGINFO(Framework::g_BootLog, "GameSource: %s", text);
		if (!found || (source.m_Platform != GameSource::E_Platform_PC && !GameSource::IsConsole()))
		{
			LOGWARN(Framework::g_BootLog, "GameSource: this build has no data layer for that source; stopping");
			GameSource::FailUnsupported(source, text);
		}
	}
	const bool xbox360Source = catalogMode || GameSource::Current().m_Platform == GameSource::E_Platform_Xbox360;	// [BlurX] catalogMode
	const bool consoleSource = catalogMode || GameSource::IsConsole();	// [BlurX] catalogMode

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
		// [Blur1Multi] an Xbox 360 source streams XMA audio: the Xenon buffer size (PS3 as PC)
		(xbox360Source ? AUDIO_XMA_BUFFER_SIZE_K : AUDIO_ADPCM_BUFFER_SIZE_K)*1024,
#endif
		true, //iscritical
		true, //asserteof
		1 // num buffers
	);

	if (!catalogMode)	// [BlurX] the retail mount sequence; catalog mode mounts each disc in its own layer below
	{
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
		// [Blur1Multi] Xbox 360 or PS3 source: the console list (the #else branch below, which
		// both the Xenon and the PS3 build compiled), unchanged.
		char* x360Paks[] =			{ "cache/root.pak",	"cache/root_streaming.pak",	"cache/shaders-speed.pak"};
		char* x360DstPaths[] =		{ "/",				"/",						"/graphics/shaders/speed/"};
		char* x360SrcPaths[] =		{ "/",				"/",						"/"};
		Sys::C_OpenParams* x360Opt[] ={ &defaultPakParams,&unalignedUbPakParams,	&defaultPakParams};
		char** nonSecurePaks = consoleSource ? x360Paks : pcPaks;
		char** nonSecureDstPaths = consoleSource ? x360DstPaths : pcDstPaths;
		char** nonSecureSrcPaths = consoleSource ? x360SrcPaths : pcSrcPaths;
		Sys::C_OpenParams** opt = consoleSource ? x360Opt : pcOpt;
		const S32 nonSecureCount = consoleSource ? lengthof(x360Paks) : lengthof(pcPaks);
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
	if (consoleSource && g_bootRamPakEnable)	// [Blur1Multi] the console boot RAM pak, for Xbox 360 and PS3 sources
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

	}	// [BlurX] !catalogMode

	// [BlurX] Catalog mode: one layer above the working directory serves every file the
	// catalog knows, each from the disc the profile picks for it, already converted to PC
	// data by that disc's own converting layer (D12).
	if (catalogMode && !s_pCatalogFs)
	{
		X360Convert::SetSource(X360Convert::E_Source_Xbox360);
		std::string error;
		s_pCatalogFs = AssetCatalog::C_CatalogFs::Create(nativeFs, error);
		if (!s_pCatalogFs)
		{
			LOGWARN(Framework::g_BootLog, "AssetCatalog: %s; stopping", error.c_str());
			GameSource::FailUnsupported(GameSource::Current(), ("BlurX asset catalog: " + error).c_str());
		}
		LogicalFs::C_OverlayLayer::C_Params params;
		params.flags.layerType = LogicalFs::C_OverlayLayer::E_Type_Overlay;
		params.flags.writable = false;
		((LogicalFs::C_OverlayFs*)m_pFileSystem)->AddLayer("/", s_pCatalogFs, "/", params, LogicalFs::E_MediaLayer_0, "AssetCatalog");
		LOGINFO(Framework::g_BootLog, "AssetCatalog: profile '%s' serves the game", s_pCatalogFs->Profile().Name().c_str());
		// [BlurX] The game source (GameSource::Current()) is the profile's first disc found on
		// this machine. Blur1Multi code picks per-platform behaviour from its platform (the
		// damage volume format in DamageLayer.cpp, the console lighting prepass in Prepass.cpp);
		// left unselected it reads E_Platform_Unknown, which built the damage volume as F32 for
		// the Xbox 360 vehicle programs and hid every car body (T093). Every disc the catalog
		// covers is an Xbox 360 disc. Select() moves the working directory to the source, so
		// the working directory (catalog mode's writable layer 0) is put back afterwards.
		{
			const AssetCatalog::C_Catalog& catalog = s_pCatalogFs->Catalog();
			const std::vector<std::string>& order = s_pCatalogFs->Profile().Order();
			std::string root;
			for (size_t o = 0; o < order.size() && root.empty(); ++o)
				for (size_t d = 0; d < catalog.Discs().size(); ++d)
					if (catalog.Discs()[d].m_Id == order[o])
					{
						const DWORD a = GetFileAttributesA(catalog.Discs()[d].m_Root.c_str());
						if (a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY))
							root = catalog.Discs()[d].m_Root;
						break;
					}
			char cwd[MAX_PATH] = { 0 };
			GetCurrentDirectoryA(MAX_PATH, cwd);
			const char* prev = getenv("BLUR_SOURCE");
			const std::string prevSource = prev ? prev : "";
			_putenv_s("BLUR_SOURCE", root.c_str());
			const bool found = !root.empty() && GameSource::Select(s_source);
			_putenv_s("BLUR_SOURCE", prevSource.c_str());
			SetCurrentDirectoryA(cwd);
			char text[1024];
			GameSource::Describe(GameSource::Current(), text, sizeof text);
			if (found && GameSource::Current().m_Platform == GameSource::E_Platform_Xbox360)
				LOGINFO(Framework::g_BootLog, "AssetCatalog: game source platform from the profile's first disc: %s", text);
			else
				LOGWARN(Framework::g_BootLog, "AssetCatalog: the profile's first disc is not an Xbox 360 source (%s); per-platform code will misbehave", text);
		}
		// BLURX_DUMP_RAW=1: the BLUR1MULTI_PAKDUMP hook below reads the discs' bytes, unconverted
		if (getenv("BLURX_DUMP_RAW"))
		{
			const int raw = GameSource::DumpListedFiles(*m_pFileSystem);
			if (raw >= 0)
				LOGINFO(Framework::g_BootLog, "AssetCatalog: BLUR1MULTI_PAKDUMP wrote %d unconverted files", raw);
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
	// A PS3 source goes through the same layer: the formats both consoles share convert
	// the same way, the rest by source (X360Convert::SetSource).
	if (consoleSource && !catalogMode && !s_pConvertFs)	// [BlurX] catalog mode: each disc has its own converters (CatalogFs)
	{
		X360Convert::SetSource(xbox360Source ? X360Convert::E_Source_Xbox360 : X360Convert::E_Source_PS3);
		s_pConvertFs = new X360Convert::C_ConvertFs(m_pFileSystem);
		SetFileSystems(s_pConvertFs, m_pMemoryFileSystem);
		FX::SetFileSystem(s_pConvertFs);
		// Xbox 360 audio banks are XMA: decoded once into the audio cache (D15); PS3
		// banks are converted in place by the converting layer
		if (xbox360Source)
		{
			s_pAudioFs = new X360Convert::C_AudioCacheFs(s_pConvertFs, nativeFs, "x360");
			SetFileSystems(s_pAudioFs, m_pMemoryFileSystem);
		}
		LOGINFO(Framework::g_BootLog, "GameSource: %s assets are converted to PC format on load",
			GameSource::PlatformName(GameSource::Current().m_Platform));
	}

	const int dumped = (catalogMode && getenv("BLURX_DUMP_RAW")) ? -1 :	// [BlurX] dumped unconverted above
		GameSource::DumpListedFiles(s_pAudioFs ? *(Sys::I_FileSystem*)s_pAudioFs :
			s_pConvertFs ? *(Sys::I_FileSystem*)s_pConvertFs : *(Sys::I_FileSystem*)m_pFileSystem);
	if (dumped >= 0)
		LOGINFO(Framework::g_BootLog, "GameSource: BLUR1MULTI_PAKDUMP wrote %d files", dumped);
	// [BlurX] a catalog-mode dump run stops here, as a single console source's does below
	if (catalogMode && getenv("BLUR1MULTI_PAKDUMP"))
		GameSource::FailUnsupported(GameSource::Current(), "BlurX: BLUR1MULTI_PAKDUMP run finished");
	// BLUR1MULTI_X360_CONTINUE=1 / BLUR1MULTI_PS3_CONTINUE=1 (exploration only) boot on past
	// this point to see which asset fails first.
	if (consoleSource && !catalogMode && !getenv(xbox360Source ? "BLUR1MULTI_X360_CONTINUE" : "BLUR1MULTI_PS3_CONTINUE"))	// [BlurX] catalog mode boots on
	{
		char text[1024];
		GameSource::Describe(GameSource::Current(), text, sizeof text);
		LOGWARN(Framework::g_BootLog, "GameSource: %s paks mounted; asset conversion (T072/T076) not complete; stopping",
			GameSource::PlatformName(GameSource::Current().m_Platform));
		GameSource::FailUnsupported(GameSource::Current(), text);
	}
}

void C_GameSystems::UnloadGamedataRampak()
{
#ifndef PLATFORM_PC
	if (g_bootRamPakEnable)
#else
	if (GameSource::IsConsole() && g_bootRamPakEnable)	// [Blur1Multi]
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
