#include "stdafx.h"

#include <display.h>
#include <Btl/Containers/Vector.h>
#include <core/Threads/Atomic.h>
#include "display/memory/d3d9/D3D9RingBuffer.h"
#include "display/memory/RingBufferAllocator.h"
#include <system/memory/MemoryTracker.h>
#include <system/time/Time.h>

namespace Display
{

#if PLATFORM_PC
/// constructor
C_RingBuffer::C_RingBuffer()
{
}

/// allocate memory for ringbuffer
void C_RingBuffer::Initialise(U32 cSize)
{
}

/// free memory for ringbuffer
void C_RingBuffer::Shutdown()
{
}

void *C_RingBuffer::Alloc(U32 cSize, C_MemorySet *pSet)
{
	BIZ_ASSERT(0);
	return NULL;
}

/// clear the contents of the set
void C_MemorySet::Clear()
{
	m_Set.Clear();
}

/// this function needs to be called every frame to clean up command buffers that were used the previous frame
void C_RingBuffer::EndFrame()
{
}

/// free memory
void C_RingBuffer::Free(const C_MemorySet *pSet)
{
}

#endif

#if PLATFORM_XENON

/// constructor
C_RingBuffer::C_RingBuffer()
{
	Reset();
};

/// allocate memory for ringbuffer
void C_RingBuffer::Initialise(U32 cSize)
{
	BIZ_ASSERT(!m_pBuffer);
	m_cSize = cSize;

	MEMORY_TRACKER_PUSH("graphics.display.ringbuffer");
	m_pBuffer = (U8*)BizPhysicalAlloc( cSize, MAXULONG_PTR, 0, PAGE_READWRITE | PAGE_WRITECOMBINE );
	m_Allocator.Initialise(cSize);
	MEMORY_TRACKER_POP("graphics.display.ringbuffer");
}

/// free memory for ringbuffer
void C_RingBuffer::Shutdown()
{
	BizPhysicalFree(m_pBuffer);
	Reset();
}

/// reset all internal variables
void C_RingBuffer::Reset()
{
	m_pBuffer = NULL;

	m_nCommandBuffers[0] = m_nCommandBuffers[1] = 0;
	m_iCurrentFrame = 0;
	m_iFreeAddr = 0;
	m_iLastFence = -1;

	for (int i=0; i<NUM_FENCES; i++)
		m_Fences[i].handle = m_Fences[i].iAddr = 0;

	m_iFenceRead = 0;
	m_iFenceWrite = 0;
	m_iUsedSize = 0;
	m_iLastAllocPtr = 0;
	m_cSize = 0;
}

void *C_RingBuffer::Alloc(U32 cSize, C_MemorySet *pSet)
{
	BIZ_ASSERT(m_pBuffer);

	// get allocation
	U32 iAddr = m_Allocator.Allocate(cSize, &pSet->m_Set);

	// check whether the GPU has finished with this bit of memory
	while (1)
	{
		U32 iFreeAddr = MT::AtomicGet(&m_iFreeAddr);
		U32 iWrappedAddr = iAddr + cSize;
		if (iFreeAddr > iAddr)
			iWrappedAddr += 4*m_cSize;
		BIZ_ASSERT(iFreeAddr <= iWrappedAddr);
		if ((iFreeAddr + m_cSize) - iWrappedAddr<0x80000000)
			break;

		FenceRead();
	}

	return m_pBuffer + (iAddr%m_cSize);
}

/// wait on a fence and update the address of where the GPU is up to
void C_RingBuffer::FenceRead()
{
	U32 iFenceRead = MT::AtomicGet(&m_iFenceRead);
	U32 iOrgFenceRead = iFenceRead;
	FenceItem *p = &m_Fences[iFenceRead];
	if (iFenceRead == m_iFenceWrite)
	{
		DebugBreak();
		return;
	}

	// *** IF THE CPU IS HUNG HERE IT MEANS WE RAN OUT OF GRAPHICS RINGBUFFER SPACE ***
	U64 t = Time::GetPerformanceCounter();
	Display::Device()->GetD3DDevice()->BlockOnFence(p->handle);
	t = Time::GetPerformanceCounter() - t;
	// *** IF THE CPU IS HUNG HERE IT MEANS WE RAN OUT OF GRAPHICS RINGBUFFER SPACE ***

	if (t > 1000)
		LOGWARN( g_Log, "GRAPHICS RINGBUFFER MEMORY LOW (wait time %d)\n", (int)t);

	iFenceRead++;
	if (iFenceRead >= NUM_FENCES)
		iFenceRead = 0;
	if (MT::AtomicCompareAndSwap(&m_iFenceRead, iFenceRead, iOrgFenceRead))
	{
		// not entirely thread-safe but worst case we temporarily miss a tiny bit of memory
		p->handle = NULL;
		m_iFreeAddr = p->iAddr;
	}
}

/// emit a fence
void C_RingBuffer::EmitFence()
{
	if (Display::Device()->IsTilingActive())
		return;
		
	U32 iAddr = m_Allocator.GetFreeMemPtr(); 
	if (iAddr == m_iLastFence)
		return;
	m_iLastFence = iAddr;
	FenceItem *p = &m_Fences[m_iFenceWrite];
	if (p->handle != NULL)
		FenceRead();
	m_iFenceWrite++;
	if (m_iFenceWrite>=NUM_FENCES)
		m_iFenceWrite = 0;
	BIZ_ASSERT(m_iFenceWrite != m_iFenceRead);

	p->iAddr = iAddr;
	p->handle = Display::Device()->GetD3DDevice()->InsertFence();
}

/// end tiling. this emits a fence assuming the ringbuffer is initialised
void C_RingBuffer::EndTiling()
{
	if (m_pBuffer)
		EmitFence();
}

/// this function needs to be called every frame to clean up command buffers that were used the previous frame
void C_RingBuffer::EndFrame()
{
	if (m_pBuffer)
		EmitFence();

	/// do a bunch of fence reads until we've caught up with the GPU
	/// this is simply to make sure we don't run out of fences when very little rendering is happening
	while (1)
	{
		U32 iFenceRead = MT::AtomicGet(&m_iFenceRead);
		U32 iOrgFenceRead = iFenceRead;
		FenceItem *p = &m_Fences[iFenceRead];
		if (iFenceRead == m_iFenceWrite)
			break;

		if (Display::Device()->GetD3DDevice()->IsFencePending(p->handle))
			break;

		iFenceRead++;
		if (iFenceRead >= NUM_FENCES)
			iFenceRead = 0;
		if (MT::AtomicCompareAndSwap(&m_iFenceRead, iFenceRead, iOrgFenceRead))
		{
			// not entirely thread-safe but worst case we temporarily miss a tiny bit of memory
			p->handle = NULL;
			m_iFreeAddr = p->iAddr;
		}
	}

	U32 x = m_Allocator.GetAllocPtr();
	m_iUsedSize = x - m_iLastAllocPtr;
	m_iLastAllocPtr = x;
	if (m_iUsedSize > 0x80000000)
		m_iUsedSize += 4*m_cSize;

	m_iCurrentFrame ^= 1;
	
	for (int i=0; i<m_nCommandBuffers[m_iCurrentFrame]; i++)
		m_CommandBuffers[m_iCurrentFrame][i]->Release();

	m_nCommandBuffers[m_iCurrentFrame] = 0;
}

/// free memory
void C_RingBuffer::Free(const C_MemorySet *pSet)
{
	m_Allocator.Free(&pSet->m_Set);
}

/// D3D9 growable commandbuffer callbacks
void* AllocateCallback( DWORD dwContext, DWORD dwFlags, DWORD* pdwSize, DWORD dwAlignment )
{
	BIZ_ASSERT(dwAlignment<=128);
	return g_RingBuffer.Alloc(*pdwSize, (Display::C_MemorySet*)dwContext);
}

void FreeCallback( DWORD dwContext )
{
}

void QueryCallback( DWORD dwContext, DWORD* pdwUsed, DWORD* pdwRemaining)
{
	BIZ_ASSERT(0);
    *pdwUsed = 0;
    *pdwRemaining = 0;
}

/// 360 specific function that allocates a commandbuffer and automatically frees in after the next frame
void C_RingBuffer::CreateCommandBuffer(D3D9::C_CommandBuffer *pCommandBuffer)
{
	pCommandBuffer->m_UsedMemory.m_Set.Clear();

#if PLATFORM_XENON
	BIZ_ASSERT(!(pCommandBuffer->m_bOwnCommandBuffer && pCommandBuffer->m_pd3dCommandBuffer));
	Display::Device()->GetD3DDevice()->CreateGrowableCommandBuffer(
	  0, 
	  AllocateCallback,
	  FreeCallback,
	  QueryCallback,
	  (DWORD)&pCommandBuffer->m_UsedMemory,
	  0,
	  &pCommandBuffer->m_pd3dCommandBuffer
	);
	assert(pCommandBuffer->m_pd3dCommandBuffer);
	pCommandBuffer->m_bOwnCommandBuffer = false;
	pCommandBuffer->m_bEmptyCommandBuffer = false;
#endif

	int x = MT::AtomicIncrement(&m_nCommandBuffers[m_iCurrentFrame]);
	BIZ_ASSERT(x < m_CommandBuffers[m_iCurrentFrame].Size());
	m_CommandBuffers[m_iCurrentFrame][x] = pCommandBuffer->m_pd3dCommandBuffer;
}

#endif

/// returns the amount of memory currently used
U32 C_RingBuffer::GetUsedSize()
{
	return m_iUsedSize;
}

C_RingBuffer g_RingBuffer;

}	// namespace
