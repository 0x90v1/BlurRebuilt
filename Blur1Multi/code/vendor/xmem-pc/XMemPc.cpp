// [Blur1Multi] PC implementation of the XDK XMem decompression API. See xcompress.h.
//
// XMem LZX data is a chain of frames (docs/analysis.md "XMem frame chain";
// Blur2Test _notes/disc_resource_map.md "The container"):
//   [u16 BE compressedSize][compressed]                       -> 0x8000 output bytes
//   [0xFF][u16 BE outputSize][u16 BE compressedSize][compressed] -> outputSize bytes
//   a header whose compressedSize is 0 ends the chain (the stream writer emits five zero bytes).
// Each frame's compressed bytes start a fresh bit reader; the LZX state
// (window, repeated offsets, Huffman tables, current block) carries over from
// frame to frame. libmspack's lzxd keeps exactly that state; this file feeds
// it one frame at a time from memory.

#include "xcompress.h"

#include <stdlib.h>
#include <string.h>

extern "C" {
#include <mspack.h>
#include <lzx.h>
}

namespace
{
	const unsigned kFrameSize = 0x8000;			// LZX_FRAME_SIZE
	const int kDefaultWindowBits = 17;			// XMem LZX default window: 128 KB
	const unsigned kMaxCompressedFrame = 0x8000 + 0x1800;	// generous bound for a single frame

	// A memory "file" for libmspack: the input side serves one frame's
	// compressed bytes, the output side writes into caller memory.
	struct MemFile
	{
		const unsigned char* in;
		size_t inSize;
		size_t inPos;
		unsigned char* out;
		size_t outCap;
		size_t outPos;
	};

	int MemRead(struct mspack_file* file, void* buffer, int bytes)
	{
		MemFile* f = (MemFile*)file;
		size_t left = f->inSize - f->inPos;
		size_t n = (size_t)bytes < left ? (size_t)bytes : left;
		memcpy(buffer, f->in + f->inPos, n);
		f->inPos += n;
		return (int)n;
	}

	int MemWrite(struct mspack_file* file, void* buffer, int bytes)
	{
		MemFile* f = (MemFile*)file;
		if (f->outPos + (size_t)bytes > f->outCap)
			return -1;
		memcpy(f->out + f->outPos, buffer, (size_t)bytes);
		f->outPos += (size_t)bytes;
		return bytes;
	}

	struct mspack_file* MemOpen(struct mspack_system*, const char*, int) { return NULL; }
	void MemClose(struct mspack_file*) {}
	int MemSeek(struct mspack_file*, off_t, int) { return -1; }
	off_t MemTell(struct mspack_file*) { return 0; }
	void MemMessage(struct mspack_file*, const char*, ...) {}
	void* MemAlloc(struct mspack_system*, size_t bytes) { return malloc(bytes); }
	void MemFree(void* p) { free(p); }
	void MemCopy(void* src, void* dst, size_t bytes) { memmove(dst, src, bytes); }

	struct mspack_system g_memSystem =
	{
		MemOpen, MemClose, MemRead, MemWrite, MemSeek, MemTell, MemMessage, MemAlloc, MemFree, MemCopy, NULL
	};

	enum E_StreamState { E_Header, E_Body, E_Ended };

	struct Context
	{
		int windowBits;
		bool stream;
		MemFile io;
		struct lzxd_stream* lzx;

		// stream mode
		E_StreamState state;
		unsigned char header[5];
		unsigned headerHave;
		unsigned char* frame;			// kMaxCompressedFrame bytes
		unsigned frameCompressed;
		unsigned frameHave;
		unsigned frameOutput;
		unsigned char decoded[kFrameSize];
		unsigned decodedHave;
		unsigned decodedPos;
	};

	bool ResetDecoder(Context& c)
	{
		if (c.lzx)
			lzxd_free(c.lzx);
		memset(&c.io, 0, sizeof c.io);
		// input_buffer_size is how much lzxd asks read() for at once; a frame is at most a little over 32 KB
		c.lzx = lzxd_init(&g_memSystem, (struct mspack_file*)&c.io, (struct mspack_file*)&c.io,
			c.windowBits, 0, 4096, 0, 0);
		c.state = E_Header;
		c.headerHave = 0;
		c.frameCompressed = c.frameHave = c.frameOutput = 0;
		c.decodedHave = c.decodedPos = 0;
		return c.lzx != NULL;
	}

