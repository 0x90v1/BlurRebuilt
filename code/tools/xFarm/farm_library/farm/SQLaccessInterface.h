#ifndef __SQLACCESSINTERFACE_H__
#define __SQLACCESSINTERFACE_H__

struct SQLOptions
{
	bool bEmailComplete;
	bool bEmailFail;
	bool bEmailCrash;
	std::string sEmail;
};

class SQLAccessInterface
{
public:
	// struct for communicating job info between sql database 
	class Job
	{
	public:
		Job() :
			m_iID(-1),
			m_iStatus(0)
		{
		}

		int				m_iID;
		std::string		m_user;
		std::string		m_comment;
		std::string		m_project;
		std::string		m_commandline;
		int				m_iStatus;
	};

	virtual ~SQLAccessInterface() {};

	// add a new job to the database. the job structure has it's m_iID updated
	virtual bool AddJob(Job &job) = 0;
	// find the job with matching m_iID and update it's data
	virtual bool UpdateJob(Job &job) = 0;
	// update the status of the job
	virtual bool UpdateJobStatus(Job &job) = 0;
	// start a log file for this job, returns the path to the log file to append text to
	virtual std::string StartJobLog(int iJob) = 0;
	// set the job's end time to now
	virtual bool SetJobStartTime(int iJob) = 0;
	// set the job's start time to now
	virtual bool SetJobEndTime(int iJob) = 0;
	// set the job's worker
	virtual bool UpdateJobWorker(int iJob, const char *psz) = 0;
	// update the status of a worker client
	virtual bool UpdateWorkerStatus(const char *pszWorker, bool bOn) = 0;
	// increase the number of seconds this worker has been active by a certain amount
	virtual bool AddWorkerTime(const char *pszWorker, int iSeconds) = 0;
	// update the progress bar
	virtual bool UpdateProgressBar(int iJob, const char *description, int iProgress) = 0;
	/// get the user's settings
	virtual bool GetUserOptions(std::string user, SQLOptions &options) = 0;
	// get the user name from a given job
	virtual std::string GetFieldFromJob(int iJob, std::string fieldname) = 0;
	// get a string field from the job table
	virtual std::string GetJobOption(int iJobID, std::string field) = 0;
};

#endif
