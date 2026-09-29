// [Blur1PCDX11] Native Display2 resource accessors and state metadata.
#ifndef DISPLAY2_D3D11_CONTEXT_INL
#define DISPLAY2_D3D11_CONTEXT_INL

#ifdef _MSC_VER
#pragma once
#endif

namespace Display2
{
	
INLINE D3DPOOL D3DPool(const U32 flags)
{
#if DEVICE_XB2
	return 0;
#else
	if(flags & E_Flags_Video)
		return D3DPOOL_DEFAULT;
	else
	if(flags & E_Flags_System)
		return D3DPOOL_SYSTEMMEM;
	else
		return D3DPOOL_MANAGED;
#endif
}

INLINE DWORD D3DTextureUsage(const U32 flags)
{
#if DEVICE_XB2
	return 0;
#else
	if(flags & E_Flags_Colour || flags & E_Flags_DepthStencil)
		return D3DUSAGE_RENDERTARGET;
	else
		return 0;
#endif
}

INLINE DWORD D3DUsage(const U32 flags)
{
#if DEVICE_XB2
	return 0;
#else
	if(flags & E_Flags_Dynamic)
		return D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
	else
		return D3DUSAGE_WRITEONLY;
#endif
}

INLINE U32 D3DPrimitiveCount(const U32 primitiveType, const U32 indexCount)
{
	switch(primitiveType)
	{
	case E_PrimitiveType_PointList:
		return indexCount;
	case E_PrimitiveType_LineList:
		return indexCount / 2;
	case E_PrimitiveType_LineStrip:
		return indexCount - 1;
	case E_PrimitiveType_TriangleList:
		return indexCount / 3;
	case E_PrimitiveType_TriangleStrip:
	case E_PrimitiveType_TriangleFan:
		return indexCount - 2;
	default:
		BIZ_ASSERT(0 && "Unsupported primitive type");
		return 0;
	}
}

// C_Viewport

INLINE U32 C_Viewport::GetX() const
{
	return TopLeftX;
}

INLINE U32 C_Viewport::GetY() const
{
	return TopLeftY;
}

INLINE U32 C_Viewport::GetWidth() const
{
	return Width;
}

INLINE U32 C_Viewport::GetHeight() const
{
	return Height;
}

INLINE F32 C_Viewport::GetMinZ() const
{
	return MinDepth;
}

INLINE F32 C_Viewport::GetMaxZ() const
{
	return MaxDepth;
}

INLINE void C_Viewport::SetX(const U32 x)
{
	TopLeftX = x;
}

INLINE void C_Viewport::SetY(const U32 y)
{
	TopLeftY = y;
}

INLINE void C_Viewport::SetWidth(const U32 width)
{
	Width = width;
}

INLINE void C_Viewport::SetHeight(const U32 height)
{
	Height = height;
}

INLINE void C_Viewport::SetMinZ(const F32 minZ)
{
	MinDepth = minZ;
}

INLINE void C_Viewport::SetMaxZ(const F32 maxZ)
{
	MaxDepth = maxZ;
}

// C_VertexElement

INLINE U16 C_VertexElement::GetStream() const
{
	return Stream;
}

INLINE U32 C_VertexElement::GetOffset() const
{
	return Offset;
}

INLINE U32 C_VertexElement::GetType() const
{
	return Type;
}

INLINE U8 C_VertexElement::GetUsage() const
{
	return Usage;
}

INLINE U8 C_VertexElement::GetUsageIndex() const
{
	return UsageIndex;
}

INLINE U32 C_VertexElement::GetSize() const
{
	switch(Type)
	{
	case E_VertexElementType_F32_1:		return sizeof(F32);
	case E_VertexElementType_F32_2:		return sizeof(F32) * 2;
	case E_VertexElementType_F32_3:		return sizeof(F32) * 3;
	case E_VertexElementType_F32_4:		return sizeof(F32) * 4;
	case E_VertexElementType_Colour:	return sizeof(U32);
	case E_VertexElementType_U8_4:		return sizeof(U8) * 4;
	case E_VertexElementType_U8_4N:		return sizeof(U8) * 4;
	case E_VertexElementType_S16_2:		return sizeof(S16) * 2;
	case E_VertexElementType_S16_4:		return sizeof(S16) * 4;
	case E_VertexElementType_F16_2:		return sizeof(U16) * 2;
	case E_VertexElementType_F16_4:		return sizeof(U16) * 4;
	case E_VertexElementType_S16_2N:	return sizeof(S16) * 2;
	case E_VertexElementType_S16_4N:	return sizeof(S16) * 4;
#if DEVICE_D3D9_XB2
	case E_VertexElementType_U16_2:		return sizeof(U16) * 2;
	case E_VertexElementType_U16_4:		return sizeof(U16) * 4;
	case E_VertexElementType_U10_3N:	return sizeof(U32);
	case E_VertexElementType_U10_4N:	return sizeof(U32);
	case E_VertexElementType_SHHD_3:	return sizeof(U32);
	case E_VertexElementType_SHHD_3N:	return sizeof(U32);
	case E_VertexElementType_U32_1:		return sizeof(U32);
#endif
	default:
		BIZ_ASSERT(0 && "Unsupported vertex element type");
		return 0;
	}
}

INLINE U32 C_VertexElement::GetCount() const
{
	switch(Type)
	{
	case E_VertexElementType_F32_1:		return 1;
	case E_VertexElementType_F32_2:		return 2;
	case E_VertexElementType_F32_3:		return 3;
	case E_VertexElementType_F32_4:		return 4;
	case E_VertexElementType_Colour:	return 1;
	case E_VertexElementType_U8_4:		return 4;
	case E_VertexElementType_U8_4N:		return 4;
	case E_VertexElementType_S16_2:		return 2;
	case E_VertexElementType_S16_4:		return 4;
	case E_VertexElementType_F16_2:		return 2;
	case E_VertexElementType_F16_4:		return 4;
	case E_VertexElementType_S16_2N:	return 2;
	case E_VertexElementType_S16_4N:	return 4;
#if DEVICE_D3D9_XB2
	case E_VertexElementType_U16_2:		return 2;
	case E_VertexElementType_U16_4:		return 4;
	case E_VertexElementType_U10_3N:	return 4;
	case E_VertexElementType_U10_4N:	return 4;
	case E_VertexElementType_SHHD_3:	return 3;
	case E_VertexElementType_SHHD_3N:	return 3;
	case E_VertexElementType_U32_1:		return 1;
#endif
	default:
		BIZ_ASSERT(0 && "Unsupported vertex element type");
		return 0;
	}
}

INLINE C_VertexElement& C_VertexElement::operator=(const C_VertexElement& other)
{
	Stream		= other.Stream;
	Offset		= other.Offset;
	Type		= other.Type;
	Method		= other.Method;
	Usage		= other.Usage;
	UsageIndex	= other.UsageIndex;

	return *this;
}

INLINE bool C_VertexElement::operator==(const C_VertexElement& other) const
{
	return
		Stream		== other.Stream		&&
		Offset		== other.Offset		&&
		Type		== other.Type		&&
		Method		== other.Method		&&
		Usage		== other.Usage		&&
		UsageIndex	== other.UsageIndex;

}
	
INLINE bool C_VertexElement::operator!=(const C_VertexElement& other) const
{
	return
		Stream		!= other.Stream		||
		Offset		!= other.Offset		||
		Type		!= other.Type		||
		Method		!= other.Method		||
		Usage		!= other.Usage		||
		UsageIndex	!= other.UsageIndex;
}

// C_VertexDeclaration

INLINE bool C_VertexDeclaration::IsCreated() const
{
	return m_ElementCount != 0;
}

INLINE GPUVertexDeclaration* C_VertexDeclaration::GetGPUVertexDeclaration()
{
	return m_pd3dVertexDeclaration;
}

INLINE const GPUVertexDeclaration* C_VertexDeclaration::GetGPUVertexDeclaration() const
{
	return m_pd3dVertexDeclaration;
}

// C_VertexBuffer

INLINE bool C_VertexBuffer::IsCreated() const
{
#if DEVICE_XB2
	return m_d3dVertexBuffer.Format.BaseAddress != 0 ? true : false;
#else
	return m_pd3dVertexBuffer ? true : false;
#endif
}

INLINE U32 C_VertexBuffer::GetSize() const
{
#if DEVICE_XB2
	return (m_d3dVertexBuffer.Format.Size << GPU_VERTEXBUFFER_SIZE_SHIFT);
#else
	if(m_pd3dVertexBuffer)
	{
		D3D11_BUFFER_DESC desc;
		m_pd3dVertexBuffer->GetDesc(&desc);
		return desc.ByteWidth;
	}
	else
	{
		return 0;
	}
#endif
}

INLINE U32 C_VertexBuffer::GetFlags() const
{
	return m_Flags;
}

INLINE void* C_VertexBuffer::GetAddress() const
{
#if DEVICE_XB2
	return (void*)(m_d3dVertexBuffer.Format.BaseAddress << GPU_VERTEXBUFFER_ADDRESS_SHIFT);
#else
	return NULL;
#endif
}

INLINE GPUVertexBuffer* C_VertexBuffer::GetGPUVertexBuffer()
{
#if DEVICE_XB2
	return IsCreated() ? &m_d3dVertexBuffer : NULL;
#else
	return m_pd3dVertexBuffer;
#endif
}

INLINE const GPUVertexBuffer* C_VertexBuffer::GetGPUVertexBuffer() const
{
#if DEVICE_XB2
	return IsCreated() ? &m_d3dVertexBuffer : NULL;
#else
	return m_pd3dVertexBuffer;
#endif
}

// C_IndexBuffer

INLINE bool C_IndexBuffer::IsCreated() const
{
#if DEVICE_XB2
	return m_d3dIndexBuffer.Address != 0 ? true : false;
#else
	return m_pd3dIndexBuffer ? true : false;
#endif
}

INLINE U32 C_IndexBuffer::GetSize() const
{
#if DEVICE_XB2
	return m_d3dIndexBuffer.Size;
#else
	if(m_pd3dIndexBuffer)
	{
		D3D11_BUFFER_DESC desc;
		m_pd3dIndexBuffer->GetDesc(&desc);
		return desc.ByteWidth;
	}
	else
	{
		return 0;
	}
#endif
}

INLINE U32 C_IndexBuffer::GetFlags() const
{
	return m_Flags;
}

INLINE void* C_IndexBuffer::GetAddress() const
{
#if DEVICE_XB2
	return (void*)m_d3dIndexBuffer.Address;
#else
	return NULL;
#endif
}

INLINE E_IndexFormat C_IndexBuffer::GetFormat() const
{
    return m_Format;
}

INLINE GPUIndexBuffer* C_IndexBuffer::GetGPUIndexBuffer()
{
#if DEVICE_XB2
	return IsCreated() ? &m_d3dIndexBuffer : NULL;
#else
	return m_pd3dIndexBuffer;
#endif
}

INLINE const GPUIndexBuffer* C_IndexBuffer::GetGPUIndexBuffer() const
{
#if DEVICE_XB2
	return IsCreated() ? &m_d3dIndexBuffer : NULL;
#else
	return m_pd3dIndexBuffer;
#endif
}

// C_VertexShader

INLINE bool C_VertexShader::IsCreated() const
{
	return m_pd3dVertexShader ? true : false;
}

INLINE U32 C_VertexShader::GetSize() const { return m_Size; }

INLINE U32 C_VertexShader::GetFlags() const
{
	return m_Flags;
}

INLINE void* C_VertexShader::GetAddress() const
{
#if DEVICE_XB2
	return ((void**)m_pd3dVertexShader)[8];   // Extract location of shader physical memory from D3D
#else
	return NULL;
#endif
}

INLINE U8 C_VertexShader::GetRegisterCount() const
{
	return 0;
}

INLINE void C_VertexShader::SetRegisterCount(const U8 registerCount)
{
}

INLINE GPUVertexShader* C_VertexShader::GetGPUVertexShader()
{
	return m_pd3dVertexShader;
}

INLINE const GPUVertexShader* C_VertexShader::GetGPUVertexShader() const
{
	return m_pd3dVertexShader;
}

// C_PixelShader

INLINE bool C_PixelShader::IsCreated() const
{
	return m_pd3dPixelShader ? true : false;
}

INLINE U32 C_PixelShader::GetSize() const { return m_Size; }

INLINE U32 C_PixelShader::GetFlags() const
{
	return m_Flags;
}

INLINE void* C_PixelShader::GetAddress() const
{
#if DEVICE_XB2
	return m_pAddress;
#else
	return NULL;
#endif
}

INLINE U8 C_PixelShader::GetRegisterCount() const
{
	return 0;
}

INLINE void C_PixelShader::SetRegisterCount(const U8 registerCount)
{
}

INLINE GPUPixelShader* C_PixelShader::GetGPUPixelShader()
{
	return m_pd3dPixelShader;
}

INLINE const GPUPixelShader* C_PixelShader::GetGPUPixelShader() const
{
	return m_pd3dPixelShader;
}

// C_LockedRect

INLINE S32 C_LockedRect::GetPitch() const
{
	return pitch;
}

INLINE U8* C_LockedRect::GetBits() const
{
	return pBits;
}

// C_LockedRect

INLINE S32 C_ConstLockedRect::GetPitch() const
{
	return pitch;
}

INLINE const U8* C_ConstLockedRect::GetBits() const
{
	return pBits;
}

// C_LockedBox

INLINE S32 C_LockedBox::GetRowPitch() const
{
	return rowPitch;
}

INLINE S32 C_LockedBox::GetSlicePitch() const
{
	return slicePitch;
}

INLINE U8* C_LockedBox::GetBits() const
{
	return pBits;
}

// C_ConstLockedBox

INLINE S32 C_ConstLockedBox::GetRowPitch() const
{
	return rowPitch;
}

INLINE S32 C_ConstLockedBox::GetSlicePitch() const
{
	return slicePitch;
}

INLINE const U8* C_ConstLockedBox::GetBits() const
{
	return pBits;
}

inline E_TextureFormat D3D11PCFormat(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_B8G8R8A8_UNORM: case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return E_TextureFormat_A8R8G8B8;
    case DXGI_FORMAT_B8G8R8X8_UNORM: case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: return E_TextureFormat_X8R8G8B8;
    case DXGI_FORMAT_R8G8B8A8_UNORM: case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return E_TextureFormat_A8B8G8R8;
    case DXGI_FORMAT_BC1_UNORM: case DXGI_FORMAT_BC1_UNORM_SRGB: return E_TextureFormat_DXT1;
    case DXGI_FORMAT_BC2_UNORM: case DXGI_FORMAT_BC2_UNORM_SRGB: return E_TextureFormat_DXT3;
    case DXGI_FORMAT_BC3_UNORM: case DXGI_FORMAT_BC3_UNORM_SRGB: return E_TextureFormat_DXT5;
    case DXGI_FORMAT_R8_UNORM: return E_TextureFormat_L8;
    case DXGI_FORMAT_A8_UNORM: return E_TextureFormat_A8;
    case DXGI_FORMAT_R8G8_UNORM: return E_TextureFormat_A8L8;
    case DXGI_FORMAT_R16_FLOAT: return E_TextureFormat_R16F;
    case DXGI_FORMAT_R32_FLOAT: return E_TextureFormat_R32F;
    case DXGI_FORMAT_R16G16B16A16_FLOAT: return E_TextureFormat_A16B16G16R16F;
    case DXGI_FORMAT_R32G32B32A32_FLOAT: return E_TextureFormat_A32B32G32R32F;
    case DXGI_FORMAT_D16_UNORM: return E_TextureFormat_D16;
    case DXGI_FORMAT_D24_UNORM_S8_UINT: return E_TextureFormat_D24S8;
    default: return E_TextureFormat_Unknown;
    }
}

