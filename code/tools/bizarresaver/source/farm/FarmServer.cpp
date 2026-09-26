#include "farmpch.h"
#include "farm/Farm.h"

const char*	DiscoverServerQuery		= "Server, Where Are You!";
const char*	DiscoverServerResponse	= "Client, I'm Over Here!";

FarmServer::FarmServer() :
		m_Jobs(0),
		m_JobNo(0),
		m_Processes(0),
		m_JobVersion(0),
		m_MachineVersion(0)
{
	InitializeCriticalSection(&m_Critical);
}

FarmServer::~FarmServer()
{
	DeleteCriticalSection(&m_Critical);
}

bool FarmServer::Run()
{
	NwSocket Svr;

	if (!Svr.Listen(NwSockAddr(INADDR_ANY, NetworkPort)))
		return false;

	HANDLE Advertise(CreateThread(0, 0, AdvertiseServerProc, this, 0, 0));

	if (Advertise == INVALID_HANDLE_VALUE)
		return false;

	CloseHandle(Advertise);

	int ProcessNo(0);

	for(;;)
	{
		int WaitResult(WaitForSingleObject(Svr.GetEvent(), 1000));

		if (WaitResult == WAIT_TIMEOUT)
		{
			EnterCriticalSection(&m_Critical);

			if (FarmProcess* Prc = m_Processes)
			{
				do
				{
					FarmProcess* Next = (Prc == Prc->m_Next) ? 0 : Prc->m_Next;

					if (!Prc->IsRunning())
					{
						Prc->Stop();

						m_MachineVersion++;

						delete DetachProcess(Prc);
					}

					Prc = Next;
				}
				while(Prc && (Prc != m_Processes));
			}

			LeaveCriticalSection(&m_Critical);
		}
		else if (WaitResult == WAIT_OBJECT_0)
		{
			WSANETWORKEVENTS Ne;

			if (!Svr.EnumNetworkEvents(Ne))
				return false;

			if (Ne.lNetworkEvents & FD_ACCEPT)
			{
				if (Ne.iErrorCode[FD_ACCEPT_BIT] == 0)
				{
					NwSocket Skt;

					if (Svr.Accept(Skt))
					{
						EnterCriticalSection(&m_Critical);

						if (FarmProcess* Prc = new FarmProcess(Skt, 0, this, ProcessNo++))
							if (Prc->Start())
								AttachProcess(Prc);
							else
								delete Prc;

						m_MachineVersion++;

						LeaveCriticalSection(&m_Critical);
					}
				}
			}
		}
		else
			return false;
	}

	return true;
}

int FarmServer::AddJob(const char* User, const char* Command, const char* Comment)
{
	EnterCriticalSection(&m_Critical);

	int JobNo(m_JobNo++);

	if (Job* Jb = new Job)
	{
		Jb->JobNo	= JobNo;
		Jb->User	= User;
		Jb->Command	= Command;
		Jb->Comment	= Comment;

		m_JobVersion++;

		AttachJob(Jb);
	}
	else
		JobNo = -1;

	LeaveCriticalSection(&m_Critical);

	return JobNo;
}

bool FarmServer::KillJob(int JobNo)
{
	bool Result(false);

	EnterCriticalSection(&m_Critical);

	if (Job* Jb = FindJob(JobNo))
	{
		if (Jb->State == JobStateFree)
			delete DetachJob(Jb);
		else
			Jb->State = JobStateKilled;

		m_JobVersion++;

		Result = true;
	}

	LeaveCriticalSection(&m_Critical);

	return Result;
}

void FarmServer::KillMyJobs(int ProcessNo)
{
	EnterCriticalSection(&m_Critical);

	if (Job* Jb = m_Jobs)
	{
		do
		{
			Job* Next = (Jb == Jb->Next) ? 0 : Jb->Next;

			if (Jb->ProcessNo == ProcessNo)
			{
				switch(Jb->State)
				{
					case JobStateAllocated:
					case JobStateKilled:
						delete DetachJob(Jb);
					break;

					case JobStatePending:
						Jb->ProcessNo	= -1;
						Jb->State		= JobStateFree;
					break;

					case JobStateRequeue:
						Jb->ProcessNo	= -1;
						Jb->State		= JobStateFree;

						AttachJob(DetachJob(Jb));
					break;
				}

				break;
			}

			Jb = Next;
		}
		while(Jb && (Jb != m_Jobs));
	}

	m_JobVersion++;

	LeaveCriticalSection(&m_Critical);
}

