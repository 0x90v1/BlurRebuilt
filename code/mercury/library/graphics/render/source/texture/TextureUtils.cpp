#include "stdafx.h"
#include "Render/Texture/TextureUtils.h"
#include "Render/Shaders/Constants.h"
#include "Render/Utils/DebugLog.h"

EXTERN_SHADER(psNormalMap);
EXTERN_SHADER(psGradientToNormal);

namespace Render
{
namespace Texture
{

void C_TextureUtilsShaders::Create()
{
	RESOURCE_MEMORY_ID("TextureUtils");

	Display::CreatePixelShaderFromFunc("Shaders/psNormalMap", 0, &m_psNormalMap, REF_SHADER(psNormalMap));
	Display::CreatePixelShaderFromFunc("Shaders/psGradientToNormal", 0, &m_psGradientToNormal, REF_SHADER(psGradientToNormal));

	m_psNormalMap_Settings = m_psNormalMap.GetParameter(CP_RENDER_SETTINGS, "g_vSettings");
}

void C_TextureUtilsShaders::Destroy()
{
	RESOURCE_MEMORY_ID("TextureUtils");

	Display::Device()->Destroy(&m_psGradientToNormal);
	Display::Device()->Destroy(&m_psNormalMap);
}

S32 C_TextureUtils::m_RefCount = 0;
C_TextureUtilsShaders C_TextureUtils::m_Shaders;

C_TextureUtils::C_TextureUtils()
{
	m_RefCount++;

	if(m_RefCount == 1)
	{
		m_Shaders.Create();
	}
}

C_TextureUtils::~C_TextureUtils()
{
	m_RefCount--;

	if(m_RefCount == 0)
	{
		m_Shaders.Destroy();
	}
}

void C_TextureUtils::CopyTexture(Display::C_RenderTexture2D* pDstTexture, Display::C_RenderTexture2D* pSrcTexture)
{
	PERF_EVENT("CopyTexture");

	if(!pDstTexture || !pSrcTexture)
		return;

	Display::Device()->SetColourDepthStencilSurface(&pDstTexture->GetSurface(), NULL);

	Display::C_Viewport viewport;
	viewport.m_W = pDstTexture->GetTexture().GetWidth();
	viewport.m_H = pDstTexture->GetTexture().GetHeight();
	Display::Device()->SetViewport(viewport);

	Display::Device()->SetVertexShader(&Display::VertexShader_XY::m_VertexShader);
	Display::Device()->SetPixelShader(&Display::PixelShader::m_PixelShaderTexture);
	
	Display::Device()->SetTexture(0, &pSrcTexture->GetTexture());
	Display::Device()->SetTextureFilter(0, Display::E_TextureFilter_Linear);
	Display::Device()->SetTextureAddress(0, Display::E_TextureAddress_Clamp);

	Display::Quad::DrawFullScreen();

	#ifdef PLATFORM_XENON
	D3DRECT rect;
	rect.x1 = 0;
	rect.y1 = 0;
	rect.x2 = viewport.m_W;
	rect.y2 = viewport.m_H;
	Display::Device()->GetD3DDevice()->Resolve(D3DRESOLVE_RENDERTARGET0 | D3DRESOLVE_ALLFRAGMENTS, &rect, pDstTexture->GetTexture().GetGPUTexture(), NULL, 0, 0, NULL, 1.f, 0, NULL);
	#endif

	Display::Device()->SetColourDepthStencilSurface();
}

void C_TextureUtils::GenerateNormalFromGradient(Display::C_RenderTexture2D* pRenderTarget, const Display::C_Texture2D* pTexture)
{
	PERF_EVENT("GenerateNormalFromGradient");

	if(!pRenderTarget || !pTexture || !pTexture->IsCreated())
		return;

	Display::Device()->SetColourDepthStencilSurface(&pRenderTarget->GetSurface(), NULL);

	Display::C_Viewport viewport;
	viewport.m_W = pTexture->GetWidth();
	viewport.m_H = pTexture->GetHeight();
	Display::Device()->SetViewport(viewport);

	Display::Device()->SetVertexShader(&Display::VertexShader_XY::m_VertexShader);
	Display::Device()->SetPixelShader(&m_Shaders.m_psGradientToNormal);

	Display::Device()->SetTexture(0, pTexture);
	Display::Device()->SetTextureFilter(0, Display::E_TextureFilter_Point);
	Display::Device()->SetTextureAddress(0, Display::E_TextureAddress_Wrap);

	Display::Quad::DrawFullScreen();

	#ifdef PLATFORM_XENON
	D3DRECT rect;
	rect.x1 = 0;
	rect.y1 = 0;
	rect.x2 = viewport.m_W;
	rect.y2 = viewport.m_H;
	Display::Device()->GetD3DDevice()->Resolve(D3DRESOLVE_RENDERTARGET0 | D3DRESOLVE_ALLFRAGMENTS, &rect, pRenderTarget->GetTexture().GetGPUTexture(), NULL, 0, 0, NULL, 1.f, 0, NULL);
	#endif
}

void C_TextureUtils::GenerateNormalMap(Display::C_RenderTexture2D* pRenderTarget, Display::C_Texture2D* pTexture, const F32 fSlopeScale)
{
	PERF_EVENT("GenerateNormalMap");

	if(!pRenderTarget || !pTexture || !pTexture->IsCreated())
		return;

	Display::Device()->SetColourDepthStencilSurface(&pRenderTarget->GetSurface(), NULL);

	Display::C_Viewport viewport;
	viewport.m_W = pTexture->GetWidth();
	viewport.m_H = pTexture->GetHeight();
	Display::Device()->SetViewport(viewport);

	Display::Device()->SetVertexShader(&Display::VertexShader_XY::m_VertexShader);
	Display::Device()->SetPixelShader(&m_Shaders.m_psNormalMap);

	Display::Device()->SetPixelShaderParameterF(m_Shaders.m_psNormalMap_Settings, Display::C_PSConstant(1.0f / (F32) pTexture->GetWidth(), 1.0f / (F32) pTexture->GetHeight(), fSlopeScale));

	Display::Device()->SetTexture(0, pTexture);
	Display::Device()->SetTextureFilter(0, Display::E_TextureFilter_Linear);
	Display::Device()->SetTextureAddress(0, Display::E_TextureAddress_Clamp);

	Display::Quad::DrawFullScreen();

	#ifdef PLATFORM_XENON
	D3DRECT rect;
	rect.x1 = 0;
	rect.y1 = 0;
	rect.x2 = viewport.m_W;
	rect.y2 = viewport.m_H;
	Display::Device()->GetD3DDevice()->Resolve(D3DRESOLVE_RENDERTARGET0 | D3DRESOLVE_ALLFRAGMENTS, &rect, pRenderTarget->GetTexture().GetGPUTexture(), NULL, 0, 0, NULL, 1.f, 0, NULL);
	#endif
}

// this currently only works on 360
void C_TextureUtils::BuildMipMaps(const Display::C_Texture2D* pTexture, const bool blurMips)
{
#ifndef PLATFORM_XENON
	LOGWARN(Render::g_Log, "C_TextureUtils::BuildMipMaps is not implemented on this platform");
	return;
#else

	PERF_EVENT("BuildMipMaps");
	
	if(!pTexture || !pTexture->IsCreated())
		return;

	Display::Device()->SetVertexShader(&Display::VertexShader_XY::m_VertexShader);

	if(blurMips)
		Display::Device()->SetPixelShader(&Display::PixelShader::m_PixelShaderTexture2x2);
	else
		Display::Device()->SetPixelShader(&Display::PixelShader::m_PixelShaderTexture);

	Display::Device()->SetTexture(0, pTexture);
	Display::Device()->SetTextureFilterMagMinMip(0, Display::E_TextureFilter_Linear, Display::E_TextureFilter_Linear, Display::E_TextureFilter_Point);
	Display::Device()->SetTextureAddress(0, Display::E_TextureAddress_Wrap);
	
	U32 width = pTexture->GetWidth() / 2;
	U32 height = pTexture->GetHeight() / 2;

	for(U32 i=1; i<pTexture->GetLevels(); i++)
	{
		Display::C_Viewport viewport;
		viewport.m_W = width;
		viewport.m_H = height;
		Display::Device()->SetViewport(viewport);

		#ifdef PLATFORM_XENON
		Display::Device()->GetD3DDevice()->SetSamplerState(0, D3DSAMP_MAXMIPLEVEL, i-1);
		Display::Device()->GetD3DDevice()->SetSamplerState(0, D3DSAMP_MINMIPLEVEL, i-1);
		#endif

		Display::Quad::DrawFullScreen();

		#ifdef PLATFORM_XENON
		Display::Device()->GetD3DDevice()->Resolve(D3DRESOLVE_RENDERTARGET0, NULL, (D3DTexture*)pTexture->GetGPUTexture(), NULL, i, 0, NULL, 1.0f, 0L, NULL );
		#endif

		width /= 2;
		height /= 2;
	}

	#ifdef PLATFORM_XENON
	Display::Device()->GetD3DDevice()->SetSamplerState(0, D3DSAMP_MAXMIPLEVEL, 0);
	Display::Device()->GetD3DDevice()->SetSamplerState(0, D3DSAMP_MINMIPLEVEL, 13);
	#endif

#endif
}

} // namespace Texture
} // namespace Render