	// Decodes one frame's compressed bytes into dst (outputSize bytes).
	bool DecodeFrame(Context& c, const unsigned char* src, unsigned compressedSize, unsigned char* dst, unsigned outputSize)
	{
		if (!c.lzx || outputSize == 0 || outputSize > kFrameSize)
			return false;
		struct lzxd_stream* lzx = c.lzx;
		// fresh bit reader over this frame only
		lzx->i_ptr = lzx->i_end = lzx->inbuf;
		lzx->bit_buffer = 0;
		lzx->bits_left = 0;
		lzx->input_end = 0;
		c.io.in = src;
		c.io.inSize = compressedSize;
		c.io.inPos = 0;
		c.io.out = dst;
		c.io.outCap = outputSize;
		c.io.outPos = 0;
		// a short frame is the last of its chain: tell lzxd where the output ends
		lzx->length = lzx->offset + (off_t)outputSize;
		// lzxd_decompress runs frames up to (offset + bytes) / 32768 + 1, so asking for
		// exactly one full frame also runs an empty trailing iteration (frame size 0,
		// since length == offset) that still counts a frame. Keep the count at one per
		// real frame, or the next short frame is never decoded ("bytes left to output").
		const unsigned frameBefore = lzx->frame;
		int err = lzxd_decompress(lzx, (off_t)outputSize);
		lzx->frame = frameBefore + 1;
		return err == MSPACK_ERR_OK && c.io.outPos == outputSize;
	}

	// Parses a frame header. Returns the header length (2 or 5), 0 if more bytes are needed.
	unsigned ParseHeader(const unsigned char* h, unsigned have, unsigned& compressedSize, unsigned& outputSize)
	{
		if (have < 1)
			return 0;
		if (h[0] == 0xFF)
		{
			if (have < 5)
				return 0;
			outputSize = (h[1] << 8) | h[2];
			compressedSize = (h[3] << 8) | h[4];
			return 5;
		}
		if (have < 2)
			return 0;
		compressedSize = (h[0] << 8) | h[1];
		outputSize = kFrameSize;
		return 2;
	}

	Context* NewContext(CONST VOID* pCodecParams, DWORD flags)
	{
		Context* c = (Context*)calloc(1, sizeof(Context));
		if (!c)
			return NULL;
		c->windowBits = kDefaultWindowBits;
		if (pCodecParams)
		{
			DWORD window = ((const XMEMCODEC_PARAMETERS_LZX*)pCodecParams)->WindowSize;
			if (window)
			{
				int bits = 0;
				while ((1u << bits) < window)
					++bits;
				c->windowBits = bits;
			}
		}
		c->stream = (flags & XMEMCOMPRESS_STREAM) != 0;
		c->frame = (unsigned char*)malloc(kMaxCompressedFrame);
		if (!c->frame || !ResetDecoder(*c))
		{
			if (c->lzx)
				lzxd_free(c->lzx);
			free(c->frame);
			free(c);
			return NULL;
		}
		return c;
	}

	void DeleteContext(Context* c)
	{
		if (!c)
			return;
		if (c->lzx)
			lzxd_free(c->lzx);
		free(c->frame);
		free(c);
	}
}

