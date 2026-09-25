#ifndef DISPLAY_DEVICE_D3D9_D3D9_DEVICE_INL
#define DISPLAY_DEVICE_D3D9_D3D9_DEVICE_INL

#ifdef _MSC_VER
#pragma once
#endif

#ifdef CFG_ALLOW_OPTIMIZE_OFF
//#pragma optimize( "", off ) // CFG_ALLOW_OPTIMIZE_OFF
#endif

namespace Display
{
namespace D3D9
{

INLINE void C_Sampler::InvalidateState()
{
	C_SamplerBase::InvalidateState();

	m_pTexture		= NULL;
	// Border colour
	m_BorderColour	= 0x00000000;
}

INLINE void C_Stream::InvalidateState()
{
	m_pVertexBuffer = NULL;
}

INLINE IDirect3DDevice9* C_D3D9Device::GetD3DDevice()
{
	return m_pd3dDevice;
}

INLINE IDirect3D9* C_D3D9Device::GetD3DObject()
{
	return m_pd3dObject;
}

INLINE D3DPRESENT_PARAMETERS& C_D3D9Device::GetD3DPresentParams()
{
	return m_d3dPresentParams;
}

INLINE const S32 C_D3D9Device::GetBackBufferCount() const
{
	return m_BackBuffers.Size();
}

INLINE const C_BackBuffer& C_D3D9Device::GetBackBuffer() const
{
	assert(m_CurrentBackBuffer >= 0 && m_CurrentBackBuffer < m_BackBuffers.Size());
	return *m_BackBuffers[m_CurrentBackBuffer];
}

INLINE const C_BackBuffer& C_D3D9Device::GetBackBuffer(const S32 backBufferIndex) const
{
	assert(backBufferIndex >= 0 && backBufferIndex < m_BackBuffers.Size());
	return *m_BackBuffers[backBufferIndex];	
}

INLINE const S32 C_D3D9Device::GetFrontBufferCount() const
{
	return m_FrontBuffers.Size();
}

INLINE const C_FrontBuffer& C_D3D9Device::GetFrontBuffer() const
{
	assert(m_CurrentFrontBuffer >= 0 && m_CurrentFrontBuffer < m_FrontBuffers.Size());
	return *m_FrontBuffers[m_CurrentFrontBuffer];
}

INLINE const C_FrontBuffer& C_D3D9Device::GetFrontBuffer(const S32 frontBufferIndex) const
{
	assert(frontBufferIndex >= 0 && frontBufferIndex < m_FrontBuffers.Size());
	return *m_FrontBuffers[frontBufferIndex];
}

INLINE const C_FrontBuffer& C_D3D9Device::GetSwappedFrontBuffer() const
{
	assert(m_SwappedFrontBuffer >= 0 && m_SwappedFrontBuffer < m_FrontBuffers.Size());
	return *m_FrontBuffers[m_SwappedFrontBuffer];
}

INLINE const C_RenderTarget& C_D3D9Device::GetRenderTarget() const
{
	return m_RenderTarget;
}

INLINE const S32 C_D3D9Device::GetWindowCount() const
{
	return m_Windows.Size();
}

INLINE C_Window* C_D3D9Device::GetWindow(const S32 windowIndex)
{
	assert(windowIndex >= 0 && windowIndex < m_Windows.Size());
	return m_Windows[windowIndex];
}
	
INLINE void C_D3D9Device::DrawPrimitive(const E_PrimitiveType primitiveType, const U32 startVertex, const U32 vertexCount)
{
#ifdef CFG_DEBUG
	ValidateIndexCount(primitiveType, vertexCount);
#endif
	assert(m_pd3dDevice);
	const U32 primitiveCount = PrimitiveCount(primitiveType, vertexCount);
	assert(primitiveCount > 0);
	m_pd3dDevice->DrawPrimitive(D3D9PrimitiveType(primitiveType), startVertex, primitiveCount);
}

INLINE void C_D3D9Device::DrawIndexedPrimitive(const E_PrimitiveType primitiveType, const U32 baseVertex, const U32 startIndex, const U32 indexCount)
{
#ifdef CFG_DEBUG
	ValidateIndexCount(primitiveType, indexCount);
#endif
	assert(m_pd3dDevice);
#ifdef PLATFORM_XENON
	assert(indexCount > 0);
	m_pd3dDevice->DrawIndexedVertices(D3D9PrimitiveType(primitiveType), baseVertex, startIndex, indexCount);
#else
	assert(0 && "You must specify minIndex and vertexCount for ATI graphics cards on the PC. Please use the other DrawIndexedPrimitive function!");
#endif
}

INLINE void C_D3D9Device::DrawIndexedPrimitive(const E_PrimitiveType primitiveType, const U32 baseVertex, const U32 minIndex, const U32 vertexCount, const U32 startIndex, const U32 indexCount)
{
#ifdef CFG_DEBUG
	ValidateIndexCount(primitiveType, indexCount);
#endif
	assert(m_pd3dDevice);
#ifdef PLATFORM_XENON
	assert(indexCount > 0);
	m_pd3dDevice->DrawIndexedVertices(D3D9PrimitiveType(primitiveType), baseVertex, startIndex, indexCount);
#else
	assert(vertexCount > 0);
	assert(indexCount > 0);
	m_pd3dDevice->DrawIndexedPrimitive(D3D9PrimitiveType(primitiveType), baseVertex, minIndex, vertexCount, startIndex, PrimitiveCount(primitiveType, indexCount));
#endif
}

INLINE void C_D3D9Device::SetAlphaTestEnable(const E_Bool alphaTestEnable)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, D3D9Bool(alphaTestEnable));
	m_AlphaTestEnable = alphaTestEnable;
}

