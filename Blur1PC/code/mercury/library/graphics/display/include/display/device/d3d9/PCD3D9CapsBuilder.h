#ifndef DISPLAY_DEVICE_D3D9_PC_D3D9_CAPS_BUILDER_H
#define DISPLAY_DEVICE_D3D9_PC_D3D9_CAPS_BUILDER_H

#ifdef _MSC_VER
#pragma once
#endif

namespace Display
{
namespace D3D9
{

void DBPrint(const char* text, ...);
void DBError(const char* text, ...);

bool D3DBuildCaps(const D3DCAPS9& d3dCaps, const char* ofilename);

}; // namespace D3D9
}; // namespace Display

#endif // DISPLAY_DEVICE_D3D9_PC_D3D9_CAPS_BUILDER_H
