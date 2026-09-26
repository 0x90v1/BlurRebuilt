#include "StdAfx.h"
#include "Farm.h"

#include "SQLaccessInterface.h"
#include <vector>

DummyFarmingServer::DummyFarmingServer(U16 broadcastPort, U16 networkPort, SQLAccessInterface *pSQLAccess) :
	FarmingServer(broadcastPort, networkPort, pSQLAccess)
{
}

void DummyFarmingServer::ThreadMain()
{
	AcceptConnectionThread();
}

void DummyFarmingServer::Run()
{
}
