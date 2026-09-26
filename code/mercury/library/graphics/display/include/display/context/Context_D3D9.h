#ifndef __DISPLAY_CONTEXT_D3D9_H__
#define __DISPLAY_CONTEXT_D3D9_H__

#include "Display/Context/Context_Base.h"

namespace Display
{

class C_ContextD3D9 : public C_ContextBase
{
public:

	C_ContextD3D9();

	void SetVertexDeclaration(const C_VertexDeclaration* pVertexDeclaration);
	void SetVertexBuffer(const U32 stream, const C_VertexBuffer* pVertexBuffer, const U32 offset, const U32 stride);
	void SetIndexBuffer(const C_IndexBuffer* pIndexBuffer);

	void SetVertexShader(const Display::C_VertexShader* pShader);
	
	void SetPixelShader(const Display::C_PixelShader* pShader);
	void SetPixelShaderParameterF(C_PixelShaderParameter handle, const F32* pData);

	void SetTexture(const U32 sampler, const void* pTexture);
	void SetVertexTexture(const U32 sampler, const void* pTexture);

	void SetAlphaTestFunc(const E_CmpFunc alphaTestFunc);
	void SetBlendHighPrecisionEnable(const E_Bool blendHighPrecisionEnable);

	void* BeginVertices(const E_PrimitiveType primitiveType, const U32 vertexCount, const U32 vertexStride, const C_VertexDeclaration* pVertexDeclaration);
	void EndVertices();

protected:

	IDirect3DDevice9*				m_pD3DCBDevice;
};

} // namespace Display

#endif // __DISPLAY_CONTEXT_D3D9_H__
