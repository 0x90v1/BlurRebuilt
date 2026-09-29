#ifndef DISPLAY_WINDOW_WINDOW_H
#define DISPLAY_WINDOW_WINDOW_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/DeviceDefines.h"

#if defined(DEVICE_PCD3D11) // [Blur1PCDX11] Native swap-chain window resource.
#include "Display/Window/D3D11/D3D11Window.h"
#elif defined(DEVICE_PS3GCM)
#include "Display/Window/GCM/GCMWindow.h"
#elif defined(DEVICE_PCD3D9) || defined(DEVICE_XB2D3D9)
#include "Display/Window/D3D9/D3D9Window.h"
#else
#pragma message("Unsupported device define when compiling Window.h")
#endif

#endif // DISPLAY_WINDOW_WINDOW_H
