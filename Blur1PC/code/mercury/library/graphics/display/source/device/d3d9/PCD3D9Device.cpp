#include "stdafx.h"
#include <system/debug/Blur2PCAutomation.h>	// [Blur2PC]
#include <d3dx9.h>	// [Blur2PC] D3DXSaveSurfaceToFile
#include "Display/Device/D3D9/PCD3D9Device.h"

//#define D3D_BUILD_OBJECT_CAPS
//#define D3D_BUILD_DEVICE_CAPS

#if defined(D3D_BUILD_OBJECT_CAPS) || defined(D3D_BUILD_DEVICE_CAPS)
#include "Display/Device/D3D9/PCD3D9CapsBuilder.h"
#endif

//#define D3D_CHECK_OBJECT_CAPS
//#define D3D_CHECK_DEVICE_CAPS

#if defined(D3D_CHECK_OBJECT_CAPS) || defined(D3D_CHECK_DEVICE_CAPS)
#include "Display/Device/D3D9/PCD3D9CapsChecker.h"
#endif

#ifndef CFG_GOLD
Debug::Int g_debugDisplayDeviceShaderProfile("Graphics.Display.Device.ShaderProfile", -1, Debug::Range(-1, 1));
#endif

#ifndef CFG_GOLD
//#define DEBUG_OUTPUT_LOST_DEVICE
#endif

