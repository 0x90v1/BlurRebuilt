#pragma once

#include "Network.h"

class FarmStranger;
class FarmingServer;
class FarmProcess;

#define LOCAL_BUFFER_SIZE 4096
#define MAX_NETWORK_BUFFER_SIZE (1024*1024)

class WriteDataConnection
{
public:

	WriteDataConnection *StartSend();

	char		*m_pData;	// actual data
	int			m_iMessageSize;
	char		m_localBuffer[LOCAL_BUFFER_SIZE];
	bool		m_bUsingLocalBuffer;
	int			m_iWritePosition;
	bool		m_bDeleteData;
	bool		m_bInitialUpdate;
	int			m_nBytesSent;
	WriteDataConnection	*m_pNext, *m_pPrev;
	FarmProcess *m_pConnection;
};

class FarmProcess : public virtual ost::SocketPort
{
public:
	FarmProcess(ost::TCPSocket &Skt, FarmStranger* Stranger, FarmingServer* Server, int ProcessNo, ost::SocketService *service);
	FarmProcess(ost::IPV4Host &Skt, int iPort, FarmStranger* Stranger, FarmingServer* Server, int ProcessNo, ost::SocketService *service, bool bDeleteStranger);
	~FarmProcess();

public:
	bool IsEnabled() const;
	bool IsActive() const;
	const char* GetUserName() const;
	void Stop();

public:
	bool Send(const char* Command);
	void SendRAW(const void* Data, int Length);
	void SetActive(bool Enabled);

public:
	int AddJob(const char* Data, int iParentProcess);
	bool KillJob(int JobNo);
	void KillMyJobs();
	int GetFreeJob(char* Buffer, int MaxLength, int iFlags);
	void AcceptJob(int JobNo);
	void CancelJob(int JobNo);
	void FinishJob(int JobNo, bool bOk);
	void FreeJob(int JobNo);
	bool IsJobCancelled(int JobNo) const;
	bool IsJobInQueue(int JobNo) const;
	bool ListJobs();
	void AppendLog(int iJobNo, int level, int secs, const char *logger, const char *msg);
	bool ListMachines();
	bool PriorityJob(int JobNo);
	bool RequeueJob(int JobNo);
	int GetJobVersion();
	int GetMachineVersion();
	bool UpdateCommand(int JobNo, const char* Command);
	bool UpdateComment(int JobNo, const char* Comment);
	bool EnableMachine(int ProcessNo);
	bool DisableMachine(int ProcessNo);
	void Join(unsigned int timeout);
	bool IsPendingConnection(int iTimeout);
	void SetCancel();
	void SetScreenSaverMode(bool bOn);
	int getFlags();
	bool IsExecuter();
	void SetIsExecuter(bool bOn);
	FarmingServer *GetServer();
	String GetMyUserName();
	int GetProcessNo();

private:
	void SetTimer();
	void CommonInit();

private:
	// ost::SocketPort function overloads
	void	disconnect();
	void	expired();
	void	pending();
	void	output();

private:
	unsigned long	m_ReceiveTime;
	unsigned long	m_SendTime;
	FarmStranger*	m_Stranger;
	int				m_Flags;
	FarmingServer*	m_Server;
	int				m_ProcessNo;
	String			m_UserName;
	bool			m_IsEnabled;	// means the process is running
	bool			m_IsActive;		// means the process is accepting jobs
	U64				m_Timer;
	bool			m_bScreenSaverMode;
	bool			m_bIsExecuter;
	bool			m_bDeleteStranger;
	U64				m_SendTimeout;

	char	m_Buffer[4096];
	char*	m_Ptr;
	int		m_TextLength;
	int		m_RequiredLength;

	// network send queue
	WriteDataConnection		*m_pSendQueue;
	WriteDataConnection		*m_pCurSendItem;
	ost::Mutex	*m_QueueLock;

	void AddToSendQueue(WriteDataConnection *p);
	void SendQueue();
	void WaitForMessageQueueToBeEmpty();

private:
	FarmProcess*	m_Next;
	FarmProcess*	m_Previous;

friend class FarmingServer;
};
