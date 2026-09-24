#ifndef DISPLAY_SHADER_D3D9_D3D9_SHADER_H
#define DISPLAY_SHADER_D3D9_D3D9_SHADER_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/D3D9/D3D9.h"

#ifdef PLATFORM_PC
struct C_ShaderProfile
{
	int		m_Size;
	int*	m_pData;
};
struct C_ShaderData
{
	C_ShaderProfile m_Profiles[Display::E_ShaderProfile_Count];

	const C_ShaderProfile& GetProfile(const Display::E_ShaderProfile profile) const
	{
		assert(profile < Display::E_ShaderProfile_Count);
		return m_Profiles[profile];
	}
};
#endif

#endif // DISPLAY_SHADER_D3D9_D3D9_SHADER_H
