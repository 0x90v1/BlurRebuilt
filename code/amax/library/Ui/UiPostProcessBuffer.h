
#ifndef UI_POSTPROCESSBUFFER_H
#define UI_POSTPROCESSBUFFER_H

#ifdef _MSC_VER
#pragma once
#endif

namespace UI
{

class C_UiPostProcessBuffer : public Display::I_Renderer
{
public:
										C_UiPostProcessBuffer();
										~C_UiPostProcessBuffer();

	bool 								OnCreateVolatile();
	void 								OnDestroyVolatile();

	bool								Bind();
	void								Unbind( const bool bDoResolve = true );

	void 								SetWidth( const U32 uWidth )
										{
											m_Width = uWidth;
										}

	void 								SetHeight( const U32 uHeight )
										{
											m_Height = uHeight;
										}

	void								SetColourFormat( const Display::E_Format cf )
										{
											m_ColourFormat = cf;
										}
	
	void								SetMultiSample( const Display::E_Multisample ms )	
										{
											m_Multisample = ms; 
										}

	void								SetUseDepth( const bool ud )
										{
											m_UseDepth = ud;
										}

	U32									GetWidth() const 
										{ 
											return m_Width; 
										}

	U32									GetHeight() const 
										{
											return m_Height; 
										}
#if PLATFORM_XENON
	Display::C_Texture2D&				GetTexture() 
										{ 
											return m_ColourTexture; 
										}

	const Display::C_Texture2D&			GetTexture() const 
										{ 
											return m_ColourTexture; 
										}
#else
	Display::C_Texture2D&				GetTexture() 
										{ 
											return m_ColourRenderTexture.GetTexture(); 
										}

	const Display::C_Texture2D&			GetTexture() const 
										{ 
											return m_ColourRenderTexture.GetTexture(); 
										}
#endif

	Display::C_Texture2D&				GetDepthTexture() 
										{ 
											return m_DepthStencilTexture; 
										}

	const Display::C_Texture2D&			GetDepthTexture() const 
										{ 
											return m_DepthStencilTexture; 
										}
private:
	bool								m_Bound;
	bool								m_UseDepth;
	U32									m_Width;
	U32									m_Height;

	Display::E_Multisample				m_Multisample;
	Display::E_Format					m_ColourFormat;
	Display::E_Format					m_DepthStencilFormat;

	Display::C_Surface					m_DepthStencilSurface;
	Display::C_Texture2D				m_DepthStencilTexture;

#ifdef PLATFORM_XENON
	Display::C_Surface					m_ColourSurface;
	Display::C_Texture2D				m_ColourTexture;
#else
	Display::C_RenderTexture2D			m_ColourRenderTexture;
#endif
};

};

#endif // UI_POSTPROCESSBUFFER_H