#include "StdAfx.h"
#include "Farm.h"

#include "SQLaccessInterface.h"
#include "FatalEmail.h"
#include <vector>

#include <core/containers/string.h>
#include <system/xml/XmlDom.h>

using namespace xFarm;

const char*	DiscoverServerQuery		= "Server, Where Are You!";
const char*	DiscoverServerResponse	= "Client, I'm Over Here!";
#ifdef _DEBUG
const U64 FarmingServer::m_lTimerUpdateInterval = 1000 * 1;
#else
const U64 FarmingServer::m_lTimerUpdateInterval = 1000 * 30; // update timers every 30 seconds
#endif

extern bool g_bOutsourcingMode;

FarmingServer::FarmingServer(U16 broadcastPort, U16 networkPort, SQLAccessInterface *pSQLAccess) :
		m_Jobs(0),
		m_JobNo(0),
		m_Processes(0),
		m_JobVersion(0),
		m_MachineVersion(0),
		m_networkPort(networkPort),
		m_broadcastPort(broadcastPort),
		m_bScreensaverMode(false),
		m_bExitFlag(false),
		m_bBroadcast(true)
{
	m_Critical = new ost::Mutex("Server");
	m_Timer = GetSystemTimeAsU64();
	m_pDB = pSQLAccess;
}

FarmingServer::~FarmingServer()
{
	// detach all processes
	std::vector<FarmProcess*> deleteList;
	if (FarmProcess* Prc = m_Processes)
	{
		do
		{
			FarmProcess* Next = (Prc == Prc->m_Next) ? 0 : Prc->m_Next;

			// delete a dead process
			if (!Prc->IsEnabled())
			{
				LOGDEBUG( g_Log, "%s: is disabled and will be deleted\n", Prc->GetMyUserName());

				Prc->Stop();

				m_MachineVersion++;

				// mark this process for deletion
				deleteList.push_back( Prc );
			}

			Prc = Next;
		}
		while(Prc && (Prc != m_Processes));
	}

	// make all processes cancel themselves
	for (int i=0; i<(int)deleteList.size(); i++)
	{
		deleteList[i]->SetCancel();
		deleteList[i]->service->detach(deleteList[i]);
	}

	// wait for them to exit
	MT::Thread::Sleep(1000);

	for (int i=0; i<(int)deleteList.size(); i++)
	{
		DetachProcess(deleteList[i]);
		delete deleteList[i];
	}

	delete m_Critical;
	m_Critical = NULL;
	delete m_pDB;
}

void FarmingServer::SetDebugMode(bool bMode)
{
	m_bDebugMode = bMode;
}

void FarmingServer::SetBroadcast(bool bOn)
{
	m_bBroadcast = bOn;
}

void FarmingServer::UpdateWorkerTimers()
{
	// is it time to update yet?
	U64 lNow = GetSystemTimeAsU64();
	if (lNow - m_Timer < m_lTimerUpdateInterval)
		return;
	m_Timer = lNow;

    // 
	m_Critical->enter();

	// iterate through all the processes, updating their timers to the database
	int iTotal = 0;
	int iTotalUsed = 0;
	if (FarmProcess* Prc = m_Processes)
	{
		do
		{
			FarmProcess* Next = (Prc == Prc->m_Next) ? 0 : Prc->m_Next;

			if (Prc->m_Stranger && (Prc->m_Stranger->GetType() == ST_Assistant))
			{
				FarmAssistant *pFa = (FarmAssistant*)Prc->m_Stranger;

				// update the time this process was connected recently
				int iThis = UpdateProcessTime( Prc );
				iTotal += iThis; // add it to the total

				if (pFa->GetState() == 2)
					iTotalUsed += iThis;	// if the process is busy, then add this time to the used total as well
			}

			Prc = Next;
		}
		while(Prc && (Prc != m_Processes));
	}

	m_pDB->AddWorkerTime("total", iTotal);
	m_pDB->AddWorkerTime("inuse", iTotalUsed);

	m_Critical->leave();
}

