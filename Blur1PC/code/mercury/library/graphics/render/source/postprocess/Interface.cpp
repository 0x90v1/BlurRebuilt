#include "StdAfx.h"
#include "Render/PostProcess/Interface.h"

#include "Render/PostProcess/Manager.h"

using namespace Display;

namespace PostProcess
{

I_PostProcess::I_PostProcess(C_Manager& manager) : m_Manager(manager), m_pShader(NULL), m_TextureFilter(Display::E_TextureFilter_Linear), m_eResolve(E_Resolve_True), m_eType(E_Type_Intermediate), m_eChainable(E_Chainable_True), m_bDepthEnable(false)
{
	m_Manager.AddPostProcess(this);
} 

I_PostProcess::~I_PostProcess()
{
	m_Manager.RemovePostProcess(this);
}

void I_PostProcess::DisplayFinal(const Display::C_PixelShader* pPixelShader, Display::E_TextureFilter textureFilter) const
{
	// if this is the final post process we tell the manager, this will set up our final viewports correctly
	if(m_eType == E_Type_Final)
	{
		m_Manager.RestoreColourSurface();
		m_Manager.RestoreViewport();
		m_Manager.SetFinalQuadSettings();
		DisplayIntermediate(pPixelShader, textureFilter);
		m_Manager.SetActive(false);
	}
	else
		DisplayIntermediate(pPixelShader, textureFilter);
}

void I_PostProcess::DisplayIntermediate(const Display::C_PixelShader* pPixelShader, Display::E_TextureFilter textureFilter) const
{
#ifdef PLATFORM_XENON
	Device()->SetBlendHighPrecisionEnable(E_Bool_False);
#endif
	
	Display::Device()->SetTextureAddressUVW(0, Display::E_TextureAddress_Clamp, Display::E_TextureAddress_Clamp, Display::E_TextureAddress_Clamp);
	Display::Device()->SetTextureGamma(0, Display::E_Bool_False);
	Display::Device()->SetDepthEnable(Display::E_Bool_False);

	if(textureFilter != Display::E_TextureFilter_Unknown)
		Display::Device()->SetTextureFilter(0, textureFilter);
	else
		Display::Device()->SetTextureFilter(0, m_TextureFilter);
	
	Display::Device()->SetBlendMode(Display::E_BlendMode_Solid);
	Display::Device()->SetVertexShader(&VertexShader_XY::m_VertexShader);
	if(pPixelShader)
		Display::Device()->SetPixelShader(pPixelShader);
	else
		Display::Device()->SetPixelShader(m_pShader);
	
	if(m_bDepthEnable)
		Display::Quad::DrawFullScreenDepth();
	else
		Display::Quad::DrawFullScreen();

#ifdef PLATFORM_XENON
	Device()->SetBlendHighPrecisionEnable(E_Bool_True);
#endif
}

// resolves the current rendertarget into m_pRenderTexture and restores the viewport for this view if not chainable
void I_PostProcess::SetViewport() const
{
	// resolve the current render target if necessary
	if(m_eResolve == E_Resolve_True)
		Resolve();

	// if this post process doesn't chain from the previous restore the full viewport for the current view
	if(m_eChainable == E_Chainable_False)
	{
		m_Manager.SetViewport();
	}

	// store the current render target as the chain render target
	m_Manager.SetChainTexture();
}

void I_PostProcess::Resolve() const
{
	m_Manager.ResolveRenderTexture();
}

}; // namespace PostProcess
