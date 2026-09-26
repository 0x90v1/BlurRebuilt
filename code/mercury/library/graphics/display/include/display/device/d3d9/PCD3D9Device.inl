#ifndef DISPLAY_DEVICE_D3D9_PC_D3D9_DEVICE_INL
#define DISPLAY_DEVICE_D3D9_PC_D3D9_DEVICE_INL

#ifdef _MSC_VER
#pragma once
#endif

namespace Display
{
namespace D3D9
{

INLINE U32 C_PCD3D9Device::GetFreeResourceMemory()
{
	return 0;
}

INLINE const bool C_PCD3D9Device::BeginVerticesLocked() const
{
	return m_BeginVerticesLocked;
}

INLINE void C_PCD3D9Device::SetMaxBeginVerticesUsage(const U32 beginVerticesUsage)
{
	m_MaxBeginVerticesUsage = beginVerticesUsage;
}

INLINE void C_PCD3D9Device::SetUserMaxBeginVerticesUsage(const F32 beginVerticesUsageScale)
{
	assert(beginVerticesUsageScale >= 0.0f && beginVerticesUsageScale <= 1.0f);
	m_UserMaxBeginVerticesUsage = (U32)(((F32)m_MaxBeginVerticesUsage)*beginVerticesUsageScale);
	m_UserBeginVerticesUsage = 0;
}

INLINE bool C_PCD3D9Device::GetLost()
{
	return m_Lost;
}

INLINE void C_PCD3D9Device::SetLost(const bool lost)
{
	m_Lost = lost;
}

INLINE bool C_PCD3D9Device::GetValid()
{
	return m_Valid;
}

INLINE void C_PCD3D9Device::SetValid(const bool valid)
{
	m_Valid = valid;
}

}; // namespace D3D9
}; // namespace Display

#endif // DISPLAY_DEVICE_D3D9_PC_D3D9_DEVICE_INL
