#ifndef DISPLAY_DEVICE_D3D9_D3D9_H
#define DISPLAY_DEVICE_D3D9_D3D9_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/D3D9/D3D9DeviceTypes.h"

namespace Display
{
namespace D3D9
{

extern U32 g_MainThreadID;

extern IDirect3DDevice9* g_pd3dDevice;

IDirect3DDevice9* GetD3DDevice();

const Util::C_String GetTextureFormatAsString(D3DFORMAT d3dFmt);

const Util::C_String GetErrorFromHRESULT(HRESULT hr, bool includeDescription=false);

const E_Format GetFormatFromD3D(const D3DFORMAT d3dFormat);

}; // namespace D3D9

#if PLATFORM_XENON
void FlushCacheForGPU(void* __restrict ptr, int dataSize); // Forces cache lines references by this memory to be flushed so the GPU has current data
#endif

#ifdef GRAPHICS_D3D9
using namespace D3D9; // Make d3d9 the global device.
#endif

}; // namespace Display

#include "Display/Device/D3D9/D3D9.inl"

#endif // DISPLAY_DEVICE_D3D9_D3D9_H
