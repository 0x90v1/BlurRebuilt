#include "stdafx.h"
#include "Display/Manager/D3D9/D3D9Manager.h"
#include "Display/Device/D3D9/D3D9DeviceCaps.h"
#include <algorithm>

#ifdef CFG_ALLOW_OPTIMIZE_OFF
//#pragma optimize( "", off ) // CFG_ALLOW_OPTIMIZE_OFF
#endif

#if !defined(USER_LLOYDW)
#define USE_GAMES_FOR_WINDOWS_LIVE_SUPPORTED_RESOLUTIONS
#endif

namespace Display
{
namespace D3D9
{

IDirect3D9*	C_D3D9Manager::s_pd3dObject = NULL;

C_D3D9Manager::C_D3D9Manager()
{
}

C_D3D9Manager::~C_D3D9Manager()
{
	m_MultisampleList.Destroy();
	m_VideoModeList.Destroy();

	assert(!s_pd3dObject && "The d3d object has not been destroyed");
}

IDirect3D9* C_D3D9Manager::GetD3DObject()
{
	return s_pd3dObject;
}

bool C_D3D9Manager::IsInitialised()
{
	return s_pd3dObject != NULL;
}

bool C_D3D9Manager::Initialise()
{
	if(!s_pd3dObject)
	{
		s_pd3dObject = Direct3DCreate9(D3D_SDK_VERSION);
		if(!s_pd3dObject)
		{
			LOGERROR(g_Log, "Failed to create d3d\n");
			PrintError(E_Error_NotSupported_D3D9);
			return false;
		}

		#if defined(PLATFORM_PC)
		{
			#ifndef CFG_GOLD
			D3DADAPTER_IDENTIFIER9 d3dAdapterInfo;
			s_pd3dObject->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &d3dAdapterInfo);
			LOGINFO(g_Log, "*** D3DAdapterInfo : Drive %s\n",					d3dAdapterInfo.Driver);
			LOGINFO(g_Log, "*** D3DAdapterInfo : Description %s\n",				d3dAdapterInfo.Description);
			LOGINFO(g_Log, "*** D3DAdapterInfo : DeviceName %s\n",				d3dAdapterInfo.DeviceName);
			LOGINFO(g_Log, "*** D3DAdapterInfo : DriverVersion is %i\n",		d3dAdapterInfo.DriverVersion);
			LOGINFO(g_Log, "*** D3DAdapterInfo : VendorId is %x\n",				d3dAdapterInfo.VendorId);
			LOGINFO(g_Log, "*** D3DAdapterInfo : DeviceId is %x\n",				d3dAdapterInfo.DeviceId);
			LOGINFO(g_Log, "*** D3DAdapterInfo : SubSysId is %x\n",				d3dAdapterInfo.SubSysId);
			LOGINFO(g_Log, "*** D3DAdapterInfo : Revision is %x\n",				d3dAdapterInfo.Revision);
			LOGINFO(g_Log, "*** D3DAdapterInfo : DeviceIdentifier is %i\n",		d3dAdapterInfo.DeviceIdentifier);
			LOGINFO(g_Log, "*** D3DAdapterInfo : WHQLLevel is %i\n",			d3dAdapterInfo.WHQLLevel);
			#endif

			const HRESULT hr = s_pd3dObject->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &C_DeviceCaps::m_d3dCaps);
			if(FAILED(hr))
			{
				LOGERROR(g_Log, "Failed to get the d3d object caps using HAL\n");
				PrintError(E_Error_NotSupported_HAL);
				return false;
			}
			
			if(C_DeviceCaps::m_d3dCaps.PixelShaderVersion < D3DPS_VERSION(2, 0))
			{
				LOGERROR(g_Log, "No support for d3d pixel shader model 2.0\n");
				PrintError(E_Error_NotSupported_PS20);
				return false;
			}

			if(C_DeviceCaps::m_d3dCaps.VertexShaderVersion < D3DVS_VERSION(2, 0))
			{
				#if 1
				LOGWARN(g_Log, "No support for d3d vertex shader model 2.0\n");
				#else
				LOGERROR(g_Log, "No support for d3d vertex shader model 2.0\n");
				PrintError(E_Error_NotSupported_VS20);
				return false;
				#endif
			}
		}
		#endif

