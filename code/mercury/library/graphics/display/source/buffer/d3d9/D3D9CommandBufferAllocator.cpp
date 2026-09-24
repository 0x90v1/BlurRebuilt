#include "stdafx.h"

#include <Btl/Containers/Vector.h>
#include <core/Threads/Atomic.h>
#include "display/Buffer/D3D9/D3D9CommandBufferAllocator.h"
#include "system\memory\MemoryTracker.h"
#include <display.h>
#include <Btl/Containers/Vector.h>
#include <core/Threads/Atomic.h>
#include "display/memory/d3d9/D3D9RingBuffer.h"
#include "display/memory/RingBufferAllocator.h"
#include <system/memory/MemoryTracker.h>
#include <system/time/Time.h>

namespace Display
{

namespace CommandBufferAllocator
{

//--------------------------------------------------------------------------------------
// Globals for our own memory allocator used by CreateGrowableCommandBuffer.
//--------------------------------------------------------------------------------------
#ifdef CFG_DEBUG
static const DWORD g_dwCommandBufferSize = 5 * 1024 * 1024;
#elif defined(CFG_RELEASE)
static const DWORD g_dwCommandBufferSize = 5 * 1024 * 1024;
#else
static const DWORD g_dwCommandBufferSize = 5 * 1024 * 1024;
#endif
char* g_pAllocateChunk[2];
char* g_pAllocateCurrent[2];
DWORD g_dwAllocateIndex;

Btl::Vector<IDirect3DCommandBuffer9*, Btl::VectorParameters_Array<64> > m_CommandBuffers[2];
int m_nCommandBuffers[2] = {0};
int m_iCurrentFrame = 1;

Debug::Logger &g_Log = Debug::GetLogger("graphics.display.cballocator");

int GetUsedSize()
{
	return g_pAllocateCurrent[g_dwAllocateIndex] - g_pAllocateChunk[g_dwAllocateIndex];
}

//--------------------------------------------------------------------------------------
// Name: AllocateCallback()
// Desc: This routine is called by D3D to allocate more physical memory for a growable 
//       command buffer.
//--------------------------------------------------------------------------------------
void* AllocateCallback( DWORD dwContext, DWORD dwFlags, DWORD* pdwSize, DWORD dwAlignment )
{
    // NOTE: It's very easy to screw up the implementation of growable
    //       buffer allocation routines.  If you accidentally give bad
    //       results, you can see all kinds of random, impossible-to-debug
    //       GPU hangs.  Be very careful!  As a result I'd recommend keeping
    //       #ifdef'd code that can use non-growable command buffers if
    //       you start seeing random GPU hangs - this can rule out problems
    //       with your growable buffer allocator.
    // 
    // The allocation convention we choose to employ here is that each growable 
    // command buffer used in a frame is allocated contiguously from one
    // chunk of write-combined memory.  We sequentially parcel out memory from 
    // that underlying chunk as the allocation requests come in from D3D.  The 
    // benefit  from growable buffers using one underlying chunk is that it's 
    // load balancing - if one command buffer ends up using less memory, 
    // another command buffer can use more.  By default D3D will ask us to
    // allocate in requests of 32K (this value can be changed via a parameter
    // to CreateGrowableCommandBuffer).  
    //
    // We also need to employ a double-buffering scheme so that the GPU can be
    // rendering one frame's command buffers while the CPU is building the
    // next frame's command buffers.  To accomplish this we employ two separate
    // buffers and 'dwContext' is 0 or 1 on alternating frames.  We could
    // have just as easily set it up so that 'dwContext' was a pointer to some
    // private heap information.
    //
    // By default D3D guarantees that it won't let the GPU get more than 1 
    // frame behind the CPU, so we only need to double-buffer the command
    // buffer chunks (and not triple or quad buffer).
    DWORD dwIndex = m_iCurrentFrame;

	MEMORY_TRACKER_CONTEXT("graphics.display.device");

    if( dwIndex != g_dwAllocateIndex )
    {
        g_pAllocateCurrent[dwIndex] = NULL; 
        g_dwAllocateIndex = dwIndex;
    }

    if( g_pAllocateChunk[dwIndex] == NULL )
    {
        g_pAllocateChunk[dwIndex] = (char*) BizPhysicalAlloc( g_dwCommandBufferSize, 
                                                            MAXULONG_PTR, 
                                                            0, 
                                                            PAGE_READWRITE | PAGE_WRITECOMBINE );
        if( g_pAllocateChunk[dwIndex] == NULL )
        {
            LOGFATAL( g_Log, "Could not create command buffer memory." );
        }
    }

    // If this is the first allocation for a frame, reset our current pointer.
    if( g_pAllocateCurrent[dwIndex] == NULL )
        g_pAllocateCurrent[dwIndex] = g_pAllocateChunk[dwIndex];

    // We have to honor the requested alignment.
    char* pAllocation = (char*) XGNextMultiple( (DWORD) g_pAllocateCurrent[dwIndex], dwAlignment );

    // Figure out where the next allocation will go, taking into account
    // the size of the current request.
    g_pAllocateCurrent[dwIndex] = pAllocation + *pdwSize;

    // If we've run out of memory, return NULL.  D3D won't crash but will return
    // failure at EndCommandBuffer time.
    if( g_pAllocateCurrent[dwIndex] > g_pAllocateChunk[dwIndex] + g_dwCommandBufferSize )
    {
        BIZ_ASSERT_FAIL("OUT OF COMMAND BUFFER MEMORY (TOO MUCH STUFF BEING DRAWN)");
        return NULL;
    }

    return pAllocation;
}


//--------------------------------------------------------------------------------------
// Name: FreeCallback()
// Desc: This routine is called by D3D to free physical memory for a growable command
//       buffer.
//--------------------------------------------------------------------------------------
void FreeCallback( DWORD dwContext )
{
    // Our AllocateCallback implementation above ping-pongs between memory 
    // buffers on  every frame and thereby takes care of memory re-use.
    // Consequently we don't have to do anything in this free routine.
    //
    // However, if we employed a more sophisticated allocator we could free
    // all memory associated with the command buffer here.  I'd recommend
    // the nice and simple approach above, however.
}


//--------------------------------------------------------------------------------------
// Name: QueryCallback()
// Desc: This routine is called by D3D to determine how much memory is available for
//       a growable command buffer.  It's only called when game code calls the
//       QueryBufferSpace() D3D API.
//--------------------------------------------------------------------------------------
void QueryCallback( DWORD dwContext, DWORD* pdwUsed, DWORD* pdwRemaining)
{
    *pdwUsed = g_pAllocateCurrent[dwContext] - g_pAllocateChunk[dwContext];
    *pdwRemaining = g_pAllocateChunk[dwContext] + g_dwCommandBufferSize - g_pAllocateCurrent[dwContext];
}

/// this function needs to be called every frame to clean up command buffers that were used the previous frame
void EndFrame()
{
	int nK = GetUsedSize()/1024;
	if (nK > 1024*6)
		LOGWARN( g_Log, "Used %d commandbuffers and %dK of memory\n", m_nCommandBuffers[m_iCurrentFrame], nK);
	else
		LOGDEBUG( g_Log, "Used %d commandbuffers and %dK of memory\n", m_nCommandBuffers[m_iCurrentFrame], nK);

	m_iCurrentFrame ^= 1;
	
	for (int i=0; i<m_nCommandBuffers[m_iCurrentFrame]; i++)
		m_CommandBuffers[m_iCurrentFrame][i]->Release();

	m_nCommandBuffers[m_iCurrentFrame] = 0;
	g_pAllocateCurrent[m_iCurrentFrame] = g_pAllocateChunk[m_iCurrentFrame]; 
}

void AddCommandBuffer(IDirect3DCommandBuffer9 *p)
{
	int x = MT::AtomicIncrement(&m_nCommandBuffers[m_iCurrentFrame]);
	BIZ_ASSERT(x < m_CommandBuffers[m_iCurrentFrame].Size());
	m_CommandBuffers[m_iCurrentFrame][x] = p;
}

void EndTiling()
{
}

}	// namespace

C_RingBuffer::C_RingBuffer() {}

void C_RingBuffer::Initialise(U32 c)
{
}

U32 C_RingBuffer::GetUsedSize()
{
	return 5*1024*1024;
}

C_RingBuffer g_RingBuffer;

void C_RingBuffer::EmitFence()
{
}

void C_RingBuffer::Shutdown()
{
}

void C_RingBuffer::Free(const Display::C_MemorySet *)
{
}

void C_RingBuffer::EndTiling()
{
}

void C_RingBuffer::EndFrame()
{
	Display::CommandBufferAllocator::EndFrame();
}

void C_RingBuffer::CreateCommandBuffer(D3D9::C_CommandBuffer *pCommandBuffer)
{
	pCommandBuffer->m_UsedMemory.m_Set.Clear();

#if PLATFORM_XENON
	BIZ_ASSERT(!(pCommandBuffer->m_bOwnCommandBuffer && pCommandBuffer->m_pd3dCommandBuffer));
	Display::Device()->GetD3DDevice()->CreateGrowableCommandBuffer(
	  0, 
	  Display::CommandBufferAllocator::AllocateCallback,
	  Display::CommandBufferAllocator::FreeCallback,
	  Display::CommandBufferAllocator::QueryCallback,
	  (DWORD)&pCommandBuffer->m_UsedMemory,
	  0,
	  &pCommandBuffer->m_pd3dCommandBuffer
	);
	assert(pCommandBuffer->m_pd3dCommandBuffer);
	pCommandBuffer->m_bOwnCommandBuffer = false;
	pCommandBuffer->m_bEmptyCommandBuffer = false;
#endif

	::Display::CommandBufferAllocator::AddCommandBuffer(pCommandBuffer->GetD3DCommandBuffer());
}


}	// namespace

