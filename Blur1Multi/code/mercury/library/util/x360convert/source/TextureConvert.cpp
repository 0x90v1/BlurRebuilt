// [Blur1Multi] .texture: Xbox 360 -> PC.
//
// Both platforms write a "Texture" block holding "Header" (U32 file version,
// U32 flags; Display::LoadTexture reads them) and "Data".
//   PC Data (display/texture/d3d9/D3D9TextureLoad.cpp, PCLoadTexture / D3DLoadTexture2D):
//     TextureDesc {S32 Width, Height, Depth, Levels; U32 Format (D3DFORMAT), Usage, Type (3 2D, 5 cube)},
//     then every level's surface, tightly packed (D3D9Texture2DSize bytes each),
//     cube maps face-major (face 0's levels, then face 1's, ...).
//   360 Data (D3D9TextureLoad.cpp, PLATFORM_XENON CreateTexture2D): the 52-byte
//     D3DBaseTexture (7 resource dwords, then the 6-dword GPUTEXTURE_FETCH_CONSTANT),
//     U32 size, U32 alignment, U32 unused, then the GPU allocation, tiled.
// The texel blocks themselves are the same bytes once the fetch constant's
// endian mode is undone and the allocation untiled (verified against the retail
// PC files, docs/analysis.md "Xbox 360 -> PC conversion").
// PS3 files have the same blocks; their Data payload is the GCM texture of
// Ps3Texture.h (docs/analysis.md "PS3 textures").

#include <x360convert/X360Convert.h>

#include "ChunkFile.h"
#include "Ps3Texture.h"
#include "XenosTexture.h"

#include <stdio.h>

namespace X360Convert
{
	namespace
	{
		const unsigned kD3DFMT_A8R8G8B8 = 21;
		const unsigned kD3DFMT_A8 = 28;
		const unsigned kD3DFMT_A8L8 = 51;
		unsigned FourCC(const char* s) { return s[0] | (s[1] << 8) | (s[2] << 16) | ((unsigned)s[3] << 24); }

		// The D3DFORMAT the PC build stores for a Xenos format (from the retail
		// PC counterparts of every 360 .texture, code/tools/xmem-verify/texsurvey.py).
		bool PcFormat(unsigned xenos, unsigned& d3d)
		{
			switch (xenos)
			{
			case 0x12: d3d = FourCC("DXT1"); return true;
			case 0x13: d3d = FourCC("DXT3"); return true;
			case 0x14: d3d = FourCC("DXT5"); return true;
			case 0x06: d3d = kD3DFMT_A8R8G8B8; return true;
			case 0x02: d3d = kD3DFMT_A8; return true;
			case 0x0A: d3d = kD3DFMT_A8L8; return true;	// scene textures (uv_distort*), retail PC models
			default: return false;
			}
		}

		E_Result Fail(std::string& error, const std::string& text)
		{
			error = text;
			return E_Result_Failed;
		}
	}

	E_Result ConvertTexturePayload(const unsigned char* p, size_t avail, size_t& consumed,
		std::vector<unsigned char>& out, std::string& error)
	{
		if (GetSource() == E_Source_PS3)
			return Ps3ConvertTexturePayload(p, avail, consumed, out, error);
		if (avail < 64)
			return Fail(error, "texture payload shorter than the Xenon texture header");
		XenosFetch f;
		ParseFetch(p + 0x1c, f);
		const unsigned gpuSize = BE32(p + 0x34);
		if (64 + (size_t)gpuSize > avail)
			return Fail(error, "GPU data runs past the block");
		consumed = 64 + (size_t)gpuSize;
		XenosFormat fmt;
		unsigned d3dFormat;
		if (!GetXenosFormat(f.format, fmt) || !PcFormat(f.format, d3dFormat))
		{
			char msg[64];
			sprintf_s(msg, "Xenos texture format 0x%02x not handled", f.format);
			return Fail(error, msg);
		}
		if (!f.tiled)
			return Fail(error, "linear (untiled) Xenos texture not handled");
		std::vector<unsigned char> gpu(p + 64, p + 64 + gpuSize);
		if (!EndianNormalize(gpu.empty() ? NULL : &gpu[0], gpu.size(), f.endian))
			return Fail(error, "endian unit does not divide the GPU data");

		std::vector<std::vector<unsigned char> > levels;
		unsigned mipCount = f.mipMaxLevel + 1;
		unsigned type;
		std::string e;
		if (f.dimension == 1)
		{
			type = 3;
			if (!DecodeXenos2DChain(gpu.empty() ? NULL : &gpu[0], gpu.size(), f, fmt, levels, e))
				return Fail(error, "2D: " + e);
		}
		else if (f.dimension == 3)
		{
			type = 5;
			if (!DecodeXenosCube(gpu.empty() ? NULL : &gpu[0], gpu.size(), f, fmt, levels, mipCount, e))
				return Fail(error, "cube: " + e);
		}
		else
			return Fail(error, "texture dimension other than 2D or cube");
		if (f.mipMinLevel != 0)
			return Fail(error, "non-zero minimum mip level");

		PutLE32(out, f.width);
		PutLE32(out, f.height);
		PutLE32(out, 1);			// Depth
		PutLE32(out, mipCount);		// Levels
		PutLE32(out, d3dFormat);
		PutLE32(out, 0);			// Usage: unused by PCLoadTexture; retail PC files hold uninitialised bytes here
		PutLE32(out, type);
		for (size_t l = 0; l < levels.size(); ++l)
			out.insert(out.end(), levels[l].begin(), levels[l].end());
		return E_Result_Converted;
	}

