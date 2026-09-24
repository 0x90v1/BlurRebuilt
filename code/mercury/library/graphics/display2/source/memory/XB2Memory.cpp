#include "StdAfx.h"
#include "Display2/Memory/XB2Memory.h"

namespace Display2
{

// C_MemoryBuffer

C_MemoryBuffer::C_MemoryBuffer() :
	m_Size(0),
	m_Align(0),
	m_Flags(0),
	m_pAddress(NULL),
	m_Type(E_MemoryBuffer_Unknown)
{
}

bool C_MemoryBuffer::Create(const U32 size, const U32 align, const U32 flags, const E_MemoryBuffer type, U8* pAddress)
{
	BIZ_ASSERT(!m_pAddress);
	
	m_Size		= size;
	m_Align		= align;
	m_Flags		= flags;
	m_Type		= type;

	if(m_Type == E_MemoryBuffer_Indirect)
	{
		m_pAddress = pAddress;
		if(!m_pAddress)
		{
			Destroy();
			LOGFATAL(g_Log, "Failed to create indirect memory buffer of size %i\n", m_Size);
			return false;
		}
	}
	else
	{
		m_pAddress = (U8*)BizHeapPhysicalAlloc(m_Size, MAXULONG_PTR, m_Align, m_Flags);
		if(!m_pAddress)
		{
			Destroy();
			LOGFATAL(g_Log, "Failed to create physical memory buffer of size %i\n", m_Size);
			return false;
		}
	}

	return true;
}

void C_MemoryBuffer::Destroy()
{
	if(m_pAddress)
	{
		if(m_Type != E_MemoryBuffer_Indirect)
		{
			BizHeapPhysicalFree(m_pAddress);
		}

		m_pAddress = NULL;
	}
}

bool C_MemoryBuffer::Inside(const U32 size, const U8* pAddress) const
{
	return (pAddress >= m_pAddress) && ((pAddress+size) <= (m_pAddress+m_Size)) ? true : false;
}

} // namespace Display2
