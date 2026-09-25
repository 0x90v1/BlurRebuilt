#include "stdafx.h"

#include <Btl/Containers/Vector.h>
#include <core/Threads/Atomic.h>
#include <System/memory/MemoryTracker.h>
#include <System/time/time.h>
#include "display/device/Device.h"
#include "display/Buffer/D3D9/D3D9CommandBufferAllocator.h"
#include "display/memory/CommandBufferAllocator.h"

namespace Display
{

namespace CommandBufferAllocator
{

#define GRAPHICS_RINGBUFFER_SIZE (4*1024*1024)	// 8MB is the maximum size supported

//#define GRAPHICS_RINGBUFFER_SIZE (256*1024)
/*
#define DEBUG_CODE
#undef optimize
#pragma optimize("", off)
*/
//#undef BIZ_ASSERT
//#define BIZ_ASSERT(X) if (!(X)) DebugBreak();


Display::RingBuffer::Allocator g_Allocator;
U8 *g_pBuffer = NULL;

Btl::Vector<IDirect3DCommandBuffer9*, Btl::VectorParameters_Array<64> > m_CommandBuffers[2];
int m_nCommandBuffers[2] = {0};
int m_iCurrentFrame = 1;
U32 g_iFreeAddr = 0;
U32 g_iLastFence = -1;


MT::Lock g_Lock;

void FenceRead();

#define NUM_FENCES 128
struct FenceItem
{
	DWORD handle;
	U32 iAddr;
};

FenceItem g_Fences[NUM_FENCES] = {0};
U32 g_iFenceRead = 0;
U32 g_iFenceWrite = 0;
U32 g_iUsedSize = 0;
U32 g_iLastAllocPtr = 0;

#ifdef DEBUG_CODE
U32 g_iAllocWrite = 0;
#define NUM_ALLOCS 128

U32 g_LastAllocs[NUM_ALLOCS];
U32 g_LastAllocSizes[NUM_ALLOCS];
U32 g_LastAllocFree[NUM_ALLOCS];
U32 g_LastAllocFree2[NUM_ALLOCS];
#endif

Debug::Logger &g_Log = Debug::GetLogger("graphics.display.cballocator");

void* AllocateCallback( DWORD dwContext, DWORD dwFlags, DWORD* pdwSize, DWORD dwAlignment )
{
//	g_Lock.Enter();

	BIZ_ASSERT(dwAlignment<=128);

	if (!g_pBuffer)
	{
		MEMORY_TRACKER_PUSH("graphics.display.ringbuffer");
		g_pBuffer = (U8*)BizPhysicalAlloc( GRAPHICS_RINGBUFFER_SIZE, MAXULONG_PTR, 0, PAGE_READWRITE | PAGE_WRITECOMBINE );
		g_Allocator.Initialise(g_pBuffer, GRAPHICS_RINGBUFFER_SIZE);
		MEMORY_TRACKER_POP("graphics.display.ringbuffer");
	}

	// get allocation
	U32 iAddr = g_Allocator.Allocate(*pdwSize, (Display::RingBuffer::MemorySet*)dwContext);

	// for debugging
#ifdef DEBUG_CODE
	g_LastAllocs[g_iAllocWrite] = iAddr;
	g_LastAllocSizes[g_iAllocWrite] = *pdwSize;
	g_LastAllocFree[g_iAllocWrite] = g_iFreeAddr;
#endif

	// check whether the GPU has finished with this bit of memory
	while (1)
	{
		U32 iFreeAddr = MT::AtomicGet(&g_iFreeAddr);
		U32 iWrappedAddr = iAddr + *pdwSize;
		if (iFreeAddr > iAddr)
			iWrappedAddr += 4*GRAPHICS_RINGBUFFER_SIZE;
		BIZ_ASSERT(iFreeAddr <= iWrappedAddr);
		if ((iFreeAddr + GRAPHICS_RINGBUFFER_SIZE) - iWrappedAddr<0x80000000)
			break;

		FenceRead();
	}


#ifdef DEBUG_CODE
	g_LastAllocFree2[g_iAllocWrite] = g_iFreeAddr;

	g_iAllocWrite++;
	if (g_iAllocWrite == NUM_ALLOCS)
		g_iAllocWrite = 0;

	LOGINFO(g_Log, "alloc [%x-%x]\n", iAddr%GRAPHICS_RINGBUFFER_SIZE, (iAddr%GRAPHICS_RINGBUFFER_SIZE)+*pdwSize);
#endif

//	g_Lock.Exit();

	return g_pBuffer + (iAddr%GRAPHICS_RINGBUFFER_SIZE);
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

void FenceRead()
{
	U32 iFenceRead = MT::AtomicGet(&g_iFenceRead);
	U32 iOrgFenceRead = iFenceRead;
	FenceItem *p = &g_Fences[iFenceRead];
	if (iFenceRead == g_iFenceWrite)
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
	if (MT::AtomicCompareAndSwap(&g_iFenceRead, iFenceRead, iOrgFenceRead))
	{
		// not entirely thread-safe but worst case we temporarily miss a tiny bit of memory
		p->handle = NULL;
#ifdef DEBUG_CODE
		LOGINFO(g_Log, "free [%x-%x]\n", g_iFreeAddr%GRAPHICS_RINGBUFFER_SIZE, p->iAddr%GRAPHICS_RINGBUFFER_SIZE);
#endif
		g_iFreeAddr = p->iAddr;
	}
}

// TODO: make this thread-safe
void EmitFence()
{
	U32 iAddr = g_Allocator.GetFreeMemPtr(); 
	if (iAddr == g_iLastFence)
		return;
	g_iLastFence = iAddr;
	FenceItem *p = &g_Fences[g_iFenceWrite];
	if (p->handle != NULL)
		FenceRead();
	g_iFenceWrite++;
	if (g_iFenceWrite>=NUM_FENCES)
		g_iFenceWrite = 0;
	BIZ_ASSERT(g_iFenceWrite != g_iFenceRead);

	p->iAddr = iAddr;
	p->handle = Display::Device()->GetD3DDevice()->InsertFence();
}

void EndTiling()
{
	if (g_pBuffer)
		EmitFence();
}

/// this function needs to be called every frame to clean up command buffers that were used the previous frame
void EndFrame()
{
//	g_Lock.Enter();

	if (g_pBuffer)
	{
		EmitFence();

#ifdef DEBUG_CODE
		U32 x = g_iFenceWrite;
		x--;
		if (x==0xffffffff)
			x=NUM_FENCES-1;
		LOGINFO( g_Log, "Endframe @ fence %x cur GPU %x\n", g_Fences[x].iAddr, g_Fences[g_iFenceRead].iAddr);

		//g_Allocator.DumpMemoryUsage();
#endif
	}

	U32 x = g_Allocator.GetAllocPtr();
	g_iUsedSize = x - g_iLastAllocPtr;
	g_iLastAllocPtr = x;
	if (g_iUsedSize > 0x80000000)
		g_iUsedSize += 4*GRAPHICS_RINGBUFFER_SIZE;

	m_iCurrentFrame ^= 1;
	
	for (int i=0; i<m_nCommandBuffers[m_iCurrentFrame]; i++)
		m_CommandBuffers[m_iCurrentFrame][i]->Release();

	m_nCommandBuffers[m_iCurrentFrame] = 0;

//	g_Lock.Exit();
}

void FreeMemory(Display::RingBuffer::MemorySet *pSet)
{
//	g_Lock.Enter();

	g_Allocator.Free(pSet);

	if (!Display::Device()->IsTilingActive())
		EmitFence();

//	g_Lock.Exit();
}

void AddCommandBuffer(IDirect3DCommandBuffer9 *p)
{
	int x = MT::AtomicIncrement(&m_nCommandBuffers[m_iCurrentFrame]);
	BIZ_ASSERT(x < m_CommandBuffers[m_iCurrentFrame].Size());
	m_CommandBuffers[m_iCurrentFrame][x] = p;
}

int GetUsedSize()
{
	return g_iUsedSize;
}

}	// namespace

}	// namespace

