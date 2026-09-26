// [BlurX] Asset catalog file-system layer. See CatalogFs.h.

#include "StdAfx.h"

#include "CatalogFs.h"

#include <OverlayFs/OverlayFs.h>
#include <GameFs/GameHelpers.h>
#include <AudioBufferSizes.h>
#include "KanzenFs.h"
#include <X360ConvertFs.h>
#include <AudioCacheFs.h>

#include <algorithm>
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <set>

namespace AssetCatalog
{
	namespace
	{
		Debug::Logger& g_catalogLog = Debug::GetLogger("BlurX.AssetCatalog");

		// "--name=value" from the command line, else the environment variable, else "".
		std::string Option(const char* name, const char* envName)
		{
			const size_t n = strlen(name);
			for (int i = 1; i < __argc; ++i)
			{
				const char* a = __argv[i];
				if (strncmp(a, "--", 2) == 0 && _strnicmp(a + 2, name, n) == 0 && a[2 + n] == '=')
					return a + 3 + n;
			}
			const char* e = getenv(envName);
			return e ? e : "";
		}

		std::string ExeDir()
		{
			char buf[MAX_PATH];
			const DWORD n = GetModuleFileNameA(NULL, buf, MAX_PATH);
			std::string p(buf, n);
			const size_t slash = p.find_last_of("\\/");
			return slash == std::string::npos ? std::string(".") : p.substr(0, slash);
		}

		bool IsFile(const std::string& p)
		{
			const DWORD a = GetFileAttributesA(p.c_str());
			return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
		}

		bool IsDir(const std::string& p)
		{
			const DWORD a = GetFileAttributesA(p.c_str());
			return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
		}

		std::string Upper(std::string s)
		{
			for (size_t i = 0; i < s.size(); ++i)
				s[i] = (char)toupper((unsigned char)s[i]);
			return s;
		}

		// Chunk files that start with a platform tag ("360 " / "PC  ") and that Blur1Multi's
		// converters rewrite in memory.
		bool HasPlatformTag(const char* path)
		{
			static const char* const kTypes[] = { ".model", ".texture", ".level", ".levelphysics", ".levelrender" };
			const size_t n = strlen(path);
			for (size_t i = 0; i < sizeof kTypes / sizeof kTypes[0]; ++i)
			{
				const size_t m = strlen(kTypes[i]);
				if (n >= m && _stricmp(path + n - m, kTypes[i]) == 0)
					return true;
			}
			return false;
		}

		std::string Lower(const char* s)
		{
			std::string r(s);
			for (size_t i = 0; i < r.size(); ++i)
				r[i] = (char)tolower((unsigned char)r[i]);
			return r;
		}

		// "\levels\<name>\level.<ext>" -> "\levels\<name>\" (the level served whole, CatalogFs.h).
		bool LevelDirOf(const char* path, std::string& dir)
		{
			const std::string key = NormalisePath(path);
			if (key.compare(0, 8, "\\levels\\") != 0)
				return false;
			const size_t slash = key.find('\\', 8);
			if (slash == std::string::npos || key.compare(slash, 7, "\\level.") != 0 || key.find('\\', slash + 1) != std::string::npos)
				return false;
			dir = key.substr(0, slash + 1);
			return true;
		}

		// the three tagged files of a level, in the order the engine opens them
		const char* const kLevelFiles[] = { "level.level", "level.levelphysics", "level.levelrender" };
	}

	bool C_CatalogFs::Requested()
	{
		return !Option("profile", "BLURX_PROFILE").empty();
	}

