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
	}

	struct C_ConvertFs::Impl
	{
		CRITICAL_SECTION lock;

		// Recently converted files, so FileInfo followed by Open converts once.
		std::list<std::pair<std::string, std::vector<unsigned char> > > recent;
		size_t recentBytes;
		std::map<std::string, U64> convertedSize;
		static const size_t kRecentBudget = 64 * 1024 * 1024;

		// Converted levels (scene + stream layout), most recent first.
		std::list<std::pair<std::string, std::shared_ptr<C_LevelConversion> > > levels;

		Impl() : recentBytes(0) { InitializeCriticalSection(&lock); }
		~Impl() { DeleteCriticalSection(&lock); }

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
			if (render.size() < 4 || memcmp(&render[0], "360 ", 4) != 0 || !ReadBlockBE(&render[4], render.size() - 4, root) || !ChildrenBE(root, kids))
			{
				error = dir + "level.levelrender is not a 360 Render block tree";
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
		return levelRender || levelStream || HandlesPath(path.Chars()) || EngineHandlesPath(path.Chars());
	}

	std::shared_ptr<C_LevelConversion> C_ConvertFs::LevelFor(const std::string& key)
	{
		std::string error;
		EnterCriticalSection(&m_pImpl->lock);
		std::shared_ptr<C_LevelConversion> level = m_pImpl->Level(*m_pInner, LevelDir(key), error);
		LeaveCriticalSection(&m_pImpl->lock);
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
			if (!LoadConverted(*m_pInner, fileName, params, level, bytes))
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
		if (LoadConverted(*m_pInner, p, params, level, bytes))
		{
			pInfo->m_FileSize = bytes.size();
			EnterCriticalSection(&m_pImpl->lock);
			m_pImpl->Remember(k, bytes);
			LeaveCriticalSection(&m_pImpl->lock);
		}
		return true;
	}
}
