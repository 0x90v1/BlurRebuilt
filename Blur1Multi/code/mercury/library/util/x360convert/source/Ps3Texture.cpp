// [Blur1Multi] PS3 (libgcm) textures -> PC (T076). See Ps3Texture.h.
//
// Every layout rule below is the exporter's (NglPs3Tex, graphics/texture_export/source/
// targets/ps3tex.cpp) or the PC loader's; the citations are to Blur1PC/code/mercury/library.
// Checked byte for byte against retail PC (docs/analysis.md "PS3 textures"): every PS3
// .texture, the debug font, the vehicle models' and the levels' LOD textures. The data
// holds DXT1/3/5 (swizzle-flagged and linear), A8R8G8B8 and A8 swizzled (square, and one
// 1024x4 A8 chain), so these paths are verified; cube maps, linear chains with more than
// one level, A8L8, the 16-bit and float formats and non-square swizzles of more than one
// byte per texel follow the exporter source and are not exercised by any retail file.

#include "Ps3Texture.h"

#include "ChunkFile.h"

#include <stdio.h>

namespace X360Convert
{
	namespace
	{
		E_Result Fail(std::string& error, const std::string& text)
		{
			error = text;
			return E_Result_Failed;
		}

		unsigned FourCC(const char* s) { return s[0] | (s[1] << 8) | (s[2] << 16) | ((unsigned)s[3] << 24); }

		// gcm.h:120-149 (texture_export/source/targets/ps3/cell/gcm.h)
		const unsigned kGcmLN = 0x20, kGcmUN = 0x40;
		const unsigned kGcmB8 = 0x81, kGcmA1R5G5B5 = 0x82, kGcmA4R4G4B4 = 0x83, kGcmR5G6B5 = 0x84,
			kGcmA8R8G8B8 = 0x85, kGcmDXT1 = 0x86, kGcmDXT23 = 0x87, kGcmDXT45 = 0x88, kGcmG8B8 = 0x8B,
			kGcmW16Z16Y16X16F = 0x9A, kGcmW32Z32Y32X32F = 0x9B, kGcmD1R5G5B5 = 0x9D;

		// CELL_GCM_REMAP_MODE (gcm.h:548-549) and the remaps of ngl_ps3_gpu_types.h:161-195.
		unsigned RemapMode(unsigned order, unsigned iA, unsigned iR, unsigned iG, unsigned iB,
			unsigned oA, unsigned oR, unsigned oG, unsigned oB)
		{
			return (order << 16) | iA | (iR << 2) | (iG << 4) | (iB << 6) | (oA << 8) | (oR << 10) | (oG << 12) | (oB << 14);
		}
		enum { FROM_A = 0, FROM_R = 1, FROM_G = 2, FROM_B = 3, R_ONE = 1, R_REMAP = 2 };
		const unsigned kRemapNone = RemapMode(0, FROM_A, FROM_R, FROM_G, FROM_B, R_REMAP, R_REMAP, R_REMAP, R_REMAP);	// 0xAAE4
		const unsigned kRemap1RGB = RemapMode(0, FROM_A, FROM_R, FROM_G, FROM_B, R_ONE, R_REMAP, R_REMAP, R_REMAP);	// 0xA9E4
		const unsigned kRemapABGR = RemapMode(0, FROM_A, FROM_B, FROM_G, FROM_R, R_REMAP, R_REMAP, R_REMAP, R_REMAP);	// 0xAA6C
		const unsigned kRemap1BGR = RemapMode(0, FROM_A, FROM_B, FROM_G, FROM_R, R_ONE, R_REMAP, R_REMAP, R_REMAP);	// 0xA96C
		const unsigned kRemapGBBB = RemapMode(0, FROM_G, FROM_B, FROM_B, FROM_B, R_REMAP, R_REMAP, R_REMAP, R_REMAP);	// 0xAAFE
		const unsigned kRemapB111 = RemapMode(0, FROM_B, FROM_B, FROM_B, FROM_B, R_REMAP, R_ONE, R_ONE, R_ONE);		// 0x56FF