INLINE void C_D3D9Device::SetAlphaToMaskEnable(const E_Bool alphaToMaskEnable)
{
	assert(m_pd3dDevice);
#ifdef PLATFORM_XENON // only in DX10 on PC
	m_pd3dDevice->SetRenderState(D3DRS_ALPHATOMASKENABLE, D3D9Bool(alphaToMaskEnable));
#endif
	m_AlphaToMaskEnable = alphaToMaskEnable;
}

INLINE void C_D3D9Device::SetAlphaTestFunc(const E_CmpFunc alphaTestFunc)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3D9CmpFunc(alphaTestFunc));
	m_AlphaTestFunc = alphaTestFunc;
}

INLINE void C_D3D9Device::SetAlphaTestRef(const U32 alphaTestRef)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_ALPHAREF, alphaTestRef);
	m_AlphaTestRef = alphaTestRef;
}

INLINE void C_D3D9Device::SetBlend(const E_Blend blendSrc, const E_Blend blendDst, const E_BlendOp blendOp, const E_Blend blendSrcAlpha, const E_Blend blendDstAlpha, const E_BlendOp blendOpAlpha)
{
	SetBlendSrc(blendSrc);
	SetBlendDst(blendDst);
	SetBlendOp(blendOp);

	SetBlendSrcAlpha(blendSrcAlpha);
	SetBlendDstAlpha(blendDstAlpha);
	SetBlendOpAlpha(blendOpAlpha);
}

INLINE void C_D3D9Device::SetBlendDst(const E_Blend blendDst)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3D9Blend(blendDst));
	m_BlendDst = blendDst;
}

INLINE void C_D3D9Device::SetBlendDstAlpha(const E_Blend blendDstAlpha)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_DESTBLENDALPHA, D3D9Blend(blendDstAlpha));
	m_BlendDstAlpha = blendDstAlpha;
}

INLINE void C_D3D9Device::SetBlendEnable(const E_Bool blendEnable)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, D3D9Bool(blendEnable));
	m_BlendEnable = blendEnable;

#ifdef PLATFORM_XENON
	if(m_HighPrecisionColourSurface)
		SetBlendHighPrecisionEnable(blendEnable);
#endif
}

INLINE void C_D3D9Device::SetBlendOp(const E_BlendOp blendOp)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3D9BlendOp(blendOp));
	m_BlendOp = blendOp;
}

INLINE void C_D3D9Device::SetBlendOpAlpha(const E_BlendOp blendOpAlpha)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_BLENDOPALPHA, D3D9BlendOp(blendOpAlpha));
	m_BlendOpAlpha = blendOpAlpha;
}

INLINE void C_D3D9Device::SetBlendSrc(const E_Blend blendSrc)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3D9Blend(blendSrc));
	m_BlendSrc = blendSrc;
}

INLINE void C_D3D9Device::SetBlendSrcAlpha(const E_Blend blendSrcAlpha)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_SRCBLENDALPHA, D3D9Blend(blendSrcAlpha));
	m_BlendSrcAlpha = blendSrcAlpha;
}

INLINE void C_D3D9Device::SetBlendHighPrecisionEnable(const E_Bool blendHighPrecisionEnable)
{
#ifdef PLATFORM_XENON
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, blendHighPrecisionEnable == E_Bool_True ? TRUE : FALSE);
	m_BlendHighPrecisionEnable = blendHighPrecisionEnable;
#endif
}

INLINE void C_D3D9Device::SetColourWriteEnable(U32 colourWriteEnable)
{
	if(colourWriteEnable == E_ColourWriteEnable_Default)
		colourWriteEnable = m_ColourWriteEnableDefault;

	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_COLORWRITEENABLE, colourWriteEnable);
	m_ColourWriteEnable = colourWriteEnable;
}

INLINE void C_D3D9Device::SetCullMode(E_CullMode cullMode)
{
	if(cullMode == E_CullMode_Front)
		cullMode = E_CullMode_Reversed;
	else if(cullMode == E_CullMode_Back)
		cullMode = E_CullMode_Default;

	if(cullMode == E_CullMode_Default)
		cullMode = m_CullModeDefault;
	if(cullMode == E_CullMode_Reversed)
		cullMode = FlippedCullMode(m_CullModeDefault);

	if(m_CullModeFlip)
		cullMode = FlippedCullMode(cullMode);

	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3D9CullMode(cullMode));
	m_CullMode = cullMode;
}

INLINE void C_D3D9Device::SetCullModeFlip()
{
	m_CullModeFlip = !m_CullModeFlip;
	if (m_CullModeFlip)
		SetCullMode(m_CullMode);
	else
		SetCullMode(FlippedCullMode(m_CullMode));
}

INLINE void C_D3D9Device::SetCullModeFlip(const bool cullModeFlip)
{
	m_CullModeFlip = cullModeFlip;
	if (m_CullModeFlip)
		SetCullMode(m_CullMode);
	else
		SetCullMode(FlippedCullMode(m_CullMode));
}

INLINE void C_D3D9Device::SetDepthBias(const F32 depthBias)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_DEPTHBIAS, *((DWORD*)&depthBias));
	m_DepthBias = depthBias;
}

