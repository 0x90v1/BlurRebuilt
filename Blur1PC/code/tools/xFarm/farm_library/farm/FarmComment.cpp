#include "StdAfx.h"
#include "Farm.h"

FarmComment::FarmComment(int JobNo, const char* Comment) :
		m_JobNo(JobNo),
		m_Comment(Comment)
{
}

FarmComment::~FarmComment()
{
}

bool FarmComment::OnStart()
{
	if (!AnnounceUser("MANAGER"))
		return false;

	return Send("COMMENT %i %s", m_JobNo, m_Comment);
}

bool FarmComment::OnStop()
{
	return Send("QUIT");
}

bool FarmComment::OnCommand(const char* Command)
{
	if (!strnicmp(Command, "UPDATED ", 8))
		FarmPrint("Job %i successfully updated.\r\n", atoi(Command + 8));
	else if (!strnicmp(Command, "BADJOB ", 7))
		FarmPrint("Couldn't find job %i.\r\n", atoi(Command + 7));
	else
		FarmPrint("Unknown message received, job has probably failed.\r\n");

	return false;
}

bool FarmComment::OnIdle()
{
	return true;
}