		struct GcmFormat
		{
			unsigned d3dFormat;
			unsigned bpp;		// GCMGetTextureFormatBPP (ps3tex.cpp:154-185): bytes per texel, per 4x4 block for DXT
			bool dxt;
			unsigned swapUnit;	// GCMEndianSwapSurface (ps3tex.cpp:298-385): 0 none, 2 per U16, 4 per U32
		};

		// Ps3TexelFormats (ps3tex.cpp:44-108), the entries whose texel bits are the named
		// D3DFORMAT's. Left out: X16/X32/depth (index and depth D3DFORMATs), HILO, R6G5B5,
		// R8G8B8A8, Y16X16 (D3DFMT_UNKNOWN there), and the A1B5G5R5 pair, which the table
		// maps to D3DFMT_A1R5G5B5 although its red and blue are exchanged by the remap.
		bool GetGcmFormat(unsigned pixelFormat, unsigned remap, GcmFormat& f)
		{
			f.dxt = false;
			f.swapUnit = 0;
			if (pixelFormat == kGcmDXT1 && (remap == kRemapNone || remap == kRemap1RGB))	// DXT1, DXT1_RGB
			{ f.d3dFormat = FourCC("DXT1"); f.bpp = 8; f.dxt = true; return true; }
			if (pixelFormat == kGcmDXT23 && remap == kRemapNone)
			{ f.d3dFormat = FourCC("DXT3"); f.bpp = 16; f.dxt = true; return true; }
			if (pixelFormat == kGcmDXT45 && remap == kRemapNone)
			{ f.d3dFormat = FourCC("DXT5"); f.bpp = 16; f.dxt = true; return true; }
			if (pixelFormat == kGcmB8 && remap == kRemapNone)			// L8
			{ f.d3dFormat = 50; f.bpp = 1; return true; }
			if (pixelFormat == kGcmB8 && remap == kRemapB111)			// A8
			{ f.d3dFormat = 28; f.bpp = 1; return true; }
			if (pixelFormat == kGcmG8B8 && remap == kRemapGBBB)			// A8L8
			{ f.d3dFormat = 51; f.bpp = 2; f.swapUnit = 2; return true; }
			if (pixelFormat == kGcmA1R5G5B5 && remap == kRemapNone)		// A1R5G5B5
			{ f.d3dFormat = 25; f.bpp = 2; f.swapUnit = 2; return true; }
			if (pixelFormat == kGcmD1R5G5B5 && remap == kRemapNone)		// X1R5G5B5
			{ f.d3dFormat = 24; f.bpp = 2; f.swapUnit = 2; return true; }
			if (pixelFormat == kGcmR5G6B5 && remap == kRemapNone)		// R5G6B5
			{ f.d3dFormat = 23; f.bpp = 2; f.swapUnit = 2; return true; }
			if (pixelFormat == kGcmA4R4G4B4 && remap == kRemapNone)		// A4R4G4B4
			{ f.d3dFormat = 26; f.bpp = 2; f.swapUnit = 2; return true; }
			if (pixelFormat == kGcmA8R8G8B8)
			{
				f.bpp = 4; f.swapUnit = 4;
				if (remap == kRemapNone) { f.d3dFormat = 21; return true; }	// A8R8G8B8
				if (remap == kRemap1RGB) { f.d3dFormat = 22; return true; }	// X8R8G8B8
				if (remap == kRemapABGR) { f.d3dFormat = 32; return true; }	// A8B8G8R8
				if (remap == kRemap1BGR) { f.d3dFormat = 33; return true; }	// X8B8G8R8
				return false;
			}
			if (pixelFormat == kGcmW16Z16Y16X16F && remap == kRemapNone)	// B16G16R16A16F -> D3DFMT_A16B16G16R16F
			{ f.d3dFormat = 113; f.bpp = 8; f.swapUnit = 2; return true; }
			if (pixelFormat == kGcmW32Z32Y32X32F && remap == kRemapNone)	// B32G32R32A32F -> D3DFMT_A32B32G32R32F
			{ f.d3dFormat = 116; f.bpp = 16; f.swapUnit = 4; return true; }
			return false;
		}