namespace Display
{
namespace D3D9
{

C_PCD3D9Device::C_PCD3D9Device()
{
	m_Lost								= false;
	m_Valid								= false;
	m_BeginVerticesUsage				= 0;
	m_MaxBeginVerticesUsage				= 0;
	m_UserBeginVerticesUsage			= 0;
	m_UserMaxBeginVerticesUsage			= 0;
	m_BeginVerticesLocked				= false;
	#ifndef CFG_GOLD
	m_BeginVerticesSize					= 0;
	m_BeginVerticesStart				= NULL;
	#endif

	SetDepthFlip(true, false);
}

C_PCD3D9Device::~C_PCD3D9Device()
{
}

bool C_PCD3D9Device::CreateDevice()
{
	// [CharlieB] do this first since D3D might override our backbuffer width and height
	if(!CreateD3D9())
		return false;

	Display2::Device(&m_Display2);

	Display2::C_DeviceParams display2DeviceParams;

	display2DeviceParams.m_pd3dDevice		= m_pd3dDevice;
	display2DeviceParams.m_pd3dObject		= m_pd3dObject;

	m_Display2.SetParams(display2DeviceParams);

	if(!m_Display2.Create())
		return false;

	if(!m_Display2.CreateVolatile())
		return false;

	C_BackBuffer* pBackBuffer			= new C_BackBuffer();
	pBackBuffer->m_Width				= m_DeviceParams.m_Width;
	pBackBuffer->m_Height				= m_DeviceParams.m_Height;
	pBackBuffer->m_Multisample			= m_DeviceParams.m_Multisample;
	pBackBuffer->m_ColourFormat			= m_DeviceParams.m_ColourFormat;
	pBackBuffer->m_DepthStencilFormat	= m_DeviceParams.m_DepthStencilFormat;
	m_BackBuffers.PushBack(pBackBuffer);

	if(!C_D3D9Device::CreateDevice())
		return false;
	if(!CreateColourSurface())
		return false;
	if(!CreateDepthStencilSurface())
		return false;
	if(!m_DynamicVertexBuffer.Create(m_DeviceParams.m_BeginVerticesSize))
		return false;

	m_MaxBeginVerticesUsage = m_UserMaxBeginVerticesUsage = (m_DeviceParams.m_BeginVerticesSize-m_DeviceParams.m_FreeBeginVerticesSize);
	
	InvalidateState();
	DefaultStates();

	return true;
}

bool C_PCD3D9Device::CreateDeviceVolatile()
{
	if(!ResetD3D9())
		return false;

	if(!m_Display2.CreateVolatile())
		return false;

	if(m_BackBuffers.Size() > 0)
	{
		C_BackBuffer* pBackBuffer			= m_BackBuffers[0];
		pBackBuffer->m_Width				= m_DeviceParams.m_Width;
		pBackBuffer->m_Height				= m_DeviceParams.m_Height;
		pBackBuffer->m_Multisample			= m_DeviceParams.m_Multisample;
		pBackBuffer->m_ColourFormat			= m_DeviceParams.m_ColourFormat;
		pBackBuffer->m_DepthStencilFormat	= m_DeviceParams.m_DepthStencilFormat;
	}

	if(!C_D3D9Device::CreateDeviceVolatile())
		return false;
	if(!CreateColourSurface())
		return false;
	if(!CreateDepthStencilSurface())
		return false;
	if(!m_DynamicVertexBuffer.Create(m_DeviceParams.m_BeginVerticesSize))
		return false;

	m_MaxBeginVerticesUsage = m_UserMaxBeginVerticesUsage = (m_DeviceParams.m_BeginVerticesSize-m_DeviceParams.m_FreeBeginVerticesSize);
	
	InvalidateState();
	DefaultStates();

	return true;
}

void C_PCD3D9Device::DestroyDeviceVolatile()
{
	m_Display2.DestroyVolatile();

	m_DynamicVertexBuffer.Destroy();

	DestroyColourSurface();
	DestroyDepthStencilSurface();
	
	C_D3D9Device::DestroyDeviceVolatile();
}

void C_PCD3D9Device::DestroyDevice()
{
	m_DynamicVertexBuffer.Destroy();

	DestroyColourSurface();
	DestroyDepthStencilSurface();

	for (S32 i = 0; i < m_BackBuffers.Size(); i++)
		delete m_BackBuffers[i];
	m_BackBuffers.Clear();

	m_Display2.Destroy();

	Display2::Device(NULL);

	C_D3D9Device::DestroyDevice();
}

void C_PCD3D9Device::SetPresentParamsD3D9()
{
	ZeroMemory(&m_d3dPresentParams, sizeof(m_d3dPresentParams));

	m_d3dPresentParams.BackBufferWidth				= m_DeviceParams.m_Width;
	m_d3dPresentParams.BackBufferHeight				= m_DeviceParams.m_Height;
	m_d3dPresentParams.BackBufferFormat				= D3D9Format(m_DeviceParams.m_ColourFormat);
	m_d3dPresentParams.BackBufferCount				= 2;
	m_d3dPresentParams.MultiSampleType				= D3D9Multisample(m_DeviceParams.m_Multisample);
	m_d3dPresentParams.MultiSampleQuality			= 0;
	m_d3dPresentParams.SwapEffect					= D3DSWAPEFFECT_DISCARD;
	m_d3dPresentParams.hDeviceWindow				= m_DeviceParams.m_hWnd;
	m_d3dPresentParams.Windowed						= m_DeviceParams.m_Windowed ? TRUE : FALSE;
	m_d3dPresentParams.EnableAutoDepthStencil		= TRUE;
	m_d3dPresentParams.AutoDepthStencilFormat		= D3D9Format(m_DeviceParams.m_DepthStencilFormat);
	m_d3dPresentParams.Flags						= 0;
	// [Blur2PC] retail PC carries the chosen mode's refresh rate in the device params (set with the size by
	// the application, retail 0x0084a450). The code that builds retail's present parameters lives in the
	// DRM-relocated section and was not recovered, so using it here is inferred; see docs/analysis.md.
	m_d3dPresentParams.FullScreen_RefreshRateInHz	= m_DeviceParams.m_Windowed ? 0 : m_DeviceParams.m_RefreshRate;
	m_d3dPresentParams.PresentationInterval			= D3D9PresentInterval(m_DeviceParams.m_PresentInterval);

	#if !defined(CFG_GOLD) && defined(USER_LLOYDW)
	LOGINFO(g_Log, "*** Setting d3d9 present params:\n");
	LOGINFO(g_Log, "	BackBufferWidth %i\n",				m_d3dPresentParams.BackBufferWidth);
	LOGINFO(g_Log, "	BackBufferHeight %i\n",				m_d3dPresentParams.BackBufferHeight);
	LOGINFO(g_Log, "	BackBufferFormat %i\n",				m_d3dPresentParams.BackBufferFormat);
	LOGINFO(g_Log, "	BackBufferCount %i\n",				m_d3dPresentParams.BackBufferCount);
	LOGINFO(g_Log, "	MultiSampleType %i\n",				m_d3dPresentParams.MultiSampleType);
	LOGINFO(g_Log, "	MultiSampleQuality %i\n",			m_d3dPresentParams.MultiSampleQuality);
	LOGINFO(g_Log, "	SwapEffect %i\n",					m_d3dPresentParams.SwapEffect);
	LOGINFO(g_Log, "	hDeviceWindow %i\n",				m_d3dPresentParams.hDeviceWindow);
	LOGINFO(g_Log, "	Windowed %i\n",						m_d3dPresentParams.Windowed);
	LOGINFO(g_Log, "	EnableAutoDepthStencil %i\n",		m_d3dPresentParams.EnableAutoDepthStencil);
	LOGINFO(g_Log, "	AutoDepthStencilFormat %i\n",		m_d3dPresentParams.AutoDepthStencilFormat);
	LOGINFO(g_Log, "	Flags %i\n",						m_d3dPresentParams.Flags);
	LOGINFO(g_Log, "	FullScreen_RefreshRateInHz %i\n",	m_d3dPresentParams.FullScreen_RefreshRateInHz);
	LOGINFO(g_Log, "	PresentationInterval %i\n",			m_d3dPresentParams.PresentationInterval);
	#endif
}

bool C_PCD3D9Device::SetVertexShaderProfile()
{
	BIZ_ASSERT(m_pd3dDevice);
	const LPCSTR profile  = D3DXGetVertexShaderProfile(m_pd3dDevice);
	
	if(!profile)
	{
		LOGINFO(g_Log, "Device does not support vertex shaders\n");
		return false;
	}
	
	LOGINFO(g_Log, "*** D3DDeviceInfo : VertexShaderProfile %s\n", profile);

	if(strcmp(profile, "vs_1_1") == 0)
	{
		m_VertexShaderProfile = E_VertexShaderProfile_1_1;
	}
	else
	if(strcmp(profile, "vs_2_0") == 0)
	{
		m_VertexShaderProfile = E_VertexShaderProfile_2_0;
	}
	else
	if(strcmp(profile, "vs_2_a") == 0)
	{
		m_VertexShaderProfile = E_VertexShaderProfile_2_a;
	}
	else
	if(strcmp(profile, "vs_3_0") == 0)
	{
		m_VertexShaderProfile = E_VertexShaderProfile_3_0;
	}
	else
	{
		LOGERROR(g_Log, "Unsupported vertex shader model\n");
		return false;
	}

	return true;
}

bool C_PCD3D9Device::SetPixelShaderProfile()
{
	BIZ_ASSERT(m_pd3dDevice);
	const LPCSTR profile  = D3DXGetPixelShaderProfile(m_pd3dDevice);
	if(!profile)
	{
		LOGERROR(g_Log, "Device does not support pixel shaders\n");
		return false;
	}
	
	LOGINFO(g_Log, "*** D3DDeviceInfo : PixelShaderProfile %s\n", profile);

	if(strcmp(profile, "ps_1_1") == 0)
	{
		m_PixelShaderProfile = E_PixelShaderProfile_1_1;
	}
	else
	if(strcmp(profile, "ps_1_2") == 0)
	{
		m_PixelShaderProfile = E_PixelShaderProfile_1_2;
	}
	else
	if(strcmp(profile, "ps_1_3") == 0)
	{
		m_PixelShaderProfile = E_PixelShaderProfile_1_3;
	}
	else
	if(strcmp(profile, "ps_1_4") == 0)
	{
		m_PixelShaderProfile = E_PixelShaderProfile_1_4;
	}
	else
	if(strcmp(profile, "ps_2_0") == 0)
	{
		m_PixelShaderProfile = E_PixelShaderProfile_2_0;
	}
	else
	if(strcmp(profile, "ps_2_a") == 0)
	{
		m_PixelShaderProfile = E_PixelShaderProfile_2_a;
	}
	else
	if(strcmp(profile, "ps_2_b") == 0)
	{
		m_PixelShaderProfile = E_PixelShaderProfile_2_b;
	}
	else
	if(strcmp(profile, "ps_3_0") == 0)
	{
		m_PixelShaderProfile = E_PixelShaderProfile_3_0;
	}
	else
	{
		LOGERROR(g_Log, "Unsupported pixel shader model\n");
		return false;
	}

	#ifndef CFG_GOLD
	if(g_debugDisplayDeviceShaderProfile != -1)
	{
		m_ShaderProfile = (E_ShaderProfile)((S32)g_debugDisplayDeviceShaderProfile);
	}
	else
	#endif
	{
		if(m_PixelShaderProfile < E_PixelShaderProfile_3_0)
		{
			m_ShaderProfile = E_ShaderProfile_2_0;
		}
		else
		{
			m_ShaderProfile = E_ShaderProfile_3_0;
		}
	}
	
	return true;
}

bool C_PCD3D9Device::CreateD3D9()
{
	//U32 threadID = GetCurrentThreadId();
	//LOGINFO(g_Log, "*** D3D was created on thread %i\n", threadID);

	BIZ_ASSERT(m_DeviceParams.m_pd3dObject);
	m_pd3dObject = m_DeviceParams.m_pd3dObject;
	
	SetPresentParamsD3D9();

	const D3DDEVTYPE d3dDevType = m_DeviceParams.m_UseREF ? D3DDEVTYPE_REF : D3DDEVTYPE_HAL;
	
	HRESULT hr = m_pd3dObject->GetDeviceCaps(
		D3DADAPTER_DEFAULT,
		d3dDevType,
		&C_DeviceCaps::m_d3dCaps);
	
	if(FAILED(hr))
	{
		LOGERROR(g_Log, "Failed to get the d3d9 object caps");
		return false;
	}

	#ifdef D3D_BUILD_OBJECT_CAPS
	if(!D3DBuildCaps(C_DeviceCaps::m_d3dCaps, "Object"))
	{
		LOGERROR(g_Log, "D3D object caps building failed\n");
	}
	#endif

	#ifdef D3D_CHECK_OBJECT_CAPS
	if(!D3DCheckObjectCaps(C_DeviceCaps::m_d3dCaps))
	{
		LOGERROR(g_Log, "D3D object caps checking failed\n");
		//C_ManagerBase::PrintError(E_Error_NotSupported_CAPS);
		//return false;
	}
	#endif

	BIZ_ASSERT(!m_pd3dDevice);
	
	DWORD behaviourFlags = D3DCREATE_MULTITHREADED;
	if(m_DeviceParams.m_UseREF)
	{
		behaviourFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	}
	else
	{
		behaviourFlags |= C_DeviceCaps::VertexProcessing();
	}

	hr = m_DeviceParams.m_pd3dObject->CreateDevice(
		D3DADAPTER_DEFAULT, 
		d3dDevType, 
		m_DeviceParams.m_hWnd,
		behaviourFlags, 
		&m_d3dPresentParams,
		&m_pd3dDevice);

	if(FAILED(hr))
	{
		LOGFATAL(g_Log, "Failed to create d3d9 device\n");
		return false;
	}

	LOGINFO(g_Log, "*** D3DDeviceInfo : AvailableTextureMem %i\n", m_pd3dDevice->GetAvailableTextureMem());

	hr = m_pd3dDevice->GetDeviceCaps(&C_DeviceCaps::m_d3dCaps);
	if(FAILED(hr))
	{
		LOGFATAL(g_Log, "Failed to get the d3d9 device caps\n");
		return false;
	}

	#ifdef D3D_BUILD_DEVICE_CAPS
	if(!D3DBuildCaps(C_DeviceCaps::m_d3dCaps, "Device"))
	{
		LOGERROR(g_Log, "D3D device caps building failed\n");
	}
	#endif

	#ifdef D3D_CHECK_DEVICE_CAPS
	if(!D3DCheckDeviceCaps(C_DeviceCaps::m_d3dCaps))
	{
		LOGERROR(g_Log, "D3D device caps checking failed\n");
		//C_ManagerBase::PrintError(E_Error_NotSupported_CAPS);
		//return false;
	}
	#endif

	if(!SetVertexShaderProfile())
	{
		LOGFATAL(g_Log, "Failed to set the vertex shader profile\n");
		return false;
	}

	if(!SetPixelShaderProfile())
	{
		LOGFATAL(g_Log, "Failed to set the pixel shader profile\n");
		return false;
	}

	m_DeviceParams.m_Width = m_d3dPresentParams.BackBufferWidth;
	m_DeviceParams.m_Height = m_d3dPresentParams.BackBufferHeight;

	m_Valid = true;

	return true;
}

bool C_PCD3D9Device::ResetD3D9()
{
	BIZ_ASSERT(m_pd3dDevice);

	//U32 threadID = GetCurrentThreadId();
	//LOGINFO(g_Log, "*** D3D was reset on thread %i\n", threadID);

	HRESULT hr = m_pd3dDevice->TestCooperativeLevel();
	switch(hr)
	{
	case D3D_OK:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::ResetD3D9::TestCooperativeLevel - D3D_OK\n"); 
			#endif

			SetValid(true);
		}
		break;
	case D3DERR_DEVICELOST:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::ResetD3D9::TestCooperativeLevel - D3DERR_DEVICELOST\n"); 
			#endif
			
			SetLost(true);
			SetValid(false);
		}
		return false;
	case D3DERR_DEVICENOTRESET:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::ResetD3D9::TestCooperativeLevel - D3DERR_DEVICENOTRESET\n"); 
			#endif
			
