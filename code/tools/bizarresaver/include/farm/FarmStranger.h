#pragma once

enum StrangerType
{
	ST_Generic,
	ST_Assistant
};

class FarmStranger
{
public:
	FarmStranger();
	virtual ~FarmStranger();

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
	bool IsEnabled();

public:
	bool AnnounceUser(const char* Type);
	bool AnnounceMachine(const char* Type);

public:
	void Attach(FarmProcess* Process);

public:
	bool Send(const char* Command, ...);

public:
	virtual bool OnStart();
	virtual bool OnStop();

public:
	virtual bool OnCommand(const char* Command);
	virtual bool OnIdle();

public:
	virtual StrangerType	GetType() const;
	virtual DWORD			GetIdleTime() const;

private:
	FarmProcess* m_Process;
};