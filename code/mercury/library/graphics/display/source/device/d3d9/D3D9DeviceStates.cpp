#include "stdafx.h"
#include "Display/Device/D3D9/D3D9DeviceStates.h"

#ifdef CFG_ALLOW_OPTIMIZE_OFF
//#pragma optimize( "", off ) // CFG_ALLOW_OPTIMIZE_OFF
#endif

namespace Display
{
namespace D3D9
{

void C_DeviceStates::Get(IDirect3DDevice9* pDevice)
{
	PERF_EVENT("D3DGetStates");

	assert(pDevice);

	pDevice->GetRenderState(D3DRS_ALPHATESTENABLE,		&m_AlphaTestEnable);
	pDevice->GetRenderState(D3DRS_ALPHAFUNC,			&m_AlphaTestFunc);
	pDevice->GetRenderState(D3DRS_ALPHAREF,				&m_AlphaTestRef);
	pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE,		&m_BlendEnable);
#ifdef PLATFORM_XENON // only in DX10 on PC
	pDevice->GetRenderState(D3DRS_ALPHATOMASKENABLE,	&m_AlphaToMaskEnable);
#endif
	pDevice->GetRenderState(D3DRS_SRCBLEND,				&m_BlendSrc);
	pDevice->GetRenderState(D3DRS_DESTBLEND,			&m_BlendDst);
	pDevice->GetRenderState(D3DRS_BLENDOP,				&m_BlendOp);
	pDevice->GetRenderState(D3DRS_SRCBLENDALPHA,		&m_BlendSrcAlpha);
	pDevice->GetRenderState(D3DRS_DESTBLENDALPHA,		&m_BlendDstAlpha);
	pDevice->GetRenderState(D3DRS_BLENDOPALPHA,			&m_BlendOpAlpha);
	pDevice->GetRenderState(D3DRS_COLORWRITEENABLE,		&m_ColourWriteEnable);
	pDevice->GetRenderState(D3DRS_CULLMODE,				&m_CullMode);
	pDevice->GetRenderState(D3DRS_FILLMODE,				&m_FillMode);
	pDevice->GetRenderState(D3DRS_DEPTHBIAS,			&m_DepthBias);
	pDevice->GetRenderState(D3DRS_ZENABLE,				&m_DepthEnable);
	pDevice->GetRenderState(D3DRS_ZFUNC,				&m_DepthFunc);
	pDevice->GetRenderState(D3DRS_ZWRITEENABLE,			&m_DepthWriteEnable);

	for(U32 i = 0; i < E_Sampler_Count; i++)
	{
		pDevice->GetTexture(i, &m_Samplers[i].m_pTexture);

		pDevice->GetSamplerState(i, D3DSAMP_ADDRESSU,		&m_Samplers[i].m_TextureAddressU);
		pDevice->GetSamplerState(i, D3DSAMP_ADDRESSV,		&m_Samplers[i].m_TextureAddressV);
		pDevice->GetSamplerState(i, D3DSAMP_ADDRESSW,		&m_Samplers[i].m_TextureAddressW);
		pDevice->GetSamplerState(i, D3DSAMP_MAGFILTER,		&m_Samplers[i].m_TextureFilterMag);
		pDevice->GetSamplerState(i, D3DSAMP_MINFILTER,		&m_Samplers[i].m_TextureFilterMin);
		pDevice->GetSamplerState(i, D3DSAMP_MIPFILTER,		&m_Samplers[i].m_TextureFilterMip);
		pDevice->GetSamplerState(i, D3DSAMP_BORDERCOLOR,	&m_Samplers[i].m_TextureBorderColour);
	}

	for(U32 i = 0; i < E_Stream_Count; i++)
	{
		pDevice->GetStreamSource(i, &m_Streams[i].m_pVertexBuffer, &m_Streams[i].m_VertexOffset, &m_Streams[i].m_VertexStride);
	}

	pDevice->GetVertexShaderConstantB(0,	m_VertexShaderConstantB,	128);
	pDevice->GetPixelShaderConstantB(0,		m_PixelShaderConstantB,		128);
	pDevice->GetVertexShaderConstantI(0,	m_VertexShaderConstantI,	16);
	pDevice->GetPixelShaderConstantI(0,		m_PixelShaderConstantI,		16);
	pDevice->GetVertexShaderConstantF(0,	m_VertexShaderConstantF,	256);
	pDevice->GetPixelShaderConstantF(0,		m_PixelShaderConstantF,		256);

