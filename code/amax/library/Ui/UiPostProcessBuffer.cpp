
#include "stdafx.h"

#include "UiPostProcessBuffer.h"

namespace UI
{

C_UiPostProcessBuffer::C_UiPostProcessBuffer() 
:	Display::I_Renderer("UiPostProcessBuffer"),
	m_Bound( false ),
	m_UseDepth( true ),
	m_Width( 0 ),
	m_Height( 0 ),
	m_Multisample( Display::E_Multisample_None ),
	m_ColourFormat( Display::E_Format_A8R8G8B8 ),
#if PLATFORM_PC
	m_DepthStencilFormat( Display::E_Format_D24S8 )
#else
	m_DepthStencilFormat( Display::E_Format_D24FS8 )
#endif
{
}

C_UiPostProcessBuffer::~C_UiPostProcessBuffer()
{
	DestroyRenderer();
}

bool C_UiPostProcessBuffer::OnCreateVolatile()
{
	RESOURCE_MEMORY_ID("UiRenderBuffer");

	Display::C_SurfaceParams surfaceParams;

#if PLATFORM_XENON
	if( !Display::Device()->CreateColourSurface(
		m_Width,
		m_Height,
		0,
		m_ColourFormat,
		m_Multisample,
		&surfaceParams,
		&m_ColourSurface ) )
	{
		LOGFATAL(Display::g_Log, "Failed to create ui colour surface\n");
		return false;
	}

	#if TRACK_DISPLAY2_RESOURCES
	Display2::Device()->SetSurfaceName(&m_ColourSurface, "_UiPostProcessBufferC");
	#endif
	
	surfaceParams.AfterSurface(&m_ColourSurface);
	
	if(!Display::Device()->CreateTexture2D(
		m_Width,
		m_Height,
		1,
		0,
		m_ColourFormat,
		&m_ColourTexture ) )
	{
		LOGFATAL(Display::g_Log, "Failed to create ui texture\n");
		return false;
	}

	#if TRACK_DISPLAY2_RESOURCES
	Display2::Device()->SetTextureName(&m_ColourTexture, "_UiPostProcessBufferColour");
	#endif
#else
	if(!Display::Device()->CreateRenderTexture2D(
		m_Width,
		m_Height,
		1,
		Display::E_Usage_Colour,
		m_ColourFormat,
		m_Multisample,
		&surfaceParams,
		&m_ColourRenderTexture ) )
	{
		LOGFATAL(Display::g_Log, "Failed to create ui texture\n");
		return false;
	}

	#if TRACK_DISPLAY2_RESOURCES
	Display2::Device()->SetRenderTexture2DName(&m_ColourRenderTexture, "_UiPostProcessBufferRT");
	#endif
#endif // PLATFORM_XENON
	if( m_UseDepth )
	{
		if( !Display::Device()->CreateDepthStencilSurface( 
			m_Width,
			m_Height,
			0,
			m_DepthStencilFormat,
			m_Multisample,
			&surfaceParams,
			&m_DepthStencilSurface ) )
		{
			LOGFATAL( Display::g_Log, "Failed to create ui depth stencil surface\n" );
			return false;
		}

		#if TRACK_DISPLAY2_RESOURCES
		Display2::Device()->SetSurfaceName(&m_DepthStencilSurface, "_UiPostProcessBufferDS");
		#endif
	}

#if PLATFORM_PS3
	if( m_UseDepth )
	{
		if ( !Display::Device()->CreateTexture2D(
			m_Width, 
			m_Height, 
			1, 
			Display::E_Usage_Indirect, 
			Display::E_Format_A8R8G8B8, 
			&m_DepthStencilTexture, 
			(U8*)m_DepthStencilSurface.GetAddress(),
			m_DepthStencilSurface.GetSize()) )
		{
			LOGFATAL( Display::g_Log, "Failed to create ui depth stencil texture\n" );
			return false;
		}

		#if TRACK_DISPLAY2_RESOURCES
		Display2::Device()->SetTextureName(&m_DepthStencilTexture, "_UiPostProcessBufferDepthStencil");
		#endif

		m_DepthStencilTexture.m_gcmTexture.remap =
			CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
			CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
			CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
			CELL_GCM_TEXTURE_REMAP_REMAP << 8 |
			CELL_GCM_TEXTURE_REMAP_FROM_G << 6 |
			CELL_GCM_TEXTURE_REMAP_FROM_R << 4 |
			CELL_GCM_TEXTURE_REMAP_FROM_A << 2 |
			CELL_GCM_TEXTURE_REMAP_FROM_B;
	}

#elif PLATFORM_PC
	if( m_UseDepth )
	{
		if ( !Display::Device()->CreateTexture2D(
			m_Width, 
			m_Height, 
			1, 
			0, 
			Display::E_Format_A8R8G8B8, 
			&m_DepthStencilTexture ) )
		{
			LOGFATAL( Display::g_Log, "Failed to create ui depth stencil texture\n" );
			return false;
		}

		#if TRACK_DISPLAY2_RESOURCES
		Display2::Device()->SetTextureName(&m_DepthStencilTexture, "_UiPostProcessBufferDepthStencil");
		#endif
	}
#elif PLATFORM_XENON
	if( m_UseDepth )
	{
		if ( !Display::Device()->CreateTexture2D(
			m_Width, 
			m_Height, 
			1, 
			0, 
			m_DepthStencilFormat, 
			&m_DepthStencilTexture ) )
		{
			LOGFATAL( Display::g_Log, "Failed to create ui depth stencil texture\n" );
			return false;
		}

		#if TRACK_DISPLAY2_RESOURCES
		Display2::Device()->SetTextureName(&m_DepthStencilTexture, "_UiPostProcessBufferDepthStencil");
		#endif
	}
#endif // PLATFORM_PS3

	return true;
}

void C_UiPostProcessBuffer::OnDestroyVolatile()
{
#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
	Display::Device()->BlockUntilIdle();
#endif

#if PLATFORM_XENON
	Display::Device()->Destroy( &m_ColourSurface );
	Display::Device()->Destroy( &m_ColourTexture );
#else
	Display::Device()->Destroy( &m_ColourRenderTexture );
#endif // PLATFORM_XENON

	if( m_UseDepth )
	{
		Display::Device()->Destroy( &m_DepthStencilSurface );
		Display::Device()->Destroy( &m_DepthStencilTexture );
	}
}


bool C_UiPostProcessBuffer::Bind()
{
	BIZ_ASSERT( !m_Bound );

#if PLATFORM_XENON
	Display::Device()->SetColourDepthStencilSurface( &m_ColourSurface, m_UseDepth ? &m_DepthStencilSurface : NULL);
#else
	Display::Device()->SetColourDepthStencilSurface( &m_ColourRenderTexture.GetSurface(), m_UseDepth ? &m_DepthStencilSurface : NULL);
#endif // PLATFORM_XENON

	m_Bound = true;
	return true;
}


void C_UiPostProcessBuffer::Unbind( const bool bDoResolve )
{
	BIZ_ASSERT( m_Bound );

#if PLATFORM_XENON
	if( bDoResolve )
	{
		if( m_UseDepth )
			Display::Device()->Resolve( Display::E_Resolve_DepthStencil,	&m_DepthStencilTexture );

		Display::Device()->Resolve( Display::E_Resolve_Colour,			&m_ColourTexture );
	}
#endif // PLATFORM_XENON

	m_Bound = false;
}


};