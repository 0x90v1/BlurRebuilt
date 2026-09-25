#pragma once

class SpawnProcess;
class QueueSubscriber;

namespace Debug
{
	class RepeaterServer;
	struct LogEvent;
}


class FarmingWorker : public FarmStranger
{	
public:
	FarmingWorker();
	~FarmingWorker();

protected:
	virtual bool OnStart();
	virtual bool OnStop();

protected:
	virtual bool OnCommand(const char* Command);
	virtual bool OnIdle();

protected:
	virtual int	GetIdleTime() const;

private:
	void StartProcess();
	void StopProcess();
	bool IsProcessAlive() const;
	bool UpdateLog();
	void SendLogEvent(Debug::LogEvent *pEvent);

private:
	enum State
	{
		StateFree,
		StateOccupied,
		StateWaitingToFinish,
		StateAccepted
	};

private:
	State		m_State;
	String		m_Command;
	const char*	m_CmdPtr;
	Debug::RepeaterServer	*m_pRepeaterServer;			// server that allows clients to connect and post debug log events
	QueueSubscriber			*m_pQueueSubscriber;		// store mercury Debug Log events in a FIFO queue

	MT::Mutex				m_Mutex;
	int			m_iIdleMessageCounter;

private:
	static SpawnProcess *s_Process;
};
