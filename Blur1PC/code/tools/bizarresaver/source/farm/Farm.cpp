#include "farmpch.h"

#include "farm/Farm.h"
#include "resource/resource.h"

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

void ExecuteServer()
{
	fprintf(stderr, "Looking for existing server on the network...\r\n");

	NwSockAddr ServerAddress;

	if (!(ServerAddress = FindServer()).GetPort())
	{
		fprintf(stderr, "No existing server found, starting...\r\n");

		FarmServer Svr;

		if (!Svr.Run())
			fprintf(stderr, "Server failed.\r\n");
	}
	else
		fprintf(stderr, "There is already a server running on the network at %s.\r\n", inet_ntoa(ServerAddress.GetAddr()));
}

void ExecuteWorker()
{
	for(;;)
	{
		ExecuteStranger(new FarmWorker);

		fprintf(stderr, "Lost server, trying to reconnect...");

		while(!FindServer().GetPort())
		{
			fprintf(stderr, ".");
			Sleep(15 * 1000);
		}

		fprintf(stderr, "\r\n");
	}
}

void ExecuteUi(bool IsManager)
{
	//where is IDR_ACCEL defined??
	//HACCEL Accel(LoadAccelerators(GetModuleHandle(0), MAKEINTRESOURCE(IDR_ACCEL)));
	HACCEL Accel;

	FreeConsole();

	INITCOMMONCONTROLSEX Ccex;

	ZeroMemory(&Ccex, sizeof(Ccex));

	Ccex.dwSize	= sizeof(Ccex);
	Ccex.dwICC	= ICC_LISTVIEW_CLASSES;

	InitCommonControlsEx(&Ccex);

	FarmWindow Wnd(IsManager);

	Wnd.Create();

	ShowWindow(Wnd, SW_SHOW);

	for(MSG Msg; GetMessage(&Msg, 0, 0, 0); )
	{
		if (TranslateAccelerator(Wnd, Accel, &Msg) == 0)
			TranslateMessage(&Msg);

		DispatchMessage(&Msg);
	}
}

int main(int Argc, const char* Argv[])
{
	if (WSAStartup(WINSOCK_VERSION, &WSADATA()) != 0)
	{
		fprintf(stderr, "Winsock error.\r\n");
		return -1;
	}

	if (Argc < 2)
	{
		fprintf(stderr, "Usage: Farm <command> <arguments>\r\n");
		fprintf(stderr, "\r\n");
		fprintf(stderr, "Command are:\r\n");
		fprintf(stderr, "  JOB <Command> [Comment] - Queue <Command>, with optional [Comment]\r\n");
		fprintf(stderr, "  COMMAND <JobNo> <Cmd>   - Set command of job <JobNo> to <Cmd>\r\n");
		fprintf(stderr, "  COMMENT <JobNo> <Cmt>   - Set comment of job <JobNo> to <Cmt>\r\n");
		fprintf(stderr, "\r\n");
		fprintf(stderr, "  LIST                    - List all jobs\r\n");
		fprintf(stderr, "\r\n");
		fprintf(stderr, "  KILL <JobNo>            - Kill job <JobNo> from queue\r\n");
		fprintf(stderr, "  PRIORITY <JobNo>        - Move job <JobNo> to top of queue\r\n");
		fprintf(stderr, "  REQUEUE <JobNo>         - Requeue job <JobNo>\r\n");
		fprintf(stderr, "\r\n");
		fprintf(stderr, "  SERVER                  - Start as server\r\n");
		fprintf(stderr, "  WORKER                  - Start as worker\r\n");
		fprintf(stderr, "\r\n");
		fprintf(stderr, "  USER                    - Start user application\r\n");
		fprintf(stderr, "  MANAGER                 - Start management application\r\n");

		return 0;
	}

	if (stricmp(Argv[1], "SERVER") == 0)
	{
		if (Argc == 2)
		{
			ExecuteServer();
			return 0;
		}
		else
			fprintf(stderr, "Syntax error: Usage: Farm SERVER\r\n");
	}
	else if (stricmp(Argv[1], "WORKER") == 0)
	{
		if (Argc == 2)
		{
			ExecuteWorker();
			return 0;
		}
		else
			fprintf(stderr, "Syntax error: Usage: Farm WORKER\r\n");
	}
	else if (stricmp(Argv[1], "MANAGER") == 0)
	{
		if (Argc == 2)
		{
			ExecuteUi(true);
			return 0;
		}
		else
			fprintf(stderr, "Syntax error: Usage: Farm MANAGER\r\n");
	}
	else if (stricmp(Argv[1], "USER") == 0)
	{
		if (Argc == 2)
		{
			ExecuteUi(false);
			return 0;
		}
		else
			fprintf(stderr, "Syntax error: Usage: Farm USER\r\n");
	}
	else if (stricmp(Argv[1], "JOB") == 0)
	{
		if (Argc == 3)
			return ExecuteStranger(new FarmJob(Argv[2], Argv[2]));
		else if (Argc == 4)
			return ExecuteStranger(new FarmJob(Argv[2], Argv[3]));
		else
			fprintf(stderr, "Syntax error: Usage: Farm JOB <Command> [Comment]\r\n");
	}
	else if (stricmp(Argv[1], "KILL") == 0)
	{
		if (Argc == 3)
			return ExecuteStranger(new FarmKill(atoi(Argv[2])));
		else
			fprintf(stderr, "Syntax error: Usage: Farm KILL <JobNo>\r\n");
	}
	else if (stricmp(Argv[1], "LIST") == 0)
	{
		if (Argc == 2)
			return ExecuteStranger(new FarmList());
		else
			fprintf(stderr, "Syntax error: Usage: Farm LIST\r\n");
	}
	else if (stricmp(Argv[1], "PRIORITY") == 0)
	{
		if (Argc == 3)
			return ExecuteStranger(new FarmPriority(atoi(Argv[2])));
		else
			fprintf(stderr, "Syntax error: Usage: Farm PRIORITY <JobNo>\r\n");
	}
	else if (stricmp(Argv[1], "REQUEUE") == 0)
	{
		if (Argc == 3)
			return ExecuteStranger(new FarmRequeue(atoi(Argv[2])));
		else
			fprintf(stderr, "Syntax error: Usage: Farm REQUEUE <JobNo>\r\n");
	}
	else if (stricmp(Argv[1], "COMMAND") == 0)
	{
		if (Argc == 4)
			return ExecuteStranger(new FarmCommand(atoi(Argv[2]), Argv[3]));
		else
			fprintf(stderr, "Syntax error: Usage: Farm COMMAND <JobNo> <Command>\r\n");
	}
	else if (stricmp(Argv[1], "COMMENT") == 0)
	{
		if (Argc == 4)
			return ExecuteStranger(new FarmComment(atoi(Argv[2]), Argv[3]));
		else
			fprintf(stderr, "Syntax error: Usage: Farm COMMENT <JobNo> <Comment>\r\n");
	}
	else
		fprintf(stderr, "Unknown command: %s\r\n", Argv[1]);

	WSACleanup();

	return -1;
}