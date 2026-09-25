#ifndef DISPLAY2_XB2_MEMORY_H
#define DISPLAY2_XB2_MEMORY_H

#ifdef _MSC_VER
#pragma once
#endif

namespace Display2
{

//! E_MemoryBuffer

enum E_MemoryBuffer
{
	E_MemoryBuffer_Unknown,
	E_MemoryBuffer_Physical,
	E_MemoryBuffer_Indirect,
};

//! C_MemoryBuffer

class C_MemoryBuffer
{
public:

	C_MemoryBuffer();
	
	U32 GetSize() const;
	U32 GetAlign() const;
	U32 GetFlags() const;
	U8* GetAddress() const;
	E_MemoryBuffer GetType() const;
	
	bool Create(const U32 size, const U32 align, const U32 flags, const E_MemoryBuffer type, U8* pAddress = NULL);
	void Destroy();

	bool Inside(const U32 size, const U8* pAddress) const;

private:

	U32					m_Size;
	U32					m_Align;
	U32					m_Flags;
	U8*					m_pAddress;
	E_MemoryBuffer		m_Type;
};

} // namespace Display2

#include "Display2/Memory/XB2Memory.inl"

#endif // DISPLAY2_XB2_MEMORY_H
