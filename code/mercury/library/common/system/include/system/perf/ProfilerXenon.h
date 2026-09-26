#ifndef __SYSTEM_PROFILER_XENON_H__
#define __SYSTEM_PROFILER_XENON_H__

#include <xbdm.h>

/// this header file is to be included by .cpp files that want to generate profiling data
/// .cpp files that want to read back and display generated profiling data must include ProfilerBackend.h
namespace Perf
{

/// PS3 specific part of the profiler backend	
class Profiler : public ProfilerCommon
{
public:
	/// constructor
			Profiler();

	/// destructor
	virtual ~Profiler();

	/// return a friendly name for the thread on an event
	virtual const char *ThreadGetFriendlyName(const Event &e);

	static DWORD __stdcall ContextSwitchNotifyFunction(ULONG dwNotification, DWORD dwParam);

private:
	/// Inititalise notification handler
	void InitialiseNotification();

	PDMN_SESSION m_pNotifySession;
};

} // namespace Perf

#endif
