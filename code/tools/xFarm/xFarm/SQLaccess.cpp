#include "stdafx.h"

#include "farm/SQLaccessInterface.h"
#include "SQLaccess.h"

void FarmPrint(const char *c, ...);

SQLAccess::SQLAccess()
{
	m_pMutex = new MT::Mutex;
	m_pDB = new SQL::Database("localhost","jan","","farm");
	if (!m_pDB)
	{
		FarmPrint("Failed to connect to database.\n");
		return;
	}
/*
	SQL::Query q(*m_pDB);
	bool bOk;

	// delete entire table + contents (for debugging)
	bOk = q.execute("DROP TABLE jobs");

	// create a new table
	bOk = q.execute("CREATE TABLE jobs( "
		"id INT NOT NULL AUTO_INCREMENT, "
		"user TEXT NOT NULL, "
		"commandline TEXT NOT NULL, "
		"comment TEXT NOT NULL, "
		"log TEXT NOT NULL, "
		"status INT NOT NULL,"
		"created DATETIME NOT NULL,"
		"start DATETIME NOT NULL,"
		"end DATETIME NOT NULL,"
		"progressv INT NOT NULL, "
		"progressd TEXT NOT NULL, "
		"worker DATETIME NOT NULL,"
		"PRIMARY KEY(id) )");
*/
	FarmPrint("Connected to database.\n");
}

SQLAccess::~SQLAccess()
{
	if (m_pDB)
		delete m_pDB;
	delete m_pMutex;
}

// todo: remove ' and " characters from the commandline and comment strings
void SQLAccess::MakeStringSafe(std::string &string)
{
	// replace slashes with forward slashes
	for (unsigned int i=0; i<string.length(); i++)
	{
		if (string.at(i) == '\\')
			string.at(i) = '/';

		if (string.at(i) == '$' ||
			string.at(i) == '#' ||
			string.at(i) == '`' ||
			string.at(i) == '\'' ||
			string.at(i) == '\"')
			string.at(i) = '_';
	}
}

// add a new job to the database. the job structure has it's m_iID updated
bool SQLAccess::AddJob(Job &job)
{
	if (!m_pDB)
		return false;

	m_pMutex->Wait();
	SQL::Query q(*m_pDB);
	bool bOk;

	// remove unsafe characters from all the string
	MakeStringSafe(job.m_user);
	MakeStringSafe(job.m_commandline);
	MakeStringSafe(job.m_comment);
	MakeStringSafe(job.m_project);

	// put in an entry (rows), using automatically incrementing id 
	std::stringstream ss;
	ss << "insert into jobs values('',"
		"'" << job.m_user << "',"				// user
		"'" << job.m_commandline << "',"		// commandline
		"'" << job.m_comment << "',"			// comment
		"'" << job.m_project << "',"			// comment
		"'',"									// log
		"'" << job.m_iStatus << "',"			// status
		"now(),"								// creation time
		"'0000-00-00 00:00:00',"				// start time
		"'0000-00-00 00:00:00',"				// end time
		"0,"									// progress value
		"'',"									// progress description
		"''"									// worker
		")";
	bOk = q.execute(ss.str()); // insert value into table

	// get the id of the last row that we added
	q.get_result("SELECT LAST_INSERT_ID(id) AS alias FROM jobs ORDER BY id DESC LIMIT 1 ;");

	if (!q.fetch_row())
	{
		m_pMutex->Release();
		return false;
	}

	job.m_iID = q.getval();

	q.free_result();

	m_pMutex->Release();
	return true;
}

// get a string field from the job table
std::string SQLAccess::GetJobOption(int iJobID, std::string field)
{
	SQL::Query q(*m_pDB);

	std::stringstream ss;
	ss << "SELECT " << field << " FROM jobs WHERE id='" << iJobID << "' LIMIT 1";
	std::string rtn = q.get_string(ss.str());
	return rtn;
}

/// get a string option from the user table
std::string SQLAccess::GetUserOption(std::string user, std::string option)
{
	SQL::Query q(*m_pDB);

	std::stringstream ss;
	ss << "SELECT " << option << " FROM users WHERE user='" << user << "' LIMIT 1";
	std::string rtn = q.get_string(ss.str());
	return rtn;
}

