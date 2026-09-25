// [Blur1Multi] Xbox 360 audio banks -> PC audio banks (T075, D15). See XmaBank.h.

#include <x360convert/XmaBank.h>

#include "ChunkFile.h"

#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>
}

namespace X360Convert
{
	namespace
	{
		const unsigned kBankHeaderSize = 0x34;
		const unsigned kWaveBaseSize = 60;			// WaveBank::WaveBaseHeader
		const unsigned kXmaWaveSize = 32;			// WaveBank::XMAWave
		const unsigned kPcmWaveSize = 20;			// WaveBank::PCMWave (U8 loop count + 3 bytes padding)
		const unsigned kAdpcmWaveSize = 16;			// WaveBank::ADPCMWave
		const unsigned kFormatAdpcm4Bit = 7;
		const unsigned kFormatPcmS16 = 3;
		const unsigned kFormatXma = 8, kFormatXma2 = 9;
		const unsigned kPacketBytes = 0x800;
		const unsigned kStreamAlign = 0x800;		// a streaming bank's data (BafConvert.cpp)
		const unsigned kFlagXmaSeekTable = 0x80;	// FLAG_HAS_XMA_SEEK_TABLE (WaveBank.h)

		// FFmpeg, loaded at run time from the exe's folder.
		struct Ffmpeg
		{
			bool tried, ok;
			std::string error;
			const AVCodec* (*find_decoder)(enum AVCodecID);
			AVCodecContext* (*alloc_context3)(const AVCodec*);
			int (*open2)(AVCodecContext*, const AVCodec*, AVDictionary**);
			void (*free_context)(AVCodecContext**);
			int (*send_packet)(AVCodecContext*, const AVPacket*);
			int (*receive_frame)(AVCodecContext*, AVFrame*);
			AVPacket* (*packet_alloc)(void);
			void (*packet_free)(AVPacket**);
			AVFrame* (*frame_alloc)(void);
			void (*frame_free)(AVFrame**);
			void* (*mallocz)(size_t);
			void (*layout_default)(AVChannelLayout*, int);
			void (*log_set_level)(int);
			Ffmpeg() { memset(this, 0, sizeof(*this) - sizeof(std::string)); tried = ok = false; }
		};
		Ffmpeg g_ff;
		CRITICAL_SECTION g_ffLock;
		struct LockInit { LockInit() { InitializeCriticalSection(&g_ffLock); } } g_lockInit;

		template<typename T> bool Get(HMODULE m, const char* name, T& f, std::string& error)
		{
			f = (T)GetProcAddress(m, name);
			if (!f)
				error = std::string("FFmpeg export missing: ") + name;
			return f != 0;
		}

		bool LoadFfmpeg()
		{
			EnterCriticalSection(&g_ffLock);
			if (!g_ff.tried)
			{
				g_ff.tried = true;
				char exe[MAX_PATH];
				GetModuleFileNameA(NULL, exe, MAX_PATH);
				std::string dir(exe);
				dir = dir.substr(0, dir.find_last_of("\\/") + 1);
				HMODULE util = LoadLibraryA((dir + "avutil-vgmstream-57.dll").c_str());
				HMODULE codec = util ? LoadLibraryA((dir + "avcodec-vgmstream-59.dll").c_str()) : 0;
				std::string& e = g_ff.error;
				if (!util || !codec)
					e = "FFmpeg DLLs (avutil-vgmstream-57.dll, avcodec-vgmstream-59.dll) not found beside the exe";
				else if (Get(codec, "avcodec_find_decoder", g_ff.find_decoder, e) && Get(codec, "avcodec_alloc_context3", g_ff.alloc_context3, e)
					&& Get(codec, "avcodec_open2", g_ff.open2, e) && Get(codec, "avcodec_free_context", g_ff.free_context, e)
					&& Get(codec, "avcodec_send_packet", g_ff.send_packet, e) && Get(codec, "avcodec_receive_frame", g_ff.receive_frame, e)
					&& Get(codec, "av_packet_alloc", g_ff.packet_alloc, e) && Get(codec, "av_packet_free", g_ff.packet_free, e)
					&& Get(util, "av_frame_alloc", g_ff.frame_alloc, e) && Get(util, "av_frame_free", g_ff.frame_free, e)
					&& Get(util, "av_mallocz", g_ff.mallocz, e) && Get(util, "av_channel_layout_default", g_ff.layout_default, e)
					&& Get(util, "av_log_set_level", g_ff.log_set_level, e))
				{
					g_ff.log_set_level(AV_LOG_ERROR);
					g_ff.ok = g_ff.find_decoder(AV_CODEC_ID_XMA2) != 0;
					if (!g_ff.ok)
						e = "the FFmpeg build has no XMA2 decoder";
				}
			}
			const bool ok = g_ff.ok;
			LeaveCriticalSection(&g_ffLock);
			return ok;
		}

