// [Blur1Multi] PC implementation of the Xbox 360 XDK XMem compression API
// (xcompress.h), decompression side only.
//
// The engine's XMem pak codecs (PakCompressXMem.cpp, PakCompressPacketXMem.cpp)
// and Compression::XMemDecompress (misc_shared) are written against this API.
// Bizarre's PC tool builds linked the XDK's Win32 Xcompress.lib for it
// (misc_shared/jamfile.jam, USE_XMEM_COMPRESSION); that library is not
// available here, so this header and XMemPc.cpp stand in for it. The LZX
// bitstream is decoded by libmspack's lzxd.c (../libmspack, LGPL 2.1).
//
// Declarations follow the XDK names and signatures the engine calls. The
// compression entry points exist so the engine's writer paths compile; they
// return E_NOTIMPL (phase 2 never writes Xbox paks).
#pragma once

#ifndef _WINDOWS_
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _XMEMCODEC_TYPE
{
	XMEMCODEC_DEFAULT = 0,
	XMEMCODEC_LZX = 1
} XMEMCODEC_TYPE;

typedef struct _XMEMCODEC_PARAMETERS_LZX
{
	DWORD Flags;
	DWORD WindowSize;				// bytes; 0 = default (128 KB)
	DWORD CompressionPartitionSize;	// accepted, not used by the decoder
} XMEMCODEC_PARAMETERS_LZX;

#define XMEMCOMPRESS_STREAM			0x00000001

typedef VOID* XMEMDECOMPRESSION_CONTEXT;
typedef VOID* XMEMCOMPRESSION_CONTEXT;

// Result codes. XMCDERR_MOREDATA is the value the engine tests after a compress call.
#define XMCDERR_MOREDATA			((HRESULT)0x81DE2001L)
#define XMCDERR_BUFFERTOOSMALL		((HRESULT)0x81DE2002L)
#define XMCDERR_CORRUPT				((HRESULT)0x81DE2003L)

HRESULT WINAPI XMemCreateDecompressionContext(XMEMCODEC_TYPE CodecType, CONST VOID* pCodecParams, DWORD Flags, XMEMDECOMPRESSION_CONTEXT* pContext);
VOID    WINAPI XMemDestroyDecompressionContext(XMEMDECOMPRESSION_CONTEXT Context);
HRESULT WINAPI XMemResetDecompressionContext(XMEMDECOMPRESSION_CONTEXT Context);

// One-shot: pSource holds a whole XMem frame chain. *pDestSize is the capacity
// on entry and the number of bytes produced on return. Context may be NULL
// (Compression::XMemDecompressGetContext returns NULL for worker ids outside
// its table); a temporary decoder is used then.
HRESULT WINAPI XMemDecompress(XMEMDECOMPRESSION_CONTEXT Context, VOID* pDestination, SIZE_T* pDestSize, CONST VOID* pSource, SIZE_T SrcSize);

// Streaming (context created with XMEMCOMPRESS_STREAM): consumes up to *pSrcSize
// bytes and produces up to *pDestSize bytes; both are updated to what was
// actually consumed and produced. Input may be consumed without output while a
// frame is incomplete. A call with *pSrcSize == 0 flushes decoded bytes.
HRESULT WINAPI XMemDecompressStream(XMEMDECOMPRESSION_CONTEXT Context, VOID* pDestination, SIZE_T* pDestSize, CONST VOID* pSource, SIZE_T* pSrcSize);

HRESULT WINAPI XMemCreateCompressionContext(XMEMCODEC_TYPE CodecType, CONST VOID* pCodecParams, DWORD Flags, XMEMCOMPRESSION_CONTEXT* pContext);
VOID    WINAPI XMemDestroyCompressionContext(XMEMCOMPRESSION_CONTEXT Context);
HRESULT WINAPI XMemResetCompressionContext(XMEMCOMPRESSION_CONTEXT Context);
HRESULT WINAPI XMemCompress(XMEMCOMPRESSION_CONTEXT Context, VOID* pDestination, SIZE_T* pDestSize, CONST VOID* pSource, SIZE_T SrcSize);
HRESULT WINAPI XMemCompressStream(XMEMCOMPRESSION_CONTEXT Context, VOID* pDestination, SIZE_T* pDestSize, CONST VOID* pSource, SIZE_T* pSrcSize);

#ifdef __cplusplus
}
#endif
