#include "Display/Device/D3D11/D3D11Formats.h"
#include "Display2/Device/D3D11TextureFormats.h"
#include <wrl/client.h>
#include <mutex>
#include <unordered_map>

namespace Display { namespace D3D11 {

namespace
{
// [Blur1PCDX11] PC binds an existing texture across materials. Native linear
// and gamma views share that texture owner's lifetime, rather than an effect's
// current binding. This registry never retains an owner after DestroyTexture.
struct S_TextureViews
{
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> views[2];
};
std::unordered_map<ID3D11Resource*, S_TextureViews> g_textureViewOwners;
std::mutex g_textureViewOwnersMutex;
}

void RegisterTextureViewOwner(ID3D11Resource* resource)
{
    if (!resource) return;
    std::lock_guard<std::mutex> lock(g_textureViewOwnersMutex);
    g_textureViewOwners.emplace(resource, S_TextureViews{});
}

void ReleaseTextureViewOwner(ID3D11Resource* resource)
{
    std::lock_guard<std::mutex> lock(g_textureViewOwnersMutex);
    g_textureViewOwners.erase(resource);
}

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

// GetDesc need not initialize inactive union storage. Compare the fields of
// the actual view dimension rather than padding or another dimension's fields.
static bool SameTextureView(const D3D11_SHADER_RESOURCE_VIEW_DESC& a,
                            const D3D11_SHADER_RESOURCE_VIEW_DESC& b)
{
    if (a.Format != b.Format || a.ViewDimension != b.ViewDimension) return false;
    switch (a.ViewDimension)
    {
    case D3D11_SRV_DIMENSION_TEXTURE2D:
        return a.Texture2D.MostDetailedMip == b.Texture2D.MostDetailedMip &&
               a.Texture2D.MipLevels == b.Texture2D.MipLevels;
    case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:
        return a.Texture2DArray.MostDetailedMip == b.Texture2DArray.MostDetailedMip &&
               a.Texture2DArray.MipLevels == b.Texture2DArray.MipLevels &&
               a.Texture2DArray.FirstArraySlice == b.Texture2DArray.FirstArraySlice &&
               a.Texture2DArray.ArraySize == b.Texture2DArray.ArraySize;
    case D3D11_SRV_DIMENSION_TEXTURE2DMS:
        return true;
    case D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY:
        return a.Texture2DMSArray.FirstArraySlice == b.Texture2DMSArray.FirstArraySlice &&
               a.Texture2DMSArray.ArraySize == b.Texture2DMSArray.ArraySize;
    case D3D11_SRV_DIMENSION_TEXTURE3D:
        return a.Texture3D.MostDetailedMip == b.Texture3D.MostDetailedMip &&
               a.Texture3D.MipLevels == b.Texture3D.MipLevels;
    case D3D11_SRV_DIMENSION_TEXTURECUBE:
        return a.TextureCube.MostDetailedMip == b.TextureCube.MostDetailedMip &&
               a.TextureCube.MipLevels == b.TextureCube.MipLevels;
    case D3D11_SRV_DIMENSION_TEXTURECUBEARRAY:
        return a.TextureCubeArray.MostDetailedMip == b.TextureCubeArray.MostDetailedMip &&
               a.TextureCubeArray.MipLevels == b.TextureCubeArray.MipLevels &&
               a.TextureCubeArray.First2DArrayFace == b.TextureCubeArray.First2DArrayFace &&
               a.TextureCubeArray.NumCubes == b.TextureCubeArray.NumCubes;
    default:
        return false;
    }
}

HRESULT CreateTextureView(ID3D11Device* device, ID3D11Resource* resource,
                          bool srgb, ID3D11ShaderResourceView** view,
                          ID3D11ShaderResourceView* existing)
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
    // [Blur1PCDX11] FX already retains its current view. Reuse only that view,
    // with the same resource, device and complete descriptor; gamma changes
    // still create the appropriate view. No extra resource lifetime or cache.
    if (existing)
    {
        ID3D11Resource* previousResource = nullptr;
        existing->GetResource(&previousResource);
        const bool sameResource = previousResource == resource;
        if (previousResource) previousResource->Release();
        if (sameResource)
        {
            ID3D11Device* previousDevice = nullptr;
            existing->GetDevice(&previousDevice);
            const bool sameDevice = previousDevice == device;
            if (previousDevice) previousDevice->Release();
            D3D11_SHADER_RESOURCE_VIEW_DESC previous = {};
            existing->GetDesc(&previous);
            if (sameDevice && SameTextureView(previous, desc))
            {
                existing->AddRef();
                *view = existing;
                return S_FALSE;
            }
        }
    }
    // [Blur1PCDX11] Only explicitly owned textures retain views across changes
    // of material. The descriptor is immutable for a resource; unsupported
    // gamma uses the same linear slot, as on PC. Validate the device as well.
    std::lock_guard<std::mutex> lock(g_textureViewOwnersMutex);
    auto owner = g_textureViewOwners.find(resource);
    if (owner == g_textureViewOwners.end())
        return device->CreateShaderResourceView(resource, &desc, view);
    auto& retained = owner->second.views[srgb && gammaFormat != DXGI_FORMAT_UNKNOWN ? 1 : 0];
    if (retained)
    {
        Microsoft::WRL::ComPtr<ID3D11Device> viewDevice;
        retained->GetDevice(&viewDevice);
        D3D11_SHADER_RESOURCE_VIEW_DESC previous = {};
        retained->GetDesc(&previous);
        if (viewDevice.Get() == device && SameTextureView(previous, desc))
        {
            retained->AddRef();
            *view = retained.Get();
            return S_FALSE;
        }
    }
    const HRESULT result = device->CreateShaderResourceView(resource, &desc, view);
    if (SUCCEEDED(result)) retained = *view;
    return result;
}

} }