// C_Surface

INLINE bool C_Surface::IsCreated() const
{
#if DEVICE_XB2
	return m_d3dSurface.Size != 0 ? true : false;
#else
	return m_pd3dSurface != NULL ? true : false;
#endif
}

INLINE U32 C_Surface::GetSize() const
{
	if(!IsCreated())
		return 0;

#if DEVICE_XB2
	return m_d3dSurface.Size;
#else
	return 0;
#endif
}

INLINE U32 C_Surface::GetFlags() const
{
	return m_Flags;
}

INLINE U8* C_Surface::GetAddress() const
{
	return NULL;
}

INLINE U32 C_Surface::GetWidth() const
{
	if(!IsCreated())
		return 0;

	D3D11_TEXTURE2D_DESC desc;
	((GPUSurface*)GetGPUSurface())->GetDesc(&desc);
	const U32 mipWidth = desc.Width >> (m_Subresource % desc.MipLevels);
	return mipWidth ? mipWidth : 1U;
}
	
INLINE U32 C_Surface::GetHeight() const
{
	if(!IsCreated())
		return 0;

	D3D11_TEXTURE2D_DESC desc;
	((GPUSurface*)GetGPUSurface())->GetDesc(&desc);
	const U32 mipHeight = desc.Height >> (m_Subresource % desc.MipLevels);
	return mipHeight ? mipHeight : 1U;
}

