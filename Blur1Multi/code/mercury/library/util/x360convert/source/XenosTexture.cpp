// [Blur1Multi] Xenos texture layout. Port of Blur2Test _tools/b2_extract_textures.py;
// see XenosTexture.h.

#include "XenosTexture.h"
#include "ChunkFile.h"

#include <stdio.h>
#include <string.h>
#include <map>

namespace X360Convert
{
	void ParseFetch(const unsigned char* p, XenosFetch& f)
	{
		const unsigned d0 = BE32(p), d1 = BE32(p + 4), d2 = BE32(p + 8), d3 = BE32(p + 12), d4 = BE32(p + 16), d5 = BE32(p + 20);
		f.type = d0 & 3;
		f.signs = (d0 >> 2) & 0xff;
		f.pitchDiv32 = (d0 >> 22) & 0x1ff;
		f.tiled = d0 >> 31;
		f.format = d1 & 0x3f;
		f.endian = (d1 >> 6) & 3;
		f.baseAddress = d1 >> 12;
		f.width = (d2 & 0x1fff) + 1;
		f.height = ((d2 >> 13) & 0x1fff) + 1;
		f.numFormat = d3 & 1;
		f.swizzle = (d3 >> 1) & 0xfff;
		f.mipMinLevel = (d4 >> 2) & 15;
		f.mipMaxLevel = (d4 >> 6) & 15;
		f.dimension = (d5 >> 9) & 3;
		f.packedMips = (d5 >> 11) & 1;
		f.mipAddress = d5 >> 12;
	}

	bool GetXenosFormat(unsigned format, XenosFormat& out)
	{
		// b2_extract_textures.py FORMAT_INFO
		switch (format)
		{
		case 0x02: out.name = "8";       out.blockW = 1; out.blockH = 1; out.bytesPerBlock = 1;  return true;
		case 0x06: out.name = "8_8_8_8"; out.blockW = 1; out.blockH = 1; out.bytesPerBlock = 4;  return true;
		case 0x0A: out.name = "8_8";     out.blockW = 1; out.blockH = 1; out.bytesPerBlock = 2;  return true;
		case 0x12: out.name = "DXT1";    out.blockW = 4; out.blockH = 4; out.bytesPerBlock = 8;  return true;
		case 0x13: out.name = "DXT2_3";  out.blockW = 4; out.blockH = 4; out.bytesPerBlock = 16; return true;
		case 0x14: out.name = "DXT4_5";  out.blockW = 4; out.blockH = 4; out.bytesPerBlock = 16; return true;
		default: return false;
		}
	}

	bool EndianNormalize(unsigned char* d, size_t n, unsigned mode)
	{
		if (mode == 0)
			return true;
		const size_t unit = mode == 1 ? 2 : 4;
		if (n % unit)
			return false;
		for (size_t o = 0; o < n; o += unit)
		{
			unsigned char* p = d + o;
			unsigned char t;
			if (mode == 1)			// 8 in 16
			{
				t = p[0]; p[0] = p[1]; p[1] = t;
			}
			else if (mode == 2)		// 8 in 32
			{
				t = p[0]; p[0] = p[3]; p[3] = t;
				t = p[1]; p[1] = p[2]; p[2] = t;
			}
			else					// 16 in 32
			{
				t = p[0]; p[0] = p[2]; p[2] = t;
				t = p[1]; p[1] = p[3]; p[3] = t;
			}
		}
		return true;
	}

	namespace
	{
		unsigned Align(unsigned v, unsigned a) { return (v + a - 1) & ~(a - 1); }
		unsigned BitLength(unsigned v) { unsigned n = 0; while (v) { ++n; v >>= 1; } return n; }
		unsigned NextPow2(unsigned v) { return 1u << BitLength(v - 1); }
		unsigned Log2Ceil(unsigned v) { return BitLength(v - 1); }
		unsigned MaxU(unsigned a, unsigned b) { return a > b ? a : b; }
		unsigned MinU(unsigned a, unsigned b) { return a < b ? a : b; }

		unsigned LogBpp(unsigned pitch) { return (pitch >> 2) + ((pitch >> 1) >> (pitch >> 2)); }

		// tiled_x / tiled_y: 'width' is the tiled surface's pitch in blocks, 'pitch' the bytes per block.
		unsigned TiledX(unsigned offset, unsigned width, unsigned pitch)
		{
			const unsigned alignedWidth = (width + 31) & ~31u;
			const unsigned logBpp = LogBpp(pitch);
			const unsigned offsetB = offset << logBpp;
			const unsigned offsetT = ((offsetB & ~4095u) >> 3) + ((offsetB & 1792) >> 2) + (offsetB & 63);
			const unsigned offsetM = offsetT >> (7 + logBpp);
			const unsigned macroX = (offsetM % (alignedWidth >> 5)) << 2;
			const unsigned tile = (((offsetT >> (5 + logBpp)) & 2) + (offsetB >> 6)) & 3;
			const unsigned macro = (macroX + tile) << 3;
			const unsigned micro = ((((offsetT >> 1) & ~15u) + (offsetT & 15)) & ((pitch << 3) - 1)) >> logBpp;
			return macro + micro;
		}

