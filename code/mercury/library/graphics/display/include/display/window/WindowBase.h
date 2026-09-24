#ifndef DISPLAY_WINDOW_WINDOW_BASE_H
#define DISPLAY_WINDOW_WINDOW_BASE_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/DeviceTypes.h"

namespace Display
{

#ifdef PLATFORM_XENON
enum E_TileDirection
{
	E_TileDirection_None,
	E_TileDirection_Horizontal,
	E_TileDirection_Vertical
};
#endif

// C_TileSetup

struct C_TileSetup
{
	U32					m_Width;
	U32					m_Height;
	E_Multisample		m_Multisample;
	#ifdef PLATFORM_XENON
	static const U32	m_MaxTiles = 8;
	U32					m_TileCount;
	D3DRECT				m_Tiles[m_MaxTiles];
	E_TileDirection		m_TileDirection;
	F32					m_TileDivisions[m_MaxTiles-1];
	#endif
};

C_TileSetup GetTileSetup(const U32 width, const U32 height, const E_Format colourFormat, const E_Format depthStencilFormat, const E_Multisample multisample);

// C_VideoMode

struct C_VideoMode
{
	U32					m_Width;
	U32					m_Height;
	E_Format			m_Format;
	#ifdef PLATFORM_PC
	U32					m_RefreshRate;	// [Blur2PC] retail PC keeps the adapter mode's refresh rate here
	#endif
	bool				m_WideScreen;
	bool				m_HiDef;

	const bool operator<(const C_VideoMode& other) const;
};

}; // namespace Display

#endif // DISPLAY_WINDOW_WINDOW_BASE_H
