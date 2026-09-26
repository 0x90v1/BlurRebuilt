#include "farmpch.h"

#include "farm/Farm.h"

#ifdef _DEBUG
#define NOISY_NETWORKING
#endif

FarmProcess::FarmProcess(NwSocket& Skt, FarmStranger* Stranger, FarmServer* Server, int ProcessNo) :
		m_Socket(Skt.DetachSocket(), Skt.DetachEvent()),
		m_Thread(INVALID_HANDLE_VALUE),
		m_QuitThread(false),
		m_ReceiveTime(0),
		m_SendTime(0),
		m_Stranger(Stranger),
		m_Server(Server),
		m_ProcessNo(ProcessNo),
		m_Next(0),
		m_Previous(0),
		m_IsEnabled(true)
{
}

FarmProcess::~FarmProcess()
{
	Stop();
}

bool FarmProcess::Start()
{
	assert(m_Thread == INVALID_HANDLE_VALUE);

	return (m_Thread = CreateThread(0, 0, ThreadStartProc, this, 0, 0)) != INVALID_HANDLE_VALUE;
}

void FarmProcess::Stop()
{
	if (m_Thread != INVALID_HANDLE_VALUE)
	{
		m_QuitThread = true;

		for(;;)
		{
			SetEvent(m_Socket.GetEvent());

			if (WaitForSingleObject(m_Thread, 1000) != WAIT_TIMEOUT)
				break;
		}

		CloseHandle(m_Thread);

		m_Thread = INVALID_HANDLE_VALUE;

		m_Socket.Close();
	}

	if (m_Stranger)
	{
		delete m_Stranger;
		m_Stranger = 0;
	}

	m_QuitThread	= false;
	m_ReceiveTime	= 0;
	m_SendTime		= 0;
	m_Server		= 0;
	m_ProcessNo		= -1;
	m_UserName		= "";
}

bool FarmProcess::IsRunning(int Sleep) const
{
	if (m_Thread == INVALID_HANDLE_VALUE)
		return false;

	return WaitForSingleObject(m_Thread, Sleep) == WAIT_TIMEOUT;
}

bool FarmProcess::IsEnabled() const
{
	return m_IsEnabled;
}

const char* FarmProcess::GetUserName() const
{
	return m_UserName;
}

bool FarmProcess::Send(const char* Command)
{
	if (!Command)
		return false;

	if (!*Command)
		return true;

	int Length = (int)strlen(Command);
	int x;
	x = htonl(Length);

	if (m_Socket.Send(&x, sizeof(Length)) < 0)
		return false;

	if (m_Socket.Send(Command, Length) < 0)
		return false;

#ifdef NOISY_NETWORKING
	fprintf(stderr, "(%i) SEND %s\r\n", m_ProcessNo, Command);
	DebugInfo("(%i) SEND %s", m_ProcessNo, Command);
#endif

	m_SendTime = GetTickCount();

	return true;
}

void FarmProcess::SetEnabled(bool Enabled)
{
	m_IsEnabled = Enabled;
}

int FarmProcess::AddJob(const char* Command, const char* Comment)
{
	if (!m_Server)
		return -1;

	return m_Server->AddJob(m_UserName, Command, Comment);
}

bool FarmProcess::KillJob(int JobNo)
{
	if (!m_Server)
		return false;

	return m_Server->KillJob(JobNo);
}

void FarmProcess::KillMyJobs()
{
	if (m_Server)
		m_Server->KillMyJobs(m_ProcessNo);
}

int FarmProcess::GetFreeJob(char* Buffer, int MaxLength)
{
	if (!m_Server)
		return -1;

	return m_Server->GetFreeJob(m_ProcessNo, Buffer, MaxLength);
}

void FarmProcess::AcceptJob(int JobNo)
{
	if (m_Server)
		m_Server->AcceptJob(JobNo);
}

void FarmProcess::CancelJob(int JobNo)
{
	if (m_Server)
		m_Server->CancelJob(JobNo);
}

void FarmProcess::FinishJob(int JobNo)
{
	if (m_Server)
		m_Server->FinishJob(JobNo);
}

void FarmProcess::FreeJob(int JobNo)
{
	if (m_Server)
		m_Server->FreeJob(JobNo);
}

bool FarmProcess::IsJobCancelled(int JobNo)
{
	if (!m_Server)
		return false;

	return m_Server->IsJobCancelled(JobNo);
}

bool FarmProcess::ListJobs()
{
	if (!m_Server)
		return false;

	return m_Server->ListJobs(this);
}

bool FarmProcess::ListMachines()
{
	if (!m_Server)
		return false;

	return m_Server->ListMachines(this);
}

bool FarmProcess::PriorityJob(int JobNo)
{
	if (!m_Server)
		return false;

	return m_Server->PriorityJob(JobNo);
}

bool FarmProcess::RequeueJob(int JobNo)
{
	if (!m_Server)
		return false;

	return m_Server->RequeueJob(JobNo);
}

int FarmProcess::GetJobVersion()
{
	return m_Server ? m_Server->GetJobVersion() : 0;
}

int FarmProcess::GetMachineVersion()
{
	return m_Server ? m_Server->GetMachineVersion() : 0;
}

