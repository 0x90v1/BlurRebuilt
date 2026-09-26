// [BlurX] Asset catalog and serving profiles. See AssetCatalog.h.

#include <assetcatalog/AssetCatalog.h>

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

namespace AssetCatalog
{
	namespace
	{
		unsigned g_crcTable[256];
		bool g_crcReady = false;

		void InitCrc()
		{
			for (unsigned i = 0; i < 256; ++i)
			{
				unsigned c = i;
				for (int k = 0; k < 8; ++k)
					c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
				g_crcTable[i] = c;
			}
			g_crcReady = true;
		}

		void Split(const std::string& line, char sep, std::vector<std::string>& out)
		{
			out.clear();
			size_t start = 0;
			for (;;)
			{
				const size_t at = line.find(sep, start);
				out.push_back(line.substr(start, at == std::string::npos ? std::string::npos : at - start));
				if (at == std::string::npos)
					return;
				start = at + 1;
			}
		}

		void Words(const std::string& line, std::vector<std::string>& out)
		{
			out.clear();
			size_t i = 0;
			while (i < line.size())
			{
				while (i < line.size() && isspace((unsigned char)line[i]))
					++i;
				size_t j = i;
				while (j < line.size() && !isspace((unsigned char)line[j]))
					++j;
				if (j > i)
					out.push_back(line.substr(i, j - i));
				i = j;
			}
		}

		bool ReadLines(const char* file, std::vector<std::string>& lines)
		{
			FILE* f = fopen(file, "rb");
			if (!f)
				return false;
			std::string cur;
			char buf[65536];
			size_t n;
			while ((n = fread(buf, 1, sizeof buf, f)) > 0)
			{
				for (size_t i = 0; i < n; ++i)
				{
					if (buf[i] == '\n')
					{
						if (!cur.empty() && cur[cur.size() - 1] == '\r')
							cur.erase(cur.size() - 1);
						lines.push_back(cur);
						cur.clear();
					}
					else
						cur += buf[i];
				}
			}
			if (!cur.empty())
				lines.push_back(cur);
			fclose(f);
			return true;
		}

		bool EntryLess(const C_Entry& a, const C_Entry& b) { return a.m_Hash < b.m_Hash; }
	}

	std::string NormalisePath(const char* path)
	{
		std::string p;
		p.reserve(strlen(path) + 1);
		for (const char* s = path; *s; ++s)
		{
			char c = *s == '/' ? '\\' : (char)tolower((unsigned char)*s);
			if (c == '\\' && !p.empty() && p[p.size() - 1] == '\\')
				continue;
			p += c;
		}
		if (p.empty() || p[0] != '\\')
			p.insert(p.begin(), '\\');
		if (p.size() > 1 && p[p.size() - 1] == '\\')
			p.erase(p.size() - 1);
		return p;
	}

	unsigned PathHash(const char* path)
	{
		if (!g_crcReady)
			InitCrc();
		const std::string p = NormalisePath(path);
		unsigned crc = 0xFFFFFFFFu;
		for (size_t i = 0; i < p.size(); ++i)
			crc = g_crcTable[(crc ^ (unsigned char)p[i]) & 0xFF] ^ (crc >> 8);
		return crc;
	}

	bool GlobMatch(const char* glob, const char* text)
	{
		// iterative '*' matching with backtracking to the last star
		const char* star = NULL;
		const char* resume = NULL;
		while (*text)
		{
			if (*glob == '*')
			{
				star = glob++;
				resume = text;
			}
			else if (*glob == '?' || *glob == *text)
			{
				++glob;
				++text;
			}
			else if (star)
			{
				glob = star + 1;
				text = ++resume;
			}
			else
				return false;
		}
		while (*glob == '*')
			++glob;
		return *glob == 0;
	}

	// ---- catalog ----------------------------------------------------------------

	int C_Catalog::FindDisc(const std::string& id) const
	{
		for (size_t i = 0; i < m_Discs.size(); ++i)
			if (m_Discs[i].m_Id == id)
				return (int)i;
		return -1;
	}

