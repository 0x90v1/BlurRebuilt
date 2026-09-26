#ifndef __DUMMY_SQLACCESS_H__
#define __DUMMY_SQLACCESS_H__

class DummySQLAccess : public SQLAccessInterface
{
public:
	DummySQLAccess();
	~DummySQLAccess();

	// add a new job to the database. the job structure has it's m_iID updated
	bool AddJob(Job &job);
	// find the job with matching m_iID and update it's data
	bool UpdateJob(Job &job);
	// update the status of the job
	bool UpdateJobStatus(Job &job);
	// start a log file for this job, returns the path to the log file to append text to
	std::string StartJobLog(int iJob);
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
	bool UpdateProgressBar(int iJobId, const char *description, int iProgress);
	/// get the user's settings
	bool GetUserOptions(std::string user, SQLOptions &options);
	// get the contents of a field from the job table
	std::string GetFieldFromJob(int iJob, std::string fieldname);
	// get a string field from the job table
	std::string GetJobOption(int iJobID, std::string field);
};

#endif