INLINE E_SurfaceFormat C_Surface::GetFormat() const
{
	if(!IsCreated())
		return E_SurfaceFormat_Unknown;

	D3D11_TEXTURE2D_DESC desc;
	((GPUSurface*)GetGPUSurface())->GetDesc(&desc);
	return (E_SurfaceFormat)D3D11PCFormat(desc.Format);
}

INLINE E_Multisample C_Surface::GetMultisample() const
{
	if(!IsCreated())
		return E_Multisample_Unknown;

	D3D11_TEXTURE2D_DESC desc;
	((GPUSurface*)GetGPUSurface())->GetDesc(&desc);
	return desc.SampleDesc.Count <= 1 ? E_Multisample_None : (E_Multisample)desc.SampleDesc.Count;
}

INLINE GPUSurface* C_Surface::GetGPUSurface()
{
#if DEVICE_XB2
	return IsCreated() ? &m_d3dSurface : NULL;
#else
	return m_pd3dSurface;
#endif
}

INLINE const GPUSurface* C_Surface::GetGPUSurface() const
{
#if DEVICE_XB2
	return IsCreated() ? &m_d3dSurface : NULL;
#else
	return m_pd3dSurface;
#endif
}

// C_Texture

INLINE bool C_Texture::IsCreated() const
{
#if DEVICE_XB2
	return GetAddress() ? true : false;
#else
	return m_pd3dTexture != NULL ? true : false;
#endif
}

