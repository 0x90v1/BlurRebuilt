// [Blur1Multi] xma_verify: convert an Xbox 360 audio bank with X360Convert::ConvertXmaBankStream
// and check the PC bank it writes.
//   xma_verify <in.baf> [--out <pc.baf>] [--ref <wav folder>] [--pc <retail PC bank>]
// --ref: a reference decode (Blur2Test's extract_xbox_baf_audio.py output:
//   <index>_<lower-case wave name>.wav, 16-bit PCM). Each ADPCM wave is decoded back
//   (adpcm64::_PackRead, tools/Audio/LeeADPCM) and compared sample by sample: the
//   signal-to-noise ratio of the round trip, and the frame counts.
// --pc: the same bank from the retail PC game. Each wave's sample count against the
//   PC row of the same name (both are the length of the authored source).
// Build: Blur1Multi/_work/tools/build_xma_verify.bat (the FFmpeg DLLs beside the exe).
#include <x360convert/XmaBank.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace
{
	unsigned LE32(const unsigned char* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned)p[3] << 24); }

	std::vector<unsigned char> Load(const std::string& path)
	{
		std::ifstream f(path, std::ios::binary);
		return std::vector<unsigned char>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
	}

	bool ReadWav(const std::string& path, std::vector<short>& pcm, unsigned& channels, unsigned& rate)
	{
		const std::vector<unsigned char> d = Load(path);
		if (d.size() < 12 || memcmp(&d[0], "RIFF", 4) || memcmp(&d[8], "WAVE", 4))
			return false;
		for (size_t at = 12; at + 8 <= d.size();)
		{
			const unsigned size = LE32(&d[at + 4]);
			if (!memcmp(&d[at], "fmt ", 4))
			{
				channels = d[at + 10] | (d[at + 11] << 8);
				rate = LE32(&d[at + 12]);
			}
			else if (!memcmp(&d[at], "data", 4))
			{
				pcm.resize(size / 2);
				memcpy(pcm.data(), &d[at + 8], pcm.size() * 2);
				return true;
			}
			at += 8 + size + (size & 1);
		}
		return false;
	}

	// adpcm64::_PackRead: 33-byte blocks, stereo as L then R, predictors carried over
	void DecodeAdpcm(const unsigned char* in, size_t bytes, unsigned channels, std::vector<short>& out)
	{
		const float f[5][2] = { { 0.0, 0.0 }, { 60.0 / 64.0, 0.0 }, { 115.0 / 64.0, -52.0 / 64.0 }, { 98.0 / 64.0, -55.0 / 64.0 }, { 122.0 / 64.0, -60.0 / 64.0 } };
		float prev1[2] = { 0, 0 }, prev2[2] = { 0, 0 };
		const size_t group = 33 * channels;
		for (size_t at = 0; at + group <= bytes; at += group)
		{
			float s[2][64];
			int predict[2];
			for (unsigned c = 0; c < channels; ++c)
			{
				const unsigned char* b = in + at + 33 * c;
				const int shift = b[0] & 0xf;
				predict[c] = b[0] >> 4;
				for (int i = 0; i < 64; i += 2)
				{
					const int d = b[1 + i / 2];
					int v = (d & 0xf) << 12;
					if (v & 0x8000) v |= 0xffff0000;
					s[c][i] = (float)(v >> shift);
					v = (d & 0xf0) << 8;
					if (v & 0x8000) v |= 0xffff0000;
					s[c][i + 1] = (float)(v >> shift);
				}
			}
			for (int i = 0; i < 64; ++i)
				for (unsigned c = 0; c < channels; ++c)
				{
					const float v = s[c][i] + prev1[c] * f[predict[c]][0] + prev2[c] * f[predict[c]][1];
					prev2[c] = prev1[c];
					prev1[c] = v;
					const int d = (int)(v + 0.5f);
					out.push_back((short)std::max(-32768, std::min(32767, d)));
				}
		}
	}

	// signal-to-noise ratio of 'b' against 'a' over the first n samples (999: identical or silent)
	double Snr(const std::vector<short>& a, const std::vector<short>& b, size_t n)
	{
		double sig = 0, noise = 0;
		for (size_t j = 0; j < n && j < a.size() && j < b.size(); ++j)
		{
			sig += (double)a[j] * a[j];
			noise += ((double)b[j] - a[j]) * ((double)b[j] - a[j]);
		}
		return noise > 0 && sig > 0 ? 10 * log10(sig / noise) : 999;
	}

	std::string Lower(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		return s;
	}

	// name -> ADPCM row of a little-endian PC bank
	std::map<std::string, const unsigned char*> PcRows(const std::vector<unsigned char>& d)
	{
		std::map<std::string, const unsigned char*> m;
		if (d.size() < 0x34 || memcmp(&d[0], "BANK", 4) || d[0x10] != 0)
			return m;
		size_t at = 0x34;
		for (unsigned i = 0, n = LE32(&d[12]); i < n && at + 60 <= d.size(); ++i)
		{
			char name[33] = { 0 };
			memcpy(name, &d[at + 12], 32);
			if (LE32(&d[at + 8]) == 7)
				m[Lower(name)] = &d[at];
			at += LE32(&d[at + 4]);
		}
		return m;
	}
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "usage: xma_verify <in.baf> [--out <pc.baf>] [--ref <wav folder>] [--pc <retail PC bank>]\n");
		return 2;
	}
	std::string outPath, refDir, pcPath;
	for (int i = 2; i + 1 < argc; i += 2)
	{
		if (!strcmp(argv[i], "--out")) outPath = argv[i + 1];
		else if (!strcmp(argv[i], "--ref")) refDir = argv[i + 1];
		else if (!strcmp(argv[i], "--pc")) pcPath = argv[i + 1];
	}
	// file to file through ConvertXmaBankStream (one wave in memory), as the game's cache does
	struct FileReader : X360Convert::I_XmaBankReader
	{
		FILE* f; unsigned long long n;
		unsigned long long Size() { return n; }
		bool Read(unsigned long long o, void* d, size_t k) { return _fseeki64(f, (long long)o, SEEK_SET) == 0 && fread(d, 1, k, f) == k; }
	} reader;
	struct FileWriter : X360Convert::I_XmaBankWriter
	{
		FILE* f;
		bool Write(const void* p, size_t k) { return fwrite(p, 1, k, f) == k; }
		bool WriteAt(unsigned long long o, const void* p, size_t k)
		{
			const long long back = _ftelli64(f);
			const bool ok = _fseeki64(f, (long long)o, SEEK_SET) == 0 && fwrite(p, 1, k, f) == k;
			return _fseeki64(f, back, SEEK_SET) == 0 && ok;
		}
		unsigned long long Position() { return (unsigned long long)_ftelli64(f); }
	} writer;
	if (fopen_s(&reader.f, argv[1], "rb"))
		return 1;
	_fseeki64(reader.f, 0, SEEK_END);
	reader.n = (unsigned long long)_ftelli64(reader.f);
	const std::string target = outPath.empty() ? std::string(argv[1]) + ".pc.baf" : outPath;
	if (fopen_s(&writer.f, target.c_str(), "w+b"))
		return 1;
	std::string error;
	X360Convert::XmaBankStats st;
	const X360Convert::E_Result r = X360Convert::ConvertXmaBankStream(reader, writer, error, &st);
	const unsigned long long written = writer.Position();
	fclose(writer.f);
	fclose(reader.f);
	if (r != X360Convert::E_Result_Converted)
	{
		printf("not converted: %s\n", error.c_str());
		return 1;
	}
	printf("%s: %u waves, %u XMA decoded, %u carried, %u sample frames, %u partial loops, %u CUE chunks dropped, %llu -> %llu bytes (%s)\n",
		argv[1], st.waves, st.xmaWaves, st.carried, st.samples, st.partialLoops, st.cues, reader.n, written, target.c_str());
	const bool detail = !refDir.empty() || !pcPath.empty();	// a line per wave; else only bad ones
	const std::vector<unsigned char> out = Load(target);
	const std::vector<unsigned char> pcBank = pcPath.empty() ? std::vector<unsigned char>() : Load(pcPath);
	const std::map<std::string, const unsigned char*> pc = PcRows(pcBank);

	const unsigned waves = LE32(&out[12]);
	unsigned compared = 0, missing = 0, pcSame = 0, pcDiffer = 0, pcMissing = 0, pcCompared = 0, badData = 0;
	double worstSnr = 1e9, sumSnr = 0, sumPcSnr = 0, sumOursWherePc = 0;
	size_t at = 0x34;
	for (unsigned i = 0; i < waves; ++i)
	{
		const unsigned char* w = &out[at];
		at += LE32(w + 4);
		if (LE32(w + 8) != 7)
			continue;	// carried PCM rows
		char name[33] = { 0 };
		memcpy(name, w + 12, 32);
		const unsigned start = LE32(w + 0x2C), length = LE32(w + 0x30), rate = LE32(w + 0x40), frames = LE32(w + 0x44);
		const unsigned channels = w[0x4B];
		char line[512];
		int n = sprintf_s(line, "%3u %-32s %u ch %u Hz loop %u  frames %u", i, name, channels, rate, w[0x48], frames);
		// the DATA chunk header before the payload, its size counting the header (as on PC)
		if (start < 8 || memcmp(&out[start - 8], "DATA", 4) || LE32(&out[start - 4]) != length + 8)
			++badData, n += sprintf_s(line + n, sizeof(line) - n, "  BAD DATA HEADER");
		const unsigned char* pcRow = 0;
		if (!pc.empty())
		{
			const auto it = pc.find(Lower(name));
			pcRow = it == pc.end() ? 0 : it->second;
			const unsigned pcFrames = pcRow ? LE32(pcRow + 0x44) : 0;
			if (!pcRow)
				++pcMissing, n += sprintf_s(line + n, sizeof(line) - n, "  (not on PC)");
			else if (pcFrames == frames)
				++pcSame, n += sprintf_s(line + n, sizeof(line) - n, "  = PC");
			else
				++pcDiffer, n += sprintf_s(line + n, sizeof(line) - n, "  PC %u (%+d)", pcFrames, (int)frames - (int)pcFrames);
		}
		if (!refDir.empty())
		{
			char file[512];
			sprintf_s(file, "%s/%03u_%s.wav", refDir.c_str(), i, Lower(name).c_str());
			std::vector<short> ref;
			unsigned refChannels = 0, refRate = 0;
			if (!ReadWav(file, ref, refChannels, refRate))
				++missing;
			else
			{
				++compared;
				std::vector<short> mine;
				DecodeAdpcm(&out[start], length, channels, mine);
				const double snr = Snr(ref, mine, (size_t)frames * channels);
				worstSnr = std::min(worstSnr, snr);
				sumSnr += snr;
				n += sprintf_s(line + n, sizeof(line) - n, "  ref %u frames  SNR %.1f dB", (unsigned)(ref.size() / (refChannels ? refChannels : 1)), snr);
				// the retail PC encode of the same wave, against the same reference
				if (pcRow && pcRow[0x4B] == channels && LE32(pcRow + 0x40) == rate)
				{
					std::vector<short> theirs;
					DecodeAdpcm(&pcBank[LE32(pcRow + 0x2C)], LE32(pcRow + 0x30), channels, theirs);
					const double pcSnr = Snr(ref, theirs, (size_t)LE32(pcRow + 0x44) * channels);
					++pcCompared;
					sumPcSnr += pcSnr;
					sumOursWherePc += snr;
					n += sprintf_s(line + n, sizeof(line) - n, "  PC SNR %.1f dB", pcSnr);
				}
			}
		}
		if (detail || strstr(line, "BAD"))
			printf("%s\n", line);
	}
	if (badData)
		printf("%u waves with a bad DATA header\n", badData);
	if (!refDir.empty())
		printf("compared %u (mean SNR %.1f dB, worst %.1f dB), no reference %u\n", compared, compared ? sumSnr / compared : 0.0, worstSnr, missing);
	if (pcCompared)
		printf("same waves, retail PC encode: mean SNR %.1f dB; this encode %.1f dB (%u waves)\n", sumPcSnr / pcCompared, sumOursWherePc / pcCompared, pcCompared);
	if (!pc.empty())
		printf("against PC: %u same length, %u differ, %u not on PC\n", pcSame, pcDiffer, pcMissing);
	return 0;
}
