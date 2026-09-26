// [BlurX] Asset catalog: every file on every disc the BlurX build can serve, and
// the policy that picks, per file, the disc that serves it (AGENTS.md "Asset
// catalog", docs/decisions.md D12). No engine dependencies: the file-system
// layer that serves the engine (engine/CatalogFs) and the verifiers use it.
//
// The catalog's contents are built off line by
// code/tools/blurx_catalog/catalog.py from the discs (BlurX/catalog/catalog.tsv);
// the policy is data too (BlurX/catalog/profiles.txt, plus an optional local
// overrides file), so neither needs a rebuild to change. catalog.py `resolve`
// runs the same policy as C_Profile::Choose.
#pragma once

#include <string>
#include <vector>

namespace AssetCatalog
{
	// The engine's pak lookup hash of a virtual path (LogicalFs C_PakCrc32 over
	// the localised path): CRC-32 with init 0xFFFFFFFF and no final XOR over the
	// lower-case path with backslash separators, a leading backslash and no
	// trailing one. Pak records carry this hash of their own path, so a row of
	// the catalog can be keyed exactly even where a disc kept only a leaf name.
	unsigned PathHash(const char* path);

	// "levels/x/Y.level" -> "\levels\x\y.level"
	std::string NormalisePath(const char* path);

	struct C_Disc
	{
		std::string	m_Id;			// nov10, aug31, blur1x360, ...
		std::string	m_Generation;	// A (Blur 1 layout) or B (Kanzen layout)
		std::string	m_Game;			// blur1 / blur2
		bool		m_Damaged;
		std::string	m_Root;			// directory on this machine (catalog value, or overridden)
		std::string	m_Cache;		// paks' folder, relative to m_Root
		std::string	m_Title;
		std::vector<std::string>	m_Layers;	// game folders overlaid at / as media layer 1 (retail X360: layer1)
	};

	struct C_Archive
	{
		int			m_Disc;			// index into Discs()
		std::string	m_File;			// root.pak, ...
		std::string	m_Mount;		// where the engine sees it ("/", "/gamedata/", ...)
		std::string	m_Src;			// path inside the pak that is mounted there
		int			m_Files;
		bool		m_Ok;			// its tables parsed
	};

	struct C_Entry
	{
		unsigned	m_Hash;
		int			m_Disc;
		int			m_Container;	// index into Archives(), -1 = loose file on the disc
		int			m_Record;		// pak record index, -1 for a loose file
		unsigned	m_Size;			// decompressed size
		bool		m_Ok;			// status "ok"
		std::string	m_Status;		// ok, suspect:<why>, bad:<why>
		std::string	m_Path;			// full virtual path, or "?<leaf>" where no disc names it
	};

	class C_Catalog
	{
	public:
		// Reads catalog.tsv; false with 'error' set when the file is missing or malformed.
		bool Load(const char* file, std::string& error);

		const std::vector<C_Disc>& Discs() const { return m_Discs; }
		std::vector<C_Disc>& Discs() { return m_Discs; }
		const std::vector<C_Archive>& Archives() const { return m_Archives; }
		int FindDisc(const std::string& id) const;

		// Every disc's copy of the file with this hash (entries sorted by hash).
		void Find(unsigned hash, const C_Entry*& first, const C_Entry*& last) const;
		size_t NumEntries() const { return m_Entries.size(); }

	private:
		std::vector<C_Disc>		m_Discs;
		std::vector<C_Archive>	m_Archives;
		std::vector<C_Entry>	m_Entries;
	};

	// A serving profile from profiles.txt (format in that file). An overrides
	// file in the same syntax is applied on top: a profile it names gets its
	// 'order' and 'accept' replaced when given, and its rules placed before the
	// profile's own; a profile it defines anew is added.
	class C_Profile
	{
	public:
		C_Profile() : m_Found(false) {}

		// Loads profile 'name' from 'file', then 'overridesFile' (may be NULL or
		// missing). False with 'error' set when the profile is not defined.
		bool Load(const char* file, const char* overridesFile, const char* name, std::string& error);

		const std::string& Name() const { return m_Name; }
		const std::string& Description() const { return m_Description; }
		const std::vector<std::string>& Order() const { return m_Order; }

		// The discs that may serve 'normalisedPath', in preference order.
		void Candidates(const std::string& normalisedPath, std::vector<std::string>& out) const;
		bool Accepts(const C_Entry& e) const;

		// The entry the profile serves the file from, or NULL. 'isMounted' says
		// whether a disc index is available on this machine.
		const C_Entry* Choose(const C_Catalog& catalog, const char* path, const std::vector<bool>& isMounted) const;

	private:
		struct C_Rule
		{
			bool						m_Pin;
			std::string					m_Glob;
			std::vector<std::string>	m_Discs;
		};

		bool Parse(const char* file, const char* name, bool overrides, std::string& error);

		std::string					m_Name;
		std::string					m_Description;
		std::vector<std::string>	m_Order;
		std::vector<C_Rule>			m_Rules;
		std::vector<std::string>	m_Accept;
		bool						m_Found;
	};

	// '*' and '?' wildcard match (both strings already normalised).
	bool GlobMatch(const char* glob, const char* text);
}
