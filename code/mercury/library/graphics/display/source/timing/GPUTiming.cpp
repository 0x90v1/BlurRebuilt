#include "stdafx.h"
#include "Display/Timing/GPUTiming.h"

#ifdef PLATFORM_PS3
#include "Display/Device/GCM/GCM.h"
#endif

#include "System/Perf/ProfilerEnable.h"

#ifdef ENABLE_MERCURY_PROFILER
#include <System/Perf/Profiler.h>
#include <System/Perf/ProfilerBackend.h>
#endif

Debug::Int		g_debugDisplayGPUTimingDepth	("Graphics.Display.Render.TimingRender.GPUDepth", 		9, Debug::Range(0, 9));
Debug::Int		g_debugDisplayGPUTimingLevel	("Graphics.Display.Render.TimingRender.GPULevel", 		3, Debug::Range(0, 9));
Debug::Bool		g_debugDisplayGPUUpdateStats	("Graphics.Display.Render.TimingRender.GPUUpdateStats",	true);

#ifdef PLATFORM_XENON

void __cdecl D3DBeginFrameCallback(DWORD context)
{
	Display::C_GPUTimingStats* pStats = Display::C_GPUTimingStats::m_StatsList.First();
	while(pStats)
	{
		pStats->m_StartTime = pStats->m_TempTime = pStats->m_EndTime = 0;
		pStats = pStats->Next();
	}
}

void __cdecl D3DEndFrameCallback(DWORD context)
{
	Display::C_GPUTimingStats* pStats = Display::C_GPUTimingStats::m_StatsList.First();
	while(pStats)
	{
		pStats->m_DisplayTime = pStats->m_TempTime;
		pStats = pStats->Next();
	}
}

void __cdecl D3DBeginTimingCallback(DWORD context)
{
	Display::C_GPUTimingStats* pStats = (Display::C_GPUTimingStats*)context;
	pStats->m_StartTime = Time::GetPerformanceCounter();

	#ifdef ENABLE_MERCURY_PROFILER
	Perf::EmitEvent(pStats->m_Name.Chars(), __mftb(), -1, 0, 0);
	#endif
}

void __cdecl D3DEndTimingCallback(DWORD context)
{
	Display::C_GPUTimingStats* pStats = (Display::C_GPUTimingStats*)context;
	pStats->m_EndTime = Time::GetPerformanceCounter();
	pStats->m_TempTime += pStats->m_EndTime-pStats->m_StartTime;

	#ifdef ENABLE_MERCURY_PROFILER
	Perf::EmitEvent(NULL, __mftb(), -1, 0, 1);
	#endif
}

#endif // PLATFORM_XENON