	C_CatalogFs* C_CatalogFs::Create(Sys::I_FileSystem& nativeFs, std::string& error)
	{
		const std::string profileName = Option("profile", "BLURX_PROFILE");
		std::string dir = Option("catalog", "BLURX_CATALOG");
		if (dir.empty())
		{
			const std::string exe = ExeDir();
			const char* candidates[] = { "\\catalog", "\\..\\..\\catalog" };
			for (size_t i = 0; i < 2 && dir.empty(); ++i)
				if (IsFile(exe + candidates[i] + "\\catalog.tsv"))
					dir = exe + candidates[i];
			if (dir.empty())
			{
				error = "no catalog: pass --catalog=<dir> (BlurX\\catalog) or set BLURX_CATALOG";
				return NULL;
			}
		}
		std::string overrides = Option("profile-overrides", "BLURX_PROFILE_OVERRIDES");
		if (overrides.empty())
			overrides = dir + "\\profiles.local.txt";

		C_CatalogFs* pFs = new C_CatalogFs();
		pFs->m_Trace = getenv("BLURX_CATALOG_TRACE") != NULL;
		pFs->m_RawMode = getenv("BLURX_DUMP_RAW") != NULL;
		if (!pFs->m_Catalog.Load((dir + "\\catalog.tsv").c_str(), error) ||
			!pFs->m_Profile.Load((dir + "\\profiles.txt").c_str(), overrides.c_str(), profileName.c_str(), error))
		{
			delete pFs;
			return NULL;
		}
		LOGINFO(g_catalogLog, "catalog %s\\catalog.tsv: %u discs, %u archives, %u rows; profile '%s' (%s)%s",
			dir.c_str(), (unsigned)pFs->m_Catalog.Discs().size(), (unsigned)pFs->m_Catalog.Archives().size(),
			(unsigned)pFs->m_Catalog.NumEntries(), profileName.c_str(), pFs->m_Profile.Description().c_str(),
			IsFile(overrides) ? (" with overrides " + overrides).c_str() : "");

		// where each disc is on this machine
		std::vector<C_Disc>& discs = pFs->m_Catalog.Discs();
		for (size_t d = 0; d < discs.size(); ++d)
		{
			std::string root = Option(("disc=" + discs[d].m_Id).c_str(), ("BLURX_DISC_" + Upper(discs[d].m_Id)).c_str());
			// --disc=<id>=<dir>: Option() matched "disc=<id>" as the name
			if (!root.empty())
				discs[d].m_Root = root;
		}

		const size_t n = discs.size();
		pFs->m_DiscFs.assign(n, (LogicalFs::C_OverlayFs*)NULL);
		pFs->m_KanzenFs.assign(n, (C_KanzenFs*)NULL);
		pFs->m_ConvertFs.assign(n, (Sys::I_FileSystem*)NULL);
		pFs->m_ServeFs.assign(n, (Sys::I_FileSystem*)NULL);
		pFs->m_Mounted.assign(n, false);
		pFs->m_MountNotes.assign(n, std::string("not in the profile"));
		pFs->m_Served.assign(n, 0);
		// every disc the profile can name (order and rules)
		std::set<std::string> wanted(pFs->m_Profile.Order().begin(), pFs->m_Profile.Order().end());
		{
			// rules can name discs outside 'order' (pin); ask with a path no rule excludes
			std::vector<std::string> any;
			pFs->m_Profile.Candidates("\\", any);
			wanted.insert(any.begin(), any.end());
		}
		int mounted = 0;
		for (size_t d = 0; d < n; ++d)
		{
			if (!wanted.count(discs[d].m_Id))
				continue;
			std::string why;
			if (pFs->Mount(nativeFs, (int)d, why))
				++mounted;
			pFs->m_MountNotes[d] = why;
		}
		std::vector<std::string> lines;
		pFs->Describe(lines);
		for (size_t i = 0; i < lines.size(); ++i)
			LOGINFO(g_catalogLog, "%s", lines[i].c_str());
		if (!mounted)
		{
			error = "none of the profile's discs could be mounted";
			delete pFs;
			return NULL;
		}
		return pFs;
	}