		// GCMUnSwizzleSurface (ps3tex.cpp:432-476): Morton order (offset bit 2b is x bit b,
		// bit 2b+1 is y bit b) inside a min(w,h) square; a non-square surface is a run of
		// such squares along its longer side. Formats wider than 32 bits are swizzled as
		// 32-bit texels over a proportionally wider surface.
		void Unswizzle(const unsigned char* src, unsigned width, unsigned height, unsigned bpp, unsigned char* dst)
		{
			if (bpp > 4)
			{
				width *= bpp / 4;
				bpp = 4;
			}
			const unsigned dimension = width < height ? width : height;
			const unsigned steps = (width > height ? width : height) / dimension;
			unsigned stepOffset = 0, stepX = 0, stepY = 0;
			for (unsigned s = 0; s < steps; ++s)
			{
				for (unsigned offset = 0; offset < dimension * dimension; ++offset)
				{
					unsigned x = 0, y = 0;
					for (unsigned b = 0; b < 16; ++b)
					{
						x |= ((offset >> (2 * b)) & 1) << b;
						y |= ((offset >> (2 * b + 1)) & 1) << b;
					}
					x += stepX;
					y += stepY;
					for (unsigned i = 0; i < bpp; ++i)
						dst[(y * width + x) * bpp + i] = src[stepOffset + offset * bpp + i];
				}
				stepOffset += dimension * dimension * bpp;
				if (dimension == height)
					stepX += dimension;
				else
					stepY += dimension;
			}
		}

		void Swap(unsigned char* p, size_t n, unsigned unit)
		{
			if (unit < 2)
				return;
			for (size_t i = 0; i + unit <= n; i += unit)
				for (unsigned a = 0, b = unit - 1; a < b; ++a, --b)
				{
					const unsigned char t = p[i + a];
					p[i + a] = p[i + b];
					p[i + b] = t;
				}
		}

		// D3D9Texture2DSize (display/include/display/device/d3d9/D3D9DeviceTypes.h:590-667):
		// what PCLoadTexture reads per level; rounded up to 4 bytes for the 8- and 16-bit
		// formats, as NglPcTex::GetSurfaceBytes writes them (texture_export/source/targets/pctex.cpp:105-116).
		unsigned PcLevelSize(unsigned w, unsigned h, const GcmFormat& f)
		{
			unsigned size;
			if (f.dxt)
				size = ((w + 3) / 4 < 1 ? 1 : (w + 3) / 4) * ((h + 3) / 4 < 1 ? 1 : (h + 3) / 4) * f.bpp;
			else
				size = w * h * f.bpp;
			return (size + 3) & ~3u;
		}

		E_Result Format(const GcmTexture& t, GcmFormat& f, std::string& error)
		{
			if (!GetGcmFormat(t.format & ~(kGcmLN | kGcmUN), t.remap, f))
			{
				char msg[80];
				sprintf_s(msg, "GCM texture format 0x%02x remap 0x%x not handled", t.format, t.remap);
				return Fail(error, msg);
			}
			// Volume textures: the exporter refuses them (ps3tex.cpp:870-878). Dimension 2
			// covers 2D and cube maps (GCMSetTextureHeader, ps3tex.cpp:255-256).
			if (t.dimension != 2)
				return Fail(error, "GCM texture dimension other than 2");
			if (t.width == 0 || t.height == 0 || t.mipmap == 0)
				return Fail(error, "GCM texture header with a zero size or level count");
			return E_Result_Converted;
		}
	}

	bool ParseGcmHeader(const unsigned char* p, size_t avail, GcmTexture& t)
	{
		if (avail < kGcmHeaderSize)
			return false;
		// Serialise(ar, CellGcmTexture) (ps3tex.cpp:845-850) through a NativeToBigEndian
		// serialiser (ps3tex.cpp:862); U32 size follows (ps3tex.cpp:1012-1013).
		t.width = BE16(p + 0);
		t.height = BE16(p + 2);
		t.mipmap = p[4];
		t.format = p[5];
		t.dimension = p[6];
		t.cubemap = p[7];
		t.remap = BE32(p + 8);
		t.depth = BE16(p + 12);
		t.pitch = BE32(p + 14);
		t.offset = BE32(p + 18);
		t.size = BE32(p + 22);
		return true;
	}

