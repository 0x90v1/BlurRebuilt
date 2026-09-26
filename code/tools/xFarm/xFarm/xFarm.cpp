// TestFarmClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#ifdef WIN32
#include <crtdbg.h>
#include "Mdump.h"
#endif

//tclap
#include <cmdline.h>
#include <system/debug/log/debuglog.h>
#include "farm_library\Farmer.h"
#include "farm/SQLAccessInterface.h"
#include "farm/farm.h"
#include "DummySQLAccess.h"
#include "system\debuglog.h"
#include "tools_shared\ToolsShared.h"
#include "tools_shared\VersionStrings.inl"
#include "system/debug/log/subscribers/E-MailSubscriber.h"
#include "xFarm.h"

#ifndef __GNUC__
#include "SQLAccess.h"
#endif
#include "FarmUserInterface.h"

using namespace TCLAP;
using namespace std;
Tu::DebugManager g_debugManager;

extern bool g_bScreenSaverMode;
extern bool g_bOutsourcingMode;

void Fork();

#ifdef WIN32
int __cdecl MyAllocHook(
						int 	nAllocType,
						void   * pvData,
						size_t	nSize,
						int 	nBlockUse,
						long	lRequest,
						const unsigned char * szFileName,
						int 	nLine
						)
{

	if (lRequest > 1000 && nSize == 4256)
	{
		int j = 1;
		//printf("blah\n");
	}

	return( TRUE ); 	   // Allow the memory operation to proceed
}
#endif


void StartDummyServer(int NetworkPort)
{
	if (g_bOutsourcingMode)
		g_debugManager << "Outsourcing Dummy Server\n\n";
	else
		g_debugManager << "Dummy Server\n\n";

	if (!g_bOutsourcingMode)
		Debug::AutoConfigureDebugLog();

	// start a dummy server which does not broadcast
	FarmPrint("Starting server...\r\n");

	DummyFarmingServer Svr((U16)-1, NetworkPort, new DummySQLAccess);

	Svr.Start();

	// start dummy worker which connects to the local server directly
	ost::IPV4Host ServerAddress("127.0.0.1");

	ost::SocketService service;

	MT::Thread::Sleep(500);

	// start the farm process
	FarmPrint("Starting worker...\r\n");
	FarmStranger *pWorker = new FarmingWorker;

	FarmProcess Prc(ServerAddress, NetworkPort, pWorker, 0, 0, &service, true);
	if (Prc.IsPendingConnection(3000))
	{
		// yield until the thread finishes
		FarmPrint("Connected.\r\n");

		Svr.Join();

		exit(0);
	}
	else
	{
		FarmPrint("Couldn't connect to server.\r\n");
	}
}

int main(int argc, char* argv[])
{
	XFarmApp app;
	return app.Run(argc, argv, false);
}

XFarmApp::XFarmApp() :
Tools::C_ToolsApplication(
		"xFarm",
		"Network Farm Utility",
		"tools.xfarm",
		"\\\\bizsymbol\\CrashDumps\\")
{
}

void AttachEMailSubscriber()
{
	// attach an e-mail subcriber to mail us the last 50 log messages when a LOGFATAL happens
	Debug::EMailSubscriber *pEMailSubscriber = new Debug::EMailSubscriber;
	Debug::GetRootLogger().AddSubscriber(pEMailSubscriber);
	pEMailSubscriber->SetXMLParameter("smtphost", "bizex01");
	pEMailSubscriber->SetXMLParameter("sender", "FarmServer");
	pEMailSubscriber->SetXMLParameter("recipient", "farm_crash@bizarrecreations.com");
	pEMailSubscriber->SetXMLParameter("subject", "Farm Server Fatal Error Report");
	pEMailSubscriber->SetXMLParameter("user", "farm");
	pEMailSubscriber->SetXMLParameter("password", "Password_1");
	pEMailSubscriber->SetXMLParameter("buffersize", "50");
	pEMailSubscriber->SetXMLParameter("triggerlevel", "Fatal");
	pEMailSubscriber->SetXMLParameter("maxemails", "1");
	pEMailSubscriber->SetXMLParameter("maxemailstimeout", "30");
	pEMailSubscriber->Start();
}