/// get a bool option from the user table
bool SQLAccess::GetUserOptionBool(std::string user, std::string option)
{
	std::string result = GetUserOption(user, option);
	return atoi(result.c_str()) != 0;
}

// get the user name from a given job
std::string SQLAccess::GetFieldFromJob(int iJob, std::string fieldname)
{
	if (!m_pDB)
		return false;

	m_pMutex->Wait();
	SQL::Query q(*m_pDB);

	std::stringstream ss;
	ss << "SELECT " << fieldname << " FROM jobs WHERE id='" << iJob << "' LIMIT 1";
	std::string rtn = q.get_string(ss.str());
	m_pMutex->Release();
	return rtn;
}

/// get the user's settings
bool SQLAccess::GetUserOptions(std::string user, SQLOptions &options)
{
	if (!m_pDB)
		return false;

	m_pMutex->Wait();
	options.bEmailComplete = GetUserOptionBool(user, "mycomplete");
	options.bEmailFail = GetUserOptionBool(user, "myfail");
	options.bEmailCrash = GetUserOptionBool(user, "mycrash");
	options.sEmail = GetUserOption(user, "email");
	m_pMutex->Release();

	return true;
}

// find the job with matching m_iID and update it's data
bool SQLAccess::UpdateJob(Job &job)
{
	if (!m_pDB)
		return false;
/*
	SQL::Query q(*m_pDB);

	// update the values for this job
	std::stringstream ss;
	ss << "update jobs set "
		"user='" << job.m_user << "' "
		"commandline='" << job.m_commandline << "' "
		"comment='" << job.m_comment << "' "
		"status=" << job.m_iStatus << " "
		"where id=" << job.m_iID << " limit 1";
	return q.execute(ss.str());
*/
	return false; // this function doesn't work
}

// set the name of the worker that is processing this job
bool SQLAccess::UpdateJobWorker(int iJob, const char *psz)
{
	if (!m_pDB)
		return false;

	m_pMutex->Wait();
	SQL::Query q(*m_pDB);

	// update the values for this job
	std::stringstream ss;
	ss << "update jobs set "
		"worker='"<<psz<<"' "
		"where id=" << iJob << " limit 1";
	bool b = q.execute(ss.str());
	m_pMutex->Release();
	return b;
}

// set start time to now
bool SQLAccess::SetJobStartTime(int iJob)
{
	if (!m_pDB)
		return false;

	m_pMutex->Wait();
	SQL::Query q(*m_pDB);

	// update the values for this job
	std::stringstream ss;
	ss << "update jobs set "
		"start=now() "
		"where id=" << iJob << " limit 1";
	bool b = q.execute(ss.str());
	m_pMutex->Release();
	return b;
}

// set end time to now
bool SQLAccess::SetJobEndTime(int iJob)
{
	if (!m_pDB)
		return false;

	m_pMutex->Wait();
	SQL::Query q(*m_pDB);

	// update the values for this job
	std::stringstream ss;
	ss << "update jobs set "
		"end=now() "
		"where id=" << iJob << " limit 1";
	bool b = q.execute(ss.str());
	m_pMutex->Release();
	return b;
}

// save job log in database
std::string SQLAccess::StartJobLog(int iJob)
{
	if (!m_pDB)
		return false;

	// TODO: make the log folder editable from an .ini file
	char buffer2[512];
#ifdef WIN32
	if (!GetTempFileName("c:/www/logs", "log", 0, buffer2))
	{
		if (!GetTempFileName("d:/work/www/farm/logs", "log", 0, buffer2))
			return "";
	}	
#else
	assert(0); // hasn't been implemented yet on GNUC
#endif

	FILE *f = fopen(buffer2,"w");
	if (f)
	{
		fclose(f);

		char *psz = strrchr(buffer2,'\\');
		if (psz)
		{
			m_pMutex->Wait();

			SQL::Query q(*m_pDB);

			// update the log entry
			std::stringstream ss;
			ss << "update jobs set "
				"log='" << psz+1 << "' "
				"where id=" << iJob << " limit 1";
			if (q.execute(ss.str()))
			{
				m_pMutex->Release();
				return std::string(buffer2);	// return pathname of log file
			}
			m_pMutex->Release();
		}
	}
	return ""; // couldn't save log file
}

