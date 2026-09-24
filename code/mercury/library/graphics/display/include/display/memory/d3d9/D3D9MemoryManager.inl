#ifndef DISPLAY_MEMORY_D3D9_D3D9_MEMORY_MANAGER_INL
#define DISPLAY_MEMORY_D3D9_D3D9_MEMORY_MANAGER_INL

#ifdef _MSC_VER
#pragma once
#endif

namespace Display
{
namespace D3D9
{

INLINE const bool C_D3D9MemoryBuffer::IsCreated() const
{
	return m_pAddress ? true : false;
}

INLINE const U32 C_D3D9MemoryBuffer::GetSize() const
{
	return m_Size;
}

INLINE const U32 C_D3D9MemoryBuffer::GetAlign() const
{
	return m_Align;
}

INLINE const U32 C_D3D9MemoryBuffer::GetFlags() const
{
	return m_Flags;
}

INLINE U8* C_D3D9MemoryBuffer::GetAddress() const
{
	return m_pAddress;
}

}; // namespace D3D9
}; // namespace Display

#endif // DISPLAY_MEMORY_D3D9_D3D9_MEMORY_MANAGER_INL
