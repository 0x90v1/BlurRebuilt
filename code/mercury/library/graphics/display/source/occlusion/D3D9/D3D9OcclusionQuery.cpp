#include "stdafx.h"
#include "Display/Occlusion/D3D9/D3D9OcclusionQuery.h"
#include "Display/Device/Device.h"

#if !CFG_SPEED
S32 g_d3dOcclusionQueryCount = 0;
#endif

namespace Display
{
namespace D3D9
{

C_OcclusionQuery::C_OcclusionQuery() : m_pd3dQuery(NULL)
{
}

C_OcclusionQuery::~C_OcclusionQuery()
{
	Destroy();
}

bool C_OcclusionQuery::Create()
{
#ifndef DEVICE_DISABLE_OCCLUSION_QUERIES
	assert(!m_pd3dQuery);
	const HRESULT hr = Device()->GetD3DDevice()->CreateQuery(D3DQUERYTYPE_OCCLUSION, &m_pd3dQuery);
#if 1
	if(FAILED(hr))
	{
		//LOGWARN(g_Log, "Failed to create the d3d occlusion query %i\n", hr);
		m_pd3dQuery = NULL;
	}
	else
	{
		#if !CFG_SPEED
		//LOGINFO(g_Log, "Creating occlusion query %i\n", g_d3dOcclusionQueryCount);
		g_d3dOcclusionQueryCount++;
		#endif
	}
	
	return true;
#else
	assert(SUCCEEDED(hr) && "Failed to create the d3d occlusion query");
	return SUCCEEDED(hr) ? true : false;
#endif
#else // DEVICE_DISABLE_OCCLUSION_QUERIES
	return true;
#endif // DEVICE_DISABLE_OCCLUSION_QUERIES
}

void C_OcclusionQuery::Destroy()
{
#ifndef DEVICE_DISABLE_OCCLUSION_QUERIES
	if(m_pd3dQuery)
	{
		const ULONG refCount = m_pd3dQuery->Release();
		assert(refCount == 0);
		m_pd3dQuery = NULL;

		#if !CFG_SPEED
		//LOGINFO(g_Log, "Destroying occlusion query %i\n", g_d3dOcclusionQueryCount);
		g_d3dOcclusionQueryCount--;
		BIZ_ASSERT(g_d3dOcclusionQueryCount >= 0);
		#endif
	}
#endif // DEVICE_DISABLE_OCCLUSION_QUERIES
}

void C_OcclusionQuery::Begin()
{
#ifndef DEVICE_DISABLE_OCCLUSION_QUERIES
	if(m_pd3dQuery)
	{
		m_pd3dQuery->Issue(D3DISSUE_BEGIN);
	}
#endif // DEVICE_DISABLE_OCCLUSION_QUERIES
}

void C_OcclusionQuery::End()
{
#ifndef DEVICE_DISABLE_OCCLUSION_QUERIES
	if(m_pd3dQuery)
	{
		m_pd3dQuery->Issue(D3DISSUE_END);
	}
#endif // DEVICE_DISABLE_OCCLUSION_QUERIES
}

}; // namespace D3D9
}; // namespace Display
