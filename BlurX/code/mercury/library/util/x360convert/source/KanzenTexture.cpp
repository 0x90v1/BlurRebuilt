// [BlurX] Kanzen compression type 3 textures. See KanzenTexture.h.

#include "KanzenTexture.h"
#include "XenosTexture.h"

#include <xcompress.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace X360Convert
{
	namespace
	{
		const unsigned kControlSize = 88;			// bytes before the first nested stream
		const unsigned kTextureWindow = 0x80000;	// Blur2Test TEXTURE_XMEM_WINDOW

		unsigned Be16(const unsigned char* p) { return (unsigned)p[0] << 8 | p[1]; }
		unsigned Be32(const unsigned char* p) { return (unsigned)p[0] << 24 | (unsigned)p[1] << 16 | (unsigned)p[2] << 8 | p[3]; }

		bool Fail(std::string& error, const char* format, ...)
		{
			char text[256];
			va_list args;
			va_start(args, format);
			vsprintf_s(text, format, args);
			va_end(args);
			error = text;
			return false;
		}

		// measure_xmem: the output size of an XMem frame chain of exactly 'stored'
		// bytes ending in the five-byte terminator; 0 when it is not one.
		unsigned MeasureXMem(const unsigned char* data, size_t size, size_t offset, size_t stored)
		{
			// (overflow-safe on 32-bit builds: 'stored' comes from the data)
			if (offset > size || stored > size - offset || stored < 7)
				return 0;
			const size_t end = offset + stored;
			for (size_t i = end - 5; i < end; ++i)
				if (data[i])
					return 0;
			size_t cursor = offset;
			unsigned output = 0;
			while (cursor < end - 5)
			{
				unsigned outputSize, compressedSize;
				if (data[cursor] == 0xFF)
				{
					if (cursor + 5 > end - 5)
						return 0;
					outputSize = Be16(data + cursor + 1);
					compressedSize = Be16(data + cursor + 3);
					cursor += 5;
				}
				else
				{
					if (cursor + 2 > end - 5)
						return 0;
					outputSize = 0x8000;
					compressedSize = Be16(data + cursor);
					cursor += 2;
				}
				if (outputSize == 0 || compressedSize == 0 || compressedSize > end - 5 - cursor)
					return 0;
				output += outputSize;
				cursor += compressedSize;
			}
			return cursor == end - 5 ? output : 0;
		}

		struct Nested
		{
			size_t sizeOffset, payload, stored;
			unsigned logical;
		};

		// nested_xmem_streams: the non-overlapping size-prefixed XMem streams in [start, end).
		void NestedStreams(const unsigned char* data, size_t start, size_t end, std::vector<Nested>& out)
		{
			size_t cursor = start;
			for (size_t sizeOffset = start; sizeOffset + 10 < end; ++sizeOffset)
			{
				if (sizeOffset < cursor)
					continue;
				const size_t stored = Be32(data + sizeOffset);
				const size_t payload = sizeOffset + 4;
				if (stored < 7 || stored > end - payload)
					continue;
				const unsigned logical = MeasureXMem(data, end, payload, stored);
				if (!logical)
					continue;
				Nested n = { sizeOffset, payload, stored, logical };
				out.push_back(n);
				cursor = payload + stored;
			}
		}

		bool DecompressStream(const unsigned char* p, size_t stored, unsigned logical, std::vector<unsigned char>& out)
		{
			XMEMCODEC_PARAMETERS_LZX params = { 0, kTextureWindow, 0 };
			XMEMDECOMPRESSION_CONTEXT context = 0;
			if (XMemCreateDecompressionContext(XMEMCODEC_LZX, &params, 0, &context) < 0)
				return false;
			const size_t at = out.size();
			out.resize(at + logical);
			SIZE_T produced = logical;
			const HRESULT hr = XMemDecompress(context, &out[at], &produced, p, stored);
			XMemDestroyDecompressionContext(context);
			return hr >= 0 && produced == logical;
		}

		struct Rect { unsigned left, top, right, bottom; };
	}

	bool ConvertKanzenTextureLod(const unsigned char* header, int lod, const std::vector<KanzenSegment>& segments,
		std::vector<unsigned char>& out, unsigned& levels, std::string& error, bool* partial, bool zeroFillMissing)
	{
		levels = 0;
		if (partial)
			*partial = false;
		XenosFetch h;
		ParseFetch(header + 0x1c, h);
		XenosFormat fmt;
		if (!GetXenosFormat(h.format, fmt))
			return Fail(error, "Xenos texture format 0x%02x not handled", h.format);
		if (h.format != 0x12 && h.format != 0x14)
			return Fail(error, "no field-plane schema for Xenos format 0x%02x (Blur2Test decodes DXT1 and DXT5)", h.format);
		if (h.dimension != 1)
			return Fail(error, "field-plane texture other than 2D");
		const unsigned planesWanted = h.format == 0x12 ? 2 : 4;
		const unsigned bw = fmt.blockW, bh = fmt.blockH, bpb = fmt.bytesPerBlock;
		const unsigned width = h.width, height = h.height;
		const unsigned gridW = (width + bw - 1) / bw, gridH = (height + bh - 1) / bh;

		std::vector<unsigned char> top(gridW * gridH * bpb, 0);
		std::vector<unsigned char> filled(gridW * gridH, 0);
		std::vector<Rect> rects;
		std::vector<std::vector<unsigned char> > excess;
		for (size_t s = 0; s < segments.size(); ++s)
		{
			const std::vector<unsigned char>& seg = segments[s].payload;
			if (seg.size() < kControlSize + 11)
				return Fail(error, "segment %u of %u bytes", (unsigned)s, (unsigned)seg.size());
			std::vector<Nested> nested;
			NestedStreams(&seg[0], 0, seg.size(), nested);
			if (nested.empty())
				return Fail(error, "segment %u has no nested XMem streams", (unsigned)s);
			if (nested[0].sizeOffset != kControlSize)
				return Fail(error, "segment %u control is %u bytes, expected 88", (unsigned)s, (unsigned)nested[0].sizeOffset);
			if (nested.size() != planesWanted)
				return Fail(error, "segment %u has %u field planes, format 0x%02x needs %u", (unsigned)s, (unsigned)nested.size(), h.format, planesWanted);
			if (nested.back().payload + nested.back().stored != seg.size())
				return Fail(error, "segment %u: %u bytes after the last field plane", (unsigned)s, (unsigned)(seg.size() - nested.back().payload - nested.back().stored));

			const unsigned rs = Be32(&seg[80]), re = Be32(&seg[84]);
			Rect r = { rs & 0x1FFF, (rs >> 13) & 0x1FFF, re & 0x1FFF, (re >> 13) & 0x1FFF };
			if (r.right < r.left || r.bottom < r.top || r.right >= width || r.bottom >= height
				|| r.left % bw || r.top % bh
				|| (r.right + 1 != width && (r.right + 1) % bw) || (r.bottom + 1 != height && (r.bottom + 1) % bh))
				return Fail(error, "segment %u rectangle %u,%u-%u,%u does not fit %ux%u", (unsigned)s, r.left, r.top, r.right, r.bottom, width, height);

			// the planes, decompressed, then interleaved back into BC blocks
			std::vector<std::vector<unsigned char> > planes(nested.size());
			for (size_t n = 0; n < nested.size(); ++n)
				if (!DecompressStream(&seg[nested[n].payload], nested[n].stored, nested[n].logical, planes[n]))
					return Fail(error, "segment %u plane %u does not decompress", (unsigned)s, (unsigned)n);
			static const unsigned kDxt1[2] = { 4, 4 }, kDxt5[4] = { 4, 4, 2, 6 };
			const unsigned* fields = h.format == 0x12 ? kDxt1 : kDxt5;
			const size_t blocks = planes[0].size() / fields[0];
			for (size_t n = 0; n < planes.size(); ++n)
				if (planes[n].size() != blocks * fields[n])
					return Fail(error, "segment %u plane %u holds %u bytes for %u blocks", (unsigned)s, (unsigned)n, (unsigned)planes[n].size(), (unsigned)blocks);
			std::vector<unsigned char> segBlocks;
			segBlocks.reserve(blocks * bpb);
			for (size_t b = 0; b < blocks; ++b)
				for (size_t n = 0; n < planes.size(); ++n)
					segBlocks.insert(segBlocks.end(), planes[n].begin() + b * fields[n], planes[n].begin() + (b + 1) * fields[n]);

			// the rectangle of the top surface, row-major
			const unsigned rw = (r.right - r.left + bw) / bw, rh = (r.bottom - r.top + bh) / bh;
			const size_t rectBytes = (size_t)rw * rh * bpb;
			if (segBlocks.size() < rectBytes)
				return Fail(error, "segment %u rebuilds %u bytes for a rectangle of %u", (unsigned)s, (unsigned)segBlocks.size(), (unsigned)rectBytes);
			for (unsigned y = 0; y < rh; ++y)
			{
				const size_t target = (size_t)(r.top / bh + y) * gridW + r.left / bw;
				for (unsigned x = 0; x < rw; ++x)
				{
					if (filled[target + x])
						return Fail(error, "segment %u overlaps block (%u,%u)", (unsigned)s, r.left / bw + x, r.top / bh + y);
					filled[target + x] = 1;
				}
				memcpy(&top[target * bpb], &segBlocks[(size_t)y * rw * bpb], (size_t)rw * bpb);
			}
			rects.push_back(r);
			excess.push_back(std::vector<unsigned char>(segBlocks.begin() + rectBytes, segBlocks.end()));
		}
		{
			size_t unfilled = 0, first = filled.size();
			for (size_t i = 0; i < filled.size(); ++i)
				if (!filled[i])
				{
					if (first == filled.size())
						first = i;
					++unfilled;
				}
			if (unfilled && !zeroFillMissing)
			{
				// every segment decoded and none overlapped: the tier ships part of its
				// surface (a partial residency transfer, KanzenTexture.h)
				if (partial)
					*partial = true;
				unsigned l = width, t = height, r = 0, b = 0;
				for (size_t i = 0; i < rects.size(); ++i)
				{
					if (rects[i].left < l) l = rects[i].left;
					if (rects[i].top < t) t = rects[i].top;
					if (rects[i].right > r) r = rects[i].right;
					if (rects[i].bottom > b) b = rects[i].bottom;
				}
				return Fail(error, "partial surface: the %u rectangles (within %u,%u-%u,%u) tile %u of %ux%u blocks (first unfilled %u)",
					(unsigned)rects.size(), l, t, r, b, (unsigned)(filled.size() - unfilled), gridW, gridH, (unsigned)first);
			}
		}

		std::vector<std::vector<unsigned char> > chain(1, top);

		// decode_tail_levels: the lower levels after each rectangle, while every
		// scaled rectangle stays block-aligned and the rectangles tile the level
		const unsigned declared = h.mipMaxLevel + 1;
		std::vector<size_t> cursors(segments.size(), 0);
		for (unsigned level = 1; lod == 0 && level < declared; ++level)
		{
			const unsigned lw = width >> level ? width >> level : 1, lh = height >> level ? height >> level : 1;
			const unsigned lgW = (lw + bw - 1) / bw, lgH = (lh + bh - 1) / bh;
			std::vector<unsigned char> rebuilt(lgW * lgH * bpb, 0), lfilled(lgW * lgH, 0);
			std::vector<size_t> advance(segments.size(), 0);
			bool stop = false;
			for (size_t s = 0; s < rects.size() && !stop; ++s)
			{
				const Rect& r = rects[s];
				const int sl = (int)(r.left >> level), st = (int)(r.top >> level);
				const int sr = (int)((r.right + 1) >> level) - 1, sb = (int)((r.bottom + 1) >> level) - 1;
				if (sr < sl || sb < st
					|| sl % bw || st % bh
					|| (sr + 1 != (int)lw && (sr + 1) % bw) || (sb + 1 != (int)lh && (sb + 1) % bh))
				{
					stop = true;	// packed-tail placement: not decoded
					break;
				}
				const unsigned rw = (unsigned)(sr - sl + bw) / bw, rh = (unsigned)(sb - st + bh) / bh;
				const size_t rectBytes = (size_t)rw * rh * bpb;
				if (cursors[s] + rectBytes > excess[s].size())
				{
					stop = true;
					break;
				}
				for (unsigned y = 0; y < rh; ++y)
				{
					const size_t target = (size_t)(st / bh + y) * lgW + sl / bw;
					for (unsigned x = 0; x < rw; ++x)
					{
						if (lfilled[target + x])
							return Fail(error, "level %u segment %u overlaps", level, (unsigned)s);
						lfilled[target + x] = 1;
					}
					memcpy(&rebuilt[target * bpb], &excess[s][cursors[s] + (size_t)y * rw * bpb], (size_t)rw * bpb);
				}
				advance[s] = rectBytes;
			}
			for (size_t i = 0; i < lfilled.size() && !stop; ++i)
				if (!lfilled[i])
					stop = true;
			if (stop)
				break;
			for (size_t s = 0; s < cursors.size(); ++s)
				cursors[s] += advance[s];
			chain.push_back(rebuilt);
		}

		out.clear();
		for (size_t l = 0; l < chain.size(); ++l)
		{
			if (!EndianNormalize(chain[l].empty() ? NULL : &chain[l][0], chain[l].size(), h.endian))
				return Fail(error, "endian unit does not divide level %u", (unsigned)l);
			out.insert(out.end(), chain[l].begin(), chain[l].end());
		}
		levels = (unsigned)chain.size();
		return true;
	}
}
