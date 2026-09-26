#ifndef DISPLAY_DEVICE_D3D9_PC_D3D9_CAPS_CHECKER_H
#define DISPLAY_DEVICE_D3D9_PC_D3D9_CAPS_CHECKER_H

#ifdef _MSC_VER
#pragma once
#endif

namespace Display
{
namespace D3D9
{

bool D3DCheckObjectCaps(const D3DCAPS9& d3dCaps);
bool D3DCheckDeviceCaps(const D3DCAPS9& d3dCaps);

}; // namespace D3D9
}; // namespace Display

#endif // DISPLAY_DEVICE_D3D9_PC_D3D9_CAPS_CHECKER_H