	bool C_CatalogFs::Mount(Sys::I_FileSystem& nativeFs, int disc, std::string& why)
	{
		const C_Disc& d = m_Catalog.Discs()[disc];
		if (!IsDir(d.m_Root))
		{
			why = "root not found: " + d.m_Root + " (--disc=" + d.m_Id + "=<dir>)";
			return false;
		}
		LogicalFs::C_OverlayFs* pFs = new LogicalFs::C_OverlayFs();
		pFs->SetWorkingDirectory("/");

		// Layer 0: the disc's loose files, as CreateGameFs mounts the working directory.
		LogicalFs::C_OverlayLayer::C_Params params;
		params.flags.layerType = LogicalFs::GameHelpers::GetDefaultOverlayType();
		params.flags.writable = false;
		pFs->AddLayer("/", &nativeFs, d.m_Root.c_str(), params, LogicalFs::E_MediaLayer_0, "loose");
		// A layer folder of the game (retail X360: "layer1"): overlaid at / as media layer 1, as the Xenon CreateGameFs does.
		for (size_t l = 0; l < d.m_Layers.size(); ++l)
			if (IsDir(d.m_Root + "/" + d.m_Layers[l]))
				pFs->AddLayer("/", &nativeFs, (d.m_Root + "/" + d.m_Layers[l]).c_str(), params, LogicalFs::E_MediaLayer_1, "loose_layer1");

		// Its paks, in the order the engine mounts them (catalog @pak order). No signature
		// check: the prototypes' gamedata.sig is not verified by this build (see D12).
		const Sys::C_OpenParams defaultParams(Sys::C_OpenParams::kBufferSizeHintDefault, true, true, 0);
		const Sys::C_OpenParams streamingParams(AUDIO_XMA_BUFFER_SIZE_K * 1024, true, true, 1);
		int paks = 0, failed = 0;
		for (size_t i = 0; i < m_Catalog.Archives().size(); ++i)
		{
			const C_Archive& a = m_Catalog.Archives()[i];
			if (a.m_Disc != disc || !a.m_Ok || a.m_Mount.empty())
				continue;
			const std::string pak = d.m_Root + "\\" + d.m_Cache + "\\" + a.m_File;
			const bool streaming = _stricmp(a.m_File.c_str(), "root_streaming.pak") == 0;
			if (LogicalFs::GameHelpers::MountPakLayer(nativeFs, *pFs, pak.c_str(), a.m_Mount.c_str(), a.m_Src.c_str(),
				NULL, streaming ? streamingParams : defaultParams))
				++paks;
			else
				++failed;
		}
		m_DiscFs[disc] = pFs;
		// What the catalog serves from: a Generation B disc through the Kanzen layer, then
		// Blur1Multi's Xbox 360 converters (PC data out; one converter set per disc, so a copy
		// that does not convert leaves the next disc to serve the file), then Blur1Multi's
		// audio cache (XMA banks decoded once, D15), in a cache folder per disc.
		// BLURX_DUMP_RAW (verification dumps, run_dump.ps1): the disc's own bytes, unconverted.
		if (m_RawMode)
			m_ServeFs[disc] = pFs;
		else
		{
			Sys::I_FileSystem* pChain = pFs;
			if (d.m_Generation == "B")
				pChain = m_KanzenFs[disc] = new C_KanzenFs(pFs, d.m_Id);
			m_ConvertFs[disc] = new X360Convert::C_ConvertFs(pChain);
			m_ServeFs[disc] = new X360Convert::C_AudioCacheFs(m_ConvertFs[disc], nativeFs, d.m_Id);
		}
		m_Mounted[disc] = true;
		char buf[160];
		sprintf(buf, "mounted: loose + %d paks%s%s", paks, failed ? " (some failed; see the pak log)" : "",
			d.m_Generation == "B" ? "; Kanzen files normalised or refused" : "");
		why = buf;
		return true;
	}

	C_CatalogFs::~C_CatalogFs()
	{
		for (size_t d = 0; d < m_DiscFs.size(); ++d)
		{
			if (m_Served[d])
				LOGINFO(g_catalogLog, "served %u files from %s", m_Served[d], m_Catalog.Discs()[d].m_Id.c_str());
			if (m_ServeFs[d] != (Sys::I_FileSystem*)m_DiscFs[d])
				delete m_ServeFs[d];
			delete m_ConvertFs[d];
			delete m_KanzenFs[d];
			delete m_DiscFs[d];
		}
	}

	void C_CatalogFs::Describe(std::vector<std::string>& lines) const
	{
		for (size_t d = 0; d < m_Catalog.Discs().size(); ++d)
		{
			const C_Disc& disc = m_Catalog.Discs()[d];
			lines.push_back(disc.m_Id + " (" + disc.m_Title + ", generation " + disc.m_Generation + "): " +
				(d < m_MountNotes.size() ? m_MountNotes[d] : std::string()) + " [" + disc.m_Root + "]");
		}
	}

