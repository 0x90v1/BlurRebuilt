#ifndef DISPLAY_BUFFER_VERTEX_BUFFER_PATCHED_H
#define DISPLAY_BUFFER_VERTEX_BUFFER_PATCHED_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/DeviceDefines.h"

#if defined(DEVICE_PS3GCM)
#include "Display/Buffer/GCM/GCMVertexBufferPatched.h"
#elif defined(DEVICE_PCD3D9) || defined(DEVICE_XB2D3D9)
#include "Display/Buffer/D3D9/D3D9VertexBufferPatched.h"
#else
#pragma message("Unsupported device define when compiling VertexBufferPatched.h")
#endif

#endif // DISPLAY_BUFFER_VERTEX_BUFFER_PATCHED_H
