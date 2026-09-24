#ifndef DISPLAY_MEMORY_D3D9_D3D9_MEMORY_MANAGER_H
#define DISPLAY_MEMORY_D3D9_D3D9_MEMORY_MANAGER_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Memory/MemoryManagerInterface.h"

namespace Display
{
namespace D3D9
{

//! E_MemoryFlags

enum E_MemoryFlags
{
	E_MemoryFlags_PageReadOnly		= 0x02,
	E_MemoryFlags_PageReadWrite		= 0x04,
	E_MemoryFlags_PageNoCache		= 0x200,
	E_MemoryFlags_PageWriteCombine	= 0x400,
};

//! C_D3D9MemoryBuffer

class C_D3D9MemoryBuffer : public I_MemoryBuffer
{
public:

	C_D3D9MemoryBuffer();
	~C_D3D9MemoryBuffer();

	const bool IsCreated() const;

	const U32 GetSize() const;
	const U32 GetAlign() const;
	const U32 GetFlags() const;
	U8* GetAddress() const;

protected:

	bool CreateAddress();
	void DestroyAddress();

protected:

	U32 m_Size;
	U32 m_Align;
	U32 m_Flags;
	U8* m_pAddress;
};

}; // namespace D3D9
}; // namespace Display

#include "Display/Memory/D3D9/D3D9MemoryManager.inl"

#endif // DISPLAY_MEMORY_D3D9_D3D9_MEMORY_MANAGER_H
