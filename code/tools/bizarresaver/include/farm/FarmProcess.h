#pragma once

class FarmStranger;
class FarmServer;

class FarmProcess
{
public:
	FarmProcess(NwSocket& Skt, FarmStranger* Stranger = 0, FarmServer* Server = 0, int ProcessNo = -1);
	~FarmProcess();

public:
	bool Start();
	void Stop();

public:
	bool IsRunning(int Sleep = 0) const;
	bool IsEnabled() const;
	const char* GetUserName() const;

public:
	bool Send(const char* Command);
	void SetEnabled(bool Enabled);

public:
	int AddJob(const char* Command, const char* Comment);
	bool KillJob(int JobNo);
	void KillMyJobs();
	int GetFreeJob(char* Buffer, int MaxLength);
	void AcceptJob(int JobNo);
	void CancelJob(int JobNo);
	void FinishJob(int JobNo);
	void FreeJob(int JobNo);
	bool IsJobCancelled(int JobNo);
	bool ListJobs();
	bool ListMachines();
	bool PriorityJob(int JobNo);
	bool RequeueJob(int JobNo);
	int GetJobVersion();
	int GetMachineVersion();
	bool UpdateCommand(int JobNo, const char* Command);
	bool UpdateComment(int JobNo, const char* Comment);
	bool EnableMachine(int ProcessNo);
	bool DisableMachine(int ProcessNo);

private:
	void Process();

private:
	static DWORD CALLBACK ThreadStartProc(void* Parameter);

private:
	NwSocket		m_Socket;
	HANDLE			m_Thread;
	bool			m_QuitThread;
	DWORD			m_ReceiveTime;
	DWORD			m_SendTime;
	FarmStranger*	m_Stranger;
	FarmServer*		m_Server;
	int				m_ProcessNo;
	String			m_UserName;
	bool			m_IsEnabled;

private:
	FarmProcess*	m_Next;
	FarmProcess*	m_Previous;

friend class FarmServer;
};