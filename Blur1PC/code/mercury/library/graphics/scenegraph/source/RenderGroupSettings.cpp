#include "stdafx.h"

#include "SceneGraph/RenderGroupSettings.h"

#include "Display/Device/Device.h"

#ifndef PLATFORM_PC
#include "Display/Window/Window.h"
#endif

namespace SceneGraph
{


void C_RenderGroupSettings::SetWindow(const S32 windowIndex, const bool aa)
{
#ifndef PLATFORM_PC
	const Display::C_Window* pWindow = Display::Device()->GetWindow(windowIndex);
	
	if(aa)
	{
		m_Multisample = pWindow->GetMultisample();
		m_WindowIndex = windowIndex;
	#ifdef PLATFORM_XENON
		m_pColourSurface = &pWindow->m_ColourSurfaceWithAA;
		m_pDepthStencilSurface = &pWindow->m_DepthStencilSurfaceWithAA;
	#endif
	}
	else
	{
		m_WindowIndex = -1;
		m_Multisample = Display::E_Multisample_None;
	#ifdef PLATFORM_XENON
		m_pColourSurface = &pWindow->m_ColourSurfaceWithoutAA;
		m_pDepthStencilSurface = &pWindow->m_DepthStencilSurfaceWithoutAA;
	#endif
	}
#else
	
	const Display::C_BackBuffer& backBuffer = Display::Device()->GetBackBuffer();
	m_Multisample = backBuffer.GetMultisample();

#endif
}

} // namespace SceneGraph

