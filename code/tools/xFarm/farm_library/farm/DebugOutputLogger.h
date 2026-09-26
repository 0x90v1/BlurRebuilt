#ifndef __LOGDEBUGOUTPUT_H__
#define __LOGDEBUGOUTPUT_H__

#ifdef WIN32

// windows debug output logger
class DebugOutputLogger
{
public:
	DebugOutputLogger();
	~DebugOutputLogger();

	const std::string GetLog();
	void ClearLog();
	void SetProcess(int i);

private:
	std::string	m_log;
	int			m_iProcess;
	HANDLE		m_thread;
    HANDLE		AckEvent;
    LPVOID		SharedMem;
    HANDLE		ReadyEvent;
    HANDLE		SharedFile;
	CRITICAL_SECTION m_CriticalSection;

	static DWORD CALLBACK ThreadEntryPoint(void *);
	void MainFunction();
};

#else

// we don't have anything similar for other OS'es at the moment
class DebugOutputLogger
{
public:
	const std::string GetLog()	{ return ""; }
	void ClearLog()				{;}
	void SetProcess(int i)		{;}
};

#endif

#endif