bool FarmingServer::AcceptConnectionThread()
{
	ost::IPV4Address machine("0.0.0.0");
	ost::TCPSocket Svr( machine, m_networkPort );

	int ProcessNo(0);

	// this where we accept new incoming connections
	for(;;)
	{
		if (!Svr.isPendingConnection(1000))
		{
			std::vector<FarmProcess*> deleteList;

			// every so often, check whether all processes are still alive
			m_Critical->enter();

			if (FarmProcess* Prc = m_Processes)
			{
				do
				{
					FarmProcess* Next = (Prc == Prc->m_Next) ? 0 : Prc->m_Next;

					// delete a dead process
					if (!Prc->IsEnabled())
					{
						LOGDEBUG( g_Log, "%s: is disabled and will be deleted\n", Prc->GetMyUserName());

						Prc->Stop();

						m_MachineVersion++;

						// mark this process for deletion
						deleteList.push_back( DetachProcess(Prc) );
					}

					Prc = Next;
				}
				while(Prc && (Prc != m_Processes));
			}

			m_Critical->leave();

			// delete processes outside the m_Critical section, to avoid deadlocks
			for (unsigned int i=0; i<deleteList.size(); i++)
			{
				deleteList[i]->service->detach(deleteList[i]);
				FarmPrint("(%s): delete\n", deleteList[i]->GetMyUserName() );
				delete deleteList[i];
			}

			if (m_bScreensaverMode)
				UpdateWorkerTimers();

			if (m_bExitFlag)
			{
				// cancel all jobs
				while (m_Jobs)
					CancelJob(m_Jobs->JobNo);

				break;
			}
		}
		// accept a new incoming connection
		else
		{
			// create a new FarmProcess class for the client that is connecting to us
			FarmProcess* Prc = new FarmProcess(Svr, 0, this, ProcessNo++, GetService());

			// attach farm process to the server
			m_Critical->enter();

			AttachProcess(Prc);
			m_MachineVersion++;

			m_Critical->leave();
		}
	}

	return true;
}


bool FarmingServer::Run()
{
	// start the thread which responds to clients that are looking for our address
	Start();

	AcceptConnectionThread();

	return true;
}

// get a socket service
ost::SocketService *FarmingServer::GetService()
{
	// find one with a free slot
	for (unsigned int i=0; i<m_Services.size(); i++)
	{
		if (m_Services[i]->getCount() < 60)
			return m_Services[i];
	}

	// create a new service
	ost::SocketService *pService = new ost::SocketService();
	m_Services.push_back(pService);
	return pService;
}

void FarmingServer::UpdateJobToSQL(Job *pJob)
{
	if (m_bScreensaverMode)
		return;

	// update job parameters in database
	SQLAccessInterface::Job job;
	job.m_iID = pJob->JobNo;
	job.m_user = pJob->User;
	job.m_commandline = pJob->Command;
	job.m_comment = pJob->Comment;
	job.m_iStatus = pJob->State;
	m_pDB->UpdateJob( job );
}

void FarmingServer::UpdateJobStatusToSQL(int iID, int iStatus)
{
	if (m_bScreensaverMode)
		return;

	// update job parameters in database
	SQLAccessInterface::Job job;
	job.m_iID = iID;
	job.m_iStatus = iStatus;
	m_pDB->UpdateJobStatus( job );

	if (iStatus == JobStateAllocated)
		m_pDB->SetJobStartTime( iID );
	else if (iStatus == JobStateFailed || iStatus == JobStateCompleted || iStatus == JobStateKilled)
		m_pDB->SetJobEndTime( iID );
}

void FarmingServer::UpdateProgressBarToSQL(int iID, int iProgress, const char *pDescription)
{
	if (m_bScreensaverMode)
		return;

	// update job parameters in database
	m_pDB->UpdateProgressBar( iID, pDescription, iProgress );
}

int FarmingServer::AddJob(const char* User, const char* Data, int iParentProcess)
{
	m_Critical->enter();
	String Command(SnapString(Data));
	String Comment(SnapString(Data));
	int iFlags = SnapInt(Data);

	int JobNo;

	// read dependencies
	Ut::Vector<int> dependencies;
	int nDependencies = SnapInt(Data);
	for (int i=0; i<nDependencies; i++)
		dependencies.PushBack(SnapInt(Data));

	// read project name
	String Project(SnapString(Data));

	if (Project == "")
		Project = "unknown";

	// by default create our own ID
	JobNo = (m_JobNo++);

	if (!m_bScreensaverMode)
	{
		// add job to SQL database
		SQLAccessInterface::Job job;
		job.m_user = User;
		job.m_commandline = Command;
		job.m_comment = Comment;
		job.m_project = Project;
		if (m_pDB->AddJob( job ))
		{
			// get the job ID from the database
			JobNo = job.m_iID;
		}
	}

	if (Job* Jb = new Job)
	{
		Jb->JobNo	= JobNo;
		Jb->User	= User;
		Jb->Command	= Command;
		Jb->Comment	= Comment;
		Jb->Flags   = iFlags;
		Jb->Project = Project;
		Jb->iEncounteredFatal = -1;
		Jb->iLogLines = 0;
		Jb->iParentProcess = iParentProcess;

		for (int i=0; i<dependencies.Size(); i++)
			Jb->Dependencies.PushBack(dependencies[i]);

		m_JobVersion++;

		AttachJob(Jb);
	}
	else
		JobNo = -1;

	m_Critical->leave();

	return JobNo;
}

