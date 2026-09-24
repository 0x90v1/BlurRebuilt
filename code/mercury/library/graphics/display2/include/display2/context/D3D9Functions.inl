#define GPU_FUNC(_func)		GPU_CLASS::_func
#define GPU_DECL(_decl)		INLINE _decl
#define D3D_CALL(_func)		GetD3DDevice()->_func

//------------------------------------------------------------------------------------------------------------------------------------------------------

#if GPU_CHECK || GPU_TRACK

GPU_DECL(const C_AlphaTestState&) GPU_FUNC(GetAlphaTestState)() const
{
	return GetState()->m_AlphaTestState;
}

GPU_DECL(const C_BlendState&) GPU_FUNC(GetBlendState)() const
{
	return GetState()->m_BlendState;
}

GPU_DECL(const C_ColourState&) GPU_FUNC(GetColourState)() const
{
	return GetState()->m_ColourState;
}

GPU_DECL(const C_CullState&) GPU_FUNC(GetCullState)() const
{
	return GetState()->m_CullState;
}

GPU_DECL(const C_DepthState&) GPU_FUNC(GetDepthState)() const
{
	return GetState()->m_DepthState;
}

GPU_DECL(const C_FillState&) GPU_FUNC(GetFillState)() const
{
	return GetState()->m_FillState;
}

GPU_DECL(const C_TextureState&) GPU_FUNC(GetTextureState)(const U32 i) const
{
	GPU_ASSERT(i < E_Sampler_Count);
	return GetState()->m_TextureState[i];
}

GPU_DECL(const C_SamplerState&) GPU_FUNC(GetSamplerState)(const U32 i) const
{
	GPU_ASSERT(i < E_Sampler_Count);
	return GetState()->m_SamplerState[i];
}

GPU_DECL(const C_ScissorState&) GPU_FUNC(GetScissorState)() const
{
	return GetState()->m_ScissorState;
}

GPU_DECL(const C_StencilState&) GPU_FUNC(GetStencilState)() const
{
	return GetState()->m_StencilState;
}

GPU_DECL(const C_VertexTextureState&) GPU_FUNC(GetVertexTextureState)(const U32 i) const
{
	GPU_ASSERT(i < E_VertexSampler_Count);
	return GetState()->m_VertexTextureState[i];
}

GPU_DECL(const C_VertexSamplerState&) GPU_FUNC(GetVertexSamplerState)(const U32 i) const
{
	GPU_ASSERT(i < E_VertexSampler_Count);
	return GetState()->m_VertexSamplerState[i];
}

#endif

//------------------------------------------------------------------------------------------------------------------------------------------------------

