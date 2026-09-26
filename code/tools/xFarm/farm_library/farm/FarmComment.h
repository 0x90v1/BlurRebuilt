#pragma once

class FarmComment : public FarmStranger
{
public:
	FarmComment(int JobNo, const char* Comment);
	~FarmComment();

protected:
	virtual bool OnStart();
	virtual bool OnStop();

protected:
	virtual bool OnCommand(const char* Command);
	virtual bool OnIdle();

private:
	int			m_JobNo;
	const char*	m_Comment;
};
