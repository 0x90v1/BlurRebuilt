#include <StdAfx.h>
#include <Network/ScoreboardAttachmentWriter.h>
#include <Systemprofile/SystemProfile.h>
#include "AsyncTask_xenon.h"


namespace Scoreboard
{
	// constructor
	C_AttachmentWriter::C_AttachmentWriter() :	m_Writing(false), m_pAsyncTask(NULL)
	{
	}

	
	// destructor
	C_AttachmentWriter::~C_AttachmentWriter()
	{
		StopAndCleanup();
	}
	
	bool C_AttachmentWriter::StopAndCleanup(void)
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

		m_Writing = false;
		return true;
	}

	bool C_AttachmentWriter::Start(const SysProfile::FullProfile& localPlayer, const U32 scoreboardId, const S64 scoreRating, U8* pAttachmentData, const U32 attachmentSize)
	{
		assert(!m_Writing);
		StopAndCleanup();

		if (NULL == pAttachmentData || 0 == attachmentSize || 0 == scoreboardId)
			return false;

		if (localPlayer.m_UserIndex >= XUSER_MAX_COUNT)
			return false;
	
		XUSER_SIGNIN_INFO signInInfo;
		if (XUserGetSigninInfo(localPlayer.m_UserIndex, 0, &signInInfo) != ERROR_SUCCESS)
			return false;
		if (eXUserSigninState_SignedInToLive != signInInfo.UserSigninState)
			return false; // offline..
		if (signInInfo.dwGuestNumber != 0)
			return false; // can't write for guests

		m_pAsyncTask = reinterpret_cast<void*> (new NetPlay::C_AsyncTask);
		if (NULL == m_pAsyncTask)
			return false;

		NetPlay::C_AsyncTask* pT = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);

		// what scoreboard we writing to.
		m_Info.dwLeaderboardID = scoreboardId;

		// try and build server storage path.
		DWORD serverPathSize = MAX_PATH;
		WCHAR serverPath[MAX_PATH] = L"\0";
		DWORD res = XStorageBuildServerPath(localPlayer.m_UserIndex, XSTORAGE_FACILITY_GAME_CLIP, &m_Info, sizeof(m_Info), L"attachment.bin", serverPath, &serverPathSize);
		if (ERROR_SUCCESS == res)
		{
			// start upload process
			res = XStorageUploadFromMemory(localPlayer.m_UserIndex, serverPath, attachmentSize, pAttachmentData, &(*pT));
			if (ERROR_IO_PENDING == res)
			{
				m_Writing = true;
				return true;
			}
		}

		StopAndCleanup();
		return false;
	}
	

	C_AttachmentWriter::eScoreboardAttachmentWriterState C_AttachmentWriter::Continue(void)
	{
		if (!m_Writing)
			return eWriteNotActive;

		NetPlay::C_AsyncTask* pAsync = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);
		if (!pAsync->Finished())
			return eWriteInProgress;

		eScoreboardAttachmentWriterState res = eWriteFailed;
		if (ERROR_SUCCESS == pAsync->ErrorCode())
			res = eWriteSuccessful;
	
		StopAndCleanup();
		return res;

	}

}// end namespace