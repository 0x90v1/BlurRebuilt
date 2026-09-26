#include "farmpch.h"

#include "farm/Farm.h"

FarmJob::FarmJob(const char* Command, const char* Comment) :
		m_Command(Command),
		m_Comment(Comment)
{
}

FarmJob::~FarmJob()
{
}

bool FarmJob::OnStart()
{
	if (!AnnounceUser("MANAGER"))
		return false;

	return Send("JOB %i %s %i %s", strlen(m_Command), m_Command, strlen(m_Comment), m_Comment);
}

bool FarmJob::OnStop()
{
	return Send("QUIT");
}

bool FarmJob::OnCommand(const char* Command)
{
	if (!strnicmp(Command, "ACCEPTED ", 9))
		fprintf(stderr, "Job %i successfully added to queue.\r\n", atoi(Command + 9));
	else
		fprintf(stderr, "Unknown message received, job has probably failed.\r\n");

	return false;
}

bool FarmJob::OnIdle()
{
	return true;
}