#include "Stdafx.h"
#include "UiRenderBuffer.h"

#if !defined(CFG_GOLD) && defined(USER_LLWRIGHT)
#pragma optimize("", off)
#endif

namespace UI 
{

C_UiRenderBuffer::C_UiRenderBuffer() : Display::I_Renderer("UiRenderBuffer"),
	m_Bound(false),
	m_Width(0),
	m_Height(0),
	m_Multisample(Display::E_Multisample_None),
	m_ColourFormat(Display::E_Format_A8R8G8B8),
#if PLATFORM_PC
	m_DepthStencilFormat(Display::E_Format_D24S8)
#else
	m_DepthStencilFormat(Display::E_Format_D24FS8)
#endif
{
}

C_UiRenderBuffer::~C_UiRenderBuffer()
{
	DestroyRenderer();
}

void C_UiRenderBuffer::SetWidth(const U32 width)
{
	m_Width = width;
}

void C_UiRenderBuffer::SetHeight(const U32 height)
{
	m_Height = height;
}

void C_UiRenderBuffer::SetMultisample(const Display::E_Multisample multisample)
{
	m_Multisample = multisample;
}

void C_UiRenderBuffer::SetColourFormat(const Display::E_Format colourFormat)
{
	m_ColourFormat = colourFormat;
}

void C_UiRenderBuffer::SetDepthStencilFormat(const Display::E_Format depthStencilFormat)
{
	m_DepthStencilFormat = depthStencilFormat;
}

bool C_UiRenderBuffer::OnCreateVolatile()
{
	RESOURCE_MEMORY_ID("UiRenderBuffer");

	Display::C_SurfaceParams surfaceParams;

#if PLATFORM_XENON
	m_TileSetup = Display::GetTileSetup(m_Width, m_Height, m_ColourFormat, m_DepthStencilFormat, m_Multisample);
	
	U32 adjustedWidth = 0;
	U32 adjustedHeight = 0;

	Display::CalculateTileSize(m_TileSetup, adjustedWidth, adjustedHeight);

	if(!Display::Device()->CreateColourSurface(
		adjustedWidth,
		adjustedHeight,
		0,
		m_ColourFormat,
		m_Multisample,
		&surfaceParams,
		&m_ColourSurface))
	{
		LOGFATAL(Display::g_Log, "Failed to create ui colour surface\n");
		return false;
	}

	#if TRACK_DISPLAY2_RESOURCES
	Display2::Device()->SetSurfaceName(&m_ColourSurface, "_UiRenderBufferC");
	#endif
	
	surfaceParams.AfterSurface(&m_ColourSurface);
	
	if(!Display::Device()->CreateTexture2D(
		m_Width,
		m_Height,
		1,
		0,
		m_ColourFormat,
		&m_ColourTexture))
	{
		LOGFATAL(Display::g_Log, "Failed to create ui texture\n");
		return false;
	}

	#if TRACK_DISPLAY2_RESOURCES
	Display2::Device()->SetTextureName(&m_ColourTexture, "_UiRenderBufferColourSurface");
	#endif
#else
	U32 adjustedWidth = m_Width;
	U32 adjustedHeight = m_Height;

	if(!Display::Device()->CreateRenderTexture2D(
		m_Width,
		m_Height,
		1,
		Display::E_Usage_Colour,
		m_ColourFormat,
		m_Multisample,
		&surfaceParams,
		&m_ColourRenderTexture))
	{
		LOGFATAL(Display::g_Log, "Failed to create ui colour render texture\n");
		return false;
	}

	#if TRACK_DISPLAY2_RESOURCES
	Display2::Device()->SetRenderTexture2DName(&m_ColourRenderTexture, "_UiRenderBufferRT");
	#endif
#endif

#if PLATFORM_PS3
	U32 depthStencilSurfaceFlags = Display::E_Usage_DepthStencil;
	U32 depthStencilSurfaceSize = 0;
	U8* pDepthStencilSurfaceAddress = NULL;
	const Display::C_Surface* pDepthStencilSurface = Display::Device()->GetBackBuffer().GetDepthStencilSurface();
	if(adjustedWidth == pDepthStencilSurface->GetWidth() && adjustedHeight == pDepthStencilSurface->GetHeight()) // Only reuse memory if surfaces the same width and height (assuming same size format)
	{
		depthStencilSurfaceFlags |= Display::E_Usage_Indirect;
		depthStencilSurfaceSize = pDepthStencilSurface->GetSize();
		pDepthStencilSurfaceAddress = pDepthStencilSurface->GetAddress();
	}
#endif

	if(!Display::Device()->CreateDepthStencilSurface(
		adjustedWidth,
		adjustedHeight,
		0,
		m_DepthStencilFormat,
		m_Multisample,
		&surfaceParams,
#if PLATFORM_PS3
		&m_DepthStencilSurface,
		pDepthStencilSurfaceAddress,
		depthStencilSurfaceSize))
#else
		&m_DepthStencilSurface))
#endif
	{
		LOGFATAL(Display::g_Log, "Failed to create ui depth stencil surface AA \n");
		return false;
	}

	#if TRACK_DISPLAY2_RESOURCES
	Display2::Device()->SetSurfaceName(&m_DepthStencilSurface, "_UiRenderBufferDS");
	#endif

	return true;
}

void C_UiRenderBuffer::OnDestroyVolatile()
{
#if PLATFORM_XENON
	Display::Device()->BlockUntilIdle();

	Display::Device()->Destroy(&m_ColourSurface);
	Display::Device()->Destroy(&m_ColourTexture);
#else
	Display::Device()->Destroy(&m_ColourRenderTexture);
#endif
	Display::Device()->Destroy(&m_DepthStencilSurface);
}

bool C_UiRenderBuffer::Bind()
{
	assert(!m_Bound);

#if PLATFORM_XENON
	Display::Device()->SetColourDepthStencilSurface( &m_ColourSurface, &m_DepthStencilSurface );
	Display::Device()->BeginTiling(m_TileSetup.m_TileCount, m_TileSetup.m_Tiles);
#else
	Display::Device()->SetColourDepthStencilSurface(&m_ColourRenderTexture.GetSurface(), &m_DepthStencilSurface );
#endif
	
	m_Bound = true;

	return true;
}

void C_UiRenderBuffer::Unbind( const bool bDoResolve )
{
#if PLATFORM_XENON
	if( bDoResolve )
	{
		for(U32 tileIndex=0; tileIndex<m_TileSetup.m_TileCount; tileIndex++)
		{
			Display::Device()->GetD3DDevice()->SetPredication(D3DPRED_TILE(tileIndex));

			D3DRECT* pSrcRect = &m_TileSetup.m_Tiles[tileIndex];

			D3DPOINT* pDstPoint = (D3DPOINT*)pSrcRect;
			
			Display::Device()->Resolve( D3DRESOLVE_RENDERTARGET0, &m_ColourTexture, pSrcRect, pDstPoint);
		}
	}

	Display::Device()->GetD3DDevice()->SetPredication(D3DPRED_TILE(0));

	Display::Device()->EndTiling(NULL);
#endif

	m_Bound = false;
}


void C_UiRenderBuffer::FinalResolve()
{
#if PLATFORM_XENON
	Display::Device()->Resolve( Display::E_Resolve_Colour, &m_ColourTexture );
#endif
}

} // namespace UI

#if !defined(CFG_GOLD) && defined(USER_LLWRIGHT)
#pragma optimize("", on)
#endif