		bool Fail(std::string& error, const char* format, ...)
		{
			char text[256];
			va_list args;
			va_start(args, format);
			vsprintf_s(text, format, args);
			va_end(args);
			error = std::string("baf: ") + text;
			return false;
		}

		short ToS16(float v)
		{
			const float s = v * 32768.0f;
			if (s >= 32767.0f)
				return 32767;
			if (s <= -32768.0f)
				return -32768;
			return (short)lrintf(s);
		}

		// One XMA2 stream (2 KB packets) to interleaved 16-bit PCM.
		bool DecodeXma(const unsigned char* data, size_t size, unsigned rate, unsigned channels,
			std::vector<short>& pcm, std::string& error)
		{
			const AVCodec* codec = g_ff.find_decoder(AV_CODEC_ID_XMA2);
			AVCodecContext* ctx = g_ff.alloc_context3(codec);
			if (!ctx)
				return Fail(error, "no FFmpeg context");
			ctx->sample_rate = (int)rate;
			g_ff.layout_default(&ctx->ch_layout, (int)channels);
			ctx->block_align = kPacketBytes;
			// XMA2WAVEFORMATEX (34 bytes, little-endian): the decoder reads NumStreams and
			// ChannelMask (0: channel order unspecified); a stream carries 1 or 2 channels.
			ctx->extradata = (uint8_t*)g_ff.mallocz(34 + AV_INPUT_BUFFER_PADDING_SIZE);
			ctx->extradata_size = 34;
			const unsigned streams = (channels + 1) / 2;
			ctx->extradata[0] = (uint8_t)streams;
			ctx->extradata[1] = (uint8_t)(streams >> 8);
			bool ok = g_ff.open2(ctx, codec, 0) >= 0;
			if (!ok)
				Fail(error, "FFmpeg XMA2 decoder refused %u channels at %u Hz", channels, rate);
			AVPacket* packet = g_ff.packet_alloc();
			AVFrame* frame = g_ff.frame_alloc();
			std::vector<unsigned char> buffer(kPacketBytes + AV_INPUT_BUFFER_PADDING_SIZE, 0);
			for (size_t at = 0; ok && at <= size; at += kPacketBytes)
			{
				int r;
				if (at < size)
				{
					memcpy(&buffer[0], data + at, kPacketBytes);
					packet->data = &buffer[0];
					packet->size = kPacketBytes;
					r = g_ff.send_packet(ctx, packet);
				}
				else
					r = g_ff.send_packet(ctx, 0);	// drain
				if (r < 0 && r != AVERROR(EAGAIN) && r != AVERROR_EOF)
				{
					ok = Fail(error, "XMA2 packet at 0x%x refused (%d)", (unsigned)at, r);
					break;
				}
				for (;;)
				{
					r = g_ff.receive_frame(ctx, frame);
					if (r == AVERROR(EAGAIN) || r == AVERROR_EOF)
						break;
					if (r < 0)
					{
						ok = Fail(error, "XMA2 decode error (%d) near 0x%x", r, (unsigned)at);
						break;
					}
					if (frame->format != AV_SAMPLE_FMT_FLTP || frame->ch_layout.nb_channels != (int)channels)
					{
						ok = Fail(error, "XMA2 frame of format %d, %d channels", frame->format, frame->ch_layout.nb_channels);
						break;
					}
					const size_t base = pcm.size();
					pcm.resize(base + (size_t)frame->nb_samples * channels);
					for (int c = 0; c < (int)channels; ++c)
					{
						const float* src = (const float*)frame->extended_data[c];
						for (int i = 0; i < frame->nb_samples; ++i)
							pcm[base + (size_t)i * channels + c] = ToS16(src[i]);
					}
				}
				if (at == size)
					break;
			}
			packet->data = 0;
			packet->size = 0;
			g_ff.packet_free(&packet);
			g_ff.frame_free(&frame);
			g_ff.free_context(&ctx);
			return ok;
		}

