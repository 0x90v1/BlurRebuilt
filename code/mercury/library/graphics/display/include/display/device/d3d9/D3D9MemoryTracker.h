#ifndef DISPLAY_DEVICE_D3D9_D3D9_MEMORY_TRACKING_H
#define DISPLAY_DEVICE_D3D9_D3D9_MEMORY_TRACKING_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/D3D9/D3D9.h"

#if defined(PLATFORM_XENON) && !defined(CFG_GOLD)
#define USE_D3D_MEMORY_TRACKER
#endif

namespace Display
{
namespace D3D9
{

//! E_D3DMemoryTracker.

enum E_D3DMemoryTracker
{
	E_D3DMemoryTracker_Unknown = 0,
	E_D3DMemoryTracker_Texture2D,
	E_D3DMemoryTracker_Texture3D,
	E_D3DMemoryTracker_TextureCube,
	E_D3DMemoryTracker_VertexBuffer,
	E_D3DMemoryTracker_IndexBuffer,
	E_D3DMemoryTracker_VertexShader,
	E_D3DMemoryTracker_PixelShader,
	E_D3DMemoryTracker_Count,
};

//! C_D3DMemoryTracker.

class C_D3DMemoryTracker
{
public:

	C_D3DMemoryTracker();
	~C_D3DMemoryTracker();

	const U32 GetEnabledType();

	void Enable(const E_D3DMemoryTracker type);
	void Disable(const E_D3DMemoryTracker type);

	static C_D3DMemoryTracker& Instance() { return s_Instance; }

public:

	bool m_Enabled[E_D3DMemoryTracker_Count];

private:

	static C_D3DMemoryTracker s_Instance;
};

//! C_D3DMemoryStat.

class C_D3DMemoryStat
{
public:

	C_D3DMemoryStat(const E_D3DMemoryTracker type);
	~C_D3DMemoryStat();

public:

	E_D3DMemoryTracker m_Type;
};

}; // namespace D3D9
}; // namespace Display

#ifdef USE_D3D_MEMORY_TRACKER
#define D3D_MEMORY_STAT_TEXTURE_2D		Display::D3D9::C_D3DMemoryStat d3dMemoryStat(Display::D3D9::E_D3DMemoryTracker_Texture2D);
#define D3D_MEMORY_STAT_TEXTURE_3D		Display::D3D9::C_D3DMemoryStat d3dMemoryStat(Display::D3D9::E_D3DMemoryTracker_Texture3D);
#define D3D_MEMORY_STAT_TEXTURE_TCUBE	Display::D3D9::C_D3DMemoryStat d3dMemoryStat(Display::D3D9::E_D3DMemoryTracker_TextureCube);
#define D3D_MEMORY_STAT_TEXTURE_VB		Display::D3D9::C_D3DMemoryStat d3dMemoryStat(Display::D3D9::E_D3DMemoryTracker_VertexBuffer);
#define D3D_MEMORY_STAT_TEXTURE_IB		Display::D3D9::C_D3DMemoryStat d3dMemoryStat(Display::D3D9::E_D3DMemoryTracker_IndexBuffer);
#define D3D_MEMORY_STAT_TEXTURE_VS		Display::D3D9::C_D3DMemoryStat d3dMemoryStat(Display::D3D9::E_D3DMemoryTracker_VertexShader);
#define D3D_MEMORY_STAT_TEXTURE_PS		Display::D3D9::C_D3DMemoryStat d3dMemoryStat(Display::D3D9::E_D3DMemoryTracker_PixelShader);
#else
#define D3D_MEMORY_STAT_TEXTURE_2D
#define D3D_MEMORY_STAT_TEXTURE_3D
#define D3D_MEMORY_STAT_TEXTURE_TCUBE
#define D3D_MEMORY_STAT_TEXTURE_VB
#define D3D_MEMORY_STAT_TEXTURE_IB
#define D3D_MEMORY_STAT_TEXTURE_VS
#define D3D_MEMORY_STAT_TEXTURE_PS
#endif

#endif // DISPLAY_DEVICE_D3D9_D3D9_MEMORY_TRACKING_H
