#include "stdafx.h"
#include "Display/Timing/GPUTiming.h"

#ifdef PLATFORM_PS3
#include "Display/Device/GCM/GCM.h"
#endif

#include "System/Perf/ProfilerEnable.h"

#ifdef PLATFORM_PC
#include "Display/Device/Device.h"	// [Blur2PC] D3D9 timestamp queries
#endif

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

#ifdef PLATFORM_PC

// [Blur2PC] PC GPU timing. The snapshot only has Xenon (GPU callbacks) and PS3 (GCM timestamps) backends, so
// on PC every GPU stat read 0. Not from retail (a Master build, CFG_GOLD, compiles GPU timing out). Each
// enabled event issues a D3D9 timestamp pair; a frame's pairs are read m_FrameCount-1 frames later, when
// the GPU has finished them, under that frame's disjoint and frequency queries.

namespace
{
	S32					s_PCTimingState = -1;	// -1 not tried, 0 unsupported, 1 ready
	bool				s_PCInFrame = false;
	U32					s_PCFrame = 0;
	IDirect3DQuery9*	s_pPCDisjoint[C_GPUTimingStats::m_FrameCount];
	IDirect3DQuery9*	s_pPCFreq[C_GPUTimingStats::m_FrameCount];
	bool				s_PCFrameIssued[C_GPUTimingStats::m_FrameCount];

	bool PCTimingReady()
	{
		if(s_PCTimingState < 0)
		{
			IDirect3DDevice9* pDevice = Device()->GetD3DDevice();
			s_PCTimingState = 0;
			if(pDevice && SUCCEEDED(pDevice->CreateQuery(D3DQUERYTYPE_TIMESTAMP, NULL)))
			{
				s_PCTimingState = 1;
				for(U32 i=0; i<C_GPUTimingStats::m_FrameCount; i++)
				{
					s_PCFrameIssued[i] = false;
					if(FAILED(pDevice->CreateQuery(D3DQUERYTYPE_TIMESTAMPDISJOINT, &s_pPCDisjoint[i])))
						s_pPCDisjoint[i] = NULL;
					if(FAILED(pDevice->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ, &s_pPCFreq[i])))
						s_pPCFreq[i] = NULL;
					if(!s_pPCDisjoint[i] || !s_pPCFreq[i])
						s_PCTimingState = 0;
				}
			}
		}
		return s_PCTimingState == 1;
	}
}

void C_GPUTimingStats::ReleaseQueries()
{
	for(C_GPUTimingStats* pStats = m_StatsList.First(); pStats; pStats = pStats->Next())
	{
		for(U32 f=0; f<m_FrameCount; f++)
		{
			for(U32 p=0; p<m_PairCount; p++)
			{
				for(U32 e=0; e<2; e++)
				{
					if(pStats->m_pQueries[f][p][e])
					{
						pStats->m_pQueries[f][p][e]->Release();
						pStats->m_pQueries[f][p][e] = NULL;
					}
				}
			}
			pStats->m_PairsUsed[f] = 0;
		}
	}

	if(s_PCTimingState >= 0)
	{
		for(U32 i=0; i<m_FrameCount; i++)
		{
			if(s_pPCDisjoint[i])
				s_pPCDisjoint[i]->Release();
			if(s_pPCFreq[i])
				s_pPCFreq[i]->Release();
			s_pPCDisjoint[i] = NULL;
			s_pPCFreq[i] = NULL;
			s_PCFrameIssued[i] = false;
		}
	}
	s_PCTimingState = -1;
	s_PCInFrame = false;
}

