#include "stdafx.h"
#include "Display/Device/D3D9/D3D9DeviceParams.h"

namespace Display
{
namespace D3D9
{

C_DeviceParams::C_DeviceParams()
{
	m_pd3dDevice						= NULL;
	m_pd3dObject						= NULL;

	#ifdef PLATFORM_XENON
	m_HiDef								= true;
	m_WideScreen						= true;
	m_BackBufferWidth					= 1280;
	m_BackBufferHeight					= 720;
	m_BackBufferColourFormat			= E_Format_A2B10G10R10F;
	m_BackBufferDepthStencilFormat		= E_Format_D24FS8;
	m_BackBufferMultisample				= E_Multisample_None;
	m_FrontBufferWidth					= 1280;
	m_FrontBufferHeight					= 720;
	m_FrontBufferFormat					= E_Format_A8R8G8B8;
	m_WindowBufferWidth					= 1280;
	m_WindowBufferHeight				= 720;
	m_WindowBufferColourFormat			= E_Format_A2B10G10R10F;
	m_WindowBufferDepthStencilFormat	= E_Format_D24FS8;
	m_WindowBufferMultisample			= E_Multisample_4;
	m_LargeHeapSize						= 1024*1024*280;
	m_DynamicHeapSize					= 1024*1024*5;
	m_SmallHeapSize						= 1024*1024*5;
	m_DeviceMemorySize					= 1024*1024*21;
	m_RingBufferPrimarySize				= 1024*128; // Default is 1024*32
	m_RingBufferSecondarySize			= 1024*1024*8; // Default is 1024*1024*2
	m_FreeBeginVerticesSize				= 1024*256;
	#endif

	#ifdef PLATFORM_PC
	m_Width								= 640;
	m_Height							= 480;
	m_Multisample						= E_Multisample_2;
	m_ColourFormat						= E_Format_A8R8G8B8;
	m_DepthStencilFormat				= E_Format_D24S8;
	m_Windowed							= true;
	m_CmdShow							= 1;
	m_pName								= NULL;
	m_hDC								= NULL;
	m_hWnd								= NULL;
	m_hInstance							= NULL;
	m_pd3dObject						= NULL;
	m_UseREF							= false;
#ifdef TOOL
#pragma message("Hack: Increased vertex buffer size to 6mb from 2mb")
	m_BeginVerticesSize					= (1024*256)+(1024*1024*6);
#else
	m_BeginVerticesSize					= (1024*256)+(1024*1024*2);
#endif
	m_FreeBeginVerticesSize				= (1024*256);
	m_PresentInterval					= E_PresentInterval_One;
	#endif
}

C_DeviceParams::~C_DeviceParams()
{
}

void C_DeviceParams::Setup(const U32 fbWidth, const U32 fbHeight, const U32 bbWidth, const U32 bbHeight, const E_Multisample bbMultisample, const U32 wbWidth, const U32 wbHeight, const E_Multisample wbMultisample)
{
	#ifdef PLATFORM_XENON
	m_FrontBufferWidth			= fbWidth;
	m_FrontBufferHeight			= fbHeight;

	m_BackBufferWidth			= bbWidth;
	m_BackBufferHeight			= bbHeight;
	m_BackBufferMultisample		= bbMultisample;

	m_WindowBufferWidth			= wbWidth;
	m_WindowBufferHeight		= wbHeight;
	m_WindowBufferMultisample	= wbMultisample;
	#endif
}

}; // namespace D3D9
}; // namespace Display
