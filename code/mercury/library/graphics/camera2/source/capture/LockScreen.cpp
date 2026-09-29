
#include "stdafx.h"
#pragma warning (push, 3)
#include <Display.h>
#pragma warning (pop)
#include "camera2/capture/LockScreen.h"

namespace Cam2
{

#ifdef PLATFORM_XENON
// Get a Xenon screen surface and return it as a IDirect3DSurface9
static IDirect3DSurface9 *GetScreenSurface()
{
	IDirect3DSurface9 *surface=NULL;

	const Display::C_FrontBuffer& frontBuffer = Display::Device()->GetSwappedFrontBuffer();
	const Display::C_Texture2D* pTexture = frontBuffer.GetTexture();
	if (pTexture->IsCreated())
	{
		((D3DTexture*)pTexture->GetGPUTexture())->GetSurfaceLevel(0, &surface);
	}

	return surface;
}
#endif

#if defined(PLATFORM_PC) && !defined(BLUR_USE_DX11)
// Get a PC screen surface and return it as a IDirect3DSurface9
static IDirect3DSurface9 *GetScreenSurface()
{
	const Display::C_BackBuffer& backBuffer = Display::Device()->GetBackBuffer();

	S32 width  = backBuffer.GetWidth();
	S32 height = backBuffer.GetHeight();
	
	Display::C_RenderTexture2D renderTexture;
	Display::Device()->CreateRenderTexture2D(width, height, 1, Display::E_Usage_Colour, Display::E_Format_A8R8G8B8,  Display::E_Multisample_None, 0, &renderTexture);
	if (!renderTexture.IsCreated())
		return 0;

	#if TRACK_DISPLAY2_RESOURCES
	Display2::Device()->SetRenderTexture2DName(&renderTexture, "_LockScreenRT");
	#endif

	Display::Device()->GetD3DDevice()->StretchRect(backBuffer.GetColourSurface()->m_pd3dSurface, 0, renderTexture.GetSurface(0).m_pd3dSurface, 0, D3DTEXF_NONE);

	IDirect3DSurface9 *surface = 0;
	Display::Device()->GetD3DDevice()->CreateOffscreenPlainSurface(width, height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surface, 0 );

	if (surface)
	{
		Display::Device()->GetD3DDevice()->GetRenderTargetData(renderTexture.GetSurface(0).m_pd3dSurface, surface);
	}

	Display::Device()->Destroy(&renderTexture);

	return surface;
}
#endif

#if defined(BLUR_USE_DX11)
// [Blur1PCDX11] Read the current back buffer through a D3D11 staging texture.
static ID3D11Texture2D* Surface = nullptr;
static ID3D11DeviceContext* s_LockContext = nullptr;

static ID3D11Texture2D* GetScreenSurface()
{
	ID3D11Device* device = Display::Device()->GetD3DDevice();
	const Display::C_Surface* colour = Display::Device()->GetBackBuffer().GetColourSurface();
	if (!device || !colour || !colour->m_pd3dSurface) return nullptr;
	ID3D11DeviceContext* context = nullptr;
	device->GetImmediateContext(&context);
	if (!context) return nullptr;
	D3D11_TEXTURE2D_DESC desc = {};
	colour->m_pd3dSurface->GetDesc(&desc);
	ID3D11Texture2D* resolved = nullptr;
	if (desc.SampleDesc.Count > 1)
	{
		D3D11_TEXTURE2D_DESC resolveDesc = desc;
		resolveDesc.SampleDesc.Count = 1;
		resolveDesc.SampleDesc.Quality = 0;
		resolveDesc.BindFlags = 0;
		resolveDesc.MiscFlags = 0;
		if (FAILED(device->CreateTexture2D(&resolveDesc, nullptr, &resolved)))
		{
			context->Release();
			return nullptr;
		}
		context->ResolveSubresource(resolved, 0, colour->m_pd3dSurface, 0, desc.Format);
	}
	D3D11_TEXTURE2D_DESC stagingDesc = desc;
	stagingDesc.SampleDesc.Count = 1;
	stagingDesc.SampleDesc.Quality = 0;
	stagingDesc.Usage = D3D11_USAGE_STAGING;
	stagingDesc.BindFlags = 0;
	stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	stagingDesc.MiscFlags = 0;
	ID3D11Texture2D* staging = nullptr;
	if (SUCCEEDED(device->CreateTexture2D(&stagingDesc, nullptr, &staging)))
		context->CopyResource(staging, resolved ? resolved : colour->m_pd3dSurface);
	if (resolved) resolved->Release();
	context->Release();
	return staging;
}
#endif

#if (defined(PLATFORM_XENON) || defined(PLATFORM_PC)) && !defined(BLUR_USE_DX11)
static IDirect3DSurface9 *Surface = 0;

S32 LockDx(IDirect3DSurface9 *surf, C_LockScreenInfo &info)
{
	// Get the dimensions of the surface:
	D3DSURFACE_DESC desc;
	memset(&desc, 0, sizeof(desc));
	surf->GetDesc(&desc);

	// Lock the memory:
	D3DLOCKED_RECT rect = {0, NULL};
	surf->LockRect(&rect, NULL, D3DLOCK_READONLY);
	if (rect.pBits == 0)
	{
		surf->Release();
		return 1;
	}

	info.bits = rect.pBits;
	info.pitch = rect.Pitch;
	info.format = desc.Format;
	info.width = desc.Width;
	info.height = desc.Height;
	return 0;
}
#endif

S32 LockScreen(C_LockScreenInfo& info)
{
#if defined(BLUR_USE_DX11)
	Surface = GetScreenSurface();
	if (!Surface) return 1;
	Display::Device()->GetD3DDevice()->GetImmediateContext(&s_LockContext);
	D3D11_MAPPED_SUBRESOURCE mapped = {};
	if (!s_LockContext || FAILED(s_LockContext->Map(Surface, 0, D3D11_MAP_READ, 0, &mapped)))
	{
		if (s_LockContext) { s_LockContext->Release(); s_LockContext = nullptr; }
		Surface->Release(); Surface = nullptr;
		return 1;
	}
	D3D11_TEXTURE2D_DESC desc = {};
	Surface->GetDesc(&desc);
	info.bits = mapped.pData;
	info.pitch = mapped.RowPitch;
	info.format = D3DFMT_A8R8G8B8;
	info.width = desc.Width;
	info.height = desc.Height;
	return 0;
#endif
#if defined(PLATFORM_XENON) || defined(PLATFORM_PC)
	#if !defined(BLUR_USE_DX11)
	Surface = GetScreenSurface();
	if (Surface == 0)
		return 1;
	
	if (LockDx(Surface, info) != 0)
		return 0;
	
	return 0;
	#endif
#endif

#ifdef PLATFORM_PS3
	const Display::C_FrontBuffer& frontBuffer = Display::Device()->GetSwappedFrontBuffer();
	const Display::C_Surface* pSurface = frontBuffer.GetSurface();
	if (!pSurface->IsCreated())
		return 1;

	info.bits = pSurface->GetAddress();
	info.pitch = GCM_ALIGN_SIZE(pSurface->GetWidth(), 64) << 2;

	if (pSurface->GetWidth() == 1920)
		info.pitch = 8192;

	info.format = 3;
	info.width = pSurface->GetWidth();
	info.height = pSurface->GetHeight();
	
	return 0;
#endif
}

void UnlockScreen()
{
#if defined(BLUR_USE_DX11)
	if (s_LockContext && Surface)
	{
		s_LockContext->Unmap(Surface, 0);
		s_LockContext->Release();
		Surface->Release();
		s_LockContext = nullptr;
		Surface = nullptr;
	}
#else
#if defined(PLATFORM_XENON) || defined(PLATFORM_PC)
	Surface->UnlockRect();
	Surface->Release();
#endif
#endif
}

} // End of namespace