	pDevice->GetViewport(&m_Viewport);
	pDevice->GetVertexDeclaration(&m_pVertexDeclaration);
	pDevice->GetIndices(&m_pIndexBuffer);
	pDevice->GetVertexShader(&m_pVertexShader);
	pDevice->GetPixelShader(&m_pPixelShader);
}

void C_DeviceStates::Set(IDirect3DDevice9* pDevice)
{
	PERF_EVENT("D3DSetStates");

	assert(pDevice);

	pDevice->SetRenderState(D3DRS_ALPHATESTENABLE,		m_AlphaTestEnable);
	pDevice->SetRenderState(D3DRS_ALPHAFUNC,			m_AlphaTestFunc);
	pDevice->SetRenderState(D3DRS_ALPHAREF,				m_AlphaTestRef);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,		m_BlendEnable);
#ifdef PLATFORM_XENON // only in DX10 on PC
	pDevice->SetRenderState(D3DRS_ALPHATOMASKENABLE,	m_AlphaToMaskEnable);
#endif
	pDevice->SetRenderState(D3DRS_SRCBLEND,				m_BlendSrc);
	pDevice->SetRenderState(D3DRS_DESTBLEND,			m_BlendDst);
	pDevice->SetRenderState(D3DRS_BLENDOP,				m_BlendOp);
	pDevice->SetRenderState(D3DRS_SRCBLENDALPHA,		m_BlendSrcAlpha);
	pDevice->SetRenderState(D3DRS_DESTBLENDALPHA,		m_BlendDstAlpha);
	pDevice->SetRenderState(D3DRS_BLENDOPALPHA,			m_BlendOpAlpha);
	pDevice->SetRenderState(D3DRS_COLORWRITEENABLE,		m_ColourWriteEnable);
	pDevice->SetRenderState(D3DRS_CULLMODE,				m_CullMode);
	pDevice->SetRenderState(D3DRS_FILLMODE,				m_FillMode);
	pDevice->SetRenderState(D3DRS_DEPTHBIAS,			m_DepthBias);
	pDevice->SetRenderState(D3DRS_ZENABLE,				m_DepthEnable);
	pDevice->SetRenderState(D3DRS_ZFUNC,				m_DepthFunc);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE,			m_DepthWriteEnable);

	for(U32 i = 0; i < E_Sampler_Count; i++)
	{
		pDevice->SetTexture(i, m_Samplers[i].m_pTexture);

		pDevice->SetSamplerState(i, D3DSAMP_ADDRESSU,		m_Samplers[i].m_TextureAddressU);
		pDevice->SetSamplerState(i, D3DSAMP_ADDRESSV,		m_Samplers[i].m_TextureAddressV);
		pDevice->SetSamplerState(i, D3DSAMP_ADDRESSW,		m_Samplers[i].m_TextureAddressW);
		pDevice->SetSamplerState(i, D3DSAMP_MAGFILTER,		m_Samplers[i].m_TextureFilterMag);
		pDevice->SetSamplerState(i, D3DSAMP_MINFILTER,		m_Samplers[i].m_TextureFilterMin);
		pDevice->SetSamplerState(i, D3DSAMP_MIPFILTER,		m_Samplers[i].m_TextureFilterMip);
		pDevice->SetSamplerState(i, D3DSAMP_BORDERCOLOR,	m_Samplers[i].m_TextureBorderColour);
	}

	for(U32 i = 0; i < E_Stream_Count; i++)
	{
		pDevice->SetStreamSource(i, m_Streams[i].m_pVertexBuffer, m_Streams[i].m_VertexOffset, m_Streams[i].m_VertexStride);
	}

	pDevice->SetVertexShaderConstantB(0,	m_VertexShaderConstantB,	128);
	pDevice->SetPixelShaderConstantB(0,		m_PixelShaderConstantB,		128);
	pDevice->SetVertexShaderConstantI(0,	m_VertexShaderConstantI,	16);
	pDevice->SetPixelShaderConstantI(0,		m_PixelShaderConstantI,		16);
	pDevice->SetVertexShaderConstantF(0,	m_VertexShaderConstantF,	256);
	pDevice->SetPixelShaderConstantF(0,		m_PixelShaderConstantF,		256);
	
	pDevice->SetViewport(&m_Viewport);
	pDevice->SetVertexDeclaration(m_pVertexDeclaration);
	pDevice->SetIndices(m_pIndexBuffer);
	pDevice->SetVertexShader(m_pVertexShader);
	pDevice->SetPixelShader(m_pPixelShader);

	// Release the handles to reduce the reference count and prevent memory leaks
	for(int i=0; i < E_Sampler_Count; i++)
	{
		if (m_Samplers[i].m_pTexture)
			m_Samplers[i].m_pTexture->Release();
	}

	for (int i=0; i < E_Stream_Count; i++)
	{
		if (m_Streams[i].m_pVertexBuffer)
			m_Streams[i].m_pVertexBuffer->Release();
	}

	if (m_pVertexDeclaration)	m_pVertexDeclaration->Release();
	if (m_pIndexBuffer)			m_pIndexBuffer->Release();
	if (m_pVertexShader)		m_pVertexShader->Release();
	if (m_pPixelShader)			m_pPixelShader->Release();
}

}; // namespace D3D9
}; // namespace Display
