#ifndef DISPLAY_SURFACE_SURFACE_H
#define DISPLAY_SURFACE_SURFACE_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/DeviceDefines.h"

#if defined(DEVICE_PS3GCM)
#include "Display/Surface/GCM/GCMSurface.h"
#elif defined(DEVICE_PCD3D9) || defined(DEVICE_XB2D3D9)
#include "Display/Surface/D3D9/D3D9Surface.h"
#elif defined(DEVICE_PCGL) || defined(DEVICE_PSGL)
#include "Display/Surface/GL/GLSurface.h"
#else
#pragma message("Unsupported device define when compiling Surface.h")
#endif

#endif // DISPLAY_SURFACE_SURFACE_H