	// The profile's choice among the discs still usable for this request.
	const C_Entry* C_CatalogFs::Choose(const Sys::C_FilePath& path, const std::vector<bool>& usable) const
	{
		return m_Profile.Choose(m_Catalog, path.Chars(), usable);
	}

	// The disc the profile serves 'path' from: the first candidate whose chain (Kanzen
	// layer, converters) has the file. -1 when none has.
	int C_CatalogFs::DiscFor(const Sys::C_FilePath& path, const C_Entry** ppEntry)
	{
		std::string levelDir;
		if (!m_RawMode && LevelDirOf(path.Chars(), levelDir))
		{
			// a level file: only the disc that serves the level whole
			const int d = LevelDisc(levelDir);
			if (ppEntry)
				*ppEntry = NULL;
			return d >= 0 && m_ServeFs[d]->FileExists(path) ? d : -1;
		}
		std::vector<bool> usable = m_Mounted;
		for (;;)
		{
			const C_Entry* e = Choose(path, usable);
			if (ppEntry)
				*ppEntry = e;
			if (!e)
				return -1;
			if (m_ServeFs[e->m_Disc]->FileExists(path))
				return e->m_Disc;
			usable[e->m_Disc] = false;
		}
	}

	Sys::I_File* C_CatalogFs::OpenConverted(int d, const Sys::C_FilePath& filePath, const Sys::C_OpenParams& params, const C_Entry* e)
	{
		Sys::C_OpenParams quiet = params;
		quiet.m_IsCritical = false;
		// a refused copy (Kanzen layer) is skipped before any converter looks at it
		Sys::I_File* pFile = m_ServeFs[d]->FileExists(filePath) ? m_ServeFs[d]->Open(filePath, Sys::E_FileAccess_Read, quiet) : NULL;
		if (m_Trace)
			LOGINFO(g_catalogLog, "%s <- %s %s%s", filePath.Chars(), m_Catalog.Discs()[d].m_Id.c_str(),
				!e ? "level" : e->m_Container < 0 ? "loose" : m_Catalog.Archives()[e->m_Container].m_File.c_str(),
				pFile ? "" : " (not served: refused or not converted)");
		// Blur1Multi's converters hand back the original bytes when a conversion fails; a
		// chunk file that still carries the Xbox 360 platform tag was not converted.
		// (Only chunk-file types the converters rewrite in memory: other files come straight
		// from a pak, where seeking back is a physical seek in a compressed entry.)
		if (pFile && !m_RawMode && pFile->Length() >= 4 && HasPlatformTag(filePath.Chars()))
		{
			char tag[4] = { 0 };
			const Sys::C_StreamPos at = pFile->GetPosition();
			pFile->Read(tag, 4);
			pFile->SetPosition(at);
			if (memcmp(tag, "360 ", 4) == 0)
			{
				LOGWARN(g_catalogLog, "%s from %s did not convert", filePath.Chars(), m_Catalog.Discs()[d].m_Id.c_str());
				delete pFile;
				pFile = NULL;
			}
		}
		if (pFile)
			++m_Served[d];
		return pFile;
	}

