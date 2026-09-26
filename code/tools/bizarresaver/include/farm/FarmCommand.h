#pragma once

class FarmCommand : public FarmStranger
{
public:
	FarmCommand(int JobNo, const char* Command);
	~FarmCommand();

protected:
	virtual bool OnStart();
	virtual bool OnStop();

protected:
	virtual bool OnCommand(const char* Command);
	virtual bool OnIdle();

private:
	int			m_JobNo;
	const char*	m_Command;
};