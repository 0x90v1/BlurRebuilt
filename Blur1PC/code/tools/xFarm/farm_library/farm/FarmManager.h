#pragma once

class FarmingManager : public FarmStranger
{
public:
	FarmingManager();
	~FarmingManager();

protected:
	virtual bool OnStart();
	virtual bool OnStop();

public:
	virtual bool OnCommand(const char* Command);
	virtual bool OnIdle();
};
