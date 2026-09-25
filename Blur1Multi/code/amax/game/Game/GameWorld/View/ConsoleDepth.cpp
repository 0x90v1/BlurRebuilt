// [Blur1Multi] Scene depth for console sources (T076).
//
// Blob shadows, drop shadows and the other readers of Render::g_DepthStencilBuffer sample
// post-projection depth. The consoles bound the hardware depth buffer itself for them
// (render/source/utils/Globals.cpp: the 360 resolved depth; the PS3 read its D24 depth as
// RGBA8 bytes, render/shaders/psCommon.h SampleDepthBuffer). The PC build cannot sample a
// D3D9 depth buffer; its prepass writes clip z / clip w into an R32F target 1, and
// View/Main.cpp copies that into the global buffer. A console source runs the console
// lighting prepass (Graphics/Lighting/Prepass.cpp overlay), which has no target 1 but packs
// clip w / g_vNearFar.w into the z and w bytes of target 0 (Shaders/Common/Pixel.h,
// ENABLE_PREPASS_VIEWDEPTH). This pass turns that back into clip z / clip w with the view's
// own projection, so the PC depth readers get what they expect.
//
// Not a console mechanism: a bridge between the console prepass layout and the PC depth
// readers, exact up to the prepass's 16-bit depth packing (1/65536 of the prepass draw
// distance). docs/analysis.md "Console sources: lighting prepass ...".

#include "StdAfx.h"

#include "ConsoleDepth.h"

#include <d3d9.h>
#include <d3dcompiler.h>
#include <math.h>
#include <string.h>

namespace ConsoleDepth
{
	namespace
	{
		typedef HRESULT(WINAPI* T_D3DCompile)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*, ID3DInclude*,
			LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);

		const char* const kVertexShader =
			"float4 main(float4 p : POSITION) : POSITION { return p; }\n";

		// c0 = (a, b, depthScale, 0), c1 = (1 / width, 1 / height, 0, 0)
		const char* const kPixelShader =
			"sampler2D G : register(s0);\n"
			"float4 c0 : register(c0);\n"
			"float4 c1 : register(c1);\n"
			"float4 main(float2 vpos : VPOS) : COLOR\n"
			"{\n"
			"	float4 g = tex2D(G, (vpos + 0.5) * c1.xy);\n"
			"	if (g.z >= 1.0 && g.w >= 1.0)\n"
			"		return 1.0;\n"
			"	float w = (g.z + g.w / 255.0) * c0.z;\n"
			"	return (c0.x + c0.y / max(w, 1e-6)).xxxx;\n"
			"}\n";

		IDirect3DVertexShader9* s_pVS = NULL;
		IDirect3DPixelShader9* s_pPS = NULL;
		IDirect3DVertexDeclaration9* s_pDecl = NULL;
		IDirect3DDevice9* s_pOwner = NULL;
		bool s_failed = false;

		bool Compile(T_D3DCompile compile, const char* source, const char* profile, ID3DBlob** ppCode)
		{
			ID3DBlob* pErrors = NULL;
			const HRESULT hr = compile(source, strlen(source), "ConsoleDepth", NULL, NULL, "main", profile, 0, 0, ppCode, &pErrors);
			if (pErrors)
				pErrors->Release();
			return SUCCEEDED(hr);
		}

		bool Create(IDirect3DDevice9* pDevice)
		{
			if (s_pOwner == pDevice && s_pVS && s_pPS && s_pDecl)
				return true;
			if (s_failed)
				return false;
			static T_D3DCompile s_pCompile = NULL;
			if (!s_pCompile)
			{
				HMODULE h = LoadLibraryA("d3dcompiler_47.dll");
				if (h)
					s_pCompile = (T_D3DCompile)GetProcAddress(h, "D3DCompile");
			}
			ID3DBlob* pVS = NULL;
			ID3DBlob* pPS = NULL;
			bool ok = s_pCompile && Compile(s_pCompile, kVertexShader, "vs_3_0", &pVS) && Compile(s_pCompile, kPixelShader, "ps_3_0", &pPS);
			if (ok)
				ok = SUCCEEDED(pDevice->CreateVertexShader((const DWORD*)pVS->GetBufferPointer(), &s_pVS)) &&
					SUCCEEDED(pDevice->CreatePixelShader((const DWORD*)pPS->GetBufferPointer(), &s_pPS));
			if (pVS) pVS->Release();
			if (pPS) pPS->Release();
			const D3DVERTEXELEMENT9 elements[] =
			{
				{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
				D3DDECL_END()
			};
			if (ok)
				ok = SUCCEEDED(pDevice->CreateVertexDeclaration(elements, &s_pDecl));
			if (!ok)
			{
				s_failed = true;
				return false;
			}
			s_pOwner = pDevice;
			return true;
		}
	}

