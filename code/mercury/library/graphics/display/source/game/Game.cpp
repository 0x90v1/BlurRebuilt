#include "stdafx.h"
#include "Display/Game/Game.h"
#include "Display/Device/Device.h"

#if 0

DEBUG_ENUM(Display::E_Resolution)
	DE_OPTION(Display::E_Resolution_640x480x1,	"640x480x1")
	DE_OPTION(Display::E_Resolution_640x480x4,	"640x480x4")
	DE_OPTION(Display::E_Resolution_960x640x2,	"960x640x2")
	DE_OPTION(Display::E_Resolution_960x640x4,	"960x640x4")
	DE_OPTION(Display::E_Resolution_1024x576x2,	"1024x576x2")
	DE_OPTION(Display::E_Resolution_1024x576x4,	"1024x576x4")
	DE_OPTION(Display::E_Resolution_1280x720x1,	"1280x720x1")
	DE_OPTION(Display::E_Resolution_1280x720x2,	"1280x720x2")
	DE_OPTION(Display::E_Resolution_1280x720x4,	"1280x720x4")
END_DEBUG_ENUM

Debug::Enum<Display::E_Resolution> g_debugGameResolution("Graphics.Render.Game.Resolution", Display::E_Resolution_1280x720x2);

#endif

namespace Display
{

#if 0

#ifdef PLATFORM_XENON

const C_TileSetup g_TileSetups[] =
{
	// 640x360 resolutions
	{
		640, 360, E_Multisample_None,
		1,
		{
			{   0, 0, 640, 360 }
		},
	},
	{
		640, 360, E_Multisample_2,
		1,
		{
			{   0, 0, 640, 360 }
		},
	},
	{
		640, 360, E_Multisample_4,
		1,
		{
			{   0, 0, 640, 360 }
		},
	},
	
	// 640x720 resolutions
	{
		640, 720, E_Multisample_None,
		1,
		{
			{   0, 0, 640, 720 }
		}
	},
	{
		640, 720, E_Multisample_2,
		1,
		{
			{   0, 0, 640, 720 }
		},
	},
	{
		640, 720, E_Multisample_4,
		2,
		{
			{   0, 0, 320, 720 }, // Vertical split
			{ 320, 0, 640, 720 }
		},
	},
	
	// 640x480 resolutions
	{
		640, 480, E_Multisample_None,
		1,
		{
			{ 0, 0, 640, 480 }
		},
	},
	{
		640, 480, E_Multisample_4,
		1,
		{
			{ 0, 0, 640, 480 }
		},
	},

	// 1024x576 resolutions
	{
		1024, 576, E_Multisample_None,
		1,
		{
			{ 0, 0, 1024, 576 }
		},
	},
	{
		1024, 576, E_Multisample_2,
		1,
		{
			{ 0, 0, 1024, 576 }
		},
	},
	{
		1024, 576, E_Multisample_4,
		3,
		{
			{ 0, 0,   1024, 256 },
			{ 0, 256, 1024, 512 },
			{ 0, 512, 1024, 576 }
		},
	},
	{
		1024, 576, E_Multisample_4,
		3,
		{
			{   0, 0, 344,  576 },
			{ 344, 0, 688,  576 },
			{ 688, 0, 1024, 576 }
		},
	},

	// 960x640 resolutions
	{
		960, 640, E_Multisample_None,
		1,
		{
			{ 0, 0, 960, 640 }
		},
	},
	{
		960, 640, E_Multisample_2,
		1,
		{
			{ 0, 0, 960, 640 }
		},
	},
	{
		960, 640, E_Multisample_4,
		2,
		{
			{ 0,   0, 960, 320 },
			{ 0, 320, 960, 640 }
		},
	},

	// 1280x720 resolutions
	{
		1280, 720, E_Multisample_None,
		1,
		{
			{ 0, 0, 1280, 720 }
		},
	},
	{
		1280, 720, E_Multisample_2,
		2,
		{
			{ 0, 0,   1280, 480 }, // Horizontal split (larger first tile)
			{ 0, 480, 1280, 720 }
		},
	},
	{
		1280, 720, E_Multisample_2,
		2,
		{
			{ 0,   0,  800, 720 }, // Vertical split (larger first tile)
			{ 800, 0, 1280, 720 }
		},
	},
	{
		1280, 720, E_Multisample_2,
		2,
		{
			{ 0, 0,   1280, 360 }, // Horizontal split
			{ 0, 360, 1280, 720 }
		},
	},
	{
		1280, 720, E_Multisample_2,
		2,
		{
			{ 0,   0,  640, 720 }, // Vertical split
			{ 640, 0, 1280, 720 }
		},
	},
	{
		1280, 720, E_Multisample_4,
		3,
		{
			{ 0, 0,   1280, 256 }, // Horizontal splits
			{ 0, 256, 1280, 512 },
			{ 0, 512, 1280, 720 }
		},
	},
	{
		1280, 720, E_Multisample_4,
		4,
		{
			{ 0,   0,   320, 720 }, // Vertical splits
			{ 320, 0,   640, 720 },
			{ 640, 0,   960, 720 },
			{ 960, 0,  1280, 720 }
		},
	},
	
	// Invalid resolution (use default setup)
	{
		1280, 720, E_Multisample_None,
		1,
		{
			{ 0, 0, 1280, 720 }
		},
	},
};

C_TileSetup LookupTileSetup(const U32 width, const U32 height, const E_Format colourFormat, const E_Format depthStencilFormat, const E_Multisample multisample)
{
	U32 surfaceSize = 0;
	surfaceSize += GetSurfaceSize(width, height, colourFormat, multisample);
	surfaceSize += GetSurfaceSize(width, height, depthStencilFormat, multisample);

	U32 tileCount = (surfaceSize + GPU_EDRAM_TILES - 1) / GPU_EDRAM_TILES;

	if (tileCount == 1)
	{
		C_TileSetup viewSetup;

		viewSetup.m_Width = width;
		viewSetup.m_Height = height;
		viewSetup.m_Multisample = multisample;
		viewSetup.m_TileCount = tileCount;
		viewSetup.m_Tiles[0].x1 = 0;
		viewSetup.m_Tiles[0].y1 = 0;
		viewSetup.m_Tiles[0].x2 = width;
		viewSetup.m_Tiles[0].y2 = height;

		return viewSetup;
	}

	U32 matchIndex = 0;
	U32 bestMatchIndex = _countof(g_TileSetups)-1; 

	for (U32 i=0; i<_countof(g_TileSetups)-1; ++i)
	{
		const C_TileSetup& tileSetup = g_TileSetups[i];

		if (tileSetup.m_Width != width)
			continue;
		if (tileSetup.m_Height != height)
			continue;
		if (tileSetup.m_Multisample != multisample)
			continue;

		bestMatchIndex = i;

		break;
	}

	return g_TileSetups[bestMatchIndex];
}

void CalculateTileSize(const C_TileSetup& tileSetup, U32& width, U32& height)
{
	U32 maxWidth  = 0;
	U32 maxHeight = 0;

    for(U32 i = 0; i < tileSetup.m_TileCount; ++i)
    {
        U32 tempWidth  = tileSetup.m_Tiles[i].x2 - tileSetup.m_Tiles[i].x1;
        U32 tempHeight = tileSetup.m_Tiles[i].y2 - tileSetup.m_Tiles[i].y1;

        if(tempWidth > maxWidth )
            maxWidth = tempWidth;
        if(tempHeight > maxHeight )
            maxHeight = tempHeight;
    }

	U32 tileWidth  = 0;
    U32 tileHeight = 0;

    switch(tileSetup.m_Multisample)
    {
		case E_Multisample_None:
            tileWidth = XGNextMultiple(maxWidth, GPU_EDRAM_TILE_WIDTH_1X);
            tileHeight = XGNextMultiple(maxHeight, GPU_EDRAM_TILE_HEIGHT_1X);
            break;
        case E_Multisample_2:
            tileWidth = XGNextMultiple(maxWidth, GPU_EDRAM_TILE_WIDTH_2X);
            tileHeight = XGNextMultiple(maxHeight, GPU_EDRAM_TILE_HEIGHT_2X);
            break;
        case E_Multisample_4:
            tileWidth = XGNextMultiple(maxWidth, GPU_EDRAM_TILE_WIDTH_4X);
            tileHeight = XGNextMultiple(maxHeight, GPU_EDRAM_TILE_HEIGHT_4X);
            break;
    }

    if (tileSetup.m_TileCount > 1)
    {
        tileWidth = XGNextMultiple(tileWidth, GPU_TEXTURE_TILE_DIMENSION);
        tileHeight = XGNextMultiple(tileHeight, GPU_TEXTURE_TILE_DIMENSION);
    }

	width = tileWidth;
	height = tileHeight;
}

#endif // PLATFORM_XENON

C_GameView::C_GameView(const U32 width, const U32 height, const E_Multisample multisample) :
	m_Width(width),
	m_Height(height),
	m_Multisample(multisample),
	m_ColourFormat(E_Format_A2B10G10R10F),
	m_DepthStencilFormat(E_Format_D24FS8)
{
}

C_GameView::~C_GameView()
{
}

bool C_GameView::Create(U32& curMemSize, U8*& curMemPtr)
{
	// with aa
	{
		#ifdef PLATFORM_XENON
		m_WithAA.m_TileSetup = LookupTileSetup(m_Width, m_Height, m_ColourFormat, m_DepthStencilFormat, m_Multisample);
		U32 tileWidth = 0;
		U32 tileHeight = 0;
		CalculateTileSize(m_WithAA.m_TileSetup, tileWidth, tileHeight);
		#else
		U32 tileWidth = m_Width;
		U32 tileHeight = m_Height;
		#endif

		C_SurfaceParams surfaceParams;

		Device()->CreateColourSurface(tileWidth, tileHeight, 0, m_ColourFormat, m_Multisample, &surfaceParams, &m_WithAA.m_ColourSurface);

		#ifdef PLATFORM_XENON
		surfaceParams.m_Base = GetSurfaceSize(tileWidth, tileHeight, m_ColourFormat, m_Multisample);
		#endif

		Device()->CreateDepthStencilSurface(tileWidth, tileHeight, 0, m_DepthStencilFormat, m_Multisample, &surfaceParams, &m_WithAA.m_DepthStencilSurface);
	}

	// without aa
	{
		#ifdef PLATFORM_XENON
		m_WithoutAA.m_TileSetup = LookupTileSetup(m_Width, m_Height, m_ColourFormat, m_DepthStencilFormat, E_Multisample_None);
		U32 tileWidth = 0;
		U32 tileHeight = 0;
		CalculateTileSize(m_WithoutAA.m_TileSetup, tileWidth, tileHeight);
		#else
		U32 tileWidth = m_Width;
		U32 tileHeight = m_Height;
		#endif

		C_SurfaceParams surfaceParams;

		Device()->CreateColourSurface(tileWidth, tileHeight, 0, m_ColourFormat, E_Multisample_None, &surfaceParams, &m_WithoutAA.m_ColourSurface);

		#ifdef PLATFORM_XENON
		surfaceParams.m_Base = GetSurfaceSize(tileWidth, tileHeight, m_ColourFormat, E_Multisample_None);
		#endif

		Device()->CreateDepthStencilSurface(tileWidth, tileHeight, 0, m_DepthStencilFormat, E_Multisample_None, &surfaceParams, &m_WithoutAA.m_DepthStencilSurface);
	}

	#ifdef PLATFORM_XENON
	{
		Device()->CreateRenderTexture2D(m_Width, m_Height, 1, E_Usage_Colour | E_Usage_Indirect, D3DFMT_A2B10G10R10F_EDRAM, D3DMULTISAMPLE_NONE, NULL, &m_ColourRenderTexture, curMemPtr);
		U32 size = XGSetTextureHeader(m_Width, m_Height, 1, 0, D3DFMT_A16B16G16R16F_EXPAND, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, NULL, NULL, NULL);
		curMemSize += size;
		curMemPtr += size;

		#ifndef CFG_GOLD
		m_ColourRenderTexture.SetFilename("_GameViewColourBuffer");
		#endif
	}
	{
		Device()->CreateRenderTexture2D(m_Width, m_Height, 1, E_Usage_DepthStencil | E_Usage_Indirect, D3DFMT_D24FS8, D3DMULTISAMPLE_NONE, NULL, &m_DepthStencilRenderTexture, curMemPtr);
		U32 size = XGSetTextureHeader(m_Width, m_Height, 1, 0, D3DFMT_D24FS8, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, NULL, NULL, NULL);
		curMemSize += size;
		curMemPtr += size;

		#ifndef CFG_GOLD
		m_DepthStencilRenderTexture.SetFilename("_GameViewDepthStencilBuffer");
		#endif
	}
	#endif
	
	return true;
}

void C_GameView::Destroy()
{
	Device()->Destroy(&m_WithAA.m_ColourSurface);
	Device()->Destroy(&m_WithAA.m_DepthStencilSurface);
	Device()->Destroy(&m_WithoutAA.m_ColourSurface);
	Device()->Destroy(&m_WithoutAA.m_DepthStencilSurface);
	Device()->Destroy(&m_ColourRenderTexture);
	Device()->Destroy(&m_DepthStencilRenderTexture);
}

C_Game::C_Game()
{
	// Memory management
	m_MemID						= -1;
	m_MemSize					= 15 * 1024 * 1024;
	m_MemPtr					= NULL;
	m_MemFlags					= 0;
	// Resolution management
	m_ResMode					= -1;
	m_ResWidth					= 0;
	m_ResHeight					= 0;
	m_ResMultisample			= E_Multisample_None;

	assert(g_pGame == NULL);
	g_pGame = this;

	SetRegion(C_CoreSystems::Instance().GetConsoleRegion());
}

C_Game::~C_Game()
{
	ClearViews();

	assert(g_pGame == this);
	g_pGame = NULL;
}

void C_Game::ClearViews()
{
	for(S32 i=0; i<m_ViewList.Size(); i++)
		delete m_ViewList[i];
	m_ViewList.Clear();
}

S32 C_Game::AddView(const U32 width, const U32 height, const E_Multisample multisample)
{
	C_GameView* pView = new C_GameView(width, height, multisample);
	m_ViewList.PushBack(pView);
	return m_ViewList.Size()-1;
}

bool C_Game::CreateViews()
{
	U32 curMemSize = 0;
	U8* curMemPtr = m_MemPtr;

	for(S32 i=0; i<m_ViewList.Size(); i++)
	{
		if(!m_ViewList[i]->Create(curMemSize, curMemPtr))
			return false;
	}

	if(curMemSize > m_MemSize)
	{
		LOGFATAL(g_Log, "Tried to allocate too much render target memory '%i / %i'\n", curMemSize, m_MemSize);
	}
	else
	{
		LOGINFO(g_Log, "Render target memory '%i'\n", curMemSize);
	}

	return true;
}

void C_Game::DestroyViews()
{
	for(S32 i=0; i<m_ViewList.Size(); i++)
	{
		m_ViewList[i]->Destroy();
	}
}

bool C_Game::CreateMemory()
{
	RESOURCE_MEMORY_ID("RenderTargets");

	#ifdef PLATFORM_XENON
	if(!m_MemPtr)
	{
		m_MemID = Device()->MemoryGetSystemID();
		if (!Device()->AllocResourceMemory(E_ResourceType_Surface, m_MemFlags, m_MemSize, &m_MemPtr, m_MemID))
			return false;
	}
	#endif

	return true;
}

void C_Game::DestroyMemory()
{
	#ifdef PLATFORM_XENON
	if(m_MemPtr)
	{
		Device()->FreeResourceMemory(E_ResourceType_Surface, m_MemFlags, m_MemSize, m_MemPtr, m_MemID);
		m_MemPtr = NULL;
		m_MemSize = 0;
		m_MemID = -1;
	}
	#endif
}

bool C_Game::CheckForResolutionChange(const bool disableMultisample)
{
	const S32 resMode = g_debugGameResolution;

	if(m_ResMode != resMode)
	{
		m_ResWidth = 1280;
		m_ResHeight = 720;
		m_ResMultisample = E_Multisample_2;

		switch(resMode)
		{
		case E_Resolution_640x480x1:
		case E_Resolution_640x480x4:
			m_ResWidth = 640;
			m_ResHeight = 480;
			if (disableMultisample)
			{
				m_ResMultisample = E_Multisample_None;
			}
			else
			{
				switch(resMode)
				{
				case E_Resolution_640x480x1:
					m_ResMultisample = E_Multisample_None;
					break;
				case E_Resolution_640x480x4:
					m_ResMultisample = E_Multisample_4;
					break;
				}
			}
			break;

		case E_Resolution_960x640x2:
		case E_Resolution_960x640x4:
			m_ResWidth = 960;
			m_ResHeight = 640;
			if (disableMultisample)
			{
				m_ResMultisample = E_Multisample_None;
			}
			else
			{
				switch(resMode)
				{
				case E_Resolution_960x640x2:
					m_ResMultisample = E_Multisample_2;
					break;
				case E_Resolution_960x640x4:
					m_ResMultisample = E_Multisample_4;
					break;
				}
			}
			break;

		case E_Resolution_1024x576x2:
		case E_Resolution_1024x576x4:
			m_ResWidth = 1024;
			m_ResHeight = 576;
			if (disableMultisample)
			{
				m_ResMultisample = E_Multisample_None;
			}
			else
			{
				switch(resMode)
				{
				case E_Resolution_1024x576x2:
					m_ResMultisample = E_Multisample_2;
					break;
				case E_Resolution_1024x576x4:
					m_ResMultisample = E_Multisample_4;
					break;
				}
			}
			break;

		case E_Resolution_1280x720x2:
		case E_Resolution_1280x720x4:
			m_ResWidth = 1280;
			m_ResHeight = 720;
			if (disableMultisample)
			{
				m_ResMultisample = E_Multisample_None;
			}
			else
			{
				switch(resMode)
				{
				case E_Resolution_1280x720x1:
					m_ResMultisample = E_Multisample_None;
					break;
				case E_Resolution_1280x720x2:
					m_ResMultisample = E_Multisample_2;
					break;
				case E_Resolution_1280x720x4:
					m_ResMultisample = E_Multisample_4;
					break;
				}
			}
			break;

		default:
			LOGFATAL(g_Log, "Unsupported resolution '%i%\n", resMode);
			return false;
		}

		#ifdef PLATFORM_XENON
		Device()->Unbind();
		
		Device()->GetD3DDevice()->SetRenderTarget(0, NULL);
		Device()->GetD3DDevice()->SetDepthStencilSurface(NULL);
		
		Device()->DestroyVolatile();
		DestroyMemory();

		Device()->m_DeviceParams.m_Width = m_ResWidth;
		Device()->m_DeviceParams.m_Height = m_ResHeight;
		Device()->m_DeviceParams.m_Multisample = m_ResMultisample;

		if(!Device()->CreateVolatile())
			return false;
		if(!CreateMemory())
			return false;
		#endif

		m_ResMode = resMode;
	}

	return true;
}

C_Game* g_pGame = NULL;

void Game(C_Game* pGame)
{
	g_pGame = pGame;
}

#endif

} // namespace Display
