#include "StdAfx.h"
#include "Display2/Device/PCD3D9Device.h"

namespace Display2
{

// C_DeviceParams

C_DeviceParams::C_DeviceParams()
{
}

C_DeviceParams::~C_DeviceParams()
{
}

// C_Device

C_Device::C_Device()
{
	m_Context.m_pState			= &m_ContextState;
	m_ContextNoCheck.m_pState	= &m_ContextState;
}

C_Device::~C_Device()
{
}

void C_Device::GetParams(C_DeviceParams& params)
{
	params.m_pd3dDevice			= m_pd3dDevice;
	params.m_pd3dObject			= m_pd3dObject;
}

void C_Device::SetParams(const C_DeviceParams& params)
{
	m_pd3dDevice				= params.m_pd3dDevice;
	m_pd3dObject				= params.m_pd3dObject;
}

bool C_Device::Create()
{
	BIZ_ASSERT(m_Created == 0);

	m_Context.m_pd3dDevice = m_pd3dDevice;
	m_ContextNoCheck.m_pd3dDevice = m_pd3dDevice;
	m_ContextNoCheckNoTrack.m_pd3dDevice = m_pd3dDevice;

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
	//BIZ_ASSERT(m_Created == 2);

	m_Created = 1;
}

void C_Device::Destroy()
{
	//BIZ_ASSERT(m_Created == 1);

	DestroyVertexDeclarations();

	m_Created = 0;
}

bool C_Device::CreateVertexBuffer(U32 size, U32 flags, C_VertexBuffer* pVertexBuffer, void* pMemory)
{
	BIZ_ASSERT(pVertexBuffer);
	BIZ_ASSERT(!pVertexBuffer->IsCreated());
	
	const DWORD d3dUsage = D3DUsage(flags);
	const D3DPOOL d3dPool = D3DPool(flags);

	BIZ_ASSERT(m_pd3dDevice);
	HRESULT hr = m_pd3dDevice->CreateVertexBuffer(size, d3dUsage, 0, d3dPool, &pVertexBuffer->m_pd3dVertexBuffer, NULL);
	if(FAILED(hr))
	{
		LOGERROR(g_Log, "Failed to create pcd3d9 vertex buffer of size %i flags %i\n", size, flags);
		return false;
	}

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
		#if TRACK_DISPLAY2_RESOURCES
		if(!(pVertexBuffer->GetFlags() & E_Flags_DoNotTrack))
			RemoveVertexBuffer(pVertexBuffer);
		#endif
		
		const ULONG refCount = pVertexBuffer->m_pd3dVertexBuffer->Release();
		BIZ_ASSERT(refCount == 0);
		pVertexBuffer->m_pd3dVertexBuffer = NULL;
	}
}

bool C_Device::CreateIndexBuffer(U32 size, U32 flags, E_IndexFormat format, C_IndexBuffer* pIndexBuffer, void* pMemory)
{
	BIZ_ASSERT(pIndexBuffer);
	BIZ_ASSERT(!pIndexBuffer->IsCreated());
	
	const DWORD d3dUsage = D3DUsage(flags);
	const D3DFORMAT d3dFormat = (D3DFORMAT)format;
	const D3DPOOL d3dPool = D3DPool(flags);

	BIZ_ASSERT(m_pd3dDevice);
	HRESULT hr = m_pd3dDevice->CreateIndexBuffer(size, d3dUsage, d3dFormat, d3dPool, &pIndexBuffer->m_pd3dIndexBuffer, NULL);
	if(FAILED(hr))
	{
		LOGERROR(g_Log, "Failed to create pcd3d9 index buffer of size %i flags %i format %i hr 0x%08x usage 0x%x pool %i\n", size, flags, format, hr, d3dUsage, d3dPool);	// [Blur2PC] hr
		return false;
	}

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
		#if TRACK_DISPLAY2_RESOURCES
		if(!(pIndexBuffer->GetFlags() & E_Flags_DoNotTrack))
			RemoveIndexBuffer(pIndexBuffer);
		#endif
		
		const ULONG refCount = pIndexBuffer->m_pd3dIndexBuffer->Release();
		BIZ_ASSERT(refCount == 0);
		pIndexBuffer->m_pd3dIndexBuffer = NULL;
	}
}

