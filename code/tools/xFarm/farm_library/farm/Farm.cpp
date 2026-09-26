#include "StdAfx.h"
#include "Farm.h"
#include "Resource.h"

namespace xFarm
{
Debug::Logger &g_Log = Debug::GetLogger("tools.xfarm");
}

bool ExecuteStranger(U16 broadcastPort, U16 networkPort, FarmStranger* Stranger, unsigned int timeout, bool bDeleteStranger)
{
	ost::IPV4Host ServerAddress;

	if (!FindServer(broadcastPort, networkPort, &ServerAddress))
	{
		FarmPrint("No server found.\r\n");
		if (bDeleteStranger)
			delete Stranger;
		return false;
	}

	FarmPrint("Found server at %s.\r\n", ServerAddress.getHostname());

	ost::SocketService service;

	// start the farm process
	// Stranger will be deleted by FarmProcess when it exits
	FarmProcess Prc(ServerAddress, networkPort, Stranger, 0, 0, &service, bDeleteStranger);
	if (Prc.IsPendingConnection(3000))
	{
		// yield until the thread finishes
		Prc.Join(timeout);

		service.cancel();
		service.join();
	}
	else
	{
		FarmPrint("Couldn't connect to server.\r\n");
		if (bDeleteStranger)
			delete Stranger;
		return false;
	}

	return true;
}

bool ExecuteServer(U16 broadcastPort, U16 networkPort, bool bScreensaverMode, bool bDebugMode, bool bBroadcast, SQLAccessInterface *pSQLAccess)
{
	ost::IPV4Host ServerAddress;

	if (!FindServer(broadcastPort, networkPort, &ServerAddress))
	{
		FarmPrint("Starting server...\r\n");

		FarmingServer Svr(broadcastPort, networkPort, pSQLAccess);

		Svr.SetScreensaverMode( bScreensaverMode );
		Svr.SetDebugMode( bDebugMode );
		Svr.SetBroadcast( bBroadcast );

		if (!Svr.Run())
		{
			FarmPrint("Server failed.\r\n");
			return false;
		}
	}
	else
	{
		FarmPrint("There is already a server running on the network at %s.\r\n", ServerAddress.getHostname());

		return false;
	}

	return true;
}

bool ExecuteWorker(U16 broadcastPort, U16 networkPort)
{
	FarmStranger *pWorker = new FarmingWorker;

	for(;;)
	{
		ExecuteStranger(broadcastPort, networkPort, pWorker, INFINITE, false);

#ifdef _DEBUG
		break;
#endif

		FarmPrint("Lost server, trying to reconnect...");

		while (!FindServer(broadcastPort, networkPort, NULL))
		{
			FarmPrint(".");
			MT::Thread::Sleep(15 * 1000);
		}

		FarmPrint("\r\n");
	}

	delete pWorker;

	return true;
}


