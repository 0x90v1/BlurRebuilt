#ifndef __DISPLAY_XENONCOMMANDBUFFERALLOCATOR_H__
#define __DISPLAY_XENONCOMMANDBUFFERALLOCATOR_H__

namespace Display
{

//namespace RingBuffer
//{
//	class MemorySet;
//}

namespace CommandBufferAllocator
{

#if PLATFORM_XENON
void* AllocateCallback( DWORD dwContext, DWORD dwFlags, DWORD* pdwSize, DWORD dwAlignment );

void FreeCallback( DWORD dwContext );

void QueryCallback( DWORD dwContext, DWORD* pdwUsed, DWORD* pdwRemaining);

void AddCommandBuffer(IDirect3DCommandBuffer9 *p);

int GetUsedSize();

void EndTiling();

//void FreeMemory(Display::RingBuffer::MemorySet *pSet);

void EndFrame();
#endif

}	// namespace

}	// namespace

#endif
