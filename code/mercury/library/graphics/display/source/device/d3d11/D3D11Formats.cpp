#include "Display/Device/D3D11/D3D11Formats.h"

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

} }
