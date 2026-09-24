#include "StdAfx.h"
#include "Farm.h"

FarmPriority::FarmPriority(int JobNo) :
		m_JobNo(JobNo)
{
}

FarmPriority::~FarmPriority()
{
}

bool FarmPriority::OnStart()
{
	if (!AnnounceUser("MANAGER"))
		return false;

	return Send("PRIORITY %i", m_JobNo);
}

bool FarmPriority::OnStop()
{
	return Send("QUIT");
}

bool FarmPriority::OnCommand(const char* Command)
{
	if (!strnicmp(Command, "PRIORITY ", 9))
		FarmPrint("Job %i successfully prioritised.\r\n", atoi(Command + 9));
	else if (!strnicmp(Command, "BADJOB ", 7))
		FarmPrint("Couldn't find job %i.\r\n", atoi(Command + 7));
	else
		FarmPrint("Unknown message received, job has probably failed.\r\n");

	return false;
}

bool FarmPriority::OnIdle()
{
	return true;
}
