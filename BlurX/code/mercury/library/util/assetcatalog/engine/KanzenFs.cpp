// [BlurX] Kanzen normalising layer for a Generation B disc. See KanzenFs.h.

#include "StdAfx.h"

#include "KanzenFs.h"

#include <MemoryFs/Impl/MemoryFile.h>
#include <assetcatalog/AssetCatalog.h>
#include <assetcatalog/KanzenNormalise.h>

namespace AssetCatalog
{
	namespace
	{
		Debug::Logger& g_kanzenLog = Debug::GetLogger("BlurX.Kanzen");

		// A read-only memory file that keeps its bytes alive.
		class C_SharedMemoryFile : public LogicalFs::C_MemoryFile
		{
		public:
			explicit C_SharedMemoryFile(const std::shared_ptr<std::vector<unsigned char> >& pData) : m_pData(pData) {}
			bool OpenShared(const Btl::String& name, const Sys::C_OpenParams& params)
			{
				static unsigned char empty = 0;
				return Open(m_pData->empty() ? &empty : &(*m_pData)[0], (U32)m_pData->size(), name, params);
			}
		private:
			std::shared_ptr<std::vector<unsigned char> > m_pData;
		};

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
	}

	C_KanzenFs::C_KanzenFs(Sys::I_FileSystem* pInner, const std::string& discId)
		: m_pInner(pInner), m_DiscId(discId), m_Normalised(0)
	{
	}

	C_KanzenFs::~C_KanzenFs()
	{
		if (m_Normalised || !m_Refused.empty())
			LOGINFO(g_kanzenLog, "%s: %u files normalised, %u refused", m_DiscId.c_str(), m_Normalised, (unsigned)m_Refused.size());
	}

	void C_KanzenFs::Refuse(const std::string& key, const char* why)
	{
		if (m_Refused.insert(key).second)
			LOGWARN(g_kanzenLog, "%s: %s refused (%s); the profile's next disc serves it", m_DiscId.c_str(), key.c_str(), why);
	}

	bool C_KanzenFs::Refused(const Sys::C_FilePath& path, T_Bytes* pBytes)
	{
		const std::string key = NormalisePath(path.Chars());
		{
			std::lock_guard<std::mutex> lock(m_Lock);
			if (m_Refused.count(key))
				return true;
			if (!KanzenNormalise::Handles(key.c_str()) || m_AsIs.count(key))
				return false;
			for (std::list<C_Cached>::iterator it = m_Cache.begin(); it != m_Cache.end(); ++it)
				if (it->m_Key == key)
				{
					if (pBytes)
						*pBytes = it->m_Bytes;
					return false;
				}
		}
		std::vector<unsigned char> raw;
		if (!ReadWhole(*m_pInner, path, raw))
			return false;			// not on this disc: the caller's own lookup says so
		T_Bytes normalised(new std::vector<unsigned char>());
		std::string error;
		const KanzenNormalise::E_Result r = KanzenNormalise::Normalise(key.c_str(), raw.empty() ? NULL : &raw[0], raw.size(), *normalised, error);
		std::lock_guard<std::mutex> lock(m_Lock);
		if (r == KanzenNormalise::E_Result_NotHandled)
		{
			m_AsIs.insert(key);
			return false;
		}
		if (r == KanzenNormalise::E_Result_Failed)
		{
			Refuse(key, error.c_str());
			return true;
		}
		++m_Normalised;
		C_Cached cached;
		cached.m_Key = key;
		cached.m_Bytes = normalised;
		m_Cache.push_front(cached);
		if (m_Cache.size() > 16)
			m_Cache.pop_back();
		if (pBytes)
			*pBytes = normalised;
		return false;
	}

	Sys::I_File* C_KanzenFs::Open(const Sys::C_FilePath& filePath, const Sys::E_FileAccess fileAccess, const Sys::C_OpenParams& params)
	{
		if (fileAccess != Sys::E_FileAccess_Read)
			return m_pInner->Open(filePath, fileAccess, params);
		T_Bytes bytes;
		if (Refused(filePath, &bytes))
			return NULL;
		if (!bytes)
			return m_pInner->Open(filePath, fileAccess, params);
		C_SharedMemoryFile* pFile = new C_SharedMemoryFile(bytes);
		if (pFile->OpenShared(filePath.Chars(), params))
			return pFile;
		delete pFile;
		return NULL;
	}

	bool C_KanzenFs::FileExists(const Sys::C_FilePath& filePath)
	{
		return m_pInner->FileExists(filePath) && !Refused(filePath);
	}

	bool C_KanzenFs::FileInfo(const Sys::C_FilePath& filePath, Sys::C_FileInfo* pInfo)
	{
		if (!m_pInner->FileInfo(filePath, pInfo))
			return false;
		if (!pInfo || pInfo->m_FileType != Sys::E_FileType_File)
			return true;
		T_Bytes bytes;
		if (Refused(filePath, &bytes))
			return false;
		if (bytes)
			pInfo->m_FileSize = bytes->size();
		return true;
	}
}