bool FarmingServer::KillJob(int JobNo)
{
	bool Result(false);

	m_Critical->enter();

	if (Job* Jb = FindJob(JobNo))
	{
		UpdateJobStatusToSQL(Jb->JobNo, JobStateKilled);

		if (Jb->State == JobStateFree)
			delete DetachJob(Jb);
		else
			Jb->State = JobStateKilled;

		m_JobVersion++;
		m_MachineVersion++;

		Result = true;
	}

	m_Critical->leave();

	return Result;
}

void FarmingServer::KillMyJobs(int ProcessNo)
{
	if (!m_Critical)
		return;
	m_Critical->enter();

	if (Job* Jb = m_Jobs)
	{
		do
		{
			Job* Next = (Jb == Jb->Next) ? 0 : Jb->Next;

			if (Jb->ProcessNo == ProcessNo)
			{
				switch(Jb->State)
				{
					case JobStateAllocated:
					case JobStateKilled:
						LOGDEBUG( g_Log, "killed job %d\n", Jb->JobNo);
						Jb->State		= JobStateKilled;
						UpdateJobStatusToSQL(Jb->JobNo, Jb->State);

						delete DetachJob(Jb);
					break;

					case JobStatePending:
						Jb->ProcessNo	= -1;
						Jb->State		= JobStateFree;

						UpdateJobStatusToSQL(Jb->JobNo, Jb->State);
					break;

					case JobStateRequeue:
						Jb->ProcessNo	= -1;
						Jb->State		= JobStateFree;

						AttachJob(DetachJob(Jb));

						UpdateJobStatusToSQL(Jb->JobNo, Jb->State);
					break;
				}

				break;
			}

			Jb = Next;
		}
		while(Jb && (Jb != m_Jobs));
	}

	m_JobVersion++;
	m_MachineVersion++;

	m_Critical->leave();
}

bool FarmingServer::CheckJobCanBeRun(Job *pJob, int iWorkerFlags)
{
	if (pJob->State != JobStateFree)
		return false;

	// flag has to match exactly, except PVS flag on worker is optional
	if ((pJob->Flags != iWorkerFlags) && (pJob->Flags != (iWorkerFlags&~JOB_FLAG_PVS)))
		return false;

	// check that all this job's dependencies have completed already
	for (int i=0; i<pJob->Dependencies.Size(); i++)
	{
		Job *pDep = FindJob(pJob->Dependencies[i]);
		if (pDep)
		{
			// one of this job's dependencies hasn't finished processing yet
			return false;
		}
	}

	return true;
}

// this is where we decide on a job to assign to a free client
int FarmingServer::GetFreeJob(int ProcessNo, char* Buffer, int MaxLength, int iWorkerFlags)
{
	int Result(-1);

	m_Critical->enter();

	// TODO: count number of workers on each job, and pick a job using that

	Job* Jb = m_Jobs;
	while (Jb)
	{
		// check that the job is free and that the flags match with the worker
		if (!CheckJobCanBeRun(Jb, iWorkerFlags))
		{
			// try next job
			Jb = Jb->Next;
			if (Jb == m_Jobs)
				break; // nothing found
			continue;
		}

		if (!m_bScreensaverMode)
		{
			Jb->State		= JobStatePending;

			UpdateJobStatusToSQL(Jb->JobNo, Jb->State);
		}

		Jb->ProcessNo	= ProcessNo;
		Result			= Jb->JobNo;

		snprintf(Buffer, MaxLength, "%s", Jb->Command.Get());

		m_JobVersion++;
		m_MachineVersion++;

		break;
	}

	m_Critical->leave();

	return Result;
}

// this is where a farm client has accepted a job and is processing it
void FarmingServer::AcceptJob(int JobNo)
{
	// notify executer that the job has been accepted by a client
	Util::C_String s;
	s.Printf("STARTED %d", JobNo);
	SendMessageToExecuter(s, JobNo);

	m_Critical->enter();

	if (Job* Jb = FindJob(JobNo))
	{
		if (Jb->State == JobStatePending)
		{
			if (!m_bScreensaverMode)
			{
				Jb->State		= JobStateAllocated;
				UpdateJobStatusToSQL(Jb->JobNo, JobStateAllocated);
				if (FarmProcess* Prc = FindProcess(Jb->ProcessNo))
				{
					m_pDB->UpdateJobWorker(Jb->JobNo, Prc->GetUserName());
				}
			}

			Jb->TimeStarted	= ost::getTicks();
			StartLog(Jb);

			m_JobVersion++;
			m_MachineVersion++;
		}
	}

	m_Critical->leave();
}

