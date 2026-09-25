// [Blur1Multi] Resource bundles (.bpr, "BZR2" version 3): Xbox 360 -> PC.
//
// Reader: display/source/bundler/ResourceBundleReaderImpl3.cpp and
// ResourceBundleSingle.cpp. File = C_BundleHeader (magic, U8 version, U8 platform
// (1 Xbox 360, 2 PC), U16 resourceInfoSize, U16 stringTableOffset, U16 flags,
// U32 uncompressedDataSize, U32 compressedDataSize), then resourceInfoSize bytes
// of "system memory" (C_ResourceHeader chain linked by offsetNext, then the
// string table at stringTableOffset), then the texel data.
// C_ResourceTexture2D = {U16 offsetNext, U16 offsetString, U32 offsetData, U16 type,
// U16 flags, U32 uncompressedDataSize, U32 compressedDataSize, U16 format,
// U16 mipLevels, U16 width, U16 height}.
// 360 writer (tools/ResourceBundler export_resource/360/ExportArrayOfTextures2D360.cpp):
// A8R8G8B8 levels are 4-byte reversed by the exporter, then every level is tiled into
// an XGSetTextureHeaderEx(defaults) allocation with XGTileTextureLevel; the DXT
// blocks come out 8-in-16 swapped (the exporter's own swap skips DXT, so the tiling
// step does it; established by comparison with the retail PC bundles), L8 unswapped. PC: every level tightly
// packed (Display::GetTexture2DSize), resources back to back. The resource flags
// word is not read by the PC loader and differs between the platforms' files; it is
// written as 0.

#include <x360convert/X360Convert.h>

#include "ChunkFile.h"
#include "XenosTexture.h"

#include <stdio.h>
#include <string.h>

namespace X360Convert
{
	namespace
	{
		E_Result Fail(std::string& error, const std::string& text)
		{
			error = text;
			return E_Result_Failed;
		}

		// ResourceBundlerDefines.h E_TextureFormat -> Xenos format
		bool XenosFormatOf(unsigned bundleFormat, unsigned& xenos)
		{
			switch (bundleFormat)
			{
			case 1: xenos = 0x12; return true;	// DXT1
			case 3: xenos = 0x13; return true;	// DXT3
			case 5: xenos = 0x14; return true;	// DXT5
			case 6: xenos = 0x06; return true;	// A8R8G8B8
			case 7: xenos = 0x02; return true;	// L8
			default: return false;
			}
		}
	}