			SetLost(true);
			SetValid(false);
		}
		break;
	case D3DERR_DRIVERINTERNALERROR:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::ResetD3D9::TestCooperativeLevel - D3DERR_DRIVERINTERNALERROR\n"); 
			#endif

			SetValid(false);
			BIZ_ASSERT_FAIL("D3DERR_DRIVERINTERNALERROR");
		}
		return false;
	default:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::ResetD3D9::TestCooperativeLevel\n"); 
			#endif

			SetValid(false);
			BIZ_ASSERT(0);
		}
		return false;
	}

	SetPresentParamsD3D9();

	D3DDEVICE_CREATION_PARAMETERS creationParameters;
	m_pd3dDevice->GetCreationParameters(&creationParameters);
	if(creationParameters.DeviceType == D3DDEVTYPE_REF)
	{
		//Multisampling is not supported by the ref device so let's make sure it's disabled.
		m_d3dPresentParams.MultiSampleType = D3DMULTISAMPLE_NONE;
	}

	#ifndef CFG_GOLD
	CheckUnManagedUserCreatedQueriesAreFreed();
	#endif
	
	hr = m_pd3dDevice->Reset(&m_d3dPresentParams);
	switch(hr)
	{
	case D3D_OK:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::ResetD3D9::Reset - D3D_OK\n"); 
			#endif

			SetLost(false);
			SetValid(true);
		}
		return true;
	case D3DERR_DEVICELOST:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::ResetD3D9::Reset - D3DERR_DEVICELOST\n"); 
			#endif

			SetLost(true);
			SetValid(false);
		}
		return false;
	case D3DERR_DRIVERINTERNALERROR:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::ResetD3D9::Reset - D3DERR_DRIVERINTERNALERROR\n"); 
			#endif

			SetValid(false);
			BIZ_ASSERT_FAIL("D3DERR_DRIVERINTERNALERROR");
		}
		return false;
	case D3DERR_OUTOFVIDEOMEMORY:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::ResetD3D9::Reset - D3DERR_OUTOFVIDEOMEMORY\n"); 
			#endif

			SetValid(false);
			BIZ_ASSERT_FAIL("D3DERR_OUTOFVIDEOMEMORY");
		}
		return false;
	default:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::ResetD3D9::Reset\n"); 
			#endif

			SetValid(false);
			
			#ifndef TOOL
			BIZ_ASSERT(0);
			#endif
		}
		return false;
	}
}

