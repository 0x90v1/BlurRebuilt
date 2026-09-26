#pragma once

class FarmRequeue : public FarmStranger
{
public:
	FarmRequeue(int JobNo);
	~FarmRequeue();

protected:
	virtual bool OnStart();
	virtual bool OnStop();

protected:
	virtual bool OnCommand(const char* Command);
	virtual bool OnIdle();

private:
	int m_JobNo;
};