		g_MainThreadID = GetCurrentThreadId(); // Get hold of the main thread id where d3d was created from
	}

#ifdef PLATFORM_PC
	{
		m_MultisampleList.Destroy();

		const E_Format colourFormat = E_Format_X8R8G8B8;
		const E_Format depthStencilFormat = E_Format_D24S8;

		E_Multisample multisamples[3] = 
		{
			E_Multisample_None,
			E_Multisample_2,
			E_Multisample_4,
		};

		for(U32 i=0; i<3; i++)
		{
			const E_Multisample multisample = multisamples[i];

			#ifdef CFG_GOLD
			bool windowedSupported = true;
			#else
			bool windowedSupported = false;
			if(CheckMultisample(multisample, colourFormat, true))
			{
				if(CheckMultisample(multisample, colourFormat, true))
				{
					windowedSupported = true;
				}
			}
			#endif

			bool fullScreenSupported = false;
			if(CheckMultisample(multisample, colourFormat, false))
			{
				if(CheckMultisample(multisample, depthStencilFormat, false))
				{
					fullScreenSupported = true;
				}
			}

			if(windowedSupported && fullScreenSupported)
			{
				C_Multisample item;

				item.m_Windowed				= windowedSupported;
				item.m_FullScreen			= fullScreenSupported;
				item.m_ColourFormat			= colourFormat;
				item.m_DepthStencilFormat	= depthStencilFormat;
				item.m_Multisample			= multisample;

				m_MultisampleList.PushBack(item);
			}

			#if !defined(CFG_GOLD)
			LOGINFO(g_Log, "*** Adding %i multisample %i fullscreen %i windowed %i\n",
				windowedSupported && fullScreenSupported,
				i,
				fullScreenSupported,
				windowedSupported);
			#endif
		}
	}
	{
		m_VideoModeList.Destroy();

		U32 adapterModeCount = s_pd3dObject->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);

		for(U32 adapterModeIndex=0; adapterModeIndex<adapterModeCount; adapterModeIndex++)
		{
			D3DDISPLAYMODE displayMode;

			s_pd3dObject->EnumAdapterModes(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, adapterModeIndex, &displayMode);

			#ifdef USE_GAMES_FOR_WINDOWS_LIVE_SUPPORTED_RESOLUTIONS
			if(displayMode.Width >= 800 && displayMode.Height >= 600)
			#endif
			{
				bool newVideoMode = AddVideoMode(displayMode.Width, displayMode.Height, E_Format_A8R8G8B8, false, false);

				#if !defined(CFG_GOLD)
				if(newVideoMode)
				{
					LOGINFO(g_Log, "*** Adding video mode width %i height %i\n",
						displayMode.Width,
						displayMode.Height);
				}
				#endif
			}
		}

		std::sort(m_VideoModeList.Begin(), m_VideoModeList.End());
	}
#endif

	return true;
}

void C_D3D9Manager::Shutdown()
{
	if(s_pd3dObject)
	{
		const ULONG refCount = s_pd3dObject->Release();
		if(refCount != 0)
		{
			LOGFATAL(g_Log, "Failed to release the d3d object %i\n", refCount);
		}
		s_pd3dObject = NULL;
	}
}

bool C_D3D9Manager::AddVideoMode(const U32 width, const U32 height, const E_Format format, const bool wideScreen, const bool hiDef)
{
	for(S32 i=0; i<m_VideoModeList.Size(); i++)
	{
		C_VideoMode& videoMode = m_VideoModeList[i];

		if(	width == videoMode.m_Width &&
			height == videoMode.m_Height &&
			format == videoMode.m_Format &&
			wideScreen == videoMode.m_WideScreen &&
			hiDef == videoMode.m_HiDef)
		{
			return false;
		}
	}

	C_VideoMode videoMode;
	videoMode.m_Width			= width;
	videoMode.m_Height			= height;
	videoMode.m_Format			= format;
	videoMode.m_WideScreen		= wideScreen;
	videoMode.m_HiDef			= hiDef;
	m_VideoModeList.PushBack(videoMode);

	return true;
}

const S32 C_D3D9Manager::FindMultisample(const E_Multisample multisample) const
{
	for(S32 i=0; i<m_MultisampleList.Size(); i++)
	{
		const C_Multisample& multisampleItem = m_MultisampleList[i];

		if(	multisample == multisampleItem.m_Multisample)
		{
			return i;
		}
	}

	return -1;
}

const S32 C_D3D9Manager::FindVideoMode(const U32 width, const U32 height, const E_Format format)
{
	for(S32 i=0; i<m_VideoModeList.Size(); i++)
	{
		C_VideoMode& videoMode = m_VideoModeList[i];

		if(	width == videoMode.m_Width &&
			height == videoMode.m_Height &&
			format == videoMode.m_Format)
		{
			return i;
		}
	}

	return -1;
}

bool C_D3D9Manager::GetVideoMode(C_VideoMode* pVideoMode)
{
	#if defined(PLATFORM_XENON)
	XVIDEO_MODE videoMode;
	XGetVideoMode(&videoMode);
	assert(pVideoMode);
	pVideoMode->m_Width			= videoMode.dwDisplayWidth;
	pVideoMode->m_Height		= videoMode.dwDisplayHeight;
	pVideoMode->m_Format		= E_Format_Unknown;
	pVideoMode->m_WideScreen	= videoMode.fIsWideScreen != FALSE;
	pVideoMode->m_HiDef			= videoMode.fIsHiDef != FALSE;
	#endif
	
	return true;
}

bool C_D3D9Manager::CheckMultisample(const E_Multisample multisample, const E_Format format, const bool windowed)
{
#ifdef PLATFORM_PC
	assert(s_pd3dObject);
	const HRESULT hr = s_pd3dObject->CheckDeviceMultiSampleType(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		D3D9Format(format),
		windowed ? TRUE : FALSE,
		D3D9Multisample(multisample),
		NULL);
	if(FAILED(hr))
		return false;
#endif

	return true;
}

}; // namespace D3D9
}; // namespace Display
