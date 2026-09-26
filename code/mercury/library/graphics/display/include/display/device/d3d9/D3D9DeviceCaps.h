#ifndef DISPLAY_DEVICE_D3D9_D3D9_DEVICE_CAPS_H
#define DISPLAY_DEVICE_D3D9_D3D9_DEVICE_CAPS_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Core/Includes.h"
#include "Display/Device/DeviceTypes.h"

namespace Display
{
namespace D3D9
{

//! C_DeviceCaps

struct C_DeviceCaps
{
	static const bool	Blend(const E_Blend blend);
	static const bool	CmpFunc(const E_CmpFunc cmpFunc);
	static const bool	MipMapLodBias();
	static const bool	OcclusionQueries();
	static const bool	VertexElementType(const E_VertexElementType vertexElementType);
	static const DWORD	VertexProcessing();
	static const bool	NonPowerOfTwoTextures();
	static const bool	StretchRect(const E_TextureFilter textureFilter);

	static D3DCAPS9		m_d3dCaps;
};

}; // namespace D3D9
}; // namespace Display

#include "Display/Device/D3D9/D3D9DeviceCaps.inl"

#endif // DISPLAY_DEVICE_D3D9_D3D9_DEVICE_CAPS_H
