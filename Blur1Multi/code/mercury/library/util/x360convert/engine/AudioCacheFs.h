// [Blur1Multi] Xbox 360 audio banks through the per-source audio cache (T075, D15).
//
// Wraps the file system the game reads (above X360Convert::C_ConvertFs for an
// Xbox 360 source). A '.baf' opened for reading is looked up in
// <exe dir>\audiocache\<source>\<name>.<key>.baf, the key hashing the converter's
// output version, the bank's size and its header region (bank header, WAVE rows,
// 'CUE ' chunks); a bank not there yet is converted once
// (X360Convert::ConvertXmaBankStream: XMA rows decoded and re-encoded to the PC
// banks' ADPCM, one wave in memory at a time) into a temporary file that is then
// renamed into place. The cached bank is opened from disk (a music bank is over
// 300 MB). A bank that cannot be converted is served as stored and logged; the
// engine then rejects its endian byte, as before this layer. Everything else
// passes through.
#pragma once

#include <system/serialise/I_FileSystem.h>

#include <mutex>
#include <string>

namespace X360Convert
{
	class C_AudioCacheFs : public Sys::I_FileSystem
	{
	public:
		// 'nativeFs' opens the cache files by absolute path; 'source' names the cache folder
		C_AudioCacheFs(Sys::I_FileSystem* pInner, Sys::I_FileSystem& nativeFs, const std::string& source);

		virtual Sys::I_File* Open(const Sys::C_FilePath& filePath, const Sys::E_FileAccess fileAccess = Sys::E_FileAccess_Read,
			const Sys::C_OpenParams& params = Sys::C_OpenParams());
		virtual bool FileExists(const Sys::C_FilePath& p) { return m_pInner->FileExists(p); }
		virtual bool FileInfo(const Sys::C_FilePath& filePath, Sys::C_FileInfo* pInfo);
		virtual bool FileAccess(const Sys::C_FilePath& p, U32 a) { return m_pInner->FileAccess(p, a); }
		virtual bool DirectoryInfo(const Sys::C_FilePath& p, Sys::C_DirectoryInfo* pInfo) { return m_pInner->DirectoryInfo(p, pInfo); }
		virtual bool SetWorkingDirectory(const Sys::C_FilePath& p) { return m_pInner->SetWorkingDirectory(p); }
		virtual Sys::C_FilePath GetWorkingDirectory() const { return m_pInner->GetWorkingDirectory(); }
		virtual Sys::C_FilePath GetSystemFilePath(const Sys::C_FilePath& p) const { return m_pInner->GetSystemFilePath(p); }

	private:
		// The cache file for a bank, converting it first when needed; empty when the
		// file is not an Xbox 360 bank or cannot be converted.
		std::string CachedBank(const Sys::C_FilePath& path, const Sys::C_OpenParams& params);

		Sys::I_FileSystem*	m_pInner;
		Sys::I_FileSystem&	m_Native;
		std::string			m_Dir;		// <exe dir>\audiocache\<source>\ (with the separator)
		std::mutex			m_Lock;		// one conversion at a time
	};
}
