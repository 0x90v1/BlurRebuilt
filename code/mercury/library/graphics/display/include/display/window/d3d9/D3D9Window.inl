#ifndef DISPLAY_WINDOW_D3D9_D3D9_WINDOW_INL
#define DISPLAY_WINDOW_D3D9_D3D9_WINDOW_INL

#ifdef _MSC_VER
#pragma once
#endif

namespace Display
{
namespace D3D9
{

INLINE E_Multisample C_Window::GetMultisample() const
{
	return m_Multisample;
}

INLINE C_Surface* C_Window::GetColourSurface(const U32 i)
{
	return &m_ColourSurfaceWithAA;
}

INLINE C_Surface* C_Window::GetDepthStencilSurface()
{
	return &m_DepthStencilSurfaceWithAA;
}

INLINE C_Texture2D* C_Window::GetColourTexture(const U32 i)
{
	return &m_ColourTexture;
}

INLINE C_Texture2D* C_Window::GetDepthStencilTexture()
{
	return &m_DepthStencilTexture;
}

}; // namespace D3D9
}; // namespace Display

#endif // DISPLAY_WINDOW_D3D9_D3D9_WINDOW_INL