	// PS3 bundles (platform 4, E_Platform_PS3_LibGCM, ResourceBundlerDefines.h:12-20). The
	// bundler's PS3 options (tools/ResourceBundler/source/Bundler.cpp:276-282): big-endian file,
	// texels not swapped, not tiled, resources on 128 bytes. ExportArrayOfTextures2DGeneric::_Export
	// (export_resource/Generic/ExportArrayOfTextures2DGeneric.cpp:39-230) writes each level at
	// ProcessedMipmapChain::GetLevelSize (process_texture/ProcessedMipmapChain.cpp:139-143,
	// floor(w/blockW)*floor(h/blockW)*bytesPerBlock: a DXT level under 4 texels takes 0 bytes on
	// every platform), and for L8, A8R8G8B8 and A32R32G32B32 on PS3 each row at the top
	// level's pitch (padPitchToWidth0, :44-53, :183-208; the runtime reads them the same way,
	// ResourceBundleSingle.cpp:201-212 with GCMContextSPU.cpp:343-355). PC: the same levels
	// without the row padding, back to back (alignment 1, Bundler.cpp:292-296).
	static E_Result ConvertPs3Bundle(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error)
	{
		if (inSize < 20 || memcmp(in, "BZR2", 4) != 0)
			return Fail(error, "not a big-endian BZR2 bundle");
		const unsigned version = in[4], platform = in[5];
		const unsigned infoSize = BE16(in + 6), stringTable = BE16(in + 8), flags = BE16(in + 10);
		const unsigned dataSize = BE32(in + 12), compressedSize = BE32(in + 16);
		if (version != 3 || platform != 4)
			return Fail(error, "bundle is not version 3 for PS3 (libgcm)");
		if ((flags & 0xf) != 0 || compressedSize != 0)
			return Fail(error, "compressed bundles not handled");
		if (20 + (size_t)infoSize + dataSize > inSize)
			return Fail(error, "bundle sizes run past the file");
		const unsigned char* sys = in + 20;
		const unsigned char* data = sys + infoSize;

		std::vector<unsigned char> pcSys(sys, sys + infoSize);
		std::vector<unsigned char> pcData;
		size_t off = 0;
		for (unsigned index = 0;; ++index)
		{
			if (off + 28 > stringTable)
				return Fail(error, "resource header runs into the string table");
			const unsigned char* h = sys + off;
			const unsigned next = BE16(h), type = BE16(h + 8);
			const unsigned offsetData = BE32(h + 4), csize = BE32(h + 16);
			const unsigned format = BE16(h + 20), levels = BE16(h + 22), width = BE16(h + 24), height = BE16(h + 26);
			if (type != 1)
			{
				char msg[64];
				sprintf_s(msg, "resource %u: type %u not handled (only 2D)", index, type);
				return Fail(error, msg);
			}
			// the formats C_ResourceBundleSingle::GetFormat takes (ResourceBundleSingle.cpp:45-68);
			// block width and bytes per block from ResourceBundlerDefines.h g_textureFormat* tables
			unsigned blockW, blockBytes;
			bool padRows;
			switch (format)
			{
			case 1: blockW = 4; blockBytes = 8; padRows = false; break;		// DXT1
			case 3: blockW = 4; blockBytes = 16; padRows = false; break;	// DXT3
			case 5: blockW = 4; blockBytes = 16; padRows = false; break;	// DXT5
			case 6: blockW = 1; blockBytes = 4; padRows = true; break;		// A8R8G8B8
			case 7: blockW = 1; blockBytes = 1; padRows = true; break;		// L8
			default:
				{
					char msg[64];
					sprintf_s(msg, "resource %u: bundle format %u not handled", index, format);
					return Fail(error, msg);
				}
			}
			if (csize != 0)
				return Fail(error, "compressed resource not handled");

			const size_t pcOffset = pcData.size();
			const size_t pitch0 = (size_t)(width / blockW) * blockBytes;
			size_t src = offsetData;
			unsigned w = width, hgt = height;
			for (unsigned l = 0; l < levels; ++l)
			{
				const size_t row = (size_t)(w / blockW) * blockBytes;
				const unsigned rows = hgt / blockW;
				const size_t stride = padRows ? pitch0 : row;
				if (rows && src + (rows - 1) * stride + row > dataSize)
				{
					char msg[96];
					sprintf_s(msg, "resource %u (%ux%u, %u levels): data outside the bundle", index, width, height, levels);
					return Fail(error, msg);
				}
				for (unsigned y = 0; y < rows; ++y)
					pcData.insert(pcData.end(), data + src + y * stride, data + src + y * stride + row);
				src += stride * rows;
				if (w > 1) w >>= 1;
				if (hgt > 1) hgt >>= 1;
			}
			const size_t pcSize = pcData.size() - pcOffset;

			unsigned char* o = &pcSys[off];
			const unsigned offsetString = BE16(h + 2);
			o[0] = (unsigned char)next; o[1] = (unsigned char)(next >> 8);
			o[2] = (unsigned char)offsetString; o[3] = (unsigned char)(offsetString >> 8);
			for (int i = 0; i < 4; ++i) o[4 + i] = (unsigned char)(pcOffset >> (8 * i));
			o[8] = (unsigned char)type; o[9] = (unsigned char)(type >> 8);
			o[10] = 0; o[11] = 0;			// flags: not read by the PC loader
			for (int i = 0; i < 4; ++i) o[12 + i] = (unsigned char)(pcSize >> (8 * i));
			for (int i = 0; i < 4; ++i) o[16 + i] = 0;
			const unsigned tail[4] = { format, levels, width, height };
			for (int i = 0; i < 4; ++i) { o[20 + i * 2] = (unsigned char)tail[i]; o[21 + i * 2] = (unsigned char)(tail[i] >> 8); }

			if (!next)
				break;
			off += next;
		}

		out.clear();
		out.reserve(20 + infoSize + pcData.size());
		const char magic[4] = { '2', 'R', 'Z', 'B' };
		out.insert(out.end(), magic, magic + 4);
		out.push_back((unsigned char)version);
		out.push_back(2);					// E_Platform_PC
		PutLE16(out, infoSize);
		PutLE16(out, stringTable);
		PutLE16(out, flags);
		PutLE32(out, (unsigned)pcData.size());
		PutLE32(out, 0);
		out.insert(out.end(), pcSys.begin(), pcSys.end());
		out.insert(out.end(), pcData.begin(), pcData.end());
		return E_Result_Converted;
	}

