#include "stdafx.h"

#include <farm_library/Farmer.h>

#include "Farm\FarmStranger.h"
#include "Farm\FarmJob.h"
#include "Farm\Farm.h"
#include "Farm\FarmMessenger.h"




Farmer::Farmer(U16 broadcastPort, U16 networkPort) : m_broadcastPort(broadcastPort), m_networkPort(networkPort), m_pStranger(NULL)
{
#ifdef WIN32
	if (WSAStartup(WINSOCK_VERSION, &WSADATA()) != 0)
	{
		FarmPrint("Winsock error.\r\n");
		assert(false);
	}
#endif
}

Farmer::~Farmer()
{
}

bool Farmer::FindServer(void)
{
	ost::IPV4Host ServerAddress;

	if (!::FindServer(m_broadcastPort, m_networkPort, &ServerAddress))
	{
		FarmPrint("No server found.\r\n");
		return false;
	}

	FarmPrint("Found server at %s.\r\n", ServerAddress.getHostname());

	return true;
}

void Farmer::SetBroadcastPort(U16 portID)
{
	assert(m_broadcastPort == 0);	// Make sure we only set the once
	m_broadcastPort = portID;
}

U16 Farmer::GetBroadcastPort(void) const
{
	return m_broadcastPort;
}

void Farmer::SetNetworkPort(U16 portID)
{
	assert(m_networkPort == 0);		// Make sure we only set the once
	m_networkPort = portID;
}

U16 Farmer::GetNetworkPort(void) const
{
	return m_networkPort;
}

/*
*/
FarmServer::FarmServer(U16 broadcastPort, U16 networkPort, SQLAccessInterface *pSQLAccess) :
	Farmer(broadcastPort, networkPort),
	m_bScreensaverMode(false),
	m_bDebugMode(false),
	m_bBroadcast(true),
	m_pSQLAccess(pSQLAccess)
{

}

FarmServer::~FarmServer()
{
}

bool FarmServer::Start(void)
{
	ExecuteServer(m_broadcastPort, m_networkPort, m_bScreensaverMode, m_bDebugMode, m_bBroadcast, m_pSQLAccess);

	return true;
}

void FarmServer::SetScreensaverMode(bool bMode)
{
	m_bScreensaverMode = bMode;
}

void FarmServer::SetBroadcast(bool bOn)
{
	m_bBroadcast = bOn;
}

void FarmServer::SetDebugMode(bool bMode)
{
	m_bDebugMode = bMode;
}

/*
*/
FarmWorker::FarmWorker(U16 broadcastPort, U16 networkPort) : Farmer(broadcastPort, networkPort)
{
}

bool FarmWorker::Start(void)
{
	ExecuteWorker(m_broadcastPort, m_networkPort);

	return true;
}

/*
*/
FarmList::FarmList(U16 broadcastPort, U16 networkPort) : Farmer(broadcastPort, networkPort)
{
}

bool FarmList::Start(void)
{
	return ExecuteStranger(m_broadcastPort, m_networkPort, new FarmingList(), 4, true);
}

/*
*/
FarmClient::FarmClient(U16 broadcastPort, U16 networkPort) : Farmer(broadcastPort, networkPort)
{
}

FarmClient::~FarmClient()
{
}

int FarmClient::StartJob(const char* command, const char* comment, int iFlags, bool waitForFinish)
{
	FarmJob* job = new FarmJob(command, comment, iFlags);

	bool bSuccess = ExecuteStranger(m_broadcastPort, m_networkPort, job, 4, false);

	int rtn = job->GetJobNumber();

	delete job;

	return rtn;
}

bool FarmClient::KillJob(int iID)
{
	FarmKill *p = new FarmKill(iID);
	return ExecuteStranger(m_broadcastPort, m_networkPort, p, 4, true);
}

bool FarmClient::PrioritiseJob(int iID)
{
	FarmPriority *p = new FarmPriority(iID);
	return ExecuteStranger(m_broadcastPort, m_networkPort, p, 4, true);
}



int GetJobStatus(int iID, int iPort)
{
	FarmMessenger *p = new FarmMessenger;

	p->SetJobNumber(iID);

	if (ExecuteStranger(iPort, iPort, p, 4, false))
	{
		int iResult = p->GetJobStatus();
		delete p;
		return iResult;
	}

	delete p;
	return -1;
}

