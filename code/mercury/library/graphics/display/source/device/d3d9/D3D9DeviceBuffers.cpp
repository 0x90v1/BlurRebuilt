#include "stdafx.h"
#include "Display/Device/D3D9/D3D9DeviceBuffers.h"

#ifdef CFG_ALLOW_OPTIMIZE_OFF
//#pragma optimize( "", off ) // CFG_ALLOW_OPTIMIZE_OFF
#endif

namespace Display
{
namespace D3D9
{

// C_BackBuffer

C_BackBuffer::C_BackBuffer() :
	m_Width(0),
	m_Height(0),
	m_ColourFormat(E_Format_Unknown),
	m_DepthStencilFormat(E_Format_Unknown),
	m_Multisample(E_Multisample_None)
{
}

C_BackBuffer::~C_BackBuffer()
{
}

// C_FrontBuffer

C_FrontBuffer::C_FrontBuffer() :
	m_Width(0),
	m_Height(0),
	m_Format(E_Format_Unknown)
{
}

C_FrontBuffer::~C_FrontBuffer()
{
}

// C_RenderTarget

C_RenderTarget::C_RenderTarget() :
	m_pColourSurface(NULL),
	m_pDepthStencilSurface(NULL)
{
}

C_RenderTarget::~C_RenderTarget()
{
}

}; // namespace D3D9
}; // namespace Display
