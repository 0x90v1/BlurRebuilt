#pragma once

#include "SQLaccessInterface.h"

class FarmingServer : public MT::Thread
{
	struct Job;

protected:
	U16	m_networkPort;
	U16	m_broadcastPort;

public:
	FarmingServer(U16 broadcastPort, U16 networkPort, SQLAccessInterface *pSQLAccess);
	~FarmingServer();

public:
	bool Run();

public:
	int AddJob(const char* User, const char* Data, int iParentProcess);
	bool KillJob(int JobNo);
	void KillMyJobs(int ProcessNo);
	int GetFreeJob(int ProcessNo, char* Buffer, int MaxLength, int iFlags);
	void AcceptJob(int JobNo);
	void CancelJob(int JobNo);
	void FinishJob(int JobNo, bool bOk);
	void FreeJob(int JobNo);
	bool IsJobCancelled(int JobNo);
	bool IsJobInQueue(int JobNo);
	bool ListJobs(FarmProcess* Prc);
	bool ListMachines(FarmProcess* Prc);
	bool PriorityJob(int JobNo);
	void UpdateJobToSQL(Job *pJob);
	void UpdateJobStatusToSQL(int iID, int iStatus);
	bool RequeueJob(int JobNo);
	int GetJobVersion();
	int GetMachineVersion();
	bool UpdateCommand(int JobNo, const char* Command);
	bool UpdateComment(int JobNo, const char* Comment);
	bool EnableMachine(int ProcessNo);
	bool DisableMachine(int ProcessNo);
	void SetScreensaverMode(bool bMode);
	bool AcceptConnectionThread();
	void SetBroadcast(bool bOn);
	void StartLog(Job *Jb);
	void AppendLog(int JobNo, int level, int secs, const char *logger, const char *msg);
	void SetDebugMode(bool bMode);
	int UpdateProcessTime(FarmProcess *pProcess);
	void UpdateWorkerTimers();
	void UpdateProgressBarToSQL(int iID, int iProgress, const char *pDescription);
	bool GetScreenSaverMode();
	bool KillJobByProcess(int iProcess);
	ost::SocketService *GetService();
	bool CheckJobCanBeRun(Job *pJob, int iWorkerFlags);
	void SendMessageToExecuter(const Util::C_String &msg, int iJobNo);
	void SetShutdown();

private:
	void ThreadMain();

private:
	enum JobState
	{
		JobStateFree,			// job is waiting to be done
		JobStatePending,		// job has been assigned to a client, but the client hasn't confirmed yet
		JobStateAllocated,		// job is being processed by a client
		JobStateKilled,
		JobStateRequeue,
		JobStateCompleted,		// job has been completed successfully
		JobStateFailed,			// job completed, but exit code was nonzero
		JobStateFatalError,
	};

	struct Job
	{
		Job*		Next;
		Job*		Previous;
		JobState	State;
		int			JobNo;
		int			ProcessNo;
		unsigned long	TimeStarted;
		String		User;
		String		Command;
		String		Comment;
		String		Project;
		int			Flags;
		std::string LogPath;
		int			iEncounteredFatal;
		int			iLogLines;
		int			iParentProcess;

		FeedbackInfo Feedback;

		Ut::Vector<int>	Dependencies;	// indices of jobs that this job is dependant on

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

	/// send e-mails to the user if requested, and to crash mailing list if it's crashed
	void SendEmails(Job *pJob, int iType);

private:
	int					m_JobNo;
	Job*				m_Jobs;
	ost::Mutex			*m_Critical;
	FarmProcess*		m_Processes;
	int					m_JobVersion;
	int					m_MachineVersion;
	bool				m_bScreensaverMode;
	bool				m_bDebugMode;
	bool				m_bBroadcast;
	U64					m_Timer;
	bool				m_bExitFlag;
	static const U64	m_lTimerUpdateInterval;
	std::vector<ost::SocketService*> m_Services;

	SQLAccessInterface	*m_pDB;
};

bool FindServer(U16 broadcastPort, U16 networkPort, ost::IPV4Host *serverAddr );
