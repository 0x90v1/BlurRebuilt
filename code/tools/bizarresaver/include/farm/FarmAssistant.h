#pragma once

class FarmAssistant : public FarmStranger
{
public:
	FarmAssistant();
	~FarmAssistant();

protected:
	virtual bool OnStart();
	virtual bool OnStop();

protected:
	virtual bool OnCommand(const char* Command);
	virtual bool OnIdle();

protected:
	virtual StrangerType GetType() const;
	virtual DWORD GetIdleTime() const;

private:
	enum State
	{
		StateFree,
		StatePending,
		StateAccepted,
		StateOccupied
	};

private:
	State	m_State;
	int		m_JobNo;
};