INLINE void C_D3D9Device::SetDepthEnable(E_Bool depthEnable)
{
	if(depthEnable == E_Bool_Default)
		depthEnable = m_DepthEnableDefault;
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_ZENABLE, D3D9Bool(depthEnable) ?  D3DZB_TRUE : D3DZB_FALSE);
#if PLATFORM_XENON
	m_pd3dDevice->SetRenderState(D3DRS_HIZENABLE, (depthEnable && m_DepthFunc == E_CmpFunc_GreaterEqual) ? D3DHIZ_ENABLE: D3DHIZ_AUTOMATIC);
#endif
	m_DepthEnable = depthEnable;
}

INLINE void C_D3D9Device::SetDepthFlip(const bool depthCmpFlip, const bool depthViewportFlip)
{
	if(depthCmpFlip != m_DepthCmpFlip || (depthCmpFlip && depthViewportFlip) != m_DepthViewportFlip)
	{
		m_DepthFuncDefault = depthCmpFlip ? E_CmpFunc_GreaterEqual : E_CmpFunc_LessEqual;

		m_DepthViewportFlip = depthCmpFlip && depthViewportFlip;
		m_DepthCmpFlip = depthCmpFlip;
	}
}

INLINE void C_D3D9Device::SetDepthFunc(E_CmpFunc depthFunc)
{
	if(depthFunc == E_CmpFunc_Default)
		depthFunc = m_DepthFuncDefault;

	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3D9CmpFunc(depthFunc));
#if PLATFORM_XENON
	m_pd3dDevice->SetRenderState(D3DRS_HIZENABLE, (m_DepthEnable && depthFunc == E_CmpFunc_GreaterEqual) ? D3DHIZ_ENABLE: D3DHIZ_AUTOMATIC);
#endif
	m_DepthFunc = depthFunc;
}

INLINE void C_D3D9Device::SetDepthWriteEnable(const E_Bool depthWriteEnable)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, D3D9Bool(depthWriteEnable));
	m_DepthWriteEnable = depthWriteEnable;
}

INLINE void C_D3D9Device::SetFillMode(E_FillMode fillMode)
{
	if(fillMode == E_FillMode_Default)
		fillMode = m_FillModeDefault;

	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3D9FillMode(fillMode));
	m_FillMode = fillMode;
}

INLINE void C_D3D9Device::SetPointSpriteEnable(const E_Bool pointSpriteEnable)
{
	if(pointSpriteEnable != m_PointSpriteEnable)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetRenderState(D3DRS_POINTSPRITEENABLE, D3D9Bool(pointSpriteEnable));

		m_PointSpriteEnable = pointSpriteEnable;
	}
}

INLINE void C_D3D9Device::SetIndexBuffer(const C_IndexBuffer* pIndexBuffer)
{
	if(pIndexBuffer != m_pIndexBuffer)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetIndices(pIndexBuffer ? (Display2::GPUIndexBuffer*)pIndexBuffer->GetGPUIndexBuffer() : NULL);

		m_pIndexBuffer = pIndexBuffer;
	}
}

INLINE const C_PixelShader* C_D3D9Device::GetPixelShader()
{
	return m_pPixelShader;
}

INLINE void C_D3D9Device::SetPixelShader(const C_PixelShader* pPixelShader)
{
	if(pPixelShader != m_pPixelShader)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetPixelShader(pPixelShader ? (Display2::GPUPixelShader*)pPixelShader->GetGPUPixelShader() : NULL);

		m_pPixelShader = pPixelShader;
	}
}

INLINE void C_D3D9Device::SetPixelShaderConstantB(const U32 startRegister, const BOOL* pData, const U32 registerCount)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetPixelShaderConstantB(startRegister, pData, registerCount);
}

INLINE void C_D3D9Device::SetPixelShaderConstantI(const U32 startRegister, const S32* pData, const U32 registerCount)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetPixelShaderConstantI(startRegister, pData, registerCount);
}

INLINE void C_D3D9Device::SetPixelShaderConstantF(const U32 startRegister, const F32* pData, const U32 registerCount)
{
	assert(m_pd3dDevice);

#ifdef CFG_DEBUG
	if(CheckPixelShaderConstantsLocked(startRegister, startRegister + registerCount))
	{
		LOGWARN(g_Log, "Shader constants %i to %i are locked\n", startRegister, startRegister + registerCount);
		return;
	}
#endif

	m_pd3dDevice->SetPixelShaderConstantF(startRegister, pData, registerCount);
}

INLINE void C_D3D9Device::SetPixelShaderParameterF(const C_PixelShaderParameter parameter, const F32* pData)
{
	if(parameter.m_RegisterIndex != UINT_MAX)
		SetPixelShaderConstantF(parameter.m_RegisterIndex, pData, 1);
}

INLINE void C_D3D9Device::SetPixelShaderParameterArrayF(const C_PixelShaderParameterArray& parameterArray, const F32* pData)
{
	assert(m_pd3dDevice);
	if(parameterArray.m_RegisterIndex != UINT_MAX && parameterArray.m_RegisterCount != 0)
		m_pd3dDevice->SetPixelShaderConstantF(parameterArray.m_RegisterIndex, pData, parameterArray.m_RegisterCount);
}

INLINE void C_D3D9Device::SetPixelShaderParameters(const C_PixelShader* pPixelShader)
{
	// this doesn't need to do anything in d3d
}

