#ifndef DISPLAY_TEXTURE_RENDER_TEXTURE_CUBE_H
#define DISPLAY_TEXTURE_RENDER_TEXTURE_CUBE_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/DeviceDefines.h"

#if defined(DEVICE_PS3GCM)
#include "Display/Texture/GCM/GCMRenderTextureCube.h"
#elif defined(DEVICE_PCD3D9) || defined(DEVICE_XB2D3D9)
#include "Display/Texture/D3D9/D3D9RenderTextureCube.h"
#elif defined(DEVICE_PCD3D10)
#include "Display/Texture/D3D10/D3D10RenderTextureCube.h"
#elif defined(DEVICE_PCGL) || defined(DEVICE_PSGL)
#include "Display/Texture/GL/GLRenderTextureCube.h"
#else
#pragma message("Unsupported device define when compiling RenderTextureCube.h")
#endif

#endif // DISPLAY_TEXTURE_RENDER_TEXTURE_CUBE_H