bool C_Device::CreateVertexShader(U32 size, const U8* pData, U32 flags, C_VertexShader* pVertexShader, void* pMemory)
{
	BIZ_ASSERT(pVertexShader);
	BIZ_ASSERT(!pVertexShader->IsCreated());
	
	BIZ_ASSERT(m_pd3dDevice);
	HRESULT hr = m_pd3dDevice->CreateVertexShader((DWORD*)pData, &pVertexShader->m_pd3dVertexShader);
	if(FAILED(hr))
	{
		LOGERROR(g_Log, "Failed to create pcd3d9 vertex shader of size %i flags %i\n", size, flags);
		return false;
	}

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
		#if TRACK_DISPLAY2_RESOURCES
		if(!(pVertexShader->GetFlags() & E_Flags_DoNotTrack))
			RemoveVertexShader(pVertexShader);
		#endif

		const ULONG refCount = pVertexShader->m_pd3dVertexShader->Release();
		BIZ_ASSERT(refCount == 0);
		pVertexShader->m_pd3dVertexShader = NULL;
	}
}

bool C_Device::CreatePixelShader(U32 size, const U8* pData, U32 flags, C_PixelShader* pPixelShader, void* pMemory)
{
	BIZ_ASSERT(pPixelShader);
	BIZ_ASSERT(!pPixelShader->IsCreated());

	BIZ_ASSERT(m_pd3dDevice);
	HRESULT hr = m_pd3dDevice->CreatePixelShader((DWORD*)pData, &pPixelShader->m_pd3dPixelShader);
	if(FAILED(hr))
	{
		LOGERROR(g_Log, "Failed to create pcd3d9 pixel shader of size %i flags %i\n", size, flags);
		return false;
	}

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
		#if TRACK_DISPLAY2_RESOURCES
		if(!(pPixelShader->GetFlags() & E_Flags_DoNotTrack))
			RemovePixelShader(pPixelShader);
		#endif

		const ULONG refCount = pPixelShader->m_pd3dPixelShader->Release();
		BIZ_ASSERT(refCount == 0);
		pPixelShader->m_pd3dPixelShader = NULL;
	}
}

bool C_Device::CreateColourSurface(U32 width, U32 height, U32 flags, E_SurfaceFormat format, E_Multisample multisample, const C_SurfaceParams* pParams, C_Surface* pSurface, void* pMemory, const U32 memorySize)
{
	BIZ_ASSERT(pSurface);
	BIZ_ASSERT(!pSurface->IsCreated());

	BIZ_ASSERT(m_pd3dDevice);
	HRESULT hr = m_pd3dDevice->CreateRenderTarget(width, height, (D3DFORMAT)format, (D3DMULTISAMPLE_TYPE)multisample, 0, FALSE, &pSurface->m_pd3dSurface, NULL);
	if(FAILED(hr))
	{
		LOGERROR(g_Log, "Failed to create d3d colour surface of width %i height %i flags %i format %i multisample %i\n", width, height, flags, format, multisample);
		return false;
	}

	#if TRACK_DISPLAY2_RESOURCES
	if(!(flags & E_Flags_DoNotTrack))
		InsertSurface(pSurface);
	#endif
	
	return true;
}

bool C_Device::CreateDepthStencilSurface(U32 width, U32 height, U32 flags, E_SurfaceFormat format, E_Multisample multisample, const C_SurfaceParams* pParams, C_Surface* pSurface, void* pMemory, const U32 memorySize)
{
	BIZ_ASSERT(pSurface);
	BIZ_ASSERT(!pSurface->IsCreated());

	BIZ_ASSERT(m_pd3dDevice);
	HRESULT hr = m_pd3dDevice->CreateDepthStencilSurface(width, height, (D3DFORMAT)format, (D3DMULTISAMPLE_TYPE)multisample, 0, FALSE, &pSurface->m_pd3dSurface, NULL);
	if(FAILED(hr))
	{
		LOGERROR(g_Log, "Failed to create d3d stencil surface surface of width %i height %i flags %i format %i multisample %i\n", width, height, flags, format, multisample);
		return false;
	}

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

		const ULONG refCount = pSurface->m_pd3dSurface->Release();
		BIZ_ASSERT(refCount == 0);
		pSurface->m_pd3dSurface = NULL;
	}
}

