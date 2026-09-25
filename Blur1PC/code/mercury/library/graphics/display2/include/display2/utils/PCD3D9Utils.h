#ifndef DISPLAY2_PCD3D9_UTILS_H
#define DISPLAY2_PCD3D9_UTILS_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display2/Device/Device.h"

namespace Display2
{

bool D3DXCreateTexture2DFromFile(const Util::C_String& fileName, C_Texture* pTexture);

bool D3DXCreateTexture3DFromFile(const Util::C_String& fileName, C_Texture* pTexture);

bool D3DXCreateTextureCubeFromFile(const Util::C_String& fileName, C_Texture* pTexture);

} // namespace Display2

#endif // DISPLAY2_PCD3D9_UTILS_H
