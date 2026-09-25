#include "farmpch.h"

#include "farm/Farm.h"

FarmKill::FarmKill(int JobNo) :
		m_JobNo(JobNo)
{
}

FarmKill::~FarmKill()
{
}

bool FarmKill::OnStart()
{
	if (!AnnounceUser("MANAGER"))
		return false;

	return Send("KILL %i", m_JobNo);
}

bool FarmKill::OnStop()
{
	return Send("QUIT");
}

bool FarmKill::OnCommand(const char* Command)
{
	if (!strnicmp(Command, "CANCELLED ", 10))
		fprintf(stderr, "Job %i successfully cancelled.\r\n", atoi(Command + 10));
	else if (!strnicmp(Command, "BADJOB ", 7))
		fprintf(stderr, "Couldn't find job %i.\r\n", atoi(Command + 7));
	else
		fprintf(stderr, "Unknown message received, job has probably failed.\r\n");

	return false;
}

bool FarmKill::OnIdle()
{
	return true;
}