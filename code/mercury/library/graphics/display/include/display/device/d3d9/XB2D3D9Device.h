#ifndef DISPLAY_DEVICE_D3D9_XB2_D3D9_DEVICE_H
#define DISPLAY_DEVICE_D3D9_XB2_D3D9_DEVICE_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/D3D9/D3D9Device.h"
#include "Display/Memory/MemoryManager2.h"

namespace Display
{
namespace D3D9
{

void CalculateTileSize(const C_TileSetup& tileSetup, U32& width, U32& height);

//! E_AllocHeap

enum E_AllocHeap
{
	E_AllocHeap_Large,
	E_AllocHeap_Small,
};

//! E_AllocType

enum E_AllocType
{
	E_AllocType_Start,
	E_AllocType_End,
};

//! C_XB2D3D9Device

class C_XB2D3D9Device : public C_D3D9Device
{
public:

	C_XB2D3D9Device();
	~C_XB2D3D9Device();

	/**************************************************************************************************
	- Public cross-platform functions.
	- Safe to use by game as these functions are guaranteed to be cross-platform.
	- Basically anything here must be the interface.
	**************************************************************************************************/

	void Unbind();
	bool Present();

	// Dynamic vertex buffer management
	const bool BeginVerticesLocked() const;
	void* BeginVertices(const E_PrimitiveType primitiveType, const U32 vertexCount, const U32 vertexStride, C_VertexDeclaration* pVertexDeclaration, const bool checkUsage = false);
	void EndVertices(void* pVertex = NULL);

	// Surface management
	bool CreateColourSurface(const U32 width, const U32 height, const U32 flags, const E_Format format, const E_Multisample multisample, const C_SurfaceParams* pParams, C_Surface* pSurface);
	bool CreateColourSurface(const U32 width, const U32 height, const U32 flags, const D3DFORMAT format, const D3DMULTISAMPLE_TYPE multisample, const C_SurfaceParams* pParams, C_Surface* pSurface);
	void DestroyColourSurface(C_Surface* pSurface);
	bool CreateDepthStencilSurface(const U32 width, const U32 height, const U32 flags, const E_Format format, const E_Multisample multisample, const C_SurfaceParams* pParams, C_Surface* pSurface);
	bool CreateDepthStencilSurface(const U32 width, const U32 height, const U32 flags, const D3DFORMAT format, const D3DMULTISAMPLE_TYPE multisample, const C_SurfaceParams* pParams, C_Surface* pSurface);
	void DestroyDepthStencilSurface(C_Surface* pSurface);
	void DestroySurface(C_Surface* pSurface);

	// Texture 2d management
	bool CreateTexture2D(const U32 width, const U32 height, const U32 levels, const U32 flags, const E_Format format, C_Texture2D* pTexture, U8* pAddress = NULL, const U32 addressSize = 0);
	void DestroyTexture2D(C_Texture2D* pTexture);

	// Texture 3d management
	bool CreateTexture3D(const U32 width, const U32 height, const U32 depth, const U32 levels, const U32 flags, const E_Format format, C_Texture3D* pTexture, U8* pAddress = NULL, const U32 addressSize = 0);
	void DestroyTexture3D(C_Texture3D* pTexture);

	// Texture cube management
	bool CreateTextureCube(const U32 width, const U32 levels, const U32 flags, const E_Format format, C_TextureCube* pTexture, U8* pAddress = NULL, const U32 addressSize = 0);
	void DestroyTextureCube(C_TextureCube* pTexture);

	// Render texture 2d management
	bool CreateRenderTexture2D(const U32 width, const U32 height, const U32 levels, const U32 flags, const E_Format format, const E_Multisample multisample, const C_SurfaceParams* pParams, C_RenderTexture2D* pRenderTexture, U8* pAddress = NULL, const U32 addressSize = 0);
	bool CreateRenderTexture2D(const U32 width, const U32 height, const U32 levels, U32 flags, const D3DFORMAT format, const D3DMULTISAMPLE_TYPE multisample, const C_SurfaceParams* pParams, C_RenderTexture2D* pRenderTexture, U8* pAddress = NULL, const U32 addressSize = 0);
	void DestroyRenderTexture2D(C_RenderTexture2D* pRenderTexture);

	// Render texture cube management
	bool CreateRenderTextureCube(const U32 width, const U32 levels, const U32 flags, const E_Format format, const E_Multisample multisample, const C_SurfaceParams* pParams, C_RenderTextureCube* pRenderTexture, U8* pAddress = NULL, const U32 addressSize = 0);
	bool CreateRenderTextureCube(const U32 width, const U32 levels, U32 flags, const D3DFORMAT format, const D3DMULTISAMPLE_TYPE multisample, const C_SurfaceParams* pParams, C_RenderTextureCube* pRenderTexture, U8* pAddress = NULL, const U32 addressSize = 0);
	void DestroyRenderTextureCube(C_RenderTextureCube* pRenderTexture);

	// Vertex buffer management
	bool CreateVertexBuffer(const U32 size, const U32 flags, C_VertexBuffer* pVertexBuffer, bool allowFail = false);
	void DestroyVertexBuffer(C_VertexBuffer* pVertexBuffer);

	// Index buffer management
	bool CreateIndexBuffer(const U32 size, const U32 flags, const E_Format format, C_IndexBuffer* pIndexBuffer, bool allowFail = false);
	void DestroyIndexBuffer(C_IndexBuffer* pIndexBuffer);