INLINE E_TextureType C_Texture::GetType() const
{
    if (!IsCreated()) return E_TextureType_Unknown;
    D3D11_RESOURCE_DIMENSION dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    ((GPUTexture*)GetGPUTexture())->GetType(&dimension);
    if (dimension == D3D11_RESOURCE_DIMENSION_TEXTURE3D) return E_TextureType_3D;
    if (dimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
        D3D11_TEXTURE2D_DESC desc = {};
        ((GPUTexture2D*)GetGPUTexture2D())->GetDesc(&desc);
        return (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) ? E_TextureType_Cube : E_TextureType_2D;
    }
    return E_TextureType_Unknown;
}

INLINE U32 C_Texture::GetSize() const
{
	return m_Size;
}

INLINE U8* C_Texture::GetAddress() const
{
#if DEVICE_XB2
	return (U8*)(m_d3dTexture.Format.BaseAddress << GPU_TEXTURE_ADDRESS_SHIFT);
#else
	return NULL;
#endif
}

INLINE U32 C_Texture::GetFlags() const
{
	return m_Flags;
}

#if DEVICE_XB2

inline U32 GetWidth2D(const C_Texture& texture)
{
	return texture.m_d3dTexture.Format.Size.TwoD.Width + 1;
}

inline U32 GetHeight2D(const C_Texture& texture)
{
	return texture.m_d3dTexture.Format.Size.TwoD.Height + 1;
}

inline U32 GetLevels(const C_Texture& texture)
{
	return texture.m_d3dTexture.Format.MaxMipLevel - texture.m_d3dTexture.Format.MinMipLevel + 1;
}

#else

inline U32 GetWidth2D(const C_Texture& texture)
{
	return texture.GetWidth();
}

inline U32 GetHeight2D(const C_Texture& texture)
{
	return texture.GetHeight();
}

inline U32 GetLevels(const C_Texture& texture)
{
	return texture.GetLevels();
}

#endif

INLINE U32 C_Texture::GetWidth(const U32 level) const
{
    if (!IsCreated()) return 0;
    if (GetType() == E_TextureType_3D)
    {
        D3D11_TEXTURE3D_DESC desc = {};
        ((GPUTexture3D*)GetGPUTexture3D())->GetDesc(&desc);
        return desc.Width >> level ? desc.Width >> level : 1;
    }
    D3D11_TEXTURE2D_DESC desc = {};
    ((GPUTexture2D*)GetGPUTexture2D())->GetDesc(&desc);
    return desc.Width >> level ? desc.Width >> level : 1;
}

INLINE U32 C_Texture::GetHeight(const U32 level) const
{
    if (!IsCreated()) return 0;
    if (GetType() == E_TextureType_3D)
    {
        D3D11_TEXTURE3D_DESC desc = {};
        ((GPUTexture3D*)GetGPUTexture3D())->GetDesc(&desc);
        return desc.Height >> level ? desc.Height >> level : 1;
    }
    D3D11_TEXTURE2D_DESC desc = {};
    ((GPUTexture2D*)GetGPUTexture2D())->GetDesc(&desc);
    return desc.Height >> level ? desc.Height >> level : 1;
}

INLINE U32 C_Texture::GetDepth(const U32 level) const
{
    if (!IsCreated() || GetType() != E_TextureType_3D) return 1;
    D3D11_TEXTURE3D_DESC desc = {};
    ((GPUTexture3D*)GetGPUTexture3D())->GetDesc(&desc);
    return desc.Depth >> level ? desc.Depth >> level : 1;
}

INLINE E_TextureFormat C_Texture::GetFormat() const
{
    if (!IsCreated()) return E_TextureFormat_Unknown;
    if (GetType() == E_TextureType_3D)
    {
        D3D11_TEXTURE3D_DESC desc = {};
        ((GPUTexture3D*)GetGPUTexture3D())->GetDesc(&desc);
        return D3D11PCFormat(desc.Format);
    }
    D3D11_TEXTURE2D_DESC desc = {};
    ((GPUTexture2D*)GetGPUTexture2D())->GetDesc(&desc);
    return D3D11PCFormat(desc.Format);
}