	E_Result ConvertBundle(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error)
	{
		if (GetSource() == E_Source_PS3)
			return ConvertPs3Bundle(in, inSize, out, error);
		if (inSize < 20 || memcmp(in, "BZR2", 4) != 0)
			return Fail(error, "not a big-endian BZR2 bundle");
		const unsigned version = in[4], platform = in[5];
		const unsigned infoSize = BE16(in + 6), stringTable = BE16(in + 8), flags = BE16(in + 10);
		const unsigned dataSize = BE32(in + 12), compressedSize = BE32(in + 16);
		if (version != 3 || platform != 1)
			return Fail(error, "bundle is not version 3 for Xbox 360");
		if ((flags & 0xf) != 0 || compressedSize != 0)
			return Fail(error, "compressed bundles not handled");
		if (20 + (size_t)infoSize + dataSize != inSize)
			return Fail(error, "bundle sizes do not add up to the file");
		const unsigned char* sys = in + 20;
		const unsigned char* data = sys + infoSize;

		std::vector<unsigned char> pcSys(sys, sys + infoSize);
		std::vector<unsigned char> pcData;
		size_t off = 0;
		for (unsigned index = 0;; ++index)
		{
			if (off + 28 > stringTable)
				return Fail(error, "resource header runs into the string table");
			const unsigned char* h = sys + off;
			const unsigned next = BE16(h), type = BE16(h + 8);
			const unsigned offsetData = BE32(h + 4), size = BE32(h + 12), csize = BE32(h + 16);
			const unsigned format = BE16(h + 20), levels = BE16(h + 22), width = BE16(h + 24), height = BE16(h + 26);
			if (type != 1)
			{
				char msg[64];
				sprintf_s(msg, "resource %u: type %u not handled (only 2D)", index, type);
				return Fail(error, msg);
			}
			unsigned xenos;
			XenosFormat fmt;
			if (!XenosFormatOf(format, xenos) || !GetXenosFormat(xenos, fmt))
			{
				char msg[64];
				sprintf_s(msg, "resource %u: bundle format %u not handled", index, format);
				return Fail(error, msg);
			}
			if (csize != 0 || (size_t)offsetData + size > dataSize)
				return Fail(error, "resource data outside the bundle");

			XenosFetch f;
			MakeXgFetch2D(width, height, levels, xenos, fmt, f);
			std::vector<std::vector<unsigned char> > mips;
			std::string e;
			if (!DecodeXenos2DChain(data + offsetData, size, f, fmt, mips, e))
			{
				char msg[96];
				sprintf_s(msg, "resource %u (%ux%u, %u levels): ", index, width, height, levels);
				return Fail(error, msg + e);
			}
			const size_t pcOffset = pcData.size();
			for (size_t l = 0; l < mips.size(); ++l)
			{
				if (xenos == 0x06)	// undo the exporter's 4-byte reversal
					EndianNormalize(mips[l].empty() ? NULL : &mips[l][0], mips[l].size(), 2);
				else if (fmt.blockW == 4)	// DXT: 8-in-16, applied when the exporter tiled the level (checked against retail PC)
					EndianNormalize(mips[l].empty() ? NULL : &mips[l][0], mips[l].size(), 1);
				pcData.insert(pcData.end(), mips[l].begin(), mips[l].end());
			}
			const size_t pcSize = pcData.size() - pcOffset;

			unsigned char* o = &pcSys[off];
			const unsigned offsetString = BE16(h + 2);
			o[0] = (unsigned char)next; o[1] = (unsigned char)(next >> 8);
			o[2] = (unsigned char)offsetString; o[3] = (unsigned char)(offsetString >> 8);
			for (int i = 0; i < 4; ++i) o[4 + i] = (unsigned char)(pcOffset >> (8 * i));
			o[8] = (unsigned char)type; o[9] = (unsigned char)(type >> 8);
			o[10] = 0; o[11] = 0;			// flags
			for (int i = 0; i < 4; ++i) o[12 + i] = (unsigned char)(pcSize >> (8 * i));
			for (int i = 0; i < 4; ++i) o[16 + i] = 0;
			const unsigned tail[4] = { format, levels, width, height };
			for (int i = 0; i < 4; ++i) { o[20 + i * 2] = (unsigned char)tail[i]; o[21 + i * 2] = (unsigned char)(tail[i] >> 8); }

			if (!next)
				break;
			off += next;
		}

		out.clear();
		out.reserve(20 + infoSize + pcData.size());
		const char magic[4] = { '2', 'R', 'Z', 'B' };
		out.insert(out.end(), magic, magic + 4);
		out.push_back((unsigned char)version);
		out.push_back(2);					// E_Platform_PC
		PutLE16(out, infoSize);
		PutLE16(out, stringTable);
		PutLE16(out, flags);
		PutLE32(out, (unsigned)pcData.size());
		PutLE32(out, 0);
		out.insert(out.end(), pcSys.begin(), pcSys.end());
		out.insert(out.end(), pcData.begin(), pcData.end());
		return E_Result_Converted;
	}
}
