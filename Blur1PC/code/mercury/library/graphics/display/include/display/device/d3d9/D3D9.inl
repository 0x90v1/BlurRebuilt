#ifndef DISPLAY_DEVICE_D3D9_D3D9_INL
#define DISPLAY_DEVICE_D3D9_D3D9_INL

#ifdef _MSC_VER
#pragma once
#endif

namespace Display
{
namespace D3D9
{

FORCE_INLINE IDirect3DDevice9* GetD3DDevice()
{
	assert(g_pd3dDevice);
	return g_pd3dDevice;
}

}; // namespace D3D9
}; // namespace Display

#endif // DISPLAY_DEVICE_D3D9_D3D9_INL
