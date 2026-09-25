#pragma once

class FarmServer
{
public:
	FarmServer();
	~FarmServer();

public:
	bool Run();

public:
	int AddJob(const char* User, const char* Command, const char* Comment);
	bool KillJob(int JobNo);
	void KillMyJobs(int ProcessNo);
	int GetFreeJob(int ProcessNo, char* Buffer, int MaxLength);
	void AcceptJob(int JobNo);
	void CancelJob(int JobNo);
	void FinishJob(int JobNo);
	void FreeJob(int JobNo);
	bool IsJobCancelled(int JobNo);
	bool ListJobs(FarmProcess* Prc);
	bool ListMachines(FarmProcess* Prc);
	bool PriorityJob(int JobNo);
	bool RequeueJob(int JobNo);
	int GetJobVersion();
	int GetMachineVersion();
	bool UpdateCommand(int JobNo, const char* Command);
	bool UpdateComment(int JobNo, const char* Comment);
	bool EnableMachine(int ProcessNo);
	bool DisableMachine(int ProcessNo);

private:
	bool AdvertiseServer();

private:
	static DWORD CALLBACK AdvertiseServerProc(LPVOID Parameter);

private:
	enum JobState
	{
		JobStateFree,
		JobStatePending,
		JobStateAllocated,
		JobStateKilled,
		JobStateRequeue
	};

private:
	struct Job
	{
		Job*		Next;
		Job*		Previous;
		JobState	State;
		int			JobNo;
		int			ProcessNo;
		DWORD		TimeStarted;
		String		User;
		String		Command;
		String		Comment;

	public:
		Job() :
				Next(0),
				Previous(0),
				State(JobStateFree),
				JobNo(-1),
				ProcessNo(-1)
		{
		}
	};

private:
	Job* AttachJob(Job* Jb);
	Job* DetachJob(Job* Jb);
	Job* FindJob(int JobNo);

	FarmProcess* AttachProcess(FarmProcess* Prc);
	FarmProcess* DetachProcess(FarmProcess* Prc);
	FarmProcess* FindProcess(int ProcessNo);

private:
	int					m_JobNo;
	Job*				m_Jobs;
	CRITICAL_SECTION	m_Critical;
	FarmProcess*		m_Processes;
	int					m_JobVersion;
	int					m_MachineVersion;
};

NwSockAddr FindServer();