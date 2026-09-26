#include "stdafx.h"
#include "Display/Memory/MemoryLinearBuffer.h"

namespace Display
{

C_LinearBuffer::C_LinearBuffer()
{
#if defined(PLATFORM_PS3)
	m_eHeap		= E_AllocHeap_Video;
#elif defined(PLATFORM_XENON)
	m_eHeap		= E_AllocHeap_Large;
#endif
	m_pBase		= 0;
	m_pTop		= 0;
	m_pPosition	= 0;
}

C_LinearBuffer::~C_LinearBuffer()
{
	Destroy();
}

bool C_LinearBuffer::Create(const U32 size, const U32 alignment, const E_AllocHeap eHeap)
{
	if (!Device()->AllocMemory(alignment, size, &m_pBase, eHeap, E_AllocType_Start))
		return false;

	m_eHeap		= eHeap;
	m_pTop		= m_pBase + size;
	m_pPosition	= m_pBase;

	return true;
}

void C_LinearBuffer::Destroy()
{
	if (m_pBase)
	{
		Device()->FreeMemory(m_pTop - m_pBase, m_pBase, m_eHeap);
		Free();
	}
}

void* C_LinearBuffer::Allocate(const U32 size, const U32 alignment)
{
	U32 allocSize	= size + alignment;
	U8* pAllocPos	= m_pPosition;

	// check if there is enough space
	U8* pAllocTop = pAllocPos + allocSize;
	if (pAllocTop > m_pTop)
		return 0;

	const U32 alignmentMask = alignment - 1;
	pAllocPos = (U8*)((U32)(pAllocPos + alignmentMask) & ~alignmentMask);

	m_pPosition	= pAllocTop;

	return pAllocPos;
}

void C_LinearBuffer::Free()
{
	m_pPosition		= m_pBase;
}

U32 C_LinearBuffer::Used() const
{
	return (U32)(m_pPosition - m_pBase);
}

}; // namespace Display
