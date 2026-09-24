#pragma once

class FarmWindow;
struct UiCommand;

class FarmMessenger : public FarmStranger
{
public:
	FarmMessenger();
	~FarmMessenger();

protected:
	virtual bool OnStart();
	virtual bool OnStop();

public:
	virtual bool OnCommand(const char* Command);
	virtual bool OnIdle();

public:
	virtual int	 GetIdleTime() const;
	int			  GetJobStatus() const;

public:
	//bool NextCommand();

	void SetJobNumber(long jn)
	{
		m_jobNumber = jn;
	}

private:
	//int			m_JobVersion;
	//int			m_MachineVersion;
	long			m_jobNumber;
	int				m_iJobStatus;
};