		// The PC banks' ADPCM encoder: WaveBundler's PcmToAdpcm (tools/Audio/WaveBundler/
		// Library/fmt_adpcm.cpp) with adpcm64 (tools/Audio/LeeADPCM/adpcm64.cpp), ported
		// as written. Blocks of 64 samples per channel, one byte of predictor and shift then
		// 32 bytes of 4-bit deltas; stereo as an L block then an R block. The last block is
		// padded with silence. The engine reads it with core/decoderadpcm4bit.cpp.
		const int kAdpcmBatch = 64;
		const unsigned kAdpcmBlockBytes = 1 + kAdpcmBatch / 2;

		void AdpcmFindPredict(float* samples, int* predict_nr, int* shift_factor, float* predictPrev1, float* predictPrev2)
		{
			const float fp[5][2] = { { 0.0, 0.0 }, {  -60.0 / 64.0, 0.0 }, { -115.0 / 64.0, 52.0 / 64.0 }, {  -98.0 / 64.0, 55.0 / 64.0 }, { -122.0 / 64.0, 60.0 / 64.0 } };
			int i, j;
			float buffer[kAdpcmBatch][5];
			float min = 1e10;
			float max[5];
			float ds;
			int min2;
			int shift_mask;
			float s_0, s_1 = 0, s_2 = 0;
			for (i = 0; i < 5; i++)
			{
				max[i] = 0.0;
				s_1 = *predictPrev1;
				s_2 = *predictPrev2;
				for (j = 0; j < kAdpcmBatch; j++)
				{
					s_0 = samples[j];
					if (s_0 > 30719.0)
						s_0 = 30719.0;
					if (s_0 < -30720.0)
						s_0 = -30720.0;
					ds = s_0 + s_1 * fp[i][0] + s_2 * fp[i][1];
					buffer[j][i] = ds;
					if (fabs(ds) > max[i])
						max[i] = fabs(ds);
					s_2 = s_1;
					s_1 = s_0;
				}
				if (max[i] < min)
				{
					min = max[i];
					*predict_nr = i;
				}
				if (min <= 7)
				{
					*predict_nr = 0;
					break;
				}
			}
			*predictPrev1 = s_1;
			*predictPrev2 = s_2;
			for (i = 0; i < kAdpcmBatch; i++)
				samples[i] = buffer[i][*predict_nr];
			min2 = (int)min;
			shift_mask = 0x4000;
			*shift_factor = 0;
			while (*shift_factor < 12)
			{
				if (shift_mask & (min2 + (shift_mask >> 3)))
					break;
				(*shift_factor)++;
				shift_mask = shift_mask >> 1;
			}
		}

		void AdpcmPack(float* samples, short* four_bit, int predict_nr, int shift_factor, float* packPrev1, float* packPrev2)
		{
			const float fp[5][2] = { { 0.0, 0.0 }, {  -60.0 / 64.0, 0.0 }, { -115.0 / 64.0, 52.0 / 64.0 }, {  -98.0 / 64.0, 55.0 / 64.0 }, { -122.0 / 64.0, 60.0 / 64.0 } };
			float ds;
			int di;
			float s_0;
			for (int i = 0; i < kAdpcmBatch; i++)
			{
				s_0 = samples[i] + *packPrev1 * fp[predict_nr][0] + *packPrev2 * fp[predict_nr][1];
				ds = s_0 * (float)(1 << shift_factor);
				di = ((int)ds + 0x800) & 0xfffff000;
				if (di > 32767)
					di = 32767;
				if (di < -32768)
					di = -32768;
				four_bit[i] = (short)di;
				di = di >> shift_factor;
				*packPrev2 = *packPrev1;
				*packPrev1 = (float)di - s_0;
			}
		}

