#ifndef DISPLAY_SHADER_D3D9_D3D9_PIXEL_SHADER_H
#define DISPLAY_SHADER_D3D9_D3D9_PIXEL_SHADER_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/D3D9/D3D9.h"
#include "Display/Resource/Resource.h"
#include "Display/Shader/D3D9/D3D9Shader.h"

namespace Display
{

typedef Display2::C_PixelShaderParam C_PixelShaderParameter;

typedef Display2::C_PixelShaderParam C_PixelShaderParameterArray;

typedef Display2::C_PixelShader C_PixelShader;

struct C_PixelShaderConstantLock
{
	U32	startRegister;
	U32 endRegister;
	U32 salt;
	bool enabled;
};

struct C_PixelShaderConstantLockKey
{
	U32 index;
	U32 salt;
};

enum E_PixelShaderConstantLock
{
	E_PixelShaderConstantLock_Count = 4
};

}; // namespace Display

#include "Display/Shader/D3D9/D3D9PixelShader.inl"

#endif // DISPLAY_SHADER_D3D9_D3D9_PIXEL_SHADER_H
