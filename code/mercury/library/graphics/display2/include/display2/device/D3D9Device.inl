#ifndef DISPLAY2_D3D9_DEVICE_INL
#define DISPLAY2_D3D9_DEVICE_INL

#ifdef _MSC_VER
#pragma once
#endif

namespace Display2
{

// C_D3D9Device

INLINE U32 C_D3D9Device::IsCreated() const
{
	return m_Created;
}

INLINE GPUDevice* C_D3D9Device::GetD3DDevice()
{
	return m_pd3dDevice;
}

INLINE GPUObject* C_D3D9Device::GetD3DObject()
{
	return m_pd3dObject;
}

INLINE C_Context& C_D3D9Device::GetContext()
{
	return m_Context;
}

INLINE const C_Context& C_D3D9Device::GetContext() const
{
	return m_Context;
}

INLINE C_ContextNoCheck& C_D3D9Device::GetContextNoCheck()
{
	return m_ContextNoCheck;
}

INLINE const C_ContextNoCheck& C_D3D9Device::GetContextNoCheck() const
{
	return m_ContextNoCheck;
}

INLINE C_ContextNoCheckNoTrack& C_D3D9Device::GetContextNoCheckNoTrack()
{
	return m_ContextNoCheckNoTrack;
}

INLINE const C_ContextNoCheckNoTrack& C_D3D9Device::GetContextNoCheckNoTrack() const
{
	return m_ContextNoCheckNoTrack;
}

INLINE S32 C_D3D9Device::GetVertexDeclarationCount() const
{
	return m_VertexDeclarationList.Size();
}

INLINE C_VertexDeclaration* C_D3D9Device::GetVertexDeclaration(const S32 index) const
{
	return m_VertexDeclarationList[index];
}

// C_D3D9DeviceManager

INLINE bool C_D3D9DeviceManager::IsCreated() const
{
	return m_pd3dObject ? true : false;
}

} // namespace Display2

#endif // DISPLAY2_D3D9_DEVICE_INL