INLINE U32 C_Texture::GetLevels() const
{
    if (!IsCreated()) return 0;
    if (GetType() == E_TextureType_3D)
    {
        D3D11_TEXTURE3D_DESC desc = {};
        ((GPUTexture3D*)GetGPUTexture3D())->GetDesc(&desc);
        return desc.MipLevels;
    }
    D3D11_TEXTURE2D_DESC desc = {};
    ((GPUTexture2D*)GetGPUTexture2D())->GetDesc(&desc);
    return desc.MipLevels;
}

INLINE GPUTexture* C_Texture::GetGPUTexture()
{
#if DEVICE_XB2
	return IsCreated() ? &m_d3dTexture : NULL;
#else
	return m_pd3dTexture;
#endif
}

INLINE const GPUTexture* C_Texture::GetGPUTexture() const
{
#if DEVICE_XB2
	return IsCreated() ? &m_d3dTexture : NULL;
#else
	return m_pd3dTexture;
#endif
}

INLINE GPUTexture2D* C_Texture::GetGPUTexture2D()
{
#if DEVICE_XB2
	return IsCreated() ? &m_d3dTexture2D : NULL;
#else
	return (GPUTexture2D*)m_pd3dTexture;
#endif
}

INLINE const GPUTexture2D* C_Texture::GetGPUTexture2D() const
{
#if DEVICE_XB2
	return IsCreated() ? &m_d3dTexture2D : NULL;
#else
	return (GPUTexture2D*)m_pd3dTexture;
#endif
}

INLINE GPUTexture3D* C_Texture::GetGPUTexture3D()
{
#if DEVICE_XB2
	return IsCreated() ? &m_d3dTexture3D : NULL;
#else
	return (GPUTexture3D*)m_pd3dTexture;
#endif
}

INLINE const GPUTexture3D* C_Texture::GetGPUTexture3D() const
{
#if DEVICE_XB2
	return IsCreated() ? &m_d3dTexture3D : NULL;
#else
	return (GPUTexture3D*)m_pd3dTexture;
#endif
}

INLINE GPUTextureCube* C_Texture::GetGPUTextureCube()
{
#if DEVICE_XB2
	return IsCreated() ? &m_d3dTextureCube : NULL;
#else
	return (GPUTextureCube*)m_pd3dTexture;
#endif
}

INLINE const GPUTextureCube* C_Texture::GetGPUTextureCube() const
{
#if DEVICE_XB2
	return IsCreated() ? &m_d3dTextureCube : NULL;
#else
	return (GPUTextureCube*)m_pd3dTexture;
#endif
}

// C_RenderTexture2D

INLINE bool C_RenderTexture2D::IsCreated() const
{
#if DEVICE_D3D9_PC || DEVICE_D3D11_PC
	return m_Texture[0].IsCreated() && m_Surfaces[0].Size() > 0 && m_Surfaces[0][0].IsCreated();
#else
	return m_Texture.IsCreated() && m_Surfaces.Size() > 0 && m_Surfaces[0].IsCreated();
#endif
}

INLINE U32 C_RenderTexture2D::GetFlags() const
{
	return m_Flags;
}

INLINE C_Texture& C_RenderTexture2D::GetTexture()
{
#if DEVICE_D3D9_PC || DEVICE_D3D11_PC
	BIZ_ASSERT(m_TextureIndex < 2);
	return m_Texture[m_TextureIndex];
#else
	return m_Texture;
#endif
}

INLINE const C_Texture& C_RenderTexture2D::GetTexture() const
{
#if DEVICE_D3D9_PC || DEVICE_D3D11_PC
	BIZ_ASSERT(m_TextureIndex < 2);
	return m_Texture[m_TextureIndex];
#else
	return m_Texture;
#endif
}

INLINE C_Surface& C_RenderTexture2D::GetSurface(const U32 level)
{
#if DEVICE_D3D9_PC || DEVICE_D3D11_PC
	BIZ_ASSERT(m_SurfaceIndex < 2);
	BIZ_ASSERT(level < (U32)m_Surfaces[m_SurfaceIndex].Size());
	return m_Surfaces[m_SurfaceIndex][level];
#else
	BIZ_ASSERT(level < (U32)m_Surfaces.Size());
	return m_Surfaces[level];
#endif
}

INLINE const C_Surface& C_RenderTexture2D::GetSurface(const U32 level) const
{
#if DEVICE_D3D9_PC || DEVICE_D3D11_PC
	BIZ_ASSERT(m_SurfaceIndex < 2);
	BIZ_ASSERT(level < (U32)m_Surfaces[m_SurfaceIndex].Size());
	return m_Surfaces[m_SurfaceIndex][level];
#else
	BIZ_ASSERT(level < (U32)m_Surfaces.Size());
	return m_Surfaces[level];
#endif
}

// C_RenderTextureCube

INLINE bool C_RenderTextureCube::IsCreated() const
{
	return m_Texture.IsCreated();
}

INLINE U32 C_RenderTextureCube::GetFlags() const
{
	return m_Flags;
}

INLINE C_Texture& C_RenderTextureCube::GetTexture()
{
	return m_Texture;
}

INLINE const C_Texture& C_RenderTextureCube::GetTexture() const
{
	return m_Texture;
}

INLINE C_Surface& C_RenderTextureCube::GetSurface(const U32 face, const U32 level)
{
	BIZ_ASSERT(face < 6);
	BIZ_ASSERT(level < (U32)m_Surfaces[face].Size());
	return m_Surfaces[face][level];
}

INLINE const C_Surface& C_RenderTextureCube::GetSurface(const U32 face, const U32 level) const
{
	BIZ_ASSERT(face < 6);
	BIZ_ASSERT(level < (U32)m_Surfaces[face].Size());
	return m_Surfaces[face][level];
}