bool FarmProcess::UpdateCommand(int JobNo, const char* Command)
{
	if (!m_Server)
		return false;

	return m_Server->UpdateCommand(JobNo, Command);
}

bool FarmProcess::UpdateComment(int JobNo, const char* Comment)
{
	if (!m_Server)
		return false;

	return m_Server->UpdateComment(JobNo, Comment);
}

bool FarmProcess::EnableMachine(int ProcessNo)
{
	if (!m_Server)
		return false;

	return m_Server->EnableMachine(ProcessNo);
}

bool FarmProcess::DisableMachine(int ProcessNo)
{
	if (!m_Server)
		return false;

	return m_Server->DisableMachine(ProcessNo);
}

void FarmProcess::Process()
{
	char	Buffer[4096];
	char*	Ptr(0);
	int		TextLength(-1);
	int		RequiredLength(0);

	m_ReceiveTime	= GetTickCount();
	m_SendTime		= GetTickCount();

#ifdef NOISY_NETWORKING
	fprintf(stderr, "(%i) CONNECT\r\n", m_ProcessNo);
	DebugInfo("(%i) CONNECT", m_ProcessNo);
#endif

	if (m_Stranger)
		m_Stranger->Attach(this);

	DWORD LastIdle(0);

	if (!m_Stranger || m_Stranger->OnStart())
	{
		while(m_Socket.IsOpen() && !m_QuitThread)
		{
			int WaitResult;
#ifdef BIZARRE_SAVER
			// wait for the socket event, but unblock when the kill event occurs as well
			HANDLE handles[2];

			handles[0] = m_Socket.GetEvent();
			handles[1] = ghKillWorkerEvent;

			WaitResult = WaitForMultipleObjects(2, handles, FALSE, (m_Stranger ? m_Stranger->GetIdleTime() : 5000));
#else
			// normal code - just waits for the socket event
			WaitResult = WaitForSingleObject(m_Socket.GetEvent(), (m_Stranger ? m_Stranger->GetIdleTime() : 5000));
#endif

			if (m_Stranger)
			{
				if (gbKillWorker || (GetTickCount() - LastIdle) >= (m_Stranger ? m_Stranger->GetIdleTime() : 5000))
				{
					if (!m_Stranger->OnIdle())
						break;

					LastIdle = GetTickCount();
				}
			}

			if (WaitResult == WAIT_TIMEOUT)
			{
			}
			else if (WaitResult == WAIT_OBJECT_0)
			{
				WSANETWORKEVENTS Ne;

				if (!m_Socket.EnumNetworkEvents(Ne))
					break;

				if (Ne.lNetworkEvents & FD_READ)
				{
					if (Ne.iErrorCode[FD_READ_BIT] == 0)
					{
						m_ReceiveTime = GetTickCount();

						if ((TextLength == -1) && (Ptr == 0))
						{
							Ptr				= Buffer;
							RequiredLength	= sizeof(int);
						}

						int RecvLength(m_Socket.Receive(Ptr, RequiredLength - int(Ptr - Buffer)));

						if (RecvLength < 0)
							break;

						if (RecvLength > 0)
						{
							Ptr				+= RecvLength;
							RequiredLength	-= RecvLength;

							if (RequiredLength <= 0)
							{
								if (TextLength == -1)
								{
									TextLength = ntohl(*(int*)Buffer);

									if ((TextLength < 0) || (TextLength > (sizeof(Buffer) - 1)))
										break;

									Ptr				= Buffer;
									RequiredLength	= TextLength;
								}
								else
								{
									*Ptr = '\0';

#ifdef NOISY_NETWORKING
									fprintf(stderr, "(%i) RECV %s\r\n", m_ProcessNo, Buffer);
									DebugInfo("(%i) RECV %s", m_ProcessNo, Buffer);
#endif

									if (m_Stranger)
									{
										if (!m_Stranger->OnCommand(Buffer))
											break;
									}
									else
									{
										char* Data(Buffer);
										String Type(SnapAtom(Data));

										if (!stricmp(Type, "WORKER"))
											m_Stranger = new FarmAssistant;
#ifndef BIZARRE_SAVER
										else if (!stricmp(Type, "MANAGER"))
											m_Stranger = new FarmManager;
#endif

										if (m_Stranger)
											m_Stranger->Attach(this);

										if (!m_Stranger || !m_Stranger->OnStart())
											break;

										m_UserName = Data;
									}

									Ptr				= 0;
									RequiredLength	= 0;
									TextLength		= -1;
								}
							}
						}
					}
					else
						break;
				}

				if ((Ne.lNetworkEvents & FD_WRITE) && (Ne.iErrorCode[FD_WRITE_BIT] != 0))
					break;

				if (Ne.lNetworkEvents & FD_CLOSE)
					break;
			}
			else
				break;
		}
	}

	if (m_Stranger)
		m_Stranger->OnStop();
	else
		Send("QUIT");

#ifdef NOISY_NETWORKING
	fprintf(stderr, "(%i) CLOSE\r\n", m_ProcessNo);
	DebugInfo("(%i) CLOSE", m_ProcessNo);
#endif
}

DWORD CALLBACK FarmProcess::ThreadStartProc(void* Parameter)
{
	((FarmProcess*)Parameter)->Process();

	return 0;
}