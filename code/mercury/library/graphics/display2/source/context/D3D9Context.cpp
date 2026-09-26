#include "StdAfx.h"
#include "Display2/Context/D3D9Context.h"

#if DONT_INLINE_DISPLAY2
#undef INLINE
#define INLINE
#endif

#if DONT_INLINE_DISPLAY2
#include "Display2/Context/D3D9Context.inl"
#endif

namespace Display2
{

#if DONT_INLINE_DISPLAY2

#define GPU_CLASS C_Context
#define GPU_CHECK 1
#define GPU_TRACK 1
#include "Display2/Context/D3D9Functions.inl"
#undef GPU_TRACK
#undef GPU_CHECK
#undef GPU_CLASS

#define GPU_CLASS C_ContextNoCheck
#define GPU_TRACK 1
#include "Display2/Context/D3D9Functions.inl"
#undef GPU_TRACK
#undef GPU_CLASS

#define GPU_CLASS C_ContextNoCheckNoTrack
#include "Display2/Context/D3D9Functions.inl"
#undef GPU_CLASS

#endif
	
E_SurfaceFormat GetSurfaceFormatFromTexture(const E_TextureFormat format)
{
	return (E_SurfaceFormat)format;
}

U32 GetSurfacePitchFromWidth(U32 width, const U32 flags, const E_SurfaceFormat format, const E_Multisample multisample)
{
	U32 pitch = 0;

	switch(format)
	{
	case E_SurfaceFormat_L8:
	case E_SurfaceFormat_A8:
		pitch = width * 1;
		break;
	case E_SurfaceFormat_D16:
		pitch = width * 2;
		break;
	case E_SurfaceFormat_A8R8G8B8:
	case E_SurfaceFormat_D24S8:
	case E_SurfaceFormat_R32F:
		pitch = width * 4;
		break;
	case E_SurfaceFormat_A16B16G16R16F:
		pitch = width * 8;
		break;
	case E_SurfaceFormat_A32B32G32R32F:
		pitch = width * 16;
		break;
	default:
		assert(0 && "Unsupported surface format");
		break;
	}

	switch(multisample)
	{
	case E_Multisample_None:
		break;
	case E_Multisample_2:
	case E_Multisample_4:
		pitch = pitch * 2;
		break;
	default:
		assert(0 && "Unsupported surface multisample");
		break;
	}

	return pitch;
}

U32 GetSurfaceSizeFromHeightAndPitch(U32 height, const U32 pitch, const U32 flags, const E_SurfaceFormat format, const E_Multisample multisample)
{
	switch(multisample)
	{
	case E_Multisample_None:
	case E_Multisample_2:
		break;
	case E_Multisample_4:
		height = height * 2;
		break;
	default:
		assert(0 && "Unsupported surface multisample");
		break;
	}

	U32 size = pitch * height;

	return size;
}

U32 GetSurfaceSize(const U32 width, const U32 height, const U32 flags, const E_SurfaceFormat format, const E_Multisample multisample)
{
#if PLATFORM_XENON
	if(!(flags & E_Flags_Raw))
		return XGSurfaceSize(width, height, (D3DFORMAT)format, (D3DMULTISAMPLE_TYPE)multisample);
#endif

	const U32 pitch = GetSurfacePitchFromWidth(width, flags, format, multisample);
	return GetSurfaceSizeFromHeightAndPitch(height, pitch, flags, format, multisample);
}

void GetSurfaceSizeAndPitch(const U32 width, const U32 height, const U32 flags, const E_SurfaceFormat format, const E_Multisample multisample, U32& size, U32& pitch)
{
	pitch = GetSurfacePitchFromWidth(width, flags, format, multisample);
	size = GetSurfaceSize(width, height, flags, format, multisample);
}

U32 GetTexturePitchFromWidth(U32 width, const U32 flags, const E_TextureFormat format)
{
	U32 pitch = 0;

	switch(format)
	{
	case E_TextureFormat_DXT1:
		pitch = ((width + 3) / 4) * 8;
		break;
	case E_TextureFormat_DXT3:
	case E_TextureFormat_DXT5:
		pitch = ((width + 3) / 4) * 16;	
		break;
	case E_TextureFormat_L8:
	case E_TextureFormat_A8:
		pitch = width * 1;
		break;
	case E_TextureFormat_D16:
		pitch = width * 2;
		break;
	case E_TextureFormat_A8R8G8B8:
	case E_TextureFormat_X8R8G8B8:
	case E_TextureFormat_D24S8:
	case E_TextureFormat_R32F:
		pitch = width * 4;
		break;
	case E_TextureFormat_A16B16G16R16F:
		return width * 8;
	case E_TextureFormat_A32B32G32R32F:
		return width * 16;
	default:
		assert(0 && "Unsupported texture format");
		break;
	}

	return pitch;
}

U32 GetTextureSizeFromHeightAndPitch(U32 height, const U32 pitch, const U32 flags, const E_TextureFormat format)
{
	switch(format)
	{
	case E_TextureFormat_DXT1:
	case E_TextureFormat_DXT3:
	case E_TextureFormat_DXT5:
		height = ((height + 3) / 4);
		break;
	default:
		break;
	}

	U32 size = pitch * height;

	return size;
}


U32 GetTexturePitch(U32 width, const U32 flags, const E_TextureFormat format)
{
	return GetTexturePitchFromWidth(width, flags, format);
}

U32 GetTextureRowPitch(U32 width, const U32 flags, const E_TextureFormat format)
{
	return GetTexturePitchFromWidth(width, flags, format);
}

U32 GetTextureSlicePitch(U32 width, U32 height, U32 levels, const U32 flags, const E_TextureFormat format)
{
	U32 pitch = GetTexturePitchFromWidth(width, flags, format);
	U32 size = GetTextureSizeFromHeightAndPitch(height, pitch, flags, format);
	return size * levels;
}

U32 GetTexture2DSize(const U32 width, const U32 height, const U32 flags, const E_TextureFormat format)
{
#if PLATFORM_XENON
	if(!(flags & E_Flags_Raw))
		return XGSetTextureHeader(width, height, 1, 0, (D3DFORMAT)format, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, NULL, NULL, NULL);
#endif

	const U32 pitch = GetTexturePitchFromWidth(width, flags, format);
	return GetTextureSizeFromHeightAndPitch(height, pitch, flags, format);
}

U32 GetTexture2DSize(U32 width, U32 height, const U32 levels, const U32 flags, const E_TextureFormat format)
{
#if PLATFORM_XENON
	if(!(flags & E_Flags_Raw))
		return XGSetTextureHeader(width, height, levels, 0, (D3DFORMAT)format, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, NULL, NULL, NULL);
#endif

	U32 size = 0;
	for(U32 level=0; level<levels; level++)
	{
		size += GetTexture2DSize(width, height, flags, format);
		if(width > 1)
			width >>= 1;
		if(height > 1)
			height >>= 1;
	}
	return size;
}

void GetTexture2DSizeAndPitch(const U32 width, const U32 height, const U32 levels, const U32 flags, const E_TextureFormat format, U32& size, U32& pitch)
{
	pitch = GetTexturePitchFromWidth(width, flags, format);
	size = GetTexture2DSize(width, height, levels, flags, format);
}

U32 GetTexture3DSize(const U32 width, const U32 height, const U32 depth, const U32 flags, const E_TextureFormat format)
{
#if PLATFORM_XENON
	if(!(flags & E_Flags_Raw))
		return XGSetVolumeTextureHeader(width, height, depth, 1, 0, (D3DFORMAT)format, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, NULL, NULL, NULL);
#endif

	return GetTexture2DSize(width, height, flags, format) * depth;
}

U32 GetTexture3DSize(U32 width, U32 height, const U32 depth, const U32 levels, const U32 flags, const E_TextureFormat format)
{
#if PLATFORM_XENON
	if(!(flags & E_Flags_Raw))
		return XGSetVolumeTextureHeader(width, height, depth, 1, 0, (D3DFORMAT)format, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, NULL, NULL, NULL);
#endif

	U32 size = 0;
	for(U32 level = 0; level < levels; level++)
	{
		size += GetTexture3DSize(width, height, depth, flags, format);
		if(width > 1)
			width >>= 1;
		if(height > 1)
			height >>= 1;
	}
	return size;
}

void GetTexture3DSizeAndPitch(const U32 width, const U32 height, const U32 depth, const U32 levels, const U32 flags, const E_TextureFormat format, U32& size, U32& pitch)
{
	pitch = GetTexturePitchFromWidth(width, flags, format);
	size = GetTexture3DSize(width, height, depth, levels, flags, format);
}

U32 GetTextureCubeSize(const U32 width, const U32 flags, const E_TextureFormat format)
{
#if PLATFORM_XENON
	if(!(flags & E_Flags_Raw))
		return XGSetCubeTextureHeader(width, 1, 0, (D3DFORMAT)format, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, NULL, NULL, NULL);
#endif

	U32 size = 0;
	for(U32 i=0; i<6; i++)
	{
		size += GetTexture2DSize(width, width, flags, format);
	}
	return size;
}

U32 GetTextureCubeSize(U32 width, const U32 levels, const U32 flags, const E_TextureFormat format)
{
#if PLATFORM_XENON
	if(!(flags & E_Flags_Raw))
		return XGSetCubeTextureHeader(width, levels, 0, (D3DFORMAT)format, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, NULL, NULL, NULL);
#endif

	U32 size = 0;
	for(U32 i=0; i<6; i++)
	{
		size += GetTexture2DSize(width, width, levels, flags, format);
	}
	return size;
}

void GetTextureCubeSizeAndPitch(const U32 width, const U32 levels, const U32 flags, const E_TextureFormat format, U32& size, U32& pitch)
{
	pitch = GetTexturePitchFromWidth(width, flags, format);
	size = GetTextureCubeSize(width, levels, flags, format);
}

U32 GetTexture2DLevelOffset(const U32 level, U32 width, U32 height, const U32 levels, const U32 flags, const E_TextureFormat format)
{
#if PLATFORM_XENON
	BIZ_ASSERT_FAIL("TODO: Implement me!");
#endif

	U32 offset = 0;

	for(U32 i=0; i<level; i++)
	{
		if(i >= levels)
			return offset;

		const U32 size = GetTexture2DSize(width, height, flags, format);

		offset += size;

		if(width > 1)
			width /= 2;
		if(height > 1)
			height /= 2;
	}

	return offset;
}

U32 GetTexture3DLevelOffset(const U32 level, U32 width, U32 height, U32 depth, const U32 levels, const U32 flags, const E_TextureFormat format)
{
#if PLATFORM_XENON
	BIZ_ASSERT_FAIL("TODO: Implement me!");
#endif

	U32 offset = 0;

	for(U32 i=0; i<level; i++)
	{
		if(i >= levels)
			return offset;

		const U32 size = GetTexture3DSize(width, height, depth, flags, format);

		offset += size;

		if(width > 1)
			width /= 2;
		if(height > 1)
			height /= 2;
		if(depth > 1)
			depth /= 2;
	}

	return offset;
}

U32 GetTextureCubeLevelOffset(const E_CubeMapFace face, const U32 level, U32 width, const U32 levels, const U32 flags, const E_TextureFormat format)
{
#if PLATFORM_XENON
	BIZ_ASSERT_FAIL("TODO: Implement me!");
#endif

	U32 offset = 0;

	for(S32 f=0; f<face; f++)
	{
		const U32 size = GetTexture2DSize(width, width, levels, flags, format);

		offset += size;
	}

	for(U32 l=0; l<level; l++)
	{
		if(l >= levels)
			return offset;

		const U32 size = GetTextureCubeSize(width, flags, format);

		offset += size;

		if(width > 1)
			width /= 2;
	}

	return offset;
}

#if PLATFORM_XENON

U32 SetTexture2DHeader(const U32 width, const U32 height, const U32 levels, const U32 flags, const E_TextureFormat format, const U32 pitch, GPUTexture2D* pTexture)
{
	BIZ_ASSERT(pTexture);
	
	DWORD d3dUsage = 0;
	if(!(flags & E_Flags_Dynamic))
		d3dUsage = D3DUSAGE_CPU_CACHED_MEMORY;

	#if !CFG_SPEED
	d3dUsage |= D3DUSAGE_RUNCOMMANDBUFFER_TIMESTAMP;
	#endif
	
	D3DFORMAT d3dFormat = (D3DFORMAT)format;
	if(flags & E_Flags_Linear)
		d3dFormat = (D3DFORMAT)MAKELINFMT(d3dFormat);
	
	U32 size = XGSetTextureHeader(width, height, levels, d3dUsage, d3dFormat, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, pTexture, NULL, NULL);

	U32 intRefCount = GPUCOMMON_DEBUG_INTREFCOUNT(pTexture->Common);
	BIZ_ASSERT_MSG_SPEED(intRefCount == 0, "intRefCount %i", intRefCount);

	return size;
}

U32 SetTexture3DHeader(const U32 width, const U32 height, const U32 depth,const U32 levels, const U32 flags, const E_TextureFormat format, const U32 pitch, GPUTexture3D* pTexture)
{
	BIZ_ASSERT(pTexture);

	DWORD d3dUsage = 0;
	if(!(flags & E_Flags_Dynamic))
		d3dUsage = D3DUSAGE_CPU_CACHED_MEMORY;

	#if !CFG_SPEED
	d3dUsage |= D3DUSAGE_RUNCOMMANDBUFFER_TIMESTAMP;
	#endif

	D3DFORMAT d3dFormat = (D3DFORMAT)format;
	if(flags & E_Flags_Linear)
		d3dFormat = (D3DFORMAT)MAKELINFMT(d3dFormat);
	
	return XGSetVolumeTextureHeader(width, height, depth, levels, d3dUsage, d3dFormat, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, pTexture, NULL, NULL);
}

U32 SetTextureCubeHeader(const U32 width, const U32 levels, const U32 flags, const E_TextureFormat format, const U32 pitch, GPUTextureCube* pTexture)
{
	BIZ_ASSERT(pTexture);

	DWORD d3dUsage = 0;
	if(!(flags & E_Flags_Dynamic))
		d3dUsage = D3DUSAGE_CPU_CACHED_MEMORY;

	#if !CFG_SPEED
	d3dUsage |= D3DUSAGE_RUNCOMMANDBUFFER_TIMESTAMP;
	#endif

	D3DFORMAT d3dFormat = (D3DFORMAT)format;
	if(flags & E_Flags_Linear)
		d3dFormat = (D3DFORMAT)MAKELINFMT(d3dFormat);

	return XGSetCubeTextureHeader(width, levels, d3dUsage, d3dFormat, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, pTexture, NULL, NULL);
}

#endif

bool DebugCheckGPUTexture(const GPUTexture* pgpuTexture)
{
	if(!pgpuTexture)
		return true;

#if DEVICE_XB2
	D3DBaseTexture* pBaseTexture = (D3DBaseTexture*)pgpuTexture;
	if(pBaseTexture)
	{
		BIZ_ASSERT((pBaseTexture->Format.BaseAddress << GPU_TEXTURE_ADDRESS_SHIFT) != 0);
		BIZ_ASSERT(pBaseTexture->GetLevelCount() > 0);
		
		if(pBaseTexture->GetType() == D3DRTYPE_TEXTURE)
		{
			D3DTexture* pTexture = (D3DTexture*)pBaseTexture;

			D3DSURFACE_DESC surfaceDesc;
			pTexture->GetLevelDesc(0, &surfaceDesc);
			
			BIZ_ASSERT(surfaceDesc.Type == D3DRTYPE_TEXTURE);
			BIZ_ASSERT(surfaceDesc.Width > 0);
			BIZ_ASSERT(surfaceDesc.Height > 0);
		}
		else
		if(pBaseTexture->GetType() == D3DRTYPE_VOLUMETEXTURE)
		{
			D3DVolumeTexture* pVolumeTexture = (D3DVolumeTexture*)pBaseTexture;
			
			D3DVOLUME_DESC volumeDesc;
			pVolumeTexture->GetLevelDesc(0, &volumeDesc);

			BIZ_ASSERT(volumeDesc.Type == D3DRTYPE_VOLUMETEXTURE);
			BIZ_ASSERT(volumeDesc.Width > 0);
			BIZ_ASSERT(volumeDesc.Height > 0);
			BIZ_ASSERT(volumeDesc.Depth > 0);
		}
		else
		if(pBaseTexture->GetType() == D3DRTYPE_CUBETEXTURE)
		{
			D3DCubeTexture* pCubeTexture = (D3DCubeTexture*)pBaseTexture;
			
			D3DSURFACE_DESC surfaceDesc;
			pCubeTexture->GetLevelDesc(0, &surfaceDesc);
			BIZ_ASSERT(surfaceDesc.Type == D3DRTYPE_CUBETEXTURE);
			BIZ_ASSERT(surfaceDesc.Width > 0);
		}
		else
		{
			BIZ_ASSERT_FAIL("Unsupported d3d resource type %i", pBaseTexture->GetType());
		}
	}
#endif
	
	return true;
}

bool DebugCheckTexture(const C_Texture* pTexture)
{
	if(!pTexture)
		return true;
	if(!pTexture->IsCreated())
		return true;
	return DebugCheckGPUTexture((GPUTexture*)pTexture->GetGPUTexture());
}

const Util::C_String GetD3DFormatAsString(const D3DFORMAT format)
{
	switch(format)
	{
	case D3DFMT_A8R8G8B8:
		return "A8R8G8B8";
	case D3DFMT_X8R8G8B8:
		return "X8R8G8B8";
	case D3DFMT_D24S8:
		return "D24S8";
	case D3DFMT_D24FS8:
		return "D24FS8";
	default:
		return "UNKNOWN";
	}
}

const Util::C_String GetD3DMultisampleTypeAsString(const D3DMULTISAMPLE_TYPE multisampleType)
{
	switch(multisampleType)
	{
	case D3DMULTISAMPLE_NONE:
		return "NONE";
	#ifdef PLATFORM_PC
    case D3DMULTISAMPLE_NONMASKABLE:
		return "NONMASKABLE";
	#endif
    case D3DMULTISAMPLE_2_SAMPLES:
		return "2_SAMPLES";
    #ifdef PLATFORM_PC
    case D3DMULTISAMPLE_3_SAMPLES:
		return "3_SAMPLES";
	#endif
    case D3DMULTISAMPLE_4_SAMPLES:
		return "4_SAMPLES";
    #ifdef PLATFORM_PC
	case D3DMULTISAMPLE_5_SAMPLES:
		return "5_SAMPLES";
	case D3DMULTISAMPLE_6_SAMPLES:
		return "6_SAMPLES";
    case D3DMULTISAMPLE_7_SAMPLES:
		return "7_SAMPLES";
    case D3DMULTISAMPLE_8_SAMPLES:
		return "8_SAMPLES";
    case D3DMULTISAMPLE_9_SAMPLES:
		return "9_SAMPLES";
    case D3DMULTISAMPLE_10_SAMPLES:
		return "10_SAMPLES";
    case D3DMULTISAMPLE_11_SAMPLES:
		return "11_SAMPLES";
    case D3DMULTISAMPLE_12_SAMPLES:
		return "12_SAMPLES";
    case D3DMULTISAMPLE_13_SAMPLES:
		return "13_SAMPLES";
    case D3DMULTISAMPLE_14_SAMPLES:
		return "14_SAMPLES";
    case D3DMULTISAMPLE_15_SAMPLES:
		return "15_SAMPLES";
    case D3DMULTISAMPLE_16_SAMPLES:
		return "16_SAMPLES";
	#endif
    default:
		return "UNKNOWN";
	}
}

const Util::C_String GetD3DSwapEffectAsString(const D3DSWAPEFFECT swapEffect)
{
	switch(swapEffect)
	{
	case D3DSWAPEFFECT_DISCARD:
		return "DISCARD";
	case D3DSWAPEFFECT_FLIP:
		return "FLIP";
	case D3DSWAPEFFECT_COPY:
		return "COPY";
	default:
		return "UNKNOWN";
	}
}

const Util::C_String GetD3DPresentFlagsAsString(const DWORD presentFlags)
{
	Util::C_String temp = "";

	#ifdef PLATFORM_PC
	if(presentFlags & D3DPRESENTFLAG_DEVICECLIP)
		temp += "DEVICECLIP/";
	#endif
	if(presentFlags & D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL)
		temp += "DISCARD_DEPTHSTENCIL/";
	#ifdef PLATFORM_PC
	if(presentFlags & D3DPRESENTFLAG_LOCKABLE_BACKBUFFER)
		temp += "LOCKABLE_BACKBUFFER/";
	if(presentFlags & D3DPRESENTFLAG_NOAUTOROTATE)
		temp += "NOAUTOROTATE/";
	if(presentFlags & D3DPRESENTFLAG_UNPRUNEDMODE)
		temp += "UNPRUNEDMODE/";
	if(presentFlags & D3DPRESENTFLAG_VIDEO)
		temp += "VIDEO/";
	#endif

	return temp;
}

const Util::C_String GetD3DPresentIntervalAsString(const UINT presentInterval)
{
	switch(presentInterval)
	{
	//case D3DPRESENT_DONOTFLIP:
		//return "DONOTFLIP";
	//case D3DPRESENT_DONOTWAIT:
		//return "DONOTWAIT";
	//case D3DPRESENT_FLIPRESTART:
		//return "FLIPRESTART";
	case D3DPRESENT_INTERVAL_DEFAULT:
		return "INTERVAL_DEFAULT";
	case D3DPRESENT_INTERVAL_ONE:
		return "INTERVAL_ONE";
	case D3DPRESENT_INTERVAL_TWO:
		return "INTERVAL_TWO";
	case D3DPRESENT_INTERVAL_THREE:
		return "INTERVAL_THREE";
	case D3DPRESENT_INTERVAL_FOUR:
		return "INTERVAL_FOUR";
	case D3DPRESENT_INTERVAL_IMMEDIATE:
		return "INTERVAL_IMMEDIATE";
	//case D3DPRESENT_LINEAR_CONTENT:
		//return "LINEAR_CONTENT";
	#ifdef PLATFORM_PC
	case D3DPRESENT_VIDEO_RESTRICT_TO_MONITOR:
		return "VIDEO_RESTRICT_TO_MONITOR";
	#endif
	default:
		return "UNKNOWN";
	}
}

const Util::C_String GetFlagsAsString(const U32 flags)
{
	Util::C_String str = "";
	
	#define ADD_FLAG_TO_STR(_str, _flags, _flag, _name) if(_flags & _flag) { if(_str != "") _str += "|"; _str += _name; }
	
	ADD_FLAG_TO_STR(str, flags, E_Flags_Video,			"Video");
	ADD_FLAG_TO_STR(str, flags, E_Flags_System,			"System");
	ADD_FLAG_TO_STR(str, flags, E_Flags_Indirect,		"Indirect");
	ADD_FLAG_TO_STR(str, flags, E_Flags_Dynamic,		"Dynamic");
	ADD_FLAG_TO_STR(str, flags, E_Flags_Colour,			"Colour");
	ADD_FLAG_TO_STR(str, flags, E_Flags_DepthStencil,	"DepthStencil");
	ADD_FLAG_TO_STR(str, flags, E_Flags_Raw,			"Raw");
	ADD_FLAG_TO_STR(str, flags, E_Flags_Tiled,			"Tiled");
	ADD_FLAG_TO_STR(str, flags, E_Flags_Linear,			"Linear");
	ADD_FLAG_TO_STR(str, flags, E_Flags_SRGB,			"SRGB");
	ADD_FLAG_TO_STR(str, flags, E_Flags_DoNotTrack,		"DoNotTrack");
	ADD_FLAG_TO_STR(str, flags, E_Flags_DoNotReload,	"DoNotReload");

	return str;
}

const Util::C_String GetIndexFormatAsString(const E_IndexFormat indexFormat)
{
	switch(indexFormat)
	{
	case E_IndexFormat_16:						return "16";	
	case E_IndexFormat_32:						return "32";		
	default:									return "Unknown";
	}
}

const Util::C_String GetMultisampleAsString(const E_Multisample multisample)
{
	switch(multisample)
	{
	case E_Multisample_None:					return "1xAA";	
	case E_Multisample_2:						return "2xAA";		
	case E_Multisample_4:						return "4xAA";		
	default:									return "Unknown";
	}
}

const Util::C_String GetSurfaceFormatAsString(const E_SurfaceFormat format)
{
	switch(format)
	{
	// 8bpp:
	case E_SurfaceFormat_A8: 					return "A8";
	case E_SurfaceFormat_L8: 					return "L8";

	// 16bpp:
	case E_SurfaceFormat_R5G6B5: 				return "R5G6B5";
	case E_SurfaceFormat_L6V5U5: 				return "L6V5U5";
	case E_SurfaceFormat_X1R5G5B5: 				return "X1R5G5B5";
	case E_SurfaceFormat_A1R5G5B5: 				return "A1R5G5B5";
	case E_SurfaceFormat_A4R4G4B4: 				return "A4R4G4B4";
	case E_SurfaceFormat_X4R4G4B4: 				return "X4R4G4B4";
	case E_SurfaceFormat_A8L8: 					return "A8L8";
	case E_SurfaceFormat_V8U8: 					return "V8U8";
	case E_SurfaceFormat_D16: 					return "D16";
	case E_SurfaceFormat_L16: 					return "L16";
	case E_SurfaceFormat_R16F: 					return "R16F";
	case E_SurfaceFormat_UYVY: 					return "UYVY";
	case E_SurfaceFormat_G8R8_G8B8: 			return "G8R8_G8B8";
	case E_SurfaceFormat_R8G8_B8G8: 			return "R8G8_B8G8";
	case E_SurfaceFormat_YUY2:					return "YUY2";
	#if DEVICE_XB2
	case E_SurfaceFormat_R6G5B5:				return "R6G5B5";
	case E_SurfaceFormat_Q4W4V4U4:				return "Q4W4V4U4";
	case E_SurfaceFormat_G8R8:					return "G8R8";
	case E_SurfaceFormat_R16F_EXPAND: 			return "R16F_EXPAND";
	case E_SurfaceFormat_LE_LIN_UYVY: 			return "LE_UYVY";
	case E_SurfaceFormat_LE_LIN_YUY2: 			return "LE_YUY2";
	#endif

	// 32bpp:
	case E_SurfaceFormat_A8R8G8B8: 				return "A8R8G8B8";
	case E_SurfaceFormat_X8R8G8B8: 				return "X8R8G8B8";
	case E_SurfaceFormat_A8B8G8R8: 				return "A8B8G8R8";
	case E_SurfaceFormat_X8B8G8R8: 				return "X8B8G8R8";
	case E_SurfaceFormat_X8L8V8U8: 				return "X8L8V8U8";
	case E_SurfaceFormat_Q8W8V8U8: 				return "Q8W8V8U8";
	case E_SurfaceFormat_A2R10G10B10: 			return "A2R10G10B10";
	case E_SurfaceFormat_A2B10G10R10: 			return "A2B10G10R10";
	case E_SurfaceFormat_A2W10V10U10: 			return "A2W10V10U10";
	case E_SurfaceFormat_G16R16: 				return "G16R16";
	case E_SurfaceFormat_V16U16: 				return "V16U16";
	case E_SurfaceFormat_G16R16F:				return "G16R16F";
	case E_SurfaceFormat_R32F:					return "R32F";
	case E_SurfaceFormat_D24S8:					return "D24S8";
	case E_SurfaceFormat_D24X8:					return "D24X8";
	case E_SurfaceFormat_D24FS8:				return "D24FS8";
	case E_SurfaceFormat_D32:					return "D32";
	#if DEVICE_XB2
	case E_SurfaceFormat_X2R10G10B10:			return "X2R10G10B10";
	case E_SurfaceFormat_A16L16:				return "A16L16";
	case E_SurfaceFormat_R10G11B11: 			return "R10G11B11";
	case E_SurfaceFormat_R11G11B10: 			return "R11G11B10";
	case E_SurfaceFormat_W10V11U11: 			return "W10V11U11";
	case E_SurfaceFormat_W11V11U10: 			return "W11V11U10";
	case E_SurfaceFormat_G16R16F_EXPAND:		return "G16R16F_EXPAND";
	case E_SurfaceFormat_L32:					return "L32";
	#endif

	// 64bpp:
	case E_SurfaceFormat_A16B16G16R16: 			return "A16B16G16R16";
	case E_SurfaceFormat_Q16W16V16U16: 			return "Q16W16V16U16";
	case E_SurfaceFormat_A16B16G16R16F:			return "A16B16G16R16F";
	case E_SurfaceFormat_G32R32F:				return "G32R32F";
	#if DEVICE_XB2
	case E_SurfaceFormat_A16B16G16R16F_EXPAND:	return "A16B16G16R16F_EXPAND";
	case E_SurfaceFormat_A32L32: 				return "A32L32";
	case E_SurfaceFormat_G32R32: 				return "G32R32";
	case E_SurfaceFormat_V32U32: 				return "V32U32";
	#endif

	// 128bpp:
	case E_SurfaceFormat_A32B32G32R32F:			return "A32B32G32R32F";
	#if DEVICE_XB2
	case E_SurfaceFormat_A32B32G32R32: 			return "A32B32G32R32";
	case E_SurfaceFormat_Q32W32V32U32: 			return "Q32W32V32U32";
	#endif
	
	// EDRAM only:
	#if DEVICE_XB2
	case E_SurfaceFormat_A2B10G10R10F_EDRAM:	return "A2B10G10R10F_EDRAM";
	case E_SurfaceFormat_G16R16_EDRAM:			return "G16R16_EDRAM";
	case E_SurfaceFormat_A16B16G16R16_EDRAM:	return "A16B16G16R16_EDRAM";
	#endif

	// Front buffer formats have to be little endian:
	#if DEVICE_XB2
	case E_SurfaceFormat_LE_X8R8G8B8: 			return "LE_X8R8G8B8";
	case E_SurfaceFormat_LE_A8R8G8B8: 			return "LE_A8R8G8B8";
	case E_SurfaceFormat_LE_X2R10G10B10: 		return "LE_X2R10G10B10";
	case E_SurfaceFormat_LE_A2R10G10B10: 		return "LE_A2R10G10B10";
	#endif

	default:									return "Unknown";
	}
}

const Util::C_String GetTextureFormatAsString(const E_TextureFormat format)
{
	switch(format)
	{
	// DXT:
	case E_TextureFormat_DXT1: 					return "DXT1";
	case E_TextureFormat_DXT3: 					return "DXT3";
	case E_TextureFormat_DXT5: 					return "DXT5";
	#if DEVICE_XB2
	case E_TextureFormat_DXT3A:					return "DXT3A";
	case E_TextureFormat_DXT3A_1111:			return "DXT3A_1111";
	case E_TextureFormat_DXT5A:					return "DXT5A";
	case E_TextureFormat_DXN:					return "DXN";
	case E_TextureFormat_CTX1:					return "CTX1";
	#endif

	// 8bpp:
	case E_TextureFormat_A8: 					return "A8";
	case E_TextureFormat_L8: 					return "L8";

	// 16bpp:
	case E_TextureFormat_R5G6B5: 				return "R5G6B5";
	case E_TextureFormat_L6V5U5: 				return "L6V5U5";
	case E_TextureFormat_X1R5G5B5: 				return "X1R5G5B5";
	case E_TextureFormat_A1R5G5B5: 				return "A1R5G5B5";
	case E_TextureFormat_A4R4G4B4: 				return "A4R4G4B4";
	case E_TextureFormat_X4R4G4B4: 				return "X4R4G4B4";
	case E_TextureFormat_A8L8: 					return "A8L8";
	case E_TextureFormat_V8U8: 					return "V8U8";
	case E_TextureFormat_D16: 					return "D16";
	case E_TextureFormat_L16: 					return "L16";
	case E_TextureFormat_R16F: 					return "R16F";
	case E_TextureFormat_UYVY: 					return "UYVY";
	case E_TextureFormat_G8R8_G8B8: 			return "G8R8_G8B8";
	case E_TextureFormat_R8G8_B8G8: 			return "R8G8_B8G8";
	case E_TextureFormat_YUY2:					return "YUY2";
	#if DEVICE_XB2
	case E_TextureFormat_R6G5B5:				return "R6G5B5";
	case E_TextureFormat_Q4W4V4U4:				return "Q4W4V4U4";
	case E_TextureFormat_G8R8:					return "G8R8";
	case E_TextureFormat_R16F_EXPAND: 			return "R16F_EXPAND";
	case E_TextureFormat_LE_LIN_UYVY: 			return "LE_UYVY";
	case E_TextureFormat_LE_LIN_YUY2: 			return "LE_YUY2";
	#endif

	// 32bpp:
	case E_TextureFormat_A8R8G8B8: 				return "A8R8G8B8";
	case E_TextureFormat_X8R8G8B8: 				return "X8R8G8B8";
	case E_TextureFormat_A8B8G8R8: 				return "A8B8G8R8";
	case E_TextureFormat_X8B8G8R8: 				return "X8B8G8R8";
	case E_TextureFormat_X8L8V8U8: 				return "X8L8V8U8";
	case E_TextureFormat_Q8W8V8U8: 				return "Q8W8V8U8";
	case E_TextureFormat_A2R10G10B10: 			return "A2R10G10B10";
	case E_TextureFormat_A2B10G10R10: 			return "A2B10G10R10";
	case E_TextureFormat_A2W10V10U10: 			return "A2W10V10U10";
	case E_TextureFormat_G16R16: 				return "G16R16";
	case E_TextureFormat_V16U16: 				return "V16U16";
	case E_TextureFormat_G16R16F:				return "G16R16F";
	case E_TextureFormat_R32F:					return "R32F";
	case E_TextureFormat_D24S8:					return "D24S8";
	case E_TextureFormat_D24X8:					return "D24X8";
	case E_TextureFormat_D24FS8:				return "D24FS8";
	case E_TextureFormat_D32:					return "D32";
	#if DEVICE_XB2
	case E_TextureFormat_X2R10G10B10:			return "X2R10G10B10";
	case E_TextureFormat_A16L16:				return "A16L16";
	case E_TextureFormat_R10G11B11: 			return "R10G11B11";
	case E_TextureFormat_R11G11B10: 			return "R11G11B10";
	case E_TextureFormat_W10V11U11: 			return "W10V11U11";
	case E_TextureFormat_W11V11U10: 			return "W11V11U10";
	case E_TextureFormat_G16R16F_EXPAND:		return "G16R16F_EXPAND";
	case E_TextureFormat_L32:					return "L32";
	#endif

	// 64bpp:
	case E_TextureFormat_A16B16G16R16: 			return "A16B16G16R16";
	case E_TextureFormat_Q16W16V16U16: 			return "Q16W16V16U16";
	case E_TextureFormat_A16B16G16R16F:			return "A16B16G16R16F";
	case E_TextureFormat_G32R32F:				return "G32R32F";
	#if DEVICE_XB2
	case E_TextureFormat_A16B16G16R16F_EXPAND:	return "A16B16G16R16F_EXPAND";
	case E_TextureFormat_A16B16G16R16_UINT:		return "A16B16G16R16_UINT";
	case E_TextureFormat_A32L32: 				return "A32L32";
	case E_TextureFormat_G32R32: 				return "G32R32";
	case E_TextureFormat_V32U32: 				return "V32U32";
	#endif

	// 128bpp:
	case E_TextureFormat_A32B32G32R32F:			return "A32B32G32R32F";
	#if DEVICE_XB2
	case E_TextureFormat_A32B32G32R32: 			return "A32B32G32R32";
	case E_TextureFormat_Q32W32V32U32: 			return "Q32W32V32U32";
	#endif
	
	// EDRAM only:
	#if DEVICE_XB2
	case E_TextureFormat_A2B10G10R10F_EDRAM:	return "A2B10G10R10F_EDRAM";
	case E_TextureFormat_G16R16_EDRAM:			return "G16R16_EDRAM";
	case E_TextureFormat_A16B16G16R16_EDRAM:	return "A16B16G16R16_EDRAM";
	#endif

	// Front buffer formats have to be little endian:
	#if DEVICE_XB2
	case E_TextureFormat_LE_X8R8G8B8: 			return "LE_X8R8G8B8";
	case E_TextureFormat_LE_A8R8G8B8: 			return "LE_A8R8G8B8";
	case E_TextureFormat_LE_X2R10G10B10: 		return "LE_X2R10G10B10";
	case E_TextureFormat_LE_A2R10G10B10: 		return "LE_A2R10G10B10";
	#endif

	default:									return "Unknown";
	}
}

const Util::C_String GetTextureTypeAsString(const E_TextureType type)
{
	switch(type)
	{
	case E_TextureType_2D:						return "2D";
	case E_TextureType_3D:						return "3D";	
	case E_TextureType_Cube:					return "Cube";
	default:									return "Unknown";
	}
}

// C_Viewport

C_Viewport::C_Viewport()
{
	X		= 0;
	Y		= 0;
	Width	= 1;
	Height	= 1;
	MinZ	= 0.0f;
	MaxZ	= 1.0f;
}

C_Viewport::C_Viewport(const U32 x, const U32 y, const U32 width, const U32 height)
{
	X		= x;
	Y		= y;
	Width	= width;
	Height	= height;
	MinZ	= 0.0f;
	MaxZ	= 1.0f;
}

C_Viewport::C_Viewport(const U32 x, const U32 y, const U32 width, const U32 height, const F32 minZ, const F32 maxZ)
{
	X		= x;
	Y		= y;
	Width	= width;
	Height	= height;
	MinZ	= minZ;
	MaxZ	= maxZ;
}

// C_VertexElement

C_VertexElement::C_VertexElement()
{
	Stream		= 0xFF;
	Offset		= 0;
	Type		= D3DDECLTYPE_UNUSED;
	Method		= 0;
	Usage		= 0;
	UsageIndex	= 0;
}

C_VertexElement::C_VertexElement(const U16 stream, const U32 offset, const U32 type, const U8 usage, const U8 usageIndex)
{
	Stream		= stream;
	Offset		= offset;
	Type		= type;
	Method		= 0;
	Usage		= usage;
	UsageIndex	= usageIndex;
}

void C_VertexElement::SetStream(const U16 stream)
{
	Stream = stream;
}

void C_VertexElement::SetOffset(const U32 offset)
{
	Offset = offset;
}

void C_VertexElement::SetType(const U32 type)
{
	Type = type;
}

void C_VertexElement::SetUsage(const U8 usage)
{
	Usage = usage;
}

void C_VertexElement::SetUsageIndex(const U8 usageIndex)
{
	UsageIndex = usageIndex;
}

// C_VertexDeclaration

C_VertexDeclaration::C_VertexDeclaration() :
	m_pd3dVertexDeclaration(NULL)
{
}

C_VertexDeclaration::~C_VertexDeclaration()
{
}

S32 C_VertexDeclaration::GetElementCount() const
{
	if(!m_pd3dVertexDeclaration)
		return 0;

	U32 elementCount = 0;
	m_pd3dVertexDeclaration->GetDeclaration(NULL, &elementCount);
	return elementCount-1;
}

bool C_VertexDeclaration::GetElements(const U32 elementCount, C_VertexElement* pElements) const
{
	BIZ_ASSERT(pElements);
	
	if(!m_pd3dVertexDeclaration)
		return false;

	U32 actualCount = 0;
	m_pd3dVertexDeclaration->GetDeclaration(NULL, &actualCount);
	
	if(actualCount-1 != elementCount)
		return false;

	m_pd3dVertexDeclaration->GetDeclaration(pElements, &actualCount);

	return true;
}

U32 C_VertexDeclaration::GetStride(const U32 stream) const
{
	if(!m_pd3dVertexDeclaration)
		return false;

	U32 elementCount = 0;
	m_pd3dVertexDeclaration->GetDeclaration(NULL, &elementCount);

	Btl::Vector<C_VertexElement> elements;
	elements.Resize(elementCount);
	m_pd3dVertexDeclaration->GetDeclaration(&elements[0], &elementCount);
	
	U32 stride = 0;
	for(S32 i=0; i<elements.Size()-1; i++)
	{
		if(stream == elements[i].GetStream())
			stride += elements[i].GetSize();
	}

	elements.Clear();

	return stride;
}

// C_VertexBuffer

C_VertexBuffer::C_VertexBuffer() : m_Flags(0)
{
#if DEVICE_XB2
	ZeroMemory(&m_d3dVertexBuffer, sizeof(GPUVertexBuffer));
#else
	m_pd3dVertexBuffer = NULL;
#endif
}

C_VertexBuffer::~C_VertexBuffer()
{
}

void C_VertexBuffer::SetSize(const U32 size)
{
#if DEVICE_XB2
	m_d3dVertexBuffer.Format.Size = size;
#endif
}

void C_VertexBuffer::SetFlags(const U32 flags)
{
	m_Flags = flags;
}

void C_VertexBuffer::SetAddress(void* pAddress)
{
#if DEVICE_XB2
	m_d3dVertexBuffer.Format.BaseAddress = (U32)pAddress;
#endif
}

void* C_VertexBuffer::Lock(const U32 offset, const U32 size, const U32 flags)
{
	void* pData = NULL;
	BIZ_ASSERT(IsCreated());
#if DEVICE_XB2
	const HRESULT hr = m_d3dVertexBuffer.Lock(offset, size, &pData, flags);
#else
	const HRESULT hr = m_pd3dVertexBuffer->Lock(offset, size, &pData, flags);
#endif
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to lock the d3d vertex buffer header");
	return pData;
}

void* C_VertexBuffer::Lock()
{
	return Lock(0, 0, 0);
}

void C_VertexBuffer::Unlock()
{
	BIZ_ASSERT(IsCreated());
#if DEVICE_XB2
	const HRESULT hr = m_d3dVertexBuffer.Unlock();
#else
	const HRESULT hr = m_pd3dVertexBuffer->Unlock();
#endif
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to unlock the d3d vertex buffer header");
}

// C_IndexBuffer

C_IndexBuffer::C_IndexBuffer() : m_Flags(0)
{
#if DEVICE_XB2
	ZeroMemory(&m_d3dIndexBuffer, sizeof(GPUIndexBuffer));
#else
	m_pd3dIndexBuffer = NULL;
#endif
}

C_IndexBuffer::~C_IndexBuffer()
{
}

void C_IndexBuffer::SetSize(const U32 size)
{
#if DEVICE_XB2
	m_d3dIndexBuffer.Size = size;
#endif
}

void C_IndexBuffer::SetFlags(const U32 flags)
{
	m_Flags = flags;
}

void C_IndexBuffer::SetAddress(void* pAddress)
{
#if DEVICE_XB2
	m_d3dIndexBuffer.Address = (U32)pAddress;
#endif
}

void* C_IndexBuffer::Lock(const U32 offset, const U32 size, const U32 flags)
{
	void* pData = NULL;
	BIZ_ASSERT(IsCreated());
#if DEVICE_XB2
	const HRESULT hr = m_d3dIndexBuffer.Lock(offset, size, &pData, flags);
#else
	const HRESULT hr = m_pd3dIndexBuffer->Lock(offset, size, &pData, flags);
#endif
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to lock the d3d index buffer header");
	return pData;
}

void* C_IndexBuffer::Lock()
{
	return Lock(0, 0, 0);
}

void C_IndexBuffer::Unlock()
{
	BIZ_ASSERT(IsCreated());
#if DEVICE_XB2
	const HRESULT hr = m_d3dIndexBuffer.Unlock();
#else
	const HRESULT hr = m_pd3dIndexBuffer->Unlock();
#endif
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to unlock the d3d index buffer header");
}

// C_ShaderSemantic

C_ShaderSemantic::C_ShaderSemantic() :
	m_Usage(UCHAR_MAX),
	m_UsageIndex(UCHAR_MAX)
{
}

// C_ShaderParam

C_ShaderParam::C_ShaderParam() :
	m_RegisterIndex(UINT_MAX),
	m_RegisterCount(0)
{
}

C_ShaderParam::C_ShaderParam(const U32 registerIndex, const U32 registerCount) :
	m_RegisterIndex(registerIndex),
	m_RegisterCount(registerCount)
{
}

void C_ShaderParam::SetupFromFunc(const char* name, const U8* func)
{
	HRESULT hr;

	LPD3DXCONSTANTTABLE pConstantTable = NULL;
	hr = D3DXGetShaderConstantTable((const DWORD*)func, &pConstantTable);
	if(FAILED(hr))
		return;

	D3DXCONSTANTTABLE_DESC constantTableDesc;
	hr = pConstantTable->GetDesc(&constantTableDesc);
	if(FAILED(hr))
	{
		pConstantTable->Release();
		return;
	}

	for(UINT i=0; i<constantTableDesc.Constants; i++)
	{
		D3DXHANDLE hConstant = pConstantTable->GetConstant(NULL, i);

		D3DXCONSTANT_DESC constantDesc;
		UINT constantCount = 1;
		hr = pConstantTable->GetConstantDesc(hConstant, &constantDesc, &constantCount);
		if(SUCCEEDED(hr))
		{
			if(strcmp(name, constantDesc.Name) == 0)
			{
				m_RegisterIndex = constantDesc.RegisterIndex;
				m_RegisterCount = constantDesc.RegisterCount;
				//pConstantTable->Release();
				//return;
			}
		}
	}

	pConstantTable->Release();
}

// C_VertexShader

C_VertexShader::C_VertexShader() : m_pd3dVertexShader(NULL), m_Flags(0)
{
}

C_VertexShader::~C_VertexShader()
{
}

void C_VertexShader::SetFlags(const U32 flags)
{
	m_Flags = flags;
}

const C_ShaderParam C_VertexShader::GetParamFromName(const char* name) const
{
	C_ShaderParam param;
	if(!m_pd3dVertexShader)
		return param;
	
	UINT size = 0;
	HRESULT hr = m_pd3dVertexShader->GetFunction(NULL, &size);
	if(FAILED(hr))
		return param;

	U8* func = new U8[size];
	hr = m_pd3dVertexShader->GetFunction(func, &size);
	if(SUCCEEDED(hr))
		param.SetupFromFunc(name, func);
	SAFE_DELETE_ARY(func);

	return param;
}

const C_ShaderParam C_VertexShader::GetParameter(const S32 index, const char* name) const
{
	return GetParamFromName(name);
}

bool C_VertexShader::GetSemantics(Btl::Vector<C_ShaderSemantic>& semantics)
{
	if(!GetGPUVertexShader())
		return false;

	UINT size = 0;
	HRESULT hr = GetGPUVertexShader()->GetFunction(NULL, &size);
	if(FAILED(hr))
		return false;

	U8* func = new U8[size];
	hr = GetGPUVertexShader()->GetFunction(func, &size);
	if(FAILED(hr))
		return false;

	bool success = false;

	UINT count = 0;
	hr = D3DXGetShaderInputSemantics((const DWORD*)func, NULL, &count);
	if(SUCCESS(hr))
	{
		if(count > 0)
		{
			D3DXSEMANTIC* pSemantics = new D3DXSEMANTIC[size];
			hr = D3DXGetShaderInputSemantics((const DWORD*)func, pSemantics, &count);
			if(SUCCESS(hr))
			{
				success = true;

				for(UINT i=0; i<count; i++)
				{
					C_ShaderSemantic semantic;
					semantic.m_Usage = pSemantics[i].Usage;
					semantic.m_UsageIndex = pSemantics[i].UsageIndex;
					semantics.PushBack(semantic);
				}
			}
			SAFE_DELETE_ARY(pSemantics);
		}
	}
	
	SAFE_DELETE_ARY(func);

	return success;
}

// C_PixelShader

C_PixelShader::C_PixelShader() : m_pd3dPixelShader(NULL), m_Flags(0)
{
#if DEVICE_XB2
	m_pAddress = NULL;
#endif
}

C_PixelShader::~C_PixelShader()
{
}

void C_PixelShader::SetFlags(const U32 flags)
{
	m_Flags = flags;
}

void C_PixelShader::SetAddress(U8* pAddress)
{
#if DEVICE_XB2
	m_pAddress = pAddress;
#endif
}

const C_ShaderParam C_PixelShader::GetParamFromName(const char* name) const
{
	C_ShaderParam param;
	if(!m_pd3dPixelShader)
		return param;
	
	UINT size = 0;
	HRESULT hr = m_pd3dPixelShader->GetFunction(NULL, &size);
	if(FAILED(hr))
		return param;

	U8* func = new U8[size];
	hr = m_pd3dPixelShader->GetFunction(func, &size);
	if(SUCCEEDED(hr))
		param.SetupFromFunc(name, func);
	SAFE_DELETE_ARY(func);

	return param;
}

const C_ShaderParam C_PixelShader::GetParameter(const S32 index, const char* name) const
{
	return GetParamFromName(name);
}

const C_ShaderParam C_PixelShader::GetParameterArray(const S32 index, const char* name, const U32 count) const
{
	return GetParamFromName(name);
}

// C_LockedRect

C_LockedRect::C_LockedRect()
{
	pBits = NULL;
	pitch = 0;
}

// C_ConstLockedRect

C_ConstLockedRect::C_ConstLockedRect()
{
	pBits = NULL;
	pitch = 0;
}

// C_LockedBox

C_LockedBox::C_LockedBox()
{
	pBits = NULL;
	rowPitch = 0;
	slicePitch = 0;
}

// C_ConstLockedBox

C_ConstLockedBox::C_ConstLockedBox()
{
	pBits = NULL;
	rowPitch = 0;
	slicePitch = 0;
}

// C_SurfaceParams

C_SurfaceParams::C_SurfaceParams()
{
#if DEVICE_XB2
	Base				= 0;
#if 1
	HierarchicalZBase	= 0; // This needs to be zero or it don't work!
#else
	HierarchicalZBase	= 0xFFFFFFFF;
#endif
	ColorExpBias		= 0;
	HiZFunc				= D3DHIZFUNC_DEFAULT;
#endif
}

C_SurfaceParams::~C_SurfaceParams()
{
}

void C_SurfaceParams::OverSurface(const C_Surface* pSurface)
{
#if DEVICE_XB2
	BIZ_ASSERT(pSurface && pSurface->IsCreated());
	
	Base = pSurface->m_d3dSurface.ColorInfo.ColorBase;
#endif
}

void C_SurfaceParams::AfterSurface(const C_Surface* pSurface)
{
#if DEVICE_XB2
	BIZ_ASSERT(pSurface && pSurface->IsCreated());
	
	Base = pSurface->m_d3dSurface.ColorInfo.ColorBase + (pSurface->m_d3dSurface.Size / GPU_EDRAM_TILE_SIZE);
#endif
}

void C_SurfaceParams::BeforeSurface(const C_Surface* pSurface)
{
#if DEVICE_XB2
	BIZ_ASSERT(pSurface && pSurface->IsCreated());
	
	Base = pSurface->m_d3dSurface.ColorInfo.ColorBase - (pSurface->m_d3dSurface.Size / GPU_EDRAM_TILE_SIZE);
#endif
}

// C_Surface

C_Surface::C_Surface() : m_Flags(0)
{
#if DEVICE_XB2
	ZeroMemory(&m_d3dSurface, sizeof(GPUSurface));
#else
	m_pd3dSurface = NULL;
#endif
}

C_Surface::~C_Surface()
{
}

void C_Surface::SetSize(const U32 size)
{
#if DEVICE_XB2
	m_d3dSurface.Size = size;
#endif
}

void C_Surface::SetFlags(const U32 flags)
{
	m_Flags = flags;
}

void C_Surface::SetAddress(U8* pAddress)
{
}

C_LockedRect C_Surface::LockRect(const U32 flags)
{
	BIZ_ASSERT(IsCreated());
	
	D3DLOCKED_RECT d3dLockedRect;
#if DEVICE_XB2
	const HRESULT hr = m_d3dSurface.LockRect(&d3dLockedRect, NULL, flags);
#else
	const HRESULT hr = m_pd3dSurface->LockRect(&d3dLockedRect, NULL, flags);
#endif
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to lock the rect of the d3d surface");

	C_LockedRect lockedRect;
	lockedRect.pBits = (U8*)d3dLockedRect.pBits;
	lockedRect.pitch = d3dLockedRect.Pitch;
	return lockedRect;
}

void C_Surface::UnlockRect()
{
	BIZ_ASSERT(IsCreated());
#if DEVICE_XB2
	const HRESULT hr = m_d3dSurface.UnlockRect();
#else
	const HRESULT hr = m_pd3dSurface->UnlockRect();
#endif
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to unlock the rect of the d3d surface");
}

// C_Texture

C_Texture::C_Texture() : m_Size(0), m_Flags(0)
{
#if DEVICE_XB2
	ZeroMemory(&m_d3dTexture, sizeof(GPUTexture));
#else
	m_pd3dTexture = NULL;
#endif
}

C_Texture::~C_Texture()
{
}

void C_Texture::SetSize(const U32 size)
{
	m_Size = size;
}

void C_Texture::SetFlags(const U32 flags)
{
	m_Flags = flags;
}

void C_Texture::SetAddress(U8* pAddress)
{
#if DEVICE_XB2
	m_d3dTexture.Format.BaseAddress = (U32)pAddress >> GPU_TEXTURE_ADDRESS_SHIFT;
#endif
}

void C_Texture::SetSRGB(const bool enable)
{
	if(enable)
	{
		m_Flags |= E_Flags_SRGB;

		#if DEVICE_XB2
		m_d3dTexture.Format.SignX = GPUSIGN_GAMMA;
		m_d3dTexture.Format.SignY = GPUSIGN_GAMMA;
		m_d3dTexture.Format.SignZ = GPUSIGN_GAMMA;
		#endif
	}
	else
	{
		m_Flags &= ~E_Flags_SRGB;

		#if DEVICE_XB2
		BIZ_ASSERT(m_d3dTexture.Format.SignX != GPUSIGN_GAMMA);
		BIZ_ASSERT(m_d3dTexture.Format.SignY != GPUSIGN_GAMMA);
		BIZ_ASSERT(m_d3dTexture.Format.SignZ != GPUSIGN_GAMMA);
		#endif
	}
}

C_LockedRect C_Texture::LockRect2D(const U32 level, const U32 flags)
{
	BIZ_ASSERT(IsCreated());
	BIZ_ASSERT(GetType() == E_TextureType_2D);
	
	D3DLOCKED_RECT d3dLockedRect;
	const HRESULT hr = GetGPUTexture2D()->LockRect(level, &d3dLockedRect, NULL, flags);
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to lock the rect of the d3d texture 2d");

	C_LockedRect lockedRect;
	if(SUCCEEDED(hr))
	{
		lockedRect.pBits = (U8*)d3dLockedRect.pBits;
		lockedRect.pitch = d3dLockedRect.Pitch;
	}
	return lockedRect;
}

C_ConstLockedRect C_Texture::LockRect2D(const U32 level, const U32 flags) const
{
	BIZ_ASSERT(IsCreated());
	BIZ_ASSERT(GetType() == E_TextureType_2D);
	
	D3DLOCKED_RECT d3dLockedRect;
	const HRESULT hr = ((GPUTexture2D*)GetGPUTexture2D())->LockRect(level, &d3dLockedRect, NULL, flags);
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to lock the rect of the d3d texture 2d");

	C_ConstLockedRect lockedRect;
	if(SUCCEEDED(hr))
	{
		lockedRect.pBits = (U8*)d3dLockedRect.pBits;
		lockedRect.pitch = d3dLockedRect.Pitch;
	}
	return lockedRect;
}

void C_Texture::UnlockRect2D(const U32 level) const
{
	BIZ_ASSERT(IsCreated());
	BIZ_ASSERT(GetType() == E_TextureType_2D);

	const HRESULT hr = ((GPUTexture2D*)GetGPUTexture2D())->UnlockRect(level);
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to unlock the rect of the d3d texture 2d");
}

C_LockedBox C_Texture::LockBox3D(const U32 level, const U32 flags)
{
	BIZ_ASSERT(IsCreated());
	BIZ_ASSERT(GetType() == E_TextureType_3D);
	
	D3DLOCKED_BOX d3dLockedBox;
	const HRESULT hr = GetGPUTexture3D()->LockBox(level, &d3dLockedBox, NULL, flags);
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to lock the box of the d3d texture 3d");

	C_LockedBox lockedBox;
	if(SUCCEEDED(hr))
	{
		lockedBox.pBits = (U8*)d3dLockedBox.pBits;
		lockedBox.rowPitch = d3dLockedBox.RowPitch;
		lockedBox.slicePitch = d3dLockedBox.SlicePitch;
	}
	return lockedBox;
}

C_ConstLockedBox C_Texture::LockBox3D(const U32 level, const U32 flags) const
{
	BIZ_ASSERT(IsCreated());
	BIZ_ASSERT(GetType() == E_TextureType_3D);
	
	D3DLOCKED_BOX d3dLockedBox;
	const HRESULT hr = ((GPUTexture3D*)GetGPUTexture3D())->LockBox(level, &d3dLockedBox, NULL, flags);
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to lock the box of the d3d texture 3d");

	C_ConstLockedBox lockedBox;
	if(SUCCEEDED(hr))
	{
		lockedBox.pBits = (U8*)d3dLockedBox.pBits;
		lockedBox.rowPitch = d3dLockedBox.RowPitch;
		lockedBox.slicePitch = d3dLockedBox.SlicePitch;
	}
	return lockedBox;
}

void C_Texture::UnlockBox3D(const U32 level) const
{
	BIZ_ASSERT(IsCreated());
	BIZ_ASSERT(GetType() == E_TextureType_3D);

	const HRESULT hr = ((GPUTexture3D*)GetGPUTexture3D())->UnlockBox(level);
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to unlock the box of the d3d texture 3d");
}

C_LockedRect C_Texture::LockRectCube(const U32 face, const U32 level, const U32 flags)
{
	BIZ_ASSERT(IsCreated());
	BIZ_ASSERT(GetType() == E_TextureType_Cube);
	
	D3DLOCKED_RECT d3dLockedRect;
	const HRESULT hr = GetGPUTextureCube()->LockRect((D3DCUBEMAP_FACES)face, level, &d3dLockedRect, NULL, flags);
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to lock the rect of the d3d texture cube");
	
	C_LockedRect lockedRect;
	if(SUCCEEDED(hr))
	{
		lockedRect.pBits = (U8*)d3dLockedRect.pBits;
		lockedRect.pitch = d3dLockedRect.Pitch;
	}
	return lockedRect;
}

C_ConstLockedRect C_Texture::LockRectCube(const U32 face, const U32 level, const U32 flags) const
{
	BIZ_ASSERT(IsCreated());
	BIZ_ASSERT(GetType() == E_TextureType_Cube);
	
	D3DLOCKED_RECT d3dLockedRect;
	const HRESULT hr = ((GPUTextureCube*)GetGPUTextureCube())->LockRect((D3DCUBEMAP_FACES)face, level, &d3dLockedRect, NULL, flags);
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to lock the rect of the d3d texture cube");
	
	C_ConstLockedRect lockedRect;
	if(SUCCEEDED(hr))
	{
		lockedRect.pBits = (U8*)d3dLockedRect.pBits;
		lockedRect.pitch = d3dLockedRect.Pitch;
	}
	return lockedRect;
}

void C_Texture::UnlockRectCube(const U32 face, const U32 level) const
{
	BIZ_ASSERT(IsCreated());
	BIZ_ASSERT(GetType() == E_TextureType_Cube);

	const HRESULT hr = ((GPUTextureCube*)GetGPUTexture())->UnlockRect((D3DCUBEMAP_FACES)face, level);
	BIZ_ASSERT_MSG(SUCCEEDED(hr), "Failed to unlock the rect of the d3d texture cube");
}

GPUTexture* C_Texture::Get()
{
#if DEVICE_XB2
	return IsCreated() ? &m_d3dTexture : NULL;
#else
	return m_pd3dTexture;
#endif

}
const Util::C_String C_Texture::GetFileName() const
{
	return "";
}

C_LockedRect C_Texture::Lock(const U32 level)
{
	BIZ_ASSERT(GetType() == E_ResourceType_Texture2D);

	return LockRect2D(level);
}

C_ConstLockedRect C_Texture::Lock(const U32 level) const
{
	BIZ_ASSERT(GetType() == E_ResourceType_Texture2D);

	return LockRect2D(level);
}

void C_Texture::Unlock(const U32 level) const
{
	BIZ_ASSERT(GetType() == E_ResourceType_Texture2D);
	
	UnlockRect2D(level);
}

// C_RenderTexture2D

C_RenderTexture2D::C_RenderTexture2D() : m_Flags(0)
{
#if DEVICE_D3D9_PC
	m_TextureIndex = 0;
	m_SurfaceIndex = 0;
	m_Multisample = D3DMULTISAMPLE_NONE;	// [Blur2PC]
#endif
}

C_RenderTexture2D::~C_RenderTexture2D()
{
}

// C_RenderTextureCube

C_RenderTextureCube::C_RenderTextureCube() : m_Flags(0)
{
}

C_RenderTextureCube::~C_RenderTextureCube()
{
}

// C_ContextBase

C_ContextBase::C_ContextBase() :
	m_pd3dDevice(NULL)
{
}

C_ContextBase::C_ContextBase(GPUDevice* pd3dDevice) :
	m_pd3dDevice(pd3dDevice)
{
}

// C_Context

C_Context::C_Context() :
	m_pState(NULL)
{
}

C_Context::C_Context(GPUDevice* pd3dDevice) :
	C_ContextBase(pd3dDevice),
	m_pState(NULL)
{
}

// C_ContextNoCheck

C_ContextNoCheck::C_ContextNoCheck() :
	m_pState(NULL)
{
}

C_ContextNoCheck::C_ContextNoCheck(GPUDevice* pd3dDevice) :
	C_ContextBase(pd3dDevice),
	m_pState(NULL)
{
}

// C_ContextNoCheckNoTrack

C_ContextNoCheckNoTrack::C_ContextNoCheckNoTrack()
{
}

C_ContextNoCheckNoTrack::C_ContextNoCheckNoTrack(GPUDevice* pd3dDevice) :
	C_ContextBase(pd3dDevice)
{
}

} // namespace Display2

#if DONT_INLINE_DISPLAY2
#undef INLINE
#define INLINE inline
#endif