INLINE bool C_D3D9Device::CheckPixelShaderConstantsLocked(const U32 startRegister, const U32 endRegister)
{
	for(U32 lock = 0; lock < E_PixelShaderConstantLock_Count; ++lock)
	{
		if(m_PixelShaderConstantLock[lock].enabled && !(endRegister <= m_PixelShaderConstantLock[lock].startRegister || startRegister > m_PixelShaderConstantLock[lock].endRegister))
		{
			return true;
		}
	}

	return false;
}

INLINE void C_D3D9Device::SetPresentInterval(E_PresentInterval presentInterval)
{
#ifndef PLATFORM_PC
	if(presentInterval == E_PresentInterval_Default)
		presentInterval = m_PresentIntervalDefault;

	//if(presentInterval != m_PresentInterval)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetRenderState(D3DRS_PRESENTINTERVAL, D3D9PresentInterval(presentInterval));

		m_PresentInterval = presentInterval;
	}
#endif
}

INLINE void C_D3D9Device::SetScissorTest(const E_Bool scissorTestEnable)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, D3D9Bool(scissorTestEnable));
}

INLINE void C_D3D9Device::SetScissorRect(const U32 x, const U32 y, const U32 w, const U32 h)
{
	assert(m_pd3dDevice);
	const RECT rect = {x, y, x+w, y+h};
	m_pd3dDevice->SetScissorRect(&rect);
}

INLINE void C_D3D9Device::SetSlopeScaledBias(const F32 slopeScaledBias)
{
	if(slopeScaledBias != m_SlopeScaledBias)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *((DWORD*)&slopeScaledBias));

		m_SlopeScaledBias = slopeScaledBias;
	}
}

INLINE void C_D3D9Device::SetStencilState(const Display::C_StencilState& stencilState)
{
	SetStencilEnable(stencilState.m_StencilEnable);
	SetStencilFunc(stencilState.m_StencilFunc);
	SetStencilRef(stencilState.m_StencilRef);
	SetStencilMask(stencilState.m_StencilMask);
	SetStencilWriteMask(stencilState.m_StencilWriteMask);
	SetStencilFail(stencilState.m_StencilFail);
	SetStencilDepthFail(stencilState.m_StencilDepthFail);
	SetStencilPass(stencilState.m_StencilDepthPass);
}

INLINE void C_D3D9Device::SetStencilEnable(const E_Bool stencilEnable)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_STENCILENABLE, D3D9Bool(stencilEnable));
}

INLINE void C_D3D9Device::SetStencilFail(const E_StencilOp stencilFail)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_STENCILFAIL, D3D9StencilOp(stencilFail));
}

INLINE void C_D3D9Device::SetStencilDepthFail(const E_StencilOp stencilDepthFail)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_STENCILZFAIL, D3D9StencilOp(stencilDepthFail));
}

INLINE void C_D3D9Device::SetStencilPass(const E_StencilOp stencilPass)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_STENCILPASS, D3D9StencilOp(stencilPass));
}

INLINE void C_D3D9Device::SetStencilFunc(const E_CmpFunc stencilFunc)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_STENCILFUNC, D3D9CmpFunc(stencilFunc));
}

INLINE void C_D3D9Device::SetStencilRef(const S32 stencilRef)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_STENCILREF, stencilRef);
}

INLINE void C_D3D9Device::SetStencilMask(const U32 stencilMask)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_STENCILMASK, stencilMask);
}

INLINE void C_D3D9Device::SetStencilWriteMask(const U32 stencilWriteMask)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_STENCILWRITEMASK, stencilWriteMask);
}

INLINE void C_D3D9Device::SetStencilCcwEnable(const E_Bool stencilCcwEnable)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE , D3D9Bool(stencilCcwEnable));
}

INLINE void C_D3D9Device::SetStencilCcwFail(const E_StencilOp stencilFail)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_CCW_STENCILFAIL, D3D9StencilOp(stencilFail));
}

INLINE void C_D3D9Device::SetStencilCcwDepthFail(const E_StencilOp stencilDepthFail)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_CCW_STENCILZFAIL, D3D9StencilOp(stencilDepthFail));
}

INLINE void C_D3D9Device::SetStencilCcwPass(const E_StencilOp stencilPass)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_CCW_STENCILPASS, D3D9StencilOp(stencilPass));
}

INLINE void C_D3D9Device::SetStencilCcwFunc(const E_CmpFunc stencilFunc)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetRenderState(D3DRS_CCW_STENCILFUNC, D3D9CmpFunc(stencilFunc));
}

INLINE void C_D3D9Device::SetStencilCcwRef(const S32 stencilRef)
{
	assert(m_pd3dDevice);
	#ifndef PLATFORM_PC
	m_pd3dDevice->SetRenderState(D3DRS_CCW_STENCILREF, stencilRef);
	#endif
}

INLINE void C_D3D9Device::SetStencilCcwMask(const U32 stencilMask)
{
	assert(m_pd3dDevice);
	#ifndef PLATFORM_PC
	m_pd3dDevice->SetRenderState(D3DRS_CCW_STENCILMASK, stencilMask);
	#endif
}

INLINE void C_D3D9Device::SetStencilCcwWriteMask(const U32 stencilWriteMask)
{
	assert(m_pd3dDevice);
	#ifndef PLATFORM_PC
	m_pd3dDevice->SetRenderState(D3DRS_CCW_STENCILWRITEMASK, stencilWriteMask);
	#endif
}

