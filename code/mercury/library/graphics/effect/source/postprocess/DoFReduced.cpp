#include "stdafx.h"
#include "Effect/PostProcess/DoFReduced.h"

#include "Render/PostProcess/Util.h"
#include "Render/Shaders/Constants.h"
#include "Render/State/RenderState.h"
#include "Render/Utils/Globals.h"
#include "System/Misc/Random.h"

EXTERN_SHADER(psDoFReducedBlur);
EXTERN_SHADER(psDoFReducedComposite);
EXTERN_SHADER(psDoFReducedDownsample);

namespace PostProcess
{

extern Debug::Bool g_debugDepthOfField;
	
extern Util::C_Random g_Random;

extern Debug::Logger& g_Log;

C_DoFReduced::C_DoFReduced(C_Manager& manager) : I_PostProcess(manager), m_bEnabled(true), m_bComposite(true), m_pTextureBuffer(0), m_iTextureBufferSize(0)
{
}

C_DoFReduced::~C_DoFReduced()
{
}

bool C_DoFReduced::Create()
{
	if (!Display::CreatePixelShaderFromFunc("Shaders/psDoFReducedBlur", 0, &m_BlurShader, REF_SHADER(psDoFReducedBlur)))
		return false;
	if (!Display::CreatePixelShaderFromFunc("Shaders/psDoFReducedComposite", 0, &m_CompositeShader, REF_SHADER(psDoFReducedComposite)))
		return false;
	if (!Display::CreatePixelShaderFromFunc("Shaders/psDoFReducedDownsample", 0, &m_DownsampleShader, REF_SHADER(psDoFReducedDownsample)))
		return false;

	m_BlurTapsParameter				= m_BlurShader.GetParameterArray(0, "g_vTaps", 6);
	m_BlurDistancesParameter		= m_BlurShader.GetParameterArray(6, "g_vTapDistances", 3);
	m_BlurDepthParameter			= m_BlurShader.GetParameter(9, "g_vDepth");

	m_DownsampleCameraParameter		= m_DownsampleShader.GetParameter(CP_RENDER_SETTINGS, "g_vCamera");
	m_DownsamplePixelSizeParameter	= m_DownsampleShader.GetParameter(CP_RENDER_SETTINGS_1, "g_vPixelSize");
	m_DownsampleDepthParameter		= m_DownsampleShader.GetParameter(CP_RENDER_SETTINGS_2, "g_vDepth");

	// black texture
	{
		if (!Display::Device()->CreateTexture2D(1, 1, 1, Display::E_Usage_Managed, Display::E_Format_A8R8G8B8, &m_BlackTexture))
			return false;

		Display::C_LockedRect rect = m_BlackTexture.Lock(0);
		if(rect.pBits)
		{
			U32 *pData = reinterpret_cast<U32*>(rect.pBits);
			*pData = 0x00000000;
		}
		m_BlackTexture.Unlock(0);
	}

	return true;
}

bool C_DoFReduced::CreateVolatile()
{
	Display::C_SurfaceParams surfaceParams;
#if defined(PLATFORM_XENON)
	const Display::C_BackBuffer& backBuffer = Display::Device()->GetBackBuffer();
	surfaceParams.Base = 0;
	surfaceParams.Base += XGSurfaceSize(backBuffer.GetWidth(), backBuffer.GetHeight(), Display::D3D9Format(backBuffer.GetColourFormat()), Display::D3D9Multisample(backBuffer.GetMultisample()));
	if(m_Manager.GetViewCount() > 1)
		surfaceParams.Base += m_Manager.GetMaximumSurfaceSize();
#endif

#if defined(PLATFORM_XENON)
	const Display::E_Format eFormat = Display::E_Format_A16B16G16R16F;
#else
	const Display::E_Format eFormat = Display::E_Format_Unknown;
#endif

	U32 flags = Display::E_Usage_Colour | Display::E_Usage_ReadWrite | Display::E_Usage_Expand;
	if(m_pTextureBuffer)
	{
		flags |= Display::E_Usage_Indirect;
		const U32 iMaximumTextureSize = m_Manager.GetMaximumTextureSize(2, 2);
		if(iMaximumTextureSize > m_iTextureBufferSize)
		{
			LOGERROR(g_Log, "Texture buffer size not large enough\n");
			return false;
		}
	}

	if(!m_Manager.CreateUserRenderTextures(2, 2, flags, eFormat, &surfaceParams, m_DownsampleRenderTextures, m_pTextureBuffer, m_iTextureBufferSize))
		return false;

	return true;
}

void C_DoFReduced::Destroy()
{
	Display::Device()->Destroy(&m_BlackTexture);
	Display::Device()->Destroy(&m_BlurShader);
	Display::Device()->Destroy(&m_CompositeShader);
	Display::Device()->Destroy(&m_DownsampleShader);
}

void C_DoFReduced::DestroyVolatile()
{
	m_Manager.DestroyUserRenderTextures(m_DownsampleRenderTextures);
}

void C_DoFReduced::SetTextureBuffer(U8* pTextureBuffer, const U32 iTextureBufferSize)
{
	m_pTextureBuffer = pTextureBuffer;
	m_iTextureBufferSize = iTextureBufferSize;
}

U32 C_DoFReduced::GetTextureBufferRequiredSize() const
{
#if defined(PLATFORM_PS3)
	return m_Manager.GetMaximumTextureSize(2, 2) * 2;
#else
	return m_Manager.GetMaximumTextureSize(2, 2);
#endif
}

void C_DoFReduced::Display() const
{
	if(!g_debugDepthOfField || !m_Manager.Enabled() || !m_bEnabled)
		return;

	const S32 iRenderTarget = m_Manager.GetRenderTargetIndex();
	if(iRenderTarget < 0 || iRenderTarget > m_DownsampleRenderTextures.Size())
		return;

	PERF_EVENT("DepthOfFieldReduced");

	const U32 iWidth = m_Manager.GetView().GetWidth();
	const U32 iHeight = m_Manager.GetView().GetHeight();

	Display::Device()->SetColourWriteEnable(Display::E_ColourWriteEnable_All);

#if defined(PLATFORM_XENON)
	{
		const Display::C_RenderTarget& renderTarget = Display::Device()->GetRenderTarget();
		const Display::C_Surface* pColourSurface = renderTarget.GetColourSurface();

		U32 size = 0;

		XGTEXTURE_DESC xgColourSurfaceDesc;
		XGGetSurfaceDesc((D3DSurface*)&pColourSurface->m_d3dSurface, &xgColourSurfaceDesc);

		static D3DSurface d3dColourSurface;

		D3DSURFACE_PARAMETERS d3dSurfaceParams;
		d3dSurfaceParams.Base = pColourSurface->m_d3dSurface.ColorInfo.ColorBase;
		d3dSurfaceParams.ColorExpBias = pColourSurface->m_d3dSurface.ColorInfo.ColorExpBias;
		d3dSurfaceParams.HierarchicalZBase = 0;
		d3dSurfaceParams.HiZFunc = D3DHIZFUNC_DEFAULT;

		size =  XGSetSurfaceHeader(xgColourSurfaceDesc.Width / 2,
									xgColourSurfaceDesc.Height / 2,
									xgColourSurfaceDesc.Format,
									D3DMULTISAMPLE_4_SAMPLES,
									&d3dSurfaceParams,
									&d3dColourSurface,
									0);
		XGOffsetSurfaceAddress(&d3dColourSurface, 0, 0);

		void* pTexture = m_DownsampleRenderTextures[iRenderTarget]->GetTexture().GetGPUTexture();
		Display::Device()->SetColourDepthStencilSurface(0, 0);
		Display::Device()->GetD3DDevice()->SetRenderTarget(0, &d3dColourSurface);
		Display::Device()->GetD3DDevice()->Resolve(D3DRESOLVE_RENDERTARGET0 | D3DRESOLVE_ALLFRAGMENTS, 0, (D3DBaseTexture*)pTexture, 0, 0, 0, 0, 0.0f, 0, 0);
	}
#endif

	// downsample the main colour buffer
	{
		Display::C_RenderTexture2D* pRenderTexture = m_DownsampleRenderTextures[iRenderTarget];

		Display::Device()->SetColourDepthStencilSurface(&pRenderTexture->GetSurface(), 0);
		Display::C_Viewport viewport;
		viewport.m_W = pRenderTexture->GetSurface().GetWidth();
		viewport.m_H = pRenderTexture->GetSurface().GetHeight();
		Display::Device()->SetViewport(viewport);

		const F32 fFarClip			= Display::Device()->GetViewSettings3D().GetFarClip();
		const F32 fPlaneInFocus		= GetPlaneInFocus(m_Manager.GetViewIndex());
		const F32 fAperture			= GetAperture(m_Manager.GetViewIndex()) * abs(g_debugDoFFocalLength / (fPlaneInFocus - g_debugDoFFocalLength));
		
		const F32 fK			= g_debugDoFMinCoC / (fAperture + 0.00001f);
		const F32 fOnePlusK		= 1.0f + fK;
		const F32 fOneMinusK	= (1.0f - fK > 0.0f) ? 1.0f - fK : 0.00001f;

		const F32 fFocus0			= fPlaneInFocus / fOneMinusK;
		const F32 fFocus1			= fPlaneInFocus / fOnePlusK;
		const F32 fFocusNear		= (fFocus0 < fFocus1) ? fFocus0 : fFocus1;
		const F32 fFocusFar			= (fFocus0 > fFocus1) ? fFocus0 : fFocus1;

		const F32 fFocusPlane		= (fFocusNear + fFocusFar) * 0.5f;
		const F32 fFocusSize		= (fFocusFar - fFocusNear) * 0.5f;

		const Display::C_Float4 vCamera(fFocusPlane, fFocusSize, 0.0f, 0.0f);
		const Display::C_Float4 vPixelSize(1.0f / (F32)iWidth, 1.0f / (F32)iHeight, 0.0f, 0.0f);
		const Display::C_Float4 vDepth = Render::C_RenderState::Instance().GetDepthScaleShaderParameter();
		Display::Device()->SetPixelShaderParameterF(m_DownsampleCameraParameter, (F32*)&vCamera);
		Display::Device()->SetPixelShaderParameterF(m_DownsamplePixelSizeParameter, (F32*)&vPixelSize);
		Display::Device()->SetPixelShaderParameterF(m_DownsampleDepthParameter, (F32*)&vDepth);
		Display::Device()->SetPixelShader(&m_DownsampleShader);

#if defined(PLATFORM_XENON)
		{
			Display::C_Texture2D* pTexture = const_cast<Display::C_Texture2D*>(&pRenderTexture->GetTexture());
			IDirect3DTexture9* pD3DTexture = (IDirect3DTexture9*)pTexture->Get();

			XGTEXTURE_DESC xgTextureDesc;
			XGGetTextureDesc(pD3DTexture, 0, &xgTextureDesc);
			if(xgTextureDesc.Format == D3DFMT_A16B16G16R16F_EXPAND)
				xgTextureDesc.Format = D3DFMT_A16B16G16R16F;

			static IDirect3DTexture9 d3dTexture;
			U32 size =  XGSetTextureHeader(xgTextureDesc.Width, xgTextureDesc.Height, 1, 0, xgTextureDesc.Format, 0, 0, 0, xgTextureDesc.Width * 8, &d3dTexture, 0, 0);

			XGOffsetResourceAddress(&d3dTexture, pTexture->GetAddress());
			Display::Device()->GetD3DDevice()->SetTexture(0, &d3dTexture);
		}
		Display::Device()->SetTextureAddress(0, Display::E_TextureAddress_Clamp);
		Display::Device()->SetTextureFilter(0, Display::E_TextureFilter_Point);
#else
		Display::Device()->SetTexture(0, m_Manager.GetMainTexture());
		Display::Device()->SetTextureAddress(0, Display::E_TextureAddress_Clamp);
		Display::Device()->SetTextureFilter(0, Display::E_TextureFilter_Linear);
#endif
		Display::Device()->SetTexture(1, Render::GetGlobalDepthStencilBuffer());
		Display::Device()->SetTextureAddress(1, Display::E_TextureAddress_Clamp);
		Display::Device()->SetTextureFilter(1, Display::E_TextureFilter_Point);

		Display::Quad::DrawFullScreen();

		pRenderTexture->Resolve();

#if defined(PLATFORM_XENON)
		Display::Device()->SetTexture(0, 0);
		Display::Device()->GetD3DDevice()->SetTexture(0, 0);
#endif
	}

	// blur the downsampled buffer
	{
		Display::C_RenderTexture2D* pRenderTexture = m_DownsampleRenderTextures[iRenderTarget];

		Display::Device()->SetColourDepthStencilSurface(&pRenderTexture->GetSurface(), 0);
		Display::C_Viewport viewport;
		viewport.m_W = pRenderTexture->GetSurface().GetWidth();
		viewport.m_H = pRenderTexture->GetSurface().GetHeight();
		Display::Device()->SetViewport(viewport);

		F32	vTapDistances[12];
		F32	vTaps[24];

		S32 iIndex = 0;
		for (S32 y = -2; y <= 2; ++y)
		{
			for (S32 x = -2; x <= 2; ++x)
			{
				const F32 fX = 0.4f * x;
				const F32 fY = 0.4f * y;
				const F32 fLen = Maths::Clamp01(sqrtf(fX * fX + fY * fY) * (1.0f / sqrtf(1.8f)));

				if(fLen < 0.01f || 0.65f < fLen)
					continue;

				vTaps[iIndex * 2] = x * (1.0f / (F32)pRenderTexture->GetTexture().GetWidth());
				vTaps[iIndex * 2 + 1] = y * (1.0f / (F32)pRenderTexture->GetTexture().GetHeight());

				vTapDistances[iIndex] = fLen;

				iIndex++;
			}
		}

		const Display::C_Float4 vDepth = Render::C_RenderState::Instance().GetDepthScaleShaderParameter();
		Display::Device()->SetPixelShaderParameterArrayF(m_BlurTapsParameter, vTaps);
		Display::Device()->SetPixelShaderParameterArrayF(m_BlurDistancesParameter, vTapDistances);
		Display::Device()->SetPixelShaderParameterF(m_BlurDepthParameter, (F32*)&vDepth);

		Display::Device()->SetPixelShader(&m_BlurShader);

#if defined(PLATFORM_XENON)
		{
			Display::C_Texture2D* pTexture = const_cast<Display::C_Texture2D*>(&pRenderTexture->GetTexture());
			IDirect3DTexture9* pD3DTexture = (IDirect3DTexture9*)pTexture->Get();

			XGTEXTURE_DESC xgTextureDesc;
			XGGetTextureDesc(pD3DTexture, 0, &xgTextureDesc);
			if(xgTextureDesc.Format == D3DFMT_A16B16G16R16F_EXPAND)
				xgTextureDesc.Format = D3DFMT_A16B16G16R16F;

			static IDirect3DTexture9 d3dTexture;
			U32 size =  XGSetTextureHeader(xgTextureDesc.Width, xgTextureDesc.Height, 1, 0, xgTextureDesc.Format, 0, 0, 0, xgTextureDesc.Width * 8, &d3dTexture, 0, 0);

			XGOffsetResourceAddress(&d3dTexture, pTexture->GetAddress());
			Display::Device()->GetD3DDevice()->SetTexture(0, &d3dTexture);
		}
#else
		Display::Device()->SetTexture(0, &pRenderTexture->GetTexture());
#endif
		Display::Device()->SetTextureAddress(0, Display::E_TextureAddress_Clamp);
		Display::Device()->SetTextureFilter(0, Display::E_TextureFilter_Point);

		Display::Quad::DrawFullScreen();

		pRenderTexture->Resolve();		
	}

	// composite
	m_Manager.SetViewport();

	if(m_bComposite)
	{
		Display::Device()->SetTexture(0, &m_DownsampleRenderTextures[iRenderTarget]->GetTexture());
		Display::Device()->SetTextureAddress(0, Display::E_TextureAddress_Clamp);
		Display::Device()->SetTextureFilter(0, Display::E_TextureFilter_Linear);
		Display::Device()->SetTextureGamma(0, Display::E_Bool_False);

		Display::Device()->SetBlendMode(Display::E_BlendMode_Alpha);

		Display::Device()->SetPixelShader(&m_CompositeShader);

		Display::Quad::DrawFullScreen();
	}

	// reset states
	Display::Device()->SetTexture(0, 0);
	Display::Device()->SetTextureAddress(0, Display::E_TextureAddress_Default);
	Display::Device()->SetTextureFilter(0, Display::E_TextureFilter_Default);
	Display::Device()->SetTexture(1, 0);
	Display::Device()->SetTextureAddress(1, Display::E_TextureAddress_Default);
	Display::Device()->SetTextureFilter(1, Display::E_TextureFilter_Default);

	Display::Device()->SetBlendMode(Display::E_BlendMode_Solid);
	Display::Device()->SetColourWriteEnable(Display::E_ColourWriteEnable_Default);
}

Display::C_Texture2D* C_DoFReduced::GetTexture()
{
	const S32 iRenderTarget = m_Manager.GetRenderTargetIndex();
	if(g_debugDepthOfField && iRenderTarget >= 0 && iRenderTarget < m_DownsampleRenderTextures.Size())
		return &m_DownsampleRenderTextures[iRenderTarget]->GetTexture();
	else
		return &m_BlackTexture;
}

}; // namespace PostProcess