		// Interleaved 16-bit PCM of one wave (1 or 2 channels) to ADPCM blocks.
		void EncodeAdpcm(const std::vector<short>& pcm, unsigned channels, std::vector<unsigned char>& out)
		{
			const size_t frames = pcm.size() / channels;
			const size_t batches = (frames + kAdpcmBatch - 1) / kAdpcmBatch;
			out.assign(batches * channels * kAdpcmBlockBytes, 0);
			float predictPrev1[2] = { 0, 0 }, predictPrev2[2] = { 0, 0 };
			float packPrev1[2] = { 0, 0 }, packPrev2[2] = { 0, 0 };
			float batch[2][kAdpcmBatch];
			short four_bit[kAdpcmBatch];
			unsigned char* o = out.empty() ? NULL : &out[0];
			for (size_t b = 0; b < batches; ++b)
			{
				for (int i = 0; i < kAdpcmBatch; ++i)
				{
					const size_t f = b * kAdpcmBatch + i;
					for (unsigned c = 0; c < channels; ++c)
						batch[c][i] = f < frames ? pcm[f * channels + c] : 0;
				}
				for (unsigned c = 0; c < channels; ++c)
				{
					int predict_nr = 0, shift_factor = 0;
					AdpcmFindPredict(batch[c], &predict_nr, &shift_factor, &predictPrev1[c], &predictPrev2[c]);
					AdpcmPack(batch[c], four_bit, predict_nr, shift_factor, &packPrev1[c], &packPrev2[c]);
					*o++ = (unsigned char)((predict_nr << 4) | shift_factor);
					for (int k = 0; k < kAdpcmBatch; k += 2)
						*o++ = (unsigned char)(((four_bit[k + 1] >> 8) & 0xf0) | ((four_bit[k] >> 12) & 0xf));
				}
			}
		}

		// One WAVE row as read from the 360 bank.
		struct Row
		{
			std::string name;
			unsigned level, flags;
			unsigned rate, channels, loops, stride;
			unsigned format, dataStart, dataLength;
			std::vector<unsigned char> pcRow;	// PCM and ADPCM rows: the PC row, ready
			bool swap16;						// PCM S16 samples to swap
			Row() : level(0), flags(0), rate(0), channels(0), loops(0), stride(0), format(0), dataStart(0), dataLength(0), swap16(false) {}
		};

		unsigned long long Align(unsigned long long v, unsigned a) { return (v + a - 1) / a * a; }

		void SetLE32(unsigned char* p, unsigned v) { for (int i = 0; i < 4; ++i) p[i] = (unsigned char)(v >> (8 * i)); }

		// In-memory reader and writer for ConvertXmaBank.
		class C_MemoryReader : public I_XmaBankReader
		{
		public:
			C_MemoryReader(const unsigned char* p, size_t n) : m_p(p), m_n(n) {}
			unsigned long long Size() { return m_n; }
			bool Read(unsigned long long offset, void* dst, size_t n)
			{
				if (offset + n > m_n)
					return false;
				memcpy(dst, m_p + offset, n);
				return true;
			}
		private:
			const unsigned char* m_p;
			size_t m_n;
		};

		class C_MemoryWriter : public I_XmaBankWriter
		{
		public:
			C_MemoryWriter(std::vector<unsigned char>& out) : m_out(out) { m_out.clear(); }
			bool Write(const void* p, size_t n) { m_out.insert(m_out.end(), (const unsigned char*)p, (const unsigned char*)p + n); return true; }
			bool WriteAt(unsigned long long offset, const void* p, size_t n)
			{
				if (offset + n > m_out.size())
					return false;
				memcpy(&m_out[(size_t)offset], p, n);
				return true;
			}
			unsigned long long Position() { return m_out.size(); }
		private:
			std::vector<unsigned char>& m_out;
		};
	}

	bool XmaDecoderAvailable(std::string& error)
	{
		if (LoadFfmpeg())
			return true;
		error = g_ff.error;
		return false;
	}

	bool IsXmaBank(const unsigned char* in, size_t size)
	{
		if (size < kBankHeaderSize || memcmp(in, "BANK", 4) != 0 || in[0x10] != 1)
			return false;
		const unsigned version = BE32(in + 0x08);
		return BE32(in + 0x04) == kBankHeaderSize && (version == 4 || version == 5);
	}