void FarmingServer::CancelJob(int JobNo)
{
	m_Critical->enter();

	if (Job* Jb = FindJob(JobNo))
	{
		switch(Jb->State)
		{
			case JobStateKilled:
				delete DetachJob(Jb);
			break;

			case JobStateRequeue:
				Jb->ProcessNo	= -1;
				Jb->State		= JobStateFree;

				AttachJob(DetachJob(Jb));
			break;
		}

//		UpdateJobStatusToSQL(Jb->JobNo, JobStateKilled);

		m_JobVersion++;
		m_MachineVersion++;
	}

	m_Critical->leave();
}

// start a new log file. log file path is generated by database
void FarmingServer::StartLog(Job *Jb)
{
	if (m_bScreensaverMode)
		return;

	// set job log in SQL database
	Jb->LogPath = m_pDB->StartJobLog( Jb->JobNo );
	Jb->Feedback.SetLogFilename( Jb->LogPath );
}


// find the instance of executer that spawned this job and send a message to them
void FarmingServer::SendMessageToExecuter(const Util::C_String &msg, int iJobNo)
{
	Job* Jb = FindJob(iJobNo);
	if (!Jb)
		return;

	m_Critical->enter();
	FarmProcess *p = m_Processes;
	if (p)
	{
		do
		{
			if (p->IsExecuter() && p->m_IsEnabled && p->m_ProcessNo == Jb->iParentProcess)
			{
				if (!p->Send(msg.Chars()))
				{
					// if we couldn't send the message to executer, disconnect it
					p->m_IsEnabled = false;
				}
			}
		}
		while((p = p->m_Next) != m_Processes);
	}
	m_Critical->leave();
}

// flag the server to be shut down (for executer to kill the dummy server)
void FarmingServer::SetShutdown()
{
	LOGDEBUG( g_Log, "received SHUTDOWN command from executer\n");

	m_bExitFlag = true;
}

// append a string to the end of the log file
void FarmingServer::AppendLog(int JobNo, int level, int secs, const char *logger, const char *msg)
{
	if (m_bScreensaverMode)
		return;

	Util::C_String s;

	// find job associated with this JobNo
	Job* Jb = FindJob(JobNo);

	// is this a message for executer?
	if (!stricmp(logger,"executer"))
	{
		// process it using the executer job feedback class
		if (Jb)
		{
			Jb->Feedback.Register(msg);

			// forward message to executer
			s.Printf("EXECUTER %d ", JobNo);
			s = s + Jb->Feedback.GetExecuterString();
			SendMessageToExecuter(s, JobNo);

			// update progress bar in SQL database
			int c = Jb->Feedback.m_Data.Size();
			if (c > FEEDBACK_INFO_PROGRESS_VALUE_1)
			{
				UpdateProgressBarToSQL(
					JobNo, 
					atoi(Jb->Feedback.m_Data[FEEDBACK_INFO_PROGRESS_VALUE_1]), 
					Jb->Feedback.m_Data[FEEDBACK_INFO_PROGRESS_TEXT_1].Chars());
			}
		}
	}
	else
	{
		// it's a normal log message; append it to the log
		if (Jb)
		{
			// remember if we encountered any FATAL log messages
			if (level == 1)
				Jb->iEncounteredFatal = Jb->iLogLines;
			Jb->iLogLines++;
		}

		// append it to the log
		Util::C_String message(msg);
		if (!g_bOutsourcingMode)
		{
			message = message.Replace("\n", "&crlf;");
			s.Printf("%d %d %s %s\n", level, (int)secs, logger, message.Chars());
		}
		else
		{
			s.Printf("%s\n", message.Chars());
		}

		if (Jb)
		{
			// get job's log file path
			FILE *f = fopen(Jb->LogPath.c_str(), "a");
			if (f)
			{
				// append text to log file
				fprintf(f, "%s", s.Chars());
				fclose(f);
			}
		}
	}
}

const Sys::C_XmlDomBaseNode* FindXmlNodeByTagValue(const Sys::C_XmlDomBaseNode* node, const Util::C_String& tag, const Util::C_String& value)
{
	if (node->GetType() == Sys::E_XmlDomNodeType_Element)
	{
		const Sys::C_XmlAttribute* xmlTag = ((const Sys::C_XmlDomElement*)node)->GetAttributes().GetAttributeByName(tag);

		if (xmlTag && xmlTag->value == value)
		{
			return node;
		}
	}

	const Sys::C_XmlDomBaseNode::Array& children = node->GetChildren();

	for (Sys::C_XmlDomBaseNode::Array::ConstIterator it = children.Begin(); it != children.End(); ++it)
	{
		const Sys::C_XmlDomBaseNode* childNode = FindXmlNodeByTagValue(*it, tag, value);
		if (childNode)
			return childNode;
	}

	return 0;
}

