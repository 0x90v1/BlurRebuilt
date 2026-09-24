#ifndef DISPLAY_DEVICE_D3D9_XB2_D3D9_DEVICE_INL
#define DISPLAY_DEVICE_D3D9_XB2_D3D9_DEVICE_INL

#ifdef _MSC_VER
#pragma once
#endif

namespace Display
{
namespace D3D9
{

INLINE bool C_XB2D3D9Device::IsTilingEnabled() const
{
	return m_TilingEnable;
}

INLINE bool C_XB2D3D9Device::IsTilingActive() const
{
	return m_TilingActive;
}

INLINE C_MemoryBuffer* C_XB2D3D9Device::GetLargeHeap()
{
	return &m_LargeHeap;
}

INLINE C_MemoryBuffer* C_XB2D3D9Device::GetDynamicHeap()
{
	return &m_DynamicHeap;
}

INLINE C_MemoryBuffer* C_XB2D3D9Device::GetSmallHeap()
{
	return &m_SmallHeap;
}

INLINE bool C_XB2D3D9Device::CreateColourSurface(const U32 width, const U32 height, const U32 flags, const E_Format format, const E_Multisample multisample, const C_SurfaceParams* pParams, C_Surface* pSurface)
{
	return CreateColourSurface(width, height, flags, D3D9Format(format, flags), D3D9Multisample(multisample), pParams, pSurface);
}

INLINE void C_XB2D3D9Device::DestroyColourSurface(C_Surface* pSurface)
{
	DestroySurface(pSurface);
}

INLINE bool C_XB2D3D9Device::CreateDepthStencilSurface(const U32 width, const U32 height, const U32 flags, const E_Format format, const E_Multisample multisample, const C_SurfaceParams* pParams, C_Surface* pSurface)
{
	return CreateDepthStencilSurface(width, height, flags, D3D9Format(format, flags), D3D9Multisample(multisample), pParams, pSurface);
}

INLINE void C_XB2D3D9Device::DestroyDepthStencilSurface(C_Surface* pSurface)
{
	DestroySurface(pSurface);
}

INLINE bool C_XB2D3D9Device::CreateRenderTexture2D(const U32 width, const U32 height, const U32 levels, const U32 flags, const E_Format format, const E_Multisample multisample, const C_SurfaceParams* pParams, C_RenderTexture2D* pRenderTexture, U8* pAddress, const U32 addressSize)
{
	return CreateRenderTexture2D(width, height, levels, flags, D3D9Format(format, flags), D3D9Multisample(multisample), pParams, pRenderTexture, pAddress, addressSize);
}

INLINE bool C_XB2D3D9Device::CreateRenderTextureCube(const U32 width, const U32 levels, const U32 flags, const E_Format format, const E_Multisample multisample, const C_SurfaceParams* pParams, C_RenderTextureCube* pRenderTexture, U8* pAddress, const U32 addressSize)
{
	return CreateRenderTextureCube(width, levels, flags, D3D9Format(format, flags), D3D9Multisample(multisample), pParams, pRenderTexture, pAddress, addressSize);
}

INLINE U32 C_XB2D3D9Device::GetFreeResourceMemory()
{
	return m_LargeHeap.GetFree();
}

INLINE void C_XB2D3D9Device::SetMaxBeginVerticesUsage(const U32 beginVerticesUsage)
{
	m_MaxBeginVerticesUsage = beginVerticesUsage;
}

INLINE const bool C_XB2D3D9Device::BeginVerticesLocked() const
{
	return m_BeginVerticesLocked;
}

}; // namespace D3D9
}; // namespace Display

#endif // DISPLAY_DEVICE_D3D9_XB2_D3D9_DEVICE_INL
