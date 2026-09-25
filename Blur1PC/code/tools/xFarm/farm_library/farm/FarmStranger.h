#pragma once

#include "FarmProcess.h"

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
	int AddJob(const char* Data, int iParentProcess);
	bool KillJob(int JobNo);
	void KillMyJobs();
	int GetFreeJob(char* Buffer, int MaxLength, int iFlags);
	void AcceptJob(int JobNo);
	void CancelJob(int JobNo);
	void FinishJob(int JobNo, bool bOk);
	void FreeJob(int JobNo);
	bool IsJobCancelled(int JobNo);
	bool IsJobInQueue(int JobNo);
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
	bool IsActive();
	void SendRAW(void *pData, U32 c);
	int GetProcessNo();

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
	virtual int				GetIdleTime() const;

protected:
	FarmProcess* m_Process;
};
