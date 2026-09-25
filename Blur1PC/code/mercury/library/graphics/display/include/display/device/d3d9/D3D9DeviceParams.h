#ifndef DISPLAY_DEVICE_D3D9_D3D9_DEVICE_PARAMS_H
#define DISPLAY_DEVICE_D3D9_D3D9_DEVICE_PARAMS_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/DeviceTypes.h"

namespace Display
{
namespace D3D9
{

class C_DeviceParams
{
public:

	C_DeviceParams();
	~C_DeviceParams();

	void Setup(const U32 fbWidth, const U32 fbHeight, const U32 bbWidth, const U32 bbHeight, const E_Multisample bbMultisample, const U32 wbWidth, const U32 wbHeight, const E_Multisample wbMultisample);

public:

	IDirect3DDevice9*	m_pd3dDevice;
	IDirect3D9*			m_pd3dObject;

	#ifdef PLATFORM_XENON
	bool				m_HiDef;
	bool				m_WideScreen;
	U32					m_BackBufferWidth;
	U32					m_BackBufferHeight;
	E_Format			m_BackBufferColourFormat;
	E_Format			m_BackBufferDepthStencilFormat;
	E_Multisample		m_BackBufferMultisample;
	U32					m_FrontBufferWidth;
	U32					m_FrontBufferHeight;
	E_Format			m_FrontBufferFormat;
	U32					m_WindowBufferWidth;
	U32					m_WindowBufferHeight;
	E_Format			m_WindowBufferColourFormat;
	E_Format			m_WindowBufferDepthStencilFormat;
	E_Multisample		m_WindowBufferMultisample;
	U32					m_ScreenWidth;					// Width/Height of the actual display setting
	U32					m_ScreenHeight;
	U32					m_LargeHeapSize;
	U32					m_DynamicHeapSize;
	U32					m_SmallHeapSize;
	U32					m_DeviceMemorySize;
	U32					m_RingBufferPrimarySize;
	U32					m_RingBufferSecondarySize;
	U32					m_FreeBeginVerticesSize;
	#endif

	#ifdef PLATFORM_PC
	U32					m_Width;
	U32					m_Height;
	E_Multisample		m_Multisample;
	E_Format			m_ColourFormat;
	E_Format			m_DepthStencilFormat;
	U32					m_RefreshRate;		// [Blur2PC] retail PC: the chosen mode's refresh rate (0 = default)
	bool				m_Windowed;
	S8					m_CmdShow;
	LPCTSTR				m_pName;
	HDC					m_hDC;
	HWND				m_hWnd;
	HINSTANCE			m_hInstance;
	bool				m_UseREF;
	U32					m_BeginVerticesSize;
	U32					m_FreeBeginVerticesSize;
	E_PresentInterval	m_PresentInterval;
	#endif
};

}; // namespace D3D9
}; // namespace Display

#endif // DISPLAY_DEVICE_D3D9_D3D9_DEVICE_PARAMS_H