bool C_PCD3D9Device::Present()
{
	BIZ_ASSERT(m_pd3dDevice);
	// [Blur2PC] scripted screenshot (system/debug/Blur2PCAutomation.h): resolve the (possibly
	// multisampled) back buffer into a plain render target and save it as PNG.
	if (const char* pShotPath = Blur2PCAutomation::TakeShotRequest())
	{
		IDirect3DSurface9* pBack = NULL;
		if (SUCCEEDED(m_pd3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBack)))
		{
			D3DSURFACE_DESC desc;
			pBack->GetDesc(&desc);
			IDirect3DSurface9* pResolved = NULL;
			if (SUCCEEDED(m_pd3dDevice->CreateRenderTarget(desc.Width, desc.Height, desc.Format, D3DMULTISAMPLE_NONE, 0, FALSE, &pResolved, NULL)))
			{
				if (SUCCEEDED(m_pd3dDevice->StretchRect(pBack, NULL, pResolved, NULL, D3DTEXF_NONE)))
					D3DXSaveSurfaceToFileA(pShotPath, D3DXIFF_PNG, pResolved, NULL, NULL);
				pResolved->Release();
			}
			pBack->Release();
		}
	}
	const HRESULT hr = m_pd3dDevice->Present(NULL, NULL, NULL, NULL);
	// [Blur2PC] log every non-OK present (bring-up diagnostics)
	{
		static U32 s_presentFailures = 0;
		if (hr != D3D_OK && s_presentFailures++ < 20)
			LOGERROR(g_Log, "Present failed hr 0x%08x\n", (U32)hr);
	}
	switch(hr)
	{
	case D3D_OK:
		{
			SetValid(true);
		}
		break;
	case D3DERR_DEVICELOST:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::ResetD3D9::Present - D3DERR_DEVICELOST\n"); 
			#endif
			
			SetLost(true);
			SetValid(false);
		}
		break;
	case D3DERR_DRIVERINTERNALERROR:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::ResetD3D9::Present - D3DERR_DRIVERINTERNALERROR\n"); 
			#endif
			
			SetLost(true);
			SetValid(false);
		}
		break;
	case D3DERR_INVALIDCALL:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::ResetD3D9::Present - D3DERR_INVALIDCALL\n"); 
			#endif
			
			SetValid(false);
			BIZ_ASSERT_FAIL("D3DERR_INVALIDCALL");
		}
		break;
	default:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::ResetD3D9::Present\n"); 
			#endif

			SetValid(false);

			#ifndef TOOL
			BIZ_ASSERT(0);
			#endif
		}
		break;
	}

	// Swap to the new dynamic vertex buffer
	m_DynamicVertexBuffer.Swap();

	// Reset begin vertices usage
	m_BeginVerticesUsage = m_UserBeginVerticesUsage = 0;

	// Make sure eveything is unbound
	Unbind();

	return !m_Lost;
}

