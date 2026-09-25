#include "stdafx.h"
#include "Display/Device/DeviceSetup.h"

#ifdef PLATFORM_XENON
#include "Display/Device/D3D9/D3D9DeviceTypes.h"
#endif

namespace Display
{
	
const C_TileSetup g_TileSetups[] =
{
	// 640x360
	{
		640, 360, E_Multisample_None,
		#ifdef PLATFORM_XENON
		1,
		{
			{   0, 0, 640, 360 },
		},
		#endif
	},
	{
		640, 360, E_Multisample_2,
		#ifdef PLATFORM_XENON
		1,
		{
			{   0, 0, 640, 360 },
		},
		#endif
	},
	{
		640, 360, E_Multisample_4,
		#ifdef PLATFORM_XENON
		1,
		{
			{   0, 0, 640, 360 },
		},
		#endif
	},

	// 640x480 resolutions
	{
		640, 480, E_Multisample_None,
		#ifdef PLATFORM_XENON
		1,
		{
			{ 0, 0, 640, 480 },
		},
		#endif
	},
	{
		640, 480, E_Multisample_2,
		#ifdef PLATFORM_XENON
		1,
		{
			{ 0, 0, 640, 480 },
		},
		#endif
	},
	{
		640, 480, E_Multisample_4,
		#ifdef PLATFORM_XENON
		1,
		{
			{ 0, 0, 640, 480 },
		},
		#endif
	},

	// 640x720
	{
		640, 720, E_Multisample_None,
		#ifdef PLATFORM_XENON
		1,
		{
			{   0, 0, 640, 720 },
		},
		#endif
	},
	{
		640, 720, E_Multisample_2,
		#ifdef PLATFORM_XENON
		1,
		{
			{   0, 0, 640, 720 },
		},
		#endif
	},
	{
		640, 720, E_Multisample_4,
		#ifdef PLATFORM_XENON
		2,
		{
			{   0, 0, 320, 720 }, // Vertical split
			{ 320, 0, 640, 720 },
		},
		#endif
	},

	// 1024x576
	{
		1024, 576, E_Multisample_None,
		#ifdef PLATFORM_XENON
		1,
		{
			{ 0, 0, 1024, 576 },
		},
		#endif
	},
	{
		1024, 576, E_Multisample_2,
		#ifdef PLATFORM_XENON
		1,
		{
			{ 0, 0, 1024, 576 },
		},
		#endif
	},
	{
		1024, 576, E_Multisample_4,
		#ifdef PLATFORM_XENON
		3,
		{
			{ 0, 0,   1024, 256 },
			{ 0, 256, 1024, 512 },
			{ 0, 512, 1024, 576 },
		},
		#endif
	},
	{
		1024, 576, E_Multisample_4,
		#ifdef PLATFORM_XENON
		3,
		{
			{   0, 0, 344,  576 },
			{ 344, 0, 688,  576 },
			{ 688, 0, 1024, 576 },
		},
		#endif
	},

	// 960x640
	{
		960, 640, E_Multisample_None,
		#ifdef PLATFORM_XENON
		1,
		{
			{ 0, 0, 960, 640 },
		},
		#endif
	},
	{
		960, 640, E_Multisample_2,
		#ifdef PLATFORM_XENON
		1,
		{
			{ 0, 0, 960, 640 },
		},
		#endif
	},
	{
		960, 640, E_Multisample_4,
		#ifdef PLATFORM_XENON
		2,
		{
			{ 0,   0, 960, 320 },
			{ 0, 320, 960, 640 },
		},
		#endif
	},

	// 1280x720
	{
		1280, 720, E_Multisample_None,
		#ifdef PLATFORM_XENON
		1,
		{
			{ 0, 0, 1280, 720 },
		},
		#endif
	},
	{
		1280, 720, E_Multisample_2,
		#ifdef PLATFORM_XENON
		2,
		{
			{ 0, 0,   1280, 480 }, // Horizontal split (larger first tile)
			{ 0, 480, 1280, 720 },
		},
		#endif
	},
	{
		1280, 720, E_Multisample_2,
		#ifdef PLATFORM_XENON
		2,
		{
			{ 0,   0,  800, 720 }, // Vertical split (larger first tile)
			{ 800, 0, 1280, 720 },
		},
		#endif
	},
	{
		1280, 720, E_Multisample_2,
		#ifdef PLATFORM_XENON
		2,
		{
			{ 0, 0,   1280, 360 }, // Horizontal split
			{ 0, 360, 1280, 720 },
		},
		#endif
	},
	{
		1280, 720, E_Multisample_2,
		#ifdef PLATFORM_XENON
		2,
		{
			{ 0,   0,  640, 720 }, // Vertical split
			{ 640, 0, 1280, 720 },
		},
		#endif
	},
	{
		1280, 720, E_Multisample_4,
		#ifdef PLATFORM_XENON
		3,
		{
			{ 0, 0,   1280, 256 }, // Horizontal split
			{ 0, 256, 1280, 512 },
			{ 0, 512, 1280, 720 },
		},
		#endif
	},
	{
		1280, 720, E_Multisample_4,
		#ifdef PLATFORM_XENON
		4,
		{
			{ 0,   0,   320, 720 }, // Vertical split
			{ 320, 0,   640, 720 },
			{ 640, 0,   960, 720 },
			{ 960, 0,  1280, 720 },
		},
		#endif
	},

	// Invalid (use default setup)
	{
		1280, 720, E_Multisample_None,
		#ifdef PLATFORM_XENON
		1,
		{
			{ 0, 0, 1280, 720 },
		},
		#endif
	},
};

C_TileSetup LookupTileSetup(const U32 width, const U32 height, const E_Format colourFormat, const E_Format depthStencilFormat, const E_Multisample multisample)
{
	#ifdef PLATFORM_XENON
	{
		U32 surfaceSize =
			XGSurfaceSize(width, height, D3D9::D3D9Format(colourFormat), D3D9::D3D9Multisample(multisample))+
			XGSurfaceSize(width, height, D3D9::D3D9Format(depthStencilFormat), D3D9::D3D9Multisample(multisample));

		U32 tileCount =
			(surfaceSize + GPU_EDRAM_TILES - 1) / GPU_EDRAM_TILES;

		if(tileCount == 1)
		{
			C_TileSetup tileSetup;
			tileSetup.m_Width			= width;
			tileSetup.m_Height			= height;
			tileSetup.m_Multisample		= multisample;
			tileSetup.m_TileCount		= tileCount;
			tileSetup.m_Tiles[0].x1 	= 0;
			tileSetup.m_Tiles[0].y1 	= 0;
			tileSetup.m_Tiles[0].x2 	= width;
			tileSetup.m_Tiles[0].y2 	= height;
			return tileSetup;
		}
	}
	#endif

	const U32 tileCount = (sizeof(g_TileSetups)/sizeof(C_TileSetup))-1;

	U32 bestMatch = tileCount; 

	for(U32 tileIndex=0; tileIndex<tileCount; tileIndex++)
	{
		const C_TileSetup& tileSetup = g_TileSetups[tileIndex];

		if(tileSetup.m_Width != width)
			continue;
		if(tileSetup.m_Height != height)
			continue;
		if(tileSetup.m_Multisample != multisample)
			continue;

		bestMatch = tileIndex;

		break;
	}

	return g_TileSetups[bestMatch];
}

}; // namespace Display
