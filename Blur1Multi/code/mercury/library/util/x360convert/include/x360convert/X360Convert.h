// [Blur1Multi] Xbox 360 -> PC asset conversion (T072, decision D11).
//
// Converts one file's bytes as they come out of an Xbox 360 pak (or the loose
// files on a 360 disc) into the bytes the PC build of Blur writes for the same
// asset, so the engine's PC readers can load it unchanged. Engine-independent:
// the runtime file-system layer and the standalone verifier
// (code/tools/x360convert-verify) share this code.
//
// Every converter is written from the engine's own serialisers for the PC side
// and from the Xbox side's writers / Blur2Test's decoders for the 360 side, and
// is checked byte for byte against the retail PC file of the same asset where
// one exists (docs/analysis.md "Xbox 360 -> PC conversion").
#pragma once

#include <stddef.h>
#include <string>
#include <vector>

namespace X360Convert
{
	enum E_Result
	{
		E_Result_NotHandled = 0,	// not a format this library converts: serve the bytes unchanged
		E_Result_Converted,			// 'out' holds the PC file
		E_Result_Failed,			// a format this library converts, but the input is not valid for it
	};

	// 'path' is the virtual path the engine asked for (used for the extension).
	E_Result ConvertFile(const char* path, const unsigned char* in, size_t inSize,
		std::vector<unsigned char>& out, std::string& error);

	// Whether ConvertFile handles files with this path at all (by extension), so a
	// caller can pass everything else straight through without reading it.
	bool HandlesPath(const char* path);

	// Individual converters.
	E_Result ConvertTexture(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error);
	E_Result ConvertFont(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error);
	E_Result ConvertBundle(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error);
	E_Result ConvertLoc(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error);
	E_Result ConvertFlask(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error);
	E_Result ConvertParticleMesh(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error);

	// One serialised texture as Display::LoadTexture2D reads it (a .texture "Data"
	// payload, a font page): the Xenon header + GPU allocation at 'p' becomes
	// TextureDesc + levels appended to 'out'. 'consumed' is the 360 bytes used.
	E_Result ConvertTexturePayload(const unsigned char* p, size_t avail, size_t& consumed,
		std::vector<unsigned char>& out, std::string& error);
}
