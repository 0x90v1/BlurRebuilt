#include "stdafx.h"
#include "Display/Device/Device.h"
#include "Display/Texture/TextureDesc.h"
#include <system\serialise\C_MemoryStream.h>

#include <d3d9.h>

namespace Display
{
namespace D3D9
{

#if PLATFORM_PC

S32 D3DLoadTexture2D(IDirect3DTexture9* pTexture, Sys::I_BinaryStream* pStream)
{
	if(!pTexture)
		return 0;

	Sys::BinaryDeserialiser<Ut::NoEndianSwap> ar(*pStream);

	S32 totalSize = 0;
	
	const U32 levelCount = pTexture->GetLevelCount();
	for(U32 level=0; level<levelCount; level++)
	{
		D3DSURFACE_DESC	surfaceDesc;
		HRESULT hr = pTexture->GetLevelDesc(level, &surfaceDesc);
		BIZ_ASSERT(SUCCEEDED(hr));

		D3DLOCKED_RECT lockedRect;
		hr = pTexture->LockRect(level, &lockedRect, NULL, 0);
		BIZ_ASSERT(SUCCEEDED(hr));

		const U32 levelSize = D3D9Texture2DSize(surfaceDesc.Width, surfaceDesc.Height, surfaceDesc.Format);
		BIZ_ASSERT(levelSize > 0);
		ar.Read(lockedRect.pBits, levelSize);

		hr = pTexture->UnlockRect(level);
		BIZ_ASSERT(SUCCEEDED(hr));

		totalSize += levelSize;
	}

	return totalSize;
}

S32 D3DLoadTextureCube(IDirect3DCubeTexture9* pTexture, Sys::I_BinaryStream* pStream)
{
	if(!pTexture)
		return 0;

	Sys::BinaryDeserialiser<Ut::NoEndianSwap> ar(*pStream);

	S32 totalSize = 0;
	
	for(U32 face=0; face<6; face++)
	{
		const U32 levelCount = pTexture->GetLevelCount();
		for(U32 level=0; level<levelCount; level++)
		{
			D3DSURFACE_DESC surfaceDesc;
			HRESULT hr = pTexture->GetLevelDesc(level, &surfaceDesc);

			D3DLOCKED_RECT lockedRect;
			pTexture->LockRect((D3DCUBEMAP_FACES)face, level, &lockedRect, NULL, 0);
			BIZ_ASSERT(SUCCEEDED(hr));

			const U32 levelSize = D3D9Texture2DSize(surfaceDesc.Width, surfaceDesc.Height, surfaceDesc.Format);
			BIZ_ASSERT(levelSize > 0);
			ar.Read(lockedRect.pBits, levelSize);

			pTexture->UnlockRect((D3DCUBEMAP_FACES)face, level);
			BIZ_ASSERT(SUCCEEDED(hr));

			totalSize += levelSize;
		}
	}

	return totalSize;
}

void PCLoadTexture(C_Texture2D *pTexture, Sys::I_BinaryStream *pStream)
{
	TextureDesc desc;
	Sys::BinaryDeserialiser<Ut::NoEndianSwap> ar(*pStream);
	ar >> desc;

	Device()->CreateTexture2D(desc.Width, desc.Height, desc.Levels, 0, (E_Format)desc.Format, pTexture);

	if(desc.Type == 3) // 2D
	{
		const U32 size = D3DLoadTexture2D((IDirect3DTexture9*)pTexture->m_pd3dTexture, pStream);
		BIZ_ASSERT(size == pTexture->GetSize());
	}
	else
	if(desc.Type == 5) // Cube
	{
		pTexture->m_pd3dTexture->Release();

		IDirect3DCubeTexture9* pd3dTexture = NULL;
		
		const HRESULT hr = Device()->GetD3DDevice()->CreateCubeTexture(
			desc.Width,
			desc.Levels,
			D3D9TextureUsage(0),
			(D3DFORMAT)desc.Format,
			D3D9Pool(0),
			&pd3dTexture, NULL);
		BIZ_ASSERT(SUCCEEDED(hr));

		pTexture->m_pd3dTexture = pd3dTexture;

		const U32 size = D3DLoadTextureCube((IDirect3DCubeTexture9*)pTexture->m_pd3dTexture, pStream);
		//BIZ_ASSERT(size == pTexture->GetSize());
	}
	else
	{
		LOGERROR(g_Log, "Unsupported texture type %i width %i height %i depth %i levels %i format %i usage %i\n",
			desc.Type, desc.Width, desc.Height, desc.Depth, desc.Levels, desc.Format, desc.Usage);
	}
}

void PCLoadSurfaces(C_Texture2D* pTexture, Sys::I_BinaryStream *pStream, int size)
{
	IDirect3DTexture9* Tex = (IDirect3DTexture9*)pTexture->m_pd3dTexture;

    // skip unsupported texture types
	if (!Tex)
		return;

	S32 totalSize = 0;

	Sys::BinaryDeserialiser<Ut::NoEndianSwap> ar(*pStream);

	U32 NMipmaps = Tex->GetLevelCount();
	for (U32 Level = 0; Level < NMipmaps; ++Level)
	{
		if (totalSize == size)
			break; // reached the end of the stream

		BIZ_ASSERT(totalSize < size);

		IDirect3DSurface9* Surface;
		Tex->GetSurfaceLevel(Level, &Surface);

		D3DSURFACE_DESC	SurfaceDesc;
		Surface->GetDesc(&SurfaceDesc);
		D3DLOCKED_RECT Lock;
		Tex->LockRect(Level, &Lock, NULL, 0);

		size_t Size = D3D9Texture2DSize(SurfaceDesc.Width, SurfaceDesc.Height, SurfaceDesc.Format);
		ar.Read(Lock.pBits, Size);
		totalSize += Size;

		Tex->UnlockRect(Level);
		Surface->Release();
	}
}

void PCCopySurfaces(C_Texture2D *pDst, C_Texture2D *pSrc)
{
	IDirect3DTexture9* pDst_D3D = (IDirect3DTexture9*)pDst->m_pd3dTexture;	
	IDirect3DTexture9* pSrc_D3D = (IDirect3DTexture9*)pSrc->m_pd3dTexture;

	S32 nSrcLevels = pSrc_D3D->GetLevelCount();
	S32 nDstLevels = pDst_D3D->GetLevelCount();

	BIZ_ASSERT(nDstLevels != nSrcLevels);
	S32 d = nDstLevels - nSrcLevels;

	for (S32 level = 0; level < Maths::Min(nSrcLevels, nDstLevels); level++)
	{
		S32 srcLevel, dstLevel;
		
		if (nDstLevels > nSrcLevels)
		{
			srcLevel = level;
			dstLevel = srcLevel + d;
		}
		else
		{
			srcLevel = level - d;
			dstLevel = level;
		}

		IDirect3DSurface9* srcSurface;
		pSrc_D3D->GetSurfaceLevel(srcLevel, &srcSurface);

		D3DSURFACE_DESC	SurfaceDesc;
		srcSurface->GetDesc(&SurfaceDesc);
		U32 size = D3D9Texture2DSize(SurfaceDesc.Width, SurfaceDesc.Height, SurfaceDesc.Format);

		D3DLOCKED_RECT srcLock;
		pSrc_D3D->LockRect(srcLevel, &srcLock, NULL, 0);

		D3DLOCKED_RECT dstLock;
		pDst_D3D->LockRect(dstLevel, &dstLock, NULL, 0);
		
		memcpy(dstLock.pBits, srcLock.pBits, size);

		pSrc_D3D->UnlockRect(srcLevel);
		pDst_D3D->UnlockRect(dstLevel);
		srcSurface->Release();
	}
}

void LoadTextureData2D(C_Texture2D *pTexture, Sys::I_BinaryStream *pStream, int size)
{
	PCLoadSurfaces(pTexture, pStream, size);
}

void LoadTexture2D(C_Texture2D *pTexture, Sys::I_BinaryStream *pStream)
{
	CreateTexture2D(pTexture, pStream);
	LoadTextureData2D(pTexture, pStream, pTexture->GetSize());
}

S32 CreateTexture2D(C_Texture2D *pTexture, Sys::I_BinaryStream *pStream)
{
	TextureDesc desc;
	Sys::BinaryDeserialiser<Ut::NoEndianSwap> ar(*pStream);
	ar >> desc;

	Device()->CreateTexture2D(desc.Width, desc.Height, desc.Levels, 0, (E_Format)desc.Format, pTexture);

	return static_cast<S32>(pTexture->GetSize());
}

void CloneTexture2D(C_Texture2D *pDst, const C_Texture2D *pSrc, int newWidth, int newHeight, int newLevels, int newSize)
{
	IDirect3DTexture9* pD3DTexture = (IDirect3DTexture9*) pSrc->m_pd3dTexture;
	D3DSURFACE_DESC desc;
	pD3DTexture->GetLevelDesc(0, &desc);
	Device()->CreateTexture2D(newWidth, newHeight, newLevels, 0, (E_Format)desc.Format, pDst, NULL);
}

void AllocateTextureMemory(C_Texture2D *pTexture)
{
}

void CopyTextureData2D(C_Texture2D *pDst, const C_Texture2D *pSrc)
{
	PCCopySurfaces(pDst, const_cast<C_Texture2D*>(pSrc));
}

#elif PLATFORM_XENON

void CreateTexture(C_Texture2D *pTexture, bool allocate)
{
	BIZ_ASSERT(pTexture);
	BIZ_ASSERT(allocate);

	pTexture->m_d3dTexture.Format.BaseAddress = 0;

	#if TRACK_DISPLAY2_RESOURCES
	Display2::Device()->InsertTexture(pTexture);
	#endif

	// debug
	//U32 width = pTexture->GetWidth();
	//U32 height = pTexture->GetHeight();
	//E_Format format = pTexture->GetFormat();
	//int breakhere = 0;

	//D3DSURFACE_DESC desc;
	//((IDirect3DTexture9*)pTexture->m_pd3dTexture)->GetLevelDesc(0, &desc);
	//U32 d3dFormat = (U32)desc.Format;

	//if ((d3dFormat & (D3DFORMAT_SIGNX_MASK | D3DFORMAT_SIGNY_MASK | D3DFORMAT_SIGNZ_MASK)) == (D3DFORMAT_SIGNX_MASK | D3DFORMAT_SIGNY_MASK | D3DFORMAT_SIGNZ_MASK))
	if (pTexture->m_d3dTexture.Format.SignX == 3 && pTexture->m_d3dTexture.Format.SignY == 3 && pTexture->m_d3dTexture.Format.SignZ == 3)
	{
		//Set flag
		pTexture->SetFlags(pTexture->GetFlags() | Display::E_Format_SRGB);
	}

	if (allocate)
	{
		AllocateTextureMemory(pTexture);

		#if TRACK_DISPLAY2_RESOURCES
		if (pTexture->GetAddress() == 0)
			Display2::Device()->RemoveTexture(pTexture);
		#endif
	}
}

S32 CreateTexture2D(C_Texture2D *pTexture, Sys::I_BinaryStream *pStream, bool allocate)
{
	BIZ_ASSERT(pTexture);

	if (pTexture->IsCreated())
		Display::Device()->DestroyTexture2D(pTexture);

	// read header
	Sys::BinaryDeserialiser<Ut::NoEndianSwap> ar(*pStream);
	ar.Read(&pTexture->m_d3dTexture, sizeof(pTexture->m_d3dTexture));

	// read texture data
	int size, alignment;
	U32 dummy;
	ar >> size >> alignment >> dummy;

	BIZ_ASSERT(size > 0);

	pTexture->SetSize(size);

	CreateTexture(pTexture, allocate);

	return size;
}

void CloneTexture2D(C_Texture2D *pDst, const C_Texture2D *pSrc, int newWidth, int newHeight, int newLevels, int newSize)
{
	BIZ_ASSERT(pDst && pSrc);

	if (pDst->IsCreated())
		Display::Device()->DestroyTexture2D(pDst);

	XGTEXTURE_DESC desc;
	XGGetTextureDesc((D3DTexture*)&pSrc->m_d3dTexture, 0, &desc);

	/*U32 size =*/ XGSetTextureHeader(newWidth, newHeight, newLevels, D3DUSAGE_CPU_CACHED_MEMORY, desc.Format, (D3DPOOL) 0, 0,
									XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, &pDst->m_d3dTexture2D, 0, 0);

	pDst->SetSize(newSize);
	pDst->SetFlags(pSrc->GetFlags());

	CreateTexture(pDst, true);
}

void LoadTexture2D(C_Texture2D *pTexture, Sys::I_BinaryStream *pStream)
{
	S32 size = CreateTexture2D(pTexture, pStream);

	//S32 size = pTexture->GetSize();
	Sys::BinaryDeserialiser<Ut::NoEndianSwap> ar(*pStream);
	U8 *p = (U8*)pTexture->GetAddress();
	//BIZ_ASSERT(size);

	ar.Read(p, size);

	if (p)
	{
		// flush the gpu cache
		for(int offset=0; offset < size; offset+=128)
			__dcbf(offset, p);
	}
}

void AllocateTextureMemory(C_Texture2D *pTexture)
{
	BIZ_ASSERT(pTexture);
	BIZ_ASSERT(pTexture->GetSize() > 0);
	BIZ_ASSERT(pTexture->GetAddress() == 0);

	U8 *p = NULL;
	U32 flags = 0;
	//Device()->AllocResourceMemory(E_ResourceType_Texture2D, flags, pTexture->GetSize(), &p, Device()->MemoryGetSystemID());
	Device()->AllocMemory(4096, pTexture->GetSize(), &p, Display::E_AllocHeap_Large, E_AllocType_Start, Display2::E_MemoryType_Texture);
	flags = Display::E_Usage_Video;
	pTexture->SetFlags(flags);

#ifndef CFG_SPEED
	pTexture->m_d3dTexture.Common |= 0x00400000; // turn on D3DUSAGE_RUNCOMMANDBUFFER_TIMESTAMP;
#endif

	XGOffsetResourceAddress(&pTexture->m_d3dTexture, p);
}

void CopyGPUData(void* pDst, const void* pSrc, S32 size)
{
	XMemCpyStreaming(pDst, pSrc, size);

	// flush the gpu cache
	for(int offset=0; offset < size; offset+=128)
		__dcbf(offset, pDst);
}

#endif

// TODO: write a xenon-specific version of this function that can take load a bunch of data without memcopying and locking

}; // namespace D3D9
}; // namespace Display
