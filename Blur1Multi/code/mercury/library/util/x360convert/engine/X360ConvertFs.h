// [Blur1Multi] File-system layer that serves Xbox 360 assets in PC format (T072, D11).
//
// Wraps the game's file system (the overlay of the 360 source's pak layers and
// its loose files) for an Xbox 360 source. Opening a path whose format the
// X360Convert library handles reads the 360 file through the wrapped file
// system, converts it and returns the PC bytes from memory; FileInfo reports
// the converted size. A level's render file and levelstream are converted
// together (C_LevelConversion); the levelstream is served as a file that
// converts chunks as they are read. Everything else is forwarded unchanged.
// Installed by CreateFsLayers (Blur1Multi overlay) through
// C_CoreSystems::SetFileSystems.
#pragma once

#include <system/serialise/I_FileSystem.h>

#include <memory>
#include <string>

namespace X360Convert
{
	class C_LevelConversion;

	class C_ConvertFs : public Sys::I_FileSystem
	{
	public:
		explicit C_ConvertFs(Sys::I_FileSystem* pInner);
		virtual ~C_ConvertFs();

		virtual Sys::I_File* Open(const Sys::C_FilePath& fileName, const Sys::E_FileAccess fileAccess = Sys::E_FileAccess_Read,
			const Sys::C_OpenParams& params = Sys::C_OpenParams());
		virtual bool FileExists(const Sys::C_FilePath& p) { return m_pInner->FileExists(p); }
		virtual bool FileInfo(const Sys::C_FilePath& p, Sys::C_FileInfo* pInfo);
		virtual bool FileAccess(const Sys::C_FilePath& p, U32 a) { return m_pInner->FileAccess(p, a); }
		virtual bool FileTime(const Sys::C_FilePath& p, const Time::C_Time& t) { return m_pInner->FileTime(p, t); }
		virtual bool DeleteFile(const Sys::C_FilePath& p) { return m_pInner->DeleteFile(p); }
		virtual bool MoveFile(const Sys::C_FilePath& a, const Sys::C_FilePath& b) { return m_pInner->MoveFile(a, b); }
		virtual bool DirectoryInfo(const Sys::C_FilePath& p, Sys::C_DirectoryInfo* pInfo) { return m_pInner->DirectoryInfo(p, pInfo); }
		virtual bool CreateSingleDirectory(const Sys::C_FilePath& p) { return m_pInner->CreateSingleDirectory(p); }
		virtual bool SetWorkingDirectory(const Sys::C_FilePath& p) { return m_pInner->SetWorkingDirectory(p); }
		virtual Sys::C_FilePath GetWorkingDirectory() const { return m_pInner->GetWorkingDirectory(); }
		virtual Sys::C_FilePath GetSystemFilePath(const Sys::C_FilePath& p) const { return m_pInner->GetSystemFilePath(p); }

		Sys::I_FileSystem* Inner() const { return m_pInner; }

	private:
		bool Handles(const Sys::C_FilePath& path, std::string& key, bool& levelRender, bool& levelStream);
		std::shared_ptr<C_LevelConversion> LevelFor(const std::string& key);

		struct Impl;
		Sys::I_FileSystem* m_pInner;
		Impl* m_pImpl;
	};
}
