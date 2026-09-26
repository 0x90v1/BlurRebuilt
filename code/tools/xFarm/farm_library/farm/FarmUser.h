#pragma once

class FarmWindow;
struct UiCommand;

class FarmUser : public FarmStranger
{
public:
	FarmUser(FarmWindow& Fw);
	~FarmUser();

protected:
	virtual bool OnStart();
	virtual bool OnStop();

public:
	virtual bool OnCommand(const char* Command);
	virtual bool OnIdle();

public:
	virtual int	 GetIdleTime() const;

public:
	bool NextCommand();

private:
	FarmWindow&	m_Wnd;
	UiCommand*	m_Command;
	int			m_JobVersion;
	int			m_MachineVersion;
};
