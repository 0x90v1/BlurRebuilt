// [BlurX] Per-disc layer for a Generation B (Kanzen) disc, between the disc's
// own mounts and Blur1Multi's Xbox 360 converters (docs/decisions.md D12/D13):
//  - files in a type KanzenNormalise handles are read whole and rewritten to the
//    Blur 1 layout (still Xbox 360 byte order), so the converters above accept them;
//  - a file whose Kanzen layout cannot be normalised is refused: it does not
//    exist as far as this layer goes, so the catalog serves it from the
//    profile's next disc.
// Level files (levels\*\level.*) pass through as they are: the BlurX scene
// transcoder's Kanzen readings (x360convert/engine/SceneTranscode*) and its
// Gravity version 11 reading convert them above this layer (T030); the catalog
// serves each level directory whole from one disc (CatalogFs.h).
// Every refusal is logged once, with the reason.
#pragma once

#include <system/serialise/I_FileSystem.h>

#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace AssetCatalog
{
	class C_KanzenFs : public Sys::I_FileSystem
	{
	public:
		C_KanzenFs(Sys::I_FileSystem* pInner, const std::string& discId);
		virtual ~C_KanzenFs();

		virtual Sys::I_File* Open(const Sys::C_FilePath& filePath, const Sys::E_FileAccess fileAccess = Sys::E_FileAccess_Read,
			const Sys::C_OpenParams& params = Sys::C_OpenParams());
		virtual bool FileExists(const Sys::C_FilePath& filePath);
		virtual bool FileInfo(const Sys::C_FilePath& filePath, Sys::C_FileInfo* pInfo);
		virtual bool FileAccess(const Sys::C_FilePath& p, U32 a) { return !Refused(p) && m_pInner->FileAccess(p, a); }
		virtual bool DirectoryInfo(const Sys::C_FilePath& p, Sys::C_DirectoryInfo* pInfo) { return m_pInner->DirectoryInfo(p, pInfo); }
		virtual bool SetWorkingDirectory(const Sys::C_FilePath& p) { return m_pInner->SetWorkingDirectory(p); }
		virtual Sys::C_FilePath GetWorkingDirectory() const { return m_pInner->GetWorkingDirectory(); }
		virtual Sys::C_FilePath GetSystemFilePath(const Sys::C_FilePath& p) const { return m_pInner->GetSystemFilePath(p); }

		unsigned Normalised() const { return m_Normalised; }
		unsigned RefusedCount() const { return (unsigned)m_Refused.size(); }

	private:
		typedef std::shared_ptr<std::vector<unsigned char> > T_Bytes;

		// true: refused; else 'bytes' holds the normalised file, or is empty to serve the original
		bool Refused(const Sys::C_FilePath& path, T_Bytes* pBytes = NULL);
		void Refuse(const std::string& key, const char* why);

		Sys::I_FileSystem*			m_pInner;
		std::string					m_DiscId;
		struct C_Cached { std::string m_Key; T_Bytes m_Bytes; };
		std::list<C_Cached>			m_Cache;		// recent normalisations
		std::set<std::string>		m_Refused;
		std::set<std::string>		m_AsIs;			// handled types already in the Blur 1 layout
		std::mutex					m_Lock;
		unsigned					m_Normalised;
	};
}
