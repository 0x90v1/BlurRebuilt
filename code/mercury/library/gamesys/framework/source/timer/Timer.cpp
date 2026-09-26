#include "FrameworkPch.h"
#include "Framework/Timer/Timer.h"

namespace Framework
{

//#pragma optimize("", off)

C_Timer::C_Timer() :
	m_Paused(false),
	m_frameAveragingCount(0.0f),
	m_FrameCycle(0.0f),
	m_FrameTime(0.0f),
	m_FrameRate(0.0f),
	m_ElapsedTime(0.0f),
	m_ForceFrameTime(-1.0f),
	m_TimerScale(1.0f)
{
	m_PrevTime = Time::GetPerformanceCounter();
}

C_Timer::~C_Timer()
{
}

void C_Timer::Update()
{
	const Time::C_Time currTime = Time::GetPerformanceCounter();
	m_FrameTime = (F32)(currTime - m_PrevTime) / (F32)Time::GetPerformanceFrequency();
	
	m_PrevTime = currTime;
	
	// update frame rate 4 times per second
	m_FrameCycle += m_FrameTime;
	m_frameAveragingCount += 1.f;
	if(m_FrameCycle > 0.25f)
	{
		m_FrameRate = (F32)m_frameAveragingCount / m_FrameCycle;
		m_frameAveragingCount = 0.f;
		m_FrameCycle = 0.f;
	}

	m_FrameTime = Maths::Clamped(m_FrameTime, 1.0f / 60.0f, 1.f / 15.f);

	// If we are forcing the frame time to be a specific value, use that value:
	if (m_ForceFrameTime >= 0.f)
	{
		m_FrameTime = m_ForceFrameTime;
	}

	// increment total elapsed time
	if(!m_Paused)
	{
		m_ElapsedTime += m_FrameTime;
	}
}

void C_Timer::SetTimerScale(F32 scale)
{
	m_TimerScale = Maths::Max(scale, 0.0f);
}

}; // namespace Framework