	E_Result Ps3TextureDesc(const GcmTexture& t, std::vector<unsigned char>& out, std::string& error)
	{
		GcmFormat f;
		const E_Result r = Format(t, f, error);
		if (r != E_Result_Converted)
			return r;
		// TextureDesc (display/include/display/texture/TextureDesc.h:9-24). Depth 1 for 2D and
		// cube alike: the GCM loader forces a cube map's depth to 1 (GCMTextureLoad.cpp:101-102).
		PutLE32(out, t.width);
		PutLE32(out, t.height);
		PutLE32(out, 1);
		PutLE32(out, t.mipmap);
		PutLE32(out, f.d3dFormat);
		PutLE32(out, 0);					// Usage: unused by PCLoadTexture; retail PC files hold uninitialised bytes
		PutLE32(out, t.cubemap ? 5 : 3);	// Type: 3 2D, 5 cube (D3D9TextureLoad.cpp:91, 97)
		return E_Result_Converted;
	}

	E_Result Ps3TextureLevels(const GcmTexture& t, unsigned levels, const unsigned char* data, size_t dataSize,
		std::vector<unsigned char>& out, std::string& error)
	{
		GcmFormat f;
		const E_Result r = Format(t, f, error);
		if (r != E_Result_Converted)
			return r;
		if (levels == 0 || levels > t.mipmap)
			return Fail(error, "GCM texture level count out of range");

		// The exporter's walk (SerialOutTexture, ps3tex.cpp:955-1008): per face, per level.
		// Swizzled means no CELL_GCM_TEXTURE_LN (GCMIsTextureFormatSwizzled, ps3tex.cpp:142-145);
		// DXT data is never swizzled whatever the flag (ps3tex.cpp:976-978). Swizzled levels are
		// packed at w*h*bpp; the others are rows at the pitch of the top level, which stays
		// constant for linear formats and halves with the level for swizzle-flagged DXT
		// (ps3tex.cpp:996-999). Level sizes follow max(1,(n+1)/2), in 4x4 blocks for DXT
		// (ps3tex.cpp:962-967, 1001-1002). A cube map's six faces (the header's depth; see
		// NglPs3Tex::SerialInTexture, ps3tex.cpp:1041) each end on a 128-byte boundary
		// (ps3tex.cpp:1005-1007, GCMCalcTextureSize :244-245).
		const bool swizzled = (t.format & kGcmLN) == 0;
		const unsigned faces = t.cubemap ? 6 : 1;
		size_t src = 0;
		std::vector<unsigned char> level;
		for (unsigned face = 0; face < faces; ++face)
		{
			unsigned pitchWidth = t.width, width = t.width, height = t.height;
			if (f.dxt)
			{
				pitchWidth = (pitchWidth + 3) / 4 < 1 ? 1 : (pitchWidth + 3) / 4;
				width = (width + 3) / 4 < 1 ? 1 : (width + 3) / 4;
				height = (height + 3) / 4 < 1 ? 1 : (height + 3) / 4;
			}
			// the PC level dimensions (D3D9Texture2DSize's caller halves with >>= 1, D3D9DeviceTypes.h:671-681)
			unsigned pcWidth = t.width, pcHeight = t.height;
			for (unsigned l = 0; l < levels; ++l)
			{
				const size_t pitch = (size_t)pitchWidth * f.bpp;
				const size_t rowBytes = (size_t)width * f.bpp;
				level.assign(rowBytes * height, 0);
				if (swizzled && !f.dxt)
				{
					if (src + level.size() > dataSize)
						return Fail(error, "GCM image shorter than its levels");
					Unswizzle(data + src, width, height, f.bpp, &level[0]);
					src += level.size();
				}
				else
				{
					for (unsigned y = 0; y < height; ++y)
					{
						if (src + rowBytes > dataSize)
							return Fail(error, "GCM image shorter than its levels");
						memcpy(&level[y * rowBytes], data + src, rowBytes);
						src += pitch;
					}
				}
				Swap(level.empty() ? NULL : &level[0], level.size(), f.swapUnit);

				// The exporter's level size must be the one the PC loader reads. They part only
				// for chains whose sides are not powers of two, which the PC exporter does not mip
				// (pctex.cpp:285-290).
				const unsigned pcSize = PcLevelSize(pcWidth, pcHeight, f);
				if (((level.size() + 3) & ~(size_t)3) != pcSize)
				{
					char msg[120];
					sprintf_s(msg, "GCM level %u of %ux%u: %u bytes where the PC level takes %u", l, t.width, t.height,
						(unsigned)level.size(), pcSize);
					return Fail(error, msg);
				}
				out.insert(out.end(), level.begin(), level.end());
				out.resize(out.size() + (pcSize - level.size()), 0);	// pad bytes: retail PC holds whatever the exporter read past the surface

				if (swizzled)
					pitchWidth = (pitchWidth + 1) / 2 < 1 ? 1 : (pitchWidth + 1) / 2;
				width = (width + 1) / 2 < 1 ? 1 : (width + 1) / 2;
				height = (height + 1) / 2 < 1 ? 1 : (height + 1) / 2;
				if (pcWidth > 1) pcWidth >>= 1;
				if (pcHeight > 1) pcHeight >>= 1;
			}
			if (faces == 6)
				src = (src + 127) & ~(size_t)127;
		}
		if (src != dataSize)
		{
			char msg[96];
			sprintf_s(msg, "GCM image of %u bytes where %u levels take %u", (unsigned)dataSize, levels, (unsigned)src);
			return Fail(error, msg);
		}
		return E_Result_Converted;
	}

