#ifndef __D3D9RINGBUFFER_H__
#define __D3D9RINGBUFFER_H__

#if defined(BLUR_USE_DX11) // [Blur1PCDX11] Legacy include path, native PC type.
#include "Display/Memory/D3D11/D3D11RingBuffer.h"
#else

#include "display/memory/RingBufferAllocator.h"

namespace Display
{

namespace D3D9
{
class C_CommandBuffer;
}

class C_MemorySet
{
	friend class C_RingBuffer;
public:
	/// clear the contents of the set
	void Clear();
private:
	RingBufferAllocator::MemorySet m_Set;
};

/// global graphics ringbuffer
class C_RingBuffer
{
public:
	/// constructor
	C_RingBuffer();

	/// initialise ringbuffer
	void Initialise(U32 cSize);

	/// allocate some memory
	void *Alloc(U32 cSize, C_MemorySet *pSet);

	/// free a memory set
	void Free(const C_MemorySet *pSet);

	/// free ringbuffer
	void Shutdown();

	/// returns the amount of memory currently used
	U32 GetUsedSize();

	/// called once at the end of each frame
	void EndFrame();

	/// 360 specific function that allocates a commandbuffer and automatically frees in after the next frame
	void CreateCommandBuffer(D3D9::C_CommandBuffer *p);

	/// 360 specific: end tiling. this emits a fence assuming the ringbuffer is initialised
	void EndTiling();

	/// emit a fence
	void EmitFence();

private:
	/// reset all internal variables
	void Reset();

	/// wait on a fence and update the address of where the GPU is up to
	void FenceRead();

	Display::RingBufferAllocator::Allocator m_Allocator;
	U8 *m_pBuffer;

#if PLATFORM_XENON
	Btl::Vector<IDirect3DCommandBuffer9*, Btl::VectorParameters_Array<64> > m_CommandBuffers[2];
	int m_nCommandBuffers[2];
#endif
	int m_iCurrentFrame;
	U32 m_iFreeAddr;
	U32 m_iLastFence;
	U32 m_cSize;

	#define NUM_FENCES 256
	struct FenceItem
	{
		DWORD handle;
		U32 iAddr;
	};

	FenceItem m_Fences[NUM_FENCES];
	U32 m_iFenceRead;
	U32 m_iFenceWrite;
	U32 m_iUsedSize;
	U32 m_iLastAllocPtr;
};

extern C_RingBuffer g_RingBuffer;

/// get pointer to global graphics ringbuffer
inline C_RingBuffer *RingBuffer()
{
	return &g_RingBuffer;
}

}

#endif // BLUR_USE_DX11
#endif // __D3D9RINGBUFFER_H__
