#pragma once

class FarmList : public FarmStranger
{
public:
	FarmList();
	~FarmList();

protected:
	virtual bool OnStart();
	virtual bool OnStop();

protected:
	virtual bool OnCommand(const char* Command);
	virtual bool OnIdle();
};