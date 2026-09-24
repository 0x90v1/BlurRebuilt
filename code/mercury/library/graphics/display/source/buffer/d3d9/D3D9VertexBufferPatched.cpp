#include "stdafx.h"
#include "Display/Buffer/D3D9/D3D9VertexBufferPatched.h"
#include "Display/Device/Device.h"

namespace Display
{
namespace D3D9
{

C_VertexBufferPatched::C_VertexBufferPatched() :
	m_pVertexBuffer(NULL),
	m_VertexStride(0),
	m_pVertexDeclaration(NULL)
{
}

C_VertexBufferPatched::~C_VertexBufferPatched()
{
}

void C_VertexBufferPatched::SetCommandBuffer() const
{
	Device()->SetVertexDeclaration(m_pVertexDeclaration);
	Device()->SetVertexBuffer(0, m_pVertexBuffer, 0, m_VertexStride);
}

bool C_VertexBufferPatched::CreateCommandBuffer(const U32 vertexStride, C_VertexDeclaration* pVertexDeclaration)
{
	m_VertexStride = vertexStride;
	m_pVertexDeclaration = pVertexDeclaration;

	return true;
}

void C_VertexBufferPatched::DestroyCommandBuffer()
{
}

}; // namespace D3D9
}; // namespace Display
