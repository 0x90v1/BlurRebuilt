#ifndef DISPLAY_UTILS_SCREENSHOT_H
#define DISPLAY_UTILS_SCREENSHOT_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Surface/Surface.h"

namespace Display
{

void TakeScreenshot(const Sys::C_FilePath& fileName, const C_Surface* pSurface);
void TakeScreenshot(const Sys::C_FilePath& fileName);

}; // namespace Display

#endif // DISPLAY_UTILS_SCREENSHOT_H
