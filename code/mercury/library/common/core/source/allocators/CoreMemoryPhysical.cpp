#include "CorePch.h"
#include <core/allocators/CoreMemoryFunctions.h>

void* BizPhysicalAlloc(size_t dwSize, U32 ulPhysicalAddress, U32 ulAlignment, U32 flProtect)
{
#ifdef PLATFORM_XENON
	return XPhysicalAlloc(dwSize, ulPhysicalAddress, ulAlignment, flProtect);
#else
	return NULL;
#endif
}

void BizPhysicalFree(void* lpAddress)
{
#ifdef PLATFORM_XENON
	XPhysicalFree(lpAddress);
#endif
}

void* BizHeapPhysicalAlloc(size_t dwSize, U32 ulPhysicalAddress, U32 ulAlignment, U32 flProtect)
{
#ifdef PLATFORM_XENON
	return XPhysicalAlloc(dwSize, ulPhysicalAddress, ulAlignment, flProtect);
#else
	return NULL;
#endif
}

void BizHeapPhysicalFree(void* lpAddress)
{
#ifdef PLATFORM_XENON
	XPhysicalFree(lpAddress);
#endif
}
