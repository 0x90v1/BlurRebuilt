#ifndef DISPLAY_TEXTURE_D3D9_D3D9_RENDER_TEXTURE_CUBE_H
#define DISPLAY_TEXTURE_D3D9_D3D9_RENDER_TEXTURE_CUBE_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Texture/D3D9/D3D9TextureCube.h"
#include "Display/Surface/D3D9/D3D9Surface.h"

namespace Display
{
namespace D3D9
{

//! C_RenderTextureCube

class C_RenderTextureCube : public Display2::C_RenderTextureCube
{
public:

	C_RenderTextureCube();
	~C_RenderTextureCube();
};

}; // namespace D3D9
}; // namespace Display

#include "Display/Texture/D3D9/D3D9RenderTextureCube.inl"

#endif // DISPLAY_TEXTURE_D3D9_D3D9_RENDER_TEXTURE_CUBE_H
