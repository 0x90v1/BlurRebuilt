#ifndef DISPLAY_BUFFER_D3D9_VERTEX_BUFFER_PATCHED_INL
#define DISPLAY_BUFFER_D3D9_VERTEX_BUFFER_PATCHED_INL

#ifdef _MSC_VER
#pragma once
#endif

namespace Display
{
namespace D3D9
{

INLINE const bool C_VertexBufferPatched::IsCommandBufferCreated() const
{
	return m_VertexStride > 0 && m_pVertexDeclaration ? true : false;
}

}; // namespace D3D9
}; // namespace Display

#endif // DISPLAY_BUFFER_D3D9_VERTEX_BUFFER_PATCHED_INL
