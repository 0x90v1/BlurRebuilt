#include "stdafx.h"

#include "System/Serialise.h"

#include "farm/SQLAccessInterface.h"
#include "DummySQLAccess.h"

// dummy functions for operating systems other than Windows
// TODO: get hold of PowerPC/Cygwin versions of libmysql and integrate that into xFarm?
DummySQLAccess::DummySQLAccess() {}
DummySQLAccess::~DummySQLAccess() {}
bool DummySQLAccess::AddJob(Job &job) { return false; }
bool DummySQLAccess::UpdateJob(Job &job) { return false; }
bool DummySQLAccess::UpdateJobWorker(int iJob, const char *psz) { return false; }
bool DummySQLAccess::SetJobStartTime(int iJob) { return false; }
bool DummySQLAccess::SetJobEndTime(int iJob) { return false; }
bool DummySQLAccess::UpdateWorkerStatus(const char *pszWorker, bool bOn) { return false; }
bool DummySQLAccess::AddWorkerTime(const char *pszWorker, int iSeconds) { return false; }
bool DummySQLAccess::UpdateJobStatus(Job &job) { return false; }
bool DummySQLAccess::UpdateProgressBar(int iJobID, const char *p, int iProgress) { return false; }
bool DummySQLAccess::GetUserOptions(std::string user, SQLOptions &options) { return false; }
std::string DummySQLAccess::GetFieldFromJob(int iJob, std::string fieldname) { return ""; }
std::string DummySQLAccess::GetJobOption(int iJobID, std::string field) { return ""; }

#ifdef WIN32

#define MAX_LOG_FILES 10

extern bool g_bOutsourcingMode;

std::string DummySQLAccess::StartJobLog(int iJob)
{
	char buffer2[1024];

	char *pszServer = "\\\\farmserver\\logs\\";
	if (g_bOutsourcingMode)
	{
		pszServer = "c:\\pgr4\\logs";
	}

	// save our local export log to the farmserver so we can view it across the network
	if (!GetTempFileName(pszServer, "log", 0, buffer2))
	{
		return "";
	}	

	FILE *f = fopen(buffer2,"w");
	if (f)
	{
		fclose(f);
		return std::string(buffer2);
	}
	return ""; // couldn't save log file
}

#else

std::string DummySQLAccess::StartJobLog(int iJob)
{
	return "";
}

#endif
