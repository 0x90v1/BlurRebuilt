#include "stdafx.h"
#include "Display/Texture/Texture3DUtils.h"
#include "Display/Device/Device.h"
#include "Display/Utils/File.h"
#include "Display/Texture/DDS/DDSTextureUtils.h"
#include <System/Misc/StringUtil.h>

#ifdef CFG_ALLOW_OPTIMIZE_OFF
//#pragma optimize( "", off ) // CFG_ALLOW_OPTIMIZE_OFF 
#endif

namespace Display
{

#if PLATFORM_XENON

bool UploadTexture3D_Tiled(C_Texture3D* pTexture, C_TextureParams* pTextureParams)
{
	BIZ_ASSERT_FAIL("TODO: Implement me!");

	return false;
}

#endif

bool UploadTexture3D(C_Texture3D* pTexture, C_TextureParams* pTextureParams)
{
#if PLATFORM_XENON
	if(!(pTextureParams->m_Flags & E_Format_Linear))
		return UploadTexture3D_Tiled(pTexture, pTextureParams);
#endif

	BIZ_ASSERT(pTexture);
	
	if(!pTexture->IsCreated())
		return false;
	
	BIZ_ASSERT(pTextureParams);
	BIZ_ASSERT(pTextureParams->m_pMipData);
	BIZ_ASSERT(pTextureParams->m_MipSize > 0);
	BIZ_ASSERT(pTextureParams->m_MipOffset < pTextureParams->m_MipSize);

	U8* pData = &pTextureParams->m_pMipData[pTextureParams->m_MipOffset];

	U32 width = pTextureParams->m_Width;
	U32 height = pTextureParams->m_Height;
	U32 depth = pTextureParams->m_Depth;

	const Display2::E_TextureFormat format = GetTextureFormatFromFormat(pTextureParams->m_Format);

	U32 slicePitch = Display2::GetTextureSlicePitch(width, height, pTextureParams->m_Levels, pTextureParams->m_Flags, format);
	
	for(U32 level=pTextureParams->m_MipHead; level<=pTextureParams->m_MipTail; level++)
	{
		BIZ_ASSERT(pData <= pTextureParams->m_pMipData+pTextureParams->m_MipSize);

		C_LockedBox lockedBox = pTexture->LockBox3D(level);

		if(!lockedBox.GetBits())
		{
			LOGERROR(g_Log, "Failed to lock 3d texture level %i\n", level);
			return false;
		}

		U32 rowPitch = Display2::GetTextureRowPitch(width, pTextureParams->m_Flags, format);
		U32 size = Display2::GetTexture3DSize(width, height, depth, pTextureParams->m_Flags, format);
		
		BIZ_ASSERT(pData+size <= pTextureParams->m_pMipData+pTextureParams->m_MipSize);

#if PLATFORM_XENON
		D3DFORMAT d3dFormat = D3D9Format(pTextureParams->m_Format, pTextureParams->m_Flags);
		if(pTextureParams->m_Flags & E_Format_SRGB)
			d3dFormat = (D3DFORMAT)MAKESRGBFMT(d3dFormat);
		XGEndianSwapVolume(lockedBox.GetBits(), lockedBox.GetRowPitch(), lockedBox.GetSlicePitch(), pData, rowPitch, slicePitch, width, height, depth, d3dFormat);
#else
		memcpy(lockedBox.GetBits(), pData, size);
#endif
		
		pData += size;
		
		pTexture->UnlockBox3D(level);
		
		if(width > 1)
			width >>= 1;
		if(height > 1)
			height >>= 1;
	}

	BIZ_ASSERT(pData == pTextureParams->m_pMipData+pTextureParams->m_MipSize);

	return true;
}


U32 GetTexture3DSize(const U32 width, const U32 height, const U32 depth, const U32 flags, const E_Format format)
{
#if PLATFORM_XENON
	return XGSetVolumeTextureHeader(width, height, depth, 1, 0, D3D9Format(format), 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, NULL, NULL, NULL);
#else
	return Display2::GetTexture3DSize(width, height, depth, flags, GetTextureFormatFromFormat(format));
#endif
}

U32 GetTexture3DSize(U32 width, U32 height, const U32 depth, const U32 levels, const U32 flags, const E_Format format)
{
#if PLATFORM_XENON
	return XGSetVolumeTextureHeader(width, height, depth, 1, 0, D3D9Format(format), 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, NULL, NULL, NULL);
#else
	return Display2::GetTexture3DSize(width, height, depth, levels, flags, GetTextureFormatFromFormat(format));
#endif
}

#if defined(GRAPHICS_D3D9)
bool D3DXCreateTexture3DFromFile(const Util::C_String& fileName, C_Texture3D* pTexture)
{
#if 1
	BIZ_ASSERT_FAIL("TODO: Fix me!");
	return false;
#else
	D3D_MEMORY_STAT_TEXTURE_3D
		
	assert(pTexture);

	if(pTexture->IsCreated())
	{
		LOGERROR(g_Log, "Failed to create the 3d texture as its already created %s\n", fileName.Chars());
		return false;
	}

	U32 fileSize = 0;
	U8* pFileData = NULL;
	if(!GetFileData(fileName.Chars(), fileSize, &pFileData))
		return false;

	IDirect3DVolumeTexture9* pd3dTexture = NULL;
	const HRESULT hr = D3DXCreateVolumeTextureFromFileInMemory(Device()->GetD3DDevice(), pFileData, fileSize, &pd3dTexture);

	SAFE_DELETE_ARY(pFileData);
	
	if(FAILED(hr))
	{
		LOGERROR(g_Log, "Failed to create the 3d texture from file in memory %s\n", fileName.Chars());
		return false;
	}

	pTexture->m_pd3dTexture = pd3dTexture;

	pTexture->SetSize(GetTexture3DSize(pTexture->GetWidth(), pTexture->GetHeight(), pTexture->GetDepth(), pTexture->GetLevels(), pTexture->GetFormat()));
	pTexture->SetFlags(E_Usage_Managed);
	pTexture->SetFileName(fileName);
	
	Device()->m_Texture3Ds.Insert(pTexture);

	pTexture->Acquire();

	return true;
#endif
}
#endif

bool CreateTexture3DFromFile( Util::C_String fileName, C_Texture3D* pTexture, const U32 flags)
{
	if(fileName == "")
	{
		LOGERROR(g_Log, "No file name specified when creating 3d texture from file\n");
		return false;
	}

	fileName.ConvertToLower();

#if 0
	return D3DXCreateTexture3DFromFile(fileName, pTexture);
#endif

	C_TextureParams params;
	
	if(fileName.Search(".dds") != -1)
	{
		if(!DDS::LoadTextureFromFile(fileName, &params))
			return false;
	}
	else
	{
		LOGERROR(g_Log, "Unsupported 3d texture file extension '%s'\n", fileName.Chars());
		return false;
	}

	params.m_Name = fileName;

	#if defined(PLATFORM_PS3) && !defined(CFG_GOLD)
	if((params.m_Width & (params.m_Width-1)) || (params.m_Height & (params.m_Height-1)) || (params.m_Depth & (params.m_Depth-1)))
	{
		LOGERROR(g_Log, "Texture '%s' is not power of 2 (%ix%ix%i)\n", fileName.Chars(), params.m_Width, params.m_Height, params.m_Depth);
	}
	#endif

	params.m_Flags = flags;
	params.m_Flags |= E_Format_Linear;
	params.m_Flags |= E_Format_Raw;
	if(!Device()->CreateTexture3D(params.m_Width, params.m_Height, params.m_Depth, params.m_Levels, params.m_Flags, params.m_Format, pTexture))
	{
		SAFE_DELETE_ARY(params.m_pMipData);
		return false;
	}
	
	params.m_MipHead = 0;
	params.m_MipTail = params.m_Levels-1;

	if(!UploadTexture3D(pTexture, &params))
	{
		Device()->DestroyTexture3D(pTexture);
		SAFE_DELETE_ARY(params.m_pMipData);
		LOGERROR(g_Log, "Failed to upload the 3d texture '%s'\n", fileName.Chars());
		return false;
	}
	
	#if TRACK_DISPLAY2_RESOURCES
	Display2::Device()->SetTextureName(pTexture, fileName);
	#endif

	SAFE_DELETE_ARY(params.m_pMipData);

	return true;
}

}; // namespace Display
