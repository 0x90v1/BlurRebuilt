#ifndef DISPLAY_DEVICE_D3D9_PC_D3D9_DEVICE_H
#define DISPLAY_DEVICE_D3D9_PC_D3D9_DEVICE_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/D3D9/D3D9Device.h"
#include "Display/Buffer/DynamicVertexBuffer.h"

namespace Display
{
namespace D3D9
{

class C_PCD3D9Device : public C_D3D9Device
{
public:

	C_PCD3D9Device();
	~C_PCD3D9Device();

	/**************************************************************************************************
	- Public cross-platform functions.
	- Safe to use by game as these functions are guaranteed to be cross-platform.
	- Basically anything here must be the interface.
	**************************************************************************************************/
	
	bool Present();

	// Dynamic vertex buffer management
	const bool BeginVerticesLocked() const;
	void* BeginVertices(const E_PrimitiveType primitiveType, const U32 vertexCount, const U32 vertexStride, C_VertexDeclaration* pVertexDeclaration, const bool checkUsage = false);
	void EndVertices(void* pVertex = NULL);
	void FakeEndVertices(void* pVertex = NULL);

	// Render texture 2d management
	bool CreateRenderTexture2D(const U32 width, const U32 height, const U32 levels, U32 flags, const E_Format format, const E_Multisample multisample, const C_SurfaceParams* pParams, C_RenderTexture2D* pRenderTexture, U8* pAddress = NULL, const U32 addressSize = 0);
	void DestroyRenderTexture2D(C_RenderTexture2D* pRenderTexture);
	
	// Memory management
	U32 GetFreeResourceMemory();

	void SetMaxBeginVerticesUsage(const U32 beginVerticesUsage);
	void SetUserMaxBeginVerticesUsage(const F32 beginVerticesUsageScale);
	
	/**************************************************************************************************
	- Public platform specific functions.
	- Not safe to use by game as these functions are not guaranteed to be cross-platform.
	- Wrap functions calls with define DEVICE_XED3D9.
	- Basically anything here dont put in the interface.
	**************************************************************************************************/

	// Memory management
	bool AllocMemory(const E_ResourceType type, const U32 size, U8** ppAddress, const S32 systemID = -1);
	void FreeMemory(const E_ResourceType type, const U32 size, U8* pAddress, const S32 systemID = -1);

	bool GetLost();
	bool GetValid();
	
	bool CanReset();
	bool CanRender();

	bool SupportForRenderTarget(const E_ResourceType resourceType, const E_Format format);

protected:

	/**************************************************************************************************
	- Protected cross-platform functions.
	- Basically anything here must be the interface.
	**************************************************************************************************/

	// Device management
	bool CreateDevice();
	bool CreateDeviceVolatile();
	void DestroyDeviceVolatile();
	void DestroyDevice();
	
	/**************************************************************************************************
	- Protected platform specific functions.
	- Basically anything here dont put in the interface.
	**************************************************************************************************/

	// TODO: Add functions here.

private:

	/**************************************************************************************************
	- Private cross-platform functions.
	- Basically anything here must be the interface.
	**************************************************************************************************/

	// TODO: Add functions here.

	/**************************************************************************************************
	- Private platform specific functions.
	- Basically anything here dont put in the interface.
	**************************************************************************************************/

	// Device management
	bool SetVertexShaderProfile();
	bool SetPixelShaderProfile();
	bool CreateD3D9();
	bool ResetD3D9();
	void SetPresentParamsD3D9();

	void CheckUnManagedUserCreatedQueriesAreFreed();

	void SetLost(const bool lost);
	void SetValid(const bool valid);

private:

	bool					m_Lost;
	bool					m_Valid;

	U32						m_BeginVerticesUsage;
	U32						m_MaxBeginVerticesUsage;

	U32						m_UserBeginVerticesUsage;
	U32						m_UserMaxBeginVerticesUsage;

	bool					m_BeginVerticesLocked;

	#ifndef CFG_GOLD
	U8*						m_BeginVerticesStart;
	U32						m_BeginVerticesSize;
	#endif

	C_DynamicVertexBuffer	m_DynamicVertexBuffer;
};

}; // namespace D3D9
}; // namespace Display

#include "Display/Device/D3D9/PCD3D9Device.inl"

#endif // DISPLAY_DEVICE_D3D9_PC_D3D9_DEVICE_H
