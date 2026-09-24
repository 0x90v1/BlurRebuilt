#ifndef DISPLAY2_D3D9_DEVICE_H
#define DISPLAY2_D3D9_DEVICE_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display2/Device/BaseDevice.h"
#include "Display2/Buffer/Buffer.h"

namespace Display2
{

#define GPU_ERROR_SUCCESS					ERROR_SUCCESS
#define GPU_ERROR_OUT_OF_MEMORY				ERROR_NOT_ENOUGH_MEMORY

//! C_D3D9DeviceParams

class C_D3D9DeviceParams
{
public:

	C_D3D9DeviceParams();
	~C_D3D9DeviceParams();

public:

	GPUDevice*								m_pd3dDevice;
	GPUObject*								m_pd3dObject;
};

//! C_D3D9Device

class C_D3D9Device : public C_BaseDevice
{
public:

	C_D3D9Device();
	~C_D3D9Device();

	U32 IsCreated() const;

	GPUDevice* GetD3DDevice();
	GPUObject* GetD3DObject();

	U32 GetLastError() const;
	void SetLastError(const U32 error);

	void AllowOutOfMemory(const bool allowOutOfMemory);

	C_Context& GetContext();
	const C_Context& GetContext() const;
	
	C_ContextNoCheck& GetContextNoCheck();
	const C_ContextNoCheck& GetContextNoCheck() const;
	
	C_ContextNoCheckNoTrack& GetContextNoCheckNoTrack();
	const C_ContextNoCheckNoTrack& GetContextNoCheckNoTrack() const;

	S32 GetVertexDeclarationCount() const;
	C_VertexDeclaration* GetVertexDeclaration(const S32 index) const;
	C_VertexDeclaration* FindVertexDeclaration(const S32 vertexElementCount, const C_VertexElement* pVertexElements) const;

	C_VertexDeclaration* CreateVertexDeclaration(const C_VertexFormat& vertexFormat);
	bool CreateVertexDeclaration(const C_VertexFormat& vertexFormat, C_VertexDeclaration* pVertexDeclaration);
	C_VertexDeclaration* CreateVertexDeclaration(const C_VertexElement* pVertexElements);
	bool CreateVertexDeclaration(const C_VertexElement* pVertexElements, C_VertexDeclaration* pVertexDeclaration);

	void DestroyVertexDeclaration(C_VertexDeclaration* pVertexDeclaration);
	void DestroyVertexDeclarations();

protected:

	U32										m_Created;

	U32										m_LastError;
	
	bool									m_AllowOutOfMemory;

	GPUDevice*								m_pd3dDevice;
	GPUObject*								m_pd3dObject;

	C_Context								m_Context;
	C_ContextNoCheck						m_ContextNoCheck;
	C_ContextNoCheckNoTrack					m_ContextNoCheckNoTrack;
	C_ContextState							m_ContextState;

	Btl::Vector<C_VertexDeclaration*>		m_VertexDeclarationList;
};

//! C_D3D9DeviceManagerParams

class C_D3D9DeviceManagerParams
{
public:
	
	C_D3D9DeviceManagerParams();
	~C_D3D9DeviceManagerParams();
};

//! C_D3D9DeviceManager

class C_D3D9DeviceManager
{
public:

	C_D3D9DeviceManager();
	~C_D3D9DeviceManager();

	bool IsCreated() const;

	GPUObject* GetD3DObject();

protected:

	bool CreateObject();
	void DestroyObject();

protected:

	GPUObject*								m_pd3dObject;
};

} // namespace Display2

#include "Display2/Device/D3D9Device.inl"

#endif // DISPLAY2_D3D9_DEVICE_H
