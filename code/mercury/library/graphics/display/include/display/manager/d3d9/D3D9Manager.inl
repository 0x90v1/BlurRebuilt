#ifndef DISPLAY_MANAGER_D3D9_D3D9_MANAGER_INL
#define DISPLAY_MANAGER_D3D9_D3D9_MANAGER_INL

#ifdef _MSC_VER
#pragma once
#endif

namespace Display
{
namespace D3D9
{

INLINE const S32 C_D3D9Manager::GetMultisampleCount() const
{
	return m_MultisampleList.Size();
}

INLINE const C_Multisample& C_D3D9Manager::GetMultisample(const S32 multisampleIndex) const
{
	assert(multisampleIndex >= 0 && multisampleIndex < m_MultisampleList.Size());
	return m_MultisampleList[multisampleIndex];
}

INLINE const S32 C_D3D9Manager::GetVideoModeCount() const
{
	return m_VideoModeList.Size();
}

INLINE const C_VideoMode& C_D3D9Manager::GetVideoMode(const S32 videoModeIndex)
{
	assert(videoModeIndex >= 0 && videoModeIndex < m_VideoModeList.Size());
	return m_VideoModeList[videoModeIndex];
}

}; // namespace D3D9
}; // namespace Display

#endif // DISPLAY_MANAGER_D3D9_D3D9_MANAGER_INL