INLINE void C_D3D9Device::SetTexture(const U32 sampler, const C_Texture* pTexture)
{
	assert(sampler < E_Sampler_Count);

	if(pTexture != m_Sampler[sampler].m_pTexture)
	{
		assert(m_pd3dDevice);
#if PLATFORM_XENON
		m_pd3dDevice->SetTexture(sampler, pTexture && pTexture->IsCreated() ? (D3DTexture*)&pTexture->m_d3dTexture : NULL);
#else
		m_pd3dDevice->SetTexture(sampler, pTexture ? pTexture->m_pd3dTexture : NULL);
#endif
	
		m_Sampler[sampler].m_pTexture = pTexture;
	}
}

INLINE void C_D3D9Device::SetTextureAddressU(const U32 sampler, E_TextureAddress addressU)
{
	assert(sampler < E_Sampler_Count);

	if(addressU == E_TextureAddress_Default)
		addressU = m_Sampler[sampler].m_AddressUDefault;

	if(addressU != m_Sampler[sampler].m_AddressU)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetSamplerState(sampler, D3DSAMP_ADDRESSU, D3D9TextureAddress(addressU));

		m_Sampler[sampler].m_AddressU = addressU;
	}
}

INLINE void C_D3D9Device::SetTextureAddressV(const U32 sampler, E_TextureAddress addressV)
{
	assert(sampler < E_Sampler_Count);

	if(addressV == E_TextureAddress_Default)
		addressV = m_Sampler[sampler].m_AddressVDefault;

	if(addressV != m_Sampler[sampler].m_AddressV)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetSamplerState(sampler, D3DSAMP_ADDRESSV, D3D9TextureAddress(addressV));

		m_Sampler[sampler].m_AddressV = addressV;
	}
}

INLINE void C_D3D9Device::SetTextureAddressW(const U32 sampler, E_TextureAddress addressW)
{
	assert(sampler < E_Sampler_Count);

	if(addressW == E_TextureAddress_Default)
		addressW = m_Sampler[sampler].m_AddressWDefault;

	if(addressW != m_Sampler[sampler].m_AddressW)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetSamplerState(sampler, D3DSAMP_ADDRESSW, D3D9TextureAddress(addressW));

		m_Sampler[sampler].m_AddressW = addressW;
	}
}

INLINE void C_D3D9Device::SetTextureAddressUVW(const U32 sampler, const E_TextureAddress addressU, const E_TextureAddress addressV, const E_TextureAddress addressW)
{
	SetTextureAddressU(sampler, addressU);
	SetTextureAddressV(sampler, addressV);
	SetTextureAddressW(sampler, addressW);
}

INLINE void C_D3D9Device::SetTextureFilterMag(const U32 sampler, E_TextureFilter filterMag)
{
	assert(sampler < E_Sampler_Count);

	if(filterMag == E_TextureFilter_Default)
		filterMag = m_Sampler[sampler].m_FilterMagDefault;

	if(filterMag != m_Sampler[sampler].m_FilterMag)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetSamplerState(sampler, D3DSAMP_MAGFILTER, D3D9TextureFilter(filterMag));

		m_Sampler[sampler].m_FilterMag = filterMag;
	}
//#ifdef CFG_DEBUG
//	else
//	{
//		DWORD value;
//		m_pd3dDevice->GetSamplerState(sampler, D3DSAMP_MAGFILTER, &value);
//		assert(value == D3D9TextureFilter(m_Sampler[sampler].m_FilterMag));
//	}
//#endif
}

INLINE void C_D3D9Device::SetTextureFilterMin(const U32 sampler, E_TextureFilter filterMin)
{
	assert(sampler < E_Sampler_Count);

	if(filterMin == E_TextureFilter_Default)
		filterMin = m_Sampler[sampler].m_FilterMinDefault;

	if(filterMin != m_Sampler[sampler].m_FilterMin)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetSamplerState(sampler, D3DSAMP_MINFILTER, D3D9TextureFilter(filterMin));

		m_Sampler[sampler].m_FilterMin = filterMin;
	}
//#ifdef CFG_DEBUG
//	else
//	{
//		DWORD value;
//		m_pd3dDevice->GetSamplerState(sampler, D3DSAMP_MINFILTER, &value);
//		assert(value == D3D9TextureFilter(m_Sampler[sampler].m_FilterMin));
//	}
//#endif
}

INLINE void C_D3D9Device::SetTextureFilterMip(const U32 sampler, E_TextureFilter filterMip)
{
	assert(sampler < E_Sampler_Count);

	if(filterMip == E_TextureFilter_Default)
		filterMip = m_Sampler[sampler].m_FilterMipDefault;

	if(filterMip != m_Sampler[sampler].m_FilterMip)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetSamplerState(sampler, D3DSAMP_MIPFILTER, D3D9TextureFilter(filterMip));

		m_Sampler[sampler].m_FilterMip = filterMip;
	}
//#ifdef CFG_DEBUG
//	else
//	{
//		DWORD value;
//		m_pd3dDevice->GetSamplerState(sampler, D3DSAMP_MIPFILTER, &value);
//		assert(value == D3D9TextureFilter(m_Sampler[sampler].m_FilterMip));
//	}
//#endif
}

INLINE void C_D3D9Device::SetTextureFilterMagMinMip(const U32 sampler, const E_TextureFilter filterMag, E_TextureFilter filterMin, E_TextureFilter filterMip)
{
	SetTextureFilterMag(sampler, filterMag);
	SetTextureFilterMin(sampler, filterMin);
	SetTextureFilterMip(sampler, filterMip);
}

