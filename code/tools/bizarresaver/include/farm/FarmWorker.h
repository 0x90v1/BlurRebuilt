#pragma once

class FarmWorker : public FarmStranger
{
public:
	FarmWorker();
	~FarmWorker();

protected:
	virtual bool OnStart();
	virtual bool OnStop();

protected:
	virtual bool OnCommand(const char* Command);
	virtual bool OnIdle();

protected:
	virtual DWORD GetIdleTime() const;

private:
	void StartProcess();
	void StopProcess();
	bool IsProcessAlive() const;

private:
	enum State
	{
		StateFree,
		StateOccupied,
		StateAccepted
	};

private:
	State		m_State;
	String		m_Command;
	const char*	m_CmdPtr;

private:
	static HANDLE s_Process;
};