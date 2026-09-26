#include "stdafx.h"
#include "Render/Render/CubeTextureRender.h"

#ifdef CFG_ALLOW_OPTIMIZE_OFF
//#pragma optimize( "", off ) // CFG_ALLOW_OPTIMIZE_OFF
#endif

namespace Render
{

Debug::Float	g_debugCubeMapRenderScale	("Graphics.Render.Misc.CubeMapRender.Scale", 200.0f);
Debug::Bool		g_debugCubeMapClearColour	("Graphics.Render.Misc.CubeMapRender.ClearColour", false);
#ifdef PLATFORM_XENON
Debug::Bool		g_debugCubeMapResolveClearOptimitation	("Graphics.Render.Misc.CubeMapRender.ResolveClearOptimitation", true);
#endif

C_CubeTextureRender::C_CubeTextureRender(const U32 _width, Display::E_Multisample multiSample) : 
Display::I_Renderer("CubeTextureRender"),
m_Width(_width),
m_MultiSample (multiSample)
{
}

C_CubeTextureRender::~C_CubeTextureRender()
{
	DestroyRenderer();
}

const Display::C_TextureCube* C_CubeTextureRender::GetTexture()
{
	return &m_RenderTexture.GetTexture();
}

bool C_CubeTextureRender::OnCreateVolatile()
{
	const Display::C_BackBuffer& backBuffer = Display::Device()->GetBackBuffer();

	#ifdef PLATFORM_XENON
	const Display::E_Format format = Display::E_Format_A8R8G8B8;
	#else
	const Display::E_Format format = backBuffer.GetColourFormat();
	#endif

	Display::C_SurfaceParams surfaceParams;

	if(!Display::Device()->CreateRenderTextureCube(m_Width, 1, Display::E_Usage_Colour, format, m_MultiSample, &surfaceParams, &m_RenderTexture))
		return false;
/*
#ifdef PLATFORM_XENON
	if(m_RenderTexture.m_Texture.Get())
	{
		// CUBEMAP SRGB
		m_RenderTexture.m_Texture.Get()->Format.SignX = 3;
		m_RenderTexture.m_Texture.Get()->Format.SignY = 3;
		m_RenderTexture.m_Texture.Get()->Format.SignZ = 3;
	}
#endif*/

	#ifdef PLATFORM_XENON
	surfaceParams.AfterSurface(&m_RenderTexture.GetSurface(0));
	#endif

	if(!Display::Device()->CreateDepthStencilSurface(m_Width, m_Width, Display::E_Usage_DepthStencil, backBuffer.GetDepthStencilFormat(), m_MultiSample, &surfaceParams, &m_DepthStencilSurface))
		return false;

	return true;
}

void C_CubeTextureRender::OnDestroyVolatile()
{
	Display::Device()->Destroy(&m_RenderTexture);
	Display::Device()->Destroy(&m_DepthStencilSurface);
}

void C_CubeTextureRender::Bind(const Display::E_CubeTextureFace face)
{
	assert(face < Display::E_CubeTextureFace_Count);

	Display::Device()->SetColourDepthStencilSurface(&m_RenderTexture.GetSurface(face), &m_DepthStencilSurface);
	
	//In XBox only clear the first time
	#ifdef PLATFORM_XENON
	if ((face == Display::E_CubeTextureFace_PosX) || !g_debugCubeMapResolveClearOptimitation || g_debugCubeMapClearColour)
	#endif
		if (!g_debugCubeMapClearColour)
			Display::Device()->Clear(Display::E_Clear_Depth, Maths::RGBA(255, 0, 0, 255));
		else
			Display::Device()->Clear(Display::E_Clear_Colour | Display::E_Clear_Depth, Maths::RGBA(255, 0, 0, 255));

	Display::Device()->SetBlendMode(Display::E_BlendMode_Solid);
}

void C_CubeTextureRender::Unbind(const Display::E_CubeTextureFace face)
{
	assert(face < Display::E_CubeTextureFace_Count);

	#ifdef PLATFORM_XENON
	D3DRECT rect;
	rect.x1 = 0;
	rect.y1 = 0;
	rect.x2 = m_Width;
	rect.y2 = m_Width;
	if (g_debugCubeMapResolveClearOptimitation && !g_debugCubeMapClearColour)
		Display::Device()->GetD3DDevice()->Resolve(D3DRESOLVE_RENDERTARGET0 | D3DRESOLVE_ALLFRAGMENTS | D3DRESOLVE_CLEARDEPTHSTENCIL, &rect, m_RenderTexture.GetTexture().GetGPUTexture(), NULL, 0, Display::D3D9CubeTextureFace(face), NULL, 0.f, 0, NULL);
	else
		Display::Device()->GetD3DDevice()->Resolve(D3DRESOLVE_RENDERTARGET0 | D3DRESOLVE_ALLFRAGMENTS, &rect, m_RenderTexture.GetTexture().GetGPUTexture(), NULL, 0, Display::D3D9CubeTextureFace(face), NULL, 0.f, 0, NULL);
	#endif

	Display::Device()->SetView(NULL, false);
}

#ifdef PLATFORM_XENON
Display::C_Texture* D3DGetCubeTexture(const U32 face, IDirect3DCubeTexture9* pd3dTexture)
{
	static IDirect3DTexture9 d3dTextures[6];
	D3DSURFACE_DESC d3dDesc;
	pd3dTexture->GetLevelDesc(0, &d3dDesc);
	DWORD d3dOffset = XGGetMipLevelOffset(pd3dTexture, face, 0);
	XGSetTextureHeader(d3dDesc.Width, d3dDesc.Height, 1, 0, d3dDesc.Format, (D3DPOOL)0, (pd3dTexture->Format.BaseAddress<<12) + d3dOffset, 0, 0, (LPDIRECT3DTEXTURE9) &d3dTextures[face], NULL, NULL);

	static Display::C_Texture2D textures[6];
	textures[face].m_d3dTexture = d3dTextures[face];

	return &textures[face];
}
#endif // PLATFORM_XENON

void C_CubeTextureRender::DebugRender()
{
	if(!m_DepthStencilSurface.IsCreated())
		return;
	if(!m_RenderTexture.IsCreated())
		return;

	F32 s = g_debugCubeMapRenderScale;
	F32 x = 512.0f - (s/2.0f);
	F32 y = 384.0f - (s/2.0f);

	#ifdef PLATFORM_XENON
	IDirect3DCubeTexture9* pd3dTexture = m_RenderTexture.GetTexture().GetGPUTextureCube();
	{
		Display::C_Float2 uv[4] =
		{
			Display::C_Float2(1.0f, 0.0f), Display::C_Float2(1.0f, 1.0f),
			Display::C_Float2(0.0f, 0.0f), Display::C_Float2(0.0f, 1.0f),
		};
		Display::BasicRender()->DrawTextured2DRect(x+s, y, s, s, uv, Maths::RGBA(255, 255, 255, 255), D3DGetCubeTexture(D3DCUBEMAP_FACE_POSITIVE_X, pd3dTexture));
	}
	{
		Display::C_Float2 uv[4] =
		{
			Display::C_Float2(0.0f, 0.0f), Display::C_Float2(1.0f, 0.0f),
			Display::C_Float2(0.0f, 1.0f), Display::C_Float2(1.0f, 1.0f),
		};
		Display::BasicRender()->DrawTextured2DRect(x, y, s, s, uv, Maths::RGBA(255, 255, 255, 255), D3DGetCubeTexture(D3DCUBEMAP_FACE_POSITIVE_Y, pd3dTexture));
	}
	{
		Display::C_Float2 uv[4] =
		{
			Display::C_Float2(0.0f, 0.0f), Display::C_Float2(1.0f, 0.0f),
			Display::C_Float2(0.0f, 1.0f), Display::C_Float2(1.0f, 1.0f),
		};
		Display::BasicRender()->DrawTextured2DRect(x, y+s, s, s, uv, Maths::RGBA(255, 255, 255, 255), D3DGetCubeTexture(D3DCUBEMAP_FACE_POSITIVE_Z, pd3dTexture));
	}
	{
		Display::C_Float2 uv[4] =
		{
			Display::C_Float2(0.0f, 1.0f), Display::C_Float2(0.0f, 0.0f),
			Display::C_Float2(1.0f, 1.0f), Display::C_Float2(1.0f, 0.0f),
		};
		Display::BasicRender()->DrawTextured2DRect(x-s, y, s, s, uv, Maths::RGBA(255, 255, 255, 255), D3DGetCubeTexture(D3DCUBEMAP_FACE_NEGATIVE_X, pd3dTexture));
	}
	{
		Display::C_Float2 uv[4] =
		{
			Display::C_Float2(1.0f, 1.0f), Display::C_Float2(0.0f, 1.0f),
			Display::C_Float2(1.0f, 0.0f), Display::C_Float2(0.0f, 0.0f),
		};
		Display::BasicRender()->DrawTextured2DRect(x+s+s, y, s, s, uv, Maths::RGBA(255, 255, 255, 255), D3DGetCubeTexture(D3DCUBEMAP_FACE_NEGATIVE_Y, pd3dTexture));
	}
	{
		Display::C_Float2 uv[4] =
		{
			Display::C_Float2(1.0f, 1.0f), Display::C_Float2(0.0f, 1.0f),
			Display::C_Float2(1.0f, 0.0f), Display::C_Float2(0.0f, 0.0f),
		};
		Display::BasicRender()->DrawTextured2DRect(x, y-s, s, s, uv, Maths::RGBA(255, 255, 255, 255), D3DGetCubeTexture(D3DCUBEMAP_FACE_NEGATIVE_Z, pd3dTexture));
	}
	#endif // PLATFORM_XENON
}
} // namespace Render