// C_AlphaTestState

INLINE void C_AlphaTestState::SetDefaultStates()
{
	m_AlphaTestCmpFunc	= E_CmpFunc_Always;
	m_AlphaTestRef		= 0;
}

INLINE void C_AlphaTestState::SetAlphaTestCmpFunc(const E_CmpFunc alphaTestCmpFunc)
{
	GPU_ASSERT_BITCOUNT(alphaTestCmpFunc, GPU_ALPHATEST_CMPFUNC_BITCOUNT);
	GPU_ASSERT_CMPFUNC(alphaTestCmpFunc);
	m_AlphaTestCmpFunc = alphaTestCmpFunc;
}

INLINE void C_AlphaTestState::SetAlphaTestRef(const U8 alphaTestRef)
{
	GPU_ASSERT_BITCOUNT(alphaTestRef, GPU_ALPHATEST_REF_BITCOUNT);
	m_AlphaTestRef = alphaTestRef;
}

// C_BlendState

INLINE void C_BlendState::SetDefaultStates()
{
	m_BlendColorSrc	= E_Blend_One;
	m_BlendColorDst	= E_Blend_Zero;
	m_BlendColorOp	= E_BlendOp_Add;
	m_BlendAlphaSrc	= E_Blend_One;
	m_BlendAlphaDst	= E_Blend_One;
	m_BlendAlphaOp	= E_BlendOp_Add;
}

INLINE void C_BlendState::SetBlendColourSrc(const E_Blend blendColorSrc)
{
	GPU_ASSERT_BITCOUNT(blendColorSrc, GPU_BLEND_COLORSRC_BITCOUNT);
	GPU_ASSERT_BLEND(blendColorSrc);
	m_BlendColorSrc = blendColorSrc;
}

INLINE void C_BlendState::SetBlendColourDst(const E_Blend blendColorDst)
{
	GPU_ASSERT_BITCOUNT(blendColorDst, GPU_BLEND_COLORDST_BITCOUNT);
	GPU_ASSERT_BLEND(blendColorDst);
	m_BlendColorDst = blendColorDst;
}

INLINE void C_BlendState::SetBlendColourOp(const E_BlendOp blendColorOp)
{
	GPU_ASSERT_BITCOUNT(blendColorOp, GPU_BLEND_COLOROP_BITCOUNT);
	GPU_ASSERT_BLENDOP(blendColorOp);
	m_BlendColorOp = blendColorOp;
}

INLINE void C_BlendState::SetBlendAlphaSrc(const E_Blend blendAlphaSrc)
{
	GPU_ASSERT_BITCOUNT(blendAlphaSrc, GPU_BLEND_ALPHASRC_BITCOUNT);
	GPU_ASSERT_BLEND(blendAlphaSrc);
	m_BlendAlphaSrc = blendAlphaSrc;
}

INLINE void C_BlendState::SetBlendAlphaDst(const E_Blend blendAlphaDst)
{
	GPU_ASSERT_BITCOUNT(blendAlphaDst, GPU_BLEND_ALPHADST_BITCOUNT);
	GPU_ASSERT_BLEND(blendAlphaDst);
	m_BlendAlphaDst = blendAlphaDst;
}

INLINE void C_BlendState::SetBlendAlphaOp(const E_BlendOp blendAlphaOp)
{
	GPU_ASSERT_BITCOUNT(blendAlphaOp, GPU_BLEND_ALPHA_OPBITCOUNT);
	GPU_ASSERT_BLENDOP(blendAlphaOp);
	m_BlendAlphaOp = blendAlphaOp;
}

// C_ColourState

INLINE void C_ColourState::SetDefaultStates()
{
	m_ColourWriteEnable = E_ColourWriteEnable_All;
}
	
INLINE void C_ColourState::SetColourWriteEnable(const U32 colourWriteEnable)
{
	m_ColourWriteEnable = colourWriteEnable;
}

// C_CullState

INLINE void C_CullState::SetDefaultStates()
{
	m_CullMode = E_CullMode_Ccw;
}

INLINE void C_CullState::SetCullMode(const E_CullMode cullMode)
{
	GPU_ASSERT_CULLMODE(cullMode);
	m_CullMode = cullMode;
}

// C_DepthState

INLINE void C_DepthState::SetDefaultStates()
{
	m_DepthFunc			= E_CmpFunc_LessEqual;
	m_DepthEnable		= E_Bool_True;
	m_DepthWriteEnable	= E_Bool_True;
	m_DepthBias			= 0.0f;
	m_SlopeScaledBias	= 0.0f;
}

INLINE void C_DepthState::SetDepthFunc(const E_CmpFunc depthFunc)
{
	GPU_ASSERT_BITCOUNT(depthFunc, GPU_DEPTH_FUNC_BITCOUNT);
	GPU_ASSERT_CMPFUNC(depthFunc);
	m_DepthFunc = depthFunc;
}

INLINE void C_DepthState::SetDepthEnable(const E_Bool depthEnable)
{
	GPU_ASSERT_BITCOUNT(depthEnable, GPU_DEPTH_ENABLE_BITCOUNT);
	GPU_ASSERT_BOOL(depthEnable);
	m_DepthEnable = depthEnable;
}

INLINE void C_DepthState::SetDepthWriteEnable(const E_Bool depthWriteEnable)
{
	GPU_ASSERT_BITCOUNT(depthWriteEnable, GPU_DEPTH_WRITEENABLE_BITCOUNT);
	GPU_ASSERT_BOOL(depthWriteEnable);
	m_DepthWriteEnable = depthWriteEnable;
}

INLINE void C_DepthState::SetDepthBias(const F32 depthBias)
{
	m_DepthBias = depthBias;
}

