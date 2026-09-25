#include <StdAfx.h>
#include <Network\PublisherServerStorageReader.h>
#include <SystemProfile\SystemProfile.h>
#include "AsyncTask_xenon.h"

namespace NetPlay
{
	// constructor
	C_PublisherServerStorageReader::C_PublisherServerStorageReader() : m_Reading(false), m_pAsyncTask(NULL), m_pDownloadBuffer(NULL), m_DownloadBufferSize(0)
	{
	}

	// destructor.
	C_PublisherServerStorageReader::~C_PublisherServerStorageReader()
	{
		StopAndCleanup();
	}

	bool C_PublisherServerStorageReader::Start(const SysProfile::FullProfile& localPlayer, const char* fileName, U8* pDownloadBuffer, const U32 downloadBufferSize)
	{
		assert(!m_Reading); 
		
		StopAndCleanup();

		if (NULL == pDownloadBuffer || 0 == downloadBufferSize)
			return false;

		if (localPlayer.m_UserIndex >= XUSER_MAX_COUNT)
			return false;

		// check file name for size
		const U32 fileNameLen = strlen(fileName);
		if (0 == fileNameLen || (fileNameLen + 1) > MAX_PER_USER_FILE_NAME)
		{
			LOGWARN(g_Log, "C_PublisherServerStorageReader::Start() - Invalid file name supplied.");
			return false;
		}

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

		DWORD res = XStorageBuildServerPath(localPlayer.m_UserIndex, XSTORAGE_FACILITY_PER_TITLE, NULL, 0, wFileName, serverPath, &serverPathSize);
		if (ERROR_SUCCESS != res)
		{
			StopAndCleanup();
			return false;
		}

		ZeroMemory(&m_Results, sizeof(m_Results));

		NetPlay::C_AsyncTask* pT = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);
		res = XStorageDownloadToMemory(localPlayer.m_UserIndex, serverPath, downloadBufferSize, pDownloadBuffer, sizeof(m_Results), &m_Results, &(*pT));

		m_pDownloadBuffer		= pDownloadBuffer;   // xbox downloads straight into the buffer so we don't really need to keep a pointer to it.
		m_DownloadBufferSize	= downloadBufferSize;

		if (ERROR_IO_PENDING == res)
		{
			m_Reading = true;
			return true;
		}

		StopAndCleanup();
		return false;
	} 

	C_PublisherServerStorageReader::ePerUserStorageReaderState C_PublisherServerStorageReader::Continue(U32& downloadedSize)
	{
		downloadedSize = 0;

		if (!m_Reading)
			return eReadNotActive;

		NetPlay::C_AsyncTask* pAsync = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);
		if (!pAsync->Finished())
			return eReadInProgress;

		ePerUserStorageReaderState ret = eReadFailed;
		const U32 error_code = pAsync->ErrorCode();

		if (ERROR_SUCCESS == error_code)
		{
			// file downloaded ok
			downloadedSize = m_Results.dwBytesTotal;
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
			LOGWARN(g_Log, "C_PublisherServerStorageReader::Continue() - generic read error Code 0x%x", error_code);
		}

		StopAndCleanup();
		return ret;
	}

	void C_PublisherServerStorageReader::StopAndCleanup(void)
	{
		if (m_pAsyncTask)
		{
			C_AsyncTask* pT = reinterpret_cast<C_AsyncTask*> (m_pAsyncTask);
			pT->Close();
			delete pT;
			m_pAsyncTask = NULL;
		}

		m_Reading = false;
		m_pDownloadBuffer = NULL;
		m_DownloadBufferSize = 0;
	}
}