GPU_DECL(GPU_VOID) GPU_FUNC(SetAlphaTestState)(const C_AlphaTestState& state)
{
	#if GPU_CHECK
	if(state.m_AlphaTestBits != GetState()->m_AlphaTestState.m_AlphaTestBits)
	#endif
	{
		GPU_ASSERT_CMPFUNC(state.m_AlphaTestCmpFunc);
		GPU_ASSERT(state.m_AlphaTestRef <= 0x000000FF);
		
		#if GPU_CHECK
		if(state.m_AlphaTestCmpFunc != GetState()->m_AlphaTestState.m_AlphaTestCmpFunc)
		#endif
			D3D_CALL(SetRenderState(D3DRS_ALPHAFUNC, state.m_AlphaTestCmpFunc));
		
		#if GPU_CHECK
		if(state.m_AlphaTestRef != GetState()->m_AlphaTestState.m_AlphaTestRef)
		#endif
			D3D_CALL(SetRenderState(D3DRS_ALPHAREF, state.m_AlphaTestRef));

		#if GPU_TRACK
		GetState()->m_AlphaTestState.m_AlphaTestBits = state.m_AlphaTestBits;
		#endif
	}

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetBlendState)(const C_BlendState& state)
{
	#if GPU_CHECK
	if(state.m_BlendBits != GetState()->m_BlendState.m_BlendBits)
	#endif
	{
		GPU_ASSERT_BLEND(state.m_BlendColorSrc);
		GPU_ASSERT_BLEND(state.m_BlendColorDst);
		GPU_ASSERT_BLENDOP(state.m_BlendColorOp);
		GPU_ASSERT_BLEND(state.m_BlendAlphaSrc);
		GPU_ASSERT_BLEND(state.m_BlendAlphaDst);
		GPU_ASSERT_BLENDOP(state.m_BlendAlphaOp);
		
		#if GPU_CHECK
		if(state.m_BlendColorSrc != GetState()->m_BlendState.m_BlendColorSrc)
		#endif
			D3D_CALL(SetRenderState(D3DRS_SRCBLEND, state.m_BlendColorSrc));

		#if GPU_CHECK
		if(state.m_BlendColorDst != GetState()->m_BlendState.m_BlendColorDst)
		#endif
			D3D_CALL(SetRenderState(D3DRS_DESTBLEND, state.m_BlendColorDst));

		#if GPU_CHECK
		if(state.m_BlendColorOp != GetState()->m_BlendState.m_BlendColorOp)
		#endif
			D3D_CALL(SetRenderState(D3DRS_BLENDOP, state.m_BlendColorOp));

		#if GPU_CHECK
		if(state.m_BlendAlphaSrc != GetState()->m_BlendState.m_BlendAlphaSrc)
		#endif
			D3D_CALL(SetRenderState(D3DRS_SRCBLENDALPHA, state.m_BlendAlphaSrc));

		#if GPU_CHECK
		if(state.m_BlendAlphaDst != GetState()->m_BlendState.m_BlendAlphaDst)
		#endif
			D3D_CALL(SetRenderState(D3DRS_DESTBLENDALPHA, state.m_BlendAlphaDst));

		#if GPU_CHECK
		if(state.m_BlendAlphaOp != GetState()->m_BlendState.m_BlendAlphaOp)
		#endif
			D3D_CALL(SetRenderState(D3DRS_BLENDOPALPHA, state.m_BlendAlphaOp));

		#if GPU_TRACK
		GetState()->m_BlendState.m_BlendBits = state.m_BlendBits;
		#endif
	}

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetColourState)(const C_ColourState& state)
{
	#if GPU_CHECK
	if(state.m_ColourWriteEnable != GetState()->m_ColourState.m_ColourWriteEnable)
	#endif
	{
		D3D_CALL(SetRenderState(D3DRS_COLORWRITEENABLE, state.m_ColourWriteEnable));

		#if GPU_TRACK
		GetState()->m_ColourState.m_ColourWriteEnable = state.m_ColourWriteEnable;
		#endif
	}

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetCullState)(const C_CullState& state)
{
	GPU_ASSERT_CULLMODE(state.m_CullMode);
	
	#if GPU_CHECK
	if(state.m_CullMode != GetState()->m_CullState.m_CullMode)
	#endif
	{
		D3D_CALL(SetRenderState(D3DRS_CULLMODE, state.m_CullMode));

		#if GPU_TRACK
		GetState()->m_CullState.m_CullMode = state.m_CullMode;
		#endif
	}

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetDepthState)(const C_DepthState& state)
{
	#if GPU_CHECK
	if(state.m_DepthBits != GetState()->m_DepthState.m_DepthBits)
	#endif
	{
		GPU_ASSERT_BOOL(state.m_DepthEnable);
		GPU_ASSERT_BOOL(state.m_DepthWriteEnable);
		GPU_ASSERT_CMPFUNC(state.m_DepthFunc);
		
		#if GPU_CHECK
		if(state.m_DepthEnable != GetState()->m_DepthState.m_DepthEnable)
		#endif
			D3D_CALL(SetRenderState(D3DRS_ZENABLE, state.m_DepthEnable == E_Bool_True ? D3DZB_TRUE : D3DZB_FALSE));

		#if GPU_CHECK
		if(state.m_DepthWriteEnable != GetState()->m_DepthState.m_DepthWriteEnable)
		#endif
			D3D_CALL(SetRenderState(D3DRS_ZWRITEENABLE, state.m_DepthWriteEnable));

		#if GPU_CHECK
		if(state.m_DepthFunc != GetState()->m_DepthState.m_DepthFunc)
		#endif
			D3D_CALL(SetRenderState(D3DRS_ZFUNC, state.m_DepthFunc));
		
		#if GPU_TRACK
		GetState()->m_DepthState.m_DepthBits = state.m_DepthBits;
		#endif
	}

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetFillState)(const C_FillState& state)
{
	GPU_ASSERT_FILLMODE(state.m_FillMode);
	
	#if GPU_CHECK
	if(state.m_FillMode != GetState()->m_FillState.m_FillMode)
	#endif
	{
		D3D_CALL(SetRenderState(D3DRS_FILLMODE, state.m_FillMode));

		#if GPU_TRACK
		GetState()->m_FillState.m_FillMode = state.m_FillMode;
		#endif
	}

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetTextureState)(const U32 index, const C_TextureState& state)
{
	GPU_ASSERT(index < E_Sampler_Count);
	GPU_ASSERT_TEXTURE(state.m_pTexture);
		
	#if GPU_CHECK
	if(state.m_pTexture != GetState()->m_TextureState[index].m_pTexture)
	#endif
	{
		D3D_CALL(SetTexture(index, state.m_pTexture ? (GPUTexture*)state.m_pTexture->GetGPUTexture() : NULL));

		#if GPU_TRACK
		GetState()->m_TextureState[index].m_pTexture = state.m_pTexture;
		#endif
	}

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetSamplerState)(const U32 index, const C_SamplerState& state)
{
	GPU_ASSERT(index < E_Sampler_Count);

	#if GPU_CHECK
	if(state.m_TextureBits != GetState()->m_SamplerState[index].m_TextureBits)
	#endif
	{
		GPU_ASSERT_TEXTUREADDRESS(state.m_TextureAddressU);
		GPU_ASSERT_TEXTUREADDRESS(state.m_TextureAddressV);
		GPU_ASSERT_TEXTUREADDRESS(state.m_TextureAddressW);
		GPU_ASSERT_TEXTUREFILTER(state.m_TextureFilterMag);
		GPU_ASSERT_TEXTUREFILTER(state.m_TextureFilterMin);
		GPU_ASSERT_TEXTUREFILTER(state.m_TextureFilterMip);
		
		#if GPU_CHECK
		if(state.m_TextureAddressU != GetState()->m_SamplerState[index].m_TextureAddressU)
		#endif
			D3D_CALL(SetSamplerState(index, D3DSAMP_ADDRESSU, state.m_TextureAddressU));

		#if GPU_CHECK
		if(state.m_TextureAddressV != GetState()->m_SamplerState[index].m_TextureAddressV)
		#endif
			D3D_CALL(SetSamplerState(index, D3DSAMP_ADDRESSV, state.m_TextureAddressV));

		#if GPU_CHECK
		if(state.m_TextureAddressW != GetState()->m_SamplerState[index].m_TextureAddressW)
		#endif
			D3D_CALL(SetSamplerState(index, D3DSAMP_ADDRESSW, state.m_TextureAddressW));

		#if GPU_CHECK
		if(state.m_TextureFilterMag != GetState()->m_SamplerState[index].m_TextureFilterMag)
		#endif
			D3D_CALL(SetSamplerState(index, D3DSAMP_MAGFILTER, state.m_TextureFilterMag));

		#if GPU_CHECK
		if(state.m_TextureFilterMin != GetState()->m_SamplerState[index].m_TextureFilterMin)
		#endif
			D3D_CALL(SetSamplerState(index, D3DSAMP_MINFILTER, state.m_TextureFilterMin));

		#if GPU_CHECK
		if(state.m_TextureFilterMip != GetState()->m_SamplerState[index].m_TextureFilterMip)
		#endif
			D3D_CALL(SetSamplerState(index, D3DSAMP_MIPFILTER, state.m_TextureFilterMip));

	#if DEVICE_D3D9_PC
		#if GPU_CHECK
		if(state.m_TextureGamma != GetState()->m_SamplerState[index].m_TextureGamma)
		#endif
			D3D_CALL(SetSamplerState(index, D3DSAMP_SRGBTEXTURE, state.m_TextureGamma));
	#endif

		#if GPU_TRACK
		GetState()->m_SamplerState[index].m_TextureBits = state.m_TextureBits;
		#endif
	}

	GPU_ASSERT_TEXTUREBORDERCOLOUR(state.m_TextureBorderColour);
	
	#if GPU_CHECK
	if(state.m_TextureBorderColour != GetState()->m_SamplerState[index].m_TextureBorderColour)
	#endif
	{
		D3D_CALL(SetSamplerState(index, D3DSAMP_BORDERCOLOR, state.m_TextureBorderColour));

		#if GPU_TRACK
		GetState()->m_SamplerState[index].m_TextureBorderColour = state.m_TextureBorderColour;
		#endif
	}

	#if GPU_CHECK
	if(state.m_TextureLodBias != GetState()->m_SamplerState[index].m_TextureLodBias)
	#endif
	{
		const F32 fTextureLodBias = (((F32)(state.m_TextureLodBias))/128.0f)*8.0f;
		GPU_ASSERT(fTextureLodBias >= -16.0f && fTextureLodBias <= 15.96875f);
		
		D3D_CALL(SetSamplerState(index, D3DSAMP_MIPMAPLODBIAS, (DWORD&)fTextureLodBias));

		#if GPU_TRACK
		GetState()->m_SamplerState[index].m_TextureLodBias = state.m_TextureLodBias;
		#endif
	}

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetScissorState)(const C_ScissorState& state)
{
	GPU_ASSERT_BOOL(state.m_ScissorEnable);

	#if GPU_CHECK
	if(state.m_ScissorEnable != GetState()->m_ScissorState.m_ScissorEnable)
	#endif
	{
		D3D_CALL(SetRenderState(D3DRS_SCISSORTESTENABLE, (BOOL)state.m_ScissorEnable));

		#if GPU_TRACK
		GetState()->m_ScissorState.m_ScissorEnable = state.m_ScissorEnable;
		#endif
	}

	GPU_CALL(SetScissorRect(state.m_ScissorRect)); // Do we want to track this?

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetStencilState)(const C_StencilState& state)
{
	#if GPU_CHECK
	if(state.m_StencilBits != GetState()->m_StencilState.m_StencilBits)
	#endif
	{
		GPU_ASSERT_CMPFUNC(state.m_StencilFunc);
		GPU_ASSERT_STENCILOP(state.m_StencilFail);
		GPU_ASSERT_STENCILOP(state.m_StencilDepthFail);
		GPU_ASSERT_STENCILOP(state.m_StencilDepthPass);
		
		#if GPU_CHECK
		if(state.m_StencilFunc != GetState()->m_StencilState.m_StencilFunc)
		#endif
			D3D_CALL(SetRenderState(D3DRS_STENCILFUNC, state.m_StencilFunc));
		
		#if GPU_CHECK
		if(state.m_StencilFail != GetState()->m_StencilState.m_StencilFail)
		#endif
			D3D_CALL(SetRenderState(D3DRS_STENCILFAIL, state.m_StencilFail));
		
		#if GPU_CHECK
		if(state.m_StencilDepthFail != GetState()->m_StencilState.m_StencilDepthFail)
		#endif
			D3D_CALL(SetRenderState(D3DRS_STENCILZFAIL, state.m_StencilDepthFail));
		
		#if GPU_CHECK
		if(state.m_StencilDepthPass != GetState()->m_StencilState.m_StencilDepthPass)
		#endif
			D3D_CALL(SetRenderState(D3DRS_STENCILPASS, state.m_StencilDepthPass));

		#if GPU_TRACK
		GetState()->m_StencilState.m_StencilBits = state.m_StencilBits;
		#endif
	}

	#if GPU_CHECK
	if(state.m_StencilRef != GetState()->m_StencilState.m_StencilRef)
	#endif
	{
		D3D_CALL(SetRenderState(D3DRS_STENCILREF, state.m_StencilRef));

		#if GPU_TRACK
		GetState()->m_StencilState.m_StencilRef = state.m_StencilRef;
		#endif
	}
	
	#if GPU_CHECK
	if(state.m_StencilMask != GetState()->m_StencilState.m_StencilMask)
	#endif
	{
		D3D_CALL(SetRenderState(D3DRS_STENCILMASK, state.m_StencilMask));

		#if GPU_TRACK
		GetState()->m_StencilState.m_StencilMask = state.m_StencilMask;
		#endif
	}
	
	#if GPU_CHECK
	if(state.m_StencilWriteMask != GetState()->m_StencilState.m_StencilWriteMask)
	#endif
	{
		D3D_CALL(SetRenderState(D3DRS_STENCILWRITEMASK, state.m_StencilWriteMask));

		#if GPU_TRACK
		GetState()->m_StencilState.m_StencilWriteMask = state.m_StencilWriteMask;
		#endif
	}

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexTextureState)(const U32 index, const C_VertexTextureState& state)
{
	GPU_ASSERT(index < E_VertexSampler_Count);
	GPU_ASSERT_TEXTURE(state.m_pTexture);

	#if GPU_CHECK
	if(state.m_pTexture != GetState()->m_VertexTextureState[index].m_pTexture)
	#endif
	{
		D3D_CALL(SetTexture(D3DVERTEXTEXTURESAMPLER0+index, state.m_pTexture ? (GPUTexture*)state.m_pTexture->GetGPUTexture() : NULL));

		#if GPU_TRACK
		GetState()->m_VertexTextureState[index].m_pTexture = state.m_pTexture;
		#endif
	}

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexSamplerState)(const U32 index, const C_VertexSamplerState& state)
{
	GPU_ASSERT(index < E_VertexSampler_Count);

	#if GPU_CHECK
	if(state.m_TextureBits != GetState()->m_VertexSamplerState[index].m_TextureBits)
	#endif
	{
		GPU_ASSERT_TEXTUREADDRESS(state.m_TextureAddressU);
		GPU_ASSERT_TEXTUREADDRESS(state.m_TextureAddressV);
		GPU_ASSERT_TEXTUREADDRESS(state.m_TextureAddressW);
		GPU_ASSERT_TEXTUREFILTER(state.m_TextureFilterMag);
		GPU_ASSERT_TEXTUREFILTER(state.m_TextureFilterMin);
		GPU_ASSERT_TEXTUREFILTER(state.m_TextureFilterMip);

		#if GPU_CHECK
		if(state.m_TextureAddressU != GetState()->m_VertexSamplerState[index].m_TextureAddressU)
		#endif
			D3D_CALL(SetSamplerState(D3DVERTEXTEXTURESAMPLER0+index, D3DSAMP_ADDRESSU, state.m_TextureAddressU));

		#if GPU_CHECK
		if(state.m_TextureAddressV != GetState()->m_VertexSamplerState[index].m_TextureAddressV)
		#endif
			D3D_CALL(SetSamplerState(D3DVERTEXTEXTURESAMPLER0+index, D3DSAMP_ADDRESSV, state.m_TextureAddressV));

		#if GPU_CHECK
		if(state.m_TextureAddressW != GetState()->m_VertexSamplerState[index].m_TextureAddressW)
		#endif
			D3D_CALL(SetSamplerState(D3DVERTEXTEXTURESAMPLER0+index, D3DSAMP_ADDRESSW, state.m_TextureAddressW));

		#if GPU_CHECK
		if(state.m_TextureFilterMag != GetState()->m_VertexSamplerState[index].m_TextureFilterMag)
		#endif
			D3D_CALL(SetSamplerState(D3DVERTEXTEXTURESAMPLER0+index, D3DSAMP_MAGFILTER, state.m_TextureFilterMag));

		#if GPU_CHECK
		if(state.m_TextureFilterMin != GetState()->m_VertexSamplerState[index].m_TextureFilterMin)
		#endif
			D3D_CALL(SetSamplerState(D3DVERTEXTEXTURESAMPLER0+index, D3DSAMP_MINFILTER, state.m_TextureFilterMin));

		#if GPU_CHECK
		if(state.m_TextureFilterMip != GetState()->m_VertexSamplerState[index].m_TextureFilterMip)
		#endif
			D3D_CALL(SetSamplerState(D3DVERTEXTEXTURESAMPLER0+index, D3DSAMP_MIPFILTER, state.m_TextureFilterMip));

		#if DEVICE_D3D9_PC
		#if GPU_CHECK
		if(state.m_TextureGamma != GetState()->m_VertexSamplerState[index].m_TextureGamma)
		#endif
			D3D_CALL(SetSamplerState(D3DVERTEXTEXTURESAMPLER0+index, D3DSAMP_SRGBTEXTURE, state.m_TextureGamma));
		#endif

		#if GPU_TRACK
		GetState()->m_VertexSamplerState[index].m_TextureBits = state.m_TextureBits;
		#endif
	}
	
	#if DEVICE_XB2
	GPU_ASSERT(state.m_TextureBorderColour == 0x00000000 || state.m_TextureBorderColour == 0xFFFFFFFF);
	#endif

	#if GPU_CHECK
	if(state.m_TextureBorderColour != GetState()->m_VertexSamplerState[index].m_TextureBorderColour)
	#endif
	{
		D3D_CALL(SetSamplerState(D3DVERTEXTEXTURESAMPLER0+index, D3DSAMP_BORDERCOLOR, state.m_TextureBorderColour));

		#if GPU_TRACK
		GetState()->m_VertexSamplerState[index].m_TextureBorderColour = state.m_TextureBorderColour;
		#endif
	}

#if DEVICE_XB2
	#if GPU_CHECK
	if(state.m_TextureLodBias != GetState()->m_VertexSamplerState[index].m_TextureLodBias)
	#endif
	{
		const F32 fTextureLodBias = (((F32)(state.m_TextureLodBias))/128.0f)*8.0f;
		GPU_ASSERT(fTextureLodBias >= -16.0f && fTextureLodBias <= 15.96875f);

		D3D_CALL(SetSamplerState(D3DVERTEXTEXTURESAMPLER0+index, D3DSAMP_MIPMAPLODBIAS, (DWORD&)fTextureLodBias));

		#if GPU_TRACK
		GetState()->m_VertexSamplerState[index].m_TextureLodBias = state.m_TextureLodBias;
		#endif
	}
#endif

	GPU_RETURN_VOID;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------

GPU_DECL(GPU_VOID) GPU_FUNC(BeginScene)()
{
	D3D_CALL(BeginScene());

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(Clear)(const U32 flags, const U32 colour, const F32 depth, const U8 stencil)
{
	D3D_CALL(Clear(0, NULL, flags, colour, depth, stencil));

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(DrawPrimitive)(const E_PrimitiveType primitiveType, const U32 startVertex, const U32 vertexCount)
{
	GPU_ASSERT_PRIMITIVETYPE(primitiveType);
	GPU_ASSERT(D3DPrimitiveCount(primitiveType, vertexCount) > 0);
	
	D3D_CALL(DrawPrimitive((D3DPRIMITIVETYPE)primitiveType, startVertex, D3DPrimitiveCount(primitiveType, vertexCount)));

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(DrawIndexedPrimitive)(const E_PrimitiveType primitiveType, const U32 baseVertex, const U32 minIndex, const U32 vertexCount, const U32 startIndex, const U32 indexCount, const C_IndexBuffer* pIndexBuffer)
{
	DrawIndexedPrimitive(primitiveType, baseVertex, minIndex, vertexCount, startIndex, indexCount);

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(DrawIndexedPrimitive)(const E_PrimitiveType primitiveType, const U32 baseVertex, const U32 minIndex, const U32 vertexCount, const U32 startIndex, const U32 indexCount)
{
	GPU_ASSERT_PRIMITIVETYPE(primitiveType);
	GPU_ASSERT(indexCount > 0);
	
#if PLATFORM_XB2
	D3D_CALL(DrawIndexedVertices((D3DPRIMITIVETYPE)primitiveType, baseVertex, startIndex, indexCount));
#else
	GPU_ASSERT(vertexCount > 0);
	GPU_ASSERT(D3DPrimitiveCount(primitiveType, indexCount) > 0);
	D3D_CALL(DrawIndexedPrimitive((D3DPRIMITIVETYPE)primitiveType, baseVertex, minIndex, vertexCount, startIndex, D3DPrimitiveCount(primitiveType, indexCount)));
#endif

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(EndScene)()
{
	D3D_CALL(EndScene());

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(Present)()
{
	D3D_CALL(Present(NULL, NULL, NULL, NULL));

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetAlphaTestEnable)(const E_Bool alphaTestEnable)
{
	GPU_ASSERT_BOOL(alphaTestEnable);

	#if GPU_CHECK
	if(alphaTestEnable == GetState()->m_AlphaTestEnable)
		GPU_RETURN_OK;
	#endif

	D3D_CALL(SetRenderState(D3DRS_ALPHATESTENABLE, (BOOL)alphaTestEnable));

	#if GPU_TRACK
	GetState()->m_AlphaTestEnable = alphaTestEnable;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetAlphaTestFunc)(const E_CmpFunc alphaTestFunc)
{
	GPU_ASSERT_CMPFUNC(alphaTestFunc);

	#if GPU_CHECK
	if(alphaTestFunc == GetState()->m_AlphaTestState.m_AlphaTestCmpFunc)
		GPU_RETURN_OK;
	#endif

	D3D_CALL(SetRenderState(D3DRS_ALPHAFUNC, alphaTestFunc));
	
	#if GPU_TRACK
	GetState()->m_AlphaTestState.m_AlphaTestCmpFunc = alphaTestFunc;
	#endif

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetAlphaTestRef)(const U32 alphaTestRef)
{
	GPU_ASSERT(alphaTestRef <= 0x000000FF);

	#if GPU_CHECK
	if(alphaTestRef == GetState()->m_AlphaTestState.m_AlphaTestRef)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetRenderState(D3DRS_ALPHAREF, alphaTestRef));
	
	#if GPU_TRACK
	GetState()->m_AlphaTestState.m_AlphaTestRef = alphaTestRef;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetBlendEnable)(const E_Bool blendEnable)
{
	GPU_ASSERT_BOOL(blendEnable);

	#if GPU_CHECK
	if(blendEnable == GetState()->m_BlendEnable)
		GPU_RETURN_OK;
	#endif

	D3D_CALL(SetRenderState(D3DRS_ALPHABLENDENABLE, blendEnable));
	
	#if GPU_TRACK
	GetState()->m_BlendEnable = blendEnable;
	#endif

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetBlendSrc)(const E_Blend blendSrc)
{
	GPU_ASSERT_BLEND(blendSrc);

	#if GPU_CHECK
	if(blendSrc == GetState()->m_BlendState.m_BlendColorSrc)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetRenderState(D3DRS_SRCBLEND, blendSrc));

	#if GPU_TRACK
	GetState()->m_BlendState.m_BlendColorSrc = blendSrc;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetBlendDst)(const E_Blend blendDst)
{
	GPU_ASSERT_BLEND(blendDst);

	#if GPU_CHECK
	if(blendDst == GetState()->m_BlendState.m_BlendColorDst)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetRenderState(D3DRS_DESTBLEND, blendDst));

	#if GPU_TRACK
	GetState()->m_BlendState.m_BlendColorDst = blendDst;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetBlendOp)(const E_BlendOp blendOp)
{
	GPU_ASSERT_BLENDOP(blendOp);

	#if GPU_CHECK
	if(blendOp == GetState()->m_BlendState.m_BlendColorOp)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetRenderState(D3DRS_BLENDOP, blendOp));
	
	#if GPU_TRACK
	GetState()->m_BlendState.m_BlendColorOp = blendOp;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetBlendSrcAlpha)(const E_Blend blendSrcAlpha)
{
	GPU_ASSERT_BLEND(blendSrcAlpha);

	#if GPU_CHECK
	if(blendSrcAlpha == GetState()->m_BlendState.m_BlendAlphaSrc)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetRenderState(D3DRS_SRCBLENDALPHA, blendSrcAlpha));
	
	#if GPU_TRACK
	GetState()->m_BlendState.m_BlendAlphaSrc = blendSrcAlpha;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetBlendDstAlpha)(const E_Blend blendDstAlpha)
{
	GPU_ASSERT_BLEND(blendDstAlpha);

	#if GPU_CHECK
	if(blendDstAlpha == GetState()->m_BlendState.m_BlendAlphaDst)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetRenderState(D3DRS_DESTBLENDALPHA, blendDstAlpha));

	#if GPU_TRACK
	GetState()->m_BlendState.m_BlendAlphaDst = blendDstAlpha;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetBlendOpAlpha)(const E_BlendOp blendOpAlpha)
{
	GPU_ASSERT_BLENDOP(blendOpAlpha);

	#if GPU_CHECK
	if(blendOpAlpha == GetState()->m_BlendState.m_BlendAlphaOp)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetRenderState(D3DRS_BLENDOPALPHA, blendOpAlpha));

	#if GPU_TRACK
	GetState()->m_BlendState.m_BlendAlphaOp = blendOpAlpha;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetColourSurface)(const U32 index, const C_Surface* pSurface)
{
	#if DEVICE_XB2
	GPU_ASSERT(index < 4);
	#endif

	D3D_CALL(SetRenderTarget(index, pSurface ? (GPUSurface*)pSurface->GetGPUSurface() : NULL));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetColourWriteEnable)(const U32 colourWriteEnable)
{
	#if GPU_CHECK
	if(colourWriteEnable == GetState()->m_ColourState.m_ColourWriteEnable)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetRenderState(D3DRS_COLORWRITEENABLE, colourWriteEnable));

	#if GPU_TRACK
	GetState()->m_ColourState.m_ColourWriteEnable = colourWriteEnable;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetCullMode)(const E_CullMode cullMode)
{
	GPU_ASSERT_CULLMODE(cullMode);

	#if GPU_CHECK
	if(cullMode == GetState()->m_CullState.m_CullMode)
		GPU_RETURN_OK;
	#endif

	D3D_CALL(SetRenderState(D3DRS_CULLMODE, cullMode));
	
	#if GPU_TRACK
	GetState()->m_CullState.m_CullMode = cullMode;
	#endif

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetDepthBias)(const F32 depthBias)
{
	D3D_CALL(SetRenderState(D3DRS_DEPTHBIAS, *((DWORD*)&depthBias)));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetDepthEnable)(const E_Bool depthEnable)
{
	GPU_ASSERT_BOOL(depthEnable);

	#if GPU_CHECK
	if(depthEnable == GetState()->m_DepthState.m_DepthEnable)
		GPU_RETURN_OK;
	#endif

	D3D_CALL(SetRenderState(D3DRS_ZENABLE, depthEnable == E_Bool_True ? D3DZB_TRUE : D3DZB_FALSE));

	#if GPU_TRACK
	GetState()->m_DepthState.m_DepthEnable = depthEnable;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetDepthFunc)(const E_CmpFunc depthFunc)
{
	GPU_ASSERT_CMPFUNC(depthFunc);

	#if GPU_CHECK
	if(depthFunc == GetState()->m_DepthState.m_DepthFunc)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetRenderState(D3DRS_ZFUNC, depthFunc));

	#if GPU_TRACK
	GetState()->m_DepthState.m_DepthFunc = depthFunc;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetDepthStencilSurface)(const C_Surface* pSurface)
{
	D3D_CALL(SetDepthStencilSurface(pSurface ? (GPUSurface*)pSurface : NULL));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetDepthWriteEnable)(const E_Bool depthWriteEnable)
{
	GPU_ASSERT_BOOL(depthWriteEnable);

	#if GPU_CHECK
	if(depthWriteEnable == GetState()->m_DepthState.m_DepthWriteEnable)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetRenderState(D3DRS_ZWRITEENABLE, depthWriteEnable));

	#if GPU_TRACK
	GetState()->m_DepthState.m_DepthWriteEnable = depthWriteEnable;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetFillMode)(const E_FillMode fillMode)
{
	GPU_ASSERT_FILLMODE(fillMode);

	#if GPU_CHECK
	if(fillMode == GetState()->m_FillState.m_FillMode)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetRenderState(D3DRS_FILLMODE, fillMode));
	
	#if GPU_TRACK
	GetState()->m_FillState.m_FillMode = fillMode;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetIndexBuffer)(const C_IndexBuffer* pIndexBuffer)
{
	#if GPU_CHECK
	if(pIndexBuffer == GetState()->m_pIndexBuffer)
		GPU_RETURN_OK;
	#endif

	D3D_CALL(SetIndices(pIndexBuffer ? (GPUIndexBuffer*)pIndexBuffer->GetGPUIndexBuffer() : NULL));
	
	#if GPU_TRACK
	GetState()->m_pIndexBuffer = pIndexBuffer;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetPixelShader)(const C_PixelShader* pPixelShader)
{
	#if GPU_CHECK
	if(pPixelShader == GetState()->m_pPixelShader)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetPixelShader(pPixelShader ? pPixelShader->m_pd3dPixelShader : NULL));
	
	#if GPU_TRACK
	GetState()->m_pPixelShader = pPixelShader;
	#endif

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetPixelShaderConstantB)(const U32 startRegister, const BOOL* pData, const U32 registerCount)
{
	GPU_ASSERT(startRegister < 256);
	GPU_ASSERT(pData);
	GPU_ASSERT(registerCount > 0);
	GPU_ASSERT(startRegister+registerCount <= 256);

	D3D_CALL(SetPixelShaderConstantB(startRegister, pData, registerCount));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetPixelShaderConstantI)(const U32 startRegister, const S32* pData, const U32 registerCount)
{
	GPU_ASSERT(startRegister < 16);
	GPU_ASSERT(pData);
	GPU_ASSERT(registerCount > 0);
	GPU_ASSERT(startRegister+registerCount <= 16);
	
	D3D_CALL(SetPixelShaderConstantI(startRegister, pData, registerCount));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetPixelShaderConstantF)(const U32 startRegister, const F32* pData, const U32 registerCount)
{
	GPU_ASSERT(startRegister < 256);
	GPU_ASSERT(pData);
	GPU_ASSERT(registerCount > 0);
	GPU_ASSERT(startRegister+registerCount <= 256);

	D3D_CALL(SetPixelShaderConstantF(startRegister, pData, registerCount));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetPixelShaderParamF)(const C_PixelShaderParam& shaderParam, const F32* pData, const U32 registerCount)
{
	if(shaderParam.m_RegisterIndex == UINT_MAX || shaderParam.m_RegisterCount == 0)
		GPU_RETURN_FAIL;

	GPU_CALL(SetPixelShaderConstantF(shaderParam.m_RegisterIndex, pData, shaderParam.m_RegisterCount));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetPixelShaderParamArrayF)(const C_PixelShaderParamArray& shaderParamArray, const F32* pData, const U32 registerCount)
{
	if(shaderParamArray.m_RegisterIndex == UINT_MAX || shaderParamArray.m_RegisterCount == 0)
		GPU_RETURN_FAIL;

	GPU_CALL(SetPixelShaderConstantF(shaderParamArray.m_RegisterIndex, pData, shaderParamArray.m_RegisterCount));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetPointSpriteEnable)(const E_Bool pointSpriteEnable)
{
	GPU_ASSERT_BOOL(pointSpriteEnable);

	#if GPU_CHECK
	if(pointSpriteEnable == GetState()->m_PointSpriteEnable)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetRenderState(D3DRS_POINTSPRITEENABLE, pointSpriteEnable));

	#if GPU_TRACK
	GetState()->m_PointSpriteEnable = pointSpriteEnable;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetRenderState)(const U32 renderState, const U32 value)
{
	D3D_CALL(SetRenderState((D3DRENDERSTATETYPE)renderState, value));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetSamplerState)(const U32 index, const U32 samplerState, const U32 value)
{
	BIZ_ASSERT(index < E_Sampler_Count);

	D3D_CALL(SetSamplerState(index, (D3DSAMPLERSTATETYPE )samplerState, value));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetScissorEnable)(const E_Bool scissorTestEnable)
{
	GPU_ASSERT_BOOL(scissorTestEnable);

	#if GPU_CHECK
	if(scissorTestEnable == GetState()->m_ScissorState.m_ScissorEnable)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetRenderState(D3DRS_SCISSORTESTENABLE, scissorTestEnable));
	
	#if GPU_TRACK
	GetState()->m_ScissorState.m_ScissorEnable = scissorTestEnable;
	#endif

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetScissorRect)(const RECT& rect)
{
	D3D_CALL(SetScissorRect(&rect));

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetSlopeScaledBias)(const F32 slopeScaledBias)
{
	D3D_CALL(SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *((DWORD*)&slopeScaledBias)));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetStencilEnable)(const E_Bool stencilEnable)
{
	GPU_ASSERT_BOOL(stencilEnable);

	#if GPU_CHECK
	if(stencilEnable == GetState()->m_StencilEnable)
		GPU_RETURN_OK;
	#endif

	D3D_CALL(SetRenderState(D3DRS_STENCILENABLE, stencilEnable));

	#if GPU_TRACK
	GetState()->m_StencilEnable = stencilEnable;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetStencilFail)(const E_StencilOp stencilFail)
{
	GPU_ASSERT_STENCILOP(stencilFail);

	#if GPU_CHECK
	if(stencilFail == GetState()->m_StencilState.m_StencilFail)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetRenderState(D3DRS_STENCILFAIL, stencilFail));
	
	#if GPU_TRACK
	GetState()->m_StencilState.m_StencilFail = stencilFail;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetStencilFunc)(const E_CmpFunc stencilFunc)
{
	GPU_ASSERT_CMPFUNC(stencilFunc);

	#if GPU_CHECK
	if(stencilFunc == GetState()->m_StencilState.m_StencilFunc)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetRenderState(D3DRS_STENCILFUNC, stencilFunc));
	
	#if GPU_TRACK
	GetState()->m_StencilState.m_StencilFunc = stencilFunc;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetStencilDepthFail)(const E_StencilOp stencilDepthFail)
{
	GPU_ASSERT_STENCILOP(stencilDepthFail);

	#if GPU_CHECK
	if(stencilDepthFail == GetState()->m_StencilState.m_StencilDepthFail)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetRenderState(D3DRS_STENCILZFAIL, stencilDepthFail));
	
	#if GPU_TRACK
	GetState()->m_StencilState.m_StencilDepthFail = stencilDepthFail;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetStencilPass)(const E_StencilOp stencilPass)
{
	GPU_ASSERT_STENCILOP(stencilPass);

	#if GPU_CHECK
	if(stencilPass == GetState()->m_StencilState.m_StencilDepthPass)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetRenderState(D3DRS_STENCILPASS, stencilPass));

	#if GPU_TRACK
	GetState()->m_StencilState.m_StencilDepthPass = stencilPass;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetStencilRef)(const U32 stencilRef)
{
	#if GPU_CHECK
	if(stencilRef == GetState()->m_StencilState.m_StencilRef)
		GPU_RETURN_OK;
	#endif

	D3D_CALL(SetRenderState(D3DRS_STENCILREF, stencilRef));
	
	#if GPU_TRACK
	GetState()->m_StencilState.m_StencilRef = stencilRef;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetStencilMask)(const U32 stencilMask)
{
	#if GPU_CHECK
	if(stencilMask == GetState()->m_StencilState.m_StencilMask)
		GPU_RETURN_OK;
	#endif

	D3D_CALL(SetRenderState(D3DRS_STENCILMASK, stencilMask));
	
	#if GPU_TRACK
	GetState()->m_StencilState.m_StencilMask = stencilMask;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetStencilWriteMask)(const U32 stencilWriteMask)
{
	#if GPU_CHECK
	if(stencilWriteMask == GetState()->m_StencilState.m_StencilWriteMask)
		GPU_RETURN_OK;
	#endif

	D3D_CALL(SetRenderState(D3DRS_STENCILWRITEMASK, stencilWriteMask));
	
	#if GPU_TRACK
	GetState()->m_StencilState.m_StencilWriteMask = stencilWriteMask;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetStencilCcwEnable)(const E_Bool stencilCcwEnable)
{
	GPU_ASSERT_BOOL(stencilCcwEnable);

	D3D_CALL(SetRenderState(D3DRS_TWOSIDEDSTENCILMODE , stencilCcwEnable));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetStencilCcwFail)(const E_StencilOp stencilFail)
{
	GPU_ASSERT_STENCILOP(stencilFail);

	D3D_CALL(SetRenderState(D3DRS_CCW_STENCILFAIL, stencilFail));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetStencilCcwDepthFail)(const E_StencilOp stencilDepthFail)
{
	GPU_ASSERT_STENCILOP(stencilDepthFail);

	D3D_CALL(SetRenderState(D3DRS_CCW_STENCILZFAIL, stencilDepthFail));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetStencilCcwPass)(const E_StencilOp stencilPass)
{
	GPU_ASSERT_STENCILOP(stencilPass);

	D3D_CALL(SetRenderState(D3DRS_CCW_STENCILPASS, stencilPass));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetStencilCcwFunc)(const E_CmpFunc stencilFunc)
{
	GPU_ASSERT_CMPFUNC(stencilFunc);

	D3D_CALL(SetRenderState(D3DRS_CCW_STENCILFUNC, stencilFunc));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetTexture)(const U32 index, const C_Texture* pTexture)
{
	GPU_ASSERT(index < E_Sampler_Count);
	GPU_ASSERT_TEXTURE(pTexture);
	
	#if GPU_CHECK
	if(pTexture == GetState()->m_TextureState[index].m_pTexture)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetTexture(index, pTexture ? (GPUTexture*)pTexture->GetGPUTexture() : NULL));
	
	#if GPU_TRACK
	GetState()->m_TextureState[index].m_pTexture = pTexture;
	#endif

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetTextureAddressU)(const U32 index, const E_TextureAddress addressU)
{
	GPU_ASSERT(index < E_Sampler_Count);
	GPU_ASSERT_TEXTUREADDRESS(addressU);

	#if GPU_CHECK
	if(addressU == GetState()->m_SamplerState[index].m_TextureAddressU)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetSamplerState(index, D3DSAMP_ADDRESSU, addressU));
	
	#if GPU_TRACK
	GetState()->m_SamplerState[index].m_TextureAddressU = addressU;
	#endif

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetTextureAddressV)(const U32 index, const E_TextureAddress addressV)
{
	GPU_ASSERT(index < E_Sampler_Count);
	GPU_ASSERT_TEXTUREADDRESS(addressV);
	
	#if GPU_CHECK
	if(addressV == GetState()->m_SamplerState[index].m_TextureAddressV)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetSamplerState(index, D3DSAMP_ADDRESSV, addressV));
	
	#if GPU_TRACK
	GetState()->m_SamplerState[index].m_TextureAddressV = addressV;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetTextureAddressW)(const U32 index, const E_TextureAddress addressW)
{
	GPU_ASSERT(index < E_Sampler_Count);
	GPU_ASSERT_TEXTUREADDRESS(addressW);
	
	#if GPU_CHECK
	if(addressW == GetState()->m_SamplerState[index].m_TextureAddressW)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetSamplerState(index, D3DSAMP_ADDRESSW, addressW));
	
	#if GPU_TRACK
	GetState()->m_SamplerState[index].m_TextureAddressW = addressW;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetTextureFilterMag)(const U32 index, const E_TextureFilter filterMag)
{
	GPU_ASSERT(index < E_Sampler_Count);
	GPU_ASSERT_TEXTUREFILTER(filterMag);
	
	#if GPU_CHECK
	if(filterMag == GetState()->m_SamplerState[index].m_TextureFilterMag)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetSamplerState(index, D3DSAMP_MAGFILTER, filterMag));
	
	#if GPU_TRACK
	GetState()->m_SamplerState[index].m_TextureFilterMag = filterMag;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetTextureFilterMin)(const U32 index, const E_TextureFilter filterMin)
{
	GPU_ASSERT(index < E_Sampler_Count);
	GPU_ASSERT_TEXTUREFILTER(filterMin);
	
	#if GPU_CHECK
	if(filterMin == GetState()->m_SamplerState[index].m_TextureFilterMin)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetSamplerState(index, D3DSAMP_MINFILTER, filterMin));
	
	#if GPU_TRACK
	GetState()->m_SamplerState[index].m_TextureFilterMin = filterMin;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetTextureFilterMip)(const U32 index, const E_TextureFilter filterMip)
{
	GPU_ASSERT(index < E_Sampler_Count);
	GPU_ASSERT_TEXTUREFILTER(filterMip);
	
	#if GPU_CHECK
	if(filterMip == GetState()->m_SamplerState[index].m_TextureFilterMip)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetSamplerState(index, D3DSAMP_MIPFILTER, filterMip));
	
	#if GPU_TRACK
	GetState()->m_SamplerState[index].m_TextureFilterMip = filterMip;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetTextureBorderColour)(const U32 index, const U32 borderColour)
{
	GPU_ASSERT(index < E_Sampler_Count);
	GPU_ASSERT_TEXTUREBORDERCOLOUR(borderColour);
	
	#if GPU_CHECK
	if(borderColour == GetState()->m_SamplerState[index].m_TextureBorderColour)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetSamplerState(index, D3DSAMP_BORDERCOLOR, borderColour));
	
	#if GPU_TRACK
	GetState()->m_SamplerState[index].m_TextureBorderColour = borderColour;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetTextureLodBias)(const U32 index, const S8 lodBias)
{
	GPU_ASSERT(index < E_Sampler_Count);
	
	#if GPU_CHECK
	if(lodBias == GetState()->m_SamplerState[index].m_TextureLodBias)
		GPU_RETURN_OK;
	#endif
	
	const F32 fTextureLodBias = (((F32)(lodBias))/128.0f)*8.0f;
	GPU_ASSERT(fTextureLodBias >= -16.0f && fTextureLodBias <= 15.96875f);

	D3D_CALL(SetSamplerState(index, D3DSAMP_MIPMAPLODBIAS, (DWORD&)fTextureLodBias));
	
	#if GPU_TRACK
	GetState()->m_SamplerState[index].m_TextureLodBias = lodBias;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexBuffer)(const U32 stream, const C_VertexBuffer* pVertexBuffer, const U32 offset, const U32 stride, const C_VertexDeclaration* pVertexDeclaration)
{
	SetVertexBuffer(stream, pVertexBuffer, offset, stride);
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexBuffer)(const U32 stream, const C_VertexBuffer* pVertexBuffer, const U32 offset, const U32 stride)
{
	BIZ_ASSERT(stream < E_Stream_Count);

	#if GPU_CHECK
	if(	pVertexBuffer == GetState()->m_pVertexBuffer[stream] &&
		offset == GetState()->m_VertexOffset[stream] &&
		stride == GetState()->m_VertexStride[stream])
		GPU_RETURN_OK;
	#endif

	D3D_CALL(SetStreamSource(stream, pVertexBuffer ? (GPUVertexBuffer*)pVertexBuffer->GetGPUVertexBuffer() : NULL, offset, stride));

	#if GPU_TRACK
	GetState()->m_pVertexBuffer[stream]	= pVertexBuffer;
	GetState()->m_VertexOffset[stream]	= offset;
	GetState()->m_VertexStride[stream]	= stride;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexDeclaration)(const C_VertexDeclaration* pVertexDeclaration)
{
	#if GPU_CHECK
	if(pVertexDeclaration == GetState()->m_pVertexDeclaration)
		GPU_RETURN_OK;
	#endif
	
	#if DEVICE_D3D9_PC
	if(pVertexDeclaration && pVertexDeclaration->IsCreated())
		D3D_CALL(SetVertexDeclaration(pVertexDeclaration->m_pd3dVertexDeclaration));
	#else
	D3D_CALL(SetVertexDeclaration(pVertexDeclaration ? pVertexDeclaration->m_pd3dVertexDeclaration : NULL));
	#endif
	
	#if GPU_TRACK
	GetState()->m_pVertexDeclaration = pVertexDeclaration;
	#endif

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexShader)(const C_VertexShader* pVertexShader)
{
	#if GPU_CHECK
	if(pVertexShader == GetState()->m_pVertexShader)
		GPU_RETURN_OK;
	#endif

	D3D_CALL(SetVertexShader(pVertexShader ? pVertexShader->m_pd3dVertexShader : NULL));
	
	#if GPU_TRACK
	GetState()->m_pVertexShader = pVertexShader;
	#endif

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexShaderConstantB)(const U32 startRegister, const BOOL* pData, const U32 registerCount)
{
	BIZ_ASSERT(startRegister < 128);
	BIZ_ASSERT(pData);
	BIZ_ASSERT(registerCount > 0);
	BIZ_ASSERT(startRegister+registerCount <= 128);

	D3D_CALL(SetVertexShaderConstantB(startRegister, pData, registerCount));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexShaderConstantI)(const U32 startRegister, const S32* pData, const U32 registerCount)
{
	BIZ_ASSERT(startRegister < 16);
	BIZ_ASSERT(pData);
	BIZ_ASSERT(registerCount > 0);
	BIZ_ASSERT(startRegister+registerCount <= 16);
	
	D3D_CALL(SetVertexShaderConstantI(startRegister, pData, registerCount));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexShaderConstantF)(const U32 startRegister, const F32* pData, const U32 registerCount)
{
	BIZ_ASSERT(startRegister < 256);
	BIZ_ASSERT(pData);
	BIZ_ASSERT(registerCount > 0);
	BIZ_ASSERT(startRegister+registerCount <= 256);

	D3D_CALL(SetVertexShaderConstantF(startRegister, pData, registerCount));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexShaderParamF)(const C_VertexShaderParam& shaderParam, const F32* pData)
{
	if(shaderParam.m_RegisterIndex == UINT_MAX || shaderParam.m_RegisterCount == 0)
		GPU_RETURN_FAIL;

	GPU_CALL(SetVertexShaderConstantF(shaderParam.m_RegisterIndex, pData, shaderParam.m_RegisterCount));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexTexture)(const U32 index, const C_Texture* pTexture)
{
	GPU_ASSERT(index < E_VertexSampler_Count);
	GPU_ASSERT_TEXTURE(pTexture);

	#if GPU_CHECK
	if(pTexture == GetState()->m_VertexTextureState[index].m_pTexture)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetTexture(D3DVERTEXTEXTURESAMPLER0+index, pTexture ? (GPUTexture*)pTexture->GetGPUTexture() : NULL));
	
	#if GPU_TRACK
	GetState()->m_VertexTextureState[index].m_pTexture = pTexture;
	#endif

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexTextureAddressU)(const U32 index, const E_TextureAddress addressU)
{
	GPU_ASSERT(index < E_VertexSampler_Count);
	GPU_ASSERT_TEXTUREADDRESS(addressU);

	#if GPU_CHECK
	if(addressU == GetState()->m_VertexSamplerState[index].m_TextureAddressU)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetSamplerState(D3DVERTEXTEXTURESAMPLER0+index, D3DSAMP_ADDRESSU, addressU));
	
	#if GPU_TRACK
	GetState()->m_VertexSamplerState[index].m_TextureAddressU = addressU;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexTextureAddressV)(const U32 index, const E_TextureAddress addressV)
{
	GPU_ASSERT(index < E_VertexSampler_Count);
	GPU_ASSERT_TEXTUREADDRESS(addressV);

	#if GPU_CHECK
	if(addressV == GetState()->m_VertexSamplerState[index].m_TextureAddressV)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetSamplerState(D3DVERTEXTEXTURESAMPLER0+index, D3DSAMP_ADDRESSV, addressV));
	
	#if GPU_TRACK
	GetState()->m_VertexSamplerState[index].m_TextureAddressV = addressV;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexTextureAddressW)(const U32 index, const E_TextureAddress addressW)
{
	GPU_ASSERT(index < E_VertexSampler_Count);
	GPU_ASSERT_TEXTUREADDRESS(addressW);

	#if GPU_CHECK
	if(addressW == GetState()->m_VertexSamplerState[index].m_TextureAddressW)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetSamplerState(D3DVERTEXTEXTURESAMPLER0+index, D3DSAMP_ADDRESSW, addressW));
	
	#if GPU_TRACK
	GetState()->m_VertexSamplerState[index].m_TextureAddressW = addressW;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexTextureFilterMag)(const U32 index, const E_TextureFilter filterMag)
{
	GPU_ASSERT(index < E_VertexSampler_Count);
	GPU_ASSERT_TEXTUREFILTER(filterMag);

	#if GPU_CHECK
	if(filterMag == GetState()->m_VertexSamplerState[index].m_TextureFilterMag)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetSamplerState(D3DVERTEXTEXTURESAMPLER0+index, D3DSAMP_MAGFILTER, filterMag));
	
	#if GPU_TRACK
	GetState()->m_VertexSamplerState[index].m_TextureFilterMag = filterMag;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexTextureFilterMin)(const U32 index, const E_TextureFilter filterMin)
{
	GPU_ASSERT(index < E_VertexSampler_Count);
	GPU_ASSERT_TEXTUREFILTER(filterMin);

	#if GPU_CHECK
	if(filterMin == GetState()->m_VertexSamplerState[index].m_TextureFilterMin)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetSamplerState(D3DVERTEXTEXTURESAMPLER0+index, D3DSAMP_MINFILTER, filterMin));
	
	#if GPU_TRACK
	GetState()->m_VertexSamplerState[index].m_TextureFilterMin = filterMin;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexTextureFilterMip)(const U32 index, const E_TextureFilter filterMip)
{
	GPU_ASSERT(index < E_VertexSampler_Count);
	GPU_ASSERT_TEXTUREFILTER(filterMip);

	#if GPU_CHECK
	if(filterMip == GetState()->m_VertexSamplerState[index].m_TextureFilterMip)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetSamplerState(D3DVERTEXTEXTURESAMPLER0+index, D3DSAMP_MIPFILTER, filterMip));
	
	#if GPU_TRACK
	GetState()->m_VertexSamplerState[index].m_TextureFilterMip = filterMip;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexTextureBorderColour)(const U32 index, const U32 borderColour)
{
	GPU_ASSERT(index < E_VertexSampler_Count);
	
	#if GPU_CHECK
	if(borderColour == GetState()->m_VertexSamplerState[index].m_TextureBorderColour)
		GPU_RETURN_OK;
	#endif
	
	D3D_CALL(SetSamplerState(D3DVERTEXTEXTURESAMPLER0+index, D3DSAMP_BORDERCOLOR, borderColour));
	
	#if GPU_TRACK
	GetState()->m_VertexSamplerState[index].m_TextureBorderColour = borderColour;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexTextureLodBias)(const U32 index, const S8 lodBias)
{
	GPU_ASSERT(index < E_VertexSampler_Count);
	
	#if GPU_CHECK
	if(lodBias == GetState()->m_VertexSamplerState[index].m_TextureLodBias)
		GPU_RETURN_OK;
	#endif
	
	const F32 fTextureLodBias = (((F32)(lodBias))/128.0f)*8.0f;
	GPU_ASSERT(fTextureLodBias >= -16.0f && fTextureLodBias <= 15.96875f);

	D3D_CALL(SetSamplerState(D3DVERTEXTEXTURESAMPLER0+index, D3DSAMP_MIPMAPLODBIAS, (DWORD&)fTextureLodBias));
	
	#if GPU_TRACK
	GetState()->m_VertexSamplerState[index].m_TextureLodBias = lodBias;
	#endif
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetViewport)(const C_Viewport& viewport)
{
	D3D_CALL(SetViewport(&viewport));
	
	GPU_RETURN_VOID;
}

