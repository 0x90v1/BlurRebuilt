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

#include <x360convert/X360Convert.h>

#include "ChunkFile.h"
#include "XenosTexture.h"

#include <stdio.h>

namespace X360Convert
{
	namespace
	{
		const unsigned kD3DFMT_A8R8G8B8 = 21;
		const unsigned kD3DFMT_A8 = 28;
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