extern "C" {

HRESULT WINAPI XMemCreateDecompressionContext(XMEMCODEC_TYPE, CONST VOID* pCodecParams, DWORD Flags, XMEMDECOMPRESSION_CONTEXT* pContext)
{
	if (!pContext)
		return E_INVALIDARG;
	*pContext = NewContext(pCodecParams, Flags);
	return *pContext ? S_OK : E_OUTOFMEMORY;
}

VOID WINAPI XMemDestroyDecompressionContext(XMEMDECOMPRESSION_CONTEXT Context)
{
	DeleteContext((::Context*)Context);
}

HRESULT WINAPI XMemResetDecompressionContext(XMEMDECOMPRESSION_CONTEXT Context)
{
	if (!Context)
		return E_INVALIDARG;
	return ResetDecoder(*(::Context*)Context) ? S_OK : E_OUTOFMEMORY;
}

HRESULT WINAPI XMemDecompress(XMEMDECOMPRESSION_CONTEXT Context, VOID* pDestination, SIZE_T* pDestSize, CONST VOID* pSource, SIZE_T SrcSize)
{
	if (!pDestination || !pDestSize || (!pSource && SrcSize))
		return E_INVALIDARG;
	::Context* temp = NULL;
	::Context* c = (::Context*)Context;
	if (!c)
	{
		temp = c = NewContext(NULL, 0);
		if (!c)
			return E_OUTOFMEMORY;
	}
	else if (!ResetDecoder(*c))
		return E_OUTOFMEMORY;

	const unsigned char* src = (const unsigned char*)pSource;
	unsigned char* dst = (unsigned char*)pDestination;
	const SIZE_T cap = *pDestSize;
	SIZE_T in = 0, out = 0;
	HRESULT hr = S_OK;
	while (in < SrcSize)
	{
		unsigned compressedSize = 0, outputSize = 0;
		unsigned hl = ParseHeader(src + in, (unsigned)(SrcSize - in), compressedSize, outputSize);
		if (hl == 0)
		{
			hr = XMCDERR_CORRUPT;	// truncated header
			break;
		}
		if (compressedSize == 0)
			break;					// end of chain
		in += hl;
		if (compressedSize > SrcSize - in)
		{
			hr = XMCDERR_CORRUPT;
			break;
		}
		if (outputSize > cap - out)
		{
			hr = XMCDERR_BUFFERTOOSMALL;
			break;
		}
		if (!DecodeFrame(*c, src + in, compressedSize, dst + out, outputSize))
		{
			hr = XMCDERR_CORRUPT;
			break;
		}
		in += compressedSize;
		out += outputSize;
	}
	*pDestSize = out;
	if (temp)
		DeleteContext(temp);
	return hr;
}

HRESULT WINAPI XMemDecompressStream(XMEMDECOMPRESSION_CONTEXT Context, VOID* pDestination, SIZE_T* pDestSize, CONST VOID* pSource, SIZE_T* pSrcSize)
{
	if (!Context || !pDestSize || !pSrcSize)
		return E_INVALIDARG;
	::Context& c = *(::Context*)Context;
	const unsigned char* src = (const unsigned char*)pSource;
	unsigned char* dst = (unsigned char*)pDestination;
	const SIZE_T srcSize = *pSrcSize;
	const SIZE_T cap = *pDestSize;
	SIZE_T in = 0, out = 0;

	for (;;)
	{
		// hand out what the last frame produced
		if (c.decodedPos < c.decodedHave)
		{
			if (out == cap)
				break;
			SIZE_T n = c.decodedHave - c.decodedPos;
			if (n > cap - out)
				n = cap - out;
			memcpy(dst + out, c.decoded + c.decodedPos, n);
			c.decodedPos += (unsigned)n;
			out += n;
			continue;
		}
		if (c.state == E_Ended)
		{
			in = srcSize;			// the terminator's trailing bytes
			break;
		}
		if (in == srcSize)
			break;					// need more input
		if (c.state == E_Header)
		{
			c.header[c.headerHave++] = src[in++];
			unsigned hl = ParseHeader(c.header, c.headerHave, c.frameCompressed, c.frameOutput);
			if (hl == 0)
				continue;
			c.headerHave = 0;
			if (c.frameCompressed == 0)
			{
				c.state = E_Ended;
				continue;
			}
			if (c.frameCompressed > kMaxCompressedFrame || c.frameOutput == 0 || c.frameOutput > kFrameSize)
				return XMCDERR_CORRUPT;
			c.frameHave = 0;
			c.state = E_Body;
			continue;
		}
		// E_Body
		SIZE_T n = c.frameCompressed - c.frameHave;
		if (n > srcSize - in)
			n = srcSize - in;
		memcpy(c.frame + c.frameHave, src + in, n);
		c.frameHave += (unsigned)n;
		in += n;
		if (c.frameHave < c.frameCompressed)
			continue;
		if (!DecodeFrame(c, c.frame, c.frameCompressed, c.decoded, c.frameOutput))
			return XMCDERR_CORRUPT;
		c.decodedHave = c.frameOutput;
		c.decodedPos = 0;
		c.state = E_Header;
	}

	*pSrcSize = in;
	*pDestSize = out;
	return S_OK;
}

HRESULT WINAPI XMemCreateCompressionContext(XMEMCODEC_TYPE, CONST VOID*, DWORD, XMEMCOMPRESSION_CONTEXT* pContext)
{
	if (pContext)
		*pContext = NULL;
	return E_NOTIMPL;
}

VOID WINAPI XMemDestroyCompressionContext(XMEMCOMPRESSION_CONTEXT) {}
HRESULT WINAPI XMemResetCompressionContext(XMEMCOMPRESSION_CONTEXT) { return E_NOTIMPL; }
HRESULT WINAPI XMemCompress(XMEMCOMPRESSION_CONTEXT, VOID*, SIZE_T*, CONST VOID*, SIZE_T) { return E_NOTIMPL; }
HRESULT WINAPI XMemCompressStream(XMEMCOMPRESSION_CONTEXT, VOID*, SIZE_T*, CONST VOID*, SIZE_T*) { return E_NOTIMPL; }

}
