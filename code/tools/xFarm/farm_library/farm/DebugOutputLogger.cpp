#include "StdAfx.h"
#include "DebugOutputLogger.h"

// initialize class, start the thread
DebugOutputLogger::DebugOutputLogger()
{
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = &sd;

	InitializeCriticalSection(&m_CriticalSection);

    if(!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        fprintf(stderr,"unable to InitializeSecurityDescriptor, err == %d\n",
            GetLastError());
        return;
    }

    if(!SetSecurityDescriptorDacl(&sd, TRUE, (PACL)NULL, FALSE)) {
        fprintf(stderr,"unable to SetSecurityDescriptorDacl, err == %d\n",
            GetLastError());
        return;
    }

    AckEvent = CreateEvent(&sa, FALSE, FALSE, "DBWIN_BUFFER_READY");

    if (!AckEvent) {
        fprintf(stderr,
                "dbmon: Unable to create synchronization object, err == %d\n",
                GetLastError());
        return;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        fprintf(stderr, "dbmon: already running\n");
        return;
    }

    ReadyEvent = CreateEvent(&sa, FALSE, FALSE, "DBWIN_DATA_READY");

    if (!ReadyEvent) {
        fprintf(stderr,
                "dbmon: Unable to create synchronization object, err == %d\n",
                GetLastError());
        return;
    }

    SharedFile = CreateFileMapping(
                        (HANDLE)-1,
                        &sa,
                        PAGE_READWRITE,
                        0,
                        4096,
                        "DBWIN_BUFFER");

    if (!SharedFile) {
        fprintf(stderr,
                "dbmon: Unable to create file mapping object, err == %d\n",
                GetLastError());
        return;
    }

    SharedMem = MapViewOfFile(
                        SharedFile,
                        FILE_MAP_READ,
                        0,
                        0,
                        512);

    if (!SharedMem) {
        fprintf(stderr,
                "dbmon: Unable to map shared memory, err == %d\n",
                GetLastError());
        return;
    }

	m_iProcess = -1;

	m_thread = CreateThread(0, 0, ThreadEntryPoint, this, 0, 0);
}

DebugOutputLogger::~DebugOutputLogger()
{
	TerminateThread(m_thread, 0);

	UnmapViewOfFile(SharedMem);

	CloseHandle(SharedFile);
	CloseHandle(ReadyEvent);
	CloseHandle(AckEvent);
}

// thread entry point function
DWORD CALLBACK DebugOutputLogger::ThreadEntryPoint(void *pData)
{
	DebugOutputLogger *pThis = (DebugOutputLogger*)pData;

	pThis->MainFunction();

	return 0;
}

// set process to log, or -1 to stop logging
void DebugOutputLogger::SetProcess(int i)
{
	EnterCriticalSection(&m_CriticalSection);

	m_iProcess = i;

	LeaveCriticalSection(&m_CriticalSection);
}

// kill the thread and retrieve the log - make sure logging is disabled when doing this
void DebugOutputLogger::ClearLog()
{
	m_log = "";
}

// return outstanding log text and clear it
const std::string DebugOutputLogger::GetLog()
{
	std::string logtxt;

	EnterCriticalSection(&m_CriticalSection);

	logtxt = m_log;
	ClearLog();

	LeaveCriticalSection(&m_CriticalSection);

	return logtxt;
}

// this is the main function which runs on it's own thread and performs the debug output logging
void DebugOutputLogger::MainFunction()
{
	LPSTR String = (LPSTR)SharedMem + sizeof(DWORD);
	LPDWORD  pThisPid = (LPDWORD)SharedMem;

    SetEvent(AckEvent);

    for (;;) {

		// this waits for a ReadyEvent, which signals a new debug string
        DWORD ret = WaitForSingleObject(ReadyEvent, INFINITE);

        if (ret != WAIT_OBJECT_0)
		{
            fprintf(stderr, "dbmon: wait failed; err == %d\n", GetLastError());
        }
		else
		{
			EnterCriticalSection(&m_CriticalSection);

			// check process id
			if (m_iProcess != -1)
			{
				// save debug string
				m_log = m_log + String;
			}

			LeaveCriticalSection(&m_CriticalSection);

			SetEvent(AckEvent);
        }
    }
}
