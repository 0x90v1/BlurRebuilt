#ifndef DISPLAY_BUFFER_INDEX_BUFFER_H
#define DISPLAY_BUFFER_INDEX_BUFFER_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/DeviceDefines.h"

#if defined(DEVICE_PS3GCM)
#include "Display/Buffer/GCM/GCMIndexBuffer.h"
#elif defined(DEVICE_PCD3D9) || defined(DEVICE_XB2D3D9)
#include "Display/Buffer/D3D9/D3D9IndexBuffer.h"
#elif defined(DEVICE_PCD3D10)
#include "Display/Buffer/D3D10/D3D10IndexBuffer.h"
#elif defined(DEVICE_PCGL) || defined(DEVICE_PS3GL)
#include "Display/Buffer/GL/GLIndexBuffer.h"
#else
#pragma message("Unsupported device define when compiling IndexBuffer.h")
#endif

namespace Display
{

bool CopyIndexBuffer(C_IndexBuffer* pSrc, C_IndexBuffer* pDst);

}; // namespace Display

#endif // DISPLAY_BUFFER_INDEX_BUFFER_H
