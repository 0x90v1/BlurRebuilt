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
#endif
}

}; // namespace D3D9
}; // namespace Display
