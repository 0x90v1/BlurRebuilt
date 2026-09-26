#ifndef DISPLAY_OCCLUSION_D3D9_D3D9_OCCLUSION_QUERY_H
#define DISPLAY_OCCLUSION_D3D9_D3D9_OCCLUSION_QUERY_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Occlusion/OcclusionQueryTypes.h"

namespace Display
{
namespace D3D9
{

//! C_OcclusionQuery

class C_OcclusionQuery
{
public:

	C_OcclusionQuery();
	~C_OcclusionQuery();

	const bool IsCreated() const;

	bool Create();
	void Destroy();

	void Begin();
	void End();

	const E_OcclusionQueryResult Visible() const;
	const E_OcclusionQueryResult Visible(U32& visible) const;
	const E_OcclusionQueryResult Visible(F32& visible) const;

protected:

	IDirect3DQuery9* m_pd3dQuery;
};

}; // namespace D3D9
}; // namespace Display

#include "Display/Occlusion/D3D9/D3D9OcclusionQuery.inl"

#endif // DISPLAY_OCCLUSION_D3D9_D3D9_OCCLUSION_QUERY_H
