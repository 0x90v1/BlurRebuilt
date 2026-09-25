#include "StdAfx.h"
#include "Farm.h"
#include "System\Time\Time.h"

using namespace xFarm;

//#define NOISY_NETWORKING
bool g_bScreenSaverMode = false;

FarmProcess::FarmProcess(ost::TCPSocket &Skt, FarmStranger* Stranger, FarmingServer* Server, int ProcessNo, ost::SocketService *service) :
		SocketPort( NULL, Skt ), // wait for incoming connection on this socket
		m_ReceiveTime(0),
		m_SendTime(0),
		m_Stranger(Stranger),
		m_Server(Server),
		m_ProcessNo(ProcessNo),
		m_Next(0),
		m_Previous(0),
		m_Flags(JOB_FLAG_WINDOWS),
		m_IsEnabled(true),
		m_bIsExecuter(false),
		m_IsActive(true),
		m_SendTimeout(0),
		m_bDeleteStranger(true)
{
	CommonInit();

	if (m_Stranger)
		m_Stranger->Attach(this);

	attach( service );

	if (Stranger)
		Stranger->OnStart();

	setDetectOutput( false );
}

FarmProcess::FarmProcess(ost::IPV4Host &Skt, int iPort, FarmStranger* Stranger, FarmingServer* Server, int ProcessNo, ost::SocketService *service, bool bDeleteStranger) :
		SocketPort( NULL, Skt, iPort ), // wait for incoming connection on this socket
		m_ReceiveTime(0),
		m_SendTime(0),
		m_Stranger(Stranger),
		m_Server(Server),
		m_ProcessNo(ProcessNo),
		m_Next(0),
		m_Previous(0),
		m_SendTimeout(0),
		m_bIsExecuter(false),
		m_IsEnabled(true),
		m_IsActive(true),
		m_bDeleteStranger(bDeleteStranger)
{
	CommonInit();

	if (m_Stranger)
		m_Stranger->Attach(this);

	attach( service );

	if (m_Stranger)
		m_Stranger->OnStart();

	setDetectOutput( false );
}

void FarmProcess::CommonInit()
{
	m_Ptr = NULL;
	m_TextLength = -1;
	m_RequiredLength = 0;

	// Set up non-blocking reads
	setCompletion( false );
	setLinger( false );

	m_QueueLock = new ost::Mutex("SendQueue");

	m_ReceiveTime	= ost::getTicks();
	m_SendTime		= ost::getTicks();
	m_pSendQueue = NULL;
	m_pCurSendItem = NULL;

	// Set to time out in 10 seconds
	SetTimer();

}

void FarmProcess::WaitForMessageQueueToBeEmpty()
{
	// wait for send queue to become empty
	// wait for 10 seconds and then give up
	for (int i=0; i<10; i++)
	{
		m_QueueLock->enter();

		if (!m_pSendQueue)
		{
			m_QueueLock->leave();
			break;
		}

		m_QueueLock->leave();

		MT::Thread::Sleep(100);
	}
}

bool FarmProcess::IsPendingConnection(int iTimeout)
{
	// TODO: program this
	return true;
//	return isPendingConnection(iTimeout);
}

// set a timer, which will automatically call 'expired' after a certain amount of network idle time
void FarmProcess::SetTimer()
{
	if (m_Stranger)
	{
		int i = m_Stranger->GetIdleTime();
		if (i > 0)
		{
			setTimer(i);
			return;
		}
	}
	setTimer(5000);
}

FarmProcess::~FarmProcess()
{
	Stop();

	WaitForMessageQueueToBeEmpty();

	// delete remaining message send queue contents
	WriteDataConnection *pWD2, *pWD = m_pSendQueue;
	while (pWD)
	{
		if (pWD->m_bDeleteData)
		{
			delete pWD->m_pData;
		}
		pWD2 = pWD;
		pWD = pWD->m_pPrev;
		delete pWD2;
	}

	delete m_QueueLock;
	if (m_Stranger && m_bDeleteStranger)
		delete m_Stranger;
}

