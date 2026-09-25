#ifndef DISPLAY_SURFACE_D3D9_D3D9_SURFACE_H
#define DISPLAY_SURFACE_D3D9_D3D9_SURFACE_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/D3D9/D3D9.h"
#include "Display/Resource/Resource.h"
#include "Display/Texture/TextureParams.h"

namespace Display
{

typedef Display2::C_SurfaceParams C_SurfaceParams;

typedef Display2::C_Surface C_Surface;

}; // namespace Display

#include "Display/Surface/D3D9/D3D9Surface.inl"

#endif // DISPLAY_SURFACE_D3D9_D3D9_SURFACE_H
