// [Blur1Multi] Xbox 360 -> PC converters built on the engine's own readers. See EngineConverters.cpp.
#pragma once

#include <x360convert/X360Convert.h>

namespace X360Convert
{
	bool EngineHandlesPath(const char* path);
	E_Result EngineConvertFile(const char* path, const unsigned char* in, size_t inSize,
		std::vector<unsigned char>& out, std::string& error);
}