#endif // PLATFORM_PC

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

	#ifdef PLATFORM_PC
	memset(m_pQueries, 0, sizeof(m_pQueries));
	memset(m_PairsUsed, 0, sizeof(m_PairsUsed));
	m_DisplayTime	= 0;
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

	#ifdef PLATFORM_PC
	// [Blur2PC] the oldest issued frame, in the slot the next BeginFrame reuses
	const U32 readSlot = s_PCFrame % m_FrameCount;
	bool pcFrameReady = false;
	U64 pcFreq = 0;
	if(s_PCTimingState == 1 && s_PCFrameIssued[readSlot])
	{
		BOOL disjoint = TRUE;
		if(s_pPCDisjoint[readSlot]->GetData(&disjoint, sizeof(disjoint), 0) == S_OK && !disjoint &&
			s_pPCFreq[readSlot]->GetData(&pcFreq, sizeof(pcFreq), 0) == S_OK && pcFreq)
		{
			pcFrameReady = true;
		}
		s_PCFrameIssued[readSlot] = false;
	}
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

			#ifdef PLATFORM_PC
			// [Blur2PC] keeps the last good value when a frame's queries are not ready or were disjoint
			if(pcFrameReady && pTimingStats->m_PairsUsed[readSlot])
			{
				U64 ticks = 0;
				bool complete = true;
				for(U32 p=0; p<pTimingStats->m_PairsUsed[readSlot] && complete; p++)
				{
					U64 t0 = 0, t1 = 0;
					if(pTimingStats->m_pQueries[readSlot][p][0]->GetData(&t0, sizeof(t0), 0) == S_OK &&
						pTimingStats->m_pQueries[readSlot][p][1]->GetData(&t1, sizeof(t1), 0) == S_OK)
					{
						ticks += (t1 > t0) ? t1-t0 : 0;
					}
					else
					{
						complete = false;
					}
				}
				if(complete)
					pTimingStats->m_DisplayTime = (U64)((double)ticks * 1000000000.0 / (double)pcFreq);
			}
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
	#ifdef PLATFORM_PC
	if(PCTimingReady())
	{
		// [Blur2PC] this slot was read (or given up on) by the last UpdateStats
		const U32 slot = s_PCFrame % m_FrameCount;
		for(C_GPUTimingStats* pStats = m_StatsList.First(); pStats; pStats = pStats->Next())
			pStats->m_PairsUsed[slot] = 0;
		s_PCFrameIssued[slot] = false;
		s_pPCDisjoint[slot]->Issue(D3DISSUE_BEGIN);
		s_PCInFrame = true;
	}
	#endif

	#ifdef PLATFORM_XENON
	GetD3DDevice()->SetPredication(D3DPRED_ALL_RENDER | D3DPRED_ALL_Z);
	GetD3DDevice()->InsertCallback(D3DCALLBACK_IMMEDIATE, D3DBeginFrameCallback, 0);
	GetD3DDevice()->SetPredication(0);
	#endif
}

void C_GPUTimingStats::EndFrame()
{
	#ifdef PLATFORM_PC
	if(s_PCInFrame)
	{
		const U32 slot = s_PCFrame % m_FrameCount;
		s_pPCFreq[slot]->Issue(D3DISSUE_END);
		s_pPCDisjoint[slot]->Issue(D3DISSUE_END);
		s_PCFrameIssued[slot] = true;
		s_PCFrame++;
		s_PCInFrame = false;
	}
	#endif

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

	#ifdef PLATFORM_PC
	m_Pair		= -1;
	#endif
	
	if(m_Depth <= g_debugDisplayGPUTimingDepth && stats.m_Level <= g_debugDisplayGPUTimingLevel)
	{
		stats.SetDepth(m_Depth);

		#ifdef PLATFORM_PC
		// [Blur2PC] events outside Begin/EndFrame (device resize, loading screen) are not timed
		if(s_PCInFrame)
		{
			const U32 slot = s_PCFrame % C_GPUTimingStats::m_FrameCount;
			const U32 pair = stats.m_PairsUsed[slot];
			if(pair < C_GPUTimingStats::m_PairCount)
			{
				IDirect3DQuery9** ppQuery = stats.m_pQueries[slot][pair];
				IDirect3DDevice9* pDevice = Device()->GetD3DDevice();
				for(U32 e=0; e<2; e++)
				{
					if(!ppQuery[e] && FAILED(pDevice->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &ppQuery[e])))
						ppQuery[e] = NULL;
				}
				if(ppQuery[0] && ppQuery[1])
				{
					ppQuery[0]->Issue(D3DISSUE_END);
					stats.m_PairsUsed[slot] = pair+1;
					m_Pair = (S32)pair;
				}
			}
		}
		#endif

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

		#ifdef PLATFORM_PC
		if(m_Pair >= 0 && s_PCInFrame)
			pTimings->m_pQueries[s_PCFrame % C_GPUTimingStats::m_FrameCount][m_Pair][1]->Issue(D3DISSUE_END);
		#endif

		#ifdef PLATFORM_XENON
		GetD3DDevice()->SetPredication(D3DPRED_ALL_RENDER | D3DPRED_ALL_Z);
		GetD3DDevice()->InsertCallback(D3DCALLBACK_IMMEDIATE, D3DEndTimingCallback, (DWORD)m_pStats);
		GetD3DDevice()->SetPredication(0);
		#endif
	}
}

} // namespace Display
