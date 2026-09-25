#include "stdafx.h"
#include "Display/Window/D3D9/D3D9Window.h"

#ifdef CFG_ALLOW_OPTIMIZE_OFF
//#pragma optimize( "", off ) // CFG_ALLOW_OPTIMIZE_OFF
#endif

namespace Display
{
namespace D3D9
{

C_Window::C_Window(const Maths::C_Rect& coords) :
	m_Coords(coords),
	m_Width(0),
	m_Height(0),
	m_Multisample(E_Multisample_None),
	m_ColourFormat(E_Format_A8R8G8B8),
	m_DepthStencilFormat(E_Format_D24FS8)
{
}

C_Window::~C_Window()
{
}

void C_Window::Resize(const U32 width, const U32 height, const E_Multisample multisample, const E_Format colourFormat, const E_Format depthStencilFormat)
{
	const F32 w1 = ((F32)(width))*m_Coords.Min().X();
	const F32 w2 = ((F32)(width))*m_Coords.Max().X();
	const F32 h1 = ((F32)(height))*m_Coords.Min().Y();
	const F32 h2 = ((F32)(height))*m_Coords.Max().Y();

	m_Width					= (U32)(w2-w1);
	m_Height				= (U32)(h2-h1);
	m_Multisample			= multisample;
	m_ColourFormat			= colourFormat;
	m_DepthStencilFormat	= depthStencilFormat;
}

}; // namespace D3D9
}; // namespace Display
