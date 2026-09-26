#include "stdafx.h"
#include "Display/Device/D3D9/XB2D3D9Device.h"
#include "Display/Core/Defines.h"
#include "Display/Memory/MemoryGraphics.h"
#include "Display/Memory/MemorySystem.h"
#include "Display/Render/Render.h"
#include "Display/Shader/VertexShaders.h"
#include "Display/Shader/PixelShaders.h"
#include "Display/DisplayToDisplay2.h"
#include <system/memory.h>
#include "display\memory/D3d9/D3D9RingBuffer.h"
#include "display\context/context_xenon.h"

#ifdef CFG_ALLOW_OPTIMIZE_OFF
//#pragma optimize( "", off ) // CFG_ALLOW_OPTIMIZE_OFF
#endif

#ifdef DO_NOT_INLINE_XB2_D3D9_DEVICE_INL
#undef INLINE
#define INLINE
#include "Display/Device/D3D9/XB2D3D9Device.inl"
#undef INLINE
#define INLINE inline
#endif

#ifndef CFG_GOLD
//#define DEVICE_INFO
#endif

Debug::Bool	g_debugDisplayResolutionUpdate		("Graphics.Display.Resolution.Update",			false);
Debug::Int	g_debugDisplayFilterProfile			("Graphics.Display.FilterProfile",				0, Debug::Range(0, 1));

#if !CFG_GOLD
Debug::Bool g_debugDisplayPrintFrameTimes		("Graphics.Display.PrintFrameTimes",			false);
#endif

void __cdecl D3DBeforePresentCallback(DWORD context)
{
	Display::D3D9::C_XB2D3D9Device* pDevice = (Display::D3D9::C_XB2D3D9Device*)context;
	pDevice->BeforePresentCallback();
}

void __cdecl D3DAfterPresentCallback(DWORD context)
{
	Display::D3D9::C_XB2D3D9Device* pDevice = (Display::D3D9::C_XB2D3D9Device*)context;
	pDevice->AfterPresentCallback();
}

