#include "StdAfx.h"
#include "Display2/Device/XB2Device.h"

namespace Display2
{

// C_DeviceParams

C_DeviceParams::C_DeviceParams()
{
	m_LargeHeapSize				= 0;
	m_DynamicHeapSize			= 0;
	m_SmallHeapSize				= 0;
}

C_DeviceParams::~C_DeviceParams()
{
}

// C_Device

C_Device::C_Device()
{
	m_ThreadID					= UINT_MAX;

	m_LargeHeapSize				= 0;
	m_DynamicHeapSize			= 0;
	m_SmallHeapSize				= 0;

	m_SmallHeapThreshold		= 1024*8;

	m_Context.m_pState			= &m_ContextState;
	m_ContextNoCheck.m_pState	= &m_ContextState;
}

C_Device::~C_Device()
{
}

void C_Device::GetParams(C_DeviceParams& params)
{
	params.m_LargeHeapSize		= m_LargeHeapSize;
	params.m_DynamicHeapSize	= m_DynamicHeapSize;
	params.m_SmallHeapSize		= m_SmallHeapSize;
	params.m_pd3dDevice			= m_pd3dDevice;
	params.m_pd3dObject			= m_pd3dObject;
}

void C_Device::SetParams(const C_DeviceParams& params)
{
	m_LargeHeapSize				= params.m_LargeHeapSize;
	m_DynamicHeapSize			= params.m_DynamicHeapSize;
	m_SmallHeapSize				= params.m_SmallHeapSize;
	m_pd3dDevice				= params.m_pd3dDevice;
	m_pd3dObject				= params.m_pd3dObject;
}

bool C_Device::Create()
{
	BIZ_ASSERT(m_Created == 0);

	m_ThreadID = GetCurrentThreadId();

	m_Context.m_pd3dDevice = m_pd3dDevice;
	m_ContextNoCheck.m_pd3dDevice = m_pd3dDevice;
	m_ContextNoCheckNoTrack.m_pd3dDevice = m_pd3dDevice;

	if(!CreateMemory())
		return false;

	m_Created = 1;

	return true;
}

bool C_Device::CreateVolatile()
{
	BIZ_ASSERT(m_Created == 1);

	m_Created = 2;

	return true;
}

void C_Device::DestroyVolatile()
{
	BIZ_ASSERT(m_Created == 2);

	m_Created = 1;
}

void C_Device::Destroy()
{
	BIZ_ASSERT(m_Created == 1);

	DestroyVertexDeclarations();

	DestroyMemory();

	m_ThreadID = UINT_MAX;

	m_Created = 0;
}

bool C_Device::CreateMemory()
{
	if(m_LargeHeapSize > 0)
	{
		MEMORY_TRACKER_PUSH("graphics.display.device.largeheap");
		bool success = m_LargeHeap.Create(m_LargeHeapSize, D3DTEXTURE_ALIGNMENT, PAGE_READWRITE, E_MemoryBuffer_Physical, "Large");
		MEMORY_TRACKER_POP("graphics.display.device.largeheap");
		if(!success)
		{
			LOGFATAL(g_Log, "Failed to allocate large graphics memory heap of size %i bytes\n", m_LargeHeapSize);
			return false;
		}
	}

	if(m_DynamicHeapSize > 0)
	{
		MEMORY_TRACKER_PUSH("graphics.display.device.dynamicheap");
		bool success = m_DynamicHeap.Create(m_DynamicHeapSize, D3DSHADER_ALIGNMENT, PAGE_READWRITE | PAGE_WRITECOMBINE, E_MemoryBuffer_Physical, "Dynamic");
		MEMORY_TRACKER_POP("graphics.display.device.dynamicheap");
		if(!success)
		{
			LOGFATAL(g_Log, "Failed to allocate dynamic graphics memory heap of size %i bytes\n", m_DynamicHeapSize);
			return false;
		}
	}

	if(m_SmallHeapSize > 0)
	{
		MEMORY_TRACKER_PUSH("graphics.display.device.smallheap");
		bool success = m_SmallHeap.Create(m_SmallHeapSize, D3DSHADER_ALIGNMENT, PAGE_READWRITE, E_MemoryBuffer_Physical, "Small");
		MEMORY_TRACKER_POP("graphics.display.device.smallheap");
		if(!success)
		{
			LOGFATAL(g_Log, "Failed to allocate small graphics memory heap of size %i bytes\n", m_SmallHeapSize);
			return false;
		}
	}
	
	return true;
}

void C_Device::DestroyMemory()
{
	m_LargeHeap.Destroy();

	m_DynamicHeap.Destroy();

	m_SmallHeap.Destroy();
}

bool C_Device::CreateVertexBuffer(U32 size, U32 flags, C_VertexBuffer* pVertexBuffer, void* pMemory)
{
	BIZ_ASSERT(pVertexBuffer);
	BIZ_ASSERT(!pVertexBuffer->IsCreated());
	BIZ_ASSERT(size > 0);

	DWORD d3dUsage = 0;
	if(!(flags & E_Flags_Dynamic))
		d3dUsage = D3DUSAGE_CPU_CACHED_MEMORY;

	#ifndef CFG_SPEED
	d3dUsage |= D3DUSAGE_RUNCOMMANDBUFFER_TIMESTAMP;
	#endif

	XGSetVertexBufferHeader(size, d3dUsage, 0, 0, &pVertexBuffer->m_d3dVertexBuffer);

	if(!pMemory)
	{
		if(!AllocResourceMemory(E_ResourceType_VertexBuffer, flags, size, (U8**)&pMemory))
		{
			pVertexBuffer->m_d3dVertexBuffer.Format.BaseAddress = 0;

			m_LastError = GPU_ERROR_OUT_OF_MEMORY;

			if(!m_AllowOutOfMemory)
			{
				DumpMemoryHeaps();

				LOGFATAL(g_Log, "Out of Memory! Failed to create vertex buffer of size %i flags %i\n", size, flags);
			}
			
			return false;
		}
		
		flags &= ~E_Flags_Indirect;
	}
	else
	{
		flags |= E_Flags_Indirect;
	}

	XGOffsetResourceAddress(&pVertexBuffer->m_d3dVertexBuffer, pMemory);

	pVertexBuffer->SetFlags(flags);

	#if TRACK_DISPLAY2_RESOURCES
	if(!(flags & E_Flags_DoNotTrack))
		InsertVertexBuffer(pVertexBuffer);
	#endif
	
	return true;
}

void C_Device::DestroyVertexBuffer(C_VertexBuffer* pVertexBuffer)
{
	BIZ_ASSERT(pVertexBuffer);

	if(pVertexBuffer->IsCreated())
	{
		BIZ_ASSERT(pVertexBuffer->m_d3dVertexBuffer.IsSet(m_pd3dDevice) == FALSE);
		BIZ_ASSERT(pVertexBuffer->m_d3dVertexBuffer.IsBusy() == FALSE);
		BIZ_ASSERT(pVertexBuffer->m_d3dVertexBuffer.ReferenceCount == 1);

#ifdef CFG_DEBUG
		DWORD ReferenceCount = ((pVertexBuffer->m_d3dVertexBuffer.Common & D3DCOMMON_DEBUG_INTREFCOUNT_MASK) >> D3DCOMMON_DEBUG_INTREFCOUNT_SHIFT);
		BIZ_ASSERT_MSG(ReferenceCount == 0, "ReferenceCount(%d)", ReferenceCount);
#endif

		#if TRACK_DISPLAY2_RESOURCES
		if(!(pVertexBuffer->GetFlags() & E_Flags_DoNotTrack))
			RemoveVertexBuffer(pVertexBuffer);
		#endif
		
		if(!(pVertexBuffer->GetFlags() & E_Flags_Indirect))
			FreeMemory(pVertexBuffer->GetSize(), (U8*)pVertexBuffer->GetAddress(), E_MemoryType_VertexBuffer, pVertexBuffer->GetFlags());
		
		pVertexBuffer->SetSize(0);
		pVertexBuffer->SetFlags(0);
		pVertexBuffer->SetAddress(NULL);
	}
}

bool C_Device::CreateIndexBuffer(U32 size, U32 flags, E_IndexFormat format, C_IndexBuffer* pIndexBuffer, void* pMemory)
{
	BIZ_ASSERT(pIndexBuffer);
	BIZ_ASSERT(!pIndexBuffer->IsCreated());
	BIZ_ASSERT(size > 0);

	DWORD d3dUsage = 0;
	if(!(flags & E_Flags_Dynamic))
		d3dUsage = D3DUSAGE_CPU_CACHED_MEMORY;

	#ifndef CFG_SPEED
	d3dUsage |= D3DUSAGE_RUNCOMMANDBUFFER_TIMESTAMP;
	#endif

	XGSetIndexBufferHeader(size, d3dUsage, (D3DFORMAT)format, 0, 0, &pIndexBuffer->m_d3dIndexBuffer);

	if(!pMemory)
	{
		if(!AllocResourceMemory(E_ResourceType_IndexBuffer, flags, size, (U8**)&pMemory))
		{
			pIndexBuffer->m_d3dIndexBuffer.Address = 0;

			m_LastError = GPU_ERROR_OUT_OF_MEMORY;

			if(!m_AllowOutOfMemory)
			{
				DumpMemoryHeaps();

				LOGFATAL(g_Log, "Out of Memory! Failed to create index buffer of size %i flags %i format %i\n", size, flags, format);
			}
			
			return false;
		}
		
		flags &= ~E_Flags_Indirect;
	}
	else
	{
		flags |= E_Flags_Indirect;
	}

	XGOffsetResourceAddress(&pIndexBuffer->m_d3dIndexBuffer, pMemory);

	pIndexBuffer->SetFlags(flags);

	#if TRACK_DISPLAY2_RESOURCES
	if(!(flags & E_Flags_DoNotTrack))
		InsertIndexBuffer(pIndexBuffer);
	#endif
	
	return true;
}

void C_Device::DestroyIndexBuffer(C_IndexBuffer* pIndexBuffer)
{
	BIZ_ASSERT(pIndexBuffer);

	if(pIndexBuffer->IsCreated())
	{
		BIZ_ASSERT(pIndexBuffer->m_d3dIndexBuffer.IsSet(m_pd3dDevice) == FALSE);
		BIZ_ASSERT(pIndexBuffer->m_d3dIndexBuffer.IsBusy() == FALSE);
		BIZ_ASSERT(pIndexBuffer->m_d3dIndexBuffer.ReferenceCount == 1);

#ifdef CFG_DEBUG
		DWORD ReferenceCount = ((pIndexBuffer->m_d3dIndexBuffer.Common & D3DCOMMON_DEBUG_INTREFCOUNT_MASK) >> D3DCOMMON_DEBUG_INTREFCOUNT_SHIFT);
		BIZ_ASSERT_MSG(ReferenceCount == 0, "ReferenceCount(%d)", ReferenceCount);
#endif

		#if TRACK_DISPLAY2_RESOURCES
		if(!(pIndexBuffer->GetFlags() & E_Flags_DoNotTrack))
			RemoveIndexBuffer(pIndexBuffer);
		#endif
		
		if(!(pIndexBuffer->GetFlags() & E_Flags_Indirect))
			FreeMemory(pIndexBuffer->GetSize(), (U8*)pIndexBuffer->GetAddress(), E_MemoryType_IndexBuffer, pIndexBuffer->GetFlags());
		
		pIndexBuffer->SetSize(0);
		pIndexBuffer->SetFlags(0);
		pIndexBuffer->SetAddress(NULL);
	}
}

bool C_Device::CreateVertexShader(U32 size, const U8* pData, U32 flags, C_VertexShader* pVertexShader, void* pMemory)
{
	BIZ_ASSERT(pVertexShader);
	BIZ_ASSERT(!pVertexShader->IsCreated());
	BIZ_ASSERT(size > 0);

	XGMICROCODESHADERPARTS parts;
	XGGetMicrocodeShaderParts(pData, &parts);

	pVertexShader->m_pd3dVertexShader = (IDirect3DVertexShader9*)new char[parts.cbCachedPartSize];

	XGSetVertexShaderHeader(pVertexShader->m_pd3dVertexShader, parts.cbCachedPartSize, &parts);

	if(!pMemory)
	{
		if(!AllocResourceMemory(E_ResourceType_VertexShader, flags, size, (U8**)&pMemory))
		{
			SAFE_DELETE_ARY(pVertexShader->m_pd3dVertexShader);

			m_LastError = GPU_ERROR_OUT_OF_MEMORY;

			if(!m_AllowOutOfMemory)
			{
				DumpMemoryHeaps();

				LOGFATAL(g_Log, "Out of Memory! Failed to create vertex shader of size %i flags %i\n", size, flags);
			}
			
			return false;
		}
		
		flags &= ~E_Flags_Indirect;
	}
	else
	{
		flags |= E_Flags_Indirect;
	}

	memcpy(pMemory, parts.pPhysicalPart, parts.cbPhysicalPartSize);

	XGRegisterVertexShader(pVertexShader->m_pd3dVertexShader, pMemory);

	#if !CFG_SPEED
	pVertexShader->m_pd3dVertexShader->Common |= 0x00400000;
	#endif

	// Verify that the address we get from D3D matches the address we allocated
	void *m_pAddress = pVertexShader->GetAddress();
	BIZ_ASSERT(m_pAddress == pMemory);

	pVertexShader->SetFlags(flags);

	#if TRACK_DISPLAY2_RESOURCES
	if(!(flags & E_Flags_DoNotTrack))
		InsertVertexShader(pVertexShader);
	#endif

	return true;
}

void C_Device::DestroyVertexShader(C_VertexShader* pVertexShader)
{
	BIZ_ASSERT(pVertexShader);

	if(pVertexShader->IsCreated())
	{
		BIZ_ASSERT(pVertexShader->m_pd3dVertexShader->IsSet(m_pd3dDevice) == FALSE);
		BIZ_ASSERT(pVertexShader->m_pd3dVertexShader->IsBusy() == FALSE);
		BIZ_ASSERT(pVertexShader->m_pd3dVertexShader->ReferenceCount == 1);

#ifdef CFG_DEBUG
		DWORD ReferenceCount = ((pVertexShader->m_pd3dVertexShader->Common & D3DCOMMON_DEBUG_INTREFCOUNT_MASK) >> D3DCOMMON_DEBUG_INTREFCOUNT_SHIFT);
		BIZ_ASSERT_MSG(ReferenceCount == 0, "ReferenceCount(%d)", ReferenceCount);
#endif

		#if TRACK_DISPLAY2_RESOURCES
		if(!(pVertexShader->GetFlags() & E_Flags_DoNotTrack))
			RemoveVertexShader(pVertexShader);
		#endif

		if(!(pVertexShader->GetFlags() & E_Flags_Indirect))
			FreeMemory(pVertexShader->GetSize(), (U8*)pVertexShader->GetAddress(), E_MemoryType_VertexShader, pVertexShader->GetFlags());

		pVertexShader->SetFlags(0);

		SAFE_DELETE_ARY(pVertexShader->m_pd3dVertexShader);
	}
}

bool C_Device::CreatePixelShader(U32 size, const U8* pData, U32 flags, C_PixelShader* pPixelShader, void* pMemory)
{
	BIZ_ASSERT(pPixelShader);
	BIZ_ASSERT(!pPixelShader->IsCreated());
	BIZ_ASSERT(size > 0);

	XGMICROCODESHADERPARTS parts;
	XGGetMicrocodeShaderParts(pData, &parts);

	pPixelShader->m_pd3dPixelShader = (IDirect3DPixelShader9*)new char[parts.cbCachedPartSize];

	XGSetPixelShaderHeader(pPixelShader->m_pd3dPixelShader, parts.cbCachedPartSize, &parts);

	if(!pMemory)
	{
		if(!AllocResourceMemory(E_ResourceType_PixelShader, flags, size, (U8**)&pMemory))
		{
			SAFE_DELETE_ARY(pPixelShader->m_pd3dPixelShader);

			m_LastError = GPU_ERROR_OUT_OF_MEMORY;

			if(!m_AllowOutOfMemory)
			{
				DumpMemoryHeaps();

				LOGFATAL(g_Log, "Out of Memory! Failed to create pixel shader of size %i flags %i\n", size, flags);
			}
			
			return false;
		}
		
		flags &= ~E_Flags_Indirect;
	}
	else
	{
		flags |= E_Flags_Indirect;
	}
	
	memcpy(pMemory, parts.pPhysicalPart, parts.cbPhysicalPartSize);

	XGRegisterPixelShader(pPixelShader->m_pd3dPixelShader, pMemory);

	#if !CFG_SPEED
	pPixelShader->m_pd3dPixelShader->Common |= 0x00400000;
	#endif

	pPixelShader->m_pAddress = pMemory; // TODO: Can we get this from the header?		

	pPixelShader->SetFlags(flags);

	#if TRACK_DISPLAY2_RESOURCES
	if(!(flags & E_Flags_DoNotTrack))
		InsertPixelShader(pPixelShader);
	#endif

	return true;
}

void C_Device::DestroyPixelShader(C_PixelShader* pPixelShader)
{
	BIZ_ASSERT(pPixelShader);

	if(pPixelShader->IsCreated())
	{
		BIZ_ASSERT(pPixelShader->m_pd3dPixelShader->IsSet(m_pd3dDevice) == FALSE);
		BIZ_ASSERT(pPixelShader->m_pd3dPixelShader->IsBusy() == FALSE);
		BIZ_ASSERT(pPixelShader->m_pd3dPixelShader->ReferenceCount == 1);

#ifdef CFG_DEBUG
		DWORD ReferenceCount = ((pPixelShader->m_pd3dPixelShader->Common & D3DCOMMON_DEBUG_INTREFCOUNT_MASK) >> D3DCOMMON_DEBUG_INTREFCOUNT_SHIFT);
		BIZ_ASSERT_MSG(ReferenceCount == 0, "ReferenceCount(%d)", ReferenceCount);
#endif

		#if TRACK_DISPLAY2_RESOURCES
		if(!(pPixelShader->GetFlags() & E_Flags_DoNotTrack))
			RemovePixelShader(pPixelShader);
		#endif

		if(!(pPixelShader->GetFlags() & E_Flags_Indirect))
			FreeMemory(pPixelShader->GetSize(), (U8*)pPixelShader->GetAddress(), E_MemoryType_PixelShader, pPixelShader->GetFlags());

		pPixelShader->SetFlags(0);
		pPixelShader->SetAddress(NULL);

		SAFE_DELETE_ARY(pPixelShader->m_pd3dPixelShader);
	}
}

bool C_Device::CreateColourSurface(U32 width, U32 height, U32 flags, E_SurfaceFormat format, E_Multisample multisample, const C_SurfaceParams* pParams, C_Surface* pSurface, void* pMemory, const U32 memorySize)
{
	return CreateSurface(width, height, flags, format, multisample, pParams, pSurface, pMemory, memorySize);
}

bool C_Device::CreateDepthStencilSurface(U32 width, U32 height, U32 flags, E_SurfaceFormat format, E_Multisample multisample, const C_SurfaceParams* pParams, C_Surface* pSurface, void* pMemory, const U32 memorySize)
{
	return CreateSurface(width, height, flags, format, multisample, pParams, pSurface, pMemory, memorySize);
}

bool C_Device::CreateSurface(const U32 width, const U32 height, const U32 flags, const E_SurfaceFormat format, const E_Multisample multisample, const C_SurfaceParams* pParams, C_Surface* pSurface, void* pMemory, const U32 memorySize)
{
	BIZ_ASSERT(pSurface);
	BIZ_ASSERT(!pSurface->IsCreated());
	BIZ_ASSERT(width > 0 && height > 0);

	if(pParams)
	{
		XGSetSurfaceHeader(width, height, (D3DFORMAT)format, (D3DMULTISAMPLE_TYPE)multisample, pParams, &pSurface->m_d3dSurface, NULL);
	}
	else
	{
		C_SurfaceParams params;
		XGSetSurfaceHeader(width, height, (D3DFORMAT)format, (D3DMULTISAMPLE_TYPE)multisample, &params, &pSurface->m_d3dSurface, NULL);
	}

	XGOffsetSurfaceAddress(&pSurface->m_d3dSurface, 0, 0);

	pSurface->SetFlags(flags);

	#if TRACK_DISPLAY2_RESOURCES
	if(!(flags & E_Flags_DoNotTrack))
		InsertSurface(pSurface);
	#endif

	return true;
}
	
void C_Device::DestroySurface(C_Surface* pSurface)
{
	BIZ_ASSERT(pSurface);

	if(pSurface->IsCreated())
	{
		#if TRACK_DISPLAY2_RESOURCES
		if(!(pSurface->GetFlags() & E_Flags_DoNotTrack))
			RemoveSurface(pSurface);
		#endif

		pSurface->SetSize(0);
		pSurface->SetFlags(0);
	}
}

bool C_Device::CreateTexture2D(U32 width, U32 height, U32 levels, U32 flags, E_TextureFormat format, C_Texture* pTexture, void* pMemory, const U32 memorySize)
{
	BIZ_ASSERT(pTexture);
	BIZ_ASSERT(!pTexture->IsCreated());
	BIZ_ASSERT(width > 0 && height > 0 && levels > 0);

	const U32 size = SetTexture2DHeader(width, height, levels, flags, format, 0, &pTexture->m_d3dTexture2D);

	if(!pMemory)
	{
		if(!AllocResourceMemory(E_ResourceType_Texture2D, flags, size, (U8**)&pMemory))
		{
			pTexture->m_d3dTexture.Format.BaseAddress = 0;

			m_LastError = GPU_ERROR_OUT_OF_MEMORY;

			if(!m_AllowOutOfMemory)
			{
				DumpMemoryHeaps();

				LOGFATAL(g_Log, "Out of Memory! Failed to create 2d texture of size %i width %i height %i levels %i flags %i format %i\n", size, width, height, levels, flags, format);
			}
			
			return false;
		}
		
		flags &= ~E_Flags_Indirect;
	}
	else
	{
		BIZ_ASSERT(size <= memorySize); // I guess its ok to use less?

		flags |= E_Flags_Indirect;
	}

	XGOffsetResourceAddress(&pTexture->m_d3dTexture, pMemory);

	pTexture->SetSize(size);
	pTexture->SetFlags(flags);
	pTexture->SetSRGB(flags & E_Flags_SRGB ? true : false);

	#if TRACK_DISPLAY2_RESOURCES
	if(!(flags & E_Flags_DoNotTrack))
		InsertTexture(pTexture);
	#endif
	
	return true;
}

bool C_Device::CreateTexture3D(const U32 width, const U32 height, const U32 depth, const U32 levels, U32 flags, const E_TextureFormat format, C_Texture* pTexture, void* pMemory, const U32 memorySize)
{
	BIZ_ASSERT(pTexture);
	BIZ_ASSERT(!pTexture->IsCreated());
	BIZ_ASSERT(width > 0 && height > 0 && depth > 0 && levels > 0);

	const U32 size = SetTexture3DHeader(width, height, depth, levels, flags, format, 0, &pTexture->m_d3dTexture3D);

	if(!pMemory)
	{
		if(!AllocResourceMemory(E_ResourceType_Texture3D, flags, size, (U8**)&pMemory))
		{
			pTexture->m_d3dTexture.Format.BaseAddress = 0;

			m_LastError = GPU_ERROR_OUT_OF_MEMORY;

			if(!m_AllowOutOfMemory)
			{
				DumpMemoryHeaps();

				LOGFATAL(g_Log, "Out of Memory! Failed to create 3d texture of size %i width %i height %i depth %i levels %i flags %i format %i\n", size, width, height, depth, levels, flags, format);
			}
			
			return false;
		}
		
		flags &= ~E_Flags_Indirect;
	}
	else
	{
		BIZ_ASSERT(size <= memorySize); // I guess its ok to use less?

		flags |= E_Flags_Indirect;
	}

	XGOffsetResourceAddress(&pTexture->m_d3dTexture, pMemory);

	pTexture->SetSize(size);
	pTexture->SetFlags(flags);
	pTexture->SetSRGB(flags & E_Flags_SRGB ? true : false);

	#if TRACK_DISPLAY2_RESOURCES
	if(!(flags & E_Flags_DoNotTrack))
		InsertTexture(pTexture);
	#endif

	return true;
}

bool C_Device::CreateTextureCube(const U32 width, const U32 levels, U32 flags, const E_TextureFormat format, C_Texture* pTexture, void* pMemory, const U32 memorySize)
{
	BIZ_ASSERT(pTexture);
	BIZ_ASSERT(!pTexture->IsCreated());
	BIZ_ASSERT(width > 0 && levels > 0);

	const U32 size = SetTextureCubeHeader(width, levels, flags, format, 0, &pTexture->m_d3dTextureCube);

	if(!pMemory)
	{
		if(!AllocResourceMemory(E_ResourceType_TextureCube, flags, size, (U8**)&pMemory))
		{
			pTexture->m_d3dTexture.Format.BaseAddress = 0;

			m_LastError = GPU_ERROR_OUT_OF_MEMORY;

			if(!m_AllowOutOfMemory)
			{
				DumpMemoryHeaps();

				LOGFATAL(g_Log, "Out of Memory! Failed to create cube texture of size %i width %i levels %i flags %i format %i\n", size, width, levels, flags, format);
			}
			
			return false;
		}
		
		flags &= ~E_Flags_Indirect;
	}
	else
	{
		BIZ_ASSERT(size <= memorySize); // I guess its ok to use less?

		flags |= E_Flags_Indirect;
	}

	XGOffsetResourceAddress(&pTexture->m_d3dTexture, pMemory);

	pTexture->SetSize(size);
	pTexture->SetFlags(flags);
	pTexture->SetSRGB(flags & E_Flags_SRGB ? true : false);

	#if TRACK_DISPLAY2_RESOURCES
	if(!(flags & E_Flags_DoNotTrack))
		InsertTexture(pTexture);
	#endif

	return true;
}

void C_Device::DestroyTexture(C_Texture* pTexture)
{
	BIZ_ASSERT(pTexture);

	if(pTexture->IsCreated())
	{
		BIZ_ASSERT(pTexture->m_d3dTexture.IsSet(m_pd3dDevice) == FALSE);
		BIZ_ASSERT(pTexture->m_d3dTexture.IsBusy() == FALSE);
		BIZ_ASSERT(pTexture->m_d3dTexture.ReferenceCount == 1);

#ifdef CFG_DEBUG
		DWORD ReferenceCount = ((pTexture->m_d3dTexture.Common & D3DCOMMON_DEBUG_INTREFCOUNT_MASK) >> D3DCOMMON_DEBUG_INTREFCOUNT_SHIFT);
		BIZ_ASSERT_MSG(ReferenceCount == 0, "ReferenceCount(%d)", ReferenceCount);
#endif

		#if TRACK_DISPLAY2_RESOURCES
		if(!(pTexture->GetFlags() & E_Flags_DoNotTrack))
			RemoveTexture(pTexture);
		#endif

		if(!(pTexture->GetFlags() & E_Flags_Indirect))
			FreeMemory(pTexture->GetSize(), (U8*)pTexture->GetAddress(), E_MemoryType_Texture, pTexture->GetFlags());

		pTexture->SetSize(0);
		pTexture->SetFlags(0);
		pTexture->SetAddress(NULL);
	}
}

bool C_Device::AllocResourceMemory(const E_ResourceType eResourceType, U32& flags, const U32 size, U8** ppAddress)
{
	U32 align;
	E_MemoryType type;

	switch(eResourceType)
	{
	case E_ResourceType_Texture2D:
		align = ALIGNMENT_TEXTURE2D;
		type = E_MemoryType_Texture;
		break;
	case E_ResourceType_Texture3D:
		align = ALIGNMENT_TEXTURE3D;
		type = E_MemoryType_Texture;
		break;
	case E_ResourceType_TextureCube:
		align = ALIGNMENT_TEXTURECUBE;
		type = E_MemoryType_Texture;
		break;
	case E_ResourceType_VertexBuffer:
		align = ALIGNMENT_VERTEXBUFFER;
		type = E_MemoryType_VertexBuffer;
		break;
	case E_ResourceType_IndexBuffer:
		align = ALIGNMENT_INDEXBUFFER;
		type = E_MemoryType_IndexBuffer;
		break;
	case E_ResourceType_VertexShader:
		align = ALIGNMENT_VERTEXSHADER;
		flags |= E_Flags_Dynamic; // Force shaders out of dynamic heap
		type = E_MemoryType_VertexShader;
		break;
	case E_ResourceType_PixelShader:
		align = ALIGNMENT_PIXELSHADER;
		flags |= E_Flags_Dynamic; // Force shaders out of dynamic heap
		type = E_MemoryType_PixelShader;
		break;
	default:
		LOGERROR(g_Log, "Unsupported resource type %i when allocating memory size %i\n", eResourceType, size);
		return false;
	}

	return AllocMemory(size, align, flags, ppAddress, type);
}

bool C_Device::AllocMemory(const U32 size, const U32 align, const U32 flags, U8** ppAddress, const E_MemoryType type)
{
	BIZ_ASSERT(align > 0);
	BIZ_ASSERT(size > 0);
	BIZ_ASSERT(ppAddress);

	*ppAddress = NULL;
	
	if(flags & E_Flags_Dynamic)
	{
		return m_DynamicHeap.Alloc(align, size, ppAddress, type, flags); // Alloc out of dynamic heap
	}
	else
	{
		if(size <= m_SmallHeapThreshold)
		{
			if(m_SmallHeap.Alloc(align, size, ppAddress, type, flags)) // Alloc out of small heap
			{
				return true;
			}
			else
			{
				return m_LargeHeap.Alloc(align, size, ppAddress, type, flags); // Alloc out of small heap failed so try large
			}
		}
		else
		{
			return m_LargeHeap.Alloc(align, size, ppAddress, type, flags); // Alloc out of large heap
		}
	}
}

void C_Device::FreeMemory(const U32 size, U8* pAddress, const E_MemoryType type, const U32 flags)
{
	if(!pAddress)
		return;

	BIZ_ASSERT(size > 0);

	if(m_LargeHeap.Inside(size, pAddress))
	{
		m_LargeHeap.Free(size, pAddress, type, flags);
	}
	else
	if(m_SmallHeap.Inside(size, pAddress))
	{
		m_SmallHeap.Free(size, pAddress, type, flags);
	}
	else
	if(m_DynamicHeap.Inside(size, pAddress))
	{
		m_DynamicHeap.Free(size, pAddress, type, flags);
	}
	else
	{
		LOGERROR(g_Log, "Freeing memory size %i and address %i not inside any large, small or dyanmic heap\n", size, pAddress);
	}
}

bool C_Device::CheckMemoryGuards() const
{
	if(!m_LargeHeap.CheckMemoryGuards())
		return false;
	if(!m_SmallHeap.CheckMemoryGuards())
		return false;
	if(!m_DynamicHeap.CheckMemoryGuards())
		return false;
	return true;
}

void C_Device::SetSmallHeapThreshold(const U32 smallHeapThreshold)
{
	m_SmallHeapThreshold = smallHeapThreshold;
}

void C_Device::BlockUntilIdle()
{
	assert(m_pd3dDevice);
	m_pd3dDevice->BlockUntilIdle();
}

void C_Device::DumpMemoryHeaps()
{
#if !CFG_GOLD
	m_LargeHeap.Dump("LargeHeap");
	m_DynamicHeap.Dump("DynamicHeap");
	m_SmallHeap.Dump("SmallHeap");
#endif
}

void C_Device::ShowMemoryHeaps()
{
#if !CFG_GOLD
	LOGINFO(g_Log, "Heaps : Large (%0.2fmb / %0.2fmb) Dynamic (%0.2fmb / %0.2fmb) Small (%0.2fmb / %0.2fmb)",
		(F32)(m_LargeHeap.GetUsed())	/ (1024.0f * 1024.0f),
		(F32)(m_LargeHeap.GetSize())	/ (1024.0f * 1024.0f),
		(F32)(m_DynamicHeap.GetUsed())	/ (1024.0f * 1024.0f),
		(F32)(m_DynamicHeap.GetSize())	/ (1024.0f * 1024.0f),
		(F32)(m_SmallHeap.GetUsed())	/ (1024.0f * 1024.0f),
		(F32)(m_SmallHeap.GetSize())	/ (1024.0f * 1024.0f));
#endif
}

// C_DeviceManager

C_DeviceManager::C_DeviceManager()
{
}

C_DeviceManager::~C_DeviceManager()
{
}

void C_DeviceManager::GetParams(C_DeviceManagerParams& params)
{
}

void C_DeviceManager::SetParams(const C_DeviceManagerParams& params)
{
}

bool C_DeviceManager::Create(const C_DeviceManagerParams& params)
{
	if(!CreateObject())
		return false;

	return true;
}

void C_DeviceManager::Destroy()
{
	DestroyObject();
}

void C_DeviceManager::SetupDefaultDeviceParams(C_DeviceParams& params)
{
	params.m_pd3dObject				= m_pd3dObject;
}

} // namespace Display2
