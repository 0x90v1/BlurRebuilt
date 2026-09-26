#include "farmpch.h"
#include "farm/Farm.h"

FarmStranger::FarmStranger() :
		m_Process(0)
{
}

FarmStranger::~FarmStranger()
{
}

int FarmStranger::AddJob(const char* Command, const char* Comment)
{
	if (!m_Process)
		return -1;

	return m_Process->AddJob(Command, Comment);
}

bool FarmStranger::KillJob(int JobNo)
{
	if (!m_Process)
		return false;

	return m_Process->KillJob(JobNo);
}

void FarmStranger::KillMyJobs()
{
	if (m_Process)
		m_Process->KillMyJobs();
}

int FarmStranger::GetFreeJob(char* Buffer, int MaxLength)
{
	if (!m_Process)
		return -1;

	return m_Process->GetFreeJob(Buffer, MaxLength);
}

void FarmStranger::AcceptJob(int JobNo)
{
	if (m_Process)
		m_Process->AcceptJob(JobNo);
}

void FarmStranger::CancelJob(int JobNo)
{
	if (m_Process)
		m_Process->CancelJob(JobNo);
}

void FarmStranger::FinishJob(int JobNo)
{
	if (m_Process)
		m_Process->FinishJob(JobNo);
}

void FarmStranger::FreeJob(int JobNo)
{
	if (m_Process)
		m_Process->FreeJob(JobNo);
}

bool FarmStranger::IsJobCancelled(int JobNo)
{
	if (!m_Process)
		return false;

	return m_Process->IsJobCancelled(JobNo);
}

bool FarmStranger::ListJobs()
{
	if (!m_Process)
		return false;

	return m_Process->ListJobs();
}

bool FarmStranger::ListMachines()
{
	if (!m_Process)
		return false;

	return m_Process->ListMachines();
}

bool FarmStranger::PriorityJob(int JobNo)
{
	if (!m_Process)
		return false;

	return m_Process->PriorityJob(JobNo);
}

bool FarmStranger::RequeueJob(int JobNo)
{
	if (!m_Process)
		return false;

	return m_Process->RequeueJob(JobNo);
}

int FarmStranger::GetJobVersion()
{
	return m_Process ? m_Process->GetJobVersion() : 0;
}

int FarmStranger::GetMachineVersion()
{
	return m_Process ? m_Process->GetMachineVersion() : 0;
}

bool FarmStranger::UpdateCommand(int JobNo, const char* Command)
{
	if (!m_Process)
		return false;

	return m_Process->UpdateCommand(JobNo, Command);
}

bool FarmStranger::UpdateComment(int JobNo, const char* Comment)
{
	if (!m_Process)
		return false;

	return m_Process->UpdateComment(JobNo, Comment);
}

bool FarmStranger::EnableMachine(int MachineNo)
{
	if (!m_Process)
		return false;

	return m_Process->EnableMachine(MachineNo);
}

bool FarmStranger::DisableMachine(int MachineNo)
{
	if (!m_Process)
		return false;

	return m_Process->DisableMachine(MachineNo);
}

bool FarmStranger::IsEnabled()
{
	if (!m_Process)
		return false;

	return m_Process->IsEnabled();
}

bool FarmStranger::AnnounceUser(const char* Type)
{
	char Buffer[4096];
	ULONG Length(sizeof(Buffer));

	if (GetUserNameEx(NameSamCompatible, Buffer, &Length) == FALSE)
		return false;

	for(char* Ptr = Buffer; *Ptr; Ptr++)
		if (*Ptr == ' ')
			*Ptr = '_';

	return Send("%s %s", Type, Buffer);
}

bool FarmStranger::AnnounceMachine(const char* Type)
{
	char Buffer[4096];
	ULONG Length(sizeof(Buffer));

	if (GetComputerNameEx(ComputerNameNetBIOS, Buffer, &Length) == FALSE)
		return false;

	for(char* Ptr = Buffer; *Ptr; Ptr++)
		if (*Ptr == ' ')
			*Ptr = '_';

	int Flags = 1;//JOB_FLAG_WINDOWS;

	int iRetry = 6;
	while (iRetry>0)
	{
		if (Send("%s %s %d", Type, Buffer, Flags))
			return true;

		Sleep(500);
		iRetry--;
	}

	return true;
}

void FarmStranger::Attach(FarmProcess* Process)
{
	m_Process = Process;
}

bool FarmStranger::Send(const char* Command, ...)
{
	if (!m_Process)
		return false;

	char Buffer[4096];

	va_list vl;

	va_start(vl, Command);
	_vsnprintf(Buffer, sizeof(Buffer), Command, vl);
	va_end(vl);

	return m_Process->Send(Buffer);
}

bool FarmStranger::OnStart()
{
	return true;
}

bool FarmStranger::OnStop()
{
	return Send("QUIT");
}

bool FarmStranger::OnCommand(const char* Command)
{
	return true;
}

bool FarmStranger::OnIdle()
{
	return true;
}

StrangerType FarmStranger::GetType() const
{
	return ST_Generic;
}

DWORD FarmStranger::GetIdleTime() const
{
	return 5000;
}