INLINE void C_DepthState::SetSlopeScaledBias(const F32 slopeScaledBias) 
{
	m_SlopeScaledBias = slopeScaledBias;
}

// C_FillState

INLINE void C_FillState::SetDefaultStates()
{
	m_FillMode = E_FillMode_Solid;
}

INLINE void C_FillState::SetFillMode(const U32 fillMode)
{
	GPU_ASSERT_FILLMODE(fillMode);
	m_FillMode = fillMode;
}

// C_TextureState

INLINE void C_TextureState::SetDefaultStates()
{
	m_pTexture = NULL;
}

INLINE void C_TextureState::SetTexture(const C_Texture* pTexture)
{
	GPU_ASSERT_TEXTURE(pTexture);
	m_pTexture = pTexture;
}

// C_SamplerState

INLINE void C_SamplerState::SetDefaultStates()
{
	m_TextureBits			= 0;
	m_TextureAddressU		= E_TextureAddress_Wrap;
	m_TextureAddressV		= E_TextureAddress_Wrap;
	m_TextureAddressW		= E_TextureAddress_Wrap;
	m_TextureFilterMag		= E_TextureFilter_Point;
	m_TextureFilterMin		= E_TextureFilter_Point;
	m_TextureFilterMip		= E_TextureFilter_None;
	#if DEVICE_D3D9_PC || DEVICE_D3D11_PC
	m_TextureGamma			= E_Bool_False;
	#endif
	m_TextureBorderColour	= 0x00000000;
	m_TextureLodBias		= 0;
}

INLINE bool C_SamplerState::IsSame(const C_SamplerState &other)
{
	return	m_TextureBits == other.m_TextureBits &&
			m_TextureBorderColour == other.m_TextureBorderColour &&
			m_TextureLodBias == other.m_TextureLodBias;
}

INLINE void C_SamplerState::SetTextureAddressU(const E_TextureAddress textureAddressU)
{
	GPU_ASSERT_BITCOUNT(textureAddressU, GPU_TEXTURE_ADDRESSU_BITCOUNT);
	GPU_ASSERT_TEXTUREADDRESS(textureAddressU);
	m_TextureAddressU = textureAddressU;
}

INLINE void C_SamplerState::SetTextureAddressV(const E_TextureAddress textureAddressV)
{
	GPU_ASSERT_BITCOUNT(textureAddressV, GPU_TEXTURE_ADDRESSV_BITCOUNT);
	GPU_ASSERT_TEXTUREADDRESS(textureAddressV);
	m_TextureAddressV = textureAddressV;
}

INLINE void C_SamplerState::SetTextureAddressW(const E_TextureAddress textureAddressW)
{
	GPU_ASSERT_BITCOUNT(textureAddressW, GPU_TEXTURE_ADDRESSW_BITCOUNT);
	GPU_ASSERT_TEXTUREADDRESS(textureAddressW);
	m_TextureAddressW = textureAddressW;
}

INLINE void C_SamplerState::SetTextureAddress(const E_TextureAddress textureAddressU, const E_TextureAddress textureAddressV)
{
	SetTextureAddressU(textureAddressU);
	SetTextureAddressV(textureAddressV);
}

INLINE void C_SamplerState::SetTextureAddress(const E_TextureAddress textureAddressU, const E_TextureAddress textureAddressV, const E_TextureAddress textureAddressW)
{
	SetTextureAddressU(textureAddressU);
	SetTextureAddressV(textureAddressV);
	SetTextureAddressW(textureAddressW);
}

INLINE void C_SamplerState::SetTextureAddress(const E_TextureAddress textureAddress)
{
	SetTextureAddressU(textureAddress);
	SetTextureAddressV(textureAddress);
	SetTextureAddressW(textureAddress);
}

INLINE void C_SamplerState::SetTextureFilter(const E_TextureFilter textureFilterMag, const E_TextureFilter textureFilterMin, const E_TextureFilter textureFilterMip)
{
	GPU_ASSERT_BITCOUNT(textureFilterMag, GPU_TEXTURE_FILTERMAG_BITCOUNT);
	GPU_ASSERT_TEXTUREFILTER(textureFilterMag);
	m_TextureFilterMag = textureFilterMag;

	GPU_ASSERT_BITCOUNT(textureFilterMin, GPU_TEXTURE_FILTERMIN_BITCOUNT);
	GPU_ASSERT_TEXTUREFILTER(textureFilterMin);
	m_TextureFilterMin = textureFilterMin;

	GPU_ASSERT_BITCOUNT(textureFilterMip, GPU_TEXTURE_FILTERMIP_BITCOUNT);
	GPU_ASSERT_TEXTUREFILTER(textureFilterMip);
	m_TextureFilterMip = textureFilterMip;
}

INLINE void C_SamplerState::SetTextureFilter(const E_TextureFilter textureFilter)
{
	SetTextureFilter(textureFilter, textureFilter, textureFilter);
}

INLINE void C_SamplerState::SetTextureBorderColour(const U32 textureBorderColour)
{
	GPU_ASSERT_TEXTUREBORDERCOLOUR(textureBorderColour);
	m_TextureBorderColour = textureBorderColour;
}

INLINE void C_SamplerState::SetTextureLodBias(const S8 textureLodBias)
{
	m_TextureLodBias = textureLodBias;
}

#if DEVICE_D3D9_PC || DEVICE_D3D11_PC
INLINE void C_SamplerState::SetTextureGamma(const E_Bool textureGamma)
{
	GPU_ASSERT_BOOL(textureGamma);
	m_TextureGamma = textureGamma;
}
#endif

// C_ScissorState

INLINE void C_ScissorState::SetDefaultStates()
{
	m_ScissorEnable = E_Bool_False;
}

