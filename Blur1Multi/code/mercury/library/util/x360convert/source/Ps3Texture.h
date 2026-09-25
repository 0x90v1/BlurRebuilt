// [Blur1Multi] PS3 (libgcm) textures -> PC (T076).
//
// A PS3 texture as Blur serialises it (NglPs3Tex::SerialOutTexture,
// graphics/texture_export/source/targets/ps3tex.cpp:859-1022; read back by
// Display::GCM::CreateTexture2D, graphics/display/source/texture/gcm/GCMTextureLoad.cpp:90-108):
// the CellGcmTexture fields written one by one, big-endian (ps3tex.cpp:845-850),
//   U16 width, U16 height, U8 mipmap, U8 format, U8 dimension, U8 cubemap,
//   U32 remap, U16 depth, U32 pitch, U32 offset            (22 bytes)
// then U32 size and 'size' bytes of RSX image. A whole texture (.texture "Data",
// font page) is header + size + image; RenderingData's LodWriter<Texture> writes the
// 26 header+size bytes (JUST_HEADER) and the image (JUST_DATA) separately
// (renderingdata/source/noninterface/build/archive.cpp:446-481).
// The PC side is what PCLoadTexture reads (display/source/texture/d3d9/D3D9TextureLoad.cpp:83-122).
#pragma once

#include <x360convert/X360Convert.h>

#include <stddef.h>
#include <string>
#include <vector>

namespace X360Convert
{
	const size_t kGcmHeaderSize = 26;	// 22 bytes of CellGcmTexture fields + U32 size

	struct GcmTexture
	{
		unsigned width, height, mipmap, format, dimension, cubemap, remap, depth, pitch, offset, size;
	};

	// Reads the 26-byte header; false when fewer bytes are available.
	bool ParseGcmHeader(const unsigned char* p, size_t avail, GcmTexture& t);

	// Appends the PC TextureDesc for 't' to 'out' (Levels = t.mipmap).
	E_Result Ps3TextureDesc(const GcmTexture& t, std::vector<unsigned char>& out, std::string& error);

	// Converts the image of 't' holding 'levels' mip levels from the header's top
	// level down (every face), appending the PC levels to 'out'.
	E_Result Ps3TextureLevels(const GcmTexture& t, unsigned levels, const unsigned char* data, size_t dataSize,
		std::vector<unsigned char>& out, std::string& error);

	// The PS3 forms of ConvertTexturePayload, ConvertTextureLodHeader and ConvertTextureLodData.
	E_Result Ps3ConvertTexturePayload(const unsigned char* p, size_t avail, size_t& consumed,
		std::vector<unsigned char>& out, std::string& error);
	E_Result Ps3ConvertTextureLodHeader(const unsigned char* header, size_t size, std::vector<unsigned char>& out, std::string& error);
	E_Result Ps3ConvertTextureLodData(const unsigned char* header, int lod, const unsigned char* data, size_t dataSize,
		std::vector<unsigned char>& out, std::string& error);
}