bool C_Device::CreateTexture2D(U32 width, U32 height, U32 levels, U32 flags, E_TextureFormat format, C_Texture* pTexture, void* pMemory, const U32 memorySize)
{
	BIZ_ASSERT(pTexture);
	BIZ_ASSERT(!pTexture->IsCreated());
	
	const DWORD d3dUsage = D3DTextureUsage(flags);
	const D3DFORMAT d3dFormat = (D3DFORMAT)format;
	const D3DPOOL d3dPool = D3DPool(flags);

	BIZ_ASSERT(m_pd3dDevice);
	GPUTexture2D* pd3dTexture = NULL;
	HRESULT hr = m_pd3dDevice->CreateTexture(width, height, levels, d3dUsage, d3dFormat, d3dPool, &pd3dTexture, NULL);
	if(FAILED(hr))
	{
		LOGERROR(g_Log, "Failed to create d3d texture 2d of width %i height %i levels %i flags %i format %i hr 0x%08x usage 0x%x pool %i\n", width, height, levels, flags, format, hr, d3dUsage, d3dPool);	// [Blur2PC] hr
		return false;
	}

	pTexture->m_pd3dTexture = pd3dTexture;

	const U32 size = GetTexture2DSize(width, height, levels, flags, format);

	pTexture->SetSize(size);
	pTexture->SetFlags(flags);

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
	
	const DWORD d3dUsage = D3DTextureUsage(flags);
	const D3DFORMAT d3dFormat = (D3DFORMAT)format;
	const D3DPOOL d3dPool = D3DPool(flags);

	BIZ_ASSERT(m_pd3dDevice);
	GPUTexture3D* pd3dTexture = NULL;
	HRESULT hr = m_pd3dDevice->CreateVolumeTexture(width, height, depth, levels, d3dUsage, d3dFormat, d3dPool, &pd3dTexture, NULL);
	if(FAILED(hr))
	{
		LOGERROR(g_Log, "Failed to create d3d texture 3d of width %i height %i depth %i levels %i flags %i format %i\n", width, height, depth, levels, flags, format);
		return false;
	}

	pTexture->m_pd3dTexture = pd3dTexture;

	const U32 size = GetTexture3DSize(width, height, depth, levels, flags, format);

	pTexture->SetSize(size);
	pTexture->SetFlags(flags);

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
	
	const DWORD d3dUsage = D3DTextureUsage(flags);
	const D3DFORMAT d3dFormat = (D3DFORMAT)format;
	const D3DPOOL d3dPool = D3DPool(flags);

	BIZ_ASSERT(m_pd3dDevice);
	GPUTextureCube* pd3dTexture = NULL;
	HRESULT hr = m_pd3dDevice->CreateCubeTexture(width, levels, d3dUsage, d3dFormat, d3dPool, &pd3dTexture, NULL);
	if(FAILED(hr))
	{
		LOGERROR(g_Log, "Failed to create d3d texture cube of width %i levels %i flags %i format %i\n", width, levels, flags, format);
		return false;
	}

	pTexture->m_pd3dTexture = pd3dTexture;

	const U32 size = GetTextureCubeSize(width, levels, flags, format);

	pTexture->SetSize(size);
	pTexture->SetFlags(flags);

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
		#if TRACK_DISPLAY2_RESOURCES
		if(!(pTexture->GetFlags() & E_Flags_DoNotTrack))
			RemoveTexture(pTexture);
		#endif

		const ULONG refCount = pTexture->m_pd3dTexture->Release();
		BIZ_ASSERT(refCount == 0);
		pTexture->m_pd3dTexture = NULL;
	}
}

void C_Device::BlockUntilIdle()
{
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