INLINE void C_D3D9Device::SetTextureBorderColour(const U32 sampler, const U32 borderColour)
{
	assert(sampler < E_Sampler_Count);

	if(borderColour != m_Sampler[sampler].m_BorderColour)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetSamplerState(sampler, D3DSAMP_BORDERCOLOR, borderColour);

		m_Sampler[sampler].m_BorderColour = borderColour;
	}
}

INLINE void C_D3D9Device::SetTextureLodBias(const U32 sampler, const S8 lodBias)
{
#ifdef PLATFORM_XENON
	assert(sampler < E_Sampler_Count);

	assert(m_pd3dDevice);		
	const F32 fLodBias = (((F32)(lodBias))/128.0f)*8.0f;
	m_pd3dDevice->SetSamplerState(sampler, D3DSAMP_MIPMAPLODBIAS, (DWORD&)fLodBias);

	m_Sampler[sampler].m_LodBias = lodBias;
#endif
}

INLINE void C_D3D9Device::SetTextureGamma(const U32 sampler, E_Bool gamma)
{
#ifdef PLATFORM_PC
	assert(sampler < E_Sampler_Count);

	if(m_ForceTextureGamma != E_Bool_Default)
		gamma = m_ForceTextureGamma;

	if(gamma != m_Sampler[sampler].m_Gamma)
	{
		assert(m_pd3dDevice);		
		DWORD value = (gamma==E_Bool_True);
		m_pd3dDevice->SetSamplerState(sampler, D3DSAMP_SRGBTEXTURE, value);

		m_Sampler[sampler].m_Gamma = gamma;
	}
#endif
}

INLINE void C_D3D9Device::SetVertexTexture(const U32 sampler, const C_Texture* pTexture)
{
	assert(sampler < E_VertexSampler_Count);

	if(pTexture != m_VertexSampler[sampler].m_pTexture)
	{
		assert(m_pd3dDevice);
#if PLATFORM_XENON
		m_pd3dDevice->SetTexture(D3DVERTEXTEXTURESAMPLER0+sampler, pTexture && pTexture->IsCreated() ? (D3DTexture*)&pTexture->m_d3dTexture : NULL);
#else
		m_pd3dDevice->SetTexture(D3DVERTEXTEXTURESAMPLER0+sampler, pTexture ? pTexture->m_pd3dTexture : NULL);
#endif
	
		m_VertexSampler[sampler].m_pTexture = pTexture;
	}
}

INLINE void C_D3D9Device::SetVertexTextureAddressU(const U32 sampler, E_TextureAddress addressU)
{
	assert(sampler < E_VertexSampler_Count);

	if(addressU == E_TextureAddress_Default)
		addressU = m_VertexSampler[sampler].m_AddressUDefault;

	if(addressU != m_VertexSampler[sampler].m_AddressU)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetSamplerState(D3DVERTEXTEXTURESAMPLER0+sampler, D3DSAMP_ADDRESSU, D3D9TextureAddress(addressU));

		m_VertexSampler[sampler].m_AddressU = addressU;
	}
}

INLINE void C_D3D9Device::SetVertexTextureAddressV(const U32 sampler, E_TextureAddress addressV)
{
	assert(sampler < E_VertexSampler_Count);

	if(addressV == E_TextureAddress_Default)
		addressV = m_VertexSampler[sampler].m_AddressVDefault;

	if(addressV != m_VertexSampler[sampler].m_AddressV)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetSamplerState(D3DVERTEXTEXTURESAMPLER0+sampler, D3DSAMP_ADDRESSV, D3D9TextureAddress(addressV));

		m_VertexSampler[sampler].m_AddressV = addressV;
	}
}

INLINE void C_D3D9Device::SetVertexTextureAddressW(const U32 sampler, E_TextureAddress addressW)
{
	assert(sampler < E_VertexSampler_Count);

	if(addressW == E_TextureAddress_Default)
		addressW = m_VertexSampler[sampler].m_AddressWDefault;

	if(addressW != m_VertexSampler[sampler].m_AddressW)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetSamplerState(D3DVERTEXTEXTURESAMPLER0+sampler, D3DSAMP_ADDRESSW, D3D9TextureAddress(addressW));

		m_VertexSampler[sampler].m_AddressW = addressW;
	}
}

INLINE void C_D3D9Device::SetVertexTextureAddressUVW(const U32 sampler, const E_TextureAddress addressU, const E_TextureAddress addressV, const E_TextureAddress addressW)
{
	SetVertexTextureAddressU(sampler, addressU);
	SetVertexTextureAddressV(sampler, addressV);
	SetVertexTextureAddressW(sampler, addressW);
}

INLINE void C_D3D9Device::SetVertexTextureFilterMag(const U32 sampler, E_TextureFilter filterMag)
{
	assert(sampler < E_VertexSampler_Count);

	if(filterMag == E_TextureFilter_Default)
		filterMag = m_VertexSampler[sampler].m_FilterMagDefault;

	if(filterMag != m_VertexSampler[sampler].m_FilterMag)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetSamplerState(D3DVERTEXTEXTURESAMPLER0+sampler, D3DSAMP_MAGFILTER, D3D9TextureFilter(filterMag));

		m_VertexSampler[sampler].m_FilterMag = filterMag;
	}
//#ifdef CFG_DEBUG
//	else
//	{
//		DWORD value;
//		m_pd3dDevice->GetSamplerState(D3DVERTEXTEXTURESAMPLER0+sampler, D3DSAMP_MAGFILTER, &value);
//		assert(value == D3D9TextureFilter(m_VertexSampler[sampler].m_FilterMag));
//	}
//#endif
}