bool AlreadyAdded(const std::string &recipient, const Ut::Vector<std::string> &recipients)
{
	for(Ut::Vector<std::string>::ConstIterator it = recipients.Begin(); it != recipients.End(); ++it)
	{
		if(recipient == (*it))
		{
			return true;
		}
	}
	return false;
}

void LookForParentDataAndGetRecipients(const Sys::C_XmlDomBaseNode* node, Ut::Vector<std::string> &recipients)
{
	const Sys::C_XmlDomBaseNode::Array& children = node->GetChildren();

	for (Sys::C_XmlDomBaseNode::Array::ConstIterator it = children.Begin(); it != children.End(); ++it)
	{
		const Sys::C_XmlDomBaseNode* childNode = *it;
		if(childNode->GetType() == Sys::E_XmlDomNodeType_Element)
		{
			if(childNode->GetName() == "parent_data")
			{
				LookForParentDataAndGetRecipients(childNode, recipients);
			}
			else //search for email_addresses tag
			{
				const Sys::C_XmlDomBaseNode* xFarmTagNode = FindXmlNodeByTagValue(childNode, "xfarm_tag", "email_addresses");
				if(xFarmTagNode)
				{
					std::string value = ((Sys::C_XmlDomElement*)xFarmTagNode)->GetAttributes().GetValueByName("value");
					std::string delim("&#xA; ");

					size_t lastPos = value.find_first_not_of(delim, 0);
					size_t pos     = value.find_first_of(delim, lastPos);

					while (std::string::npos != pos || std::string::npos != lastPos)
					{
						std::string recipientToAdd = value.substr(lastPos, pos - lastPos);
						if(!AlreadyAdded(recipientToAdd, recipients))
						{
							recipients.PushBack(recipientToAdd);
						}
						lastPos = value.find_first_not_of(delim, pos);
						pos = value.find_first_of(delim, lastPos);
					}
				}
			}
		}
	}
}

void GetUniqueRecipients(const std::string &xmlPath, Ut::Vector<std::string> &recipients)
{
	Sys::C_XmlDom dom;
	if( dom.Read(xmlPath.c_str()) )
	{
		LookForParentDataAndGetRecipients(dom.GetRootNodePtr()->GetChildren()[0], recipients);
	}
}

/// send e-mails to the user if requested, and to crash mailing list if it's crashed
void FarmingServer::SendEmails(Job *pJob, int iType)
{
	if (m_bScreensaverMode)
		return;

	if (pJob->iEncounteredFatal != -1)
		iType = EMAIL_TYPE_CRASH;	// encountered fatal error - send a crash message

	Ut::Vector<std::string> recipients;

	// get the name of the user who started this job
	SQLOptions options;
	std::string username = m_pDB->GetFieldFromJob(pJob->JobNo, "user");
	std::string comment = m_pDB->GetFieldFromJob(pJob->JobNo, "comment");
	std::string mailsubject;

	// username: strip anything before the '/' e.g. Bizarre2/janv
	const char *psz = username.c_str();
	const char *p = strrchr(psz, '/');
	if (p)
		psz = p+1;
	username = psz;

	if(username == "")
	{
		return;
	}
	else if (username == "luntbuild") //job started by daemon, use email notification list
	{
		options.bEmailComplete	= true;
		options.bEmailCrash		= true;
		options.bEmailFail		= true;
		comment = pJob->Comment;

		std::string command = pJob->Command;
		std::string xmlPath = command.substr(command.find("--xml") + 6);
		GetUniqueRecipients(xmlPath, recipients);
	}
	else
	{
		//get the user's options. do we want to send him an e-mail?
		m_pDB->GetUserOptions(username, options);
	}

	std::string emailTypeDesc = "";
	
	switch (iType)
	{
	case EMAIL_TYPE_COMPLETE:
		if (options.bEmailComplete && options.sEmail != "")
			recipients.PushBack(options.sEmail);
		mailsubject = "Job Complete: " + comment;
		emailTypeDesc = "COMPLETED";
		break;
	case EMAIL_TYPE_FAILED:
		if (options.bEmailFail && options.sEmail != "")
			recipients.PushBack(options.sEmail);
		mailsubject = "Job Failed: " + comment;
		emailTypeDesc = "FAILED";
		break;
	case EMAIL_TYPE_CRASH:
		if (options.bEmailCrash && options.sEmail != "")
			recipients.PushBack(options.sEmail);
		
		mailsubject = "Fatal Error Report: " + comment;
		emailTypeDesc = "CRASH";

		std::string sProject(pJob->Project);

		// crash report recipient depends on the job type
		if (sProject == "Gotham Car")
			recipients.PushBack(std::string("gothamcar_crash@bizarrecreations.com"));
		else if (sProject == "Gotham City")
			recipients.PushBack(std::string("gotham_crash@bizarrecreations.com"));
		else if (sProject == "The Club")
			recipients.PushBack(std::string("theclub_crash@bizarrecreations.com"));
		else if (sProject == "Amax")
			recipients.PushBack(std::string("amax_crash@bizarrecreations.com"));
		else if (sProject == "MercuryGame")
			recipients.PushBack(std::string("mercury_crash@bizarrecreations.com"));
		else if (sProject == "Nightwing")
			recipients.PushBack(std::string("nightwing_crash@bizarrecreations.com"));
		else
			recipients.PushBack(std::string("janv@bizarrecreations.com"));

		break;
	}

	for (int i = 0; i < recipients.Size(); i++)
	{
		printf("Sending e-mail job no: %d, type: %s,  recipient: %s\n", pJob->JobNo, emailTypeDesc.c_str(), recipients[i].c_str());
	}

	/// send the e-mail
	if (SendEmail(pJob->JobNo, iType, pJob->iEncounteredFatal, recipients, mailsubject))
		printf("Email sent.\n", pJob->JobNo);
	else
		printf("Failed to send.\n", pJob->JobNo);
}

