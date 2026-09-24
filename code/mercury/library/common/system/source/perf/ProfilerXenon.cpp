#include "SystemPch.h"

#include "System/Perf/ProfilerBackend.h"
#include "System/Perf/Profiler.h"
#include "System/Time/Time.h"
#include <xbdm.h>

namespace Perf
{

#ifndef ENABLE_MERCURY_PROFILER

/// constructor
Profiler::Profiler()
{
}

// destructor
Profiler::~Profiler()
{
}

const char *Profiler::ThreadGetFriendlyName(const Event &e)
{
	return "";
}

#else

extern Debug::Logger &g_Log;

/// constructor
Profiler::Profiler()
{
	InitialiseNotification();
}

// destructor
Profiler::~Profiler()
{
}

/// Inititalise notification handler
void Profiler::InitialiseNotification()
{
	if (DmOpenNotificationSession(0, &m_pNotifySession) !=  XBDM_NOERR)
		return;

	DmNotify(m_pNotifySession, DM_THREADSWITCH, ContextSwitchNotifyFunction);
}

DWORD Profiler::ContextSwitchNotifyFunction(ULONG dwNotification, DWORD dwParam)
{
	Profiler &profiler = Profiler::Instance();
	U64 iTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&iTime);
	int iCore = 1 + static_cast<U8>(GetCurrentProcessorNumber());
	PDMN_THREADSWITCH p = (PDMN_THREADSWITCH)dwParam;

	Perf::EmitEvent((char*)0xffffffff, iTime, p->OldThreadId, iCore, 1);
	Perf::EmitEvent((char*)0xffffffff, iTime, p->NewThreadId, iCore, 0);

	return TRUE;
}

/// return a friendly name for the thread on an event
const char *Profiler::ThreadGetFriendlyName(const Event &e)
{
	int iCore = PROFILER_EVENT_CORE(e);
	if (iCore == 0)
		return "Graphics";

	static char buffer[512];
	DM_THREADINFOEX dmti;
	dmti.Size = sizeof(DM_THREADINFOEX);
	if (e.thread == 0)
		return "[Idle]";

	if (DmGetThreadInfoEx(e.thread, &dmti) == XBDM_NOERR && dmti.ThreadNameAddress)
		return dmti.ThreadNameAddress;

	sprintf(buffer, "Thread %x", e.thread&0xffff);
	return buffer;
}

#endif

};
