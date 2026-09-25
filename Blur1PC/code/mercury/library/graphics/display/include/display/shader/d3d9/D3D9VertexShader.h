#ifndef DISPLAY_SHADER_D3D9_D3D9_VERTEX_SHADER_H
#define DISPLAY_SHADER_D3D9_D3D9_VERTEX_SHADER_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Display/Device/D3D9/D3D9.h"
#include "Display/Resource/Resource.h"
#include "Display/Shader/D3D9/D3D9Shader.h"

namespace Display
{

typedef Display2::C_VertexShaderParam C_VertexShaderParameter;

typedef Display2::C_VertexShader C_VertexShader;

}; // namespace Display

#include "Display/Shader/D3D9/D3D9VertexShader.inl"

#endif // DISPLAY_SHADER_D3D9_D3D9_VERTEX_SHADER_H