		unsigned TiledY(unsigned offset, unsigned width, unsigned pitch)
		{
			const unsigned alignedWidth = (width + 31) & ~31u;
			const unsigned logBpp = LogBpp(pitch);
			const unsigned offsetB = offset << logBpp;
			const unsigned offsetT = ((offsetB & ~4095u) >> 3) + ((offsetB & 1792) >> 2) + (offsetB & 63);
			const unsigned offsetM = offsetT >> (7 + logBpp);
			const unsigned macroY = (offsetM / (alignedWidth >> 5)) << 2;
			const unsigned tile = ((offsetT >> (6 + logBpp)) & 1) + ((offsetB & 2048) >> 10);
			const unsigned macro = (macroY + tile) << 3;
			const unsigned micro = ((((offsetT & (((pitch << 6) - 1) & ~31u)) + ((offsetT & 15) << 1)) >> (3 + logBpp)) & ~1u);
			return macro + micro + ((offsetT & 16) >> 4);
		}

		unsigned PackedMipLevel(unsigned width, unsigned height)
		{
			const unsigned log2Size = Log2Ceil(MinU(width, height));
			return log2Size > 4 ? log2Size - 4 : 0;
		}

		bool PackedMipOffset(unsigned width, unsigned height, unsigned blockW, unsigned blockH, unsigned mip,
			unsigned& left, unsigned& top)
		{
			// Xenia texture_util::GetPackedMipOffset, 2D/cube case
			const unsigned log2Width = Log2Ceil(width), log2Height = Log2Ceil(height);
			const unsigned log2Size = MinU(log2Width, log2Height);
			if (log2Size > 4 + mip)
				return false;
			const unsigned packedBase = log2Size > 4 ? log2Size - 4 : 0;
			const unsigned packedMip = mip - packedBase;
			unsigned xTexels, yTexels;
			if (packedMip < 3)
			{
				if (log2Width > log2Height) { xTexels = 0; yTexels = 16 >> packedMip; }
				else { xTexels = 16 >> packedMip; yTexels = 0; }
			}
			else
			{
				if (log2Width > log2Height) { xTexels = (1u << (log2Width - packedBase)) >> (packedMip - 2); yTexels = 0; }
				else { xTexels = 0; yTexels = (1u << (log2Height - packedBase)) >> (packedMip - 2); }
			}
			left = xTexels / blockW;
			top = yTexels / blockH;
			return true;
		}

		// _untile_block_region: one block rectangle out of a tiled slice.
		bool UntileRegion(const unsigned char* data, size_t size, unsigned pitchBlocks, unsigned bpb,
			unsigned left, unsigned top, unsigned widthBlocks, unsigned heightBlocks,
			std::vector<unsigned char>& out, std::string& error, const char* context)
		{
			out.assign((size_t)widthBlocks * heightBlocks * bpb, 0);
			std::vector<unsigned char> filled((size_t)widthBlocks * heightBlocks, 0);
			const size_t sourceBlocks = size / bpb;
			for (size_t s = 0; s < sourceBlocks; ++s)
			{
				const unsigned x = TiledX((unsigned)s, pitchBlocks, bpb);
				const unsigned y = TiledY((unsigned)s, pitchBlocks, bpb);
				if (x < left || x >= left + widthBlocks || y < top || y >= top + heightBlocks)
					continue;
				const size_t t = (size_t)(y - top) * widthBlocks + (x - left);
				if (filled[t])
				{
					char msg[160];
					sprintf_s(msg, "%s: tiled mapping writes block (%u,%u) twice", context, x, y);
					error = msg;
					return false;
				}
				memcpy(&out[t * bpb], data + s * bpb, bpb);
				filled[t] = 1;
			}
			size_t missing = 0;
			for (size_t i = 0; i < filled.size(); ++i)
				missing += !filled[i];
			if (missing)
			{
				char msg[160];
				sprintf_s(msg, "%s: tiled mapping left %u requested blocks unfilled", context, (unsigned)missing);
				error = msg;
				return false;
			}
			return true;
		}

		bool Fail(std::string& error, const char* text)
		{
			error = text;
			return false;
		}
	}

