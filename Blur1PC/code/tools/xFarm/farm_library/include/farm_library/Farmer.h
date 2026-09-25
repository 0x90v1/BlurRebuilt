#pragma once

#include "../Farm/FarmJobFlags.h"

#define SCREENSAVER_PORT	8769

class FarmStranger;
class SQLAccessInterface;

class Farmer
{
protected:
	U16		m_broadcastPort;
	U16		m_networkPort;

	FarmStranger *m_pStranger;

public:
	Farmer(U16 broadcastPort, U16 networkPort);
	~Farmer();

	void SetBroadcastPort(U16 portID);
	U16 GetBroadcastPort(void) const;
	void SetNetworkPort(U16 portID);
	U16 GetNetworkPort(void) const;

	bool FindServer(void);
};

// This is an instance of a server that receives jobs and farms them out
class FarmServer : public Farmer
{
protected:
	bool m_bScreensaverMode;
	bool m_bDebugMode;
	bool m_bBroadcast;
	SQLAccessInterface *m_pSQLAccess;

public:
	FarmServer(U16 broadcastPort, U16 networkPort, SQLAccessInterface *pSQLAccess);
	~FarmServer();

	bool Start(void);
	void SetScreensaverMode(bool bMode);
	void SetDebugMode(bool bMode);
	void SetBroadcast(bool bOn);
};

// This is what does the grunt work
class FarmList : public Farmer
{
protected:
public:
	FarmList(U16 broadcastPort, U16 networkPort);

	bool Start(void);
};

// This is what does the grunt work
class FarmWorker : public Farmer
{
protected:
public:
	FarmWorker(U16 broadcastPort, U16 networkPort);

	bool Start(void);
};

// The client is basically for kicking off jobs, or querying work in progress
class FarmClient : public Farmer
{
protected:
public:
	FarmClient(U16 broadcastPort, U16 networkPort);
	~FarmClient();

	int StartJob(const char* command, const char* comment=NULL, int iFlags = JOB_FLAG_LINUX, bool waitForFinish=false);
	bool KillJob(int iID);
	bool PrioritiseJob(int iID);
};

int GetJobStatus(int iID, int iPort);

extern bool g_bSilentMode;
extern bool g_bScreenSaverMode;
