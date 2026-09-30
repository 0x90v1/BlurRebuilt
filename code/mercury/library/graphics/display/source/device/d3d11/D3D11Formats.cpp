#include "Display/Device/D3D11/D3D11Formats.h"
#include "Display2/Device/D3D11TextureFormats.h"

namespace Display { namespace D3D11 {

bool GetFormatLayout(D3DFORMAT source, C_FormatLayout* layout)
{
    if (!layout) return false;
    C_FormatLayout result = { DXGI_FORMAT_UNKNOWN, 1, 1, 0 };
    switch (source)
    {
    case D3DFMT_A8R8G8B8: result.m_Format = DXGI_FORMAT_B8G8R8A8_UNORM; result.m_BlockBytes = 4; break;
    case D3DFMT_X8R8G8B8: result.m_Format = DXGI_FORMAT_B8G8R8X8_UNORM; result.m_BlockBytes = 4; break;
    case D3DFMT_A8B8G8R8: result.m_Format = DXGI_FORMAT_R8G8B8A8_UNORM; result.m_BlockBytes = 4; break;
    case D3DFMT_DXT1: result = { DXGI_FORMAT_BC1_UNORM, 4, 4, 8 }; break;
    case D3DFMT_DXT3: result = { DXGI_FORMAT_BC2_UNORM, 4, 4, 16 }; break;
    case D3DFMT_DXT5: result = { DXGI_FORMAT_BC3_UNORM, 4, 4, 16 }; break;
    case D3DFMT_A8: result.m_Format = DXGI_FORMAT_A8_UNORM; result.m_BlockBytes = 1; break;
    case D3DFMT_L8: result.m_Format = DXGI_FORMAT_R8_UNORM; result.m_BlockBytes = 1; break;
    case D3DFMT_A8L8: result.m_Format = DXGI_FORMAT_R8G8_UNORM; result.m_BlockBytes = 2; break;
    case D3DFMT_V8U8: result.m_Format = DXGI_FORMAT_R8G8_SNORM; result.m_BlockBytes = 2; break;
    case D3DFMT_L16: result.m_Format = DXGI_FORMAT_R16_UNORM; result.m_BlockBytes = 2; break;
    case D3DFMT_R16F: result.m_Format = DXGI_FORMAT_R16_FLOAT; result.m_BlockBytes = 2; break;
    case D3DFMT_G16R16: result.m_Format = DXGI_FORMAT_R16G16_UNORM; result.m_BlockBytes = 4; break;
    case D3DFMT_G16R16F: result.m_Format = DXGI_FORMAT_R16G16_FLOAT; result.m_BlockBytes = 4; break;
    case D3DFMT_R32F: result.m_Format = DXGI_FORMAT_R32_FLOAT; result.m_BlockBytes = 4; break;
    case D3DFMT_A16B16G16R16F: result.m_Format = DXGI_FORMAT_R16G16B16A16_FLOAT; result.m_BlockBytes = 8; break;
    case D3DFMT_A32B32G32R32F: result.m_Format = DXGI_FORMAT_R32G32B32A32_FLOAT; result.m_BlockBytes = 16; break;
    case D3DFMT_D16: result.m_Format = DXGI_FORMAT_D16_UNORM; result.m_BlockBytes = 2; break;
    case D3DFMT_D24S8: result.m_Format = DXGI_FORMAT_D24_UNORM_S8_UINT; result.m_BlockBytes = 4; break;
    default: return false;
    }
    *layout = result;
    return true;
}

DXGI_FORMAT GetPCTextureResourceFormat(D3DFORMAT source)
{
    DXGI_FORMAT storage;
    unsigned pcBytes, nativeBytes;
    if (Display2::D3D11TextureFormats::LuminanceLayout(source,&storage,&pcBytes,&nativeBytes)) return storage;
    C_FormatLayout layout;
    return GetFormatLayout(source,&layout) ? GetTextureResourceFormat(layout.m_Format) : DXGI_FORMAT_UNKNOWN;
}

void PreparePCTextureData(D3DFORMAT source, unsigned width, unsigned height, unsigned depth,
                         const void* bytes, unsigned rowPitch, unsigned slicePitch,
                         std::vector<unsigned char>& converted, D3D11_SUBRESOURCE_DATA& data)
{
    data.pSysMem = bytes;
    data.SysMemPitch = rowPitch;
    data.SysMemSlicePitch = slicePitch;
    DXGI_FORMAT storage;
    unsigned pcBytes, nativeBytes;
    if (!Display2::D3D11TextureFormats::LuminanceLayout(source,&storage,&pcBytes,&nativeBytes)) return;
    const unsigned nativeRow = width * nativeBytes, nativeSlice = nativeRow * height;
    converted.resize(static_cast<size_t>(nativeSlice)*depth);
    Display2::D3D11TextureFormats::ConvertLuminance(source,width,height,depth,bytes,rowPitch,slicePitch,
        converted.data(),nativeRow,nativeSlice,true);
    data.pSysMem = converted.data();
    data.SysMemPitch = nativeRow;
    data.SysMemSlicePitch = nativeSlice;
}

DXGI_FORMAT GetSRGBFormat(DXGI_FORMAT linearFormat)
{
    switch (linearFormat)
    {
    case DXGI_FORMAT_B8G8R8A8_UNORM: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    case DXGI_FORMAT_B8G8R8X8_UNORM: return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
    case DXGI_FORMAT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case DXGI_FORMAT_BC1_UNORM: return DXGI_FORMAT_BC1_UNORM_SRGB;
    case DXGI_FORMAT_BC2_UNORM: return DXGI_FORMAT_BC2_UNORM_SRGB;
    case DXGI_FORMAT_BC3_UNORM: return DXGI_FORMAT_BC3_UNORM_SRGB;
    default: return DXGI_FORMAT_UNKNOWN;
    }
}

DXGI_FORMAT GetLinearFormat(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_B8G8R8A8_TYPELESS: case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS: case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8X8_UNORM;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS: case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_BC1_TYPELESS: case DXGI_FORMAT_BC1_UNORM_SRGB: return DXGI_FORMAT_BC1_UNORM;
    case DXGI_FORMAT_BC2_TYPELESS: case DXGI_FORMAT_BC2_UNORM_SRGB: return DXGI_FORMAT_BC2_UNORM;
    case DXGI_FORMAT_BC3_TYPELESS: case DXGI_FORMAT_BC3_UNORM_SRGB: return DXGI_FORMAT_BC3_UNORM;
    default: return format;
    }
}

DXGI_FORMAT GetTextureResourceFormat(DXGI_FORMAT format)
{
    switch (GetLinearFormat(format))
    {
    case DXGI_FORMAT_B8G8R8A8_UNORM: return DXGI_FORMAT_B8G8R8A8_TYPELESS;
    case DXGI_FORMAT_B8G8R8X8_UNORM: return DXGI_FORMAT_B8G8R8X8_TYPELESS;
    case DXGI_FORMAT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_TYPELESS;
    case DXGI_FORMAT_BC1_UNORM: return DXGI_FORMAT_BC1_TYPELESS;
    case DXGI_FORMAT_BC2_UNORM: return DXGI_FORMAT_BC2_TYPELESS;
    case DXGI_FORMAT_BC3_UNORM: return DXGI_FORMAT_BC3_TYPELESS;
    default: return format;
    }
}

HRESULT CreateTextureView(ID3D11Device* device, ID3D11Resource* resource,
                          bool srgb, ID3D11ShaderResourceView** view)
{
    if (!device || !resource || !view) return E_INVALIDARG;
    *view = nullptr;
    D3D11_RESOURCE_DIMENSION dimension;
    resource->GetType(&dimension);
    D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
    if (dimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
        D3D11_TEXTURE2D_DESC texture = {};
        static_cast<ID3D11Texture2D*>(resource)->GetDesc(&texture);
        desc.Format = texture.Format;
        if (texture.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
        {
            if (texture.ArraySize == 6)
            {
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                desc.TextureCube.MipLevels = texture.MipLevels;
            }
            else
            {
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
                desc.TextureCubeArray.MipLevels = texture.MipLevels;
                desc.TextureCubeArray.NumCubes = texture.ArraySize / 6;
            }
        }
        else if (texture.SampleDesc.Count > 1)
        {
            desc.ViewDimension = texture.ArraySize > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY : D3D11_SRV_DIMENSION_TEXTURE2DMS;
            if (texture.ArraySize > 1) desc.Texture2DMSArray.ArraySize = texture.ArraySize;
        }
        else if (texture.ArraySize > 1)
        {
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
            desc.Texture2DArray.MipLevels = texture.MipLevels;
            desc.Texture2DArray.ArraySize = texture.ArraySize;
        }
        else
        {
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipLevels = texture.MipLevels;
        }
    }
    else if (dimension == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
    {
        D3D11_TEXTURE3D_DESC texture = {};
        static_cast<ID3D11Texture3D*>(resource)->GetDesc(&texture);
        desc.Format = texture.Format;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
        desc.Texture3D.MipLevels = texture.MipLevels;
    }
    else return E_NOTIMPL;
    desc.Format = GetLinearFormat(desc.Format);
    if (desc.Format == DXGI_FORMAT_R24G8_TYPELESS) desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    // [Blur1PCDX11] D3D9 ignores SRGBTEXTURE on formats without SRGBREAD support.
    const DXGI_FORMAT gammaFormat = GetSRGBFormat(desc.Format);
    if (srgb && gammaFormat != DXGI_FORMAT_UNKNOWN) desc.Format = gammaFormat;
    return device->CreateShaderResourceView(resource, &desc, view);
}

} }
