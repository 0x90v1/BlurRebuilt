// [Blur1Multi] File-system layer serving Xbox 360 assets in PC format. See X360ConvertFs.h.

#include "StdAfx.h"

#include "X360ConvertFs.h"

#include <system/serialise/I_File.h>
#include <MemoryFs/Impl/MemoryFile.h>
#include <x360convert/X360Convert.h>
#include "EngineConverters.h"
#include "LevelTranscode.h"
#include "../source/ChunkFile.h"

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace X360Convert
{
	namespace
	{
		Debug::Logger& g_convertLog = Debug::GetLogger("Blur1Multi.X360Convert");

		// A read-only memory file that owns its bytes.
		class C_OwnedMemoryFile : public LogicalFs::C_MemoryFile
		{
		public:
			explicit C_OwnedMemoryFile(const std::vector<unsigned char>& data) : m_data(data) {}
			bool OpenOwned(const Btl::String& name, const Sys::C_OpenParams& params)
			{
				static unsigned char empty = 0;
				return Open(m_data.empty() ? &empty : &m_data[0], (U32)m_data.size(), name, params);
			}
		private:
			std::vector<unsigned char> m_data;
		};

		std::string Key(const Sys::C_FilePath& p)
		{
			std::string k = p.Chars();
			for (size_t i = 0; i < k.size(); ++i)
			{
				char c = k[i];
				if (c == '/')
					c = '\\';
				k[i] = (char)tolower((unsigned char)c);
			}
			if (!k.empty() && k[0] == '\\')
				k.erase(0, 1);
			return k;
		}

		bool EndsWith(const std::string& s, const char* ext)
		{
			const size_t m = strlen(ext);
			return s.size() >= m && s.compare(s.size() - m, m, ext) == 0;
		}

		// The level directory of a level file key ("levels\showroom\level.levelrender" -> "levels\showroom\").
		std::string LevelDir(const std::string& key)
		{
			const size_t slash = key.find_last_of('\\');
			return slash == std::string::npos ? std::string() : key.substr(0, slash + 1);
		}

		bool ReadWhole(Sys::I_FileSystem& fs, const Sys::C_FilePath& path, std::vector<unsigned char>& out)
		{
			Sys::C_OpenParams params;
			params.m_IsCritical = false;
			Sys::I_File* pFile = fs.Open(path, Sys::E_FileAccess_Read, params);
			if (!pFile)
				return false;
			out.resize((size_t)pFile->Length());
			const bool read = out.empty() || pFile->Read(&out[0], (U32)out.size());
			delete pFile;
			return read;
		}

		// Console effects (.fxb) are recompiled once and kept beside the executable
		// (decisions D03, D11): <exe dir>\shadercache\<x360|ps3>\<name>.<hash>.fxb, the
		// hash taken over the console file and the converter tag, so a different disc's
		// effect of the same name, or a converter change, gets its own entry.
		const char* ShaderConverterTag()
		{
			return GetSource() == E_Source_PS3 ? "rsx-hlsl-d3d9 2" : "xenosrecomp-d3d9 1";
		}

		unsigned long long Fnv1a64(const unsigned char* p, size_t n, unsigned long long h = 0xcbf29ce484222325ull)
		{
			for (size_t i = 0; i < n; ++i)
			{
				h ^= p[i];
				h *= 0x100000001b3ull;
			}
			return h;
		}

		std::string ShaderCacheDir()
		{
			char exe[MAX_PATH];
			const DWORD n = GetModuleFileNameA(NULL, exe, MAX_PATH);
			std::string dir(exe, n);
			dir = dir.substr(0, dir.find_last_of("\\/") + 1) + "shadercache";
			CreateDirectoryA(dir.c_str(), NULL);
			dir += GetSource() == E_Source_PS3 ? "\\ps3" : "\\x360";
			CreateDirectoryA(dir.c_str(), NULL);
			return dir + "\\";
		}

		bool ReadDiskFile(const std::string& path, std::vector<unsigned char>& out)
		{
			HANDLE h = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			if (h == INVALID_HANDLE_VALUE)
				return false;
			LARGE_INTEGER size;
			bool ok = GetFileSizeEx(h, &size) != 0;
			if (ok)
			{
				out.resize((size_t)size.QuadPart);
				DWORD read = 0;
				ok = out.empty() || (ReadFile(h, &out[0], (DWORD)out.size(), &read, NULL) && read == out.size());
			}
			CloseHandle(h);
			return ok;
		}

		void WriteDiskFile(const std::string& path, const std::vector<unsigned char>& data)
		{
			const std::string temp = path + ".tmp";
			HANDLE h = CreateFileA(temp.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			if (h == INVALID_HANDLE_VALUE)
				return;
			DWORD written = 0;
			const bool ok = data.empty() || (WriteFile(h, &data[0], (DWORD)data.size(), &written, NULL) && written == data.size());
			CloseHandle(h);
			if (ok)
				MoveFileExA(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING);
			else
				DeleteFileA(temp.c_str());
		}

		// Reads the 360 effect through 'fs' and returns the PC effect from the cache,
		// converting it on a miss. Returns false when the file cannot be opened.
		bool LoadShader(Sys::I_FileSystem& fs, const Sys::C_FilePath& path, const std::string& key,
			const Sys::C_OpenParams& params, std::vector<unsigned char>& bytes)
		{
			std::vector<unsigned char> in;
			Sys::I_File* pFile = fs.Open(path, Sys::E_FileAccess_Read, params);
			if (!pFile)
				return false;
			in.resize((size_t)pFile->Length());
			const bool read = in.empty() || pFile->Read(&in[0], (U32)in.size());
			delete pFile;
			if (!read)
				return false;

			const unsigned long long hash = Fnv1a64(in.empty() ? NULL : &in[0], in.size(),
				Fnv1a64((const unsigned char*)ShaderConverterTag(), strlen(ShaderConverterTag())));
			std::string name = key.substr(key.find_last_of('\\') + 1);
			name = name.substr(0, name.size() - 4);
			char suffix[32];
			sprintf_s(suffix, ".%016llx.fxb", hash);
			const std::string cachePath = ShaderCacheDir() + name + suffix;
			if (ReadDiskFile(cachePath, bytes))
				return true;

			EffectStats stats;
			std::string error;
			const DWORD start = GetTickCount();
			const E_Result converted = GetSource() == E_Source_PS3
				? ConvertPs3Effect(in.empty() ? NULL : &in[0], in.size(), bytes, error, &stats)
				: ConvertEffect(in.empty() ? NULL : &in[0], in.size(), bytes, error, &stats);
			if (converted == E_Result_Converted)
			{
				WriteDiskFile(cachePath, bytes);
				LOGINFO(g_convertLog, "X360Convert: shader %s recompiled (%u programs, %u ms) into %s",
					name.c_str(), stats.compiled, GetTickCount() - start, cachePath.c_str());
				if (stats.unsupported)
					LOGWARN(g_convertLog, "X360Convert: shader %s: %s", name.c_str(), stats.firstUnsupported.c_str());
				return true;
			}
			LOGERROR(g_convertLog, "X360Convert: cannot convert %s: %s", path.Chars(), error.c_str());
			bytes.swap(in);
			return true;
		}

		// The 360 levelstream, read chunk by chunk (C_LevelConversion keeps it).
		class C_LevelStreamSource : public I_ChunkSource
		{
		public:
			explicit C_LevelStreamSource(Sys::I_File* pFile) : m_pFile(pFile) {}
			~C_LevelStreamSource() { delete m_pFile; }
			bool Read(unsigned long long offset, unsigned size, std::vector<unsigned char>& out)
			{
				if (!m_pFile || offset + size > m_pFile->Length())
					return false;
				out.resize(size);
				m_pFile->SetPosition(offset);
				return size == 0 || m_pFile->Read(&out[0], size);
			}
		private:
			Sys::I_File* m_pFile;
		};

		// The PC levelstream, converted chunk by chunk as it is read
		// (StreamManager's loading thread: SetPosition, then Read).
		class C_LevelStreamFile : public Sys::I_File
		{
		public:
			C_LevelStreamFile(const std::shared_ptr<C_LevelConversion>& level, const Btl::String& name)
				: m_level(level), m_name(name), m_position(0), m_version(0) {}

			const bool IsWriting() const { return false; }
			Btl::String Name() const { return m_name; }
			const bool GetEndianSwapFlag() const { return false; }
			void SetEndianSwapFlag(const bool) {}
			U32 GetVersionNumber() const { return m_version; }
			void SetVersionNumber(U32 v) { m_version = v; }
			const Sys::C_StreamPos GetPosition() const { return m_position; }
			const Sys::C_StreamSize Length() { return m_level->StreamLength(); }
			void SetPosition(const Sys::C_StreamPos p) { m_position = p; }
			bool Overflow() { return false; }

			bool Underflow(UnderflowContext& uc)
			{
				if (m_position + uc.readSize > m_level->StreamLength())
					return false;
				if (uc.readTarget)
				{
					std::string error;
					if (!m_level->ReadStream(m_position, uc.readTarget, uc.readSize, error))
					{
						LOGERROR(g_convertLog, "X360Convert: %s: %s", m_name.Chars(), error.c_str());
						return false;
					}
				}
				m_position += uc.readSize;
				uc.bytesRead = uc.readSize;
				return true;
			}

		private:
			std::shared_ptr<C_LevelConversion> m_level;
			Btl::String m_name;
			U64 m_position;
			U32 m_version;
		};

		// A PS3 streaming audio bank served in place: the header region comes from the
		// converted copy (BankPatch::headers), payload reads go to the file as stored with
		// the DATA chunk sizes and PCM samples swapped (PlanBank), so a streaming bank (up
		// to 328 MB, ingamemusic.baf) is never held in memory. Sizes do not change. The
		// inner file is only repositioned when a read does not continue where the last
		// one ended (a seek inside a compressed pak is slow and asserts).
		class C_BankFile : public Sys::I_File
		{
		public:
			C_BankFile(Sys::I_File* pInner, U64 innerPosition, const BankPatch& patch, const Btl::String& name)
				: m_pInner(pInner), m_innerPosition(innerPosition), m_patch(patch), m_name(name), m_position(0), m_version(0) {}
			~C_BankFile() { delete m_pInner; }

			const bool IsWriting() const { return false; }
			Btl::String Name() const { return m_name; }
			const bool GetEndianSwapFlag() const { return false; }
			void SetEndianSwapFlag(const bool) {}
			U32 GetVersionNumber() const { return m_version; }
			void SetVersionNumber(U32 v) { m_version = v; }
			const Sys::C_StreamPos GetPosition() const { return m_position; }
			const Sys::C_StreamSize Length() { return m_pInner->Length(); }
			void SetPosition(const Sys::C_StreamPos p) { m_position = p; }
			bool Overflow() { return false; }

			bool Underflow(UnderflowContext& uc)
			{
				if (m_position + uc.readSize > m_pInner->Length())
					return false;
				if (uc.readTarget && uc.readSize && !Fill(uc.readTarget, m_position, uc.readSize))
					return false;
				m_position += uc.readSize;
				uc.bytesRead = uc.readSize;
				return true;
			}

		private:
			// The converted bytes of [at, at + size).
			bool Fill(U8* target, U64 at, U32 size)
			{
				// the header region: converted already
				const U64 headers = m_patch.headers.size();
				if (at < headers)
				{
					const U32 n = (U32)((at + size <= headers ? at + size : headers) - at);
					memcpy(target, &m_patch.headers[(size_t)at], n);
					target += n;
					at += n;
					size -= n;
					if (!size)
						return true;
				}
				unsigned long long from = at, count = size;
				WidenForBankPatch(m_patch, from, count);
				U8* p = target;
				if (from != at || count != size)
				{
					m_scratch.resize((size_t)count);	// a swapped word straddles the read
					p = &m_scratch[0];
				}
				if (from != m_innerPosition)
					m_pInner->SetPosition(from);
				if (!m_pInner->Read(p, (U32)count))
					return false;
				m_innerPosition = from + count;
				ApplyBankPatch(m_patch, from, p, (size_t)count);
				if (p != target)
					memcpy(target, p + (at - from), size);
				return true;
			}

			Sys::I_File* m_pInner;
			U64 m_innerPosition;
			BankPatch m_patch;
			Btl::String m_name;
			U64 m_position;
			U32 m_version;
			std::vector<U8> m_scratch;
		};

		// Opens a PS3 bank through 'fs', reading it front to back only: an in-memory bank
		// (chStreaming 0) is read whole and converted in memory; a streaming bank is served
		// by a C_BankFile. NULL if the file cannot be opened; a bank that cannot be
		// converted is served unchanged (logged; the engine then rejects its endian byte).
		Sys::I_File* OpenBank(Sys::I_FileSystem& fs, const Sys::C_FilePath& path, const Sys::C_OpenParams& params)
		{
			Sys::I_File* pFile = fs.Open(path, Sys::E_FileAccess_Read, params);
			if (!pFile)
				return NULL;
			const size_t length = (size_t)pFile->Length();
			std::vector<unsigned char> head;
			std::string error;
			// grows 'head' to 'want' bytes by reading on from where it ends
			struct Local
			{
				static bool ReadTo(Sys::I_File* f, std::vector<unsigned char>& h, size_t want)
				{
					const size_t have = h.size();
					if (want <= have)
						return true;
					h.resize(want);
					return f->Read(&h[have], (U32)(want - have));
				}
			};
			size_t needed = 0x34 < length ? 0x34 : length;
			while (error.empty())
			{
				if (!Local::ReadTo(pFile, head, needed < length ? needed : length))
					error = "short read of the bank headers";
				else if (BankHeadersSize(head.empty() ? NULL : &head[0], head.size(), needed) || needed > length || needed <= head.size())
					break;
			}

			const bool streaming = head.size() > 0x11 && head[0x11] != 0;
			if (error.empty() && !streaming)
			{
				std::vector<unsigned char> bytes;
				if (!Local::ReadTo(pFile, head, length))
					error = "short read of the bank";
				else if (ConvertBank(head.empty() ? NULL : &head[0], head.size(), bytes, error) == E_Result_Converted)
				{
					delete pFile;
					C_OwnedMemoryFile* pMemory = new C_OwnedMemoryFile(bytes);
					pMemory->OpenOwned(Btl::String(path.Chars()), params);
					return pMemory;
				}
			}
			BankPatch patch;
			if (error.empty() && PlanBank(head.empty() ? NULL : &head[0], head.size(), length, patch, error) == E_Result_Converted)
				return new C_BankFile(pFile, head.size(), patch, Btl::String(path.Chars()));
			LOGERROR(g_convertLog, "X360Convert: cannot convert %s: %s", path.Chars(), error.c_str());
			delete pFile;
			return fs.Open(path, Sys::E_FileAccess_Read, params);
		}
	}

	struct C_ConvertFs::Impl
	{
		CRITICAL_SECTION lock;
		// Guards 'levels' and level preparation. Separate from 'lock' because preparing a
		// level runs the engine's scene reader, which takes RenderingData's serialise lock:
		// the main thread holds that lock while FX opens an effect through this layer
		// (Effect::OnLoad -> FX::LoadEffect -> Open, which takes 'lock'), so holding 'lock'
		// here deadlocked the two (seen on a PS3 source, 2026-09-25).
		CRITICAL_SECTION levelLock;

		// Recently converted files, so FileInfo followed by Open converts once.
		std::list<std::pair<std::string, std::vector<unsigned char> > > recent;
		size_t recentBytes;
		std::map<std::string, U64> convertedSize;
		static const size_t kRecentBudget = 64 * 1024 * 1024;

		// Converted levels (scene + stream layout), most recent first.
		std::list<std::pair<std::string, std::shared_ptr<C_LevelConversion> > > levels;

		Impl() : recentBytes(0) { InitializeCriticalSection(&lock); InitializeCriticalSection(&levelLock); }
		~Impl() { DeleteCriticalSection(&levelLock); DeleteCriticalSection(&lock); }

		bool Find(const std::string& k, std::vector<unsigned char>& out)
		{
			for (std::list<std::pair<std::string, std::vector<unsigned char> > >::iterator it = recent.begin(); it != recent.end(); ++it)
				if (it->first == k)
				{
					out = it->second;
					return true;
				}
			return false;
		}

		void Remember(const std::string& k, const std::vector<unsigned char>& data)
		{
			convertedSize[k] = data.size();
			recent.push_front(std::make_pair(k, data));
			recentBytes += data.size();
			while (recentBytes > kRecentBudget && recent.size() > 1)
			{
				recentBytes -= recent.back().second.size();
				recent.pop_back();
			}
		}

		// The conversion of the level in 'dir' (its levelrender scene and levelstream).
		std::shared_ptr<C_LevelConversion> Level(Sys::I_FileSystem& fs, const std::string& dir, std::string& error)
		{
			for (std::list<std::pair<std::string, std::shared_ptr<C_LevelConversion> > >::iterator it = levels.begin(); it != levels.end(); ++it)
				if (it->first == dir)
					return it->second;

			std::vector<unsigned char> render;
			if (!ReadWhole(fs, Sys::C_FilePath((dir + "level.levelrender").c_str()), render))
			{
				error = "cannot read " + dir + "level.levelrender";
				return std::shared_ptr<C_LevelConversion>();
			}
			// 360 FourCC, Render { ..., Scene, ... }
			Block root, scene;
			std::vector<Block> kids;
			if (render.size() < 4 || memcmp(&render[0], SourceFourCC(), 4) != 0 || !ReadBlockBE(&render[4], render.size() - 4, root) || !ChildrenBE(root, kids))
			{
				error = dir + "level.levelrender is not a console Render block tree";
				return std::shared_ptr<C_LevelConversion>();
			}
			bool found = false;
			for (size_t i = 0; i < kids.size(); ++i)
				if (kids[i].id == "Scene")
				{
					scene = kids[i];
					found = true;
				}
			if (!found)
			{
				error = dir + "level.levelrender has no Scene";
				return std::shared_ptr<C_LevelConversion>();
			}

			Sys::C_OpenParams params;
			params.m_IsCritical = false;
			Sys::I_File* pStream = fs.Open(Sys::C_FilePath((dir + "level.levelstream").c_str()), Sys::E_FileAccess_Read, params);

			std::shared_ptr<C_LevelConversion> level(new C_LevelConversion);
			if (!level->Prepare(scene.payload, scene.PayloadSize(), new C_LevelStreamSource(pStream), error))
				return std::shared_ptr<C_LevelConversion>();

			levels.push_front(std::make_pair(dir, level));
			while (levels.size() > 2)
				levels.pop_back();
			return level;
		}
	};

	C_ConvertFs::C_ConvertFs(Sys::I_FileSystem* pInner) : m_pInner(pInner), m_pImpl(new Impl) {}

	C_ConvertFs::~C_ConvertFs()
	{
		delete m_pImpl;
	}

	namespace
	{
		// Reads the 360 file through 'fs' and converts it. Returns false when the
		// file cannot be opened; 'bytes' holds the PC file, or the original bytes
		// when conversion is not possible (the failure is logged).
		bool LoadConverted(Sys::I_FileSystem& fs, const Sys::C_FilePath& path, const Sys::C_OpenParams& params,
			const std::shared_ptr<C_LevelConversion>& level, std::vector<unsigned char>& bytes)
		{
			Sys::I_File* pFile = fs.Open(path, Sys::E_FileAccess_Read, params);
			if (!pFile)
				return false;
			std::vector<unsigned char> in((size_t)pFile->Length());
			const bool read = in.empty() || pFile->Read(&in[0], (U32)in.size());
			delete pFile;
			if (!read)
			{
				LOGERROR(g_convertLog, "X360Convert: short read of %s", path.Chars());
				return false;
			}
			std::string error;
			E_Result r;
			if (level)
				r = ConvertLevelRender(in.empty() ? NULL : &in[0], in.size(), level->Scene(), bytes, error);
			else
			{
				r = ConvertFile(path.Chars(), in.empty() ? NULL : &in[0], in.size(), bytes, error);
				if (r == E_Result_NotHandled)
					r = EngineConvertFile(path.Chars(), in.empty() ? NULL : &in[0], in.size(), bytes, error);
			}
			if (r == E_Result_Converted)
				return true;
			if (r == E_Result_Failed)
				LOGERROR(g_convertLog, "X360Convert: cannot convert %s: %s", path.Chars(), error.c_str());
			bytes.swap(in);
			return true;
		}
	}

	bool C_ConvertFs::Handles(const Sys::C_FilePath& path, std::string& key, bool& levelRender, bool& levelStream)
	{
		key = Key(path);
		levelRender = EndsWith(key, ".levelrender");
		levelStream = EndsWith(key, ".levelstream");
		return levelRender || levelStream || EndsWith(key, ".fxb") || HandlesPath(path.Chars()) || EngineHandlesPath(path.Chars());
	}

	std::shared_ptr<C_LevelConversion> C_ConvertFs::LevelFor(const std::string& key)
	{
		std::string error;
		EnterCriticalSection(&m_pImpl->levelLock);
		std::shared_ptr<C_LevelConversion> level = m_pImpl->Level(*m_pInner, LevelDir(key), error);
		LeaveCriticalSection(&m_pImpl->levelLock);
		if (!level)
			LOGERROR(g_convertLog, "X360Convert: cannot convert the level of %s: %s", key.c_str(), error.c_str());
		return level;
	}

	Sys::I_File* C_ConvertFs::Open(const Sys::C_FilePath& fileName, const Sys::E_FileAccess fileAccess, const Sys::C_OpenParams& params)
	{
		std::string k;
		bool levelRender, levelStream;
		if (fileAccess != Sys::E_FileAccess_Read || !Handles(fileName, k, levelRender, levelStream))
			return m_pInner->Open(fileName, fileAccess, params);

		if (GetSource() == E_Source_PS3 && EndsWith(k, ".baf"))
			return OpenBank(*m_pInner, fileName, params);

		if (levelStream)
		{
			std::shared_ptr<C_LevelConversion> level = LevelFor(k);
			if (!level)
				return m_pInner->Open(fileName, fileAccess, params);
			return new C_LevelStreamFile(level, Btl::String(fileName.Chars()));
		}

		std::vector<unsigned char> bytes;
		EnterCriticalSection(&m_pImpl->lock);
		const bool cached = m_pImpl->Find(k, bytes);
		LeaveCriticalSection(&m_pImpl->lock);
		if (!cached)
		{
			std::shared_ptr<C_LevelConversion> level;
			if (levelRender && !(level = LevelFor(k)))
				return m_pInner->Open(fileName, fileAccess, params);
			if (EndsWith(k, ".fxb") ? !LoadShader(*m_pInner, fileName, k, params, bytes) : !LoadConverted(*m_pInner, fileName, params, level, bytes))
				return NULL;
			EnterCriticalSection(&m_pImpl->lock);
			m_pImpl->Remember(k, bytes);
			LeaveCriticalSection(&m_pImpl->lock);
		}
		C_OwnedMemoryFile* pFile = new C_OwnedMemoryFile(bytes);
		pFile->OpenOwned(Btl::String(fileName.Chars()), params);
		return pFile;
	}

	bool C_ConvertFs::FileInfo(const Sys::C_FilePath& p, Sys::C_FileInfo* pInfo)
	{
		if (!m_pInner->FileInfo(p, pInfo))
			return false;
		std::string k;
		bool levelRender, levelStream;
		if (!pInfo || pInfo->m_FileType != Sys::E_FileType_File || !Handles(p, k, levelRender, levelStream))
			return true;
		if (GetSource() == E_Source_PS3 && EndsWith(k, ".baf"))
			return true;	// served in place (C_BankFile): same size

		if (levelStream)
		{
			std::shared_ptr<C_LevelConversion> level = LevelFor(k);
			if (level)
				pInfo->m_FileSize = level->StreamLength();
			return true;
		}

		EnterCriticalSection(&m_pImpl->lock);
		std::map<std::string, U64>::iterator it = m_pImpl->convertedSize.find(k);
		const bool known = it != m_pImpl->convertedSize.end();
		if (known)
			pInfo->m_FileSize = it->second;
		LeaveCriticalSection(&m_pImpl->lock);
		if (known)
			return true;
		Sys::C_OpenParams params;
		params.m_IsCritical = false;
		std::shared_ptr<C_LevelConversion> level;
		if (levelRender && !(level = LevelFor(k)))
			return true;
		std::vector<unsigned char> bytes;
		if (EndsWith(k, ".fxb") ? LoadShader(*m_pInner, p, k, params, bytes) : LoadConverted(*m_pInner, p, params, level, bytes))
		{
			pInfo->m_FileSize = bytes.size();
			EnterCriticalSection(&m_pImpl->lock);
			m_pImpl->Remember(k, bytes);
			LeaveCriticalSection(&m_pImpl->lock);
		}
		return true;
	}
}