void* C_PCD3D9Device::BeginVertices(const E_PrimitiveType primitiveType, const U32 vertexCount, const U32 vertexStride, C_VertexDeclaration* pVertexDeclaration, const bool checkUsage)
{
	BIZ_ASSERT(!m_BeginVerticesLocked);

	U32 size = vertexCount * vertexStride;

	if(checkUsage)
	{
		if((m_UserBeginVerticesUsage+size) > m_UserMaxBeginVerticesUsage)
		{
			return NULL;
		}

		if((m_BeginVerticesUsage+size) > m_MaxBeginVerticesUsage)
		{
			return NULL;
		}
	}
	
	void* pVertices = m_DynamicVertexBuffer.Begin(primitiveType, vertexCount, vertexStride, pVertexDeclaration);
	
	if(pVertices)
	{
		m_BeginVerticesLocked = true;
		m_BeginVerticesUsage += size;
		m_UserBeginVerticesUsage += size;

		#ifndef CFG_GOLD
		m_BeginVerticesSize = vertexCount * vertexStride;
		m_BeginVerticesStart = (U8*)pVertices;
		#endif

		return pVertices;
	}
	else
	{
		LOGFATAL(g_Log, "Begin vertices returned null! Failed to render count '%i' stride '%i' size '%i' primitive '%i'\n", vertexCount, vertexStride, vertexCount * vertexStride, primitiveType);

		return NULL;
	}
}