INLINE void C_ScissorState::SetScissorEnable(const E_Bool scissorEnable)
{
	GPU_ASSERT_BOOL(scissorEnable);
	m_ScissorEnable = scissorEnable;
}

INLINE void C_ScissorState::SetScissorRect(const RECT& scissorRect)
{
	m_ScissorRect = scissorRect;
}

// C_StencilState

INLINE void C_StencilState::SetDefaultStates()
{
	m_StencilFunc		= E_CmpFunc_Always;
	m_StencilFail		= E_StencilOp_Keep;
	m_StencilDepthFail	= E_StencilOp_Keep;
	m_StencilDepthPass	= E_StencilOp_Keep;
	m_StencilRef		= 0;
	m_StencilMask		= 0xFFFFFFFF;
	m_StencilWriteMask	= 0xFFFFFFFF;
}

INLINE void C_StencilState::SetStencilFunc(const E_CmpFunc stencilFunc)
{
	GPU_ASSERT_BITCOUNT(stencilFunc, GPU_STENCIL_FUNC_BITCOUNT);
	GPU_ASSERT_CMPFUNC(stencilFunc);
	m_StencilFunc = stencilFunc;
}

INLINE void C_StencilState::SetStencilFail(const E_StencilOp stencilFail)
{
	GPU_ASSERT_BITCOUNT(stencilFail, GPU_STENCIL_FAIL_BITCOUNT);
	GPU_ASSERT_STENCILOP(stencilFail);
	m_StencilFail = stencilFail;
}

INLINE void C_StencilState::SetStencilDepthFail(const E_StencilOp stencilDepthFail)
{
	GPU_ASSERT_BITCOUNT(stencilDepthFail, GPU_STENCIL_DEPTHFAIL_BITCOUNT);
	GPU_ASSERT_STENCILOP(stencilDepthFail);
	m_StencilDepthFail = stencilDepthFail;
}

INLINE void C_StencilState::SetStencilDepthPass(const E_StencilOp stencilDepthPass)
{
	GPU_ASSERT_BITCOUNT(stencilDepthPass, GPU_STENCIL_DEPTHPASS_BITCOUNT);
	GPU_ASSERT_STENCILOP(stencilDepthPass);
	m_StencilDepthPass = stencilDepthPass;
}

INLINE void C_StencilState::SetStencilRef(const U32 stencilRef)
{
	m_StencilRef = stencilRef;
}

INLINE void C_StencilState::SetStencilMask(const U32 stencilMask)
{
	m_StencilMask = stencilMask;
}

INLINE void C_StencilState::SetStencilWriteMask(const U32 stencilWriteMask)
{
	m_StencilWriteMask = stencilWriteMask;
}

// C_ContextState

INLINE void C_ContextState::SetDefaultStates()
{
	m_AlphaTestEnable						= E_Bool_False;
	m_BlendEnable							= E_Bool_False;
	m_StencilEnable							= E_Bool_False;
	m_PointSpriteEnable						= E_Bool_False;
	
	m_AlphaTestState.SetDefaultStates();
	m_BlendState.SetDefaultStates();
	m_ColourState.SetDefaultStates();
	m_CullState.SetDefaultStates();
	m_DepthState.SetDefaultStates();
	m_FillState.SetDefaultStates();
	m_ScissorState.SetDefaultStates();
	m_StencilState.SetDefaultStates();

	m_pVertexDeclaration					= NULL;
	m_pIndexBuffer							= NULL;

	for(U32 i=0; i<E_Stream_Count; i++)
	{
		m_pVertexBuffer[i]					= NULL;
		m_VertexOffset[i]					= 0;
		m_VertexStride[i]					= 0;
	}

	m_pVertexShader							= NULL;
	m_pPixelShader							= NULL;

	for(U32 i=0; i<E_Sampler_Count; i++)
		m_TextureState[i].SetDefaultStates();
	for(U32 i=0; i<E_Sampler_Count; i++)
		m_SamplerState[i].SetDefaultStates();
	
	for(U32 i=0; i<E_VertexSampler_Count; i++)
		m_VertexTextureState[i].SetDefaultStates();
	for(U32 i=0; i<E_VertexSampler_Count; i++)
		m_VertexSamplerState[i].SetDefaultStates();
}

// C_BlendMode

INLINE void C_BlendMode::SetDefaultStates()
{
	m_AlphaTestState.SetDefaultStates();
	m_BlendState.SetDefaultStates();
}

// C_ContextBase

INLINE GPUDevice* C_ContextBase::GetD3DDevice()
{
	BIZ_ASSERT(m_pd3dDevice);
	return m_pd3dDevice;
}

INLINE const GPUDevice* C_ContextBase::GetD3DDevice() const
{
	BIZ_ASSERT(m_pd3dDevice);
	return m_pd3dDevice;
}

INLINE void C_ContextBase::SetD3DDevice(GPUDevice* pd3dDevice)
{
	m_pd3dDevice = pd3dDevice;
}

// C_Context

INLINE C_ContextState* C_Context::GetState()
{
	BIZ_ASSERT(m_pState);
	return m_pState;
}

INLINE const C_ContextState* C_Context::GetState() const
{
	BIZ_ASSERT(m_pState);
	return m_pState;
}

INLINE void C_Context::SetState(C_ContextState* pState)
{
	m_pState = pState;
}

// C_ContextNoCheck

INLINE C_ContextState* C_ContextNoCheck::GetState()
{
	BIZ_ASSERT(m_pState);
	return m_pState;
}

INLINE const C_ContextState* C_ContextNoCheck::GetState() const
{
	return m_pState;
}

INLINE void C_ContextNoCheck::SetState(C_ContextState* pState)
{
	BIZ_ASSERT(pState);
	m_pState = pState;
}

} // namespace Display2

#endif // DISPLAY2_D3D11_CONTEXT_INL
