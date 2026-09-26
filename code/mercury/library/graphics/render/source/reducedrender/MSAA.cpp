#include "StdAfx.h"
#include "Render/ReducedRender/MSAA.h"

namespace Render
{
namespace Reduced
{

C_MSAA::C_MSAA()
{
}

C_MSAA::~C_MSAA()
{
}

void C_MSAA::End() const
{

}

void C_MSAA::Begin() const
{
	// save current state
	const Display::C_RenderTarget& renderTarget = Display::Device()->GetRenderTarget();
	
	if(m_pColourSurface != renderTarget.GetColourSurface())
	{
		m_pColourSurface = renderTarget.GetColourSurface();
		m_pDepthStencilSurface = renderTarget.GetDepthStencilSurface();

		assert(m_pColourSurface);
		assert(m_pDepthStencilSurface);

		// set and create new surfaces
	#if defined(PLATFORM_XENON)
		U32 size = 0;
		
		XGTEXTURE_DESC xgColourSurfaceDesc;
		XGGetSurfaceDesc((D3DSurface*)&m_pColourSurface->m_d3dSurface, &xgColourSurfaceDesc);

		D3DSURFACE_PARAMETERS d3dSurfaceParams;
		d3dSurfaceParams.Base = m_pColourSurface->m_d3dSurface.ColorInfo.ColorBase;
		d3dSurfaceParams.ColorExpBias = m_pColourSurface->m_d3dSurface.ColorInfo.ColorExpBias;
		d3dSurfaceParams.HierarchicalZBase = 0;
		d3dSurfaceParams.HiZFunc = D3DHIZFUNC_DEFAULT;

		size =  XGSetSurfaceHeader(xgColourSurfaceDesc.Width / 2,
									xgColourSurfaceDesc.Height / 2,
									xgColourSurfaceDesc.Format,
									D3DMULTISAMPLE_4_SAMPLES,
									&d3dSurfaceParams,
									&m_d3dColourSurface,
									0);
		XGOffsetSurfaceAddress(&m_d3dColourSurface, 0, 0);

		XGTEXTURE_DESC xgDepthStencilSurfaceDesc;
		XGGetSurfaceDesc((D3DSurface*)&m_pDepthStencilSurface->m_d3dSurface, &xgDepthStencilSurfaceDesc);

		d3dSurfaceParams.Base = m_pDepthStencilSurface->m_d3dSurface.DepthInfo.DepthBase;
		d3dSurfaceParams.HierarchicalZBase = 0;
		d3dSurfaceParams.ColorExpBias = 0;
		d3dSurfaceParams.HiZFunc = D3DHIZFUNC_DEFAULT;

		size =  XGSetSurfaceHeader(xgDepthStencilSurfaceDesc.Width / 2,
									xgDepthStencilSurfaceDesc.Height / 2,
									xgDepthStencilSurfaceDesc.Format,
									D3DMULTISAMPLE_4_SAMPLES,
									&d3dSurfaceParams,
									&m_d3dDepthStencilSurface,
									0);
		XGOffsetSurfaceAddress(&m_d3dDepthStencilSurface, 0, 0);
	#elif defined(PLATFORM_PS3)
		m_gcmSurface.type				= CELL_GCM_SURFACE_PITCH;
		m_gcmSurface.antialias			= CELL_GCM_SURFACE_SQUARE_CENTERED_4;
		m_gcmSurface.x 					= 0;
		m_gcmSurface.y 					= 0;
		m_gcmSurface.width 				= m_pColourSurface->GetWidth() / 2;
		m_gcmSurface.height 			= m_pColourSurface->GetHeight() / 2;
		
		U32 colourLocation = CELL_GCM_LOCATION_LOCAL;
		if(m_pColourSurface->GetFlags() & Display::E_Usage_System)
			colourLocation = CELL_GCM_LOCATION_MAIN;
		
		// colour buffers
		m_gcmSurface.colorFormat 		= m_pColourSurface->GetFormat();
		m_gcmSurface.colorTarget		= CELL_GCM_SURFACE_TARGET_0;
		m_gcmSurface.colorLocation[0]	= colourLocation;
		m_gcmSurface.colorLocation[1]	= colourLocation;
		m_gcmSurface.colorLocation[2]	= colourLocation;
		m_gcmSurface.colorLocation[3]	= colourLocation;
		m_gcmSurface.colorOffset[0] 	= m_pColourSurface->GetOffset();
		m_gcmSurface.colorOffset[1] 	= 0;
		m_gcmSurface.colorOffset[2] 	= 0;
		m_gcmSurface.colorOffset[3] 	= 0;
		m_gcmSurface.colorPitch[0] 		= m_pColourSurface->GetPitch();
		m_gcmSurface.colorPitch[1]		= 64;
		m_gcmSurface.colorPitch[2]		= 64;
		m_gcmSurface.colorPitch[3]		= 64;

		U32 depthLocation = CELL_GCM_LOCATION_LOCAL;
		if(m_pDepthStencilSurface->GetFlags() & Display::E_Usage_System)
			depthLocation = CELL_GCM_LOCATION_MAIN;
		
		// depth buffer
		m_gcmSurface.depthFormat 		= m_pDepthStencilSurface->GetFormat();
		m_gcmSurface.depthLocation		= depthLocation;
		m_gcmSurface.depthOffset		= m_pDepthStencilSurface->GetOffset();
		m_gcmSurface.depthPitch 		= m_pDepthStencilSurface->GetPitch();
	#endif
	}

	// set reduced surfaces
#if defined(PLATFORM_XENON)
	Display::Device()->GetD3DDevice()->SetRenderTarget(0, &m_d3dColourSurface);
	Display::Device()->GetD3DDevice()->SetDepthStencilSurface(&m_d3dDepthStencilSurface);
#elif defined(PLATFORM_PS3)
	cellGcmSetSurface(&m_gcmSurface);
#endif
}

} // namespace Reduced
} // namespace Render
