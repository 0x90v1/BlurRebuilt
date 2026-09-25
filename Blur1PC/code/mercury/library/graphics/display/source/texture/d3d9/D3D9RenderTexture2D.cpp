#include "stdafx.h"
#include "Display/Texture/D3D9/D3D9RenderTexture2D.h"
#include "Display/Device/Device.h"

namespace Display
{
namespace D3D9
{

C_RenderTexture2D::C_RenderTexture2D()
{
}

C_RenderTexture2D::~C_RenderTexture2D()
{
}

void C_RenderTexture2D::Resolve(const U32 level) const
{
#ifdef DEVICE_XB2D3D9
	Display::Device()->Resolve(Display::E_Resolve_Colour, &m_Texture, level);
#endif

#ifdef DEVICE_PCD3D9
	if(m_Flags & E_Usage_ReadWrite)
	{
		// alternate the surface and texture indicies
		++m_SurfaceIndex;
		++m_TextureIndex;
		
		m_SurfaceIndex &= ~(1<<1);
		m_TextureIndex &= ~(1<<1);
	}

	// [Blur2PC] retail Blur PC (Blur.exe 1.2 0x005adff0): a multisampled render texture copies its render
	// target into the texture (see C_PCD3D9Device::CreateRenderTexture2D)
	if(m_Multisample != D3DMULTISAMPLE_NONE)
	{
		IDirect3DSurface9* pSource = m_Surfaces[0][0].m_pd3dSurface;
		IDirect3DSurface9* pDest = m_ResolveSurfaces[0][0].m_pd3dSurface;
		if(pSource != pDest)
			Display::Device()->GetD3DDevice()->StretchRect(pSource, NULL, pDest, NULL, D3DTEXF_LINEAR);
	}
#endif
}

}; // namespace D3D9
}; // namespace Display
