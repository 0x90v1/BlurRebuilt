// [Blur1Multi] PS3 -> PC audio banks (.baf, T076). See X360Convert.h.
//
// Layout (audio/source/common/WaveBank.h, read by WaveBankReader.cpp; docs/analysis.md
// "PS3 -> PC conversion", audio banks). The same BANK version 4 container on every
// platform; PS3 writes it big-endian with chEndian 1, and the game's reader never swaps
// (NON_NATIVE_BANK_READING_SUPPORT is a tools-only path, and its extra-header swaps do
// not match the retail files).
//
//   BankHeader, 0x34 bytes: char uTag[4] "BANK", U32 uSize, U32 uVersion, U32 uNumWaves,
//     U8 chEndian, U8 chStreaming, char pchName[32], 2 bytes padding
//   uNumWaves WAVE chunks back to back, then one DATA chunk per wave (in memory: packed;
//     streaming: each at a 2 KB boundary). Every wave's DATA chunk header sits at
//     uDataStart - 8: char tag[4], U32 size.
//   WaveBaseHeader, 60 bytes: char uTag[4] "WAVE", U32 uSize (60 + extra), U32 uFormat,
//     char pchName[32], U32 uDataStart, U32 uDataLength, F32 fDefaultLevel, U32 uFlags
//   extra header by format (SampleTypes.h):
//     7 ADPCM 4-bit (16 bytes): U32 uSize, U32 uSampleRate, U32 uNrSamples,
//       U8 uLoop, U8 uNrInterleavedStreams, U8 uBitsPerSample, U8 uNumChannels
//     3 PCM S16 (20 bytes): U32 uSampleRate, U32 uBitsPerSample, U32 uNumChannels,
//       U32 uStride, U8 uNumLoops, 3 bytes padding
//
// ADPCM payloads are read byte by byte (core/decoderadpcm4bit.cpp) and are copied
// unchanged; PCM S16 samples are big-endian on PS3 and are swapped. Nothing is
// repacked: a bank PS3 flags streaming stays a streaming bank (a valid PC layout).

#include <x360convert/X360Convert.h>

#include "ChunkFile.h"

#include <stdio.h>

namespace X360Convert
{
	namespace
	{
		const unsigned kBankHeaderSize = 0x34;
		const unsigned kWaveBaseSize = 60;
		const unsigned kBankVersion = 4;
		const unsigned kFormatPcmS16 = 3;
		const unsigned kFormatAdpcm4Bit = 7;

		void Swap32At(std::vector<unsigned char>& b, size_t at)
		{
			unsigned char t;
			t = b[at]; b[at] = b[at + 3]; b[at + 3] = t;
			t = b[at + 1]; b[at + 1] = b[at + 2]; b[at + 2] = t;
		}

		void Fail(std::string& error, const char* what, unsigned value = 0)
		{
			char buf[160];
			sprintf_s(buf, "baf: %s (%u)", what, value);
			error = buf;
		}
	}

	bool BankHeadersSize(const unsigned char* head, size_t headSize, size_t& needed)
	{
		needed = kBankHeaderSize;
		if (headSize < kBankHeaderSize)
			return false;
		const unsigned waves = BE32(head + 0x0C);
		size_t at = kBankHeaderSize;
		for (unsigned i = 0; i < waves; ++i)
		{
			if (headSize < at + 8)
			{
				needed = at + 8;
				return false;
			}
			const unsigned size = BE32(head + at + 4);
			if (size < kWaveBaseSize)
				return true;		// malformed: PlanBank reports it
			at += size;
		}
		needed = at;
		return headSize >= at;
	}

