#pragma once

class FarmAssistant : public FarmStranger
{
public:
	enum State
	{
		StateFree,
		StatePending,
		StateAccepted,
		StateOccupied
	};

public:
	FarmAssistant();
	~FarmAssistant();

	void	SetScreensaverMode(bool bMode);
	int		GetState() const;
	int		GetJobNo() const;
	void	SetFlags(int);

protected:
	virtual bool OnStart();
	virtual bool OnStop();

protected:
	virtual bool OnCommand(const char* Command);
	virtual bool OnIdle();
	void	ReceiveLogEvent(const char *pData);

protected:
	virtual StrangerType GetType() const;
	virtual int GetIdleTime() const;

private:
	State	m_State;
	int		m_JobNo;
	unsigned long m_LastCommandTime;
	bool	m_bScreensaverMode;
	bool	m_bReceiveLogEvent;
	int		m_Flags;
};