	void ProjectionTerms(const float m[4][4], float& a, float& b)
	{
		// clip = (x, y, z, 1) * m: the depth coordinate is the row with the largest clip-w term
		int k = 2;
		for (int i = 0; i < 3; ++i)
			if (fabsf(m[i][3]) > fabsf(m[k][3]))
				k = i;
		const float wk = m[k][3];
		if (wk == 0.0f)
		{
			a = 0.0f;
			b = 0.0f;
			return;
		}
		// w = t * m[k][3] + m[3][3], z = t * m[k][2] + m[3][2]
		a = m[k][2] / wk;
		b = m[3][2] - m[k][2] * m[3][3] / wk;
	}

	bool Resolve(IDirect3DDevice9* pDevice, IDirect3DTexture9* pGBuffer, IDirect3DTexture9* pDst,
		float a, float b, float depthScale)
	{
		if (!pDevice || !pGBuffer || !pDst || !Create(pDevice))
			return false;

		D3DSURFACE_DESC desc;
		IDirect3DSurface9* pDstSurface = NULL;
		if (FAILED(pDst->GetLevelDesc(0, &desc)) || FAILED(pDst->GetSurfaceLevel(0, &pDstSurface)))
			return false;

		// render targets and the depth surface are not part of a state block
		IDirect3DStateBlock9* pState = NULL;
		pDevice->CreateStateBlock(D3DSBT_ALL, &pState);
		IDirect3DSurface9* pOldTargets[4] = { NULL, NULL, NULL, NULL };
		for (DWORD i = 0; i < 4; ++i)
			pDevice->GetRenderTarget(i, &pOldTargets[i]);
		IDirect3DSurface9* pOldDepth = NULL;
		pDevice->GetDepthStencilSurface(&pOldDepth);

		pDevice->SetRenderTarget(0, pDstSurface);
		for (DWORD i = 1; i < 4; ++i)
			pDevice->SetRenderTarget(i, NULL);
		pDevice->SetDepthStencilSurface(NULL);
		const D3DVIEWPORT9 viewport = { 0, 0, desc.Width, desc.Height, 0.0f, 1.0f };
		pDevice->SetViewport(&viewport);

		pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
		pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
		pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
		pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

		pDevice->SetTexture(0, pGBuffer);
		pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		pDevice->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);

		const float c0[4] = { a, b, depthScale, 0.0f };
		const float c1[4] = { 1.0f / (float)desc.Width, 1.0f / (float)desc.Height, 0.0f, 0.0f };
		pDevice->SetPixelShaderConstantF(0, c0, 1);
		pDevice->SetPixelShaderConstantF(1, c1, 1);
		pDevice->SetVertexDeclaration(s_pDecl);
		pDevice->SetVertexShader(s_pVS);
		pDevice->SetPixelShader(s_pPS);

		const float quad[4][4] =
		{
			{ -1.0f, -1.0f, 0.5f, 1.0f },
			{ -1.0f,  1.0f, 0.5f, 1.0f },
			{  1.0f, -1.0f, 0.5f, 1.0f },
			{  1.0f,  1.0f, 0.5f, 1.0f },
		};
		const bool drawn = SUCCEEDED(pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0])));

		pDevice->SetDepthStencilSurface(pOldDepth);
		for (DWORD i = 0; i < 4; ++i)
		{
			if (i == 0 || pOldTargets[i])
				pDevice->SetRenderTarget(i, pOldTargets[i]);
			if (pOldTargets[i])
				pOldTargets[i]->Release();
		}
		if (pOldDepth)
			pOldDepth->Release();
		if (pState)
		{
			pState->Apply();
			pState->Release();
		}
		pDstSurface->Release();
		return drawn;
	}
}
