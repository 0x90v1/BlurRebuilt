#ifndef __SQLACCESS_H__
#define __SQLACCESS_H__

class SQLAccess
{
public:
	// struct for communicating job info between sql database 
	class Job
	{
	public:
		Job();

		int				m_iID;
		std::string		m_user;
		std::string		m_comment;
		std::string		m_commandline;
		int				m_iStatus;
	};

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

private:
	bool			m_bConnected;
	CMyVector<Job>	m_jobs;
	SQL::Database	*m_pDB;

	// fetch the worker time entry from the database, create a new entry if needed
	int GetCurrentWorkerTime(const char *pszWorker, std::string &time);
	// remove unsafe characters from a string
	void MakeStringSafe(std::string &string);

};

#endif
