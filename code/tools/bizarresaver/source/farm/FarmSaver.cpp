#include "farmpch.h"
#include "farm/Farm.h"

bool gbJobRunning = false;
bool gbKillWorker = false;
HANDLE ghWorker;
HANDLE ghKillWorkerEvent;

int ExecuteStranger(FarmStranger* Stranger)
{
	NwSockAddr ServerAddress;

	if (!(ServerAddress = FindServer()).GetPort())
	{
		fprintf(stderr, "No server found.\r\n");
		delete Stranger;
		return -1;
	}

	fprintf(stderr, "Found server at %s.\r\n", inet_ntoa(ServerAddress.GetAddr()));

	NwSocket Cli;

	if (Cli.Connect(ServerAddress))
	{
		FarmProcess Prc(Cli, Stranger);

		if (Prc.Start())
		{
			Prc.IsRunning(INFINITE);
			Prc.Stop();
		}
	}
	else
	{
		fprintf(stderr, "Couldn't connect to server.\r\n");
		delete Stranger;
		return -1;
	}

	return 0;
}

DWORD CALLBACK ExecuteWorker(void*)
{
	for(;;)
	{
		ExecuteStranger(new FarmWorker);

		if (gbKillWorker)
			return 0;

		fprintf(stderr, "Lost server, trying to reconnect...");

		while(!FindServer().GetPort())
		{
			fprintf(stderr, ".");
			WaitForSingleObject(ghKillWorkerEvent, 15000);
			if (gbKillWorker)
				return 0;
		}

		fprintf(stderr, "\r\n");
	}
}

void ExecuteWorkerThread()
{
	ghKillWorkerEvent = CreateEvent( NULL, TRUE, FALSE, NULL);

	WSAStartup(WINSOCK_VERSION, &WSADATA());

	ghWorker = CreateThread(0, 0, ExecuteWorker, NULL, 0, 0);	
}

bool IsProcessRunning()
{
	return gbJobRunning;
}

void KillWorker()
{
	gbKillWorker = true;
	SetEvent( ghKillWorkerEvent );

	WaitForSingleObject(ghWorker, INFINITE);
}
