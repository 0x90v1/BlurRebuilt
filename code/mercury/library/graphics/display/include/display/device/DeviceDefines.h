#ifndef DISPLAY_DEVICE_DEVICE_DEFINES_H
#define DISPLAY_DEVICE_DEVICE_DEFINES_H

#ifdef _MSC_VER
#pragma once
#endif

#include <Core/Core.h>

#if defined(PLATFORM_XENON)
	#define GRAPHICS_D3D9
	#define SHADERS_HLSL
	#define DEVICE_XB2D3D9
#elif defined(PLATFORM_SPU)
	#define GRAPHICS_GCM
	#define DEVICE_PS3GCM
#elif defined(PLATFORM_PS3)
	#if !defined(GRAPHICS_PS3GL) && !defined(GRAPHICS_GCM)
		#if 1
			#define SHADERS_CG_PS
			#define GRAPHICS_GCM
			#define DEVICE_PS3GCM
		#else
			#define GRAPHICS_GL
			#define DEVICE_PS3GL
		#endif
	#else
		#if defined(GRAPHICS_GL)
			#define SHADERS_CG_VS
			#define SHADERS_CG_PS
			#define DEVICE_PS3GL
		#elif defined(GRAPHICS_GCM)
			#define SHADERS_CG_PS
			#define DEVICE_PS3GCM
		#else
			#pragma message("PS3_DEVICE_UNSUPPORTED")
		#endif
	#endif
	#define SHADERS_CG
#elif defined(PLATFORM_PC)
	#if defined(BLUR_USE_DX11) // [Blur1PCDX11] Select the native PC backend.
		#define GRAPHICS_D3D11
		#define SHADERS_HLSL
		#define DEVICE_PCD3D11
	#elif !defined(GRAPHICS_D3D9) && !defined(GRAPHICS_GL)
		#if 1
			#define GRAPHICS_D3D9
			#define SHADERS_HLSL
			#define DEVICE_PCD3D9
		#else
			#define GRAPHICS_GL
			#define SHADERS_CG
			#define SHADERS_CG_VS
			#define SHADERS_CG_PS
			#define DEVICE_PCGL
		#endif
	#else
		#if defined(GRAPHICS_D3D9)
			#define SHADERS_HLSL
			#define DEVICE_PCD3D9
		#elif defined(GRAPHICS_GL)
			#define SHADERS_CG
			#define DEVICE_PCGL
		#else
			#pragma message("PC_DEVICE_UNSUPPORTED")
		#endif
	#endif
#else
	#pragma message("DEVICE_UNSUPPORTED")
#endif

#if !defined(CFG_SPEED)
#define DEVICE_DEBUG // Run display in debug mode.
#endif

#if !defined(CFG_SPEED)
//#define DEVICE_INTERFACES // Enforce interface esque function consistency.
#endif

#if !defined(CFG_SPEED) || (defined(PLATFORM_PS3) && !defined(CFG_GOLD))
#define USE_DISPLAY_MEMORY_TRACKER
#endif

#if defined(PLATFORM_XENON)
#define DEVICE_DISABLE_PREDICATED_TILING // Take out predicated tiling on the xenon.
#endif

#if (defined(PLATFORM_XENON) && (defined(CFG_DEBUG) || !defined(DEVICE_DISABLE_PREDICATED_TILING)))
#define DEVICE_DISABLE_OCCLUSION_QUERIES // Take out d3d occlusion queries.
#endif

#if defined(PLATFORM_XENON) || defined(PLATFORM_PC)
#define USE_PCFSHADOWMAP // TODO: These need to be moved out of here at some point!
#endif
#if defined(PLATFORM_PS3)
#define USE_PCFSHADOWMAP
#define USE_HARDWARESHADOWMAP
#endif

#if defined(PLATFORM_PS3)
#define USE_CGB_SHADERS
#endif

#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3) || defined(PLATFORM_PC)	// [Blur2PC] retail Blur PC renders into display windows too (T089)
#define USE_DISPLAY_WINDOWS
#endif

#if defined(GRAPHICS_D3D9) && (defined(CFG_PROFILE) || defined(CFG_RELEASE) || (defined(CFG_DEBUG) && defined(PLATFORM_PC)))
#define USE_D3D_PIX
#endif

#if defined(GRAPHICS_GCM) && (defined(CFG_PROFILE) || defined(CFG_RELEASE))
//#define USE_GCM_P3X
#endif

#if defined(GRAPHICS_GCM) && (defined(CFG_PROFILE) || defined(CFG_RELEASE) || defined(CFG_DEBUG))
#define USE_GCM_HUD
#endif

#endif // DISPLAY_DEVICE_DEVICE_DEFINES_H
