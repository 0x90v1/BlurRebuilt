#include "farmpch.h"

#include "farm/Farm.h"

FarmManager::FarmManager()
{
}

FarmManager::~FarmManager()
{
}

bool FarmManager::OnStart()
{
	return true;
}

bool FarmManager::OnStop()
{
	return Send("QUIT");
}

bool FarmManager::OnCommand(const char* Command)
{
	if (strnicmp(Command, "JOB ", 4) == 0)
	{
		const char* Data(Command + 4);
		String Command(SnapString(Data));
		String Comment(SnapString(Data));

		int JobNo(AddJob(Command, Comment));

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
	else
		return false;

	return true;
}

bool FarmManager::OnIdle()
{
	return true;
}