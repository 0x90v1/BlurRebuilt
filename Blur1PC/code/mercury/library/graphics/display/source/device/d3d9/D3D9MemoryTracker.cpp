#include "stdafx.h"
#include "Display/Device/D3D9/D3D9MemoryTracker.h"

#ifdef CFG_ALLOW_OPTIMIZE_OFF
//#pragma optimize( "", off ) // CFG_ALLOW_OPTIMIZE_OFF
#endif

namespace Display
{
namespace D3D9
{

// C_D3DMemoryTracker.

C_D3DMemoryTracker C_D3DMemoryTracker ::s_Instance;

C_D3DMemoryTracker::C_D3DMemoryTracker()
{
	for(U32 i=0; i<E_D3DMemoryTracker_Count; i++)
	{
		m_Enabled[i] = false;
	}
}

C_D3DMemoryTracker::~C_D3DMemoryTracker()
{
}

const U32 C_D3DMemoryTracker::GetEnabledType()
{
	U32 type = E_D3DMemoryTracker_Count;
	for(U32 i=0; i<E_D3DMemoryTracker_Count; i++)
	{
		if(m_Enabled[i])
		{
			for(U32 j=i+1; j<E_D3DMemoryTracker_Count; j++)
			{
				assert(!m_Enabled[j]);
			}
			return i;
		}
	}
	return type;
}

void C_D3DMemoryTracker::Enable(const E_D3DMemoryTracker type)
{
	assert(type < E_D3DMemoryTracker_Count);
	for(U32 i=0; i<E_D3DMemoryTracker_Count; i++)
	{
		if(i == type)
		{
			assert(!m_Enabled[i]);
			m_Enabled[i] = true;
		}
		else
		{
			assert(!m_Enabled[i]);
		}
	}
}

void C_D3DMemoryTracker::Disable(const E_D3DMemoryTracker type)
{
	assert(type < E_D3DMemoryTracker_Count);
	for(U32 i=0; i<E_D3DMemoryTracker_Count; i++)
	{
		if(i == type)
		{
			assert(m_Enabled[i]);
			m_Enabled[i] = false;
		}
		else
		{
			assert(!m_Enabled[i]);
		}
	}
}

// C_D3DMemoryStat.

C_D3DMemoryStat::C_D3DMemoryStat(const E_D3DMemoryTracker type) :
	m_Type(type)
{
	C_D3DMemoryTracker::Instance().Enable(m_Type);
}

C_D3DMemoryStat::~C_D3DMemoryStat()
{
	C_D3DMemoryTracker::Instance().Disable(m_Type);
}

}; // namespace D3D9
}; // namespace Display