void C_PCD3D9Device::EndVertices(void* pVertex)
{
	BIZ_ASSERT(m_BeginVerticesLocked);

	m_DynamicVertexBuffer.End();
	
	SetVertexBuffer(0, m_DynamicVertexBuffer.GetVertexBuffer(), m_DynamicVertexBuffer.GetOffset() - m_DynamicVertexBuffer.GetVertexSize(), m_DynamicVertexBuffer.GetVertexStride());
	SetVertexDeclaration(m_DynamicVertexBuffer.GetVertexDeclaration());
	
	DrawPrimitive(m_DynamicVertexBuffer.GetPrimitiveType(), 0, m_DynamicVertexBuffer.GetVertexCount());

	m_BeginVerticesLocked = false;

	#ifndef CFG_GOLD
	if(pVertex)
	{
		U32 usedEnd = (U32)pVertex;
		U32 actualEnd = (U32)m_BeginVerticesStart+m_BeginVerticesSize;
		if(usedEnd > actualEnd)
		{
			LOGFATAL(g_Log, "C_PCD3D9Device::EndVertices - usedEnd > actualEnd by %i\n", usedEnd-actualEnd);
		}
		else
		if(usedEnd < actualEnd)
		{
			LOGWARN(g_Log, "C_PCD3D9Device::EndVertices - usedEnd < actualEnd by %i\n", actualEnd-usedEnd);
		}
	}
	m_BeginVerticesSize = 0;
	m_BeginVerticesStart = NULL;
	#endif
}