void FarmingServer::FinishJob(int JobNo, bool bOk)
{
	// notify executer that the job has finished
	Util::C_String s;
	if (bOk)
		s.Printf("COMPLETED %d", JobNo);
	else
		s.Printf("FAILED %d", JobNo);
	SendMessageToExecuter(s, JobNo);

	// find the job in the job list and delete it
	m_Critical->enter();

	if (Job* Jb = FindJob(JobNo))
	{
		if (bOk)
		{
			UpdateJobStatusToSQL(Jb->JobNo, JobStateCompleted);
			SendEmails(Jb, EMAIL_TYPE_COMPLETE);
		}
		else
		{
			UpdateJobStatusToSQL(Jb->JobNo, JobStateFailed);
			SendEmails(Jb, EMAIL_TYPE_FAILED);
		}

		delete DetachJob(Jb);

		m_JobVersion++;
		m_MachineVersion++;
	}

	m_Critical->leave();
}

void FarmingServer::FreeJob(int JobNo)
{
	m_Critical->enter();

	if (Job* Jb = FindJob(JobNo))
	{
		if (Jb->State != JobStateKilled)
		{
			Jb->ProcessNo	= -1;
			Jb->State		= JobStateFree;

			m_JobVersion++;
			m_MachineVersion++;
		}
		else
			CancelJob(JobNo);
	}

	m_Critical->leave();
}

bool FarmingServer::IsJobInQueue(int JobNo)
{
	bool Result(false);

	m_Critical->enter();

	Result = FindJob(JobNo) ? true : false;

	m_Critical->leave();

	return Result;
}

bool FarmingServer::IsJobCancelled(int JobNo)
{
	bool Result(false);
	if (!m_bScreensaverMode)
		Result = true;

	m_Critical->enter();

	if (Job* Jb = FindJob(JobNo))
		Result = (Jb->State == JobStateKilled) || (Jb->State == JobStateRequeue);

	m_Critical->leave();

	return Result;
}

bool FarmingServer::ListJobs(FarmProcess* Prc)
{
	bool Result(true);

	m_Critical->enter();

	if (Job* Jb = m_Jobs)
	{
		int Idx(0);

		do
		{
			if (Jb->State != JobStateKilled)
			{
				char Buffer[4096];

				snprintf(Buffer, sizeof(Buffer),
						"JOB %i %i %c %i %i %s %i %s %i %s",
						Idx++,
						Jb->JobNo,
						(Jb->State == JobStateFree) ? 'F' : 'A',
						Jb->ProcessNo,
						Jb->TimeStarted,
						Jb->User.Get(),
						strlen(Jb->Command),
						Jb->Command.Get(),
						strlen(Jb->Comment),
						Jb->Comment.Get()
					);

				if (!Prc->Send(Buffer))
				{
					Result = false;
					break;
				}
			}
		}
		while((Jb = Jb->Next) != m_Jobs);
	}

	m_Critical->leave();

	return Result;
}

void FarmingServer::SetScreensaverMode(bool bMode)
{
	m_bScreensaverMode = bMode;
}

