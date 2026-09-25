#ifndef DISPLAY_WINDOW_D3D9_D3D9_WINDOW_H
#define DISPLAY_WINDOW_D3D9_D3D9_WINDOW_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Surface/D3D9/D3D9Surface.h"
#include "Display/Texture/D3D9/D3D9Texture2D.h"
#include "Display/Window/WindowBase.h"

namespace Display
{
namespace D3D9
{

//! C_Window

class C_Window
{
public:

	C_Window(const Maths::C_Rect& coords);
	~C_Window();

	E_Multisample GetMultisample() const;

	C_Surface* GetColourSurface(const U32 i = 0);
	C_Surface* GetDepthStencilSurface();
	
	C_Texture2D* GetColourTexture(const U32 i = 0);
	C_Texture2D* GetDepthStencilTexture();

	void Resize(const U32 width, const U32 height, const E_Multisample multisample, const E_Format colourFormat, const E_Format depthStencilFormat);

public:

	Maths::C_Rect				m_Coords;
	
	U32							m_Width;
	U32							m_Height;
	E_Multisample				m_Multisample;

	C_TileSetup					m_TileSetup;

	E_Format					m_ColourFormat;
	C_Surface					m_ColourSurfaceWithAA;
	C_Surface					m_ColourSurfaceWithoutAA;
	C_Texture2D					m_ColourTexture;

	E_Format					m_DepthStencilFormat;
	C_Surface					m_DepthStencilSurfaceWithAA;
	C_Surface					m_DepthStencilSurfaceWithoutAA;
	C_Texture2D					m_DepthStencilTexture;
};

}; // namespace D3D9
}; // namespace Display

#include "Display/Window/D3D9/D3D9Window.inl"

#endif // DISPLAY_WINDOW_D3D9_D3D9_WINDOW_H
