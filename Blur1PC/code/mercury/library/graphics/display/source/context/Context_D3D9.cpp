#include "stdafx.h"
#include "Display/Context/Context_D3D9.h"
#include "Display/DisplayToDisplay2.h"

namespace Display
{

C_ContextD3D9::C_ContextD3D9() : m_pD3DCBDevice(NULL)
{
}

void C_ContextD3D9::SetVertexDeclaration(const C_VertexDeclaration* pVertexDeclaration)
{
	BIZ_ASSERT(m_Context.m_pd3dDevice);
	m_Context.m_pd3dDevice->SetVertexDeclaration(pVertexDeclaration ? pVertexDeclaration->m_pd3dVertexDeclaration : NULL);
}

void C_ContextD3D9::SetVertexBuffer(const U32 stream, const C_VertexBuffer* pVertexBuffer, const U32 offset, const U32 stride)
{
	BIZ_ASSERT(m_Context.m_pd3dDevice);
	m_Context.m_pd3dDevice->SetStreamSource(stream, pVertexBuffer ? (Display2::GPUVertexBuffer*)pVertexBuffer->GetGPUVertexBuffer() : NULL, offset, stride);
}

void C_ContextD3D9::SetIndexBuffer(const C_IndexBuffer* pIndexBuffer)
{
	BIZ_ASSERT(m_Context.m_pd3dDevice);
	m_Context.m_pd3dDevice->SetIndices(pIndexBuffer ? (Display2::GPUIndexBuffer*)pIndexBuffer->GetGPUIndexBuffer() : NULL);
}

void C_ContextD3D9::SetVertexShader(const Display::C_VertexShader* pShader)
{
	BIZ_ASSERT(m_Context.m_pd3dDevice);
	m_Context.m_pd3dDevice->SetVertexShader(pShader ? (Display2::GPUVertexShader*)pShader->GetGPUVertexShader() : NULL);
}

void C_ContextD3D9::SetPixelShader(const Display::C_PixelShader* pShader)
{
	BIZ_ASSERT(m_Context.m_pd3dDevice);
	m_Context.m_pd3dDevice->SetPixelShader(pShader ? (Display2::GPUPixelShader*)pShader->GetGPUPixelShader(): NULL);
}

void C_ContextD3D9::SetPixelShaderParameterF(C_PixelShaderParameter handle, const F32* pData)
{
	if(handle.m_RegisterIndex != UINT_MAX && handle.m_RegisterCount != 0)
		SetPixelShaderConstantF(handle.m_RegisterIndex, pData, 1);
}

void C_ContextD3D9::SetTexture(const U32 sampler, const void* pTexture)
{
	BIZ_ASSERT(m_Context.m_pd3dDevice);
	m_Context.m_pd3dDevice->SetTexture(sampler, (IDirect3DBaseTexture9*)pTexture);
}

void C_ContextD3D9::SetVertexTexture(const U32 sampler, const void* pTexture)
{
	BIZ_ASSERT(m_Context.m_pd3dDevice);
	m_Context.m_pd3dDevice->SetTexture(sampler + D3DVERTEXTEXTURESAMPLER0, (IDirect3DBaseTexture9*)pTexture);
}

void C_ContextD3D9::SetAlphaTestFunc(const E_CmpFunc alphaTestFunc)
{
	BIZ_ASSERT(m_Context.m_pd3dDevice);
	m_Context.m_pd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3D9CmpFunc(alphaTestFunc));
}

void C_ContextD3D9::SetBlendHighPrecisionEnable(const E_Bool blendHighPrecisionEnable)
{
}

void* C_ContextD3D9::BeginVertices(const E_PrimitiveType primitiveType, const U32 vertexCount, const U32 vertexStride, const C_VertexDeclaration* pVertexDeclaration)
{
	return 0;
}

void C_ContextD3D9::EndVertices()
{
}

} // namespace Display
