// [Blur1Multi] Xbox 360 -> PC converters built on the engine's own readers. See EngineConverters.cpp.
#pragma once

#include <x360convert/X360Convert.h>

namespace X360Convert { struct Block; }

namespace X360Convert
{
	bool EngineHandlesPath(const char* path);
	E_Result EngineConvertFile(const char* path, const unsigned char* in, size_t inSize,
		std::vector<unsigned char>& out, std::string& error);

	// C_ModelData::Read's MdlDat block (models and levels).
	bool ConvertMdlDat(const Block& mdl, std::vector<unsigned char>& out, std::string& error);

	// LevelConvert.cpp: .level and .levelphysics; .levelrender with its scene
	// already converted (C_LevelConversion, LevelTranscode.h).
	bool LevelHandlesPath(const char* path);
	E_Result LevelConvertFile(const char* path, const unsigned char* in, size_t inSize,
		std::vector<unsigned char>& out, std::string& error);
	E_Result ConvertLevelRender(const unsigned char* in, size_t inSize, const std::vector<unsigned char>& scene,
		std::vector<unsigned char>& out, std::string& error);

	// A Gravity payload (model Coll, level Main/Route) converted by running the
	// importer over a C_ScalarSwapFile; appends the PC payload to 'out'.
	bool ConvertGravityPayload(const unsigned char* p, size_t n, std::vector<unsigned char>& out, std::string& error);
}
