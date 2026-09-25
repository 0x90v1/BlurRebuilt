// [Blur1Multi] Xenos (Xbox 360 GPU) texture layout: fetch constant, formats,
// endian modes, tiling, mip chains. A C++ port of Blur2Test's
// _tools/b2_extract_textures.py (itself the XG 2D tiled-address mapping and
// Xenia's packed-mip offsets); names follow that file so the two can be read
// side by side.
#pragma once

#include <string>
#include <vector>

namespace X360Convert
{
	// GPUTEXTURE_FETCH_CONSTANT fields (Xenia xe_gpu_texture_fetch_t), dwords big-endian.
	struct XenosFetch
	{
		unsigned type;			// dword0 bits 0-1
		unsigned signs;			// dword0 bits 2-9
		unsigned pitchDiv32;	// dword0 bits 22-30: base pitch in texels / 32
		unsigned tiled;			// dword0 bit 31
		unsigned format;		// dword1 bits 0-5
		unsigned endian;		// dword1 bits 6-7: 0 none, 1 8in16, 2 8in32, 3 16in32
		unsigned baseAddress;	// dword1 bits 12-31 (4 KB pages)
		unsigned width;			// dword2 (2D): bits 0-12 + 1
		unsigned height;		// dword2 (2D): bits 13-25 + 1
		unsigned numFormat;		// dword3 bit 0
		unsigned swizzle;		// dword3 bits 1-12
		unsigned mipMinLevel;	// dword4 bits 2-5
		unsigned mipMaxLevel;	// dword4 bits 6-9
		unsigned dimension;		// dword5 bits 9-10: 0 1D, 1 2D, 2 3D, 3 cube
		unsigned packedMips;	// dword5 bit 11
		unsigned mipAddress;	// dword5 bits 12-31 (4 KB pages)
	};

	// 'fetch' points at the six big-endian fetch-constant dwords.
	void ParseFetch(const unsigned char* fetch, XenosFetch& f);

	struct XenosFormat
	{
		const char* name;
		unsigned blockW, blockH, bytesPerBlock;
	};

	// Formats the retail data uses; false for anything else.
	bool GetXenosFormat(unsigned format, XenosFormat& out);

	// Undo the fetch constant's endian mode in place.
	bool EndianNormalize(unsigned char* data, size_t size, unsigned mode);

	// Decodes a tiled 2D allocation (already endian-normalised) into its mip
	// levels, row-major blocks, top level first. See b2_extract_textures.py
	// decode_xenos_2d_chain for the layout rules; every check there is kept.
	bool DecodeXenos2DChain(const unsigned char* payload, size_t size, const XenosFetch& f,
		const XenosFormat& fmt, std::vector<std::vector<unsigned char> >& levels, std::string& error);

	// Decodes a tiled cube allocation into face-major levels (face 0 mips, then
	// face 1 mips, ...), as decode_xenos_cubemap.
	bool DecodeXenosCube(const unsigned char* payload, size_t size, const XenosFetch& f,
		const XenosFormat& fmt, std::vector<std::vector<unsigned char> >& faceLevels, unsigned& mipCount, std::string& error);
}