	E_Result Ps3ConvertTexturePayload(const unsigned char* p, size_t avail, size_t& consumed,
		std::vector<unsigned char>& out, std::string& error)
	{
		GcmTexture t;
		if (!ParseGcmHeader(p, avail, t))
			return Fail(error, "texture payload shorter than the GCM texture header");
		if (kGcmHeaderSize + (size_t)t.size > avail)
			return Fail(error, "GCM image runs past the block");
		consumed = kGcmHeaderSize + (size_t)t.size;
		E_Result r = Ps3TextureDesc(t, out, error);
		if (r != E_Result_Converted)
			return r;
		return Ps3TextureLevels(t, t.mipmap, p + kGcmHeaderSize, t.size, out, error);
	}

	E_Result Ps3ConvertTextureLodHeader(const unsigned char* header, size_t size, std::vector<unsigned char>& out, std::string& error)
	{
		// LodWriter<Texture> writes JUST_HEADER: the 26 bytes (renderingdata/source/noninterface/build/archive.cpp:446-452;
		// the reader's buffer is sizeof(CellGcmTexture)+12, target/archive.cpp:876)
		GcmTexture t;
		if (size != kGcmHeaderSize || !ParseGcmHeader(header, size, t))
			return Fail(error, "GCM texture header of unexpected size");
		return Ps3TextureDesc(t, out, error);
	}

	E_Result Ps3ConvertTextureLodData(const unsigned char* header, int lod, const unsigned char* data, size_t dataSize,
		std::vector<unsigned char>& out, std::string& error)
	{
		GcmTexture t;
		if (!ParseGcmHeader(header, kGcmHeaderSize, t))
			return Fail(error, "no GCM texture header");
		if (t.cubemap)
			return Fail(error, "resource block texture other than 2D not handled");
		// LodCalculator::GetMipLevelsForLod (renderingdata/source/noninterface/lods.cpp:47-78): lod 0
		// writes the header's whole chain, any later lod its top level only; JUST_DATA of a
		// texture cut to those levels (build/archive.cpp:455-481). 'data' is the inflated image.
		return Ps3TextureLevels(t, lod == 0 ? t.mipmap : 1, data, dataSize, out, error);
	}
}