	E_Result ConvertXmaBankStream(I_XmaBankReader& src, I_XmaBankWriter& dst, std::string& error, XmaBankStats* stats)
	{
		unsigned char head[kBankHeaderSize];
		if (src.Size() < kBankHeaderSize || !src.Read(0, head, kBankHeaderSize) || !IsXmaBank(head, kBankHeaderSize))
			return E_Result_NotHandled;
		if (!LoadFfmpeg())
		{
			error = "baf: " + g_ff.error;
			return E_Result_Failed;
		}
		XmaBankStats local;
		XmaBankStats& st = stats ? *stats : local;
		const unsigned version = BE32(head + 0x08);
		const unsigned tail = version == 5 ? 4 : 0;
		const unsigned waves = BE32(head + 0x0C);
		const unsigned char streaming = head[0x11];
		const unsigned long long size = src.Size();
		st.waves = waves;

		// the rows (and 'CUE ' chunks) between the header and the data
		std::vector<Row> rows;
		unsigned long long at = kBankHeaderSize;
		std::vector<unsigned char> w;
		while (rows.size() < waves)
		{
			unsigned char h[8];
			if (at + 8 > size || !src.Read(at, h, 8))
				{ Fail(error, "chunk header past the end at 0x%llx", at); return E_Result_Failed; }
			const unsigned chunk = BE32(h + 4);
			if (chunk < 8 || at + chunk > size)
				{ Fail(error, "chunk at 0x%llx of size 0x%x", at, chunk); return E_Result_Failed; }
			if (memcmp(h, "CUE ", 4) == 0)
			{
				++st.cues;
				at += chunk;
				continue;
			}
			if (memcmp(h, "WAVE", 4) != 0)
				{ Fail(error, "unexpected chunk at 0x%llx", at); return E_Result_Failed; }
			w.resize(chunk);
			if (!src.Read(at, &w[0], chunk))
				{ Fail(error, "WAVE row at 0x%llx unreadable", at); return E_Result_Failed; }
			const unsigned format = BE32(&w[0x08]);
			if (format == kFormatPcmS16 || format == kFormatAdpcm4Bit)
			{
				// Carried over as the PS3 converter carries them (BafConvert.cpp): header
				// words swapped; PCM S16 samples swapped, ADPCM bytes as stored. The
				// extra header (PCMWave 20 bytes, ADPCMWave 16) follows the base header,
				// after the BANK v5 word when the row has room for it (Nov 9: PCM rows 84
				// bytes, ADPCM rows 76, as in version 4).
				const unsigned extra = format == kFormatPcmS16 ? kPcmWaveSize : kAdpcmWaveSize;
				unsigned x;
				if (chunk == kWaveBaseSize + extra)
					x = kWaveBaseSize;
				else if (tail && chunk == kWaveBaseSize + tail + extra)
					x = kWaveBaseSize + tail;
				else
					{ Fail(error, "wave %u of format %u with a header of 0x%x bytes", (unsigned)rows.size(), format, chunk); return E_Result_Failed; }
				Row r;
				char name[33];
				memcpy(name, &w[0x0C], 32);
				name[32] = 0;
				r.name = name;
				r.format = format;
				r.dataStart = BE32(&w[0x2C]);
				r.dataLength = BE32(&w[0x30]);
				if ((unsigned long long)r.dataStart + r.dataLength > size)
					{ Fail(error, "wave %s data 0x%x+0x%x", name, r.dataStart, r.dataLength); return E_Result_Failed; }
				r.level = BE32(&w[0x34]);
				r.flags = BE32(&w[0x38]);
				r.pcRow.assign(kWaveBaseSize + extra, 0);
				unsigned char* o = &r.pcRow[0];
				memcpy(o, "WAVE", 4);
				SetLE32(o + 4, kWaveBaseSize + extra);
				SetLE32(o + 8, format);
				memcpy(o + 12, &w[0x0C], 32);
				SetLE32(o + 0x34, r.level);
				SetLE32(o + 0x38, r.flags);
				const unsigned words = format == kFormatPcmS16 ? 4 : 3;	// then U8s as stored
				for (unsigned k = 0; k < words; ++k)
					SetLE32(o + kWaveBaseSize + 4 * k, BE32(&w[x + 4 * k]));
				memcpy(o + kWaveBaseSize + 4 * words, &w[x + 4 * words], extra - 4 * words);
				r.swap16 = format == kFormatPcmS16 && BE32(&w[x + 4]) == 16;
				rows.push_back(r);
				++st.carried;
				at += chunk;
				continue;
			}
			if (format != kFormatXma && format != kFormatXma2)
				{ Fail(error, "wave %u of format %u", (unsigned)rows.size(), format); return E_Result_Failed; }
			const unsigned x = kWaveBaseSize + tail;	// XMAWave
			if (chunk < x + kXmaWaveSize)
				{ Fail(error, "XMA wave %u header of 0x%x bytes", (unsigned)rows.size(), chunk); return E_Result_Failed; }
			const unsigned frames = BE32(&w[x + 28]);
			if (chunk != x + kXmaWaveSize + (format == kFormatXma2 ? frames * 4 : 0))
				{ Fail(error, "XMA wave %u header of 0x%x bytes for %u frames", (unsigned)rows.size(), chunk, frames); return E_Result_Failed; }
			Row r;
			char name[33];
			memcpy(name, &w[0x0C], 32);
			name[32] = 0;
			r.name = name;
			r.format = format;
			r.dataStart = BE32(&w[0x2C]);
			r.dataLength = BE32(&w[0x30]);
			r.stride = BE32(&w[x + 12]);
			if ((unsigned long long)r.dataStart + r.dataLength > size || r.dataLength % kPacketBytes)
				{ Fail(error, "XMA wave %s data 0x%x+0x%x", name, r.dataStart, r.dataLength); return E_Result_Failed; }
			r.level = BE32(&w[0x34]);
			r.flags = BE32(&w[0x38]) & ~kFlagXmaSeekTable;
			r.rate = BE32(&w[x]);
			const unsigned bits = BE32(&w[x + 4]);
			r.channels = BE32(&w[x + 8]);
			const unsigned loopStart = BE32(&w[x + 16]);
			r.loops = w[x + 24];
			// the PC ADPCM decoder plays one or two channels
			if (bits != 16 || r.channels < 1 || r.channels > 2 || r.rate < 8000 || r.rate > 96000)
				{ Fail(error, "XMA wave %s: %u bits, %u channels, %u Hz", name, bits, r.channels, r.rate); return E_Result_Failed; }
			// PCMWave loops the whole wave. An XMA loop start is a bit offset into the data;
			// bit 32 is the first frame (every 2 KB packet opens with a 32-bit header), so
			// 0 and 32 loop the whole wave and a later start loses its intro on PC.
			if (r.loops && loopStart > 32)
				++st.partialLoops;
			rows.push_back(r);
			at += chunk;
		}

		// the PC bank: header and PCM rows now, each wave's DATA chunk as it is decoded
		unsigned char bank[kBankHeaderSize];
		memcpy(bank, "BANK", 4);
		SetLE32(bank + 4, kBankHeaderSize);
		SetLE32(bank + 8, 4);
		SetLE32(bank + 12, waves);
		bank[0x10] = 0;				// chEndian: little
		bank[0x11] = streaming;
		memcpy(bank + 0x12, head + 0x12, kBankHeaderSize - 0x12);	// name and padding
		if (!dst.Write(bank, sizeof(bank)))
			{ Fail(error, "write failed"); return E_Result_Failed; }
		std::vector<unsigned long long> rowAt(rows.size());
		for (size_t i = 0; i < rows.size(); ++i)
		{
			const Row& r = rows[i];
			if (!r.pcRow.empty())
			{
				rowAt[i] = dst.Position();
				if (!dst.Write(&r.pcRow[0], r.pcRow.size()))
					{ Fail(error, "write failed"); return E_Result_Failed; }
				continue;
			}
			// an ADPCM row as the PC banks have it: uSize and uNrInterleavedStreams 0,
			// 4 bits; uNrSamples (0x44) with uDataStart and uDataLength (0x2C, 0x30) once
			// the wave is decoded
			unsigned char row[kWaveBaseSize + kAdpcmWaveSize] = { 0 };
			memcpy(row, "WAVE", 4);
			SetLE32(row + 4, sizeof(row));
			SetLE32(row + 8, kFormatAdpcm4Bit);
			memcpy(row + 12, r.name.c_str(), r.name.size() < 32 ? r.name.size() : 32);
			SetLE32(row + 0x34, r.level);	// fDefaultLevel (bits)
			SetLE32(row + 0x38, r.flags);
			SetLE32(row + 0x40, r.rate);
			row[0x48] = r.loops ? 1 : 0;	// uLoop: WaveBundler writes IsLoop() ? 1 : 0
			row[0x4A] = 4;					// uBitsPerSample
			row[0x4B] = (unsigned char)r.channels;
			rowAt[i] = dst.Position();
			if (!dst.Write(row, sizeof(row)))
				{ Fail(error, "write failed"); return E_Result_Failed; }
		}
		std::vector<unsigned char> xma;
		std::vector<short> pcm;
		std::vector<unsigned char> adpcm;
		for (size_t i = 0; i < rows.size(); ++i)
		{
			const Row& r = rows[i];
			xma.resize(r.dataLength);
			if (r.dataLength && !src.Read(r.dataStart, &xma[0], r.dataLength))
				{ Fail(error, "wave %s data unreadable", r.name.c_str()); return E_Result_Failed; }
			const void* data;
			unsigned long long bytes;
			pcm.clear();
			if (!r.pcRow.empty())
			{
				if (r.swap16)
					for (size_t k = 0; k + 1 < xma.size(); k += 2)
						std::swap(xma[k], xma[k + 1]);
				data = xma.empty() ? NULL : &xma[0];
				bytes = xma.size();
			}
			else
			{
				std::string e;
				if (!DecodeXma(xma.empty() ? NULL : &xma[0], xma.size(), r.rate, r.channels, pcm, e))
				{
					error = e + " (wave " + r.name + ")";
					return E_Result_Failed;
				}
				const unsigned frames = (unsigned)(pcm.size() / r.channels);
				EncodeAdpcm(pcm, r.channels, adpcm);
				data = adpcm.empty() ? NULL : &adpcm[0];
				bytes = adpcm.size();
				++st.xmaWaves;
				st.samples += frames;
				unsigned char nrSamples[4];
				SetLE32(nrSamples, frames);
				if (!dst.WriteAt(rowAt[i] + 0x44, nrSamples, 4))
					{ Fail(error, "write failed"); return E_Result_Failed; }
			}
			if (streaming)
			{
				static const unsigned char zeros[kStreamAlign] = { 0 };
				const unsigned long long pad = Align(dst.Position() + 8, kStreamAlign) - (dst.Position() + 8);
				if (pad && !dst.Write(zeros, (size_t)pad))
					{ Fail(error, "write failed"); return E_Result_Failed; }
			}
			unsigned char chunk[8];
			memcpy(chunk, "DATA", 4);
			SetLE32(chunk + 4, (unsigned)bytes + 8);	// the chunk's size counts its header (every PC bank)
			if (!dst.Write(chunk, 8))
				{ Fail(error, "write failed"); return E_Result_Failed; }
			const unsigned long long start = dst.Position();
			if (start + bytes > 0xFFFFFFFFull)
				{ Fail(error, "PC bank past 4 GB at wave %s", r.name.c_str()); return E_Result_Failed; }
			if (bytes && !dst.Write(data, (size_t)bytes))
				{ Fail(error, "write failed"); return E_Result_Failed; }
			unsigned char patch[8];
			SetLE32(patch, (unsigned)start);
			SetLE32(patch + 4, (unsigned)bytes);
			if (!dst.WriteAt(rowAt[i] + 0x2C, patch, 8))
				{ Fail(error, "write failed"); return E_Result_Failed; }
		}
		// a streaming bank ends on a 2 KB boundary (all 19 retail PC streaming banks do; the
		// streamer reads them in 2 KB-aligned chunks), an in-memory bank where its data ends
		if (streaming)
		{
			static const unsigned char zeros[kStreamAlign] = { 0 };
			const unsigned long long pad = Align(dst.Position(), kStreamAlign) - dst.Position();
			if (pad && !dst.Write(zeros, (size_t)pad))
				{ Fail(error, "write failed"); return E_Result_Failed; }
		}
		return E_Result_Converted;
	}

	E_Result ConvertXmaBank(const unsigned char* in, size_t size, std::vector<unsigned char>& out,
		std::string& error, XmaBankStats* stats)
	{
		C_MemoryReader reader(in, size);
		C_MemoryWriter writer(out);
		return ConvertXmaBankStream(reader, writer, error, stats);
	}
}
