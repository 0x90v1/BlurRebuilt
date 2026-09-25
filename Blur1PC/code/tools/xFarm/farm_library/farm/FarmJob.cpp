
#include "StdAfx.h"

#include "Farm.h"

FarmJob::FarmJob(const char* Command, const char* Comment, int iFlags) :
		m_Command(Command),
		m_Comment(Comment),
		m_jobNumber(-1),
		m_Flags(iFlags)
{
}

FarmJob::~FarmJob()
{
}

bool FarmJob::OnStart()
{
	if (!AnnounceUser("MANAGER"))
		return false;

	return Send("JOB %i %s %i %s %i", strlen(m_Command), m_Command, strlen(m_Comment), m_Comment, m_Flags);
}

bool FarmJob::OnStop()
{
	return Send("QUIT");
}

bool FarmJob::OnCommand(const char* Command)
{
	if (!strnicmp(Command, "ACCEPTED ", 9))
	{
		m_jobNumber = atoi(Command + 9);

		FarmPrint("Job %i successfully added to queue.\r\n", m_jobNumber);
	}
	else
	{
		FarmPrint("Unknown message received, job has probably failed.\r\n");
	}

	return false;
}

bool FarmJob::OnIdle()
{
	return true;
}
