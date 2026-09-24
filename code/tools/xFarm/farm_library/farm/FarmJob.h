#pragma once

class FarmJob : public FarmStranger
{
protected:
	long	m_jobNumber;

public:
	FarmJob(const char* Command, const char* Comment, int iFlags);
	~FarmJob();

	bool IsJobStarted(void) const
	{
		return (m_jobNumber != -1);
	}

	long GetJobNumber(void) const
	{
		return m_jobNumber;
	}

protected:
	virtual bool OnStart();
	virtual bool OnStop();

protected:
	virtual bool OnCommand(const char* Command);
	virtual bool OnIdle();

private:
	const char* m_Command;
	const char* m_Comment;
	int	m_Flags;
};

