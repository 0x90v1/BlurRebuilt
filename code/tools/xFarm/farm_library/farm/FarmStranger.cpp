#include "StdAfx.h"
#include "Farm.h"
#define SECURITY_WIN32
#ifdef WIN32
#include "security.h"
#endif

FarmStranger::FarmStranger() :
		m_Process(0)
{
}

FarmStranger::~FarmStranger()
{
}

int FarmStranger::AddJob(const char *Data, int iParentProcess)
{
	if (!m_Process)
		return -1;

	return m_Process->AddJob(Data, iParentProcess);
}

bool FarmStranger::KillJob(int JobNo)
{
	if (!m_Process)
		return false;

	return m_Process->KillJob(JobNo);
}

int FarmStranger::GetProcessNo()
{
	if (m_Process)
		return m_Process->GetProcessNo();
	return -1;
}


void FarmStranger::KillMyJobs()
{
	if (m_Process)
		m_Process->KillMyJobs();
}

int FarmStranger::GetFreeJob(char* Buffer, int MaxLength, int iFlags)
{
	if (!m_Process)
		return -1;

	return m_Process->GetFreeJob(Buffer, MaxLength, iFlags);
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

void FarmStranger::FinishJob(int JobNo, bool bOk)
{
	if (m_Process)
		m_Process->FinishJob(JobNo, bOk);
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

bool FarmStranger::IsJobInQueue(int JobNo)
{
	if (!m_Process)
		return false;

	return m_Process->IsJobInQueue(JobNo);
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

bool FarmStranger::IsActive()
{
	if (!m_Process)
		return false;

	return m_Process->IsActive();
}

bool FarmStranger::AnnounceUser(const char* Type)
{
	char Buffer[4096] = "<unknown>";
	unsigned long Length(sizeof(Buffer));

	gethostname(Buffer, 4096);

#ifdef WIN32
	GetUserNameEx(NameSamCompatible, Buffer, &Length);
#endif

	for(char* Ptr = Buffer; *Ptr; Ptr++)
		if (*Ptr == ' ')
			*Ptr = '_';

	return Send("%s %s", Type, Buffer);
}

bool FarmStranger::AnnounceMachine(const char* Type)
{
	char Buffer[1024];
	const char *psz;

	ost::IPV4Address localhost = m_Process->getLocal();
	psz = localhost.getHostname();

	strcpy(Buffer, psz);

	for(char* Ptr = Buffer; *Ptr; Ptr++)
		if (*Ptr == ' ')
			*Ptr = '_';

#ifdef WIN32
	int Flags = JOB_FLAG_WINDOWS | JOB_FLAG_PVS;
#else
	int Flags = JOB_FLAG_LINUX;
#endif

	int iRetry = 6;
	while (iRetry>0)
	{
		if (Send("%s %s %d", Type, Buffer, Flags))
			return true;

		MT::Thread::Sleep(500);
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
	vsnprintf(Buffer, sizeof(Buffer), Command, vl);
	va_end(vl);

	return m_Process->Send(Buffer);
}

void FarmStranger::SendRAW(void *pData, U32 c)
{
	m_Process->SendRAW(pData, c);
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

int FarmStranger::GetIdleTime() const
{
	return 5000;
}