// wait until the farm process exits
void FarmProcess::Join(unsigned int timeout)
{
	// TODO: improve the way this is done
	if (timeout != INFINITE)
	{
		while (IsEnabled() && timeout>0)
		{
			MT::Thread::Sleep(1000);
			timeout--;
		}
	}
	else
	{
		while (IsEnabled())
		{
			MT::Thread::Sleep(1000);
		}
	}
}

// forcibly stop the thread
void FarmProcess::Stop()
{
	SetCancel();

	m_ReceiveTime	= 0;
	m_SendTime		= 0;
	m_Server		= 0;
	m_ProcessNo		= -1;
}

bool FarmProcess::IsEnabled() const
{
	return m_IsEnabled;
}

bool FarmProcess::IsActive() const
{
	return m_IsActive;
}

const char* FarmProcess::GetUserName() const
{
	return m_UserName;
}

void FarmProcess::disconnect()
{
	LOGDEBUG( g_Log, "%s: port disconnected\n", GetMyUserName());
	m_IsEnabled = false;
}

bool FarmProcess::Send(const char* Command)
{
	if (!Command)
		return false;

	if (!*Command)
		return true;

	if (!m_IsEnabled)
	{
		FarmPrint("(%s) SEND (disabled): %s\n", GetMyUserName(), Command);
		return false;
	}

	FarmPrint("(%s) SEND: %s\n", GetMyUserName(), Command);

	int Length = (int)strlen(Command);
	int x;
	x = htonl(Length);

	// create a buffer containing the length + the data
	U8 *pData = new U8[Length+sizeof(Length)];
	memcpy(pData, &x, sizeof(Length));
	memcpy(pData+sizeof(Length), Command, Length);
	// send the data. buffer will be deleted by the network code
	SendRAW(pData, Length + sizeof(Length));

	m_SendTime = ost::getTicks();

	return true;
}

void FarmProcess::SetActive(bool bOn)
{
	m_IsActive = bOn;
}

void FarmProcess::AppendLog(int iJobNo, int level, int secs, const char *logger, const char *msg)
{
	m_Server->AppendLog(iJobNo, level, secs, logger, msg);
}

int FarmProcess::AddJob(const char* Data, int iParentProcess)
{
	if (!m_Server)
		return -1;

	return m_Server->AddJob(m_UserName, Data, iParentProcess);
}

