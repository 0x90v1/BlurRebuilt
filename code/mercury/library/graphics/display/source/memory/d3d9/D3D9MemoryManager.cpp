#include "stdafx.h"
#include "Display/Memory/D3D9/D3D9MemoryManager.h"
#include <system/memory/MemoryTracker.h>

#ifdef CFG_ALLOW_OPTIMIZE_OFF
//#pragma optimize( "", off ) // CFG_ALLOW_OPTIMIZE_OFF
#endif

namespace Display
{
namespace D3D9
{

C_D3D9MemoryBuffer::C_D3D9MemoryBuffer() :
	m_Size(0),
	m_Align(0),
	m_Flags(0),
	m_pAddress(NULL)
{
}

C_D3D9MemoryBuffer::~C_D3D9MemoryBuffer()
{
	DestroyAddress();
}

bool C_D3D9MemoryBuffer::CreateAddress()
{
	assert(!m_pAddress);
	m_pAddress = (U8*)BizHeapPhysicalAlloc(m_Size, MAXULONG_PTR, m_Align, m_Flags);
	if(m_pAddress)
	{
		return true;
	}
	else
	{
		LOGFATAL(g_Log, "Failed to allocate %s bytes of xenon physical memory\n", m_Size);
		return false;
	}
}

void C_D3D9MemoryBuffer::DestroyAddress()
{
	if(m_pAddress)
	{
		BizHeapPhysicalFree(m_pAddress);
		m_pAddress = NULL;
	}
}

}; // namespace D3D9
}; // namespace Display
