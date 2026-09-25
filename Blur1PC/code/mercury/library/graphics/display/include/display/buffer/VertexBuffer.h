#ifndef DISPLAY_BUFFER_VERTEX_BUFFER_H
#define DISPLAY_BUFFER_VERTEX_BUFFER_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/DeviceDefines.h"

#if defined(DEVICE_PS3GCM)
#include "Display/Buffer/GCM/GCMVertexBuffer.h"
#elif defined(DEVICE_PCD3D9) || defined(DEVICE_XB2D3D9)
#include "Display/Buffer/D3D9/D3D9VertexBuffer.h"
#elif defined(DEVICE_PCD3D10)
#include "Display/Buffer/D3D10/D3D10VertexBuffer.h"
#elif defined(DEVICE_PCGL) || defined(DEVICE_PS3GL)
#include "Display/Buffer/GL/GLVertexBuffer.h"
#else
#pragma message("Unsupported device define when compiling VertexBuffer.h")
#endif

namespace Display
{

bool CopyVertexBuffer(C_VertexBuffer* pSrc, C_VertexBuffer* pDst);

}; // namespace Display

#endif // DISPLAY_BUFFER_VERTEX_BUFFER_H