void C_PCD3D9Device::FakeEndVertices(void* pVertex)
{
	BIZ_ASSERT(m_BeginVerticesLocked);

	m_DynamicVertexBuffer.End();
	
	SetVertexBuffer(0, m_DynamicVertexBuffer.GetVertexBuffer(), m_DynamicVertexBuffer.GetOffset() - m_DynamicVertexBuffer.GetVertexSize(), m_DynamicVertexBuffer.GetVertexStride());
	SetVertexDeclaration(m_DynamicVertexBuffer.GetVertexDeclaration());
	
	m_BeginVerticesLocked = false;

	#ifndef CFG_GOLD
	if(pVertex)
	{
		U32 usedEnd = (U32)pVertex;
		U32 actualEnd = (U32)m_BeginVerticesStart+m_BeginVerticesSize;
		if(usedEnd > actualEnd)
		{
			LOGFATAL(g_Log, "C_PCD3D9Device::EndVertices - usedEnd > actualEnd by %i\n", usedEnd-actualEnd);
		}
		else
		if(usedEnd < actualEnd)
		{
			LOGWARN(g_Log, "C_PCD3D9Device::EndVertices - usedEnd < actualEnd by %i\n", actualEnd-usedEnd);
		}
	}
	m_BeginVerticesSize = 0;
	m_BeginVerticesStart = NULL;
	#endif
}

bool C_PCD3D9Device::CreateRenderTexture2D(const U32 width, const U32 height, const U32 levels, U32 flags, const E_Format format, const E_Multisample multisample, const C_SurfaceParams* pParams, C_RenderTexture2D* pRenderTexture, U8* pAddress, const U32 addressSize)
{
	BIZ_ASSERT(width > 0);
	BIZ_ASSERT(height > 0);
	BIZ_ASSERT(levels > 0);
	BIZ_ASSERT(pRenderTexture);
	BIZ_ASSERT(!pRenderTexture->IsCreated());

	if(!(flags & Display2::E_Flags_Colour) || !(flags & Display2::E_Flags_DepthStencil))
		flags |= Display2::E_Flags_Colour;

	flags |= E_Usage_Video;
	
	pRenderTexture->m_Flags = flags;

	const U32 buffers = flags & E_Usage_ReadWrite ? 2 : 1;
	for(U32 buffer=0; buffer<buffers; buffer++)
	{
		C_Texture& texture = pRenderTexture->m_Texture[buffer];

		if(!CreateTexture2D(width, height, levels, flags, format, &texture, pAddress, addressSize))
			return false;

		BIZ_ASSERT(pRenderTexture->m_Surfaces[buffer].Size() == 0);
		pRenderTexture->m_Surfaces[buffer].Resize(levels);
		
		for(U32 level=0; level<levels; level++)
		{
			C_Surface& surface = pRenderTexture->m_Surfaces[buffer][level];
			
			const HRESULT hr = texture.GetGPUTexture2D()->GetSurfaceLevel(level, &surface.m_pd3dSurface);
			BIZ_ASSERT(SUCCEEDED(hr));
		}
	}

	pRenderTexture->m_TextureIndex = buffers == 2 ? 1 : 0;
	pRenderTexture->m_SurfaceIndex = 0;

	#if TRACK_DISPLAY2_RESOURCES
	if(!(flags & Display2::E_Flags_DoNotTrack))
		m_Display2.InsertRenderTexture2D(pRenderTexture);
	#endif
	
	return true;
}

void C_PCD3D9Device::DestroyRenderTexture2D(C_RenderTexture2D* pRenderTexture)
{
	BIZ_ASSERT(pRenderTexture);

	const U32 buffers = pRenderTexture->m_Flags & E_Usage_ReadWrite ? 2 : 1;
	for(U32 buffer=0; buffer<buffers; buffer++)
	{
		for(S32 level=0; level<pRenderTexture->m_Surfaces[buffer].Size(); level++)
		{
			C_Surface& surface = pRenderTexture->m_Surfaces[buffer][level];

			if(surface.m_pd3dSurface)
			{
				const ULONG refCount = surface.m_pd3dSurface->Release();
				//BIZ_ASSERT(refCount == 0);
				surface.m_pd3dSurface = NULL;
			}
		}

		pRenderTexture->m_Surfaces[buffer].Clear();

		DestroyTexture2D(&pRenderTexture->m_Texture[buffer]);
	}

	pRenderTexture->m_TextureIndex = 0;
	pRenderTexture->m_SurfaceIndex = 0;

	#if TRACK_DISPLAY2_RESOURCES
	if(!(pRenderTexture->m_Flags & Display2::E_Flags_DoNotTrack))
		m_Display2.RemoveRenderTexture2D(pRenderTexture);
	#endif

	pRenderTexture->m_Flags = 0;
}

