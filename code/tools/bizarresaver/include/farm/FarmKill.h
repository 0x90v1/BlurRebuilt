#pragma once

class FarmKill : public FarmStranger
{
public:
	FarmKill(int JobNo);
	~FarmKill();

protected:
	virtual bool OnStart();
	virtual bool OnStop();

protected:
	virtual bool OnCommand(const char* Command);
	virtual bool OnIdle();

private:
	int m_JobNo;
};