	E_Result PlanBank(const unsigned char* head, size_t headSize, size_t fileSize, BankPatch& patch, std::string& error)
	{
		patch.headers.clear();
		patch.swap32.clear();
		patch.swap16.clear();
		if (headSize < kBankHeaderSize || memcmp(head, "BANK", 4) != 0)
			return E_Result_NotHandled;
		if (head[0x10] != 1)
		{
			Fail(error, "not a big-endian bank, chEndian", head[0x10]);
			return E_Result_Failed;
		}
		if (BE32(head + 0x04) != kBankHeaderSize || BE32(head + 0x08) != kBankVersion)
		{
			Fail(error, "unexpected header size or version", BE32(head + 0x08));
			return E_Result_Failed;
		}
		size_t needed;
		if (!BankHeadersSize(head, headSize, needed))
		{
			Fail(error, "wave headers run past the data given, need", (unsigned)needed);
			return E_Result_Failed;
		}

		const unsigned waves = BE32(head + 0x0C);
		std::vector<unsigned char>& h = patch.headers;
		h.assign(head, head + needed);
		Swap32At(h, 0x04);
		Swap32At(h, 0x08);
		Swap32At(h, 0x0C);
		h[0x10] = 0;

		size_t at = kBankHeaderSize;
		for (unsigned i = 0; i < waves; ++i)
		{
			const unsigned char* w = head + at;
			const unsigned size = BE32(w + 4);
			if (memcmp(w, "WAVE", 4) != 0 || size < kWaveBaseSize)
			{
				Fail(error, "bad WAVE chunk, index", i);
				return E_Result_Failed;
			}
			const unsigned format = BE32(w + 0x08);
			const unsigned dataStart = BE32(w + 0x2C);
			const unsigned dataLength = BE32(w + 0x30);
			for (unsigned f = 0x04; f <= 0x08; f += 4)
				Swap32At(h, at + f);
			for (unsigned f = 0x2C; f <= 0x38; f += 4)
				Swap32At(h, at + f);
			if (format == kFormatAdpcm4Bit)
			{
				if (size != kWaveBaseSize + 16)
				{
					Fail(error, "ADPCM wave with an unexpected header size", size);
					return E_Result_Failed;
				}
				for (unsigned f = 0x3C; f <= 0x44; f += 4)
					Swap32At(h, at + f);
			}
			else if (format == kFormatPcmS16)
			{
				if (size != kWaveBaseSize + 20)
				{
					Fail(error, "PCM wave with an unexpected header size", size);
					return E_Result_Failed;
				}
				for (unsigned f = 0x3C; f <= 0x48; f += 4)
					Swap32At(h, at + f);
			}
			else
			{
				Fail(error, "sample format not handled", format);
				return E_Result_Failed;
			}

			if (dataStart < needed + 8 || (unsigned long long)dataStart + dataLength > fileSize)
			{
				Fail(error, "DATA chunk outside the payload area, wave", i);
				return E_Result_Failed;
			}
			patch.swap32.push_back(dataStart - 4);		// the DATA chunk's U32 size
			if (format == kFormatPcmS16)
			{
				BankPatch::Range r = { dataStart, dataLength & ~1u };
				patch.swap16.push_back(r);
			}
			at += size;
		}
		return E_Result_Converted;
	}

	void WidenForBankPatch(const BankPatch& patch, unsigned long long& offset, unsigned long long& size)
	{
		unsigned long long begin = offset, end = offset + size;
		for (size_t i = 0; i < patch.swap32.size(); ++i)
		{
			const unsigned long long a = patch.swap32[i], b = a + 4;
			if (begin > a && begin < b)
				begin = a;
			if (end > a && end < b)
				end = b;
		}
		for (size_t i = 0; i < patch.swap16.size(); ++i)
		{
			const BankPatch::Range& r = patch.swap16[i];
			const unsigned long long rEnd = (unsigned long long)r.offset + r.size;
			if (begin > r.offset && begin < rEnd && ((begin - r.offset) & 1))
				--begin;
			if (end > r.offset && end < rEnd && ((end - r.offset) & 1))
				++end;
		}
		offset = begin;
		size = end - begin;
	}

	void ApplyBankPatch(const BankPatch& patch, unsigned long long offset, unsigned char* data, size_t size)
	{
		const unsigned long long end = offset + size;
		if (offset < patch.headers.size())
		{
			const unsigned long long to = patch.headers.size() < end ? patch.headers.size() : end;
			memcpy(data, &patch.headers[(size_t)offset], (size_t)(to - offset));
		}
		for (size_t i = 0; i < patch.swap32.size(); ++i)
		{
			const unsigned long long a = patch.swap32[i];
			if (a < offset || a + 4 > end)
				continue;
			unsigned char* p = data + (a - offset);
			unsigned char t;
			t = p[0]; p[0] = p[3]; p[3] = t;
			t = p[1]; p[1] = p[2]; p[2] = t;
		}
		for (size_t i = 0; i < patch.swap16.size(); ++i)
		{
			const BankPatch::Range& r = patch.swap16[i];
			unsigned long long from = r.offset > offset ? r.offset : offset;
			if ((from - r.offset) & 1)
				++from;
			const unsigned long long rEnd = (unsigned long long)r.offset + r.size;
			const unsigned long long to = rEnd < end ? rEnd : end;
			for (unsigned long long a = from; a + 2 <= to; a += 2)
			{
				unsigned char* p = data + (a - offset);
				const unsigned char t = p[0];
				p[0] = p[1];
				p[1] = t;
			}
		}
	}

	E_Result ConvertBank(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error)
	{
		BankPatch patch;
		const E_Result r = PlanBank(in, inSize, inSize, patch, error);
		if (r != E_Result_Converted)
			return r;
		out.assign(in, in + inSize);
		if (!out.empty())
			ApplyBankPatch(patch, 0, &out[0], out.size());
		return E_Result_Converted;
	}
}