namespace Display
{
namespace D3D9
{

#define USE_D3D_SURFACE_POOL
#define USE_D3D_TEXTURE_POOL
#define USE_D3D_CUBE_TEXTURE_POOL
#define USE_D3D_VOLUME_TEXTURE_POOL
#define USE_D3D_VERTEX_BUFFER_POOL
#define USE_D3D_INDEX_BUFFER_POOL

#ifdef USE_D3D_SURFACE_POOL
Ut::DynamicPool<IDirect3DSurface9, Ut::ArrayAllocator<U8>, Ut::ObjectLockable> g_d3dSurfacePool;
#endif
#ifdef USE_D3D_TEXTURE_POOL
Ut::DynamicPool<IDirect3DTexture9, Ut::ArrayAllocator<U8>, Ut::ObjectLockable> g_d3dTexturePool;
Debug::Bool g_debugShowTexturePoolInfo("Graphics.Display.Device.ShowTexturePoolInfo", false);
#endif
#ifdef USE_D3D_CUBE_TEXTURE_POOL
Ut::DynamicPool<IDirect3DCubeTexture9, Ut::ArrayAllocator<U8>, Ut::ObjectLockable> g_d3dCubeTexturePool;
#endif
#ifdef USE_D3D_VOLUME_TEXTURE_POOL
Ut::DynamicPool<IDirect3DVolumeTexture9, Ut::ArrayAllocator<U8>, Ut::ObjectLockable> g_d3dVolumeTexturePool;
#endif
#ifdef USE_D3D_VERTEX_BUFFER_POOL
Ut::DynamicPool<IDirect3DVertexBuffer9, Ut::ArrayAllocator<U8>, Ut::ObjectLockable> g_d3dVertexBufferPool;
#endif
#ifdef USE_D3D_INDEX_BUFFER_POOL
Ut::DynamicPool<IDirect3DIndexBuffer9, Ut::ArrayAllocator<U8>, Ut::ObjectLockable> g_d3dIndexBufferPool;
#endif

const D3DVECTOR4 D3D9GetColour(const U32 colour)
{
	const D3DVECTOR4 col = 
	{
		(F32)((colour & 0xFF000000) >> 24) / 255.f,
		(F32)((colour & 0x00FF0000) >> 16) / 255.f,
		(F32)((colour & 0x0000FF00) >> 8)  / 255.f,
		(F32)( colour & 0x000000FF)        / 255.f,
	};
	return col;
}

void CalculateTileSize(const C_TileSetup& tileSetup, U32& width, U32& height)
{
	U32 dwMaxWidth  = 0;
	U32 dwMaxHeight = 0;

    for(U32 i = 0; i < tileSetup.m_TileCount; ++i)
    {
        U32 dwWidth  = tileSetup.m_Tiles[i].x2 - tileSetup.m_Tiles[i].x1;
        U32 dwHeight = tileSetup.m_Tiles[i].y2 - tileSetup.m_Tiles[i].y1;

        if( dwWidth > dwMaxWidth )
            dwMaxWidth = dwWidth;
        if( dwHeight > dwMaxHeight )
            dwMaxHeight = dwHeight;
    }

	U32 dwTileWidth  = 0;
    U32 dwTileHeight = 0;

    switch(tileSetup.m_Multisample)
    {
        case E_Multisample_None:
            dwTileWidth = XGNextMultiple(dwMaxWidth, GPU_EDRAM_TILE_WIDTH_1X );
            dwTileHeight = XGNextMultiple(dwMaxHeight, GPU_EDRAM_TILE_HEIGHT_1X );
            break;
        case E_Multisample_2:
            dwTileWidth = XGNextMultiple(dwMaxWidth, GPU_EDRAM_TILE_WIDTH_2X );
            dwTileHeight = XGNextMultiple(dwMaxHeight, GPU_EDRAM_TILE_HEIGHT_2X );
            break;
        case E_Multisample_4:
            dwTileWidth = XGNextMultiple(dwMaxWidth, GPU_EDRAM_TILE_WIDTH_4X );
            dwTileHeight = XGNextMultiple(dwMaxHeight, GPU_EDRAM_TILE_HEIGHT_4X );
            break;
    }

    if(tileSetup.m_TileCount > 1)
    {
        dwTileWidth = XGNextMultiple(dwTileWidth, GPU_TEXTURE_TILE_DIMENSION);
        dwTileHeight = XGNextMultiple(dwTileHeight, GPU_TEXTURE_TILE_DIMENSION);
    }

	width = dwTileWidth;
	height = dwTileHeight;
}

void CalculateTileLayout(C_TileSetup& tileSetup)
{
	if(tileSetup.m_TileCount == 1)
		tileSetup.m_TileDirection = E_TileDirection_None;
	else
	{
		if(tileSetup.m_Tiles[0].y2 != tileSetup.m_Tiles[tileSetup.m_TileCount-1].y2)
			tileSetup.m_TileDirection = E_TileDirection_Horizontal;
		else
			tileSetup.m_TileDirection = E_TileDirection_Vertical;
	}

	F32 screenWidth = (F32)tileSetup.m_Width;
	F32 screenHeight = (F32)tileSetup.m_Height;
	
	for (DWORD i = 1; i < tileSetup.m_TileCount; ++i)
	{
		if(tileSetup.m_TileDirection == E_TileDirection_Horizontal)
			tileSetup.m_TileDivisions[i-1] = tileSetup.m_Tiles[i].y1 / screenHeight;
		else
			tileSetup.m_TileDivisions[i-1] = tileSetup.m_Tiles[i].x1 / screenWidth;
	}
}

C_XB2D3D9Device::C_XB2D3D9Device() :
	m_TilingEnable(true),
	m_TilingActive(false),
	m_BeginVerticesUsage(0),
	m_MaxBeginVerticesUsage(0),
	m_FreeBeginVerticesUsage(0),
	m_UserBeginVerticesUsage(0),
	m_UserMaxBeginVerticesUsage(0),
	m_BeginVerticesLocked(false)
{
	m_VertexShaderProfile	= E_VertexShaderProfile_3_0;
	m_PixelShaderProfile	= E_PixelShaderProfile_3_0;
	m_MemoryID				= -1;
	m_MemoryPtr				= NULL;
	m_MemorySize			= 0;
	m_MemoryFlags			= 0;
	m_WindowOffset			= 0;

	m_CpuTimeBegin			= 0;
	m_CpuTimeEnd			= 0;
	m_GpuTimeBegin[0]		= 0;
	m_GpuTimeEnd[0]			= 0;
	m_GpuTimeBegin[1]		= 0;
	m_GpuTimeEnd[1]			= 0;
	m_GpuTimeIndex			= 0;

	#ifndef CFG_GOLD
	m_BeginVerticesSize		= 0;
	m_BeginVerticesStart	= NULL;
	#endif
	
	SetDepthFlip(true, false);
}

C_XB2D3D9Device::~C_XB2D3D9Device()
{
}

bool C_XB2D3D9Device::CreateDevice()
{
	BIZ_ASSERT(m_DeviceParams.m_pd3dObject);
	m_pd3dObject = m_DeviceParams.m_pd3dObject;

	XGetVideoMode(&m_VideoMode);

	m_DeviceParams.m_ScreenWidth = m_VideoMode.dwDisplayWidth;
	m_DeviceParams.m_ScreenHeight = m_VideoMode.dwDisplayHeight;

	m_HiDef			= m_DeviceParams.m_HiDef;
	m_WideScreen	= m_DeviceParams.m_WideScreen;
	m_AspectRatio   = m_WideScreen ? E_AspectRatio_16_9 : E_AspectRatio_4_3;
	
	m_RefreshRate	= m_VideoMode.RefreshRate == 50.0f ? E_RefreshRate_50HZ : E_RefreshRate_60HZ;
	
	// Create the d3d device.

	if(m_DeviceParams.m_pd3dDevice)
	{
		m_InParallel = true;
		
		BIZ_ASSERT(m_DeviceParams.m_pd3dDevice);
		m_pd3dDevice = m_DeviceParams.m_pd3dDevice;
	}
	else
	{
		m_InParallel = false;
		
		if(!CreateD3D9())
		{
			return false;
		}
	}

	Display2::Device(&m_Display2);

	Display2::C_DeviceParams display2DeviceParams;

	display2DeviceParams.m_pd3dDevice		= m_pd3dDevice;
	display2DeviceParams.m_pd3dObject		= m_pd3dObject;
	display2DeviceParams.m_LargeHeapSize 	= m_DeviceParams.m_LargeHeapSize;
	display2DeviceParams.m_DynamicHeapSize 	= m_DeviceParams.m_DynamicHeapSize;
	display2DeviceParams.m_SmallHeapSize 	= m_DeviceParams.m_SmallHeapSize;

	m_Display2.SetParams(display2DeviceParams);

	if(!m_Display2.Create())
		return false;
	if(!m_Display2.CreateVolatile())
		return false;

	m_LargeHeap.SetDisplay2(m_Display2.GetLargeHeap());
	m_DynamicHeap.SetDisplay2(m_Display2.GetDynamicHeap());
	m_SmallHeap.SetDisplay2(m_Display2.GetSmallHeap());

	// Create the device.

	if(C_D3D9Device::CreateDevice())
	{
		// Is this device in parrllel with another one?

		if(!m_InParallel)
		{
			// Create the memory.
			
			if(!CreateMemory())
			{
				LOGFATAL(g_Log, "Failed to create the memory of size %i\n", m_DeviceParams.m_DeviceMemorySize);
				return false;
			}

			// Create the surfaces.
			
			if(!CreateSurfaces())
			{
				LOGFATAL(g_Log, "Failed to create the surfaces\n");
				return false;
			}

			// Create the windows.
			
			if(!CreateWindows())
			{
				LOGFATAL(g_Log, "Failed to create the windows\n");
				return false;
			}
		}
		else
		{
			// Create the colour surface.

			if(!C_D3D9Device::CreateColourSurface())
			{
				LOGFATAL(g_Log, "Failed to create the colour surface\n");
				return false;
			}

			// Create the depth stencil surface.
			
			if(!C_D3D9Device::CreateDepthStencilSurface())
			{
				LOGFATAL(g_Log, "Failed to create the depth stencil surface\n");
				return false;
			}
		}

		m_MaxBeginVerticesUsage = m_UserMaxBeginVerticesUsage = (m_DeviceParams.m_RingBufferSecondarySize/4);
		m_FreeBeginVerticesUsage = m_DeviceParams.m_FreeBeginVerticesSize;
		
		SetColourDepthStencilSurface();

		InvalidateState();
		
		return true;
	}
	else
	{
		return false;
	}
}

bool C_XB2D3D9Device::CreateDeviceVolatile()
{
	if(!m_Display2.CreateVolatile())
		return false;

	if(!C_D3D9Device::CreateDeviceVolatile())
		return false;

	return true;
}

void C_XB2D3D9Device::DestroyDeviceVolatile()
{
	m_Display2.DestroyVolatile();

	C_D3D9Device::DestroyDeviceVolatile();
}

void C_XB2D3D9Device::DestroyDevice()
{	
	Unbind();

	m_pd3dDevice->SetRenderTarget(0, NULL);
	m_pd3dDevice->SetDepthStencilSurface(NULL);

	m_pd3dDevice->UnsetAll();

	if(!m_InParallel)
	{
		ClearWindows();

		DestroySurfaces();

		DestroyMemory();
	}
	else
	{
		C_D3D9Device::DestroyColourSurface();
		C_D3D9Device::DestroyDepthStencilSurface();
	}

	m_Display2.Destroy();

	Display2::Device(NULL);

	C_D3D9Device::DestroyDevice();
}

bool C_XB2D3D9Device::CreateMemory()
{
	if(!m_MemoryPtr && m_DeviceParams.m_DeviceMemorySize > 0)
	{
		m_MemoryID = MemoryGetSystemID();
		m_MemorySize = m_DeviceParams.m_DeviceMemorySize;
		if(!AllocMemory(4096, m_MemorySize, &m_MemoryPtr, E_AllocHeap_Large, E_AllocType_Start, Display2::E_MemoryType_Device))
			return false;
	}
	
	return true;
}

void C_XB2D3D9Device::DestroyMemory()
{
	if(m_MemoryPtr)
	{
		FreeMemory(m_MemorySize, m_MemoryPtr, E_AllocHeap_Large, Display2::E_MemoryType_Device);
		m_MemoryPtr = NULL;
	}
}

void C_XB2D3D9Device::ResizeSurfaces()
{
	DestroySurfaces();
	CreateSurfaces();
}

bool C_XB2D3D9Device::CreateSurfaces()
{
	U32 maxSize = m_MemorySize;
	U8* curPtr = m_MemoryPtr;
	U32 curSize = 0;

	// Create the back buffer.
	{
		C_BackBuffer* pBackBuffer			= new C_BackBuffer();
		pBackBuffer->m_Width				= m_DeviceParams.m_BackBufferWidth;
		pBackBuffer->m_Height				= m_DeviceParams.m_BackBufferHeight;
		pBackBuffer->m_ColourFormat			= m_DeviceParams.m_BackBufferColourFormat;
		pBackBuffer->m_DepthStencilFormat	= m_DeviceParams.m_BackBufferDepthStencilFormat;
		pBackBuffer->m_Multisample			= m_DeviceParams.m_BackBufferMultisample;
		m_BackBuffers.PushBack(pBackBuffer);

		C_SurfaceParams surfaceParams;

		// Create the back buffer colour surface.
		{
			if(!CreateColourSurface(
				pBackBuffer->m_Width,
				pBackBuffer->m_Height,
				E_Usage_Managed,
				pBackBuffer->m_ColourFormat,
				pBackBuffer->m_Multisample,
				&surfaceParams,
				&pBackBuffer->m_ColourSurface))
			{
				LOGFATAL(g_Log, "Failed to create the back buffer colour surface\n");
				return false;
			}
		}

		surfaceParams.AfterSurface(&pBackBuffer->m_ColourSurface);

		// Create the back buffer depth stencil surface.
		{
			if(!CreateDepthStencilSurface(
				pBackBuffer->m_Width,
				pBackBuffer->m_Height,
				E_Usage_Managed,
				pBackBuffer->m_DepthStencilFormat,
				pBackBuffer->m_Multisample,
				&surfaceParams,
				&pBackBuffer->m_DepthStencilSurface))
			{
				LOGFATAL(g_Log, "Failed to create the back buffer depth stencil surface\n");
				return false;
			}
		}
	}
	
	// Create the first front buffer.
	{
		C_FrontBuffer* pFrontBuffer		= new C_FrontBuffer();
		pFrontBuffer->m_Width			= m_DeviceParams.m_FrontBufferWidth;
		pFrontBuffer->m_Height			= m_DeviceParams.m_FrontBufferHeight;
		pFrontBuffer->m_Format			= m_DeviceParams.m_FrontBufferFormat;
		m_FrontBuffers.PushBack(pFrontBuffer);

		const U32 size = XGSetTextureHeader(
			pFrontBuffer->m_Width,
			pFrontBuffer->m_Height,
			1,
			0,
			D3DFMT_LE_X8R8G8B8,
			0,
			0,
			XGHEADER_CONTIGUOUS_MIP_OFFSET,
			0,
			NULL,
			NULL,
			NULL);

		if(!CreateTexture2D(
			pFrontBuffer->m_Width,
			pFrontBuffer->m_Height,
			1,
			E_Usage_Video | E_Usage_Indirect,
			(E_Format)D3DFMT_LE_X8R8G8B8,
			&pFrontBuffer->m_Texture,
			curPtr,
			size))
		{
			LOGERROR(g_Log, "Failed to create the first front buffer\n");
			return false;
		}

		#if TRACK_DISPLAY2_RESOURCES
		m_Display2.SetTextureName(&pFrontBuffer->m_Texture, "_FrontBuffer0");
		#endif

		curSize += size;
		
		#ifdef DEVICE_INFO
		LOGINFO(g_Log, "*** FrontBuffer size (%i / %i / %i) width (%i) height (%i) format (%i)\n",
			size, curSize, maxSize,
			pFrontBuffer->m_Width, pFrontBuffer->m_Height, pFrontBuffer->m_Format);
		#endif

		if(curSize > maxSize)
		{
			LOGFATAL(g_Log, "Run out of render target memory %i / %i\n", curSize, maxSize);
			return false;
		}

		curPtr += pFrontBuffer->m_Texture.GetSize();
	}

	// Create the second front buffer.
	{
		C_FrontBuffer* pFrontBuffer		= new C_FrontBuffer();
		pFrontBuffer->m_Width			= m_DeviceParams.m_FrontBufferWidth;
		pFrontBuffer->m_Height			= m_DeviceParams.m_FrontBufferHeight;
		pFrontBuffer->m_Format			= m_DeviceParams.m_FrontBufferFormat;
		m_FrontBuffers.PushBack(pFrontBuffer);

		const U32 size = XGSetTextureHeader(
			pFrontBuffer->m_Width,
			pFrontBuffer->m_Height,
			1,
			0,
			D3DFMT_LE_X8R8G8B8,
			0,
			0,
			XGHEADER_CONTIGUOUS_MIP_OFFSET,
			0,
			NULL,
			NULL,
			NULL);

		if(!CreateTexture2D(
			pFrontBuffer->m_Width,
			pFrontBuffer->m_Height,
			1,
			E_Usage_Video | E_Usage_Indirect,
			(E_Format)D3DFMT_LE_X8R8G8B8,
			&pFrontBuffer->m_Texture,
			curPtr,
			size))
		{
			LOGERROR(g_Log, "Failed to create the second front buffer\n");
			return false;
		}

		#if TRACK_DISPLAY2_RESOURCES
		m_Display2.SetTextureName(&pFrontBuffer->m_Texture, "_FrontBuffer1");
		#endif

		curSize += size;
		
		#ifdef DEVICE_INFO
		LOGINFO(g_Log, "*** FrontBuffer size (%i / %i / %i) width (%i) height (%i) format (%i)\n",
			size, curSize, maxSize,
			pFrontBuffer->m_Width, pFrontBuffer->m_Height, pFrontBuffer->m_Format);
		#endif

		if(curSize > maxSize)
		{
			LOGFATAL(g_Log, "Run out of render target memory %i / %i\n", curSize, maxSize);
			return false;
		}

		curPtr += pFrontBuffer->m_Texture.GetSize();
	}

	m_WindowOffset = curSize;

	#ifdef DEVICE_INFO
	LOGINFO(g_Log, "*** Render target memory %i / %i\n",
		curSize, maxSize);
	#endif

	return true;
}

void C_XB2D3D9Device::DestroySurfaces()
{
	DestroyBackBuffers();
	DestroyFrontBuffers();
}

void C_XB2D3D9Device::ClearWindows()
{
	DestroyWindows();
	
	for(S32 i=0; i<m_Windows.Size(); i++)
	{
		SAFE_DELETE_PTR(m_Windows[i]);
	}

	m_Windows.Clear();
}

void C_XB2D3D9Device::ResizeWindows()
{
	DestroyWindows();

	const S32 windowCount = m_Windows.Size();
	for(S32 windowIndex=0; windowIndex<windowCount; windowIndex++)
	{
		Display::C_Window* pWindow = m_Windows[windowIndex];

		pWindow->Resize(
			m_DeviceParams.m_WindowBufferWidth,
			m_DeviceParams.m_WindowBufferHeight,
			m_DeviceParams.m_WindowBufferMultisample,
			m_DeviceParams.m_WindowBufferColourFormat,
			m_DeviceParams.m_WindowBufferDepthStencilFormat);
	}

	CreateWindows();
}

bool C_XB2D3D9Device::CreateWindows()
{
	U32 maxSize = m_MemorySize;
	U8* curPtr = m_MemoryPtr+m_WindowOffset;
	U32 curSize = m_WindowOffset;
	
	const S32 windowCount = m_Windows.Size();
	for(S32 windowIndex=0; windowIndex<windowCount; windowIndex++)
	{
		C_Window* pWindow = m_Windows[windowIndex];

		// Get the tile setup.

		pWindow->m_TileSetup = GetTileSetup(
			pWindow->m_Width,
			pWindow->m_Height,
			pWindow->m_ColourFormat,
			pWindow->m_DepthStencilFormat,
			pWindow->m_Multisample);

		// Calculate the tile layout.

		CalculateTileLayout(pWindow->m_TileSetup);

		// Calculate the tile size.

		U32 tileWidth = 0;
		U32 tileHeight = 0;
		CalculateTileSize(
			pWindow->m_TileSetup,
			tileWidth,
			tileHeight);

		// Create the surfaces with aa.
		{
			// Create the colour surface with aa.

			C_SurfaceParams surfaceParams;

			if(!CreateColourSurface(
				tileWidth,
				tileHeight,
				0,
				pWindow->m_ColourFormat,
				pWindow->m_Multisample,
				&surfaceParams,
				&pWindow->m_ColourSurfaceWithAA))
			{
				LOGFATAL(g_Log, "Failed to create window %i colour surface with aa\n", windowIndex);
				return false;
			}

			// Create the depth stencil surface with aa.

			surfaceParams.AfterSurface(&pWindow->m_ColourSurfaceWithAA);
			
			if(!CreateDepthStencilSurface(
				tileWidth,
				tileHeight,
				0,
				pWindow->m_DepthStencilFormat,
				pWindow->m_Multisample,
				&surfaceParams,
				&pWindow->m_DepthStencilSurfaceWithAA))
			{
				LOGFATAL(g_Log, "Failed to create window %i depth stencil surface with aa\n", windowIndex);
				return false;
			}
		}
		
		// Create the surfaces without aa.
		{
			// Create the colour surface without aa.

			C_SurfaceParams surfaceParams;

			if(!CreateColourSurface(
				m_DeviceParams.m_BackBufferWidth,
				m_DeviceParams.m_BackBufferHeight,
				0,
				m_DeviceParams.m_BackBufferColourFormat,
				E_Multisample_None,
				&surfaceParams,
				&pWindow->m_ColourSurfaceWithoutAA))
			{
				LOGFATAL(g_Log, "Failed to create window %i colour surface without aa\n", windowIndex);
				return false;
			}

			// Create the depth stencil surface without aa.

			surfaceParams.AfterSurface(&pWindow->m_ColourSurfaceWithoutAA);
			
			if(!CreateDepthStencilSurface(
				m_DeviceParams.m_BackBufferWidth,
				m_DeviceParams.m_BackBufferHeight,
				0,
				m_DeviceParams.m_BackBufferDepthStencilFormat,
				E_Multisample_None,
				&surfaceParams,
				&pWindow->m_DepthStencilSurfaceWithoutAA))
			{
				LOGFATAL(g_Log, "Failed to create window %i depth stencil surface without aa\n", windowIndex);
				return false;
			}
		}

		// Create the textures.
		{
			D3DFORMAT d3dFormat = D3D9Format(pWindow->m_ColourFormat);
			if(d3dFormat == D3DFMT_A2B10G10R10F_EDRAM)
				d3dFormat = D3DFMT_A16B16G16R16F; // [StephenM] Stop this being _EXPAND, big performance hit

			// Create the colour texture.
			{
				const U32 size = XGSetTextureHeader(
					pWindow->m_Width,
					pWindow->m_Height,
					1,
					0,
					d3dFormat,
					0,
					0,
					XGHEADER_CONTIGUOUS_MIP_OFFSET,
					0,
					NULL,
					NULL,
					NULL);

				if(!CreateTexture2D(
					pWindow->m_Width,
					pWindow->m_Height,
					1,
					E_Usage_Colour | E_Usage_Indirect,
					(E_Format)d3dFormat,
					&pWindow->m_ColourTexture,
					curPtr,
					size))
				{
					LOGFATAL(g_Log, "Failed to create window %i colour texture with aa\n", windowIndex);
					return false;
				}

				#if TRACK_DISPLAY2_RESOURCES
				m_Display2.SetTextureName(&pWindow->m_ColourTexture, "_WindowColour");
				#endif

				curSize += size;

				#ifdef DEVICE_INFO
				LOGINFO(g_Log, "*** WindowTexture size (%i / %i / %i) width (%i) height (%i) format (%i)\n",
					size, curSize, maxSize,
					pWindow->m_ColourTexture.GetWidth(),
					pWindow->m_ColourTexture.GetHeight(),
					pWindow->m_ColourTexture.GetFormat());
				#endif

				if(curSize > maxSize)
				{
					LOGFATAL(g_Log, "Run out of render target memory %i / %i\n", curSize, maxSize);
					return false;
				}

				curPtr += size;
			}

			// Create the depth stencil texture.
			{
				const U32 size = XGSetTextureHeader(
					pWindow->m_Width,
					pWindow->m_Height,
					1,
					0,
					D3D9Format(pWindow->m_DepthStencilFormat),
					0,
					0,
					XGHEADER_CONTIGUOUS_MIP_OFFSET,
					0,
					NULL,
					NULL,
					NULL);

				if(!CreateTexture2D(
					pWindow->m_Width,
					pWindow->m_Height,
					1,
					E_Usage_DepthStencil | E_Usage_Indirect,
					pWindow->m_DepthStencilFormat,
					&pWindow->m_DepthStencilTexture,
					curPtr,
					size))
				{
					LOGFATAL(g_Log, "Failed to create window %i depth stencil texture with aa\n", windowIndex);
					return false;
				}

				#if TRACK_DISPLAY2_RESOURCES
				m_Display2.SetTextureName(&pWindow->m_DepthStencilTexture, "_WindowDepthStencil");
				#endif

				curSize += size;

				#ifdef DEVICE_INFO
				LOGINFO(g_Log, "*** DepthTexture size (%i / %i / %i) width (%i) height (%i) format (%i)\n",
					size, curSize, maxSize,
					pWindow->m_DepthStencilTexture.GetWidth(),
					pWindow->m_DepthStencilTexture.GetHeight(),
					pWindow->m_DepthStencilTexture.GetFormat());
				#endif

				if(curSize > maxSize)
				{
					LOGFATAL(g_Log, "Run out of render target memory %i / %i\n", curSize, maxSize);
					return false;
				}

				curPtr += size;
			}
		}
	}

	#ifdef DEVICE_INFO
	LOGINFO(g_Log, "*** Render target memory %i / %i\n",
		curSize, maxSize);
	#endif
	
	return true;
}

void C_XB2D3D9Device::DestroyWindows()
{
	const S32 windowCount = m_Windows.Size();
	for(S32 windowIndex=0; windowIndex<windowCount; windowIndex++)
	{
		C_Window* pWindow = m_Windows[windowIndex];

		DestroySurface(&pWindow->m_ColourSurfaceWithAA);
		DestroySurface(&pWindow->m_DepthStencilSurfaceWithAA);

		DestroySurface(&pWindow->m_ColourSurfaceWithoutAA);
		DestroySurface(&pWindow->m_DepthStencilSurfaceWithoutAA);

		DestroyTexture2D(&pWindow->m_ColourTexture);
		DestroyTexture2D(&pWindow->m_DepthStencilTexture);
	}
}

void C_XB2D3D9Device::DestroyBackBuffers()
{
	for(S32 i=0; i<m_BackBuffers.Size(); i++)
	{
		DestroySurface(&m_BackBuffers[i]->m_ColourSurface);
		DestroySurface(&m_BackBuffers[i]->m_DepthStencilSurface);
		SAFE_DELETE_PTR(m_BackBuffers[i]);
	}
	m_BackBuffers.Clear();
}

void C_XB2D3D9Device::DestroyFrontBuffers()
{
	for(S32 i=0; i<m_FrontBuffers.Size(); i++)
	{
		DestroyTexture2D(&m_FrontBuffers[i]->m_Texture);
		SAFE_DELETE_PTR(m_FrontBuffers[i]);
	}
	m_FrontBuffers.Clear();
}

void C_XB2D3D9Device::SetPresentParams(D3DPRESENT_PARAMETERS& d3dPresentParams)
{
	ZeroMemory(&d3dPresentParams, sizeof(d3dPresentParams));

	d3dPresentParams.BackBufferWidth							= m_DeviceParams.m_BackBufferWidth;
	d3dPresentParams.BackBufferHeight							= m_DeviceParams.m_BackBufferHeight;
	d3dPresentParams.BackBufferFormat							= D3D9Format(m_DeviceParams.m_BackBufferColourFormat);
	d3dPresentParams.BackBufferCount							= 0;
	d3dPresentParams.MultiSampleType							= D3DMULTISAMPLE_NONE;
	d3dPresentParams.MultiSampleQuality							= 0;
	d3dPresentParams.SwapEffect									= D3DSWAPEFFECT_DISCARD;
	d3dPresentParams.hDeviceWindow								= NULL;
	d3dPresentParams.Windowed									= FALSE;
	d3dPresentParams.EnableAutoDepthStencil						= FALSE;
	d3dPresentParams.AutoDepthStencilFormat						= D3D9Format(m_DeviceParams.m_BackBufferDepthStencilFormat);
	d3dPresentParams.Flags										= 0;
	d3dPresentParams.FullScreen_RefreshRateInHz					= 0;
	d3dPresentParams.PresentationInterval						= D3D9PresentInterval(m_PresentIntervalDefault);
	d3dPresentParams.DisableAutoBackBuffer						= TRUE;
	d3dPresentParams.DisableAutoFrontBuffer						= TRUE;
	d3dPresentParams.FrontBufferFormat							= D3DFMT_LE_INDEX16;
	d3dPresentParams.FrontBufferColorSpace						= D3DCOLORSPACE_RGB;
	d3dPresentParams.RingBufferParameters.Flags					= 0;
	d3dPresentParams.RingBufferParameters.PrimarySize			= m_DeviceParams.m_RingBufferPrimarySize;
	d3dPresentParams.RingBufferParameters.pPrimary				= 0;
	d3dPresentParams.RingBufferParameters.SecondarySize			= m_DeviceParams.m_RingBufferSecondarySize;
	d3dPresentParams.RingBufferParameters.pSecondary			= 0;
	d3dPresentParams.RingBufferParameters.SegmentCount			= 0;
}

bool C_XB2D3D9Device::CreateD3D9()
{
	D3DPRESENT_PARAMETERS d3dPresentParams;
	SetPresentParams(d3dPresentParams);

	BIZ_ASSERT(m_DeviceParams.m_pd3dObject);
	BIZ_ASSERT(!m_pd3dDevice);
	const UINT d3dAdapter			= D3DADAPTER_DEFAULT;
	const D3DDEVTYPE d3dDeviceType	= D3DDEVTYPE_HAL;
	const DWORD d3dBehaviorFlags	= D3DCREATE_BUFFER_2_FRAMES;
	const HRESULT hr = m_DeviceParams.m_pd3dObject->CreateDevice(d3dAdapter, d3dDeviceType, NULL, d3dBehaviorFlags, &d3dPresentParams, &m_pd3dDevice);
	if(SUCCEEDED(hr))
	{
		g_pd3dDevice = m_pd3dDevice;
		return true;
	}
	else
	{
		g_pd3dDevice = NULL;
		LOGFATAL( g_Log, "Failed to create d3d9\n");
		return false;
	}
}

void C_XB2D3D9Device::Unbind()
{
	C_D3D9Device::Unbind();
}

#if defined(PLATFORM_XENON) && defined(CFG_DEBUG)
Debug::Bool g_debugDisplayD3DSingleStepper("Graphics.Display.D3D.SingleStepper", false);
#endif

bool C_XB2D3D9Device::Present()
{
	BIZ_ASSERT(m_pd3dDevice);

	SetColourDepthStencilSurface();

	EndTimingFrame();

	m_pd3dDevice->SynchronizeToPresentationInterval();
	
	const C_FrontBuffer& frontBuffer = GetFrontBuffer();

	m_pd3dDevice->Resolve(
		D3DRESOLVE_RENDERTARGET0 | D3DRESOLVE_ALLFRAGMENTS,
		NULL,
		(D3DTexture*)&frontBuffer.m_Texture.m_d3dTexture,
		NULL,
		0,
		0,
		&D3D9GetColour(GetColourClear()),
		GetDepthClear(),
		GetStencilClear(),
		NULL);
	
	D3DVIDEO_SCALER_PARAMETERS params;
	params.ScalerSourceRect.x1 	= 0;
	params.ScalerSourceRect.y1 	= 0;
	params.ScalerSourceRect.x2 	= frontBuffer.GetWidth();
	params.ScalerSourceRect.y2 	= frontBuffer.GetHeight();
	params.ScaledOutputWidth	= m_VideoMode.dwDisplayWidth;
	params.ScaledOutputHeight	= m_VideoMode.dwDisplayHeight;
	params.FilterProfile		= (S32)g_debugDisplayFilterProfile;
	m_pd3dDevice->Swap((D3DTexture*)&frontBuffer.m_Texture.m_d3dTexture, &params);

	BeginTimingFrame();
	
	m_SwappedFrontBuffer = m_CurrentFrontBuffer;
	m_CurrentFrontBuffer++;
	if(m_CurrentFrontBuffer >= m_FrontBuffers.Size())
		m_CurrentFrontBuffer = 0;

	Unbind();

	#if defined(PLATFORM_XENON) && defined(CFG_DEBUG)
	D3D__SingleStepper = g_debugDisplayD3DSingleStepper ? TRUE : FALSE;
	#endif

	m_BeginVerticesUsage = m_UserBeginVerticesUsage = 0;

	SetColourSurface(0, NULL);
	SetDepthStencilSurface(NULL);
	
	return true;
}

void C_XB2D3D9Device::Resolve(const E_Resolve resolve, const C_Texture2D* pTexture, const U32 level)
{
	BIZ_ASSERT(pTexture);
	BIZ_ASSERT(pTexture->IsCreated());
	BIZ_ASSERT(m_pd3dDevice);

	D3DSURFACE_DESC textureDesc;
	((D3DTexture*)&pTexture->m_d3dTexture2D)->GetLevelDesc(level, &textureDesc);

	U32 colourExpBias = 0;
	if(textureDesc.Format == D3DFMT_A16B16G16R16_UINT)
		colourExpBias = D3DRESOLVE_EXPONENTBIAS(+11);

	U32 fragment = 0;
	if(resolve == E_Resolve_Colour)
		fragment = D3DRESOLVE_ALLFRAGMENTS;
	else if(resolve == E_Resolve_DepthStencil)
		fragment = D3DRESOLVE_FRAGMENT0;

	m_pd3dDevice->Resolve(
		D3D9Resolve(resolve) | fragment | colourExpBias,
		NULL,
		(D3DTexture*)&pTexture->m_d3dTexture,
		NULL,
		level,
		0,
		&D3D9GetColour(GetColourClear()),
		GetDepthClear(),
		GetStencilClear(),
		NULL);
}

void C_XB2D3D9Device::Resolve(const DWORD flags, const C_Texture2D* pTexture, const D3DRECT* pSrcRect, const D3DPOINT* pDstPoint, const U32 level)
{
	BIZ_ASSERT(pTexture);
	BIZ_ASSERT(pTexture->IsCreated());
	BIZ_ASSERT(m_pd3dDevice);
	m_pd3dDevice->Resolve(
		flags,
		pSrcRect,
		(D3DTexture*)&pTexture->m_d3dTexture,
		pDstPoint,
		level,
		0,
		&D3D9GetColour(GetColourClear()),
		GetDepthClear(),
		GetStencilClear(),
		NULL);
}

void C_XB2D3D9Device::BeginTiling(const U32 tileCount, const D3DRECT* pTileRects)
{
	BIZ_ASSERT(!m_TilingActive);
	m_TilingActive = true;
	
	BIZ_ASSERT(m_pd3dDevice);
	m_pd3dDevice->BeginTiling(
		0,
		tileCount,
		pTileRects,
		&D3D9GetColour(GetColourClear()),
		GetDepthClear(),
		GetStencilClear());
}

void C_XB2D3D9Device::EndTiling(const C_Texture* pTexture)
{
	BIZ_ASSERT(m_TilingActive);
	m_TilingActive = false;
	
	BIZ_ASSERT(m_pd3dDevice);
	const HRESULT hr = m_pd3dDevice->EndTiling(
		0,
		NULL,
		NULL,
		&D3D9GetColour(GetColourClear()),
		GetDepthClear(),
		GetStencilClear(),
		NULL);
	BIZ_ASSERT(SUCCEEDED(hr) && "Failed to end tiling");

	Display::RingBuffer()->EndTiling();
}

void* C_XB2D3D9Device::BeginVertices(const E_PrimitiveType primitiveType, const U32 vertexCount, const U32 vertexStride, C_VertexDeclaration* pVertexDeclaration, const bool checkUsage)
{
	BIZ_ASSERT(m_pd3dDevice);
	BIZ_ASSERT(pVertexDeclaration);
	BIZ_ASSERT(vertexCount > 0);
	BIZ_ASSERT(vertexCount <= 65535);
	BIZ_ASSERT(!m_BeginVerticesLocked);

	U32 size = vertexCount * vertexStride;

	if (IsTilingActive())
	{
		if(checkUsage)
		{
			if((m_UserBeginVerticesUsage+size) > m_UserMaxBeginVerticesUsage)
			{
				return NULL; // Gone over the user specified amount! Fail gracefully.
			}

			if((m_BeginVerticesUsage+size) > (m_MaxBeginVerticesUsage-m_FreeBeginVerticesUsage))
			{
				return NULL; // Nearly maxed out begin vertices! Make sure theres some free space for other things such as post effects. Fail gracefully.
			}
		}

		if((m_BeginVerticesUsage+size) > m_MaxBeginVerticesUsage)
		{
			LOGFATAL(g_Log, "Begin vertices was maxed out! Failed to render count '%i' stride '%i' size '%i' primitive '%i'\n", vertexCount, vertexStride, vertexCount * vertexStride, primitiveType);
			return NULL;
		}
	}
	
	SetVertexDeclaration(pVertexDeclaration);

	void* pVertices = NULL;
	const HRESULT hr = m_pd3dDevice->BeginVertices(D3D9PrimitiveType(primitiveType), vertexCount, vertexStride, &pVertices);
	if(FAILED(hr))
	{
		if(hr == E_OUTOFMEMORY)
		{
			LOGFATAL(g_Log, "Begin vertices ran out of memory! Failed to render count '%i' stride '%i' size '%i' primitive '%i'\n", vertexCount, vertexStride, vertexCount * vertexStride, primitiveType);
			return NULL;
		}
		else
		{
			LOGFATAL(g_Log, "Begin vertices failed! Failed to begin vertices of count '%i' stride '%i' size '%i' primitive '%i'\n", vertexCount, vertexStride, vertexCount * vertexStride, primitiveType);
			return NULL;
		}
	}
	else
	{
		if(pVertices)
		{
			m_BeginVerticesLocked = true;
			m_UserBeginVerticesUsage += size;
			m_BeginVerticesUsage += size;

			#ifndef CFG_GOLD
			m_BeginVerticesSize = size;
			m_BeginVerticesStart = (U8*)pVertices;
			#endif
		}
		else
		{
			LOGFATAL(g_Log, "Begin vertices returned null! Failed to render count '%i' stride '%i' size '%i' primitive '%i'\n", vertexCount, vertexStride, vertexCount * vertexStride, primitiveType);
			return NULL;
		}
	}
	
	return pVertices;
}

void C_XB2D3D9Device::EndVertices(void* pVertex)
{
	BIZ_ASSERT(m_BeginVerticesLocked);

	BIZ_ASSERT(m_pd3dDevice);
	m_pd3dDevice->EndVertices();

	m_BeginVerticesLocked = false;

	#ifndef CFG_GOLD
	if(pVertex)
	{
		U32 usedEnd = (U32)pVertex;
		U32 actualEnd = (U32)m_BeginVerticesStart+m_BeginVerticesSize;
		if(usedEnd > actualEnd)
		{
			LOGFATAL(g_Log, "C_XB2D3D9Device::EndVertices - usedEnd > actualEnd by %i\n", usedEnd-actualEnd);
		}
		else
		if(usedEnd < actualEnd)
		{
			LOGWARN(g_Log, "C_XB2D3D9Device::EndVertices - usedEnd < actualEnd by %i\n", actualEnd-usedEnd);
		}
	}
	m_BeginVerticesSize = 0;
	m_BeginVerticesStart = NULL;
	#endif
}

bool C_XB2D3D9Device::CreateColourSurface(const U32 width, const U32 height, const U32 flags, const D3DFORMAT format, const D3DMULTISAMPLE_TYPE multisample, const C_SurfaceParams* pParams, C_Surface* pSurface)
{
	return m_Display2.CreateColourSurface(width, height, flags, (Display2::E_SurfaceFormat)format, (Display2::E_Multisample)multisample, pParams, pSurface);
}

bool C_XB2D3D9Device::CreateDepthStencilSurface(const U32 width, const U32 height, const U32 flags, const D3DFORMAT format, const D3DMULTISAMPLE_TYPE multisample, const C_SurfaceParams* pParams, C_Surface* pSurface)
{
	return m_Display2.CreateDepthStencilSurface(width, height, flags, (Display2::E_SurfaceFormat)format, (Display2::E_Multisample)multisample, pParams, pSurface);
}

void C_XB2D3D9Device::DestroySurface(C_Surface* pSurface)
{
	m_Display2.DestroySurface(pSurface);
}

bool C_XB2D3D9Device::CreateTexture2D(const U32 width, const U32 height, const U32 levels, const U32 flags, const E_Format format, C_Texture2D* pTexture, U8* pAddress, const U32 addressSize)
{
	return m_Display2.CreateTexture2D(width, height, levels, flags, (Display2::E_TextureFormat)format, pTexture, pAddress, addressSize);
}

void C_XB2D3D9Device::DestroyTexture2D(C_Texture2D* pTexture)
{
	m_Display2.DestroyTexture(pTexture);
}

bool C_XB2D3D9Device::CreateTexture3D(const U32 width, const U32 height, const U32 depth, const U32 levels, const U32 flags, const E_Format format, C_Texture3D* pTexture, U8* pAddress, const U32 addressSize)
{
	return m_Display2.CreateTexture3D(width, height, depth, levels, flags, (Display2::E_TextureFormat)format, pTexture, pAddress, addressSize);
}

void C_XB2D3D9Device::DestroyTexture3D(C_Texture3D* pTexture)
{
	m_Display2.DestroyTexture(pTexture);
}

bool C_XB2D3D9Device::CreateTextureCube(const U32 width, const U32 levels, const U32 flags, const E_Format format, C_TextureCube* pTexture, U8* pAddress, const U32 addressSize)
{
	return m_Display2.CreateTextureCube(width, levels, flags, (Display2::E_TextureFormat)format, pTexture, pAddress, addressSize);
}

void C_XB2D3D9Device::DestroyTextureCube(C_TextureCube* pTexture)
{
	m_Display2.DestroyTexture(pTexture);
}

bool C_XB2D3D9Device::CreateRenderTexture2D(const U32 width, const U32 height, const U32 levels, U32 flags, const D3DFORMAT format, const D3DMULTISAMPLE_TYPE multisample, const C_SurfaceParams* pParams, C_RenderTexture2D* pRenderTexture, U8* pAddress, const U32 addressSize)
{
	BIZ_ASSERT(width > 0);
	BIZ_ASSERT(height > 0);
	BIZ_ASSERT(levels > 0);
	BIZ_ASSERT(pRenderTexture);
	BIZ_ASSERT(!pRenderTexture->IsCreated());

	if(!(flags & Display2::E_Flags_Colour) || !(flags & Display2::E_Flags_DepthStencil))
		flags |= Display2::E_Flags_Colour;
	
	pRenderTexture->m_Flags = flags;

	D3DFORMAT textureFormat = format;
	if(textureFormat == D3DFMT_A2B10G10R10F_EDRAM)
		textureFormat = D3DFMT_A16B16G16R16F;
	if(textureFormat == D3DFMT_A16B16G16R16F && (flags & E_Usage_Expand))
		textureFormat = D3DFMT_A16B16G16R16F_EXPAND;

	S32 surfaceExpBias = 0;
	S32 textureExpBias = 0;
	if(textureFormat == D3DFMT_A16B16G16R16F && (flags & E_Usage_Uint))
	{
		surfaceExpBias = +5;
		textureExpBias = -16;
		textureFormat = D3DFMT_A16B16G16R16_UINT;
	}

	if(!CreateTexture2D(width, height, levels, flags, (E_Format)textureFormat, &pRenderTexture->m_Texture, pAddress, addressSize))
		return false;

	BIZ_ASSERT(pRenderTexture->m_Surfaces.Size() == 0);
	pRenderTexture->m_Surfaces.Resize(levels);
	
	for(U32 level=0;level <levels; level++)
	{
		C_Surface& surface = pRenderTexture->m_Surfaces[level];
		
		const U32 levelWidth = pRenderTexture->m_Texture.GetWidth(level);
		const U32 levelHeight = pRenderTexture->m_Texture.GetHeight(level);

		if(!CreateColourSurface(levelWidth, levelHeight, flags, format, multisample, pParams, &surface))
			return false;

		surface.m_d3dSurface.ColorInfo.ColorExpBias = surfaceExpBias;
	}

	pRenderTexture->m_Texture.m_d3dTexture.Format.ExpAdjust = textureExpBias;

	#if TRACK_DISPLAY2_RESOURCES
	if(!(flags & Display2::E_Flags_DoNotTrack))
		m_Display2.InsertRenderTexture2D(pRenderTexture);
	#endif

	return true;
}

void C_XB2D3D9Device::DestroyRenderTexture2D(C_RenderTexture2D* pRenderTexture)
{
	BIZ_ASSERT(pRenderTexture);

	for(S32 level=0; level<pRenderTexture->m_Surfaces.Size(); level++)
	{
		C_Surface& surface = pRenderTexture->m_Surfaces[level];
	
		DestroySurface(&surface);
	}

	pRenderTexture->m_Surfaces.Clear();

	DestroyTexture2D(&pRenderTexture->m_Texture);

	#if TRACK_DISPLAY2_RESOURCES
	if(!(pRenderTexture->m_Flags & Display2::E_Flags_DoNotTrack))
		m_Display2.RemoveRenderTexture2D(pRenderTexture);
	#endif
	
	pRenderTexture->m_Flags = 0;
}

bool C_XB2D3D9Device::CreateRenderTextureCube(const U32 width, const U32 levels, U32 flags, const D3DFORMAT format, const D3DMULTISAMPLE_TYPE multisample, const C_SurfaceParams* pParams, C_RenderTextureCube* pRenderTexture, U8* pAddress, const U32 addressSize)
{
	BIZ_ASSERT(width > 0);
	BIZ_ASSERT(levels > 0);
	BIZ_ASSERT(pRenderTexture);
	BIZ_ASSERT(!pRenderTexture->IsCreated());

	if(!(flags & Display2::E_Flags_Colour) || !(flags & Display2::E_Flags_DepthStencil))
		flags |= Display2::E_Flags_Colour;
	
	pRenderTexture->m_Flags = flags;

	if(!CreateTextureCube(width, levels, flags, (E_Format)format, &pRenderTexture->m_Texture, pAddress, addressSize))
		return false;

	for(U32 face=0; face<6; face++)
	{
		BIZ_ASSERT(pRenderTexture->m_Surfaces[face].Size() == 0);
		pRenderTexture->m_Surfaces[face].Resize(levels);

		for(U32 level=0; level<levels; level++)
		{
			C_Surface& surface = pRenderTexture->m_Surfaces[face][level];
			
			const U32 levelWidth = pRenderTexture->m_Texture.GetWidth(level);

			if(!CreateColourSurface(levelWidth, levelWidth, flags, format, multisample, pParams, &surface))
				return false;
		}
	}

	#if TRACK_DISPLAY2_RESOURCES
	if(!(flags & Display2::E_Flags_DoNotTrack))
		m_Display2.InsertRenderTextureCube(pRenderTexture);
	#endif

	return true;
}

void C_XB2D3D9Device::DestroyRenderTextureCube(C_RenderTextureCube* pRenderTexture)
{
	BIZ_ASSERT(pRenderTexture);

	for(U32 face=0; face<6; face++)
	{
		for(S32 level=0; level<pRenderTexture->m_Surfaces[face].Size(); level++)
		{
			C_Surface& surface = pRenderTexture->m_Surfaces[face][level];
			
			DestroySurface(&surface);
		}

		pRenderTexture->m_Surfaces[face].Clear();
	}

	DestroyTextureCube(&pRenderTexture->m_Texture);

	#if TRACK_DISPLAY2_RESOURCES
	if(!(pRenderTexture->m_Flags & Display2::E_Flags_DoNotTrack))
		m_Display2.RemoveRenderTextureCube(pRenderTexture);
	#endif

	pRenderTexture->m_Flags = 0;
}

bool C_XB2D3D9Device::CreateVertexBuffer(const U32 size, const U32 flags, C_VertexBuffer* pVertexBuffer, bool allowFail)
{
	return m_Display2.CreateVertexBuffer(size, flags, pVertexBuffer);
}

void C_XB2D3D9Device::DestroyVertexBuffer(C_VertexBuffer* pVertexBuffer)
{
	m_Display2.DestroyVertexBuffer(pVertexBuffer);
}

bool C_XB2D3D9Device::CreateIndexBuffer(const U32 size, const U32 flags, const E_Format format, C_IndexBuffer* pIndexBuffer, bool allowFail)
{
	return m_Display2.CreateIndexBuffer(size, flags, (Display2::E_IndexFormat)format, pIndexBuffer);
}

void C_XB2D3D9Device::DestroyIndexBuffer(C_IndexBuffer* pIndexBuffer)
{
	m_Display2.DestroyIndexBuffer(pIndexBuffer);
}

bool C_XB2D3D9Device::CreateVertexShader(const U32 size, const S32* pData, const U32 flags, C_VertexShader* pVertexShader)
{
	return m_Display2.CreateVertexShader(size, (const U8*)pData, flags, pVertexShader);
}

void C_XB2D3D9Device::DestroyVertexShader(C_VertexShader* pVertexShader)
{
	if(pVertexShader && pVertexShader->IsCreated())
		g_ContextManager.FreePatchedVertexShader(pVertexShader->GetGPUVertexShader());

	m_Display2.DestroyVertexShader(pVertexShader);
}

bool C_XB2D3D9Device::CreatePixelShader(const U32 size, const S32* pData, const U32 flags, C_PixelShader* pPixelShader)
{
	return m_Display2.CreatePixelShader(size, (const U8*)pData, flags, pPixelShader);
}

void C_XB2D3D9Device::DestroyPixelShader(C_PixelShader* pPixelShader)
{
	m_Display2.DestroyPixelShader(pPixelShader);
}

IDirect3DTexture9* C_XB2D3D9Device::AllocTextureHeader()
{
	#ifdef USE_D3D_TEXTURE_POOL
	IDirect3DTexture9* pd3dTexture = g_d3dTexturePool.Allocate();
	#if !defined(CFG_GOLD) && defined(USE_D3D_TEXTURE_POOL) && defined(USER_LLOYDW)
	if(g_debugShowTexturePoolInfo)
	{
		LOGINFO(g_Log, "*** TexturePool : Used '%i' Free '%i' Size '%i'\n", g_d3dTexturePool.UsedCount(), g_d3dTexturePool.FreeCount(), g_d3dTexturePool.TotalSize());
	}
	#endif
	return pd3dTexture;
	#else
	return new IDirect3DTexture9;
	#endif
}

void C_XB2D3D9Device::FreeTextureHeader(IDirect3DTexture9*& pd3dTexture)
{
	if(pd3dTexture)
	{
		#ifdef USE_D3D_TEXTURE_POOL
		g_d3dTexturePool.Free(pd3dTexture);
		#if !defined(CFG_GOLD) && defined(USE_D3D_TEXTURE_POOL) && defined(USER_LLOYDW)
		if(g_debugShowTexturePoolInfo)
		{
			LOGINFO(g_Log, "*** TexturePool : Used '%i' Free '%i' Size '%i'\n", g_d3dTexturePool.UsedCount(), g_d3dTexturePool.FreeCount(), g_d3dTexturePool.TotalSize());
		}
		#endif
		#else
		delete pd3dTexture;
		#endif
		pd3dTexture = NULL;
	}
}

void C_XB2D3D9Device::FreeBaseTextureHeader(IDirect3DBaseTexture9*& pd3dBaseTexture)
{
	if(pd3dBaseTexture)
	{
		IDirect3DTexture9* pdd3dTexture = (IDirect3DTexture9*)pd3dBaseTexture;
		#ifdef USE_D3D_TEXTURE_POOL
		g_d3dTexturePool.Free(pdd3dTexture);
		#else
		delete pdd3dTexture;
		#endif
		pd3dBaseTexture = NULL;
	}
}

const U32 C_XB2D3D9Device::GetMemoryAlignment(const E_ResourceType type, const U32 flags)
{
	switch(type)
	{
	case E_ResourceType_Surface:
		return D3DSURFACE_ALIGNMENT;
	case E_ResourceType_Texture2D:
		return D3DTEXTURE_ALIGNMENT;
	case E_ResourceType_Texture3D:
		return D3DVOLUME_ALIGNMENT;
	case E_ResourceType_TextureCube:
		return D3DTEXTURE_ALIGNMENT;
	case E_ResourceType_VertexBuffer:
		return D3DVERTEXBUFFER_ALIGNMENT;
	case E_ResourceType_IndexBuffer:
		return D3DINDEXBUFFER_ALIGNMENT;
	case E_ResourceType_VertexShader:
		return VERTEXSHADER_ALIGNMENT;
	case E_ResourceType_PixelShader:
		return PIXELSHADER_ALIGNMENT;
	default:
		BIZ_ASSERT_FAIL("Unsupported resource type");
		return 0;
	}
}

bool C_XB2D3D9Device::AllocResourceMemory(const E_ResourceType type, U32& flags, const U32 size, U8** ppAddress, const S32 systemID)
{
#if 1
	return m_Display2.AllocResourceMemory(ToResourceType2(type), flags, size, ppAddress);
#else
	BIZ_ASSERT(size > 0 && "Attempting to alloc graphics memory of zero size...this is bad!");

	#ifdef DEVICE_SUPER_DEBUG_MODE
	Output("C_XB2D3D9Device::AllocResourceMemory\n");
	#endif

	#ifndef CFG_SPEED
	if(MemoryGetSystemID() == 1)
	{
		// you need to add RESOURCE_MEMORY_ID("GraphicsMemoryType"); somewhere in the callstack before this resource is allocated so it can be tracked
		LOGWARN(g_Log, "Allocating unknown graphics memory of size %i type %i\n", size, type);
	}
	#endif

	E_AllocHeap allocHeap;
	E_AllocType allocType;
	E_MemoryType memoryType;
	U32 memoryProtect = PAGE_READWRITE;

	switch(type)
	{
	case E_ResourceType_Surface:
		allocHeap			= E_AllocHeap_Large;
		allocType			= E_AllocType_Start;
		memoryType			= E_MemoryType_Surface;
		break;
	case E_ResourceType_Texture2D:
		allocHeap			= E_AllocHeap_Large;
		allocType			= E_AllocType_Start;
		memoryType			= E_MemoryType_Texture2D;
		if(flags & E_Usage_Dynamic)
			memoryProtect	|= PAGE_WRITECOMBINE;
		break;
	case E_ResourceType_Texture3D:
		allocHeap			= E_AllocHeap_Large;
		allocType			= E_AllocType_Start;
		memoryType			= E_MemoryType_Texture3D;
		if(flags & E_Usage_Dynamic)
			memoryProtect	|= PAGE_WRITECOMBINE;
		break;
	case E_ResourceType_TextureCube:
		allocType			= E_AllocType_Start;
		allocHeap			= E_AllocHeap_Large;
		memoryType			= E_MemoryType_TextureCube;
		if(flags & E_Usage_Dynamic)
			memoryProtect	|= PAGE_WRITECOMBINE;
		break;
	case E_ResourceType_VertexBuffer:
		allocHeap			= E_AllocHeap_Large;
		allocType			= E_AllocType_End;
		memoryType			= E_MemoryType_VertexBuffer;
		if(flags & E_Usage_Dynamic)
			memoryProtect	|= PAGE_WRITECOMBINE;
		break;
	case E_ResourceType_IndexBuffer:
		allocHeap			= E_AllocHeap_Large;
		allocType			= E_AllocType_End;
		memoryType			= E_MemoryType_IndexBuffer;
		if(flags & E_Usage_Dynamic)
			memoryProtect	|= PAGE_WRITECOMBINE;
		break;
	case E_ResourceType_VertexShader:
		allocHeap			= E_AllocHeap_Small;
		allocType			= E_AllocType_Start;
		memoryType			= E_MemoryType_VertexShader;
		memoryProtect		|= PAGE_WRITECOMBINE;
		break;
	case E_ResourceType_PixelShader:
		allocHeap			= E_AllocHeap_Small;
		allocType			= E_AllocType_Start;
		memoryType			= E_MemoryType_PixelShader;
		memoryProtect		|= PAGE_WRITECOMBINE;
		break;
	default:
		BIZ_ASSERT_FAIL("Unsupported resource type");
		return false;
	}

	U32 memoryAlign = GetMemoryAlignment(type, flags);

	bool allocOutOfSystemMemory = false;
	bool allocOutOfGraphicsMemory = false;

	if ((flags & E_Usage_System) || (flags & E_Usage_Dynamic))
	{
		allocOutOfSystemMemory = true;
	}
	else
	{
		allocOutOfGraphicsMemory = true;
	}

	if(allocOutOfGraphicsMemory)
	{
		// We want to alloc out of video memory.

		if(AllocMemory(memoryAlign, size, ppAddress, allocHeap,  allocType))
		{
			MemoryIncSystem(systemID, size, memoryType);

			flags &= ~E_Usage_System;
			flags |= E_Usage_Video;
		}
		else
		{
			// We failed to alloc out of video memory so lets try system memory.

			allocOutOfSystemMemory = true;
		}
	}

	if(allocOutOfSystemMemory)
	{
		// We want to alloc out of system memory.

		*ppAddress = (U8*)BizPhysicalAlloc(size, MAXULONG_PTR, memoryAlign, memoryProtect);

		if(*ppAddress)
		{
			flags &= ~E_Usage_Video;
			flags |= E_Usage_System;
		}
		else
		{
			return false; // We failed to alloc any graphics memory...this is bad!
		}
	}
#endif

	return true;
}

void C_XB2D3D9Device::FreeResourceMemory(const E_ResourceType type, const U32 flags, const U32 size, U8* pAddress, const S32 systemID)
{
#if 1
	Display2::E_MemoryType memoryType;
	switch(type)
	{
	case E_ResourceType_Surface:		memoryType = Display2::E_MemoryType_Surface; 		break;
	case E_ResourceType_Texture2D:		memoryType = Display2::E_MemoryType_Texture; 		break;
	case E_ResourceType_Texture3D:		memoryType = Display2::E_MemoryType_Texture; 		break;
	case E_ResourceType_TextureCube:	memoryType = Display2::E_MemoryType_Texture; 		break;
	case E_ResourceType_VertexBuffer:	memoryType = Display2::E_MemoryType_VertexBuffer;	break;
	case E_ResourceType_IndexBuffer:	memoryType = Display2::E_MemoryType_IndexBuffer;	break;
	case E_ResourceType_VertexShader:	memoryType = Display2::E_MemoryType_VertexShader;	break;
	case E_ResourceType_PixelShader:	memoryType = Display2::E_MemoryType_PixelShader;	break;
	default:							memoryType = Display2::E_MemoryType_Unknown;		break;
	}
	m_Display2.FreeMemory(size, pAddress, memoryType);
#else
	BIZ_ASSERT(size > 0 && "Attempting to free graphics memory of zero size...this is bad!");

	#ifdef DEVICE_SUPER_DEBUG_MODE
	Output("C_XB2D3D9Device::FreeResourceMemory\n");
	#endif

	if(flags & E_Usage_System)
	{
		BizPhysicalFree(pAddress);
	}
	else
	{
		E_AllocHeap allocHeap;
		E_MemoryType memoryType;
		
		switch(type)
		{
		case E_ResourceType_Surface:
			allocHeap		= E_AllocHeap_Large;
			memoryType		= E_MemoryType_Surface;
			break;
		case E_ResourceType_Texture2D:
			allocHeap		= E_AllocHeap_Large;
			memoryType		= E_MemoryType_Texture2D;
			break;
		case E_ResourceType_Texture3D:
			allocHeap		= E_AllocHeap_Large;
			memoryType		= E_MemoryType_Texture3D;
			break;
		case E_ResourceType_TextureCube:
			allocHeap		= E_AllocHeap_Large;
			memoryType		= E_MemoryType_TextureCube;
			break;
		case E_ResourceType_VertexBuffer:
			allocHeap		= E_AllocHeap_Large;
			memoryType		= E_MemoryType_VertexBuffer;
			break;
		case E_ResourceType_IndexBuffer:
			allocHeap		= E_AllocHeap_Large;
			memoryType		= E_MemoryType_IndexBuffer;
			break;
		case E_ResourceType_VertexShader:
			allocHeap		= E_AllocHeap_Small;
			memoryType		= E_MemoryType_VertexShader;
			break;
		case E_ResourceType_PixelShader:
			allocHeap		= E_AllocHeap_Small;
			memoryType		= E_MemoryType_PixelShader;
			break;
		default:
			BIZ_ASSERT_FAIL("Unsupported resource type");
			return;
		}

		if(flags & E_Usage_System)
		{
			BizPhysicalFree(pAddress);
		}
		else
		{
			FreeMemory(size, pAddress, allocHeap);

			MemoryDecSystem(systemID, size, memoryType);
		}
	}
#endif
}

bool C_XB2D3D9Device::AllocMemory(const U32 align, const U32 size, U8** ppAddress, const E_AllocHeap allocHeap, const E_AllocType allocType, const Display2::E_MemoryType memoryType)
{
#if 1
	U32 flags = 0;
	switch(allocHeap)
	{
	case E_AllocHeap_Large:
		flags &= ~E_Usage_Dynamic;
		break;
	case E_AllocHeap_Small:
		flags |= E_Usage_Dynamic;
		break;
	default:
		BIZ_ASSERT_FAIL("Unsupported alloc heap");
		return false;
	}
	return m_Display2.AllocMemory(size, align, flags, ppAddress, memoryType);
#else
	C_MemoryBuffer* pHeap = NULL;

	switch(allocHeap)
	{
	case E_AllocHeap_Large:
		pHeap = &m_LargeHeap;
		break;
	case E_AllocHeap_Small:
		pHeap = &m_SmallHeap;
		break;
	default:
		BIZ_ASSERT_FAIL("Unsupported alloc heap");
		return false;
	}

	switch(allocType)
	{
	case E_AllocType_Start:
		return pHeap->AllocateFromStart(align, size, ppAddress);
	case E_AllocType_End:
		return pHeap->AllocateFromEnd(align, size, ppAddress);
	default:
		BIZ_ASSERT_FAIL("Unsupported alloc type");
		return false;
	}
#endif
}

void C_XB2D3D9Device::FreeMemory(const U32 size, U8* pAddress, const E_AllocHeap allocHeap, const Display2::E_MemoryType memoryType)
{
#if 1
	m_Display2.FreeMemory(size, pAddress, memoryType);
#else
	C_MemoryBuffer* pHeap = NULL;

	switch(allocHeap)
	{
	case E_AllocHeap_Large:
		pHeap = &m_LargeHeap;
		break;
	case E_AllocHeap_Small:
		pHeap = &m_SmallHeap;
		break;
	default:
		BIZ_ASSERT_FAIL("Unsupported alloc heap");
		return;
	}

	pHeap->Free(size, pAddress);
#endif
}

void C_XB2D3D9Device::SetWindow(const S32 windowIndex, const bool aa)
{
	if(windowIndex != -1)
	{
		C_Window* pWindow = GetWindow(windowIndex);

		if(aa)
		{
			SetColourDepthStencilSurface(&pWindow->m_ColourSurfaceWithAA, &pWindow->m_DepthStencilSurfaceWithAA);
		}
		else
		{
			SetColourDepthStencilSurface(&pWindow->m_ColourSurfaceWithoutAA, &pWindow->m_DepthStencilSurfaceWithoutAA);
		}
	}

	m_CurrentWindow = windowIndex;
}

S32 C_XB2D3D9Device::GetWindowIndex()
{
	return m_CurrentWindow;
}

void C_XB2D3D9Device::BeginWindow()
{
	BIZ_ASSERT(m_CurrentWindow != -1);

	C_Window* pWindow = GetWindow(m_CurrentWindow);
	
	BeginTiling(pWindow->m_TileSetup.m_TileCount, pWindow->m_TileSetup.m_Tiles);
}

void C_XB2D3D9Device::EndWindow()
{
	BIZ_ASSERT(m_CurrentWindow != -1);

	C_Window* pWindow = GetWindow(m_CurrentWindow);

	#ifdef USE_D3D_PIX
	#if _XDK_VER < 7776
	if(pWindow->m_TileSetup.m_TileCount > 1)
		PIXBeginNamedEvent(0xFFFFFFFF, "HackyFix");
	#endif
	#endif

	for(U32 tileIndex=0; tileIndex<pWindow->m_TileSetup.m_TileCount; tileIndex++)
	{
		m_pd3dDevice->SetPredication(D3DPRED_TILE(tileIndex));

		D3DRECT* pSrcRect = &pWindow->m_TileSetup.m_Tiles[tileIndex];
		D3DPOINT* pDstPoint = (D3DPOINT*)pSrcRect;
		
		Resolve(
			D3DRESOLVE_DEPTHSTENCIL | D3DRESOLVE_FRAGMENT0,
			&pWindow->m_DepthStencilTexture,
			pSrcRect,
			pDstPoint);

		Resolve(D3DRESOLVE_RENDERTARGET0 | D3DRESOLVE_CLEARRENDERTARGET | D3DRESOLVE_CLEARDEPTHSTENCIL,
			&pWindow->m_ColourTexture,
			pSrcRect,
			pDstPoint);
	}

	m_pd3dDevice->SetPredication(D3DPRED_TILE(0));

	EndTiling(NULL);

	m_CurrentWindow = -1;
}

void C_XB2D3D9Device::RestoreWindow(const S32 windowIndex, const U32 colourWrite)
{
	BIZ_ASSERT(windowIndex != -1);

	C_Window* pWindow = GetWindow(windowIndex);

	SetBlendMode(E_BlendMode_Solid);
	SetAlphaToMaskEnable(E_Bool_False);
	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	m_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

	SetVertexDeclaration(NULL);
	SetVertexShader(&VertexShader_XY::m_VertexShader);
	SetPixelShader(&PixelShader::m_PixelShaderDepth);
	
	SetTexture(0, &pWindow->m_ColourTexture);
	SetTextureFilterMagMinMip(0, E_TextureFilter_Point, E_TextureFilter_Point, E_TextureFilter_Point);
	SetTextureAddressUVW(0, E_TextureAddress_Clamp, E_TextureAddress_Clamp, E_TextureAddress_Clamp);
	
	SetTexture(1, &pWindow->m_DepthStencilTexture);
	SetTextureFilterMagMinMip(1, E_TextureFilter_Point, E_TextureFilter_Point, E_TextureFilter_Point);
	SetTextureAddressUVW(1, E_TextureAddress_Clamp, E_TextureAddress_Clamp, E_TextureAddress_Clamp);

	SetColourWriteEnable(colourWrite);
	
	SetDepthFunc(E_CmpFunc_Always);
	SetStencilEnable(Display::E_Bool_False);

	Quad::DrawFullScreenDepth();

	SetDepthFunc(E_CmpFunc_Default);
	
	SetColourWriteEnable(E_ColourWriteEnable_Default);
}

void C_XB2D3D9Device::BlockUntilIdle()
{
	AcquireThreadOwnership();
	BIZ_ASSERT(HaveThreadOwnership()); // Make sure this is called from the render thread
	BIZ_ASSERT(m_pd3dDevice);
	m_pd3dDevice->BlockUntilIdle();
	ReleaseThreadOwnership();
}

U32 C_XB2D3D9Device::InsertFence()
{
	return m_pd3dDevice->InsertFence();
}

U32 C_XB2D3D9Device::GetCurrentFence()
{
	return m_pd3dDevice->GetCurrentFence();
}

bool C_XB2D3D9Device::IsFencePassed(const U32 value)
{
	return !m_pd3dDevice->IsFencePending(value);
}

void C_XB2D3D9Device::BlockOnFence(const U32 value)
{
	m_pd3dDevice->BlockOnFence(value);
}

void C_XB2D3D9Device::SetShaderGPRAllocation(const U32 flags, const U32 vertexShaderCount, const U32 pixelShaderCount)
{
	BIZ_ASSERT(m_pd3dDevice);
	m_pd3dDevice->SetShaderGPRAllocation(flags, vertexShaderCount, pixelShaderCount);
}

void C_XB2D3D9Device::SetUserMaxBeginVerticesUsage(const F32 beginVerticesUsageScale)
{
	BIZ_ASSERT(beginVerticesUsageScale >= 0.0f && beginVerticesUsageScale <= 1.0f);
	if(m_TilingActive)
	{
		BIZ_ASSERT(m_CurrentWindow != -1);
		C_Window* pWindow = GetWindow(m_CurrentWindow);

		F32 tilingScale = 1.0f/(F32)pWindow->m_TileSetup.m_TileCount;
		m_UserMaxBeginVerticesUsage = (U32)(((F32)m_MaxBeginVerticesUsage)*beginVerticesUsageScale*tilingScale);
	}
	else
	{
		m_UserMaxBeginVerticesUsage = (U32)(((F32)m_MaxBeginVerticesUsage)*beginVerticesUsageScale);
	}
	m_UserBeginVerticesUsage = 0;
}

void C_XB2D3D9Device::DumpMemoryHeaps()
{
#ifndef CFG_GOLD
	m_Display2.DumpMemoryHeaps();
#endif
}

void C_XB2D3D9Device::BeginTimingFrame()
{
	if(m_CpuTimeEnd > m_CpuTimeBegin)
		m_CpuTime = ((F32)(m_CpuTimeEnd - m_CpuTimeBegin) / (F32)SYS_USEC_PER_SEC) * 1000.0f;

	U32 i = m_GpuTimeIndex == 0 ? 1 : 0;

	if(m_GpuTimeEnd[i] > m_GpuTimeBegin[i])
		m_GpuTime = (F32)(m_GpuTimeEnd[i] - m_GpuTimeBegin[i]) / 1000.0f;

	m_CpuTimeBegin = Time::GetPerformanceCounter();

	BIZ_ASSERT(m_pd3dDevice);
	m_pd3dDevice->SetPredication(D3DPRED_ALL_RENDER | D3DPRED_ALL_Z);
	m_pd3dDevice->InsertCallback(D3DCALLBACK_IMMEDIATE, D3DAfterPresentCallback, (DWORD)this);
	m_pd3dDevice->SetPredication(0);

	#if !CFG_GOLD
	if(g_debugDisplayPrintFrameTimes)
	{
		LOGINFO(g_Log, "Gpu %.2f Cpu %.2f\n", m_GpuTime, m_CpuTime);
	}
	#endif
}

void C_XB2D3D9Device::EndTimingFrame()
{
	m_CpuTimeEnd = Time::GetPerformanceCounter();

	BIZ_ASSERT(m_pd3dDevice);
	m_pd3dDevice->SetPredication(D3DPRED_ALL_RENDER | D3DPRED_ALL_Z);
	m_pd3dDevice->InsertCallback(D3DCALLBACK_IMMEDIATE, D3DBeforePresentCallback, (DWORD)this);
	m_pd3dDevice->SetPredication(0);
}

void C_XB2D3D9Device::BeforePresentCallback()
{
	m_GpuTimeEnd[m_GpuTimeIndex] = Time::GetPerformanceCounter();

	m_GpuTimeIndex = m_GpuTimeIndex == 0 ? 1 : 0;
}

void C_XB2D3D9Device::AfterPresentCallback()
{
	m_GpuTimeBegin[m_GpuTimeIndex] = Time::GetPerformanceCounter();
}

}; // namespace D3D9
}; // namespace Display
