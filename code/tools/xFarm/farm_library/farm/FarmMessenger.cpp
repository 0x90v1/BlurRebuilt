#include "StdAfx.h"
#include "Farm.h"
#include "FarmMessenger.h"

FarmMessenger::FarmMessenger() : m_jobNumber(-1), m_iJobStatus(0)
{
}

FarmMessenger::~FarmMessenger()
{
}

bool FarmMessenger::OnStart()
{
	if (!AnnounceUser("MANAGER"))
		return false;

	return Send("JOBSTATUS %d", m_jobNumber);
}

bool FarmMessenger::OnStop()
{
	return Send("QUIT");
}

bool FarmMessenger::OnCommand(const char* Command)
{
	if (!strnicmp(Command, "JOBSTATUS ", 10))
	{
		const char* Data(Command + 10);
		m_iJobStatus = SnapInt(Data);
	}
	else
		return true;	// received an unrecognized command - wait a bit longer

	return false;
}

bool FarmMessenger::OnIdle()
{
	assert(m_jobNumber != -1);

	return Send("JOBSTATUS %d", m_jobNumber);
}

int FarmMessenger::GetIdleTime() const
{
	return 500;
}

int FarmMessenger::GetJobStatus() const
{
	return m_iJobStatus;
}