int FarmProcess::GetProcessNo()
{
	return m_ProcessNo;
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

int FarmProcess::GetFreeJob(char* Buffer, int MaxLength, int iFlags)
{
	if (!m_Server)
		return -1;

	return m_Server->GetFreeJob(m_ProcessNo, Buffer, MaxLength, iFlags);
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

void FarmProcess::FinishJob(int JobNo, bool bOk)
{
	if (m_Server)
		m_Server->FinishJob(JobNo, bOk);
}

void FarmProcess::FreeJob(int JobNo)
{
	if (m_Server)
		m_Server->FreeJob(JobNo);
}

bool FarmProcess::IsJobCancelled(int JobNo) const
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

bool FarmProcess::IsJobInQueue(int JobNo) const
{
	if (!m_Server)
		return false;

	return m_Server->IsJobInQueue(JobNo);
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

void FarmProcess::expired()
{
	SetTimer();	// reset the timer

	if (m_Stranger && !m_Stranger->OnIdle())
	{
//		LOGDEBUG( g_Log, "%s: disconnected because OnIdle returned NULL\n", GetMyUserName());
//		FarmPrint("OnIdle() returned false.\n");
		SetCancel();
	}

	// speculatively try to read some data. this is a hack to try and stop the farm disconnect problem
	pending();
}

void FarmProcess::output()
{
	m_QueueLock->enter();

	SendQueue();

	m_QueueLock->leave();
}

void FarmProcess::pending()
{
	int LastIdle = 0;

	while (1)
	{
		m_ReceiveTime = ost::getTicks();

		if ((m_TextLength == -1) && (m_Ptr == 0))
		{
			m_Ptr				= m_Buffer;
			m_RequiredLength	= sizeof(int);
		}

		int RecvLength = receive(m_Ptr, m_RequiredLength);

		if (RecvLength <= 0)
			return;

		m_Ptr				+= RecvLength;
		m_RequiredLength	-= RecvLength;

		if (m_RequiredLength > 0)
			continue; // need to read more

		if (m_TextLength == -1)
		{
			m_TextLength = ntohl(*(int*)m_Buffer);

			if ((m_TextLength < 0) || (m_TextLength > (sizeof(m_Buffer) - 1)))
			{
				printf( "ERROR: TextLength is out of range\n");
				LOGERROR( g_Log, "TextLength is out of range\n");
				continue;
			}

			m_Ptr				= m_Buffer;
			m_RequiredLength	= m_TextLength;
		}
		else
		{
			*m_Ptr = '\0';

			FarmPrint("(%s) RECV: %s\n", GetMyUserName(), m_Buffer);

			if (m_Stranger)
			{
				if (!m_Stranger->OnCommand(m_Buffer))
				{
//					FarmPrint("m_Stranger->OnCommand() returned false.\n");
//					LOGDEBUG( g_Log, "%s: disconnected because OnCommand returned false\n", GetMyUserName());
					SetCancel();
					m_IsEnabled = false;
					return;
				}
			}
			else
			{
				const char* Data = m_Buffer;

				String Type = SnapAtom(Data);

				m_UserName = SnapAtom(Data);		// save the username

				if (!stricmp(Type, "WORKER"))
				{
					m_Flags = SnapInt(Data);
					m_Stranger = new FarmAssistant;
				}
				else if (!stricmp(Type, "MANAGER"))
					m_Stranger = new FarmingManager;
				else if (!stricmp(Type, "EXECUTER"))
				{
					m_Stranger = new FarmingManager;
					SetIsExecuter(true);
				}
				else
				{
					FarmPrint("Unrecognized client type.\n");
				}

				if (m_Stranger)
					m_Stranger->Attach(this);

				if (!m_Stranger || !m_Stranger->OnStart())
				{
					LOGERROR( g_Log, "%s: m_Stranger->OnStart() returned false.\n", GetMyUserName());
					return;
				}
			}

			m_Ptr				= 0;
			m_RequiredLength	= 0;
			m_TextLength		= -1;
		}
	}
}

// return process flags (linux, windows)
int FarmProcess::getFlags()
{
	return m_Flags;
}

void FarmProcess::SetCancel()
{
	// the FarmServer class will periodically check IsEnabled() on this class (FarmProcess)
	// if this FarmProcess not enabled it will delete this class, thereby closing the port

	// calling SetCancel on a farm worker or messenger will make the main thread quit out
	if (GetServer())
		LOGDEBUG( g_Log, "%s: SetCancel()\n", GetMyUserName());

	m_IsEnabled = false;

	// send a QUIT messages to whatever is connected to us
	if (m_Stranger)
		m_Stranger->OnStop();
	else
		Send("QUIT");
}

void FarmProcess::SetScreenSaverMode(bool b)
{
	m_bScreenSaverMode = b;
}

FarmingServer *FarmProcess::GetServer()
{
	return m_Server;
}

bool FarmProcess::IsExecuter()
{
	return m_bIsExecuter;
}

void FarmProcess::SetIsExecuter(bool bOn)
{
	m_bIsExecuter = bOn;
}

String FarmProcess::GetMyUserName()
{
	return m_UserName;
}

// send the contents of the send queue 
void FarmProcess::SendQueue()
{
	bool bSentSomething = false;

	WriteDataConnection *pItem = m_pCurSendItem;

	while (pItem)
	{
		int iSendSize = pItem->m_iMessageSize - pItem->m_iWritePosition;
		if (iSendSize > MAX_NETWORK_BUFFER_SIZE)
			iSendSize = MAX_NETWORK_BUFFER_SIZE;
		// has the item finished?
		if (iSendSize<=0)
		{
			// finished - do cleanup
			if (pItem->m_bDeleteData)
				delete pItem->m_pData;

			// nothing to send here, so get pointer to next send in the queue
			WriteDataConnection *pNext = pItem->m_pNext;
			if (pNext) pNext->m_pPrev = NULL;
			delete pItem;
			pItem = pNext; // send next in the queue
			continue;
		}

		// send this item
		void *buf = pItem->m_pData + pItem->m_iWritePosition;
		int i = send(buf, iSendSize);
		if (i>0)
		{
			bSentSomething = true;
			pItem->m_iWritePosition += i;
		}

		// if the call to 'send' didn't manage to send all the data, break and wait until we can send more
		if (i<iSendSize)
			break;
	}

	// update current item + queue head
	m_pCurSendItem = pItem;
	if (!pItem)
	{
		m_pSendQueue = NULL;
		setDetectOutput( false );
		bSentSomething = true;
	}
	else
	{
		setDetectOutput( true );	// detect when we can send the rest of the data
	}

	if (bSentSomething)
	{
		m_SendTimeout = 0; // reset the timer because we managed to send some data, or the send queue is empty
	}
	else
	{
		if (m_SendTimeout == 0)
		{
			// start measuring how long we haven't been able to transmit data for
			m_SendTimeout = Time::GetCurrentTime();
		}
		else
		{
			// measure how long the sending has stalled
			U64 s = Time::GetCurrentTime() - m_SendTimeout;
			if (s > SYS_USEC_PER_SEC*60*5)
			{
				if (IsExecuter() && GetServer())
				{
					// it's stalled for over 5 minutes. disconnect
					printf("SEND QUEUE FULL. DISCONNECTOR.\n");
					LOGERROR( g_Log, "%s: SEND QUEUE FULL. DISCONNECTOR\n", GetMyUserName());
					SetCancel();
				}
				else
				{
					printf("%s: warning: send hasn't been empty for %s\n", GetMyUserName(), Time::FormatDurationString(s).Chars());
				}
			}
		}
	}
}

// add to send queue, or if there is no queue, just send this
void FarmProcess::AddToSendQueue(WriteDataConnection *p)
{
	m_QueueLock->enter();

	if (m_pSendQueue)
	{
		// add this send to the start of the queue
		m_pSendQueue->m_pNext = p;
		p->m_pPrev = m_pSendQueue;
		m_pSendQueue = p;
	}
	else
	{
		// start a new send queue
		m_pSendQueue = p;
		m_pCurSendItem = p;
	}

	// attempt to send the data. the socket might be busy in which case the send will fail, but that doesn't matter
	SendQueue();

	m_QueueLock->leave();
}

// The send function sends data on a connected socket.
// data is deleted automatically after it has been sent
void FarmProcess::SendRAW(const void* Data, int Length)
{
	// create a new buffer to send
	WriteDataConnection *p = new WriteDataConnection;
	p->m_iWritePosition = 0;
	p->m_iMessageSize = Length;
	p->m_pConnection = this;
	p->m_pNext = NULL;
	p->m_pPrev = NULL;
	p->m_bInitialUpdate = false;
	if (Length < LOCAL_BUFFER_SIZE)
	{
		p->m_pData = p->m_localBuffer;
		memcpy(p->m_localBuffer, Data, Length);
		p->m_bUsingLocalBuffer = true;
		p->m_bDeleteData = false;
		delete Data;
	}
	else
	{
		p->m_bUsingLocalBuffer = false;
		p->m_pData = (char*)Data;
		p->m_bDeleteData = true;
	}

	AddToSendQueue(p);
}
