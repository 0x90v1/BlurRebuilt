#ifndef __SQLACCESS_H__
#define __SQLACCESS_H__

class SQLAccess : public SQLAccessInterface
{
public:
	SQLAccess();
	~SQLAccess();

	// add a new job to the database. the job structure has it's m_iID updated
	bool AddJob(Job &job);
	// find the job with matching m_iID and update it's data
	bool UpdateJob(Job &job);
	// update the status of the job
	bool UpdateJobStatus(Job &job);
	// start a log file for this job, returns the path to the log file to append text to
	std::string SQLAccess::StartJobLog(int iJob);
	// set the job's end time to now
	bool SetJobStartTime(int iJob);
	// set the job's start time to now
	bool SetJobEndTime(int iJob);
	// set the job's worker
	bool UpdateJobWorker(int iJob, const char *psz);
	// update the status of a worker client
	bool UpdateWorkerStatus(const char *pszWorker, bool bOn);
	// increase the number of seconds this worker has been active by a certain amount
	bool AddWorkerTime(const char *pszWorker, int iSeconds);
	// update the progress bar
	bool UpdateProgressBar(int iJob, const char *description, int iProgress);
	/// get the user's settings
	bool GetUserOptions(std::string user, SQLOptions &options);
	// retrieve a specific field from a particular job from the job database
	std::string GetFieldFromJob(int iJob, std::string fieldname);
	// get a string field from the job table
	std::string GetJobOption(int iJobID, std::string field);

private:
	bool			m_bConnected;
	Ut::Vector<Job>	m_jobs;
	SQL::Database	*m_pDB;

	MT::Mutex		*m_pMutex;

	// fetch the worker time entry from the database, create a new entry if needed
	int GetCurrentWorkerTime(const char *pszWorker, std::string &time);
	// remove unsafe characters from a string
	void MakeStringSafe(std::string &string);
	/// get a string option from the user table
	std::string GetUserOption(std::string user, std::string option);
	/// get a bool option from the user table
	bool GetUserOptionBool(std::string user, std::string option);

};

#endif
