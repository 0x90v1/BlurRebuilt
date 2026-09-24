#include "StdAfx.h"
#include "Display2/Utils/Utils.h"
#include "Display2/Device/Device.h"
#include <System/Time/Time.h>

void __cdecl GPUFinishTimingCallback(DWORD context)
{
	Display2::C_Timing* pTiming = (Display2::C_Timing*)context;
	pTiming->m_LastTime = pTiming->m_CurrTime;
	pTiming->m_TempTime = pTiming->m_CurrTime = 0;
}

void __cdecl GPUStartTimingCallback(DWORD context)
{
	Display2::C_Timing* pTiming = (Display2::C_Timing*)context;
	pTiming->m_TempTime = Time::GetPerformanceCounter();
}

void __cdecl GPUStopTimingCallback(DWORD context)
{
	Display2::C_Timing* pTiming = (Display2::C_Timing*)context;
	pTiming->m_TempTime = Time::GetPerformanceCounter() - pTiming->m_TempTime;
	pTiming->m_CurrTime += pTiming->m_TempTime;
}

namespace Display2
{

Util::C_String g_VertexShaderFileExt = "x2vsh";

Util::C_String g_PixelShaderFileExt = "x2psh";

void FlushDataCacheBlock(const U32 size, U8* pAddress)
{
	for(U32 i=0; i<size; i+=128)
		__dcbf(i, pAddress);
}

} // namespace Display2
