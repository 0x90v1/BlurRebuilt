#include "stdafx.h"
#include "Display/Device/Device.h"
#include "Display/Buffer/D3D9/D3D9CommandBuffer.h"
#include "display\memory/D3d9/D3D9RingBuffer.h"

namespace Display
{
namespace D3D9
{

C_CommandBuffer::C_CommandBuffer() :
	m_Size(0),
	m_pd3dCommandBuffer(NULL),
	m_bOwnCommandBuffer(false),
	m_bEmptyCommandBuffer(false)
{
}

C_CommandBuffer::~C_CommandBuffer()
{
	assert(!(m_bOwnCommandBuffer && m_pd3dCommandBuffer));
}

/// create a new commandbuffer of a given size
void C_CommandBuffer::Create(const U32 size)
{
#if PLATFORM_XENON
	assert(!(m_bOwnCommandBuffer && m_pd3dCommandBuffer));
	Display::Device()->GetD3DDevice()->CreateCommandBuffer(
	  size,
	  0,
	  &m_pd3dCommandBuffer
	);
	assert(m_pd3dCommandBuffer);
	m_bOwnCommandBuffer = true;
	m_bEmptyCommandBuffer = false;
#endif
}

/// create a new growable commandbuffer
void C_CommandBuffer::Create()
{
}

void C_CommandBuffer::Destroy()
{
#if PLATFORM_XENON
	assert(m_pd3dCommandBuffer && m_bOwnCommandBuffer);
	m_pd3dCommandBuffer->Release();
#endif
}

void C_CommandBuffer::Run()
{
#if PLATFORM_XENON
	assert(m_pd3dCommandBuffer);
	if (!m_bEmptyCommandBuffer)
		Display::Device()->GetD3DDevice()->RunCommandBuffer(m_pd3dCommandBuffer, 0);

	Display::RingBuffer()->Free(&m_UsedMemory);
	Display::RingBuffer()->EmitFence();
#endif
}

}; // namespace D3D9
}; // namespace Display