bool FarmingServer::ListMachines(FarmProcess* ManagerPrc)
{
	bool Result(true);

	m_Critical->enter();

	if (FarmProcess* Prc = m_Processes)
	{
		do
		{
			if (Prc->m_Stranger && (Prc->m_Stranger->GetType() == ST_Assistant))
			{
				FarmAssistant *pStranger = (FarmAssistant*)Prc->m_Stranger;

				char Buffer[4096];

				int iJobNo = pStranger->GetJobNo();
				int iState = pStranger->GetState();
				if ((iState == FarmAssistant::StateAccepted || iState == FarmAssistant::StateOccupied) &&
					iJobNo == -1)
					iJobNo = -2;

				snprintf(Buffer, sizeof(Buffer),
						"MACHINE %i %s %i %i",
						Prc->m_ProcessNo,
						Prc->m_UserName.Get(),
						Prc->m_IsActive,
						iJobNo
					);

				if (!ManagerPrc->Send(Buffer))
				{
					Result = false;
					break;
				}
			}
		}
		while((Prc = Prc->m_Next) != m_Processes);
	}

	m_Critical->leave();

	return Result;
}

bool FarmingServer::PriorityJob(int JobNo)
{
	bool Result(false);

	m_Critical->enter();

	if (Job* Jb = FindJob(JobNo))
	{
		if (m_Jobs != Jb)
		{
			m_Jobs = AttachJob(DetachJob(Jb));

			m_JobVersion++;
		}

		Result = true;
	}

	m_Critical->leave();

	return Result;
}

bool FarmingServer::RequeueJob(int JobNo)
{
	bool Result(false);

	m_Critical->enter();

	if (Job* Jb = FindJob(JobNo))
	{
		if (Jb->State == JobStateFree)
			AttachJob(DetachJob(Jb));
		else
			Jb->State = JobStateRequeue;

		m_JobVersion++;

		Result = true;
	}

	m_Critical->leave();

	return Result;
}

int FarmingServer::GetJobVersion()
{
	return m_JobVersion;
}

int FarmingServer::GetMachineVersion()
{
	return m_MachineVersion;
}

bool FarmingServer::UpdateCommand(int JobNo, const char* Command)
{
	bool Result(false);

	m_Critical->enter();

	if (Job* Jb = FindJob(JobNo))
	{
		Jb->Command = Command;

		m_JobVersion++;

		Result = true;
	}

	m_Critical->leave();

	return Result;
}

bool FarmingServer::UpdateComment(int JobNo, const char* Comment)
{
	bool Result(false);

	m_Critical->enter();

	if (Job* Jb = FindJob(JobNo))
	{
		Jb->Comment = Comment;

		m_JobVersion++;

		Result = true;
	}

	m_Critical->leave();

	return Result;
}

bool FarmingServer::EnableMachine(int ProcessNo)
{
	bool Result(false);

	m_Critical->enter();

	if (FarmProcess* Prc = FindProcess(ProcessNo))
	{
		Prc->SetActive(true);
		m_MachineVersion++;
		Result = true;
	}

	m_Critical->leave();

	return Result;
}

bool FarmingServer::DisableMachine(int ProcessNo)
{
	bool Result(false);

	m_Critical->enter();

	if (FarmProcess* Prc = FindProcess(ProcessNo))
	{
		Prc->SetActive(false);
		m_MachineVersion++;
		Result = true;
	}

	m_Critical->leave();

	return Result;
}

// main fuction for the thread which replies to clients looking for this server (UDP)
void FarmingServer::ThreadMain()
{
	ost::IPV4Address everyone("0.0.0.0");			// receive packets from everyone
	ost::IPV4Broadcast subnet("255.255.255.255");	// broadcast to whole subnet
	ost::UDPBroadcast Skt(everyone, m_broadcastPort);

	Skt.setPeer(subnet, m_broadcastPort);

	char Buffer[4096];

	while(1)
	{
		// wait until we receive a message
		if (Skt.isPending(ost::Socket::pendingInput))
		{
			ost::IPV4Host serverAddr = Skt.getIPV4Sender();

			if (Skt.receive(Buffer, sizeof(Buffer)) == strlen(DiscoverServerQuery))
			{
				if (strncmp(Buffer, DiscoverServerQuery, strlen(DiscoverServerQuery)) == 0)
				{
					// if we received the server discover query, send back our response
					Skt.send(DiscoverServerResponse, (int)strlen(DiscoverServerResponse));
				}
			}
		}
	}
}

bool FindServer(U16 broadcastPort, U16 networkPort, ost::IPV4Host *pServerAddr )
{
	assert(broadcastPort != 0);
	assert(networkPort != 0);

	ost::IPV4Address everyone("0.0.0.0");			// receive packets from everyone
	ost::IPV4Broadcast subnet("255.255.255.255");	// broadcast to whole subnet
	ost::UDPBroadcast Skt(everyone, broadcastPort);

	Skt.setPeer(subnet, broadcastPort);

	char Buffer[4096];

	unsigned long	NextSendTime	= 0;
	int				Retries			= 5;

	for(;;)
	{
		if (NextSendTime < ost::getTicks())
		{
			if (Retries-- <= 0)
				return false;

			// send the "server where are you" broadcast query
			Skt.send(DiscoverServerQuery, strlen(DiscoverServerQuery));

			NextSendTime = ost::getTicks() + 200;
		}

		if (Skt.isPending(ost::Socket::pendingInput, 200))
		{
			if (pServerAddr)
				*pServerAddr = Skt.getIPV4Sender();

			int iReceived = Skt.receive(Buffer, sizeof(Buffer));
			if (iReceived <= 0)
				return false; // something went wrong

			Buffer[iReceived] = 0;

			if (iReceived == strlen(DiscoverServerResponse))
			{
				if (strncmp(Buffer, DiscoverServerResponse, strlen(DiscoverServerResponse)) == 0)
					return true; // found the server
			}
		}
	}
}