int XFarmApp::Main(int argc, char **argv, Progress::I_ProgressBar *pProgress)
{
#ifdef _DEBUG
#ifdef WIN32
	_CrtSetDbgFlag( _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF );
	_CrtSetAllocHook(MyAllocHook);
#endif
#endif

	int	NetworkPort		= 8769;
	int	BroadcastPort	= 8769;

	CmdLine cmd("Network Farm Utility", ' ', "1.0");

#ifdef WIN32
	UnlabeledValueArg<string> modeArg("mode", "Farm Mode", "", "server/worker/manager/list/user/job/kill/prioritise/dummy"  ); cmd.add( modeArg );
#else
	UnlabeledValueArg<string> modeArg("mode", "Farm Mode", "", "server/worker/list/job/kill/dummy"  ); cmd.add( modeArg );
#endif
	std::vector<Arg*> xorList;
	// robot farm switch
	SwitchArg robotSwitch("r","robot","Use robot farm", false);
	xorList.push_back(&robotSwitch);
	// screensaver farm switch
	SwitchArg saverSwitch("s","saver","Use screensaver farm", false);
	xorList.push_back(&saverSwitch);
	// 64-bit farm switch
	SwitchArg sixtyfourSwitch("a","AMD64","Use 64-bit farm", false);
	xorList.push_back(&sixtyfourSwitch);
	// test switch
	SwitchArg testSwitch("t","test","Use farm settings for testing", false);
	xorList.push_back(&testSwitch);
	// saver test switch
	SwitchArg test2Switch("","t2","Use farm settings for screensaver testing", false);
	xorList.push_back(&test2Switch);
	// specify a specific port
	ValueArg<int> specifyPort("p","port","Specify a custom port", false, 0, "port");
	xorList.push_back(&specifyPort);
	// add xor switches to commandline
	cmd.xorAdd( xorList );
#ifdef __GNUC__
	// switch for running the farm process as a daemon
	SwitchArg daemonSwitch("","startdaemon","Run as a daemon process", false);
	cmd.add( daemonSwitch );
#endif

	// if it's a job, remove the actual job description
	std::string sJob;
	bool bJob = false;
	int nargs = argc;
	for (int i=0; i<nargs; i++)
	{
		if (!bJob)
		{
			if (stricmp(argv[i],"job") == 0)
			{
				bJob = true;
				argc = i+1;
			}
		}
		else
		{
			sJob += std::string(argv[i]) + " ";
		}
	}

	// if it's a kill command, remove the job id
	int iKillId = -1;
	bool bKill = false;
	nargs = argc;
	for (int i=0; i<nargs; i++)
	{
		if (!bKill)
		{
			if (stricmp(argv[i],"kill") == 0)
			{
				bKill = true;
				argc = i+1;
			}
		}
		else
		{
			iKillId = atoi(argv[i]);
		}
	}

	// if it's a prioritise command, remove the job id
	int iPrioritiseId = -1;
	bool bPrioritise = false;
	nargs = argc;
	for (int i=0; i<nargs; i++)
	{
		if (!bPrioritise)
		{
			if (stricmp(argv[i],"prioritise") == 0)
			{
				bPrioritise = true;
				argc = i+1;
			}
		}
		else
		{
			iPrioritiseId = atoi(argv[i]);
		}
	}


	cmd.parse( argc, argv );

#ifdef __GNUC__
	bool bDaemon = daemonSwitch.getValue();
#else
	const bool bDaemon = false;
#endif

#if defined(_DEBUG) && defined(WIN32)
	minidump_Init();
#endif

	g_debugManager.SetConsoleEnabledFlag(true);
	g_debugManager.Create("c:/xfarm.log");

	g_debugManager << "Running ";

	if (saverSwitch.isSet())
	{
		// set ports for screensaver PCs
		NetworkPort		= SCREENSAVER_PORT; //8769 or 16674;
		BroadcastPort	= SCREENSAVER_PORT;

		g_debugManager << "Screen Saver PCs";
	}
	else if (sixtyfourSwitch.isSet())
	{
		// set ports for 64-bit farm PCs
		NetworkPort		= 9765;
		BroadcastPort	= 9766;

		g_debugManager << "64-bit farm";

//		InitFarmPrintLog();	// enables verbose network logging
	}
	else if (testSwitch.isSet())
	{
		// set some unused ports for debugging
		NetworkPort		= 8770;
		BroadcastPort	= 8770;

		g_debugManager << "debug farm";
	}
	else if (test2Switch.isSet())
	{
		// set some unused ports for debugging screensaver mode
		NetworkPort		= 16674;
		BroadcastPort	= 16674;

		g_debugManager << "debug screensaver farm";
	}
	else if (robotSwitch.isSet())
	{
		// set ports for robot farm PCs
		NetworkPort		= 8765;
		BroadcastPort	= 8766;

		g_debugManager << "Robot farm";
	}
	else if (specifyPort.isSet())
	{
		// set user-defined port
		NetworkPort = BroadcastPort = specifyPort.getValue();

		g_debugManager << "custom farm on port " << NetworkPort;
	}
	else
	{
		assert(0);
	}

	g_debugManager << " - ";

	int iExitCode = 0;

	const char *pMode = modeArg.getValue().c_str();

	g_bScreenSaverMode = saverSwitch.isSet() || test2Switch.isSet();

	if (!stricmp(pMode, "Server"))
	{
		g_debugManager << "Server\n";

		AttachEMailSubscriber();

		if (bDaemon) Fork();

		Debug::AutoConfigureDebugLog();

#ifdef __GNUC__
		FarmServer farmServer(BroadcastPort, NetworkPort, new DummySQLAccess);
#else
		FarmServer farmServer(BroadcastPort, NetworkPort, new SQLAccess);
#endif
		
		farmServer.SetScreensaverMode( saverSwitch.isSet() || test2Switch.isSet() );
		if (testSwitch.isSet())
			farmServer.SetDebugMode( true );

		if (!farmServer.Start())
		{
			g_debugManager << "Error occured starting server";
			iExitCode = -1;
		}
	}
	else if (!stricmp(pMode, "Worker"))
	{
		g_debugManager << "Worker\n";

		if (bDaemon) Fork();

		FarmWorker farmWorker(BroadcastPort, NetworkPort);
		
		if (!farmWorker.Start())
		{
			g_debugManager << "Error occured starting worker";
			iExitCode = -1;
		}
	}
#ifdef WIN32
	else if (!stricmp(pMode, "Manager"))
	{
		g_debugManager << "Manager\n";

		FarmUserInterface farmUI(true, BroadcastPort, NetworkPort);
		
		if (!farmUI.Start())
		{
			g_debugManager << "Error occured starting manager";
			iExitCode = -1;
		}
	}
	else if (!stricmp(pMode, "User"))
	{
		g_debugManager << "User\n";

		FarmUserInterface farmUI(false, BroadcastPort, NetworkPort);
		
		if (!farmUI.Start())
		{
			g_debugManager << "Error occured starting user client";
			iExitCode = -1;
		}
	}
#endif
	else if (!stricmp(pMode, "List"))
	{
		g_debugManager << "List\n";

		if (bDaemon) Fork();

		FarmList farmList(BroadcastPort, NetworkPort);
		
		if (!farmList.Start())
		{
			g_debugManager << "Error occured running list.";
			iExitCode = -1;
		}
	}
	else if (!stricmp(pMode, "Job"))
	{
		g_debugManager << "Job: " << sJob << "\n";

		FarmClient farmClient(BroadcastPort, NetworkPort);

		int nJobNumber = farmClient.StartJob(sJob.c_str(), sJob.c_str(), true);

		printf("Job Number is %d\n", nJobNumber);
	}
	else if (!stricmp(pMode, "Kill"))
	{
		g_debugManager << "Kill Job: " << iKillId << "\n";

		FarmClient farmClient(BroadcastPort, NetworkPort);

		farmClient.KillJob(iKillId);
	}
	else if (!stricmp(pMode, "Prioritise"))
	{
		g_debugManager << "Prioritise Job: " << iPrioritiseId << "\n";

		FarmClient farmClient(BroadcastPort, NetworkPort);

		farmClient.PrioritiseJob(iPrioritiseId);
	}
	else if (!stricmp(pMode, "Dummy"))
	{
		StartDummyServer(NetworkPort);
	}
	else if (!stricmp(pMode, "Outsourcing"))
	{
		g_bOutsourcingMode = true;

		StartDummyServer(NetworkPort);
	}
	else
	{
		g_debugManager <<	"Please specify one of the following modes:\n"
							"Server / Worker / Manager / User / List";

		iExitCode = -1;
	}

	g_debugManager.Destroy();

	return iExitCode;
}

#ifdef __GNUC__
void Fork()
{
	/* Our process ID and Session ID */
	pid_t pid, sid;

	/* Fork off the parent process */
	pid = fork();
	if (pid < 0) {
		printf("Daemon failed to start.\n");
		exit(EXIT_FAILURE);
	}
	/* If we got a good PID, then
	we can exit the parent process. */
	if (pid > 0) {
		printf("Daemon started successfully.\n");
		exit(EXIT_SUCCESS);
	}

	/* Change the file mode mask */
//	umask(0);

	/* Open any logs here */        

	/* Create a new SID for the child process */
	sid = setsid();
	if (sid < 0) {
		/* Log the failure */
		exit(EXIT_FAILURE);
	}

	/* Change the current working directory */
	if ((chdir("/")) < 0) {
		/* Log the failure */
		exit(EXIT_FAILURE);
	}

	/* Close out the standard file descriptors */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	/* Daemon-specific initialization goes here */
}
#else
void Fork() {}	// dummy function for windows
#endif
