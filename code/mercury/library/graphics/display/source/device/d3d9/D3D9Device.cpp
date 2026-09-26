#include "stdafx.h"
#include "Display/Device/D3D9/D3D9Device.h"
#include "Display/Memory/MemoryGraphics.h"
#include "Display/Shaders/Constants.h"

//lint -e613

#ifdef CFG_ALLOW_OPTIMIZE_OFF
//#pragma optimize( "", off ) // CFG_ALLOW_OPTIMIZE_OFF
#endif

#ifdef DO_NOT_INLINE_D3D9_DEVICE_INL
#undef INLINE
#define INLINE
#include "Display/Device/D3D9/D3D9Device.inl"
#undef INLINE
#define INLINE inline
#endif

#ifdef PLATFORM_XENON
bool g_CreatingPixelShader = false;
bool g_CreatingVertexShader = false;
#endif

namespace Display
{

THREAD_LOCAL_STORAGE int g_iOwnerThreadRefCount = 0;

namespace D3D9
{

C_Sampler::C_Sampler()
{
	m_pTexture				= NULL;
	m_LodBias				= 0;
	// Border colour
	m_BorderColour			= 0x00000000;
	m_BorderColourDefault	= 0x00000000;
}

C_Sampler::~C_Sampler()
{
}

C_Stream::C_Stream() :
	m_pVertexBuffer(NULL),
	m_VertexStride(0),
	m_VertexOffset(0)
{
}

C_Stream::~C_Stream()
{
}

C_D3D9Device::C_D3D9Device()
{
	#ifdef PLATFORM_XENON
	m_HighPrecisionColourSurface		= false;
	#endif
	m_BlendHighPrecisionEnable			= E_Bool_Unknown;
	m_SlopeScaledBias					= 0.f;
	m_PresentInterval					= E_PresentInterval_Unknown;
	m_PresentIntervalDefault			= E_PresentInterval_Two;
	m_ForceTextureGamma					= E_Bool_Default;
	m_pVertexDeclaration				= NULL;
	m_pIndexBuffer						= NULL;
	m_pVertexShader						= NULL;
	m_pPixelShader						= NULL;
	m_pd3dDevice						= NULL;
	m_pd3dObject						= NULL;
	m_CurrentBackBuffer					= 0;
	m_CurrentFrontBuffer				= 0;
	m_SwappedFrontBuffer				= 0;
	m_CurrentWindow						= -1;
	
	for(U32 lock = 0; lock < E_PixelShaderConstantLock_Count; ++lock)
	{
		m_PixelShaderConstantLock[lock].enabled = false;
	}

	ZeroMemory(&m_d3dPresentParams, sizeof(m_d3dPresentParams));
}

C_D3D9Device::~C_D3D9Device()
{
	BIZ_ASSERT(m_Windows.Size() == 0);
}

void C_D3D9Device::InvalidateState()
{
	C_DeviceBase::InvalidateState();

	m_BlendHighPrecisionEnable = E_Bool_Unknown;
	
	for(U32 sampler = 0; sampler < E_Sampler_Count; ++sampler)
	{
		m_Sampler[sampler].InvalidateState();
	}

	for(U32 sampler = 0; sampler < E_VertexSampler_Count; ++sampler)
	{
		m_VertexSampler[sampler].InvalidateState();
	}

	for(U32 stream = 0; stream < E_Stream_Count; ++stream)
	{
		m_Stream[stream].InvalidateState();
	}

	m_pVertexShader = 0;
	// setting a NULL pixel shader is valid in D3D so set this to something else
	m_pPixelShader = (const Display::C_PixelShader*)0xFFFFFFFF;
	m_pVertexDeclaration = 0;
	m_pIndexBuffer = 0;
}

void C_D3D9Device::DefaultStates()
{
	// alpha test
	SetAlphaTestEnable(m_AlphaTestEnableDefault);
	SetAlphaTestFunc(m_AlphaTestFuncDefault);
	SetAlphaTestRef(m_AlphaTestRefDefault);
	SetAlphaToMaskEnable(m_AlphaToMaskEnableDefault);
	// blend
	SetBlendEnable(m_BlendEnableDefault);
	SetBlendSrc(m_BlendSrcDefault);
	SetBlendDst(m_BlendDstDefault);
	SetBlendOp(m_BlendOpDefault);
	SetBlendSrcAlpha(m_BlendSrcAlphaDefault);
	SetBlendDstAlpha(m_BlendDstAlphaDefault);
	SetBlendOpAlpha(m_BlendOpAlphaDefault);
	SetBlendHighPrecisionEnable(E_Bool_False);
	SetBlendMode(m_BlendModeDefault);
	// colour
	SetColourWriteEnable(m_ColourWriteEnableDefault);
	// cull
	SetCullMode(m_CullModeDefault);
	// depth
	SetDepthEnable(m_DepthEnableDefault);
	SetDepthFunc(m_DepthFuncDefault);
	SetDepthWriteEnable(m_DepthWriteEnableDefault);
	// fill
	SetFillMode(m_FillModeDefault);
	// point sprite
	SetPointSpriteEnable(m_PointSpriteEnableDefault);
	// pixel shader samplers
	for(U32 sampler = 0; sampler < E_Sampler_Count; sampler++)
	{
		SetTextureFilterMag(sampler, m_Sampler[sampler].m_FilterMagDefault);
		SetTextureFilterMin(sampler, m_Sampler[sampler].m_FilterMinDefault);
		SetTextureFilterMip(sampler, m_Sampler[sampler].m_FilterMipDefault);
		SetTextureAddressU(sampler, m_Sampler[sampler].m_AddressUDefault);
		SetTextureAddressV(sampler, m_Sampler[sampler].m_AddressVDefault);
		SetTextureAddressW(sampler, m_Sampler[sampler].m_AddressWDefault);
		SetTextureBorderColour(sampler, m_Sampler[sampler].m_BorderColourDefault);
		SetTextureGamma(sampler, m_Sampler[sampler].m_GammaDefault);
	}
	// vertex shader samplers
	for(U32 sampler = 0; sampler < E_VertexSampler_Count; sampler++)
	{
		SetVertexTextureFilterMag(sampler, m_VertexSampler[sampler].m_FilterMagDefault);
		SetVertexTextureFilterMin(sampler, m_VertexSampler[sampler].m_FilterMinDefault);
		SetVertexTextureFilterMip(sampler, m_VertexSampler[sampler].m_FilterMipDefault);
		SetVertexTextureAddressU(sampler, m_VertexSampler[sampler].m_AddressUDefault);
		SetVertexTextureAddressV(sampler, m_VertexSampler[sampler].m_AddressVDefault);
		SetVertexTextureAddressW(sampler, m_VertexSampler[sampler].m_AddressWDefault);
		SetVertexTextureBorderColour(sampler, m_VertexSampler[sampler].m_BorderColourDefault);
		SetVertexTextureGamma(sampler, m_VertexSampler[sampler].m_GammaDefault);
	}
}

bool C_D3D9Device::CreateDevice()
{
	InitialiseThreadOwnership();

	return true;
}

bool C_D3D9Device::CreateDeviceVolatile()
{
	return true;
}

void C_D3D9Device::DestroyDeviceVolatile()
{
}

void C_D3D9Device::DestroyDevice()
{
	DestroyD3D9();
}

void C_D3D9Device::DestroyD3D9()
{
	if(m_pd3dDevice)
	{
		
		if(!m_InParallel)
		{
			#if TRACK_DISPLAY2_RESOURCES
			m_Display2.DumpResources(); // We're about to destroy d3d so let's dump any resources that haven't been destroyed...
			#endif

			const ULONG refCount = m_pd3dDevice->Release();
#if 1
			if(refCount != 0)
			{
				LOGFATAL(g_Log, "Failed to release the d3d device %i\n", refCount);
			}
#else
			BIZ_ASSERT(refCount == 0);
#endif
		}
		m_pd3dDevice = g_pd3dDevice = NULL;
	} 
}

bool C_D3D9Device::CreateColourSurface()
{
	if(m_BackBuffers.Size() == 0)
	{
		return false;
	}

	C_BackBuffer* pBackBuffer = m_BackBuffers[0];

	BIZ_ASSERT(m_pd3dDevice);
	BIZ_ASSERT(!pBackBuffer->m_ColourSurface.IsCreated());
#if PLATFORM_XENON
	const HRESULT hr = E_FAIL;
#else
	const HRESULT hr = m_pd3dDevice->GetRenderTarget(0, &pBackBuffer->m_ColourSurface.m_pd3dSurface);
	BIZ_ASSERT(pBackBuffer->m_ColourSurface.m_pd3dSurface);
#endif
	if(SUCCEEDED(hr))
	{
		D3DSURFACE_DESC desc;
		HRESULT hr = pBackBuffer->m_ColourSurface.GetGPUSurface()->GetDesc(&desc);
		BIZ_ASSERT(SUCCEEDED(hr));
		pBackBuffer->m_Width = desc.Width;
		pBackBuffer->m_Height = desc.Height;

		m_RenderTarget.m_pColourSurface = &pBackBuffer->m_ColourSurface;
		return true;
	}
	else
	{
		LOGFATAL(g_Log, "Failed to create the device colour surface\n");
		return false;
	}
}

void C_D3D9Device::DestroyColourSurface()
{
	if(m_BackBuffers.Size() == 0)
	{
		return;
	}

	C_BackBuffer* pBackBuffer = m_BackBuffers[0];

#if PLATFORM_XENON
	BIZ_ASSERT(0);
#else
	if(pBackBuffer->m_ColourSurface.m_pd3dSurface)
	{
		const ULONG refCount = pBackBuffer->m_ColourSurface.m_pd3dSurface->Release();
		BIZ_ASSERT(refCount == 0);
		pBackBuffer->m_ColourSurface.m_pd3dSurface = NULL;
	}
#endif
}

bool C_D3D9Device::CreateDepthStencilSurface()
{
	if(m_BackBuffers.Size() == 0)
	{
		return false;
	}

	C_BackBuffer* pBackBuffer = m_BackBuffers[0];

	BIZ_ASSERT(m_pd3dDevice);
#if PLATFORM_XENON
	const HRESULT hr = E_FAIL;
#else
	BIZ_ASSERT(!pBackBuffer->m_DepthStencilSurface.m_pd3dSurface);
	const HRESULT hr = m_pd3dDevice->GetDepthStencilSurface(&pBackBuffer->m_DepthStencilSurface.m_pd3dSurface);
	BIZ_ASSERT(pBackBuffer->m_DepthStencilSurface.m_pd3dSurface);
#endif
	if(SUCCEEDED(hr))
	{
		m_RenderTarget.m_pDepthStencilSurface = &pBackBuffer->m_DepthStencilSurface;

		return true;
	}
	else
	{
		LOGFATAL(g_Log, "Failed to get the device depth stencil surface\n");
		return false;
	}
}

void C_D3D9Device::DestroyDepthStencilSurface()
{
	if(m_BackBuffers.Size() == 0)
	{
		return;
	}

	C_BackBuffer* pBackBuffer = m_BackBuffers[0];

#if PLATFORM_XENON
	BIZ_ASSERT(0);
#else
	if(pBackBuffer->m_DepthStencilSurface.m_pd3dSurface)
	{
		const ULONG refCount = pBackBuffer->m_DepthStencilSurface.m_pd3dSurface->Release();
		BIZ_ASSERT(refCount == 0);
		pBackBuffer->m_DepthStencilSurface.m_pd3dSurface = NULL;
	}
#endif
}

bool C_D3D9Device::BeginScene()
{
	BIZ_ASSERT(HaveThreadOwnership());

	BIZ_ASSERT(!m_pView);
	m_pView = NULL;

	BIZ_ASSERT(!m_InScene);
	BIZ_ASSERT(m_pd3dDevice);
	const HRESULT hr = m_pd3dDevice->BeginScene();
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to begin the scene");
	if(SUCCEEDED(hr))
	{
		m_InScene = true;
		SetColourSurface();
		SetDepthStencilSurface();
		Reset();
		return true;
	}
	else
	{
		return false;
	}
}

void C_D3D9Device::EndScene()
{
	BIZ_ASSERT(!m_pView);
	m_pView = NULL;

	BIZ_ASSERT(m_InScene);
	BIZ_ASSERT(m_pd3dDevice);
	const HRESULT hr = m_pd3dDevice->EndScene();
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to end the scene");
	Reset();
	m_InScene = false;
	
#if PLATFORM_XENON
	SetColourSurface(0, NULL);
	SetColourSurface(1, NULL);
	SetColourSurface(2, NULL);
	SetColourSurface(3, NULL);
	SetDepthStencilSurface(NULL);
#endif
}

void C_D3D9Device::Reset()
{
	PERF_EVENT("Display.Reset");

	// primitive reset
	#ifdef PLATFORM_XENON
	m_pd3dDevice->SetRenderState(D3DRS_PRIMITIVERESETENABLE, TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_PRIMITIVERESETINDEX, 0xFFFF);
	#endif

	m_pd3dDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);

