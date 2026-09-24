#ifndef DISPLAY2_INCLUDES_H
#define DISPLAY2_INCLUDES_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display2Defines.h"

#if PLATFORM_PC

	#ifdef BUILD_CONFIG_DEBUG
	#define D3D_DEBUG_INFO
	#endif

	#include <d3d9.h>
	#include <d3dx9.h>
	#include <dxerr.h>

#elif PLATFORM_XENON

	#include <xgraphics.h>
	#include <d3dx9.h>

#elif PLATFORM_SPU

	#include <cell/gcm_spu.h>

#elif PLATFORM_PS3
	#if CFG_DEBUG
	//#define CELL_GCM_DEBUG
	#endif

	#include <cell/gcm.h>
	#include <cell/cgb.h>
	#include <cell/resc.h>

#endif

#endif // DISPLAY2_INCLUDES_H
