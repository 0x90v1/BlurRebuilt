#ifndef DISPLAY_TEXTURE_D3D9_D3D9_TEXTURE_H
#define DISPLAY_TEXTURE_D3D9_D3D9_TEXTURE_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/D3D9/D3D9.h"
#include "Display/Resource/Resource.h"

namespace Display
{

typedef Display2::C_Texture C_Texture;
typedef Display2::C_Texture C_Texture2D;
typedef Display2::C_Texture C_Texture3D;
typedef Display2::C_Texture C_TextureCube;

}; // namespace Display

#include "Display/Texture/D3D9/D3D9Texture.inl"

#endif // DISPLAY_TEXTURE_D3D9_D3D9_TEXTURE_H
