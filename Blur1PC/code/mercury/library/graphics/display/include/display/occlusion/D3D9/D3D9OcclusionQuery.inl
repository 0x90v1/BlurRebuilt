#ifndef DISPLAY_OCCLUSION_D3D9_D3D9_OCCLUSION_QUERY_INL
#define DISPLAY_OCCLUSION_D3D9_D3D9_OCCLUSION_QUERY_INL

#ifdef _MSC_VER
#pragma once
#endif

namespace Display
{
namespace D3D9
{

INLINE const bool C_OcclusionQuery::IsCreated() const
{
	return m_pd3dQuery ? true : false;
}

INLINE const E_OcclusionQueryResult C_OcclusionQuery::Visible() const
{
#ifdef DEVICE_DISABLE_OCCLUSION_QUERIES
	return E_OcclusionQueryResult_Visible;
#else // DEVICE_DISABLE_OCCLUSION_QUERIES
	DWORD visiblePixelCount = 0;
	if(m_pd3dQuery)
	{
		const HRESULT hr = m_pd3dQuery->GetData(&visiblePixelCount, sizeof(DWORD), 0);
		if(hr == S_OK)
		{
			return visiblePixelCount == 0 ? E_OcclusionQueryResult_NotVisible : E_OcclusionQueryResult_Visible;
		}
		else
		{
			return E_OcclusionQueryResult_Failed;
		}
	}
	else
	{
		return E_OcclusionQueryResult_NotVisible;
	}
#endif // DEVICE_DISABLE_OCCLUSION_QUERIES
}

INLINE const E_OcclusionQueryResult C_OcclusionQuery::Visible(U32& visible) const
{
#ifdef DEVICE_DISABLE_OCCLUSION_QUERIES
	return E_OcclusionQueryResult_Visible;
#else // DEVICE_DISABLE_OCCLUSION_QUERIES
	if(m_pd3dQuery)
	{
		const HRESULT hr = m_pd3dQuery->GetData(&visible, sizeof(DWORD), 0);
		if(hr == S_OK)
		{
			return visible == 0 ? E_OcclusionQueryResult_NotVisible : E_OcclusionQueryResult_Visible;
		}
		else
		{
			return E_OcclusionQueryResult_Failed;
		}
	}
	else
	{
		return E_OcclusionQueryResult_NotVisible;
	}
#endif // DEVICE_DISABLE_OCCLUSION_QUERIES
}

INLINE const E_OcclusionQueryResult C_OcclusionQuery::Visible(F32& visible) const
{
#ifdef DEVICE_DISABLE_OCCLUSION_QUERIES
	return E_OcclusionQueryResult_Visible;
#else // DEVICE_DISABLE_OCCLUSION_QUERIES
#ifdef PLATFORM_PC
	const E_OcclusionQueryResult result = Visible();
	visible = result == E_OcclusionQueryResult_Visible ? 1.f : 0.f;
	return result;
#else // PLATFORM_PC
	assert(m_pd3dQuery);
    D3DOCCLUSIONSTATS occlusionStats;
	const HRESULT hr = m_pd3dQuery->GetData(&occlusionStats, sizeof(D3DOCCLUSIONSTATS), 0);
	if(hr == S_OK)
	{
		if(occlusionStats.ZPass + occlusionStats.ZFail > 0)
		{
			visible	= F32(occlusionStats.ZPass) / F32(occlusionStats.ZPass + occlusionStats.ZFail);
			return visible == 0.f ? E_OcclusionQueryResult_NotVisible : E_OcclusionQueryResult_Visible;
		}
		else
		{
			visible	= 0.0f;
			return E_OcclusionQueryResult_NotVisible;
		}
	}
	else
	{
		visible	= 0.0f;
		return E_OcclusionQueryResult_Failed;
	}
#endif // PLATFORM_PC
#endif // DEVICE_DISABLE_OCCLUSION_QUERIES
}

}; // namespace D3D9
}; // namespace Display

#endif // DISPLAY_OCCLUSION_D3D9_D3D9_OCCLUSION_QUERY_INL
