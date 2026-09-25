#include "stdafx.h"
#include <windows.h>
#include <string.h>
#include <tchar.h>
#include <stdio.h>
#include "mdump.h"

#include "Tools_Common\SendEmail.h"

LPCSTR m_szAppName;

extern std::string g_inputSceneFile;
extern std::string g_lightingModelFile;

void minidump_Init()
{
	m_szAppName = "xFarm Server";

	::SetUnhandledExceptionFilter( TopLevelFilter );
}

LONG WINAPI TopLevelFilter( struct _EXCEPTION_POINTERS *pExceptionInfo )
{
	LONG retval = EXCEPTION_CONTINUE_SEARCH;
	HWND hParent = NULL;						// find a better value for your app

//	char buffer[512];
//	sprintf(buffer,"exception number: %d\n",pExceptionInfo->ExceptionRecord->ExceptionCode);
//	OutputDebugString(buffer);

	// don't handle CTRL+C
	if (pExceptionInfo->ExceptionRecord->ExceptionCode == DBG_CONTROL_C)
		return EXCEPTION_CONTINUE_SEARCH;

	// catch 'out of memory' error separately
	if (pExceptionInfo->ExceptionRecord->ExceptionCode == -529697949)
	{
		printf("Out of memory. Exiting silently.\n");
		exit(-1);
	}

	// firstly see if dbghelp.dll is around and has the function we need
	// look next to the EXE first, as the one in System32 might be old 
	// (e.g. Windows 2000)
	HMODULE hDll = NULL;
	char szDbgHelpPath[_MAX_PATH];

	if (GetModuleFileName( NULL, szDbgHelpPath, _MAX_PATH ))
	{
		char *pSlash = _tcsrchr( szDbgHelpPath, '\\' );
		if (pSlash)
		{
			_tcscpy( pSlash+1, "DBGHELP.DLL" );
			hDll = ::LoadLibrary( szDbgHelpPath );
		}
	}

	if (hDll==NULL)
	{
		// load any version we can
		hDll = ::LoadLibrary( "DBGHELP.DLL" );
	}

	LPCTSTR szResult = NULL;

	if (hDll)
	{
		MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress( hDll, "MiniDumpWriteDump" );
		if (pDump)
		{
			char szDumpPath[_MAX_PATH];
			char szScratch [_MAX_PATH];

			// work out a good place for the dump file
//			if (!GetTempPath( _MAX_PATH, szDumpPath ))
				_tcscpy( szDumpPath, "c:\\" );

			_tcscat( szDumpPath, m_szAppName );
			_tcscat( szDumpPath, ".dmp" );

			// create the file
			HANDLE hFile = ::CreateFile( szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
										FILE_ATTRIBUTE_NORMAL, NULL );

			if (hFile!=INVALID_HANDLE_VALUE)
			{
				_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

				ExInfo.ThreadId = ::GetCurrentThreadId();
				ExInfo.ExceptionPointers = pExceptionInfo;
				ExInfo.ClientPointers = NULL;

				// write the dump
				BOOL bOK = pDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpWithDataSegs /*MiniDumpNormal*/, &ExInfo, NULL, NULL );
				if (bOK)
				{
					sprintf( szScratch, "Saved minidump file to '%s'.", szDumpPath);
					szResult = szScratch;
					retval = EXCEPTION_EXECUTE_HANDLER;
				}
				else
				{
					sprintf( szScratch, "Failed to save dump file to '%s' (error %d)", szDumpPath, GetLastError() );
					szResult = szScratch;
				}
				::CloseHandle(hFile);
			}
			else
			{
				sprintf( szScratch, "Failed to create dump file '%s' (error %d)", szDumpPath, GetLastError() );
				szResult = szScratch;
			}

			szResult = NULL;

			{
				printf("An error occured in the xFarm tool.\n");
			}
		}
		else
		{
			szResult = "DBGHELP.DLL too old";
		}
	}
	else
	{
		szResult = "DBGHELP.DLL not found";
	}

	if (szResult)
	{
		printf(szResult);
	}

	// create process to re-start the farm server after a crash
	{
		PROCESS_INFORMATION	Pi;
		STARTUPINFO			Si;

		ZeroMemory(&Pi, sizeof(Pi));
		ZeroMemory(&Si, sizeof(Si));

		Si.cb = sizeof(Si);

		CreateProcess(0, GetCommandLine(), 0, 0, FALSE, 0, 0, 0, &Si, &Pi);
	}
	ExitProcess(-1); // exit this process (which has crashed), but keep running the new child process

	return retval;
}

