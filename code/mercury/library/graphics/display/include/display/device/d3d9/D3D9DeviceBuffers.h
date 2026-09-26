#ifndef DISPLAY_DEVICE_D3D9_D3D9_DEVICE_BUFFERS_H
#define DISPLAY_DEVICE_D3D9_D3D9_DEVICE_BUFFERS_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Surface/D3D9/D3D9Surface.h"
#include "Display/Texture/D3D9/D3D9Texture2D.h"

namespace Display
{
namespace D3D9
{

class C_D3D9Device;
class C_PCD3D9Device;
class C_XB2D3D9Device;

//! C_BackBuffer

class C_BackBuffer
{
friend class C_D3D9Device;
friend class C_PCD3D9Device;
friend class C_XB2D3D9Device;
public:

	C_BackBuffer();
	~C_BackBuffer();

	const U32 GetWidth() const;
	const U32 GetHeight() const;
	const E_Format GetColourFormat() const;
	const E_Format GetDepthStencilFormat() const;
	const E_Multisample GetMultisample() const;
	const C_Surface* GetColourSurface() const;
	const C_Surface* GetDepthStencilSurface() const;

private:

	U32					m_Width;
	U32					m_Height;
	E_Format			m_ColourFormat;
	C_Surface			m_ColourSurface;
	E_Format			m_DepthStencilFormat;
	C_Surface			m_DepthStencilSurface;
	E_Multisample		m_Multisample;
};

//! C_FrontBuffer

class C_FrontBuffer
{
friend class C_D3D9Device;
friend class C_PCD3D9Device;
friend class C_XB2D3D9Device;
public:

	C_FrontBuffer();
	~C_FrontBuffer();

	const U32 GetWidth() const;
	const U32 GetHeight() const;
	const E_Format GetFormat() const;
	const C_Texture2D* GetTexture() const;

private:

	U32					m_Width;
	U32					m_Height;
	E_Format			m_Format;
	C_Texture2D			m_Texture;
};

//! C_RenderTarget

class C_RenderTarget
{
friend class C_D3D9Device;
friend class C_PCD3D9Device;
friend class C_XB2D3D9Device;
public:

	C_RenderTarget();
	~C_RenderTarget();

	const U32 GetColourWidth() const;
	const U32 GetColourHeight() const;
	const E_Format GetColourFormat() const;
	const E_Multisample GetColourMultisample() const;
	const C_Surface* GetColourSurface() const;
	
	const U32 GetDepthStencilWidth() const;
	const U32 GetDepthStencilHeight() const;
	const E_Format GetDepthStencilFormat() const;
	const E_Multisample GetDepthStencilMultisample() const;
	const C_Surface* GetDepthStencilSurface() const;

private:

	const C_Surface*	m_pColourSurface;
	const C_Surface*	m_pDepthStencilSurface;
};

}; // namespace D3D9
}; // namespace Display

#include "Display/Device/D3D9/D3D9DeviceBuffers.inl"

#endif // DISPLAY_DEVICE_D3D9_D3D9_DEVICE_BUFFERS_H