	E_Result ConvertTextureLodHeader(const unsigned char* header, size_t size, std::vector<unsigned char>& out, std::string& error)
	{
		if (GetSource() == E_Source_PS3)
			return Ps3ConvertTextureLodHeader(header, size, out, error);
		if (size != 64)
			return Fail(error, "Xenon texture header of unexpected size");
		XenosFetch f;
		ParseFetch(header + 0x1c, f);
		unsigned d3dFormat;
		if (!PcFormat(f.format, d3dFormat))
		{
			char msg[64];
			sprintf_s(msg, "Xenos texture format 0x%02x not handled", f.format);
			return Fail(error, msg);
		}
		if (f.dimension != 1 && f.dimension != 3)
			return Fail(error, "texture dimension other than 2D or cube");
		PutLE32(out, f.width);
		PutLE32(out, f.height);
		PutLE32(out, 1);					// Depth
		PutLE32(out, f.mipMaxLevel + 1);	// Levels
		PutLE32(out, d3dFormat);
		PutLE32(out, 0);					// Usage (see ConvertTexturePayload)
		PutLE32(out, f.dimension == 1 ? 3 : 5);
		return E_Result_Converted;
	}

	E_Result ConvertTextureLodData(const unsigned char* header, int lod, const unsigned char* data, size_t dataSize,
		std::vector<unsigned char>& out, std::string& error)
	{
		if (GetSource() == E_Source_PS3)
			return Ps3ConvertTextureLodData(header, lod, data, dataSize, out, error);
		XenosFetch h;
		ParseFetch(header + 0x1c, h);
		XenosFormat fmt;
		if (!GetXenosFormat(h.format, fmt))
			return Fail(error, "Xenos texture format not handled");
		if (h.dimension != 1)
			return Fail(error, "resource block texture other than 2D not handled");
		if (!h.tiled)
			return Fail(error, "linear (untiled) Xenos texture not handled");

		// LodCalculator::GetMipLevelsForLod: lod 0 writes every level from its first
		// to the last (the header's whole chain), any later lod one level.
		const unsigned levels = lod == 0 ? h.mipMaxLevel + 1 : 1;

		// NglXeTex::SerialOutTexture writes sizeUsed bytes: the XG allocation, less
		// 4 KB (DXT1) or 8 KB (DXT5) when the chain reaches the packed tail and the
		// texture is less than four times taller than wide.
		unsigned used = XgTexture2DSize(h.width, h.height, levels, fmt);
		if (levels > XgMipTailBaseLevel(h.width, h.height) && h.height < h.width * 4)
		{
			if (h.format == 0x12)
				used -= 4096;
			else if (h.format == 0x14)
				used -= 8192;
		}
		if (used != dataSize)
		{
			char msg[160];
			sprintf_s(msg, "texture data of 0x%x bytes where %u levels of %ux%u take 0x%x",
				(unsigned)dataSize, levels, h.width, h.height, used);
			return Fail(error, msg);
		}

		XenosFetch f;
		MakeXgFetch2D(h.width, h.height, levels, h.format, fmt, f);
		std::vector<unsigned char> gpu(data, data + dataSize);
		if (!EndianNormalize(gpu.empty() ? NULL : &gpu[0], gpu.size(), h.endian))
			return Fail(error, "endian unit does not divide the GPU data");
		std::vector<std::vector<unsigned char> > decoded;
		std::string e;
		if (!DecodeXenos2DChain(gpu.empty() ? NULL : &gpu[0], gpu.size(), f, fmt, decoded, e))
			return Fail(error, "2D: " + e);
		for (size_t l = 0; l < decoded.size(); ++l)
			out.insert(out.end(), decoded[l].begin(), decoded[l].end());
		return E_Result_Converted;
	}

	E_Result ConvertTexture(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error)
	{
		Block root;
		if (!ReadBlockBE(in, inSize, root) || root.id != "Texture" || root.size != inSize)
			return Fail(error, "not a big-endian Texture block tree");
		std::vector<Block> children;
		if (!ChildrenBE(root, children))
			return Fail(error, "bad child blocks");

		out.clear();
		out.reserve(inSize);
		const size_t rootAt = BeginBlockLE(out, root, root.count);
		for (size_t i = 0; i < children.size(); ++i)
		{
			const Block& b = children[i];
			if (b.id == "Header")
			{
				// U32 fileVersion, U32 flags
				if (b.PayloadSize() % 4)
					return Fail(error, "Header block is not whole U32s");
				const size_t at = BeginBlockLE(out, b, b.count);
				for (size_t o = 0; o < b.PayloadSize(); o += 4)
					PutLE32(out, BE32(b.payload + o));
				EndBlockLE(out, at);
			}
			else if (b.id == "Data")
			{
				const size_t at = BeginBlockLE(out, b, b.count);
				size_t consumed = 0;
				const E_Result r = ConvertTexturePayload(b.payload, b.PayloadSize(), consumed, out, error);
				if (r != E_Result_Converted)
					return r;
				if (consumed != b.PayloadSize())
					return Fail(error, "GPU data size does not fill the Data block");
				EndBlockLE(out, at);
			}
			else
				return Fail(error, "unknown block '" + b.id + "' in a texture");
		}
		EndBlockLE(out, rootAt);
		return E_Result_Converted;
	}
}