INLINE void C_D3D9Device::SetVertexTextureFilterMin(const U32 sampler, E_TextureFilter filterMin)
{
	assert(sampler < E_VertexSampler_Count);

	if(filterMin == E_TextureFilter_Default)
		filterMin = m_VertexSampler[sampler].m_FilterMinDefault;

	if(filterMin != m_VertexSampler[sampler].m_FilterMin)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetSamplerState(D3DVERTEXTEXTURESAMPLER0+sampler, D3DSAMP_MINFILTER, D3D9TextureFilter(filterMin));

		m_VertexSampler[sampler].m_FilterMin = filterMin;
	}
//#ifdef CFG_DEBUG
//	else
//	{
//		DWORD value;
//		m_pd3dDevice->GetSamplerState(D3DVERTEXTEXTURESAMPLER0+sampler, D3DSAMP_MINFILTER, &value);
//		assert(value == D3D9TextureFilter(m_VertexSampler[sampler].m_FilterMin));
//	}
//#endif
}

INLINE void C_D3D9Device::SetVertexTextureFilterMip(const U32 sampler, E_TextureFilter filterMip)
{
	assert(sampler < E_VertexSampler_Count);

	if(filterMip == E_TextureFilter_Default)
		filterMip = m_VertexSampler[sampler].m_FilterMipDefault;

	if(filterMip != m_VertexSampler[sampler].m_FilterMip)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetSamplerState(D3DVERTEXTEXTURESAMPLER0+sampler, D3DSAMP_MIPFILTER, D3D9TextureFilter(filterMip));

		m_VertexSampler[sampler].m_FilterMip = filterMip;
	}
//#ifdef CFG_DEBUG
//	else
//	{
//		DWORD value;
//		m_pd3dDevice->GetSamplerState(D3DVERTEXTEXTURESAMPLER0+sampler, D3DSAMP_MIPFILTER, &value);
//		assert(value == D3D9TextureFilter(m_VertexSampler[sampler].m_FilterMip));
//	}
//#endif
}

INLINE void C_D3D9Device::SetVertexTextureFilterMagMinMip(const U32 sampler, const E_TextureFilter filterMag, E_TextureFilter filterMin, E_TextureFilter filterMip)
{
	SetVertexTextureFilterMag(sampler, filterMag);
	SetVertexTextureFilterMin(sampler, filterMin);
	SetVertexTextureFilterMip(sampler, filterMip);
}

INLINE void C_D3D9Device::SetVertexTextureBorderColour(const U32 sampler, const U32 borderColour)
{
	assert(sampler < E_VertexSampler_Count);

	if(borderColour != m_VertexSampler[sampler].m_BorderColour)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetSamplerState(D3DVERTEXTEXTURESAMPLER0+sampler, D3DSAMP_BORDERCOLOR, borderColour);

		m_VertexSampler[sampler].m_BorderColour = borderColour;
	}
}

INLINE void C_D3D9Device::SetVertexTextureLodBias(const U32 sampler, const S8 lodBias)
{
#ifdef PLATFORM_XENON
	assert(sampler < E_VertexSampler_Count);

	assert(m_pd3dDevice);		
	const F32 fLodBias = (((F32)(lodBias))/128.0f)*8.0f;
	m_pd3dDevice->SetSamplerState(D3DVERTEXTEXTURESAMPLER0+sampler, D3DSAMP_MIPMAPLODBIAS, (DWORD&)fLodBias);

	m_VertexSampler[sampler].m_LodBias = lodBias;
#endif
}

INLINE void C_D3D9Device::SetVertexTextureGamma(const U32 sampler, E_Bool gamma)
{
#ifdef PLATFORM_PC
	assert(sampler < E_VertexSampler_Count);

	if(m_ForceTextureGamma != E_Bool_Default)
		gamma = m_ForceTextureGamma;

	if(gamma != m_VertexSampler[sampler].m_Gamma)
	{
		assert(m_pd3dDevice);		
		DWORD value = (gamma==E_Bool_True);
		m_pd3dDevice->SetSamplerState(D3DVERTEXTEXTURESAMPLER0+sampler, D3DSAMP_SRGBTEXTURE, value);

		m_VertexSampler[sampler].m_Gamma = gamma;
	}
#endif
}

INLINE void C_D3D9Device::SetVertexBuffer(const U32 stream, const C_VertexBuffer* pVertexBuffer, const U32 offset, const U32 stride)
{
	assert(stream < E_Stream_Count);

	if(pVertexBuffer != m_Stream[stream].m_pVertexBuffer || stride != m_Stream[stream].m_VertexStride || offset != m_Stream[stream].m_VertexOffset)
	{
		assert(m_pd3dDevice);
		m_pd3dDevice->SetStreamSource(stream, pVertexBuffer ? (Display2::GPUVertexBuffer*)pVertexBuffer->GetGPUVertexBuffer() : NULL, offset, stride);
	
		m_Stream[stream].m_pVertexBuffer = pVertexBuffer;
		m_Stream[stream].m_VertexOffset = offset;
		m_Stream[stream].m_VertexStride = stride;
	}
}

INLINE void C_D3D9Device::SetVertexDeclaration(const C_VertexDeclaration* pVertexDeclaration)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetVertexDeclaration(pVertexDeclaration ? pVertexDeclaration->m_pd3dVertexDeclaration : NULL);
	m_pVertexDeclaration = pVertexDeclaration;
}

