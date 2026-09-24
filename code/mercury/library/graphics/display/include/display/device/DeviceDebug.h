#ifndef DISPLAY_DEVICE_DEVICE_DEBUG_H
#define DISPLAY_DEVICE_DEVICE_DEBUG_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/DeviceTypes.h"

namespace Display
{

//! C_Resolution

struct C_Resolution
{
	S32							m_Value;
	U32							m_Width;
	U32							m_Height;
	Display::E_Multisample		m_Multisample;
	Util::C_String 				m_Name;
};

//! C_ResolutionManager

class C_ResolutionManager
{
public:

	C_ResolutionManager();
	~C_ResolutionManager();

	void ClearResolutions();
	void ResetResolutions();
	void AddResolution(const U32 width, const U32 height, const E_Format format, const E_Multisample multisample, const U32 flags);
	void SetResolutions(Debug::EnumBase* pEnum);
	void SetMaxResolution(const U32 width, const U32 height, const E_Format format, const E_Multisample multisample, const U32 flags);
	C_Resolution* GetResolution(const S32 resolutionIndex);

private:

	U32								m_MaxSize;
	Ut::Vector<C_Resolution>		m_Resolutions;
	Ut::Vector<Debug::Option>		m_Options;
};

extern C_ResolutionManager g_Resolutions;

}; // namespace Display

#endif // DISPLAY_DEVICE_DEVICE_DEBUG_H
