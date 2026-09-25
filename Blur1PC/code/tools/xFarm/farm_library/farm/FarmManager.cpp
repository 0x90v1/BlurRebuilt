#include "StdAfx.h"
#include "Farm.h"

#include <system\DebugLog.h>

FarmingManager::FarmingManager()
{
}

FarmingManager::~FarmingManager()
{
}

bool FarmingManager::OnStart()
{
	return true;
}

bool FarmingManager::OnStop()
{
	return Send("QUIT");
}

using namespace xFarm;

// server-side code for interpreting commands that are received from a manager
// (commands like add job, delete job, get job list, get machine list, etc.)
bool FarmingManager::OnCommand(const char* Command)
{
	if (strnicmp(Command, "JOB ", 4) == 0)
	{
		const char* Data(Command + 4);

		int JobNo(AddJob(Data, GetProcessNo()));

		LOGDEBUG( g_Log, "%s: %s = %d", m_Process->GetMyUserName(), Command, JobNo);

		if (JobNo >= 0)
		{
			if (!Send("ACCEPTED %i", JobNo))
				return false;
		}
		else
		{
			if (!Send("FAILED"))
				return false;
		}
	}
	else if (strnicmp(Command, "KILL ", 5) == 0)
	{
		LOGDEBUG( g_Log, "%s: %s", m_Process->GetMyUserName(), Command);

		int JobNo(atoi(Command + 5));

		if (KillJob(JobNo))
		{
			if (!Send("CANCELLED %i", JobNo))
				return false;
		}
		else
		{
			if (!Send("BADJOB %i", JobNo))
				return false;
		}
	}
	else if (stricmp(Command, "LIST") == 0)
	{
		if (!Send("LIST START"))
			return false;

		if (!ListJobs())
			return false;

		if (!Send("LIST END"))
			return false;
	}
	else if (stricmp(Command, "MACHINES") == 0)
	{
		if (!Send("MACHINE START"))
			return false;

		if (!ListMachines())
			return false;

		if (!Send("MACHINE END"))
			return false;
	}
	else if (strnicmp(Command, "PRIORITY ", 9) == 0)
	{
		LOGDEBUG( g_Log, "%s: %s", m_Process->GetMyUserName(), Command);

		int JobNo(atoi(Command + 9));

		if (PriorityJob(JobNo))
		{
			if (!Send("PRIORITY %i", JobNo))
				return false;
		}
		else
		{
			if (!Send("BADJOB %i", JobNo))
				return false;
		}
	}
	else if (strnicmp(Command, "REQUEUE ", 8) == 0)
	{
		int JobNo(atoi(Command + 8));

		if (RequeueJob(JobNo))
		{
			if (!Send("REQUEUED %i", JobNo))
				return false;
		}
		else
		{
			if (!Send("BADJOB %i", JobNo))
				return false;
		}
	}
	else if (stricmp(Command, "VERSION") == 0)
	{
		if (!Send("VERSION %i %i", GetJobVersion(), GetMachineVersion()))
			return false;
	}
	else if (strnicmp(Command, "JOBSTATUS ", 10) == 0)
	{
		const char* Data(Command + 10);
		int JobNo(SnapInt(Data));

		long status = (IsJobInQueue(JobNo)) ? 1 : 0;

		if (!Send("JOBSTATUS %i", status))
			return false;
	}
	else if (strnicmp(Command, "COMMAND ", 8) == 0)
	{
		const char* Data(Command + 8);
		int JobNo(SnapInt(Data));

		if (UpdateCommand(JobNo, Data))
		{
			if (!Send("UPDATED %i", JobNo))
				return false;
		}
		else
		{
			if (!Send("BADJOB %i", JobNo))
				return false;
		}
	}
	else if (strnicmp(Command, "COMMENT ", 8) == 0)
	{
		const char* Data(Command + 8);
		int JobNo(SnapInt(Data));

		if (UpdateComment(JobNo, Data))
		{
			if (!Send("UPDATED %i", JobNo))
				return false;
		}
		else
		{
			if (!Send("BADJOB %i", JobNo))
				return false;
		}
	}
	else if (strnicmp(Command, "ENABLE ", 7) == 0)
	{
		LOGDEBUG( g_Log, "%s: %s", m_Process->GetMyUserName(), Command);

		int ProcessNo(atoi(Command + 7));

		if (EnableMachine(ProcessNo))
		{
			if (!Send("ENABLED %i", ProcessNo))
				return false;
		}
		else
		{
			if (!Send("BADMACHINE %i", ProcessNo))
				return false;
		}
	}
	else if (strnicmp(Command, "DISABLE ", 8) == 0)
	{
		LOGDEBUG( g_Log, "%s: %s", m_Process->GetMyUserName(), Command);

		int ProcessNo(atoi(Command + 8));

		if (DisableMachine(ProcessNo))
		{
			if (!Send("DISABLED %i", ProcessNo))
				return false;
		}
		else
		{
			if (!Send("BADMACHINE %i", ProcessNo))
				return false;
		}
	}
	else if (strnicmp(Command, "KILLBYPROC ", 8) == 0)
	{
		LOGDEBUG( g_Log, "%s: %s", m_Process->GetMyUserName(), Command);

		int iProcessNo = atoi(Command + 10);

		if (m_Process->GetServer()->KillJobByProcess( iProcessNo ))
		{
			if (!Send("OK"))
				return false;
		}
		else
		{
			if (!Send("FAIL"))
				return false;
		}
	}
	else if (strcmp(Command, "SHUTDOWN") == 0)
	{
		LOGDEBUG( g_Log, "%s: %s", m_Process->GetMyUserName(), Command);

		m_Process->SetCancel();
		m_Process->GetServer()->SetShutdown();
		if (!Send("OK"))
			return false;

		exit(0); // nasty - but it works
	}
	else if (strcmp(Command, "IDLE") == 0)
	{
		// do nothing
	}
	else
		return false;

	return true;
}

bool FarmingManager::OnIdle()
{
	return true;
}