INLINE const C_VertexShader* C_D3D9Device::GetVertexShader()
{
	return m_pVertexShader;
}

INLINE void C_D3D9Device::SetVertexShader(const C_VertexShader* pVertexShader)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetVertexShader(pVertexShader ? (Display2::GPUVertexShader*)pVertexShader->GetGPUVertexShader() : NULL);
	m_pVertexShader = pVertexShader;
}

INLINE void C_D3D9Device::SetVertexShaderConstantB(const U32 startRegister, const BOOL* pData, const U32 registerCount)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetVertexShaderConstantB(startRegister, pData, registerCount);
}

INLINE void C_D3D9Device::SetVertexShaderConstantI(const U32 startRegister, const S32* pData, const U32 registerCount)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetVertexShaderConstantI(startRegister, pData, registerCount);
}

INLINE void C_D3D9Device::SetVertexShaderConstantF(const U32 startRegister, const F32* pData, const U32 registerCount)
{
	assert(m_pd3dDevice);
	m_pd3dDevice->SetVertexShaderConstantF(startRegister, pData, registerCount);
}

INLINE void C_D3D9Device::SetVertexShaderParameterF(const C_VertexShaderParameter startParameter, const F32* pData, const U32 parameterCount)
{
	assert(m_pd3dDevice);
	if(startParameter.m_RegisterIndex != UINT_MAX && parameterCount != 0)
		m_pd3dDevice->SetVertexShaderConstantF(startParameter.m_RegisterIndex, pData, parameterCount);
}

//INLINE void C_D3D9Device::SetVertexShaderParameterArray(const U32 startRegister, const F32* pData, const U32 registerCount, const C_ShaderParameter* pParameters)
//{
//	assert(m_pd3dDevice);
//	m_pd3dDevice->SetVertexShaderConstantF(startRegister, pData, registerCount);
//}

INLINE void C_D3D9Device::SetViewport(const C_Viewport& viewport)
{
	m_Viewport = viewport;

	D3DVIEWPORT9 d3dViewport;
	d3dViewport.X		= viewport.GetX();
	d3dViewport.Y		= viewport.GetY();
	d3dViewport.Width	= viewport.GetW();
	d3dViewport.Height	= viewport.GetH();	
	//d3dViewport.MinZ	= m_DepthViewportFlip ? viewport.GetMaxZ() : viewport.GetMinZ();
	//d3dViewport.MaxZ	= m_DepthViewportFlip ? viewport.GetMinZ() : viewport.GetMaxZ();
	d3dViewport.MinZ	= viewport.GetMinZ();
	d3dViewport.MaxZ	= viewport.GetMaxZ();
	assert(m_pd3dDevice);
	m_pd3dDevice->SetViewport(&d3dViewport);
}

INLINE void C_D3D9Device::SetColourDepthStencilSurface()
{
	const C_BackBuffer& backBuffer = GetBackBuffer();
	SetColourDepthStencilSurface(&backBuffer.m_ColourSurface, &backBuffer.m_DepthStencilSurface);
	SetColourSurface(1, NULL);
	SetColourSurface(2, NULL);
	SetColourSurface(3, NULL);
}

INLINE void C_D3D9Device::SetColourDepthStencilSurface(const C_Surface* pColourSurface, const C_Surface* pDepthStencilSurface)
{
	SetColourSurface(pColourSurface);
	SetDepthStencilSurface(pDepthStencilSurface);
}

INLINE void C_D3D9Device::SetColourSurface()
{
	const C_BackBuffer& backBuffer = GetBackBuffer();
	SetColourSurface(&backBuffer.m_ColourSurface);
	SetColourSurface(1, NULL);
	SetColourSurface(2, NULL);
	SetColourSurface(3, NULL);
}

INLINE void C_D3D9Device::SetColourSurface(const C_Surface* pSurface)
{
	SetColourSurface(0, pSurface);
}

INLINE void C_D3D9Device::SetDepthStencilSurface()
{
	const C_BackBuffer& backBuffer = GetBackBuffer();
	SetDepthStencilSurface(&backBuffer.m_DepthStencilSurface);
}

INLINE bool C_D3D9Device::CreateColourSurface(const U32 width, const U32 height, const U32 flags, const E_Format format, const E_Multisample multisample, const C_SurfaceParams* pParams, C_Surface* pSurface)
{
	return CreateColourSurface(width, height, flags, D3D9Format(format), D3D9Multisample(multisample), pParams, pSurface);
}

INLINE void C_D3D9Device::DestroyColourSurface(C_Surface* pSurface)
{
	DestroySurface(pSurface);
}

INLINE bool C_D3D9Device::CreateDepthStencilSurface(const U32 width, const U32 height, const U32 flags, const E_Format format, const E_Multisample multisample, const C_SurfaceParams* pParams, C_Surface* pSurface)
{
	return CreateDepthStencilSurface(width, height, flags, D3D9Format(format), D3D9Multisample(multisample), pParams, pSurface);
}

INLINE void C_D3D9Device::DestroyDepthStencilSurface(C_Surface* pSurface)
{
	DestroySurface(pSurface);
}

INLINE const U32 C_D3D9Device::GetVertexDeclarationCount() const
{
	return m_Display2.GetVertexDeclarationCount();
}

INLINE const E_Bool C_D3D9Device::GetForceTextureGamma() const
{
	return m_ForceTextureGamma;
}

}; // namespace D3D9
}; // namespace Display

#endif // DISPLAY_DEVICE_D3D9_D3D9_DEVICE_INL