	InvalidateState();
	DefaultStates();
	Unbind();
	
	SetView(m_pView);
}

void C_D3D9Device::Unbind()
{
	SetVertexShader(NULL);
	m_pd3dDevice->SetVertexShader(NULL);

	SetPixelShader(NULL);
	m_pd3dDevice->SetPixelShader(NULL);
	
	#ifndef DEVICE_PCD3D9
	SetVertexDeclaration(NULL);
	m_pd3dDevice->SetVertexDeclaration(NULL);
	#else	
	m_pVertexDeclaration = NULL; // Can't set a null vertex declaration on the pc...
	#endif

	for(U32 stream=0; stream<E_Stream_Count; stream++)
	{
		SetVertexBuffer(stream, NULL, 0, 0);
		m_pd3dDevice->SetStreamSource(stream, NULL, 0, 0);
	}

	SetIndexBuffer(NULL);
	m_pd3dDevice->SetIndices(NULL);

	for(U32 vertexSampler = 0; vertexSampler<E_VertexSampler_Count; vertexSampler++)
	{
		SetVertexTexture(vertexSampler, NULL);
		m_pd3dDevice->SetTexture(D3DVERTEXTEXTURESAMPLER0+vertexSampler, NULL);
	}

	for(U32 sampler=0; sampler<E_Sampler_Count; sampler++)
	{
		SetTexture(sampler, NULL);
		m_pd3dDevice->SetTexture(sampler, NULL);
	}
}

bool C_D3D9Device::IsSet(const C_VertexBuffer* pVertexBuffer) const
{
	for(U32 stream=0; stream<E_Stream_Count; stream++)
	{
		if (m_Stream[stream].m_pVertexBuffer == pVertexBuffer)
			return true;
	}

	return false;
}

void C_D3D9Device::Clear(const U32 flags)
{
	Clear(flags, GetColourClear(), GetDepthClear(), GetStencilClear());
}

void C_D3D9Device::Clear(const U32 flags, const Maths::RGBA colour)
{
	Clear(flags, colour, GetDepthClear(), GetStencilClear());
}

void C_D3D9Device::Clear(const U32 flags, const Maths::RGBA colour, const F32 depth, const U8 stencil)
{
	U32 d3dFlags = 0;
	if(flags & E_Clear_Colour)
		d3dFlags |= D3DCLEAR_TARGET;
	if(flags & E_Clear_Depth)
		d3dFlags |= D3DCLEAR_ZBUFFER;
	if(flags & E_Clear_Stencil)
		d3dFlags |= D3DCLEAR_STENCIL;
	const U32 d3dColour = ((colour.A() & 0xff) << 24) | ((colour.R() & 0xff) << 16) | ((colour.G() & 0xff) << 8) | (colour.B() & 0xff);;
	BIZ_ASSERT(m_pd3dDevice);
    const HRESULT hr = m_pd3dDevice->Clear(0, NULL, d3dFlags, d3dColour, depth, stencil);
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to clear the device");
}

bool C_D3D9Device::Present()
{
	BIZ_ASSERT(m_pd3dDevice);
#ifdef PLATFORM_PC
	const HRESULT hr = m_pd3dDevice->Present(NULL, NULL, NULL, NULL);
	switch(hr)
	{
	case D3D_OK:
		break;
	case D3DERR_DEVICELOST:
		BIZ_ASSERT_FAIL("Failed to present the device - D3DERR_DEVICELOST");
		return false;
	case D3DERR_DRIVERINTERNALERROR:
		BIZ_ASSERT_FAIL("Failed to present the device - D3DERR_DRIVERINTERNALERROR");
		return false;
	case D3DERR_INVALIDCALL:
		BIZ_ASSERT_FAIL("Failed to present the device - D3DERR_INVALIDCALL");
		return false;
	default:
		BIZ_ASSERT_FAIL("Failed to present the device");
		return false;
	}
	Unbind();

	return true;
#else
	m_pd3dDevice->Present(NULL, NULL, NULL, NULL);
	Unbind();

	return true;
#endif
}

void C_D3D9Device::SetColourSurface(const U32 index, const C_Surface* pSurface)
{
	BIZ_ASSERT(m_pd3dDevice);
	BIZ_ASSERT(pSurface ? pSurface->IsCreated() : 1);
	const HRESULT hr = m_pd3dDevice->SetRenderTarget(index, pSurface ? (Display2::GPUSurface*)pSurface->GetGPUSurface() : NULL);
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to set the colour surface");

	if(index == 0)
		m_RenderTarget.m_pColourSurface = pSurface;

	#ifdef PLATFORM_XENON
	m_HighPrecisionColourSurface = m_RenderTarget.m_pColourSurface ? m_RenderTarget.m_pColourSurface->GetFormat() == E_Format_A2B10G10R10F ? true : false : false;
	SetBlendHighPrecisionEnable(m_HighPrecisionColourSurface ? E_Bool_True : E_Bool_False);
	m_BlendMode = E_BlendMode_Unknown;
	#endif
}

void C_D3D9Device::SetDepthStencilSurface(const C_Surface* pSurface)
{
	BIZ_ASSERT(m_pd3dDevice);
	BIZ_ASSERT(pSurface ? pSurface->IsCreated() : 1);
	const HRESULT hr = m_pd3dDevice->SetDepthStencilSurface(pSurface ? (Display2::GPUSurface*)pSurface->GetGPUSurface() : NULL);
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to set the depth stencil surface");
	m_RenderTarget.m_pDepthStencilSurface = pSurface;
}

bool C_D3D9Device::CreateColourSurface(const U32 width, const U32 height, const U32 flags, const D3DFORMAT format, const D3DMULTISAMPLE_TYPE multisample, const C_SurfaceParams* pParams, C_Surface* pSurface)
{
	return m_Display2.CreateColourSurface(width, height, flags, (Display2::E_SurfaceFormat)format, (Display2::E_Multisample)multisample, pParams, pSurface);
}

bool C_D3D9Device::CreateDepthStencilSurface(const U32 width, const U32 height, const U32 flags, const D3DFORMAT format, const D3DMULTISAMPLE_TYPE multisample, const C_SurfaceParams* pParams, C_Surface* pSurface)
{
	return m_Display2.CreateDepthStencilSurface(width, height, flags, (Display2::E_SurfaceFormat)format, (Display2::E_Multisample)multisample, pParams, pSurface);
}

void C_D3D9Device::DestroySurface(C_Surface* pSurface)
{
	m_Display2.DestroySurface(pSurface);
}

bool C_D3D9Device::CreateTexture2D(const U32 width, const U32 height, const U32 levels, const U32 flags, const E_Format format, C_Texture2D* pTexture, U8* pAddress, const U32 addressSize)
{
	return m_Display2.CreateTexture2D(width, height, levels, flags, (Display2::E_TextureFormat)format, pTexture, pAddress, addressSize);
}

void C_D3D9Device::DestroyTexture2D(C_Texture2D* pTexture)
{
	m_Display2.DestroyTexture(pTexture);
}

bool C_D3D9Device::CreateTexture3D(const U32 width, const U32 height, const U32 depth, const U32 levels, const U32 flags, const E_Format format, C_Texture3D* pTexture, U8* pAddress, const U32 addressSize)
{
	return m_Display2.CreateTexture3D(width, height, depth, levels, flags, (Display2::E_TextureFormat)format, pTexture, pAddress, addressSize);
}

void C_D3D9Device::DestroyTexture3D(C_Texture3D* pTexture)
{
	m_Display2.DestroyTexture(pTexture);
}

bool C_D3D9Device::CreateTextureCube(const U32 width, const U32 levels, const U32 flags, const E_Format format, C_TextureCube* pTexture, U8* pAddress, const U32 addressSize)
{
	return m_Display2.CreateTextureCube(width, levels, flags, (Display2::E_TextureFormat)format, pTexture, pAddress, addressSize);
}

void C_D3D9Device::DestroyTextureCube(C_TextureCube* pTexture)
{
	m_Display2.DestroyTexture(pTexture);
}

bool C_D3D9Device::CreateRenderTextureCube(const U32 width, const U32 levels, U32 flags, const E_Format format, const E_Multisample multisample, const C_SurfaceParams* pParams, C_RenderTextureCube* pRenderTexture, U8* pAddress, const U32 addressSize)
{
#if PLATFORM_XENON
	return false;
#else
	BIZ_ASSERT(width > 0);
	BIZ_ASSERT(levels > 0);
	BIZ_ASSERT(pRenderTexture);
	BIZ_ASSERT(!pRenderTexture->IsCreated());
	
	if(!(flags & Display2::E_Flags_Colour) || !(flags & Display2::E_Flags_DepthStencil))
		flags |= Display2::E_Flags_Colour;

	flags |= E_Usage_Video;
	
	pRenderTexture->m_Flags = flags;

	if(!CreateTextureCube(width, levels, flags, format, &pRenderTexture->m_Texture, pAddress, addressSize))
		return false;

	for(U32 face=0; face<6; face++)
	{
		BIZ_ASSERT(pRenderTexture->m_Surfaces[face].Size() == 0);
		pRenderTexture->m_Surfaces[face].Resize(levels);

		for(S32 level=0; level<pRenderTexture->m_Surfaces[face].Size(); level++)
		{
			C_Surface& surface = pRenderTexture->m_Surfaces[face][level];

			const HRESULT hr = pRenderTexture->m_Texture.GetGPUTextureCube()->GetCubeMapSurface((D3DCUBEMAP_FACES)face, 0, &surface.m_pd3dSurface);
			BIZ_ASSERT(SUCCEEDED(hr));
		}
	}

	#if TRACK_DISPLAY2_RESOURCES
	if(!(flags & Display2::E_Flags_DoNotTrack))
		m_Display2.InsertRenderTextureCube(pRenderTexture);
	#endif

	return true;
#endif
}

void C_D3D9Device::DestroyRenderTextureCube(C_RenderTextureCube* pRenderTexture)
{
#if PLATFORM_XENON
	BIZ_ASSERT(0);
#else
	BIZ_ASSERT(pRenderTexture);

	for(U32 face=0; face<6; face++)
	{
		for(S32 level=0; level<pRenderTexture->m_Surfaces[face].Size(); level++)
		{
			C_Surface& surface = pRenderTexture->m_Surfaces[face][level];

			if(surface.m_pd3dSurface)
			{
				const ULONG refCount = surface.m_pd3dSurface->Release();
				BIZ_ASSERT(refCount == 6-face);
				surface.m_pd3dSurface = NULL;
			}
		}

		pRenderTexture->m_Surfaces[face].Clear();
	}

	DestroyTextureCube(&pRenderTexture->m_Texture);

	#if TRACK_DISPLAY2_RESOURCES
	if(!(pRenderTexture->m_Flags & Display2::E_Flags_DoNotTrack))
		m_Display2.RemoveRenderTextureCube(pRenderTexture);
	#endif

	pRenderTexture->m_Flags = 0;
#endif
}

C_VertexDeclaration* C_D3D9Device::GetVertexDeclaration(const S32 vertexElementCount, const C_VertexElement* pVertexElements)
{
	return m_Display2.FindVertexDeclaration(vertexElementCount, pVertexElements);
}

C_VertexDeclaration* C_D3D9Device::InitVertexDeclaration(const S32 vertexElementCount, const C_VertexElement* pVertexElements)
{
	return m_Display2.CreateVertexDeclaration(pVertexElements);
}

bool C_D3D9Device::CreateVertexBuffer(const U32 size, const U32 flags, C_VertexBuffer* pVertexBuffer, bool allowFail)
{
	return m_Display2.CreateVertexBuffer(size, flags, pVertexBuffer);
}

void C_D3D9Device::DestroyVertexBuffer(C_VertexBuffer* pVertexBuffer)
{
	m_Display2.DestroyVertexBuffer(pVertexBuffer);
}

bool C_D3D9Device::CreateIndexBuffer(const U32 size, const U32 flags, const E_Format format, C_IndexBuffer* pIndexBuffer, bool allowFail)
{
	return m_Display2.CreateIndexBuffer(size, flags, (Display2::E_IndexFormat)format, pIndexBuffer);
}

void C_D3D9Device::DestroyIndexBuffer(C_IndexBuffer* pIndexBuffer)
{
	m_Display2.DestroyIndexBuffer(pIndexBuffer);
}

#ifdef PLATFORM_PC
#ifdef OBJ_SHADERS
bool C_D3D9Device::CreateVertexShader(const U32 size, const C_ShaderData& data, const U32 flags, C_VertexShader* pVertexShader)
{
	U32 *pData = (U32*)&data;
	if (pData[1] > 0x10000)
	{
		// HACK TO SUPPORT BACKWARDS COMPATIBILITY
		const C_ShaderProfile& profile = data.GetProfile(m_ShaderProfile);
		return CreateVertexShader(profile.m_Size, profile.m_pData, flags, pVertexShader);
	}
	int c = pData[m_ShaderProfile*2+1];
	int *pShaderData = (int*)(((U8*)pData) + pData[m_ShaderProfile*2+0]);
	return CreateVertexShader(c, pShaderData, flags, pVertexShader);
}
#else
bool C_D3D9Device::CreateVertexShader(const U32 size, const C_ShaderData& data, const U32 flags, C_VertexShader* pVertexShader)
{
	const C_ShaderProfile& profile = data.GetProfile(m_ShaderProfile);
	return CreateVertexShader(profile.m_Size, profile.m_pData, flags, pVertexShader);
}
#endif
#endif

bool C_D3D9Device::CreateVertexShader(const U32 size, const S32* pData, const U32 flags, C_VertexShader* pVertexShader)
{
	return m_Display2.CreateVertexShader(size, (const U8*)pData, flags, pVertexShader);
}

void C_D3D9Device::DestroyVertexShader(C_VertexShader* pVertexShader)
{
	m_Display2.DestroyVertexShader(pVertexShader);
}

#ifdef PLATFORM_PC
#ifdef OBJ_SHADERS
bool C_D3D9Device::CreatePixelShader(const U32 size, const C_ShaderData& data, const U32 flags, C_PixelShader* pPixelShader)
{
	U32 *pData = (U32*)&data;
	if (pData[1] > 0x10000)
	{
		// HACK TO SUPPORT BACKWARDS COMPATIBILITY
		const C_ShaderProfile& profile = data.GetProfile(m_ShaderProfile);
		return CreatePixelShader(profile.m_Size, profile.m_pData, flags, pPixelShader);
	}
	int c = pData[m_ShaderProfile*2+1];
	int *pShaderData = (int*)(((U8*)pData) + pData[m_ShaderProfile*2+0]);
	return CreatePixelShader(c, pShaderData, flags, pPixelShader);
}
#else
bool C_D3D9Device::CreatePixelShader(const U32 size, const C_ShaderData& data, const U32 flags, C_PixelShader* pPixelShader)
{
	const C_ShaderProfile& profile = data.GetProfile(m_ShaderProfile);
	return CreatePixelShader(profile.m_Size, profile.m_pData, flags, pPixelShader);
}
#endif
#endif

bool C_D3D9Device::CreatePixelShader(const U32 size, const S32* pData, const U32 flags, C_PixelShader* pPixelShader)
{
	return m_Display2.CreatePixelShader(size, (const U8*)pData, flags, pPixelShader);
}

void C_D3D9Device::DestroyPixelShader(C_PixelShader* pPixelShader)
{
	m_Display2.DestroyPixelShader(pPixelShader);
}

void C_D3D9Device::SetupViewIndex(const S32 viewIndex, const C_ViewSetup& viewSetup)
{
	BIZ_ASSERT(viewIndex >= 0 && viewIndex < m_Views.Size());
	m_Views[viewIndex]->Setup(viewSetup, m_RenderTarget.GetColourWidth(), m_RenderTarget.GetColourHeight());
}

void C_D3D9Device::SetupViewIndex(const S32 viewIndex, const C_ViewSetup& viewSetup, const U32 width, const U32 height)
{
	BIZ_ASSERT(viewIndex >= 0 && viewIndex < m_Views.Size());
	m_Views[viewIndex]->Setup(viewSetup, width, height);
}

void C_D3D9Device::SetViewIndex(const S32 viewIndex, const bool setStates)
{
	if(viewIndex >= 0 && viewIndex < m_Views.Size())
	{
		SetView(m_Views[viewIndex], setStates);
	}
	else
	{
		SetView(NULL, setStates);
	}
}

void C_D3D9Device::SetupView(C_View* pView, const C_ViewSetup& viewSetup)
{
	BIZ_ASSERT(pView);
	pView->Setup(viewSetup, m_RenderTarget.GetColourWidth(), m_RenderTarget.GetColourHeight());
}

void C_D3D9Device::SetupView(C_View* pView, const C_ViewSetup& viewSetup, const U32 width, const U32 height)
{
	BIZ_ASSERT(pView);
	pView->Setup(viewSetup, width, height);
}

void C_D3D9Device::SetView(const C_View* pView, const bool setStates)
{
	BIZ_ASSERT(m_pd3dDevice);

	m_pView = pView;
	
	if(m_pView)
	{
		if(setStates)
		{
			m_Viewport = m_pView->GetViewport();
			
			SetViewport(m_Viewport);
			SetDepthFunc(E_CmpFunc_Default);

			SetViewShaderConstants2D();
			SetViewShaderConstants3D();
		}
	}
	else
	{
		if(setStates)
		{
			SetViewShaderConstants2D();
		}
	}
}

void C_D3D9Device::SetViewShaderConstants()
{
	SetViewShaderConstants2D();
	SetViewShaderConstants3D();
}

void C_D3D9Device::SetViewShaderConstants2D()
{
	if(m_pView)
	{
		const C_ViewSettings2D& viewSettings2D = m_pView->GetViewSettings2D();
		m_pd3dDevice->SetVertexShaderConstantF(CV_DISPLAY_CLIP_VECTOR, (F32*)&viewSettings2D.GetClipTransform(), 1);
	}
	else
	{
		const C_Float4 clipTransform(0.001953125f, -0.00260416674f, -1.f, 1.f);
		m_pd3dDevice->SetVertexShaderConstantF(CV_DISPLAY_CLIP_VECTOR, (F32*)&clipTransform, 1);
	}
}

void C_D3D9Device::SetViewShaderConstants3D()
{
	if(m_pView)
	{
		const C_ViewSettings3D& viewSettings3D = m_pView->GetViewSettings3D();
		m_pd3dDevice->SetVertexShaderConstantF(CV_DISPLAY_VIEW_MATRIX, (F32*)&viewSettings3D.GetViewMatrix(), 4);
		m_pd3dDevice->SetVertexShaderConstantF(CV_DISPLAY_PROJ_MATRIX, (F32*)&viewSettings3D.GetProjMatrix(), 4);
		m_pd3dDevice->SetVertexShaderConstantF(CV_DISPLAY_VIEWPROJ_MATRIX, (F32*)&viewSettings3D.GetViewProjMatrix(), 4);
		m_pd3dDevice->SetVertexShaderConstantF(CV_DISPLAY_VIEW_POSITION, (F32*)&viewSettings3D.GetViewPosition(), 1);
		m_pd3dDevice->SetPixelShaderConstantF(CP_DISPLAY_VIEW_POSITION, (F32*)&viewSettings3D.GetViewPosition(), 1);
	}
}

void C_D3D9Device::InitialiseBuffers()
{
	BIZ_ASSERT(m_BackBuffers.Size() > 0);

	BIZ_ASSERT(m_pd3dDevice);
#if PLATFORM_XENON
	BIZ_ASSERT(0);
#else
	m_pd3dDevice->GetRenderTarget(0, &m_BackBuffers[0]->m_ColourSurface.m_pd3dSurface);
#endif
	m_RenderTarget.m_pColourSurface = &m_BackBuffers[0]->m_ColourSurface;

#if PLATFORM_XENON
	BIZ_ASSERT(0);
#else
	m_pd3dDevice->GetDepthStencilSurface(&m_BackBuffers[0]->m_DepthStencilSurface.m_pd3dSurface);
#endif
	m_RenderTarget.m_pDepthStencilSurface = &m_BackBuffers[0]->m_DepthStencilSurface;
}

C_PixelShaderConstantLockKey C_D3D9Device::LockPixelShaderConstants(const U32 startRegister, const U32 endRegister)
{
	C_PixelShaderConstantLockKey lockKey;
	lockKey.index = 0xFFFFFFFF;
		
	if(!CheckPixelShaderConstantsLocked(startRegister, endRegister))
	{
		for(U32 i = 0; i < E_PixelShaderConstantLock_Count; ++i)
		{
			C_PixelShaderConstantLock& lock = m_PixelShaderConstantLock[i];
			if(!lock.enabled)
			{
				lock.enabled = true;
				lock.salt++;
				lock.startRegister = startRegister;
				lock.endRegister = endRegister;

				lockKey.index = i;
				lockKey.salt = lock.salt;

				return lockKey;
			}
		}
	}
	else
	{
		LOGWARN(g_Log, "Shader constants %i to %i are already locked\n", startRegister, endRegister);
		return lockKey;
	}

	LOGWARN(g_Log, "No more shader constant locks available\n");
	return lockKey;	
}

void C_D3D9Device::UnlockPixelShaderConstants(const C_PixelShaderConstantLockKey lockKey)
{
	if(lockKey.index != 0xFFFFFFFF && lockKey.index < E_PixelShaderConstantLock_Count)
	{
		C_PixelShaderConstantLock& lock = m_PixelShaderConstantLock[lockKey.index];
		if(lock.enabled && lock.salt == lockKey.salt)
		{
			lock.enabled = false;
			return;
		}
	}
	
	LOGINFO(g_Log, "Invalid PixelShaderConstantLockKey\n");
}

const S32 C_D3D9Device::AddWindow(const Maths::C_Rect& windowCoords)
{
	C_Window* pWindow = new C_Window(windowCoords);
	m_Windows.PushBack(pWindow);
	return m_Windows.Size()-1;
}

void C_D3D9Device::SetForceTextureGamma(const E_Bool forceTextureGamma)
{
	BIZ_ASSERT(forceTextureGamma == E_Bool_False || forceTextureGamma == E_Bool_True || forceTextureGamma == E_Bool_Default);
	m_ForceTextureGamma = forceTextureGamma;
}

void C_D3D9Device::SetBlendMode(const E_BlendMode blendMode)
{
	if(m_BlendMode != blendMode)
    {
        switch(blendMode)
        {
			case E_BlendMode_Solid:
			{
				SetAlphaTestEnable(E_Bool_False);

				SetBlendEnable(E_Bool_False);

				if(m_LuminanceWrite == E_LuminanceWrite_Enabled)
					SetColourWriteEnable(E_ColourWriteEnable_All);
			
				#ifdef PLATFORM_XENON
				if(m_HighPrecisionColourSurface)
					SetBlendHighPrecisionEnable(E_Bool_False);
				#endif
			}
			break;

			case E_BlendMode_SolidAlphaTest:
			{
				SetAlphaTestEnable(E_Bool_True);
				SetAlphaTestFunc(E_CmpFunc_Greater);
				SetAlphaTestRef(0x80);

				SetBlendEnable(E_Bool_False);

				if(m_LuminanceWrite == E_LuminanceWrite_Enabled)
					SetColourWriteEnable(E_ColourWriteEnable_Red|E_ColourWriteEnable_Green|E_ColourWriteEnable_Blue);
			
				#ifdef PLATFORM_XENON
				if(m_HighPrecisionColourSurface)
					SetBlendHighPrecisionEnable(E_Bool_False);
				#endif
			}
			break;

			case E_BlendMode_Alpha:
			{
				SetAlphaTestEnable(E_Bool_True);
				SetAlphaTestFunc(E_CmpFunc_Greater);
				SetAlphaTestRef(0x00);
				
				SetBlendEnable(E_Bool_True);
				SetBlendSrc(E_Blend_SrcAlpha);
				SetBlendDst(E_Blend_InvSrcAlpha);
				SetBlendOp(E_BlendOp_Add);
				SetBlendSrcAlpha(E_Blend_SrcAlpha);
				SetBlendDstAlpha(E_Blend_InvSrcAlpha);
				SetBlendOpAlpha(E_BlendOp_Add);

				if(m_LuminanceWrite == E_LuminanceWrite_Enabled)
					SetColourWriteEnable(E_ColourWriteEnable_Red|E_ColourWriteEnable_Green|E_ColourWriteEnable_Blue);
			
				#ifdef PLATFORM_XENON
				if(m_HighPrecisionColourSurface)
					SetBlendHighPrecisionEnable(E_Bool_True);
				#endif
			}
			break;

			case E_BlendMode_AlphaPreMultiply:
			{
				SetAlphaTestEnable(E_Bool_False);
				SetAlphaTestFunc(E_CmpFunc_Greater);
				SetAlphaTestRef(0x00);
				
				SetBlendEnable(E_Bool_True);
				SetBlendSrc(E_Blend_One);
				SetBlendDst(E_Blend_InvSrcAlpha);
				SetBlendOp(E_BlendOp_Add);
				SetBlendSrcAlpha(E_Blend_One);
				SetBlendDstAlpha(E_Blend_InvSrcAlpha);
				SetBlendOpAlpha(E_BlendOp_Add);
				
				if(m_LuminanceWrite == E_LuminanceWrite_Enabled)
					SetColourWriteEnable(E_ColourWriteEnable_Red|E_ColourWriteEnable_Green|E_ColourWriteEnable_Blue);

				#ifdef PLATFORM_XENON
				if(m_HighPrecisionColourSurface)
					SetBlendHighPrecisionEnable(E_Bool_True);
				#endif
			}
			break;

			case E_BlendMode_Additive:
			{
				SetAlphaTestEnable(E_Bool_True);
				
				SetBlendEnable(E_Bool_True);
				SetBlendSrc(E_Blend_One);
				SetBlendDst(E_Blend_One);
				SetBlendOp(E_BlendOp_Add);
				SetBlendSrcAlpha(E_Blend_One);
				SetBlendDstAlpha(E_Blend_One);
				SetBlendOpAlpha(E_BlendOp_Add);
				
				if(m_LuminanceWrite == E_LuminanceWrite_Enabled)
					SetColourWriteEnable(E_ColourWriteEnable_Red|E_ColourWriteEnable_Green|E_ColourWriteEnable_Blue);
			
				#ifdef PLATFORM_XENON
				if(m_HighPrecisionColourSurface)
					SetBlendHighPrecisionEnable(E_Bool_False);
				#endif
			}
			break;

			case E_BlendMode_AlphaAdditive:
			{
        		SetAlphaTestEnable(E_Bool_True);
				SetAlphaTestFunc(E_CmpFunc_Greater);
				SetAlphaTestRef(0x00);
				
				SetBlendEnable(E_Bool_True);
				SetBlendSrc(E_Blend_SrcAlpha);
				SetBlendDst(E_Blend_One);
				SetBlendOp(E_BlendOp_Add);
				SetBlendSrcAlpha(E_Blend_SrcAlpha);
				SetBlendDstAlpha(E_Blend_One);
				SetBlendOpAlpha(E_BlendOp_Add);
				
				if(m_LuminanceWrite == E_LuminanceWrite_Enabled)
					SetColourWriteEnable(E_ColourWriteEnable_Red|E_ColourWriteEnable_Green|E_ColourWriteEnable_Blue);
			
				#ifdef PLATFORM_XENON
				if(m_HighPrecisionColourSurface)
					SetBlendHighPrecisionEnable(E_Bool_True);
				#endif
			}
			break;

			case E_BlendMode_AlphaSubtractive:
			{
        		SetAlphaTestEnable(E_Bool_True);
				SetAlphaTestFunc(E_CmpFunc_Greater);
				SetAlphaTestRef(0x00);
				
				SetBlendEnable(E_Bool_True);
				SetBlendSrc(E_Blend_SrcAlpha);
				SetBlendDst(E_Blend_One);
				SetBlendOp(E_BlendOp_RevSubtract);
				SetBlendSrcAlpha(E_Blend_SrcAlpha);
				SetBlendDstAlpha(E_Blend_One);
				SetBlendOpAlpha(E_BlendOp_RevSubtract);
				
				if(m_LuminanceWrite == E_LuminanceWrite_Enabled)
					SetColourWriteEnable(E_ColourWriteEnable_Red|E_ColourWriteEnable_Green|E_ColourWriteEnable_Blue);
			
				#ifdef PLATFORM_XENON
				if(m_HighPrecisionColourSurface)
					SetBlendHighPrecisionEnable(E_Bool_True);
				#endif
			}
			break;

			case E_BlendMode_Subtractive:
			{
				SetAlphaTestEnable(E_Bool_False);
				
				SetBlendEnable(E_Bool_True);
				SetBlendSrc(E_Blend_One);
				SetBlendDst(E_Blend_One);
				SetBlendOp(E_BlendOp_RevSubtract);
				SetBlendSrcAlpha(E_Blend_One);
				SetBlendDstAlpha(E_Blend_One);
				SetBlendOpAlpha(E_BlendOp_RevSubtract);
				
				if(m_LuminanceWrite == E_LuminanceWrite_Enabled)
					SetColourWriteEnable(E_ColourWriteEnable_Red|E_ColourWriteEnable_Green|E_ColourWriteEnable_Blue);
			
				#ifdef PLATFORM_XENON
				if(m_HighPrecisionColourSurface)
					SetBlendHighPrecisionEnable(E_Bool_False);
				#endif
			}
			break;

			case E_BlendMode_Multiply:
			{
				SetAlphaTestEnable(E_Bool_False);
				
				SetBlendEnable(E_Bool_True);
				SetBlendSrc(E_Blend_Zero);
				SetBlendDst(E_Blend_SrcColour);
				SetBlendOp(E_BlendOp_Add);
				SetBlendSrcAlpha(E_Blend_Zero);
				SetBlendDstAlpha(E_Blend_SrcColour);
				SetBlendOpAlpha(E_BlendOp_Add);
				
				if(m_LuminanceWrite == E_LuminanceWrite_Enabled)
					SetColourWriteEnable(E_ColourWriteEnable_Red|E_ColourWriteEnable_Green|E_ColourWriteEnable_Blue);
			
				#ifdef PLATFORM_XENON
				if(m_HighPrecisionColourSurface)
					SetBlendHighPrecisionEnable(E_Bool_True);
				#endif
			}
			break;

			case E_BlendMode_AdditiveSmooth:
			{
				SetAlphaTestEnable(E_Bool_True);
				
				SetBlendEnable(E_Bool_True);
				SetBlendSrc(E_Blend_One);
				SetBlendDst(E_Blend_InvSrcColour);
				SetBlendOp(E_BlendOp_Add);
				SetBlendSrcAlpha(E_Blend_One);
				SetBlendDstAlpha(E_Blend_InvSrcColour);
				SetBlendOpAlpha(E_BlendOp_Add);
				
				if(m_LuminanceWrite == E_LuminanceWrite_Enabled)
					SetColourWriteEnable(E_ColourWriteEnable_Red|E_ColourWriteEnable_Green|E_ColourWriteEnable_Blue);
			
				#ifdef PLATFORM_XENON
				if(m_HighPrecisionColourSurface)
					SetBlendHighPrecisionEnable(E_Bool_True);
				#endif
			}
			break;

			case E_BlendMode_Unknown:
				break;

			default:
			{
				SetAlphaTestEnable(E_Bool_False);
				
				SetBlendEnable(E_Bool_False);
				
				if(m_LuminanceWrite == E_LuminanceWrite_Enabled)
					SetColourWriteEnable(E_ColourWriteEnable_Red|E_ColourWriteEnable_Green|E_ColourWriteEnable_Blue);
				
				#ifdef PLATFORM_XENON
				if(m_HighPrecisionColourSurface)
					SetBlendHighPrecisionEnable(E_Bool_False);
				#endif
			}
			break;
        }

        m_BlendMode = blendMode;
    }
}

void C_D3D9Device::InitialiseThreadOwnership()
{
	// different case for init as the main thread already has ownership of device
	g_iOwnerThreadRefCount = 1;
	m_ThreadOwnershipMutex.Wait();
}

bool C_D3D9Device::HaveThreadOwnership()
{
	return (g_iOwnerThreadRefCount > 0);
}

bool C_D3D9Device::AcquireThreadOwnership( U32 timeMs )
{
	if (m_ThreadOwnershipMutex.Wait(timeMs))
	{
		if (g_iOwnerThreadRefCount++ == 0)
		{
#ifdef PLATFORM_XENON
			m_pd3dDevice->AcquireThreadOwnership();
#endif
		}

		return true;
	}

	return false;
}

void C_D3D9Device::ReleaseThreadOwnership()
{
	// Check this thread has ownership before we allow it to release
	BIZ_ASSERT(g_iOwnerThreadRefCount > 0);

	if (--g_iOwnerThreadRefCount == 0)
	{
#ifdef PLATFORM_XENON
		m_pd3dDevice->ReleaseThreadOwnership();
#endif
	}

	m_ThreadOwnershipMutex.Release();
}

}; // namespace D3D9
}; // namespace Display
