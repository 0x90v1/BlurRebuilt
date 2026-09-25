// [Blur1Multi] Localisation string files (.loc, tag 'XTLO'): Xbox 360 -> PC.
//
// Writer: common/xt2/source/tool/Export/XtLocExport.cpp; layout XtLocFile.h:
//   XtLocHeader {U32 tag, U32 version 1, XtLocBlock toc {U32 offset, U32 size}}
//   at toc.offset: XtLocToc {U32 langOffset, U32 langCount, U32 lookupOffset}
//   at toc.offset + langOffset: langCount x XtLocLang {char name[16], XtLocBlock data}
//   language blocks, the lookup ("name", 8-bit strings) at toc.offset + lookupOffset
//   first, then each language's block at its data.offset:
//     XtLocTable {U32 count, U32 offset}, count x XtLocEntry {U32 id, U32 offset},
//     then the strings (lookup: bytes; languages: UTF-16 code units, each swapped
//     by the exporter).
// Every U32 above and every UTF-16 unit is swapped; names and 8-bit strings are bytes.

#include <x360convert/X360Convert.h>

#include "ChunkFile.h"

#include <string.h>

namespace X360Convert
{
	namespace
	{
		void Swap16(unsigned char* p) { unsigned char t = p[0]; p[0] = p[1]; p[1] = t; }
		void Swap32(unsigned char* p) { unsigned char t = p[0]; p[0] = p[3]; p[3] = t; t = p[1]; p[1] = p[2]; p[2] = t; }
		unsigned LE32(const unsigned char* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned)p[3] << 24); }

		E_Result Fail(std::string& error, const char* text)
		{
			error = text;
			return E_Result_Failed;
		}

		// Swaps a language block in place: table, entries, and (for UTF-16 languages) the strings.
		bool SwapLanguage(unsigned char* d, size_t n, size_t at, size_t size, bool utf16)
		{
			if (at > n || size > n - at || size < 8)
				return false;
			unsigned char* t = d + at;
			Swap32(t);
			Swap32(t + 4);
			const unsigned count = LE32(t), entries = LE32(t + 4);
			const size_t strings = (size_t)entries + (size_t)count * 8;
			if (strings > size)
				return false;
			for (unsigned i = 0; i < count; ++i)
			{
				Swap32(t + entries + i * 8);
				Swap32(t + entries + i * 8 + 4);
			}
			if (utf16)
			{
				if ((size - strings) % 2)
					return false;
				for (size_t o = strings; o < size; o += 2)
					Swap16(t + o);
			}
			return true;
		}
	}

	E_Result ConvertLoc(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error)
	{
		if (inSize < 28 || memcmp(in, "XTLO", 4) != 0)
			return Fail(error, "not a big-endian loc file");
		out.assign(in, in + inSize);
		unsigned char* d = &out[0];
		for (int i = 0; i < 4; ++i)
			Swap32(d + i * 4);
		if (LE32(d + 4) != 1)
			return Fail(error, "loc version is not 1");
		const size_t toc = LE32(d + 8);
		if (toc + 12 > inSize)
			return Fail(error, "loc table of contents outside the file");
		for (int i = 0; i < 3; ++i)
			Swap32(d + toc + i * 4);
		const size_t langOffset = LE32(d + toc), langCount = LE32(d + toc + 4), lookup = LE32(d + toc + 8);
		const size_t langs = toc + langOffset;
		if (langs + langCount * 24 > inSize)
			return Fail(error, "loc language list outside the file");
		// lookup block: from toc + lookupOffset to the first language block (or the end)
		size_t lookupEnd = inSize;
		for (size_t i = 0; i < langCount; ++i)
		{
			unsigned char* l = d + langs + i * 24;
			Swap32(l + 16);
			Swap32(l + 20);
			if (LE32(l + 16) < lookupEnd && LE32(l + 16) > toc + lookup)
				lookupEnd = LE32(l + 16);
		}
		if (!SwapLanguage(d, inSize, toc + lookup, lookupEnd - (toc + lookup), false))
			return Fail(error, "bad lookup block");
		for (size_t i = 0; i < langCount; ++i)
		{
			const unsigned char* l = d + langs + i * 24;
			if (!SwapLanguage(d, inSize, LE32(l + 16), LE32(l + 20), true))
				return Fail(error, "bad language block");
		}
		return E_Result_Converted;
	}
}