int FarmServer::GetFreeJob(int ProcessNo, char* Buffer, int MaxLength)
{
	int Result(-1);

	EnterCriticalSection(&m_Critical);

	if (Job* Jb = m_Jobs)
	{
		do
		{
			if (Jb->State == JobStateFree)
			{
				Jb->State		= JobStatePending;
				Jb->ProcessNo	= ProcessNo;
				Result			= Jb->JobNo;

				_snprintf(Buffer, MaxLength, "%s", Jb->Command);

				m_JobVersion++;

				break;
			}
		}
		while((Jb = Jb->Next) != m_Jobs);
	}

	LeaveCriticalSection(&m_Critical);

	return Result;
}

void FarmServer::AcceptJob(int JobNo)
{
	EnterCriticalSection(&m_Critical);

	if (Job* Jb = FindJob(JobNo))
	{
		if (Jb->State == JobStatePending)
		{
			Jb->State		= JobStateAllocated;
			Jb->TimeStarted	= GetTickCount();

			m_JobVersion++;
		}
	}

	LeaveCriticalSection(&m_Critical);
}

void FarmServer::CancelJob(int JobNo)
{
	EnterCriticalSection(&m_Critical);

	if (Job* Jb = FindJob(JobNo))
	{
		switch(Jb->State)
		{
			case JobStateKilled:
				delete DetachJob(Jb);
			break;

			case JobStateRequeue:
				Jb->ProcessNo	= -1;
				Jb->State		= JobStateFree;

				AttachJob(DetachJob(Jb));
			break;
		}

		m_JobVersion++;
	}

	LeaveCriticalSection(&m_Critical);
}

void FarmServer::FinishJob(int JobNo)
{
	EnterCriticalSection(&m_Critical);

	if (Job* Jb = FindJob(JobNo))
	{
		delete DetachJob(Jb);

		m_JobVersion++;
	}

	LeaveCriticalSection(&m_Critical);
}

void FarmServer::FreeJob(int JobNo)
{
	EnterCriticalSection(&m_Critical);

	if (Job* Jb = FindJob(JobNo))
	{
		if (Jb->State != JobStateKilled)
		{
			Jb->ProcessNo	= -1;
			Jb->State		= JobStateFree;

			m_JobVersion++;
		}
		else
			CancelJob(JobNo);
	}

	LeaveCriticalSection(&m_Critical);
}

bool FarmServer::IsJobCancelled(int JobNo)
{
	bool Result(false);

	EnterCriticalSection(&m_Critical);

	if (Job* Jb = FindJob(JobNo))
		Result = (Jb->State == JobStateKilled) || (Jb->State == JobStateRequeue);

	LeaveCriticalSection(&m_Critical);

	return Result;
}

bool FarmServer::ListJobs(FarmProcess* Prc)
{
	bool Result(true);

	EnterCriticalSection(&m_Critical);

	if (Job* Jb = m_Jobs)
	{
		int Idx(0);

		do
		{
			if (Jb->State != JobStateKilled)
			{
				char Buffer[4096];

				_snprintf(Buffer, sizeof(Buffer),
						"JOB %i %i %c %i %i %s %i %s %i %s",
						Idx++,
						Jb->JobNo,
						(Jb->State == JobStateFree) ? 'F' : 'A',
						Jb->ProcessNo,
						Jb->TimeStarted,
						Jb->User,
						strlen(Jb->Command),
						Jb->Command,
						strlen(Jb->Comment),
						Jb->Comment
					);

				if (!Prc->Send(Buffer))
				{
					Result = false;
					break;
				}
			}
		}
		while((Jb = Jb->Next) != m_Jobs);
	}

	LeaveCriticalSection(&m_Critical);

	return Result;
}

