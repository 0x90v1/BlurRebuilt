// [Blur1Multi] XenosRecomp as a library call: one Xbox 360 shader container
// (the microcode block of an XEFX function) to Direct3D 9 shader model 3 HLSL.
// Plain types only, so callers need no C++20.
#pragma once

#include <stddef.h>
#include <string>

namespace XenosRecompD3D9
{
	// 'container' is the whole big-endian Xenos shader container. On success
	// 'hlsl' holds the program (entry point "main"), 'isPixelShader' its stage and
	// 'unsupported' what the Direct3D 9 output could not express (empty when all
	// of it could). False when the bytes are not a shader container.
	bool Recompile(const unsigned char* container, size_t size, std::string& hlsl, bool& isPixelShader,
		std::string& unsupported, std::string& error);
}
