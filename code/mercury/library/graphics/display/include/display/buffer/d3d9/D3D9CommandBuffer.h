#ifndef DISPLAY_BUFFER_D3D9_D3D9_COMMAND_BUFFER_H
#define DISPLAY_BUFFER_D3D9_D3D9_COMMAND_BUFFER_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/D3D9/D3D9.h"
#include "Display/Memory/D3D9/D3D9RingBuffer.h"

#if !PLATFORM_XENON
typedef void D3DCommandBuffer;
#endif

namespace Display
{
class C_Context;

namespace D3D9
{

//! Command buffer wrapper for d3d9.

class C_CommandBuffer
{
	friend class C_RingBuffer;
public:

	C_CommandBuffer();
	~C_CommandBuffer();

	const bool IsCreated() const;

	/// create commandbuffer
	void Create(const U32 size);

	/// create resizeable commandbuffer
	void Create();

	/// free the resources associated with this commandbuffer. will stall until the GPU has finished rendering the commmandbuffer
	void Destroy();

	/// to be called before rendering into this commandbuffer
	void Bind() {};

	/// to be called after finished rendering into this commandbuffer
	void Unbind() {};

	/// insert this commandbuffer into the global commandbuffer
	void Run();

	/// set empty command buffer flag
	void SetEmptyCommandBuffer(bool bEmpty) { m_bEmptyCommandBuffer = bEmpty; }

	/// get the D3D command buffer pointer
	D3DCommandBuffer *GetD3DCommandBuffer() { return m_pd3dCommandBuffer; }

protected:
	U32						m_Size;	
	D3DCommandBuffer*		m_pd3dCommandBuffer;
	bool					m_bOwnCommandBuffer;
	bool					m_bEmptyCommandBuffer;
	Display::C_MemorySet	m_UsedMemory;
};

}; // namespace D3D9
}; // namespace Display

#include "Display/Buffer/D3D9/D3D9CommandBuffer.inl"

#endif // DISPLAY_BUFFER_D3D9_D3D9_COMMAND_BUFFER_H
