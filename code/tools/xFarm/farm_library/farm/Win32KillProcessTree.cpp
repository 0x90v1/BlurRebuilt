#include "StdAfx.h"

/// the following is some Windows code to terminate an entire process tree i.e. a process and all it's child processes
#ifdef WIN32

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <winternl.h>
#include <Tlhelp32.h>

#ifndef STATUS_INFO_LENGTH_MISMATCH
#define STATUS_INFO_LENGTH_MISMATCH	((NTSTATUS)0xC0000004L)
#endif
#define NT_SUCCESS(Status)  (((NTSTATUS)(Status)) >= 0)

//
typedef struct {
    ULONG NextEntryOffset;
    BYTE Reserved1[52];
    PVOID Reserved2[3];
    HANDLE UniqueProcessId;
    ULONG InheritedFromProcessId;
    ULONG HandleCount;
    BYTE Reserved4[4];
    PVOID Reserved5[11];
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER Reserved6[6];
} GRAH_SYSTEM_PROCESS_INFORMATION;

BOOL KillProcess(IN DWORD dwProcessId)
{
    HANDLE hProcess;
    DWORD dwError;

    // first, try to obtain a process handle without using
    // additional privileges
    hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessId);
    if (hProcess == NULL)
		return FALSE;

    // try to terminate the process
    if (!TerminateProcess(hProcess, (UINT)-1))
    {
        dwError = GetLastError();
        CloseHandle(hProcess);
        return SetLastError(dwError), FALSE;
    }

    CloseHandle(hProcess);

    // completed successfully
    return TRUE;
}

// a helper function that walks a process tree recursively
// on Windows NT and terminates all processes in the tree
static BOOL KillProcessTreeNtHelper(IN PSYSTEM_PROCESS_INFORMATION pInfo,
									IN DWORD dwProcessId
									)
{
	_ASSERTE(pInfo != NULL);

	PSYSTEM_PROCESS_INFORMATION p = pInfo;

	// terminate all children first
	for (;;)
	{
		if (((GRAH_SYSTEM_PROCESS_INFORMATION*)p)->InheritedFromProcessId == dwProcessId)
			KillProcessTreeNtHelper(pInfo, (DWORD)p->UniqueProcessId);

		if (p->NextEntryOffset == 0)
			break;

		// find address of the next structure
		p = (PSYSTEM_PROCESS_INFORMATION)(((PUCHAR)p) 
			+ p->NextEntryOffset);
	}

	// terminate the specified process
	if (!KillProcess(dwProcessId))
		return GetLastError();

	return ERROR_SUCCESS;
}

// a helper function that walks a process tree recursively
// on Windows 9x and terminates all processes in the tree
static BOOL KillProcessTreeWinHelper(IN DWORD dwProcessId)
{
	HINSTANCE hKernel;
	HANDLE (WINAPI * pCreateToolhelp32Snapshot)(DWORD, DWORD);
	BOOL (WINAPI * pProcess32First)(HANDLE, PROCESSENTRY32 *);
	BOOL (WINAPI * pProcess32Next)(HANDLE, PROCESSENTRY32 *);

	// get KERNEL32.DLL handle
	hKernel = GetModuleHandle("kernel32.dll");
	_ASSERTE(hKernel != NULL);

	// find necessary entrypoints in KERNEL32.DLL
	*(FARPROC *)&pCreateToolhelp32Snapshot =
		GetProcAddress(hKernel, "CreateToolhelp32Snapshot");
	*(FARPROC *)&pProcess32First =
		GetProcAddress(hKernel, "Process32First");
	*(FARPROC *)&pProcess32Next =
		GetProcAddress(hKernel, "Process32Next");

	if (pCreateToolhelp32Snapshot == NULL ||
		pProcess32First == NULL ||
		pProcess32Next == NULL)
		return ERROR_PROC_NOT_FOUND;

	HANDLE hSnapshot;
	PROCESSENTRY32 Entry;

	// create a snapshot of all processes
	hSnapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
		return GetLastError();

	Entry.dwSize = sizeof(Entry);
	if (!pProcess32First(hSnapshot, &Entry))
	{
		DWORD dwError = GetLastError();
		CloseHandle(hSnapshot);
		return dwError;
	}

	// terminate children first
	do
	{
		if (Entry.th32ParentProcessID == dwProcessId)
			KillProcessTreeWinHelper(Entry.th32ProcessID);

		Entry.dwSize = sizeof(Entry);
	}
	while (pProcess32Next(hSnapshot, &Entry));

	CloseHandle(hSnapshot);

	// terminate the specified process
	if (!KillProcess(dwProcessId))
		return GetLastError();

	return ERROR_SUCCESS;
}

BOOL KillProcessEx(IN DWORD dwProcessId,   // process identifier
				   IN BOOL bTree           // terminate tree flag
				   )
{
	if (!bTree)
		return KillProcess(dwProcessId);

	OSVERSIONINFO osvi;
	DWORD dwError;

	// determine operating system version
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	GetVersionEx(&osvi);

	if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		HINSTANCE hNtDll;
		NTSTATUS (WINAPI * pZwQuerySystemInformation)(UINT, PVOID, 
			ULONG, PULONG);

		// get NTDLL.DLL handle
		hNtDll = GetModuleHandle("ntdll.dll");
		_ASSERTE(hNtDll != NULL);

		// find address of ZwQuerySystemInformation
		*(FARPROC *)&pZwQuerySystemInformation =
			GetProcAddress(hNtDll, "ZwQuerySystemInformation");
		if (pZwQuerySystemInformation == NULL)
			return SetLastError(ERROR_PROC_NOT_FOUND), NULL;

		// get default process heap handle
		HANDLE hHeap = GetProcessHeap();

		NTSTATUS Status;
		ULONG cbBuffer = 0x8000;
		PVOID pBuffer = NULL;

		// it is difficult to predict what buffer size will be
		// enough, so we start with 32K buffer and increase its
		// size as needed
		do
		{
			pBuffer = HeapAlloc(hHeap, 0, cbBuffer);
			if (pBuffer == NULL)
				return SetLastError(ERROR_NOT_ENOUGH_MEMORY), FALSE;

			Status = pZwQuerySystemInformation(
				5,//SystemProcessesAndThreadsInformation,
				pBuffer, cbBuffer, NULL);

			if (Status == STATUS_INFO_LENGTH_MISMATCH)
			{
				HeapFree(hHeap, 0, pBuffer);
				cbBuffer *= 2;
			}
			else if (!NT_SUCCESS(Status))
			{
				HeapFree(hHeap, 0, pBuffer);
				return SetLastError(Status), NULL;
			}
		}
		while (Status == STATUS_INFO_LENGTH_MISMATCH);

		// call the helper
		dwError = KillProcessTreeNtHelper(
			(PSYSTEM_PROCESS_INFORMATION)pBuffer, 
			dwProcessId);

		HeapFree(hHeap, 0, pBuffer);
	}
	else
	{
		// call the helper
		dwError = KillProcessTreeWinHelper(dwProcessId);
	}

	SetLastError(dwError);
	return dwError == ERROR_SUCCESS;
}

#endif // WIN32
