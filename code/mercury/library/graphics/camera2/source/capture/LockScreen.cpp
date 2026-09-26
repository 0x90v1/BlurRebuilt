
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

#ifdef PLATFORM_PC
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

#if defined(PLATFORM_XENON) || defined(PLATFORM_PC)
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
#if defined(PLATFORM_XENON) || defined(PLATFORM_PC)
	Surface = GetScreenSurface();
	if (Surface == 0)
		return 1;
	
	if (LockDx(Surface, info) != 0)
		return 0;
	
	return 0;
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
#if defined(PLATFORM_XENON) || defined(PLATFORM_PC)
	Surface->UnlockRect();
	Surface->Release();
#endif
}

} // End of namespace
