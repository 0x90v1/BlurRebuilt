#ifndef DISPLAY2_XB2_BUFFER_INL
#define DISPLAY2_XB2_BUFFER_INL

#ifdef _MSC_VER
#pragma once
#endif

namespace Display2
{

// C_CommandBuffer

INLINE bool C_CommandBuffer::IsCreated() const
{
	return false;
}

INLINE U32 C_CommandBuffer::GetSize() const
{
	return 0;
}

INLINE U8* C_CommandBuffer::GetAddress() const
{
	return NULL;
}

} // namespace Display2

#endif // DISPLAY2_XB2_BUFFER_INL