	bool DecodeXenos2DChain(const unsigned char* payload, size_t size, const XenosFetch& f,
		const XenosFormat& fmt, std::vector<std::vector<unsigned char> >& levels, std::string& error)
	{
		const unsigned width = f.width, height = f.height;
		const unsigned bw = fmt.blockW, bh = fmt.blockH, bpb = fmt.bytesPerBlock;
		if (f.mipMaxLevel > BitLength(MaxU(width, height)) - 1)
			return Fail(error, "mip max exceeds the chain");
		const unsigned basePitchTexels = f.pitchDiv32 << 5;
		if (basePitchTexels < width)
			return Fail(error, "base pitch smaller than width");
		const unsigned basePitchBlocks = Align(basePitchTexels, bw) / bw;
		const unsigned baseRows = Align(Align(height, bh) / bh, 32);
		const unsigned baseStride = Align(basePitchBlocks * baseRows * bpb, 0x1000);

		const unsigned mipCount = f.mipMaxLevel + 1;
		const unsigned packedLevel = f.packedMips ? PackedMipLevel(width, height) : mipCount;
		const unsigned storedLevelLast = MinU(f.mipMaxLevel, packedLevel);

		struct Region { unsigned left, top, w, h; };
		std::vector<Region> region(mipCount);
		for (unsigned level = 0; level < mipCount; ++level)
		{
			Region& r = region[level];
			r.left = r.top = 0;
			if (level >= packedLevel && !PackedMipOffset(width, height, bw, bh, level, r.left, r.top))
				return Fail(error, "mip is not in the packed tail");
			r.w = (MaxU(width >> level, 1) + bw - 1) / bw;
			r.h = (MaxU(height >> level, 1) + bh - 1) / bh;
		}

		struct Layout { unsigned offset, pitchBlocks, stride, usedRows; };
		std::map<unsigned, Layout> layouts;
		unsigned mipCursor = 0;
		for (unsigned level = 1; level <= storedLevelLast; ++level)
		{
			const unsigned levelPitchTexels = MaxU(NextPow2(width) >> level, 1);
			const unsigned levelRowsTexels = MaxU(NextPow2(height) >> level, 1);
			const unsigned pitchBlocks = Align(Align(levelPitchTexels, bw) / bw, 32);
			const unsigned rowsBlocks = Align(Align(levelRowsTexels, bh) / bh, 32);
			const unsigned stride = Align(pitchBlocks * rowsBlocks * bpb, 0x1000);
			unsigned usedRows = 0;
			for (unsigned stored = level; stored < mipCount; ++stored)
				if (MinU(stored, packedLevel) == level)
					usedRows = MaxU(usedRows, region[stored].top + region[stored].h);
			Layout l = { mipCursor, pitchBlocks, stride, usedRows };
			layouts[level] = l;
			mipCursor += stride;
		}

		unsigned mipStart, extentMin, extentMax;
		if (storedLevelLast >= 1)
		{
			if (f.mipAddress == 0)
				return Fail(error, "mip chain has no serialised mip-page offset");
			mipStart = f.mipAddress << 12;
			if (mipStart != baseStride)
				return Fail(error, "mip allocation does not start after the base slice");
			const Layout& last = layouts[storedLevelLast];
			extentMax = mipStart + mipCursor;
			extentMin = mipStart + last.offset + Align(last.usedRows * last.pitchBlocks * bpb, 0x1000);
		}
		else
		{
			mipStart = baseStride;
			if (f.packedMips && mipCount > 1 && f.mipAddress)
				return Fail(error, "packed base tile also declares a mip page");
			unsigned used = 0;
			for (unsigned level = 0; level < mipCount; ++level)
				used = MaxU(used, region[level].top + region[level].h);
			extentMax = baseStride;
			extentMin = Align(used * basePitchBlocks * bpb, 0x1000);
		}
		if (size < extentMin || size > extentMax || (size % 0x1000))
		{
			char msg[200];
			sprintf_s(msg, "2D allocation is 0x%x bytes; layout needs 0x%x..0x%x on 4 KB pages", (unsigned)size, extentMin, extentMax);
			return Fail(error, msg);
		}

		levels.assign(mipCount, std::vector<unsigned char>());
		const size_t baseSize = MinU(baseStride, (unsigned)size);
		for (unsigned level = 0; level < mipCount; ++level)
		{
			const unsigned storageLevel = MinU(level, packedLevel);
			const unsigned char* levelData;
			size_t levelSize;
			unsigned pitchBlocks;
			if (storageLevel == 0)
			{
				levelData = payload;
				levelSize = baseSize;
				pitchBlocks = basePitchBlocks;
			}
			else
			{
				const Layout& l = layouts[storageLevel];
				const size_t start = mipStart + l.offset;
				levelData = payload + start;
				levelSize = start < size ? MinU(l.stride, (unsigned)(size - start)) : 0;
				pitchBlocks = l.pitchBlocks;
			}
			char context[32];
			sprintf_s(context, "mip%u", level);
			const Region& r = region[level];
			if (!UntileRegion(levelData, levelSize, pitchBlocks, bpb, r.left, r.top, r.w, r.h, levels[level], error, context))
				return false;
		}
		return true;
	}

