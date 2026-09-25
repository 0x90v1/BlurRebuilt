// [Blur1Multi] Xbox 360 audio banks through the per-source audio cache. See AudioCacheFs.h.

#include "StdAfx.h"

#include "AudioCacheFs.h"

#include <x360convert/XmaBank.h>

#include <windows.h>
#include <stdio.h>

namespace X360Convert
{
	namespace
	{
		Debug::Logger& g_audioLog = Debug::GetLogger("Blur1Multi.X360Convert");

		bool IsBank(const Sys::C_FilePath& path)
		{
			const char* p = path.Chars();
			const size_t n = p ? strlen(p) : 0;
			return n > 4 && _stricmp(p + n - 4, ".baf") == 0;
		}

		std::string BaseName(const char* path)
		{
			std::string s(path ? path : "");
			const size_t slash = s.find_last_of("\\/");
			if (slash != std::string::npos)
				s = s.substr(slash + 1);
			for (size_t i = 0; i < s.size(); ++i)
				s[i] = (char)tolower((unsigned char)s[i]);
			return s.size() > 4 ? s.substr(0, s.size() - 4) : s;
		}

		U64 Fnv(U64 h, const void* p, size_t n)
		{
			const unsigned char* b = (const unsigned char*)p;
			for (size_t i = 0; i < n; ++i)
				h = (h ^ b[i]) * 1099511628211ull;
			return h;
		}

		unsigned Be32(const unsigned char* p) { return (unsigned)p[0] << 24 | (unsigned)p[1] << 16 | (unsigned)p[2] << 8 | p[3]; }

		// The engine file of the 360 bank, freshly opened, read by offset. A bank in a
		// compressed pak decompresses from its start on every seek, so the reader goes
		// front to back: a forward gap is read and discarded, and the converter's
		// re-read of a row after its 8-byte chunk header comes from the last read kept
		// here. Only a jump further back seeks.
		class C_InnerReader : public I_XmaBankReader
		{
		public:
			explicit C_InnerReader(Sys::I_File* pFile) : m_pFile(pFile), m_position(0), m_lastAt(0) {}
			unsigned long long Size() { return m_pFile->Length(); }
			bool Read(unsigned long long offset, void* dst, size_t n)
			{
				unsigned char* out = (unsigned char*)dst;
				if (offset < m_position && offset >= m_lastAt && m_position - m_lastAt == m_last.size())
				{
					const size_t kept = (size_t)(m_position - offset);
					const size_t take = kept < n ? kept : n;
					memcpy(out, &m_last[(size_t)(offset - m_lastAt)], take);
					if (take == n)
						return true;
					out += take;
					offset += take;
					n -= take;
				}
				else if (offset < m_position)
				{
					m_pFile->SetPosition(offset);
					m_position = offset;
				}
				while (m_position < offset)
				{
					unsigned char skip[0x10000];
					const U32 k = (U32)(offset - m_position < sizeof(skip) ? offset - m_position : sizeof(skip));
					if (!m_pFile->Read(skip, k))
						return false;
					m_position += k;
				}
				if (n && !m_pFile->Read(out, (U32)n))
					return false;
				if (n <= 0x10000)
					m_last.assign(out, out + n);
				else
					m_last.clear();
				m_lastAt = offset;
				m_position = offset + n;
				return true;
			}
		private:
			Sys::I_File* m_pFile;
			unsigned long long m_position;
			unsigned long long m_lastAt;
			std::vector<unsigned char> m_last;
		};

		class C_FileWriter : public I_XmaBankWriter
		{
		public:
			explicit C_FileWriter(FILE* f) : m_f(f) {}
			bool Write(const void* p, size_t n) { return fwrite(p, 1, n, m_f) == n; }
			bool WriteAt(unsigned long long o, const void* p, size_t n)
			{
				const long long back = _ftelli64(m_f);
				const bool ok = _fseeki64(m_f, (long long)o, SEEK_SET) == 0 && fwrite(p, 1, n, m_f) == n;
				return _fseeki64(m_f, back, SEEK_SET) == 0 && ok;
			}
			unsigned long long Position() { return (unsigned long long)_ftelli64(m_f); }
		private:
			FILE* m_f;
		};

		bool Exists(const std::string& path)
		{
			const DWORD a = GetFileAttributesA(path.c_str());
			return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
		}
	}

	C_AudioCacheFs::C_AudioCacheFs(Sys::I_FileSystem* pInner, Sys::I_FileSystem& nativeFs, const std::string& source)
		: m_pInner(pInner), m_Native(nativeFs)
	{
		char exe[MAX_PATH];
		GetModuleFileNameA(NULL, exe, MAX_PATH);
		std::string dir(exe);
		dir = dir.substr(0, dir.find_last_of("\\/") + 1) + "audiocache";
		::CreateDirectoryA(dir.c_str(), NULL);
		dir += "\\" + source;
		::CreateDirectoryA(dir.c_str(), NULL);
		m_Dir = dir + "\\";
	}