namespace Display
{

// C_GPUTimingStats

Util::C_List<C_GPUTimingStats> C_GPUTimingStats::m_StatsList;

C_GPUTimingStats::C_GPUTimingStats(const S32 level, const Maths::RGBA colour, const Util::C_String80& name) :
	C_TimingStats(level, colour, name)
{
	#ifdef PLATFORM_PS3
	m_TimingRead	= 1;
	m_TimingWrite	= 0;
	for(U32 i=0; i<m_TimingCount; i++)
		m_TimingIndex[i] = 0;
	#endif

	#ifdef PLATFORM_XENON
	m_DisplayTime	= 0;
	m_StartTime		= 0;
	m_TempTime		= 0;
	m_EndTime		= 0;
	#endif

	m_StatsList.PushLast(this);
}

C_GPUTimingStats::~C_GPUTimingStats()
{
	m_StatsList.Remove(this);
}

const bool C_GPUTimingStats::Show() const
{
	if(m_Depth != -1 && m_Depth <= g_debugDisplayGPUTimingDepth && m_Level <= g_debugDisplayGPUTimingLevel)
	{
		return true;
	}
	else
	{
		return false;
	}
}

const C_GPUTimingStats *C_GPUTimingStats::FindStats(const char *psz)
{
	for (C_GPUTimingStats* pStats = m_StatsList.First(); pStats; pStats = pStats->Next())
	{
		if (!stricmp(pStats->m_Name.Chars(), psz))
			return pStats;
	}

	return NULL;
}

void C_GPUTimingStats::UpdateStats()
{
	if(!g_debugDisplayGPUUpdateStats)
		return;

	#ifdef PLATFORM_PS3
	//cellGcmFinish(0);
	#endif

	C_GPUTimingStats* pTimingStats = m_StatsList.First();
	while(pTimingStats)
	{
		if(pTimingStats->m_Enabled)
		{
			#ifdef PLATFORM_PS3
			U64 oldTime = cellGcmGetTimeStamp(pTimingStats->m_TimingIndex[pTimingStats->m_TimingRead]);
			U64 newTime = cellGcmGetTimeStamp(pTimingStats->m_TimingIndex[pTimingStats->m_TimingRead]+1);
			U64 theTime = newTime-oldTime;
			pTimingStats->m_Frames[Perf::E_TimingFrame_Curr].m_TotalTime = theTime;
			pTimingStats->AdvanceTimingRead();
			pTimingStats->AdvanceTimingWrite();

			#ifdef ENABLE_MERCURY_PROFILER
			Perf::EmitEvent(pTimingStats->m_Name.Chars(), oldTime/14, -1, 0, 0);
			Perf::EmitEvent(NULL, newTime/14, -1, 0, 1);
			#endif

			#endif

			#ifdef PLATFORM_XENON
			pTimingStats->m_Frames[Perf::E_TimingFrame_Curr].m_TotalTime = pTimingStats->m_DisplayTime;

			#endif
		}
		else
		{
			pTimingStats->m_Frames[Perf::E_TimingFrame_Curr].m_TotalTime = 0;
		}
		
		pTimingStats->Update();

		pTimingStats = pTimingStats->Next();
	}
}

void C_GPUTimingStats::ResetPeaks()
{
	C_GPUTimingStats* pStats = m_StatsList.First();
	while(pStats)
	{
		pStats->ResetPeak();
		pStats = pStats->Next();
	}
}

void C_GPUTimingStats::BeginFrame()
{
	#ifdef PLATFORM_XENON
	GetD3DDevice()->SetPredication(D3DPRED_ALL_RENDER | D3DPRED_ALL_Z);
	GetD3DDevice()->InsertCallback(D3DCALLBACK_IMMEDIATE, D3DBeginFrameCallback, 0);
	GetD3DDevice()->SetPredication(0);
	#endif
}

void C_GPUTimingStats::EndFrame()
{
	#ifdef PLATFORM_XENON
	GetD3DDevice()->SetPredication(D3DPRED_ALL_RENDER | D3DPRED_ALL_Z);
	GetD3DDevice()->InsertCallback(D3DCALLBACK_IMMEDIATE, D3DEndFrameCallback, 0);
	GetD3DDevice()->SetPredication(0);
	#endif
}

// C_GPUTimingEvent

MT::ThreadLocalStorage C_GPUTimingEvent::m_Tls;

C_GPUTimingEvent::C_GPUTimingEvent(C_GPUTimingStats& stats) // __restrict // JASOND: Intel compiler doesn't allow this...
{
	C_GPUTimingEvent* const __restrict pParent = (C_GPUTimingEvent*)m_Tls.GetData();
	m_Tls.SetData(this);

	m_pParent	= pParent;
	m_pStats	= &stats;
	m_Depth		= m_pParent ? m_pParent->m_Depth+1 : m_Depth = 0;
	
	if(m_Depth <= g_debugDisplayGPUTimingDepth && stats.m_Level <= g_debugDisplayGPUTimingLevel)
	{
		stats.SetDepth(m_Depth);

		#ifdef PLATFORM_PS3
		m_Index = Display::GCM::GCMGetNextReportIndex2();
		GCMSetTimeStamp(m_Index);
		#endif

		#ifdef PLATFORM_XENON
		GetD3DDevice()->SetPredication(D3DPRED_ALL_RENDER | D3DPRED_ALL_Z);
		GetD3DDevice()->InsertCallback(D3DCALLBACK_IMMEDIATE, D3DBeginTimingCallback, (DWORD)m_pStats);
		GetD3DDevice()->SetPredication(0);
		#endif

		stats.m_Enabled = true;
	}
	else
	{
		stats.m_Enabled = false;
	}
}

C_GPUTimingEvent::~C_GPUTimingEvent() // __restrict // JASOND: Intel compiler doesn't allow this...
{
	C_GPUTimingStats* const __restrict pTimings(m_pStats);
	C_GPUTimingEvent* const __restrict pParent(m_pParent);
	
	pTimings->AddCallCount(Perf::E_TimingFrame_Curr, 1);
	
	m_Tls.SetData(pParent);

	if(m_Depth <= g_debugDisplayGPUTimingDepth && pTimings->m_Level <= g_debugDisplayGPUTimingLevel)
	{
		#ifdef PLATFORM_PS3
		pTimings->m_TimingIndex[pTimings->m_TimingWrite] = m_Index;
		GCMSetTimeStamp(m_Index+1);
		#endif

		#ifdef PLATFORM_XENON
		GetD3DDevice()->SetPredication(D3DPRED_ALL_RENDER | D3DPRED_ALL_Z);
		GetD3DDevice()->InsertCallback(D3DCALLBACK_IMMEDIATE, D3DEndTimingCallback, (DWORD)m_pStats);
		GetD3DDevice()->SetPredication(0);
		#endif
	}
}

} // namespace Display
