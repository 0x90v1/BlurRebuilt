#include "farmpch.h"

#include "farm/Farm.h"

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
		fprintf(stderr, "Job %i successfully requeued.\r\n", atoi(Command + 9));
	else if (!strnicmp(Command, "BADJOB ", 7))
		fprintf(stderr, "Couldn't find job %i.\r\n", atoi(Command + 7));
	else
		fprintf(stderr, "Unknown message received, job has probably failed.\r\n");

	return false;
}

bool FarmRequeue::OnIdle()
{
	return true;
}