	bool DecodeXenosCube(const unsigned char* payload, size_t size, const XenosFetch& f,
		const XenosFormat& fmt, std::vector<std::vector<unsigned char> >& faceLevels, unsigned& mipCount, std::string& error)
	{
		const unsigned width = f.width, height = f.height;
		const unsigned bw = fmt.blockW, bh = fmt.blockH, bpb = fmt.bytesPerBlock;
		if (width != height)
			return Fail(error, "cube faces are not square");
		if (f.mipMaxLevel > BitLength(MaxU(width, height)) - 1)
			return Fail(error, "mip max exceeds the chain");
		const unsigned basePitchTexels = f.pitchDiv32 << 5;
		if (basePitchTexels < width)
			return Fail(error, "base pitch smaller than width");
		const unsigned basePitchBlocks = Align(basePitchTexels, bw) / bw;
		const unsigned baseRows = Align(Align(height, bh) / bh, 32);
		const unsigned baseStride = Align(basePitchBlocks * baseRows * bpb, 0x1000);

		mipCount = f.mipMaxLevel + 1;
		const unsigned packedLevel = f.packedMips ? PackedMipLevel(width, height) : mipCount;
		const unsigned storedLevelLast = MinU(f.mipMaxLevel, packedLevel);
		struct Layout { unsigned offset, pitchBlocks, sliceStride; };
		std::map<unsigned, Layout> layouts;
		unsigned mipCursor = 0;
		for (unsigned level = 1; level <= storedLevelLast; ++level)
		{
			const unsigned levelPitchTexels = MaxU(NextPow2(width) >> level, 1);
			const unsigned levelRowsTexels = MaxU(NextPow2(height) >> level, 1);
			const unsigned pitchBlocks = Align(Align(levelPitchTexels, bw) / bw, 32);
			const unsigned rowsBlocks = Align(Align(levelRowsTexels, bh) / bh, 32);
			const unsigned sliceStride = Align(pitchBlocks * rowsBlocks * bpb, 0x1000);
			Layout l = { mipCursor, pitchBlocks, sliceStride };
			layouts[level] = l;
			mipCursor += sliceStride * 6;
		}
		const unsigned baseExtent = baseStride * 6;
		unsigned mipStart, expected;
		if (mipCount > 1)
		{
			if (f.mipAddress == 0)
				return Fail(error, "mip chain has no serialised mip-page offset");
			mipStart = f.mipAddress << 12;
			if (mipStart != baseExtent)
				return Fail(error, "mip allocation does not start after six base slices");
			expected = mipStart + mipCursor;
		}
		else
		{
			mipStart = (unsigned)size;
			expected = baseExtent;
		}
		if (size != expected)
			return Fail(error, "cube allocation size does not match its layout");

		faceLevels.assign(6 * mipCount, std::vector<unsigned char>());
		for (unsigned face = 0; face < 6; ++face)
		{
			if (!UntileRegion(payload + face * baseStride, baseStride, basePitchBlocks, bpb, 0, 0,
				(width + bw - 1) / bw, (height + bh - 1) / bh, faceLevels[face * mipCount], error, "cube mip0"))
				return false;
		}
		for (unsigned level = 1; level < mipCount; ++level)
		{
			const unsigned storageLevel = MinU(level, packedLevel);
			if (!layouts.count(storageLevel))
				return Fail(error, "cube mip has no storage level");
			const Layout& l = layouts[storageLevel];
			unsigned left = 0, top = 0;
			if (level >= packedLevel && !PackedMipOffset(width, height, bw, bh, level, left, top))
				return Fail(error, "cube mip is not in the packed tail");
			const unsigned wb = (MaxU(width >> level, 1) + bw - 1) / bw;
			const unsigned hb = (MaxU(height >> level, 1) + bh - 1) / bh;
			for (unsigned face = 0; face < 6; ++face)
			{
				const size_t start = mipStart + l.offset + face * l.sliceStride;
				if (start + l.sliceStride > size)
					return Fail(error, "cube mip storage truncated");
				if (!UntileRegion(payload + start, l.sliceStride, l.pitchBlocks, bpb, left, top, wb, hb,
					faceLevels[face * mipCount + level], error, "cube mip"))
					return false;
			}
		}
		return true;
	}
}
