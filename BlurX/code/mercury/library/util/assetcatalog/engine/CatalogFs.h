// [BlurX] File-system layer that serves the game from several discs at once,
// choosing per file the disc the asset catalog and the active profile name
// (docs/decisions.md D12). Each disc of the profile is mounted as the engine
// would mount it on its own (loose files, then its paks at their mount points,
// in the engine's order) into a private LogicalFs::C_OverlayFs; this layer
// sends each request to one of them. Read-only: writes go to the layers below
// it in the game's overlay file system.
//
// Each disc is served through its own chain: its mounts, then for a Generation B
// (Kanzen) disc the Kanzen layer (KanzenFs.h: normalises Kanzen layouts, refuses
// what has no converter yet), then Blur1Multi's Xbox 360 converters, so this
// layer hands out PC data. A copy its chain refuses or cannot convert is not
// servable: the next disc the profile names serves the file. (In catalog mode
// the game has no other converting layer.)
//
// A level (levels\<name>\level.level, .levelphysics, .levelrender, .levelstream)
// is served whole from one disc: the render file's scene names the stream's
// chunks and the physics matches the geometry, so the files of one level never
// mix discs. The disc is the profile's first choice for the level directory
// whose .level, .levelphysics and .levelrender all convert (T030); a level none
// converts does not exist.
//
// Selection (all optional but the profile):
//   --profile=<name> | BLURX_PROFILE            catalog mode; profile from profiles.txt
//   --catalog=<dir> | BLURX_CATALOG             holds catalog.tsv and profiles.txt
//                                               (default: <exe>\catalog, then <exe>\..\..\catalog)
//   --profile-overrides=<file> | BLURX_PROFILE_OVERRIDES
//                                               (default: <catalog dir>\profiles.local.txt)
//   --disc=<id>=<dir> | BLURX_DISC_<ID>=<dir>   where a disc is on this machine
//                                               (default: the root the catalog was built from)
//   BLURX_CATALOG_TRACE=1                       log every file the layer serves and its disc
#pragma once

#include <system/serialise/I_FileSystem.h>
#include <assetcatalog/AssetCatalog.h>

#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace LogicalFs { class C_OverlayFs; }

namespace AssetCatalog
{
	class C_KanzenFs;

	class C_CatalogFs : public Sys::I_FileSystem
	{
	public:
		// True when catalog mode was asked for (a profile is named).
		static bool Requested();

		// Loads the catalog and the profile and mounts the profile's discs.
		// NULL with 'error' set on failure.
		static C_CatalogFs* Create(Sys::I_FileSystem& nativeFs, std::string& error);

		virtual ~C_CatalogFs();

		virtual Sys::I_File* Open(const Sys::C_FilePath& filePath, const Sys::E_FileAccess fileAccess = Sys::E_FileAccess_Read,
			const Sys::C_OpenParams& params = Sys::C_OpenParams());
		virtual bool FileExists(const Sys::C_FilePath& filePath);
		virtual bool FileInfo(const Sys::C_FilePath& filePath, Sys::C_FileInfo* pInfo);
		virtual bool FileAccess(const Sys::C_FilePath& filePath, U32 fileAccess);
		virtual bool DirectoryInfo(const Sys::C_FilePath& filePath, Sys::C_DirectoryInfo* pDirInfo);
		virtual Sys::C_FilePath GetSystemFilePath(const Sys::C_FilePath& filePath) const { return filePath; }

		// One line per disc: id, root, mounted or why not.
		void Describe(std::vector<std::string>& lines) const;
		const C_Profile& Profile() const { return m_Profile; }
		const C_Catalog& Catalog() const { return m_Catalog; }

		// The disc that serves 'path' (-1: none) and the catalog row behind it.
		int DiscFor(const Sys::C_FilePath& path, const C_Entry** ppEntry = NULL);

	private:
		C_CatalogFs() {}
		bool Mount(Sys::I_FileSystem& nativeFs, int disc, std::string& why);
		const C_Entry* Choose(const Sys::C_FilePath& path, const std::vector<bool>& usable) const;
		// Opens 'path' through disc 'd' (NULL when it has no converted copy: refused,
		// or the converters handed back the Xbox 360 bytes).
		Sys::I_File* OpenConverted(int d, const Sys::C_FilePath& path, const Sys::C_OpenParams& params, const C_Entry* e);
		// The disc that serves the level directory 'dir' ("\levels\<name>\") whole; -1 when none.
		int LevelDisc(const std::string& dir);

		C_Catalog							m_Catalog;
		C_Profile							m_Profile;
		std::vector<LogicalFs::C_OverlayFs*>	m_DiscFs;		// per catalog disc: its own mounts (NULL = not mounted)
		std::vector<C_KanzenFs*>			m_KanzenFs;		// Generation B discs: the Kanzen layer over them
		std::vector<Sys::I_FileSystem*>		m_ConvertFs;	// Blur1Multi's converters over the above
		std::vector<Sys::I_FileSystem*>		m_ServeFs;		// what is served: the audio cache over the converters (T075)
		std::vector<bool>					m_Mounted;
		std::vector<std::string>			m_MountNotes;
		std::vector<unsigned>				m_Served;		// files opened per disc
		bool								m_Trace;
		bool								m_RawMode;		// BLURX_DUMP_RAW: discs' own bytes, no Kanzen layer or converters
		std::map<std::string, int>			m_LevelDisc;	// level directory -> the disc serving it whole (-1: none)
		std::mutex							m_LevelLock;	// guards m_LevelDisc and its resolution

	};
}
