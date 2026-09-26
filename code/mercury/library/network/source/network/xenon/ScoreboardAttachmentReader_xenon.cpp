#include <StdAfx.h>
#include <SystemProfile/SystemProfile.h>
#include <Network/ScoreboardAttachmentReader.h>
#include "AsyncTask_xenon.h"


namespace Scoreboard
{
	// constructor
	C_AttachmentReader::C_AttachmentReader() : m_Reading(false), m_NumBytesDownloaded(0), m_pAsyncTask(NULL)
	{
	}

	
	// destructor
	C_AttachmentReader::~C_AttachmentReader()
	{
		StopAndCleanup();
	}


	// stop any scoreboard write and reset internal data to default.
	bool C_AttachmentReader::StopAndCleanup(void)
	{
		// stop asynchronous write if active
		NetPlay::C_AsyncTask* pT = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);
		if (pT)
		{
			if (!pT->Finished())
				pT->Close();
			delete pT;
			m_pAsyncTask = NULL;
		}

		m_Reading = false;
		m_NumBytesDownloaded = 0;
		return true;
	}

	bool C_AttachmentReader::Start(const SysProfile::FullProfile& localPlayer, const SysProfile::BaseProfile& attachmentOwner, const U32 scoreboardId, U8* pStorage, const U32 storageSize)
	{
		assert(!m_Reading);
		StopAndCleanup();

		if (NULL == pStorage || 0 == storageSize || 0 == scoreboardId)
			return false;
	
		if (localPlayer.m_UserIndex >= XUSER_MAX_COUNT)
			return false;

		if (attachmentOwner.m_IsGuest)
		{
			LOGERROR(NetPlay::g_Log, "C_AttachmentReader::Start() - Can't read scoreboard attachments for guest players.");
			return false;
		}

		// allocate async task
		m_pAsyncTask = reinterpret_cast<void*> (new NetPlay::C_AsyncTask);
		if (NULL == m_pAsyncTask)
			return false;

		NetPlay::C_AsyncTask* pT = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);

		// what scoreboard we writing to.
		m_Info.dwLeaderboardID = scoreboardId;

		// create server path.
		DWORD serverPathSize = MAX_PATH;
		WCHAR serverPath[MAX_PATH] = L"\0";
		DWORD res = XStorageBuildServerPathByXuid(attachmentOwner.m_Uid, XSTORAGE_FACILITY_GAME_CLIP, &m_Info, sizeof(m_Info), L"attachment.bin", serverPath, &serverPathSize);
		if (ERROR_SUCCESS != res)
		{
			StopAndCleanup();
			return false;
		}

		ZeroMemory(&m_Results, sizeof(m_Results));

		res = XStorageDownloadToMemory(localPlayer.m_UserIndex, serverPath, storageSize, pStorage, sizeof(m_Results), &m_Results, &(*pT));
		if (ERROR_IO_PENDING == res)
		{
			m_Reading = true;
			return true;
		}

		StopAndCleanup();
		return false;
	}
	
	C_AttachmentReader::eScoreboardAttachmentReaderState C_AttachmentReader::Continue(U32& numBytesDownloaded)
	{
		numBytesDownloaded = 0;

		if (!m_Reading)
			return eReadNotActive;

		NetPlay::C_AsyncTask* pAsync = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);
		if (!pAsync->Finished())
			return eReadInProgress;

		eScoreboardAttachmentReaderState ret = eReadFailed;
		const U32 error_code = pAsync->ErrorCode();

		if (ERROR_SUCCESS == error_code)
		{
			// file downloaded ok
			numBytesDownloaded = m_Results.dwBytesTotal;
			ret = eReadSuccessful;
		}
		else if (XONLINE_E_STORAGE_FILE_NOT_FOUND == error_code)
		{
			// file doesn't exist on per user title storage
			ret = eReadFileDoesNotExist;
		}
		else if (ERROR_INSUFFICIENT_BUFFER == error_code)
		{
			// file exists but the supplied buffer is too small to download the complete file.
			ret = eReadDownloadBufferToSmall;
		}
		else
		{
			// generic download error. must assume file exists but failed to download.
			LOGWARN(NetPlay::g_Log, "C_AttachmentReader::Continue() - generic read error Code 0x%x", error_code);
		}

		StopAndCleanup();
		return ret;
	}

}// end namespace