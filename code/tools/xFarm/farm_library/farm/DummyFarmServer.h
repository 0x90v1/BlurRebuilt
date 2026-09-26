#pragma once

#include "SQLaccessInterface.h"

class DummyFarmingServer : public FarmingServer
{
public:
	DummyFarmingServer(U16 broadcastPort, U16 networkPort, SQLAccessInterface *pSQLAccess);

	void ThreadMain();

	void Run();
};

