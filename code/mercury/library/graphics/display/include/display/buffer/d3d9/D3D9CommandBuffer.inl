#ifndef DISPLAY_BUFFER_D3D9_D3D9_COMMAND_BUFFER_INL
#define DISPLAY_BUFFER_D3D9_D3D9_COMMAND_BUFFER_INL

#ifdef _MSC_VER
#pragma once
#endif

namespace Display
{
namespace D3D9
{

INLINE const bool C_CommandBuffer::IsCreated() const
{
	return m_pd3dCommandBuffer ? true : false;
}

}; // namespace D3D9
}; // namespace Display

#endif // DISPLAY_BUFFER_D3D9_D3D9_COMMAND_BUFFER_INL
