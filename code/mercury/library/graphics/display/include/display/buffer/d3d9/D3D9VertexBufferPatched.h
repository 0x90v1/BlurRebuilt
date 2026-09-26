#ifndef DISPLAY_BUFFER_D3D9_VERTEX_BUFFER_PATCHED_H
#define DISPLAY_BUFFER_D3D9_VERTEX_BUFFER_PATCHED_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Buffer/D3D9/D3D9VertexBuffer.h"
#include "Display/Buffer/D3D9/D3D9VertexDeclaration.h"

namespace Display
{
namespace D3D9
{

//! C_VertexBufferPatched

class C_VertexBufferPatched : public C_VertexBuffer
{
public:

	C_VertexBufferPatched();
	~C_VertexBufferPatched();

	const bool IsCommandBufferCreated() const;
	
	void SetCommandBuffer() const;
	
	bool CreateCommandBuffer(const U32 vertexStride, C_VertexDeclaration* pVertexDeclaration);
	void DestroyCommandBuffer();

public:

	Display::C_VertexBuffer*	m_pVertexBuffer;

private:

	U32							m_VertexStride;
	C_VertexDeclaration*		m_pVertexDeclaration;
};

}; // namespace D3D9
}; // namespace Display

#include "Display/Buffer/D3D9/D3D9VertexBufferPatched.inl"

#endif // DISPLAY_BUFFER_D3D9_VERTEX_BUFFER_PATCHED_H
