#ifndef DISPLAY2_XB2_UTILS_H
#define DISPLAY2_XB2_UTILS_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display2/Device/Device.h"

void __cdecl GPUFinishTimingCallback(DWORD context);
void __cdecl GPUStartTimingCallback(DWORD context);
void __cdecl GPUStopTimingCallback(DWORD context);

namespace Display2
{

void FlushDataCacheBlock(const U32 size, U8* pAddress);

} // namespace Display2

#endif // DISPLAY2_XB2_UTILS_H
