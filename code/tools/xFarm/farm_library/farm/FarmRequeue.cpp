#include "StdAfx.h"
#include "Farm.h"

FarmRequeue::FarmRequeue(int JobNo) :
		m_JobNo(JobNo)
{
}

FarmRequeue::~FarmRequeue()
{
}

bool FarmRequeue::OnStart()
{
	if (!AnnounceUser("MANAGER"))
		return false;

	return Send("REQUEUE %i", m_JobNo);
}

bool FarmRequeue::OnStop()
{
	return Send("QUIT");
}

bool FarmRequeue::OnCommand(const char* Command)
{
	if (!strnicmp(Command, "REQUEUED ", 9))
		FarmPrint("Job %i successfully requeued.\r\n", atoi(Command + 9));
	else if (!strnicmp(Command, "BADJOB ", 7))
		FarmPrint("Couldn't find job %i.\r\n", atoi(Command + 7));
	else
		FarmPrint("Unknown message received, job has probably failed.\r\n");

	return false;
}

bool FarmRequeue::OnIdle()
{
	return true;
}
