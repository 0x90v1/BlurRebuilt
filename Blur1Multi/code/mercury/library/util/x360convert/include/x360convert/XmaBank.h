// [Blur1Multi] Xbox 360 audio banks (.baf) -> PC audio banks (T075, D15).
//
// A 360 bank is the engine's BANK container (audio/source/common/WaveBank.h)
// written big-endian (chEndian 1): BankHeader (0x34 bytes), one WAVE row per wave,
// then the wave data (DATA chunk header at uDataStart - 8). Its rows are XMA:
// format 8 (uXMA16Bit) or 9 (uXMA216Bit), WaveBaseHeader then WaveBank::XMAWave
// (rate, bits, channels, stride, loop start/end offsets in bits, loop count,
// subframe data, frame count) and, for format 9, a seek table of uNumFrames U32s.
// BANK version 5 (Nov 2010 prototypes) inserts one U32 between the base header
// and XMAWave (meaning not established; Blur2Test extract_xbox_baf_audio.py).
// Data is raw XMA2 in 2 KB packets.
//
// Banks also hold PCM (format 3) and ADPCM (format 7) rows (the music banks, Nov 9's
// voice-over banks); those are carried over as Blur1Multi's PS3 converter does.
// The PC reader takes PCM, ADPCM or MP3 rows, so every XMA wave is decoded with
// FFmpeg's XMA2 decoder (the vgmstream n5.1.2 Win32 DLLs beside the exe, loaded at
// run time; the decoder Blur2Test's vgmstream-cli uses) to 16-bit PCM, then encoded
// as the retail PC banks store their waves: 4-bit ADPCM (format 7, WaveBank::ADPCMWave)
// with WaveBundler's encoder (tools/Audio/WaveBundler PcmToAdpcm, LeeADPCM adpcm64),
// in a little-endian BANK version 4. The PC audio memory budget is sized for ADPCM
// banks; 16-bit PCM is four times larger and exhausts it. uNrSamples is the decoded
// length, which equals the retail PC row's for most waves (FFmpeg applies the XMA
// end trim). Each DATA chunk's size counts its 8-byte header, as in every PC bank.
// A streaming bank stays streaming (its waves' data at 2 KB boundaries). ADPCMWave
// loops the whole wave (uLoop 0/1): one whose XMA loop starts after the first frame
// (bit 32, past the packet header) is counted in 'partialLoops' (its intro repeats).
// 'CUE ' chunks between rows (BANK v5) have no reader on PC and are dropped ('cues').
#pragma once

#include <x360convert/X360Convert.h>

#include <string>
#include <vector>

namespace X360Convert
{
	struct XmaBankStats
	{
		unsigned waves;			// rows in the bank
		unsigned xmaWaves;		// XMA rows decoded
		unsigned samples;		// sample frames decoded from XMA, all waves
		unsigned partialLoops;	// looping waves whose XMA loop starts after the first frame
		unsigned cues;			// 'CUE ' chunks dropped
		unsigned carried;		// PCM and ADPCM rows carried over (swapped as on PS3)
		XmaBankStats() : waves(0), xmaWaves(0), samples(0), partialLoops(0), cues(0), carried(0) {}
	};

	// True when 'in' is a big-endian BANK this converter reads (v4/v5, chEndian 1).
	bool IsXmaBank(const unsigned char* in, size_t size);

	// Where a streamed conversion reads the 360 bank and writes the PC bank (the
	// writer patches each row's data start and length once the wave is decoded).
	class I_XmaBankReader
	{
	public:
		virtual ~I_XmaBankReader() {}
		virtual unsigned long long Size() = 0;
		virtual bool Read(unsigned long long offset, void* dst, size_t n) = 0;
	};
	class I_XmaBankWriter
	{
	public:
		virtual ~I_XmaBankWriter() {}
		virtual bool Write(const void* p, size_t n) = 0;
		virtual bool WriteAt(unsigned long long offset, const void* p, size_t n) = 0;
		virtual unsigned long long Position() = 0;
	};

	// One wave in memory at a time: a music bank's decoded PCM (over 1 GB) never is whole.
	E_Result ConvertXmaBankStream(I_XmaBankReader& src, I_XmaBankWriter& dst, std::string& error, XmaBankStats* stats = 0);

	// The whole 360 bank in, the PC bank out (small banks, tools).
	E_Result ConvertXmaBank(const unsigned char* in, size_t size, std::vector<unsigned char>& out,
		std::string& error, XmaBankStats* stats = 0);

	// The FFmpeg DLLs are loaded on first use from the exe's folder; false (and
	// 'error') when they or the XMA2 decoder are missing.
	bool XmaDecoderAvailable(std::string& error);
}
