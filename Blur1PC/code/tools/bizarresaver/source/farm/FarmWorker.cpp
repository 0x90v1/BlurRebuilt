#include "farmpch.h"
#include "farm/Farm.h"

HANDLE FarmWorker::s_Process(INVALID_HANDLE_VALUE);

FarmWorker::FarmWorker() :
		m_State(StateFree),
		m_CmdPtr(0)
{
}

FarmWorker::~FarmWorker()
{
}

bool FarmWorker::OnStart()
{
	if (!AnnounceMachine("WORKER"))
		return false;

	m_State = (s_Process != INVALID_HANDLE_VALUE) ? StateOccupied : StateFree;

	return true;
}

bool FarmWorker::OnStop()
{
	return Send("QUIT");
} 

bool FarmWorker::OnCommand(const char* Command)
{
	if (!strnicmp(Command, "JOB ", 4))
	{
		if (m_State == StateFree)
		{
			m_State = StateAccepted;

			m_Command	= Command + 4;
			m_CmdPtr	= m_Command;

			StartProcess();

			if (!Send("ACCEPTED"))
				return false;
		}
		else
		{
			if (!Send("IGNORED"))
				return false;
		}
	}
	else if (!stricmp(Command, "KILL"))
	{
		StopProcess();

		m_State = StateFree;

		if (!Send("CANCELLED"))
			return false;
	}
	else if (!stricmp(Command, "DIE"))
	{
		exit(-1);
	}
	else
		return false;

	return true;
}

bool FarmWorker::OnIdle()
{
	if (gbKillWorker)
	{
		StopProcess();
		return false;
	}

	if ((m_State == StateOccupied) || (m_State == StateAccepted))
	{
		if (IsProcessAlive())
		{
			if (!Send("BUSY"))
				return false;
		}
		else
		{
			// get the exit code
			DWORD iExitCode = -1;
			GetExitCodeProcess(s_Process, &iExitCode);

			StopProcess();

			if (m_CmdPtr && *m_CmdPtr)
			{
				StartProcess();
			}
			else
			{
				if (m_State == StateAccepted)
				{
					if (iExitCode == 0)
					{
						if (!Send("FINISHED"))
							return false;
					}
					else
					{
						if (!Send("FAILED %d",iExitCode))
							return false;
					}
				}
				else
				{
					if (!Send("FREE"))
						return false;
				}

				m_State = StateFree;
			}
		}
	}
	else
	{
		if (!Send("FREE"))
			return false;
	}

	return true;
}

DWORD FarmWorker::GetIdleTime() const
{
	return 2500;
}

char gCmdLineBuffer[4096] = "";

void FarmWorker::StartProcess()
{
	PROCESS_INFORMATION	Pi;
	STARTUPINFOW		Si;

	ZeroMemory(&Pi, sizeof(Pi));
	ZeroMemory(&Si, sizeof(Si));

	Si.cb = sizeof(Si);

	Si.wShowWindow = SW_SHOWNOACTIVATE;
	Si.dwFlags = STARTF_USESHOWWINDOW;

	gbJobRunning = true;

	while(*m_CmdPtr)
	{
		char Buffer[4096];

		char* Ptr = Buffer;
		for(Ptr ; *m_CmdPtr && (Ptr < (Buffer + sizeof(Buffer) - 1)); )
		{
			if (strnicmp(m_CmdPtr, "$$next$$", 8) == 0)
			{
				m_CmdPtr += strlen("$$next$$");
				break;
			}
			else
				*Ptr++ = *m_CmdPtr++;
		}

		*Ptr = '\0';

		if (*Buffer)
		{
			strcpy(gCmdLineBuffer, Buffer);
			wchar_t SomeUnicodeStr[1024];
			MultiByteToWideChar(CP_ACP, 0, Buffer, -1, SomeUnicodeStr, 1024);

			//if (CreateProcessWithLogonW(L"farm", L"Atvi", L"Password_1", LOGON_NETCREDENTIALS_ONLY,
			//	NULL, SomeUnicodeStr, 0,
			//	NULL, NULL, &Si, &Pi) != FALSE)
			if (CreateProcessW(0, SomeUnicodeStr, 0, 0, FALSE, 0, 0, 0, &Si, &Pi) != FALSE)
			{
				s_Process = Pi.hProcess;

				CloseHandle(Pi.hThread);
			}

			break;
		}
	}
}

void FarmWorker::StopProcess()
{
	gbJobRunning = false;

	if (s_Process != INVALID_HANDLE_VALUE)
	{
		TerminateProcess(s_Process, 0);
		CloseHandle(s_Process);
		s_Process = INVALID_HANDLE_VALUE;
	}
}

bool FarmWorker::IsProcessAlive() const
{
	if (s_Process == INVALID_HANDLE_VALUE)
		return false;

	return WaitForSingleObject(s_Process, 0) == WAIT_TIMEOUT;
}