#include <StdAfx.h>
#include <Network\PerUserServerStorageWriter.h>
#include <SystemProfile\SystemProfile.h>
#include "AsyncTask_xenon.h"


namespace NetPlay
{
	// constructor
	C_PerUserServerStorageWriter::C_PerUserServerStorageWriter() : m_Writing(false), m_pUploadBuffer(NULL), m_UploadFileSize(0), m_pAsyncTask(NULL)
	{
	}

	// destructor.
	C_PerUserServerStorageWriter::~C_PerUserServerStorageWriter()
	{
		StopAndCleanup();
	}

	bool C_PerUserServerStorageWriter::Start(const SysProfile::FullProfile& localPlayer, const char* fileName, U8* pUploadBuffer, const U32 uploadFileSize)
	{
		assert(!m_Writing); 

		StopAndCleanup();

		if (NULL == pUploadBuffer || 0 == uploadFileSize)
			return false;

		// check file name for size
		const U32 fileNameLen = strlen(fileName);
		if (0 == fileNameLen || (fileNameLen + 1) > MAX_PER_USER_FILE_NAME)
		{
			LOGWARN(g_Log, "C_PerUserServerStorageWriter::Start() - Invalid file name supplied.");
			return false;
		}

		if (SysProfile::eProfileStateOnline != localPlayer.m_State || localPlayer.m_IsGuest)
			return false; // only full online players can upload files to the storage service.

		// allocate async task.
		m_pAsyncTask = reinterpret_cast<C_AsyncTask*> (new C_AsyncTask);
		if (NULL == m_pAsyncTask)
			return false;

		// create wide string version of file name
		WCHAR wFileName[MAX_PER_USER_FILE_NAME];
		swprintf_s(wFileName, MAX_PER_USER_FILE_NAME, L"%S",fileName);
		wFileName[MAX_PER_USER_FILE_NAME-1] = 0;

		DWORD serverPathSize = MAX_PATH;
		WCHAR serverPath[MAX_PATH] = L"\0";

		DWORD res = XStorageBuildServerPathByXuid(localPlayer.m_Uid, XSTORAGE_FACILITY_PER_USER_TITLE, NULL, 0, wFileName, serverPath, &serverPathSize);
		if (ERROR_SUCCESS != res)
		{
			StopAndCleanup();
			return false;
		}

		m_pUploadBuffer = pUploadBuffer;
		m_UploadFileSize =uploadFileSize;
		strcpy_s(m_FileName, MAX_PER_USER_FILE_NAME, fileName);

		NetPlay::C_AsyncTask* pT = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);
		res = XStorageUploadFromMemory(localPlayer.m_UserIndex, serverPath, uploadFileSize, pUploadBuffer, &(*pT));
		if (ERROR_IO_PENDING == res)
		{
			m_Writing = true;
			return true;
		}

		StopAndCleanup();
		return false;
	}

	C_PerUserServerStorageWriter::ePerUserStorageWriterState C_PerUserServerStorageWriter::Continue(void)
	{
		if (!m_Writing)
			return eWriteNotActive;

		NetPlay::C_AsyncTask* pAsync = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);
		if (!pAsync->Finished())
			return eWriteInProgress;

		ePerUserStorageWriterState ret = pAsync->ErrorCode() == ERROR_SUCCESS ? eWriteSuccessful : eWriteFailed;

		StopAndCleanup();
		return ret;
	}

	void C_PerUserServerStorageWriter::StopAndCleanup(void)
	{
		if (m_pAsyncTask)
		{
			C_AsyncTask* pT = reinterpret_cast<C_AsyncTask*> (m_pAsyncTask);
			pT->Close();
			delete pT;
			m_pAsyncTask = NULL;
		}

		m_Writing = false;
		m_FileName[0] = '\0';
		m_pUploadBuffer = NULL;
		m_UploadFileSize = 0;
	}
}