	// The disc the level directory 'dir' is served from, decided once: the profile's
	// first choice for its level.levelrender whose .level, .levelphysics and
	// .levelrender all convert (the render file's conversion prepares the scene and
	// the stream layout, so the .levelstream of that disc is convertible too).
	int C_CatalogFs::LevelDisc(const std::string& dir)
	{
		std::lock_guard<std::mutex> lock(m_LevelLock);
		std::map<std::string, int>::iterator it = m_LevelDisc.find(dir);
		if (it != m_LevelDisc.end())
			return it->second;
		int chosen = -1;
		std::vector<bool> usable = m_Mounted;
		const Sys::C_FilePath render((dir + kLevelFiles[2]).c_str());
		for (;;)
		{
			const C_Entry* e = Choose(render, usable);
			if (!e)
				break;
			const int d = e->m_Disc;
			bool ok = true;
			for (size_t i = 0; ok && i < sizeof kLevelFiles / sizeof kLevelFiles[0]; ++i)
			{
				Sys::I_File* pFile = OpenConverted(d, Sys::C_FilePath((dir + kLevelFiles[i]).c_str()), Sys::C_OpenParams(), e);
				ok = pFile != NULL;
				delete pFile;
			}
			if (ok)
			{
				chosen = d;
				break;
			}
			LOGWARN(g_catalogLog, "level %s from %s does not convert whole; trying the profile's next disc", dir.c_str(), m_Catalog.Discs()[d].m_Id.c_str());
			usable[d] = false;
		}
		if (chosen >= 0)
			LOGINFO(g_catalogLog, "level %s served whole from %s", dir.c_str(), m_Catalog.Discs()[chosen].m_Id.c_str());
		else
			LOGWARN(g_catalogLog, "level %s: no disc of the profile serves it", dir.c_str());
		m_LevelDisc[dir] = chosen;
		return chosen;
	}

	Sys::I_File* C_CatalogFs::Open(const Sys::C_FilePath& filePath, const Sys::E_FileAccess fileAccess, const Sys::C_OpenParams& params)
	{
		if (fileAccess != Sys::E_FileAccess_Read)
			return NULL;
		std::string levelDir;
		if (!m_RawMode && LevelDirOf(filePath.Chars(), levelDir))
		{
			const int d = LevelDisc(levelDir);
			return d >= 0 ? OpenConverted(d, filePath, params, NULL) : NULL;
		}
		// A copy that its disc's chain refuses or cannot convert leaves the next disc to serve it.
		std::vector<bool> usable = m_Mounted;
		for (;;)
		{
			const C_Entry* e = Choose(filePath, usable);
			if (!e)
				return NULL;
			Sys::I_File* pFile = OpenConverted(e->m_Disc, filePath, params, e);
			if (pFile)
				return pFile;
			usable[e->m_Disc] = false;
		}
	}

	bool C_CatalogFs::FileExists(const Sys::C_FilePath& filePath)
	{
		return DiscFor(filePath) >= 0;
	}

	bool C_CatalogFs::FileInfo(const Sys::C_FilePath& filePath, Sys::C_FileInfo* pInfo)
	{
		const int d = DiscFor(filePath);
		return d >= 0 && m_ServeFs[d]->FileInfo(filePath, pInfo);
	}

	bool C_CatalogFs::FileAccess(const Sys::C_FilePath& filePath, U32 fileAccess)
	{
		const int d = DiscFor(filePath);
		return d >= 0 && m_ServeFs[d]->FileAccess(filePath, fileAccess);
	}

	// The union of the directory on every mounted disc, each name listed once. A name is
	// kept only when the profile serves it from the disc that listed it, so the listing
	// matches what Open returns.
	bool C_CatalogFs::DirectoryInfo(const Sys::C_FilePath& filePath, Sys::C_DirectoryInfo* pDirInfo)
	{
		std::set<std::string> seen;
		Sys::FileInfoArr merged;
		bool any = false;
		for (size_t d = 0; d < m_DiscFs.size(); ++d)
		{
			if (!m_ServeFs[d])
				continue;
			Sys::C_DirectoryInfo info;
			if (!m_ServeFs[d]->DirectoryInfo(filePath, &info))
				continue;
			any = true;
			const Sys::FileInfoArr& arr = info.FileInfoArray();
			for (U32 i = 0; i < arr.Size(); ++i)
			{
				const Sys::C_FileInfo& fi = arr[i];
				const std::string key = Lower(fi.m_FileName.Chars());
				if (seen.count(key))
					continue;
				// files (C_OverlayFs lists full paths): only from the disc the profile serves them from
				if (fi.m_FileType != Sys::E_FileType_Directory && DiscFor(fi.m_FileName) != (int)d)
					continue;
				seen.insert(key);
				merged.PushBack(fi);
			}
		}
		if (!any)
			return false;
		pDirInfo->SetNumFiles((int)merged.Size());
		for (U32 i = 0; i < merged.Size(); ++i)
			pDirInfo->FileInfo(i) = merged[i];
		return true;
	}
}
