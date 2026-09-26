#include "StdAfx.h"
#include "SpawnProcess.h"
#include "Win32KillProcessTree.h"
#ifndef WIN32
#include "sys/wait.h"
#include <errno.h>
#endif

SpawnProcess::SpawnProcess(const char *psz) :
	m_iExitCode(0),
	m_iSpawnProcess(0),
	m_bActive(true)
{
	strcpy(m_pszCommandLine, psz);
}

bool SpawnProcess::IsActive()
{
	return m_bActive;
}

#ifndef WIN32
bool ActiveTest( int processId )
{
	if ( kill ( processId, 0 ) == -1 && errno == ESRCH )
		return false;
	else
		return true;
}
#endif

void SpawnProcess::ThreadMain()
{
#ifdef WIN32
	// start the process using windows' createprocess function
	PROCESS_INFORMATION	Pi;
	STARTUPINFO			Si;

	ZeroMemory(&Pi, sizeof(Pi));
	ZeroMemory(&Si, sizeof(Si));

	Si.cb = sizeof(Si);

	if (CreateProcess(0, m_pszCommandLine, 0, 0, FALSE, 0, 0, 0, &Si, &Pi) != FALSE)
	{
		m_iSpawnProcess = (int)Pi.hProcess;
		m_iPID = (int)Pi.dwProcessId;
		WaitForSingleObject( Pi.hProcess, INFINITE);
		GetExitCodeProcess( Pi.hProcess, (LPDWORD)&m_iExitCode);
	}
	else
		m_iExitCode = -1;
	Sleep(5*1000);
#else
	// the GCC version is complicated, because we have to split all the arguments into separate strings
	char *cmd_line = (char *) malloc(strlen(m_pszCommandLine ) + 1 );
	char *exec_path_name = m_pszCommandLine;
	int pid = -1;
	int *processId = &pid;

	char *_cmd_line = m_pszCommandLine;
	while (*_cmd_line != ' ' && *_cmd_line != 0)
		_cmd_line++;
	*_cmd_line = 0;
	_cmd_line++;

	if(cmd_line == NULL)
	{
		m_iExitCode = -1;
		return;
	}

#define WR_MAX_ARG 32

	strcpy(cmd_line, _cmd_line );

	if( ( *processId = fork() ) == 0 )// Create child
	{
		char*pArg, *pPtr;
		char*argv[WR_MAX_ARG + 1];
		int argc;
		if( ( pArg = strrchr( exec_path_name, '/' ) ) != NULL )
			pArg++;
		else
			pArg = exec_path_name;
		argv[0] = pArg;
		argc = 1;

		if( cmd_line != NULL && *cmd_line != '\0' )
		{

			pArg = strtok_r(cmd_line, " ", &pPtr);

			while( pArg != NULL )
			{
				argv[argc] = pArg;
				argc++;
				if( argc >= WR_MAX_ARG )
					break;
				pArg = strtok_r(NULL, " ", &pPtr);
			}
		}
		argv[argc] = NULL;

		execv(exec_path_name, argv);
		free(cmd_line);

		::exit( -1 );
	}
	else if( *processId == -1 )
	{
		*processId = 0;
		free(cmd_line);
	}

	m_iSpawnProcess = 0;
	m_iPID = *processId;

	// wait for child process to terminate
	int child_status;
	::waitpid( *processId , &child_status, 0);

	// TODO: get exit code
#endif
	m_bActive = false;
}

int SpawnProcess::GetProcessHandle()
{
#ifdef WIN32
	return m_iSpawnProcess;
#else
	assert(0); // only supported on windows
	return 0;
#endif
}

int SpawnProcess::GetExitCode()
{
	return m_iExitCode;
}

void SpawnProcess::Terminate()
{
	// if the thread is not running then we don't have to do anything
	if (!IsRunning())
		return;

	// terminate our child process tree
#ifdef WIN32
	KillProcessEx(GetProcessId((HANDLE)m_iSpawnProcess), TRUE);
#else
	int d = kill(m_iPID, SIGKILL);
	if (d != 0)
	{
		printf("kill failed. errno = %d\n",  errno);
		// EINVAL
	}
#endif

	// once the process is terminated, the thread should exit automatically
	Join();
}
