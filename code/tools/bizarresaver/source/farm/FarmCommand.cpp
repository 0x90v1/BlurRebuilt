#include "farmpch.h"
#include "farm/Farm.h"

FarmCommand::FarmCommand(int JobNo, const char* Command) :
		m_JobNo(JobNo),
		m_Command(Command)
{
}

FarmCommand::~FarmCommand()
{
}

bool FarmCommand::OnStart()
{
	if (!AnnounceUser("MANAGER"))
		return false;

	return Send("COMMAND %i %s", m_JobNo, m_Command);
}

bool FarmCommand::OnStop()
{
	return Send("QUIT");
}

bool FarmCommand::OnCommand(const char* Command)
{
	if (!strnicmp(Command, "UPDATED ", 8))
		fprintf(stderr, "Job %i successfully updated.\r\n", atoi(Command + 8));
	else if (!strnicmp(Command, "BADJOB ", 7))
		fprintf(stderr, "Couldn't find job %i.\r\n", atoi(Command + 7));
	else
		fprintf(stderr, "Unknown message received, job has probably failed.\r\n");

	return false;
}

bool FarmCommand::OnIdle()
{
	return true;
}