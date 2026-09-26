#ifndef DISPLAY2_XB2_BUFFER_H
#define DISPLAY2_XB2_BUFFER_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display2/Context/Context.h"

namespace Display2
{

//! C_CommandBuffer

class C_CommandBuffer
{
public:

	C_CommandBuffer();
	~C_CommandBuffer();

	bool IsCreated() const;

	U32 GetSize() const;
	U8* GetAddress() const;

	bool Run();
};

} // namespace Display2

#include "Display2/Buffer/XB2Buffer.inl"

#endif // DISPLAY2_XB2_BUFFER_H