	std::string C_AudioCacheFs::CachedBank(const Sys::C_FilePath& path, const Sys::C_OpenParams& params)
	{
		Sys::I_File* pFile = m_pInner->Open(path, Sys::E_FileAccess_Read, params);
		if (!pFile)
			return std::string();
		const U64 size = pFile->Length();
		unsigned char head[0x34];
		if (size < sizeof(head) || !pFile->Read(head, sizeof(head)) || !IsXmaBank(head, sizeof(head)))
		{
			delete pFile;
			return std::string();
		}

		// key: the converter's output version, the size and the header region (rows and
		// CUE chunks up to the first data). Raise the version when the output changes.
		static const char kVersion[] = "xma->adpcm 2";
		U64 key = Fnv(14695981039346656037ull, kVersion, sizeof(kVersion));
		key = Fnv(key, &size, sizeof(size));
		key = Fnv(key, head, sizeof(head));
		const unsigned waves = Be32(head + 0x0C);
		U64 at = sizeof(head);
		std::vector<unsigned char> chunk;
		for (unsigned rows = 0; rows < waves;)
		{
			unsigned char h[8];
			if (at + 8 > size || !pFile->Read(h, 8))
				break;
			const unsigned n = Be32(h + 4);
			if (n < 8 || at + n > size)
				break;
			chunk.resize(n - 8);
			if (!chunk.empty() && !pFile->Read(&chunk[0], (U32)chunk.size()))
				break;
			key = Fnv(key, h, 8);
			if (!chunk.empty())
				key = Fnv(key, &chunk[0], chunk.size());
			if (memcmp(h, "WAVE", 4) == 0)
				++rows;
			at += n;
		}
		char suffix[32];
		sprintf_s(suffix, ".%016llx.baf", (unsigned long long)key);
		const std::string cache = m_Dir + BaseName(path.Chars()) + suffix;

		std::lock_guard<std::mutex> lock(m_Lock);
		if (Exists(cache))
		{
			delete pFile;
			return cache;
		}

		// convert once, into a temporary file renamed into place, reading the bank
		// again from its start
		delete pFile;
		pFile = m_pInner->Open(path, Sys::E_FileAccess_Read, params);
		if (!pFile)
			return std::string();
		const std::string temp = cache + ".tmp";
		FILE* f = NULL;
		if (fopen_s(&f, temp.c_str(), "w+b") || !f)
		{
			LOGERROR(g_audioLog, "X360Convert: cannot write %s", temp.c_str());
			delete pFile;
			return std::string();
		}
		const DWORD t0 = GetTickCount();
		C_InnerReader reader(pFile);
		C_FileWriter writer(f);
		std::string error;
		XmaBankStats stats;
		const E_Result r = ConvertXmaBankStream(reader, writer, error, &stats);
		const unsigned long long written = writer.Position();
		fclose(f);
		delete pFile;
		if (r == E_Result_NotHandled)
			error = "not read as a big-endian BANK v4/v5";
		else if (r == E_Result_Converted && !MoveFileExA(temp.c_str(), cache.c_str(), MOVEFILE_REPLACE_EXISTING))
			error = "cannot rename the temporary file";
		if (!error.empty())
		{
			DeleteFileA(temp.c_str());
			LOGERROR(g_audioLog, "X360Convert: cannot convert audio bank %s: %s", path.Chars(), error.c_str());
			return std::string();
		}
		LOGINFO(g_audioLog, "X360Convert: audio bank %s converted (%u waves, %u XMA decoded, %u carried, %u partial loops, %u CUE dropped, %llu -> %llu bytes, %u ms) into %s",
			path.Chars(), stats.waves, stats.xmaWaves, stats.carried, stats.partialLoops, stats.cues, (unsigned long long)size, written, (unsigned)(GetTickCount() - t0), cache.c_str());
		return cache;
	}

	Sys::I_File* C_AudioCacheFs::Open(const Sys::C_FilePath& filePath, const Sys::E_FileAccess fileAccess, const Sys::C_OpenParams& params)
	{
		if (fileAccess != Sys::E_FileAccess_Read || !IsBank(filePath))
			return m_pInner->Open(filePath, fileAccess, params);
		const std::string cache = CachedBank(filePath, params);
		if (!cache.empty())
		{
			Sys::I_File* pCached = m_Native.Open(Sys::C_FilePath(cache.c_str()), Sys::E_FileAccess_Read, params);
			if (pCached)
				return pCached;
			LOGERROR(g_audioLog, "X360Convert: cannot open %s", cache.c_str());
		}
		return m_pInner->Open(filePath, fileAccess, params);
	}

	bool C_AudioCacheFs::FileInfo(const Sys::C_FilePath& filePath, Sys::C_FileInfo* pInfo)
	{
		if (!m_pInner->FileInfo(filePath, pInfo))
			return false;
		if (!pInfo || pInfo->m_FileType != Sys::E_FileType_File || !IsBank(filePath))
			return true;
		const std::string cache = CachedBank(filePath, Sys::C_OpenParams());
		if (!cache.empty())
		{
			WIN32_FILE_ATTRIBUTE_DATA a;
			if (GetFileAttributesExA(cache.c_str(), GetFileExInfoStandard, &a))
				pInfo->m_FileSize = ((U64)a.nFileSizeHigh << 32) | a.nFileSizeLow;
		}
		return true;
	}
}
