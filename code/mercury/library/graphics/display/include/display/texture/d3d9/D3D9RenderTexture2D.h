#ifndef DISPLAY_TEXTURE_D3D9_D3D9_RENDER_TEXTURE_2D_H
#define DISPLAY_TEXTURE_D3D9_D3D9_RENDER_TEXTURE_2D_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Texture/D3D9/D3D9Texture2D.h"
#include "Display/Surface/D3D9/D3D9Surface.h"

namespace Display
{
namespace D3D9
{

//! C_RenderTexture2D

class C_RenderTexture2D : public Display2::C_RenderTexture2D
{
public:

	C_RenderTexture2D();
	~C_RenderTexture2D();

	void				Resolve(const U32 level = 0) const;
};

}; // namespace D3D9
}; // namespace Display

#include "Display/Texture/D3D9/D3D9RenderTexture2D.inl"

#endif // DISPLAY_TEXTURE_D3D9_D3D9_RENDER_TEXTURE_2D_H
