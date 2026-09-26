#pragma once

class FarmManager : public FarmStranger
{
public:
	FarmManager();
	~FarmManager();

protected:
	virtual bool OnStart();
	virtual bool OnStop();

public:
	virtual bool OnCommand(const char* Command);
	virtual bool OnIdle();
};