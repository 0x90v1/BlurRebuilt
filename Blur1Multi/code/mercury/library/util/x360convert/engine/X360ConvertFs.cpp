// [Blur1Multi] File-system layer serving Xbox 360 assets in PC format. See X360ConvertFs.h.

#include "StdAfx.h"

#include "X360ConvertFs.h"

#include <system/serialise/I_File.h>
#include <MemoryFs/Impl/MemoryFile.h>
#include <x360convert/X360Convert.h>

#include <list>
#include <map>
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
	}

	// Recently converted files, so FileInfo followed by Open converts once.
	struct C_ConvertFs::Impl
	{
		CRITICAL_SECTION lock;
		std::list<std::pair<std::string, std::vector<unsigned char> > > recent;
		size_t recentBytes;
		std::map<std::string, U64> convertedSize;
		static const size_t kRecentBudget = 64 * 1024 * 1024;

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
			std::vector<unsigned char>& bytes)
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
			const E_Result r = ConvertFile(path.Chars(), in.empty() ? NULL : &in[0], in.size(), bytes, error);
			if (r == E_Result_Converted)
				return true;
			if (r == E_Result_Failed)
				LOGERROR(g_convertLog, "X360Convert: cannot convert %s: %s", path.Chars(), error.c_str());
			bytes.swap(in);
			return true;
		}
	}

	Sys::I_File* C_ConvertFs::Open(const Sys::C_FilePath& fileName, const Sys::E_FileAccess fileAccess, const Sys::C_OpenParams& params)
	{
		if (fileAccess != Sys::E_FileAccess_Read || !HandlesPath(fileName.Chars()))
			return m_pInner->Open(fileName, fileAccess, params);

		const std::string k = Key(fileName);
		std::vector<unsigned char> bytes;
		EnterCriticalSection(&m_pImpl->lock);
		const bool cached = m_pImpl->Find(k, bytes);
		LeaveCriticalSection(&m_pImpl->lock);
		if (!cached)
		{
			if (!LoadConverted(*m_pInner, fileName, params, bytes))
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
		if (!pInfo || pInfo->m_FileType != Sys::E_FileType_File || !HandlesPath(p.Chars()))
			return true;
		const std::string k = Key(p);
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
		std::vector<unsigned char> bytes;
		if (LoadConverted(*m_pInner, p, params, bytes))
		{
			pInfo->m_FileSize = bytes.size();
			EnterCriticalSection(&m_pImpl->lock);
			m_pImpl->Remember(k, bytes);
			LeaveCriticalSection(&m_pImpl->lock);
		}
		return true;
	}
}
