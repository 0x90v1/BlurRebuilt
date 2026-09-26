#pragma once

class FarmingList : public FarmStranger
{
public:
	FarmingList();
	~FarmingList();

protected:
	virtual bool OnStart();
	virtual bool OnStop();

protected:
	virtual bool OnCommand(const char* Command);
	virtual bool OnIdle();
};