	bool C_Catalog::Load(const char* file, std::string& error)
	{
		std::vector<std::string> lines;
		if (!ReadLines(file, lines))
		{
			error = std::string("cannot read ") + file;
			return false;
		}
		m_Discs.clear();
		m_Archives.clear();
		m_Entries.clear();
		m_Entries.reserve(lines.size());
		std::vector<std::string> f;
		for (size_t n = 0; n < lines.size(); ++n)
		{
			const std::string& line = lines[n];
			if (line.empty() || line[0] == '#')
				continue;
			Split(line, '\t', f);
			char where[64];
			sprintf(where, "%s line %u: ", file, (unsigned)(n + 1));
			if (f[0] == "@disc")
			{
				if (f.size() < 8) { error = std::string(where) + "short @disc"; return false; }
				C_Disc d;
				d.m_Id = f[1]; d.m_Generation = f[2]; d.m_Game = f[3]; d.m_Damaged = f[4] == "1";
				d.m_Root = f[5]; d.m_Cache = f[6]; d.m_Title = f[7];
				if (f.size() > 8 && f[8] != "-")
					Split(f[8], ',', d.m_Layers);
				m_Discs.push_back(d);
			}
			else if (f[0] == "@pak")
			{
				if (f.size() < 9) { error = std::string(where) + "short @pak"; return false; }
				C_Archive a;
				a.m_Disc = FindDisc(f[1]);
				a.m_File = f[2]; a.m_Mount = f[5]; a.m_Src = f[6];
				a.m_Files = atoi(f[7].c_str());
				a.m_Ok = f[8] == "ok";
				if (a.m_Disc < 0) { error = std::string(where) + "unknown disc " + f[1]; return false; }
				m_Archives.push_back(a);
			}
			else
			{
				if (f.size() < 7) { error = std::string(where) + "short row"; return false; }
				C_Entry e;
				e.m_Hash = (unsigned)strtoul(f[0].c_str(), NULL, 16);
				e.m_Disc = FindDisc(f[1]);
				if (e.m_Disc < 0) { error = std::string(where) + "unknown disc " + f[1]; return false; }
				e.m_Container = -1;
				if (f[2] != "loose")
				{
					for (size_t i = 0; i < m_Archives.size(); ++i)
						if (m_Archives[i].m_Disc == e.m_Disc && m_Archives[i].m_File == f[2])
							e.m_Container = (int)i;
					if (e.m_Container < 0) { error = std::string(where) + "unknown archive " + f[2]; return false; }
				}
				e.m_Record = f[3] == "-" ? -1 : atoi(f[3].c_str());
				e.m_Size = (unsigned)strtoul(f[4].c_str(), NULL, 10);
				e.m_Status = f[5];
				e.m_Ok = f[5] == "ok";
				e.m_Path = f[6];
				m_Entries.push_back(e);
			}
		}
		std::stable_sort(m_Entries.begin(), m_Entries.end(), EntryLess);
		if (m_Discs.empty())
		{
			error = std::string(file) + ": no @disc lines";
			return false;
		}
		return true;
	}

	void C_Catalog::Find(unsigned hash, const C_Entry*& first, const C_Entry*& last) const
	{
		C_Entry key;
		key.m_Hash = hash;
		std::vector<C_Entry>::const_iterator lo = std::lower_bound(m_Entries.begin(), m_Entries.end(), key, EntryLess);
		std::vector<C_Entry>::const_iterator hi = std::upper_bound(lo, m_Entries.end(), key, EntryLess);
		first = m_Entries.empty() ? NULL : &*m_Entries.begin() + (lo - m_Entries.begin());
		last = m_Entries.empty() ? NULL : &*m_Entries.begin() + (hi - m_Entries.begin());
	}

	// ---- profiles ---------------------------------------------------------------

