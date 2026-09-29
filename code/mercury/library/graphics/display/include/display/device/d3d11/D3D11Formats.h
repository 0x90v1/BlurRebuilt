#ifndef DISPLAY_DEVICE_D3D11_FORMATS_H
#define DISPLAY_DEVICE_D3D11_FORMATS_H

#include <d3d9.h>
#include <dxgiformat.h>

namespace Display { namespace D3D11 {

struct C_FormatLayout
{
    DXGI_FORMAT m_Format;
    unsigned m_BlockWidth;
    unsigned m_BlockHeight;
    unsigned m_BlockBytes;
};

// [Blur1PCDX11] Map the PC asset's D3D9 texel layout to native DXGI storage.
// Return false for layouts that need conversion rather than silently changing data.
bool GetFormatLayout(D3DFORMAT source, C_FormatLayout* layout);
DXGI_FORMAT GetSRGBFormat(DXGI_FORMAT linearFormat);

} }

#endif
