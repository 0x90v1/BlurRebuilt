#pragma once

class FarmJob : public FarmStranger
{
public:
	FarmJob(const char* Command, const char* Comment);
	~FarmJob();

protected:
	virtual bool OnStart();
	virtual bool OnStop();

protected:
	virtual bool OnCommand(const char* Command);
	virtual bool OnIdle();

private:
	const char* m_Command;
	const char* m_Comment;
};