bool FarmServer::ListMachines(FarmProcess* ManagerPrc)
{
	bool Result(true);

	EnterCriticalSection(&m_Critical);

	if (FarmProcess* Prc = m_Processes)
	{
		do
		{
			if (Prc->m_Stranger && (Prc->m_Stranger->GetType() == ST_Assistant))
			{
				char Buffer[4096];

				_snprintf(Buffer, sizeof(Buffer),
						"MACHINE %i %s %i",
						Prc->m_ProcessNo,
						Prc->m_UserName,
						Prc->m_IsEnabled
					);

				if (!ManagerPrc->Send(Buffer))
				{
					Result = false;
					break;
				}
			}
		}
		while((Prc = Prc->m_Next) != m_Processes);
	}

	LeaveCriticalSection(&m_Critical);

	return Result;
}

bool FarmServer::PriorityJob(int JobNo)
{
	bool Result(false);

	EnterCriticalSection(&m_Critical);

	if (Job* Jb = FindJob(JobNo))
	{
		if (m_Jobs != Jb)
		{
			m_Jobs = AttachJob(DetachJob(Jb));

			m_JobVersion++;
		}

		Result = true;
	}

	LeaveCriticalSection(&m_Critical);

	return Result;
}

bool FarmServer::RequeueJob(int JobNo)
{
	bool Result(false);

	EnterCriticalSection(&m_Critical);

	if (Job* Jb = FindJob(JobNo))
	{
		if (Jb->State == JobStateFree)
			AttachJob(DetachJob(Jb));
		else
			Jb->State = JobStateRequeue;

		m_JobVersion++;

		Result = true;
	}

	LeaveCriticalSection(&m_Critical);

	return Result;
}

int FarmServer::GetJobVersion()
{
	return m_JobVersion;
}

int FarmServer::GetMachineVersion()
{
	return m_MachineVersion;
}

bool FarmServer::UpdateCommand(int JobNo, const char* Command)
{
	bool Result(false);

	EnterCriticalSection(&m_Critical);

	if (Job* Jb = FindJob(JobNo))
	{
		Jb->Command = Command;

		m_JobVersion++;

		Result = true;
	}

	LeaveCriticalSection(&m_Critical);

	return Result;
}

bool FarmServer::UpdateComment(int JobNo, const char* Comment)
{
	bool Result(false);

	EnterCriticalSection(&m_Critical);

	if (Job* Jb = FindJob(JobNo))
	{
		Jb->Comment = Comment;

		m_JobVersion++;

		Result = true;
	}

	LeaveCriticalSection(&m_Critical);

	return Result;
}

bool FarmServer::EnableMachine(int ProcessNo)
{
	bool Result(false);

	EnterCriticalSection(&m_Critical);

	if (FarmProcess* Prc = FindProcess(ProcessNo))
	{
		Prc->SetEnabled(true);
		m_MachineVersion++;
		Result = true;
	}

	LeaveCriticalSection(&m_Critical);

	return Result;
}

bool FarmServer::DisableMachine(int ProcessNo)
{
	bool Result(false);

	EnterCriticalSection(&m_Critical);

	if (FarmProcess* Prc = FindProcess(ProcessNo))
	{
		Prc->SetEnabled(false);
		m_MachineVersion++;
		Result = true;
	}

	LeaveCriticalSection(&m_Critical);

	return Result;
}

bool FarmServer::AdvertiseServer()
{
	NwSocket Skt;

	if (!Skt.Broadcast(BroadcastPort))
		return false;

	for(;;)
	{
		if (WaitForSingleObject(Skt.GetEvent(), INFINITE) == WAIT_OBJECT_0)
		{
			WSANETWORKEVENTS Ne;

			if (!Skt.EnumNetworkEvents(Ne))
				return false;

			if (Ne.lNetworkEvents & FD_READ)
			{
				if (Ne.iErrorCode[FD_READ_BIT] != 0)
					return false;

				NwSockAddr	Where;
				char		Buffer[512];

				if (Skt.ReceiveFrom(Buffer, sizeof(Buffer), Where) == strlen(DiscoverServerQuery))
				{
					if (strncmp(Buffer, DiscoverServerQuery, strlen(DiscoverServerQuery)) == 0)
					{
						Skt.SendTo(
								DiscoverServerResponse,
								(int)strlen(DiscoverServerResponse),
								NwSockAddr(INADDR_BROADCAST, BroadcastPort)
							);
					}
				}
			}
		}
	}
}