	bool C_Profile::Parse(const char* file, const char* name, bool overrides, std::string& error)
	{
		std::vector<std::string> lines;
		if (!ReadLines(file, lines))
		{
			if (overrides)
				return true;	// an overrides file is optional
			error = std::string("cannot read ") + file;
			return false;
		}
		bool inside = false;
		std::vector<C_Rule> newRules;
		std::vector<std::string> w;
		for (size_t n = 0; n < lines.size(); ++n)
		{
			std::string line = lines[n];
			const size_t hash = line.find('#');
			if (hash != std::string::npos)
				line.erase(hash);
			Words(line, w);
			if (w.empty())
				continue;
			if (w[0] == "profile")
			{
				inside = w.size() > 1 && w[1] == name;
				if (inside)
				{
					m_Found = true;
					m_Name = w[1];
					std::string d;
					for (size_t i = 2; i < w.size(); ++i)
						d += (i > 2 ? " " : "") + w[i];
					if (!d.empty() || !overrides)
						m_Description = d;
				}
				continue;
			}
			if (!inside)
				continue;
			if (w[0] == "order")
				m_Order.assign(w.begin() + 1, w.end());
			else if (w[0] == "accept")
				m_Accept.assign(w.begin() + 1, w.end());
			else if ((w[0] == "prefer" || w[0] == "pin") && w.size() >= 3)
			{
				C_Rule r;
				r.m_Pin = w[0] == "pin";
				r.m_Glob = w[1];
				for (size_t i = 0; i < r.m_Glob.size(); ++i)
					r.m_Glob[i] = r.m_Glob[i] == '/' ? '\\' : (char)tolower((unsigned char)r.m_Glob[i]);
				r.m_Discs.assign(w.begin() + 2, w.end());
				newRules.push_back(r);
			}
			else
			{
				char where[32];
				sprintf(where, " line %u", (unsigned)(n + 1));
				error = std::string(file) + where + ": unknown keyword '" + w[0] + "'";
				return false;
			}
		}
		// an override's rules come before the profile's own
		m_Rules.insert(overrides ? m_Rules.begin() : m_Rules.end(), newRules.begin(), newRules.end());
		return true;
	}

	bool C_Profile::Load(const char* file, const char* overridesFile, const char* name, std::string& error)
	{
		m_Name.clear();
		m_Description.clear();
		m_Order.clear();
		m_Rules.clear();
		m_Accept.assign(1, "ok");
		m_Found = false;
		if (!Parse(file, name, false, error))
			return false;
		if (overridesFile && *overridesFile && !Parse(overridesFile, name, true, error))
			return false;
		if (!m_Found)
		{
			error = std::string("profile '") + name + "' is not defined in " + file +
				(overridesFile && *overridesFile ? std::string(" or ") + overridesFile : std::string());
			return false;
		}
		return true;
	}

	void C_Profile::Candidates(const std::string& p, std::vector<std::string>& out) const
	{
		out.clear();
		for (size_t i = 0; i < m_Rules.size(); ++i)
		{
			const C_Rule& r = m_Rules[i];
			if (!GlobMatch(r.m_Glob.c_str(), p.c_str()))
				continue;
			out = r.m_Discs;
			if (!r.m_Pin)
				for (size_t j = 0; j < m_Order.size(); ++j)
					if (std::find(out.begin(), out.end(), m_Order[j]) == out.end())
						out.push_back(m_Order[j]);
			return;
		}
		out = m_Order;
	}

	bool C_Profile::Accepts(const C_Entry& e) const
	{
		for (size_t i = 0; i < m_Accept.size(); ++i)
		{
			const std::string& a = m_Accept[i];
			if (e.m_Status == a || (e.m_Status.size() > a.size() && e.m_Status.compare(0, a.size(), a) == 0 && e.m_Status[a.size()] == ':'))
				return true;
		}
		return false;
	}

	const C_Entry* C_Profile::Choose(const C_Catalog& catalog, const char* path, const std::vector<bool>& isMounted) const
	{
		const std::string p = NormalisePath(path);
		const C_Entry* first;
		const C_Entry* last;
		catalog.Find(PathHash(p.c_str()), first, last);
		if (first == last)
			return NULL;
		std::vector<std::string> discs;
		Candidates(p, discs);
		for (size_t i = 0; i < discs.size(); ++i)
		{
			const int d = catalog.FindDisc(discs[i]);
			if (d < 0 || d >= (int)isMounted.size() || !isMounted[d])
				continue;
			// A disc may hold the file in several containers (a pak and a loose copy); its own
			// mount order picks which one it serves, so the disc qualifies only when every copy
			// it holds is acceptable.
			const C_Entry* found = NULL;
			bool allAccepted = true;
			for (const C_Entry* e = first; e != last; ++e)
				if (e->m_Disc == d)
				{
					if (!found)
						found = e;
					allAccepted = allAccepted && Accepts(*e);
				}
			if (found && allAccepted)
				return found;
		}
		return NULL;
	}
}