#if DEVICE_D3D9_PC

GPU_DECL(GPU_VOID) GPU_FUNC(SetTextureGamma)(const U32 index, const U32 gamma)
{
	GPU_ASSERT(index < E_Sampler_Count);

	D3D_CALL(SetSamplerState(index, D3DSAMP_SRGBTEXTURE, gamma));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetVertexTextureGamma)(const U32 index, const U32 gamma)
{
	GPU_ASSERT(index < E_VertexSampler_Count);

	D3D_CALL(SetSamplerState(D3DVERTEXTEXTURESAMPLER0+index, D3DSAMP_SRGBTEXTURE, gamma));
	
	GPU_RETURN_VOID;
}

#endif

#if DEVICE_XB2

GPU_DECL(GPU_VOID) GPU_FUNC(SetAlphaToMaskEnable)(const E_Bool alphaToMaskEnable)
{
	GPU_ASSERT_BOOL(alphaToMaskEnable);

	D3D_CALL(SetRenderState(D3DRS_ALPHATOMASKENABLE, alphaToMaskEnable));

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetAlphaToMaskOffsets)(const U32 alphaToMaskOffsets)
{
	D3D_CALL(SetRenderState(D3DRS_ALPHATOMASKOFFSETS, alphaToMaskOffsets));

	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetPresentInterval)(const E_PresentInterval presentInterval)
{
	D3D_CALL(SetRenderState(D3DRS_PRESENTINTERVAL, presentInterval));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetStencilCcwRef)(const S32 stencilRef)
{
	D3D_CALL(SetRenderState(D3DRS_CCW_STENCILREF, stencilRef));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetStencilCcwMask)(const U32 stencilMask)
{
	D3D_CALL(SetRenderState(D3DRS_CCW_STENCILMASK, stencilMask));
	
	GPU_RETURN_VOID;
}

GPU_DECL(GPU_VOID) GPU_FUNC(SetStencilCcwWriteMask)(const U32 stencilWriteMask)
{
	D3D_CALL(SetRenderState(D3DRS_CCW_STENCILWRITEMASK, stencilWriteMask));
	
	GPU_RETURN_VOID;
}

#endif

//------------------------------------------------------------------------------------------------------------------------------------------------------

#undef GPU_DECL
#undef GPU_FUNC