NwSockAddr FindServer()
{
	NwSocket Skt;

	if (!Skt.Broadcast(BroadcastPort))
		return NwSockAddr();

	DWORD	NextSendTime	= 0;
	int		Retries			= 5;

	for(;;)
	{
		if (NextSendTime < GetTickCount())
		{
			if (Retries-- <= 0)
				return NwSockAddr();

			if (Skt.SendTo(DiscoverServerQuery, (int)strlen(DiscoverServerQuery), NwSockAddr(INADDR_BROADCAST, BroadcastPort)) < 0)
				return NwSockAddr();

			NextSendTime = GetTickCount() + 200;
		}

		if (WaitForSingleObject(Skt.GetEvent(), 200) == WAIT_OBJECT_0)
		{
			WSANETWORKEVENTS Ne;

			if (!Skt.EnumNetworkEvents(Ne))
				return false;

			if (Ne.lNetworkEvents & FD_READ)
			{
				if (Ne.iErrorCode[FD_READ_BIT] != 0)
					return NwSockAddr();

				NwSockAddr	Where;
				char		Buffer[512];

				if (Skt.ReceiveFrom(Buffer, sizeof(Buffer), Where) == strlen(DiscoverServerResponse))
				{
					if (strncmp(Buffer, DiscoverServerResponse, strlen(DiscoverServerResponse)) == 0)
						return NwSockAddr(Where.GetAddr(), NetworkPort);
				}
			}
		}
	}
}

DWORD CALLBACK FarmServer::AdvertiseServerProc(LPVOID Parameter)
{
	return ((FarmServer*)Parameter)->AdvertiseServer();
}

FarmServer::Job* FarmServer::AttachJob(Job* Jb)
{
	if (Jb)
	{
		assert((Jb->Next == 0) && (Jb->Previous == 0));

		if (m_Jobs)
		{
			Jb->Next			= m_Jobs;
			Jb->Previous		= m_Jobs->Previous;
			Jb->Next->Previous	= Jb;
			Jb->Previous->Next	= Jb;
		}
		else
			m_Jobs = Jb->Next = Jb->Previous = Jb;
	}

	return Jb;
}

FarmServer::Job* FarmServer::DetachJob(Job* Jb)
{
	if (Jb)
	{
		assert((Jb->Next != 0) && (Jb->Previous != 0));

		if ((Jb == m_Jobs) && ((m_Jobs = m_Jobs->Next) == Jb))
			m_Jobs = Jb->Next = Jb->Previous = 0;
		else
		{
			Jb->Next->Previous = Jb->Previous;
			Jb->Previous->Next = Jb->Next;

			Jb->Next = Jb->Previous = 0;
		}
	}

	return Jb;
}

FarmServer::Job* FarmServer::FindJob(int JobNo)
{
	if (Job* Jb = m_Jobs)
	{
		do
		{
			if (Jb->JobNo == JobNo)
				return Jb;
		}
		while((Jb = Jb->Next) != m_Jobs);
	}

	return 0;
}

FarmProcess* FarmServer::AttachProcess(FarmProcess* Prc)
{
	if (Prc)
	{
		assert((Prc->m_Next == 0) && (Prc->m_Previous == 0));

		if (m_Processes)
		{
			Prc->m_Next			= m_Processes;
			Prc->m_Previous		= m_Processes->m_Previous;
			Prc->m_Next->m_Previous	= Prc;
			Prc->m_Previous->m_Next	= Prc;
		}
		else
			m_Processes = Prc->m_Next = Prc->m_Previous = Prc;
	}

	return Prc;
}

FarmProcess* FarmServer::DetachProcess(FarmProcess* Prc)
{
	if (Prc)
	{
		assert((Prc->m_Next != 0) && (Prc->m_Previous != 0));

		if ((Prc == m_Processes) && ((m_Processes = m_Processes->m_Next) == Prc))
			m_Processes = Prc->m_Next = Prc->m_Previous = 0;
		else
		{
			Prc->m_Next->m_Previous = Prc->m_Previous;
			Prc->m_Previous->m_Next = Prc->m_Next;

			Prc->m_Next = Prc->m_Previous = 0;
		}
	}

	return Prc;
}

FarmProcess* FarmServer::FindProcess(int ProcessNo)
{
	if (FarmProcess* Prc = m_Processes)
	{
		do
		{
			if (Prc->m_ProcessNo == ProcessNo)
				return Prc;
		}
		while((Prc = Prc->m_Next) != m_Processes);
	}

	return 0;
}