// update just the status of the job
bool SQLAccess::UpdateJobStatus(Job &job)
{
	if (!m_pDB)
		return false;

	m_pMutex->Wait();

	SQL::Query q(*m_pDB);

	// if the job is completed, update the progress bar as well
	if (job.m_iStatus == 5)
		UpdateProgressBar(job.m_iID, "Completed", 100);

	// update the values for this job
	std::stringstream ss;
	ss << "update jobs set "
		"status=" << job.m_iStatus << " "
		"where id=" << job.m_iID << " limit 1";
	bool b = q.execute(ss.str());
	m_pMutex->Release();
	return b;
}

// update the status of a worker
bool SQLAccess::UpdateWorkerStatus(const char *pszWorker, bool bOn)
{
    // get the last entry in the table for this worker
	if (!m_pDB)
		return false;

	m_pMutex->Wait();
	SQL::Query q(*m_pDB);

	// get the id of the last row that we added
	q.get_result("SELECT LAST_INSERT_ID(id) FROM workers ORDER BY time DESC LIMIT 1 ;");

	if (!q.fetch_row())
	{
		q.free_result();
		m_pMutex->Release();
		return false;
	}

	q.free_result();
	m_pMutex->Release();

	return true;
}

// get the current amount of seconds a worker has been active this timeslice
int SQLAccess::GetCurrentWorkerTime(const char *pszWorker, std::string &time)
{
	std::stringstream ss;
	SQL::Query q(*m_pDB);

	ss << "select `" << pszWorker << "` from workers where time='" << time << "';";
	q.get_result( ss.str() );
	MYSQL_ROW result = q.fetch_row();
	if (result)
	{
		// if the query succeeded, return the result
		return q.getval();
	}
	q.free_result();

	// query failed - this could be because of a missing worker field, or because of a missing entry for the current time
	// we will simply try to add both, if either one fails then that's no problem.

	// first, try adding a worker field
	ss.str("");
	ss << "alter table workers add `" << pszWorker << "` mediumint default 0 not null";
	q.execute(ss.str());
    
	// next, try adding an entry for the current time
	ss.str("");
	ss << "insert into workers ( time ) values ( '" << time << "' )";
	q.execute(ss.str());

	return 0;
}

// increase the number of seconds this worker has been active by a certain amount
bool SQLAccess::AddWorkerTime(const char *pszWorker, int iSeconds)
{
	if (!m_pDB)
		return false;

	m_pMutex->Wait();
	SQL::Query q(*m_pDB);

	// TODO: check whether there is an entry for the current hour, if not create one
	// add the number of points to the current hour

	// get system time and convert it to MySQL readable format
#ifdef WIN32
	SYSTEMTIME systime;
	GetLocalTime( &systime );
	char pszNowTime[512];
	sprintf(pszNowTime, "%04d-%02d-%02d %02d:00:00", systime.wYear, systime.wMonth, systime.wDay, systime.wHour);
	std::string time(pszNowTime);
#else
	std::string time("00-00-00- 00:00:00");
#endif

	// retrieve the current number of seconds for this worker
	int iCurWorkerTime = GetCurrentWorkerTime( pszWorker, time );

	iCurWorkerTime += iSeconds;

	// set the updated value
	std::stringstream ss;
	ss << "update workers set `" << pszWorker << "`=" << iCurWorkerTime << " where time='" << time << "' LIMIT 1";
	q.execute( ss.str() );

	m_pMutex->Release();
	return true;
};

// update the progress bar
bool SQLAccess::UpdateProgressBar(int iJob, const char *description, int iProgress)
{
	if (!m_pDB)
		return false;

	m_pMutex->Wait();
	SQL::Query q(*m_pDB);

	std::string _description(description);
	MakeStringSafe(_description);

	// update the values for this job
	std::stringstream ss;
	ss << "update jobs set "
		"progressv=" << iProgress << ", "
		"progressd='" << _description << "' "
		"where id=" << iJob << " limit 1";
	bool b = q.execute(ss.str());
	m_pMutex->Release();
	return b;
}