bool C_PCD3D9Device::AllocMemory(const E_ResourceType type, const U32 size, U8** ppAddress, const S32 systemID)
{
	return true;
}

void C_PCD3D9Device::FreeMemory(const E_ResourceType type, const U32 size, U8* pAddress, const S32 systemID)
{
}

bool C_PCD3D9Device::CanReset()
{
	BIZ_ASSERT(m_pd3dDevice);
	const HRESULT hr = m_pd3dDevice->TestCooperativeLevel();
	switch(hr)
	{
	case D3D_OK:
		{
			SetValid(true);
		}
		return true;
	case D3DERR_DEVICELOST:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::CanReset::TestCooperativeLevel - D3DERR_DEVICELOST\n"); 
			#endif
			
			SetLost(true);
			SetValid(false);
		}
		return false;
	case D3DERR_DEVICENOTRESET:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::CanReset::TestCooperativeLevel - D3DERR_DEVICENOTRESET\n"); 
			#endif
			
			SetLost(true);
			SetValid(false);
		}
		return true;
	case D3DERR_DRIVERINTERNALERROR:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::CanReset::TestCooperativeLevel - D3DERR_DRIVERINTERNALERROR\n"); 
			#endif
			
			SetValid(false);
			BIZ_ASSERT_FAIL("D3DERR_DRIVERINTERNALERROR");
		}
		return false;
	default:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::CanReset::TestCooperativeLevel\n"); 
			#endif
			
			SetValid(false);
			BIZ_ASSERT(0);
		}
		return false;
	}
}

bool C_PCD3D9Device::CanRender()
{
	BIZ_ASSERT(m_pd3dDevice);
	const HRESULT hr = m_pd3dDevice->TestCooperativeLevel();
	switch(hr)
	{
	case D3D_OK:
		{
			SetValid(true);
		}
		return true;
	case D3DERR_DEVICELOST:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::CanRender::TestCooperativeLevel - D3DERR_DEVICELOST\n"); 
			#endif

			SetLost(true);
			SetValid(false);
		}
		return false;
	case D3DERR_DEVICENOTRESET:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::CanRender::TestCooperativeLevel - D3DERR_DEVICENOTRESET\n"); 
			#endif

			SetLost(true);
			SetValid(false);
		}
		return false;
	case D3DERR_DRIVERINTERNALERROR:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::CanRender::TestCooperativeLevel - D3DERR_DRIVERINTERNALERROR\n"); 
			#endif

			SetValid(false);
			BIZ_ASSERT_FAIL("D3DERR_DRIVERINTERNALERROR");
		}
		return false;
	default:
		{
			#ifdef DEBUG_OUTPUT_LOST_DEVICE
			OutputDebugString("C_PCD3D9Device::CanRender::TestCooperativeLevel\n"); 
			#endif

			SetValid(false);
			BIZ_ASSERT(0);
		}
		return false;
	}
}

void C_PCD3D9Device::CheckUnManagedUserCreatedQueriesAreFreed()
{
}

bool C_PCD3D9Device::SupportForRenderTarget(const E_ResourceType resourceType, const E_Format format)
{
	D3DRESOURCETYPE d3dResourceType;
	switch(resourceType)
	{
	case E_ResourceType_Texture2D:
		d3dResourceType = D3DRTYPE_TEXTURE;
		break;
	case E_ResourceType_Texture3D:
		d3dResourceType = D3DRTYPE_VOLUMETEXTURE;
		break;
	case E_ResourceType_TextureCube:
		d3dResourceType = D3DRTYPE_CUBETEXTURE;
		break;
	default:
		return false;
	}

	BIZ_ASSERT(m_pd3dObject);
	const HRESULT hr = m_pd3dObject->CheckDeviceFormat(
		C_DeviceCaps::m_d3dCaps.AdapterOrdinal,
		C_DeviceCaps::m_d3dCaps.DeviceType,
		D3DFMT_X8R8G8B8,
		D3DUSAGE_RENDERTARGET, 
		d3dResourceType,
		D3D9Format(format));
	if(SUCCEEDED(hr))
	{
		return true;
	}
	else
	{
		return false;
	}
}

}; // namespace D3D9
}; // namespace Display