	// Vertex shader management
	bool CreateVertexShader(const U32 size, const S32* pData, const U32 flags, C_VertexShader* pVertexShader);
	void DestroyVertexShader(C_VertexShader* pVertexShader);

	// Pixel shader management
	bool CreatePixelShader(const U32 size, const S32* pData, const U32 flags, C_PixelShader* pPixelShader);
	void DestroyPixelShader(C_PixelShader* pPixelShader);

	// Memory management
	U32 GetFreeResourceMemory();

	/**************************************************************************************************
	- Public platform specific functions.
	- Not safe to use by game as these functions are not guaranteed to be cross-platform.
	- Wrap functions calls with define DEVICE_XB2D3D9.
	- Basically anything here dont put in the interface.
	**************************************************************************************************/
	
	void Resolve(const E_Resolve resolve, const C_Texture2D* pTexture, const U32 level = 0);
	void Resolve(const DWORD flags, const C_Texture2D* pTexture, const D3DRECT* pSrcRect, const D3DPOINT* pDstPoint, const U32 level = 0);

	// Tiling management
	bool IsTilingEnabled() const;
	bool IsTilingActive() const;
	void BeginTiling(const U32 tileCount, const D3DRECT* pTileRects);
	void EndTiling(const C_Texture* pTexture);
	
	// Memory management
	const U32 GetMemoryAlignment(const E_ResourceType type, const U32 flags);
	bool AllocResourceMemory(const E_ResourceType type, U32& flags, const U32 size, U8** ppAddress, const S32 systemID);
	void FreeResourceMemory(const E_ResourceType type, const U32 flags, const U32 size, U8* pAddress, const S32 systemID);
	bool AllocMemory(const U32 align, const U32 size, U8** ppAddress, const E_AllocHeap allocHeap, const E_AllocType allocType, const Display2::E_MemoryType memoryType = Display2::E_MemoryType_Unknown);
	void FreeMemory(const U32 size, U8* pAddress, const E_AllocHeap allocHeap, const Display2::E_MemoryType memoryType = Display2::E_MemoryType_Unknown);

	// Heap management
	C_MemoryBuffer* GetLargeHeap();
	C_MemoryBuffer* GetDynamicHeap();
	C_MemoryBuffer* GetSmallHeap();

	// Fence management
	U32 InsertFence();
	U32 GetCurrentFence();
	bool IsFencePassed(const U32 value);
	void BlockOnFence(const U32 value);

	IDirect3DTexture9* AllocTextureHeader();
	void FreeTextureHeader(IDirect3DTexture9*& pd3dTexture);
	void FreeBaseTextureHeader(IDirect3DBaseTexture9*& pd3dBaseTexture);

	void ResizeSurfaces();
	
	void ClearWindows();
	void ResizeWindows();
	bool CreateWindows();
	void DestroyWindows();

	void SetWindow(const S32 windowIndex, const bool aa);
	S32	 GetWindowIndex();
	void BeginWindow();
	void EndWindow();
	void RestoreWindow(const S32 windowIndex, const U32 colourWrite = E_ColourWriteEnable_All);

	void BlockUntilIdle();

	void SetShaderGPRAllocation(const U32 flags, const U32 vertexShaderCount, const U32 pixelShaderCount);

	void SetMaxBeginVerticesUsage(const U32 beginVerticesUsage);
	void SetUserMaxBeginVerticesUsage(const F32 beginVerticesUsageScale);

	void DumpMemoryHeaps();

	void BeforePresentCallback();
	void AfterPresentCallback();

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

	bool CreateD3D9();

	bool CreateMemory();
	void DestroyMemory();
	
	bool CreateSurfaces();
	void DestroySurfaces();

	void DestroyBackBuffers();
	void DestroyFrontBuffers();

	void SetPresentParams(D3DPRESENT_PARAMETERS& d3dPresentParams);

	void BeginTimingFrame();
	void EndTimingFrame();

protected:

	bool					m_TilingEnable;
	bool					m_TilingActive;
	
	U32						m_BeginVerticesUsage;
	U32						m_MaxBeginVerticesUsage;
	U32						m_FreeBeginVerticesUsage;

	U32						m_UserBeginVerticesUsage;
	U32						m_UserMaxBeginVerticesUsage;

	bool					m_BeginVerticesLocked;

	#ifndef CFG_GOLD
	U8*						m_BeginVerticesStart;
	U32						m_BeginVerticesSize;
	#endif
	
	S32						m_MemoryID;
	U8*						m_MemoryPtr;
	U32						m_MemorySize;
	U32						m_MemoryFlags;
	U32						m_WindowOffset;

	XVIDEO_MODE				m_VideoMode;

	C_MemoryBuffer			m_LargeHeap; // Allocs greater than 8K
	C_MemoryBuffer			m_DynamicHeap; // Dynamic allocs
	C_MemoryBuffer			m_SmallHeap; // Allocs less than or equal to 8K
	
	Time::C_Time 			m_CpuTimeBegin;
	Time::C_Time 			m_CpuTimeEnd;
	Time::C_Time 			m_GpuTimeBegin[2];
	Time::C_Time 			m_GpuTimeEnd[2];
	U32						m_GpuTimeIndex;
};

}; // namespace D3D9
}; // namespace Display

#ifdef CFG_DEBUG
#define DO_NOT_INLINE_XB2_D3D9_DEVICE_INL
#endif

#ifndef DO_NOT_INLINE_XB2_D3D9_DEVICE_INL
#include "Display/Device/D3D9/XB2D3D9Device.inl"
#endif

#endif // DISPLAY_DEVICE_D3D9_XB2_D3D9_DEVICE_H