FarmingServer::Job* FarmingServer::AttachJob(Job* Jb)
{
	if (Jb)
	{
		assert((Jb->Next == 0) && (Jb->Previous == 0));

		if (m_Jobs)
		{
			Jb->Next			= m_Jobs;
			Jb->Previous		= m_Jobs->Previous;
			Jb->Next->Previous	= Jb;
			Jb->Previous->Next	= Jb;
		}
		else
			m_Jobs = Jb->Next = Jb->Previous = Jb;
	}

	return Jb;
}

FarmingServer::Job* FarmingServer::DetachJob(Job* Jb)
{
	if (Jb)
	{
		assert((Jb->Next != 0) && (Jb->Previous != 0));

		if ((Jb == m_Jobs) && ((m_Jobs = m_Jobs->Next) == Jb))
			m_Jobs = Jb->Next = Jb->Previous = 0;
		else
		{
			Jb->Next->Previous = Jb->Previous;
			Jb->Previous->Next = Jb->Next;

			Jb->Next = Jb->Previous = 0;
		}
	}

	return Jb;
}

FarmingServer::Job* FarmingServer::FindJob(int JobNo)
{
	if (Job* Jb = m_Jobs)
	{
		do
		{
			if (Jb->JobNo == JobNo)
				return Jb;
		}
		while((Jb = Jb->Next) != m_Jobs);
	}

	return 0;
}

FarmProcess* FarmingServer::AttachProcess(FarmProcess* Prc)
{
	if (Prc)
	{
		assert((Prc->m_Next == 0) && (Prc->m_Previous == 0));

		if (m_Processes)
		{
			Prc->m_Next			= m_Processes;
			Prc->m_Previous		= m_Processes->m_Previous;
			Prc->m_Next->m_Previous	= Prc;
			Prc->m_Previous->m_Next	= Prc;
		}
		else
			m_Processes = Prc->m_Next = Prc->m_Previous = Prc;

		// initialize the timer that counts how long this process has been active
		Prc->m_Timer = GetSystemTimeAsU64();
		Prc->SetScreenSaverMode( m_bScreensaverMode );
	}

	return Prc;
}

// read the amount of time a process has been active and save it to the SQL database
int FarmingServer::UpdateProcessTime(FarmProcess *pProcess)
{
	U64 lNewTime = GetSystemTimeAsU64();

	U64 lDiff = lNewTime - pProcess->m_Timer;
	pProcess->m_Timer = lNewTime;

	lDiff /= 1000;

	m_pDB->AddWorkerTime(pProcess->GetUserName(), lDiff );

	return lDiff;
}

FarmProcess* FarmingServer::DetachProcess(FarmProcess* Prc)
{
	if (Prc)
	{
		assert((Prc->m_Next != 0) && (Prc->m_Previous != 0));

		if ((Prc == m_Processes) && ((m_Processes = m_Processes->m_Next) == Prc))
			m_Processes = Prc->m_Next = Prc->m_Previous = 0;
		else
		{
			Prc->m_Next->m_Previous = Prc->m_Previous;
			Prc->m_Previous->m_Next = Prc->m_Next;

			Prc->m_Next = Prc->m_Previous = 0;
		}

		if (m_bScreensaverMode)
		{
			if (Prc->m_Stranger && (Prc->m_Stranger->GetType() == ST_Assistant))
				UpdateProcessTime(Prc);
		}
	}

	return Prc;
}

// find process with a given process number
FarmProcess* FarmingServer::FindProcess(int ProcessNo)
{
	if (FarmProcess* Prc = m_Processes)
	{
		do
		{
			if (Prc->m_ProcessNo == ProcessNo)
				return Prc;
		}
		while((Prc = Prc->m_Next) != m_Processes);
	}

	return 0;
}

bool FarmingServer::GetScreenSaverMode()
{
	return m_bScreensaverMode;
}

bool FarmingServer::KillJobByProcess(int iProcess)
{
	FarmProcess *p = FindProcess(iProcess);
	if (!p)
		return false;

	return p->Send("KILL");
}

