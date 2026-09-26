#include "stdafx.h"
#include "Display/Device/D3D9/D3D9.h"

#ifdef CFG_ALLOW_OPTIMIZE_OFF
//#pragma optimize( "", off ) // CFG_ALLOW_OPTIMIZE_OFF
#endif

namespace Display
{
namespace D3D9
{

U32 g_MainThreadID = UINT_MAX;

IDirect3DDevice9* g_pd3dDevice = NULL;

const Util::C_String GetTextureFormatAsString(D3DFORMAT d3dFmt)
{
	switch (d3dFmt)
	{
	case D3DFMT_A8:
		return "A8";
	case D3DFMT_L8:
		return "L8";

	case D3DFMT_R5G6B5:
		return "R5G6B5";
	case D3DFMT_A4R4G4B4:
		return "A4R4G4B4";

#ifdef PLATFORM_PC
	case D3DFMT_R8G8B8:
		return "R8G8B8";
#endif

	case D3DFMT_A8R8G8B8:
		return "A8R8G8B8";
	case D3DFMT_DXT1:
		return "DXT1";
	case D3DFMT_DXT3:
		return "DXT3";
	case D3DFMT_DXT5:
		return "DXT5";

	default:
		return Util::C_String::Format("Unknown 0x%x", d3dFmt);
	}
}

const Util::C_String GetErrorFromHRESULT(HRESULT hr, bool includeDescription)
{
#ifdef PLATFORM_PC
	Util::C_String errorString = DXGetErrorString(hr);

	if (includeDescription)
	{
		errorString += Util::C_String::Format(" - %s", DXGetErrorDescription(hr));
	}

	return errorString;
#else
	return Util::C_String::Format("0x%x", hr);
#endif
}

const E_Format GetFormatFromD3D(const D3DFORMAT d3dFormat)
{
	switch(d3dFormat)
	{
#ifdef PLATFORM_XENON
	// 8bit
	case D3DFMT_L8:						return E_Format_L8;
	case D3DFMT_A8:						return E_Format_A8;
	case D3DFMT_LIN_L8:					return E_Format_L8;
	case D3DFMT_LIN_A8:					return E_Format_A8;
	// 16bit
	case D3DFMT_LIN_A8L8:				return E_Format_A8L8;
	case D3DFMT_LIN_L16:				return E_Format_L16;
	// 32bit
	case D3DFMT_A8R8G8B8:				return E_Format_A8R8G8B8;
	case D3DFMT_X8R8G8B8:				return E_Format_X8R8G8B8;
	case D3DFMT_LIN_A8R8G8B8:			return E_Format_A8R8G8B8;
	case D3DFMT_LIN_X8R8G8B8:			return E_Format_X8R8G8B8;
	case D3DFMT_R32F:					return E_Format_R32F;
	case D3DFMT_A2B10G10R10F_EDRAM:		return E_Format_A2B10G10R10F;
	// 64bit
	case D3DFMT_A16B16G16R16F:			return E_Format_A16B16G16R16F;
	case D3DFMT_A16B16G16R16F_EXPAND:	return E_Format_A16B16G16R16F;
	// DXT
	case D3DFMT_DXT1:					return E_Format_DXT1;
	case D3DFMT_DXT3:					return E_Format_DXT3;
	case D3DFMT_DXT5:					return E_Format_DXT5;
	case D3DFMT_DXN:					return E_Format_DXN;
	case D3DFMT_LIN_DXT1:				return E_Format_DXT1;
	case D3DFMT_LIN_DXT3:				return E_Format_DXT3;
	case D3DFMT_LIN_DXT5:				return E_Format_DXT5;
	case D3DFMT_LIN_DXN:				return E_Format_DXN;
#else
	// 8bit
	case D3DFMT_L8:						return E_Format_L8;
	case D3DFMT_A8:						return E_Format_A8;
	// 32bit
	case D3DFMT_A8R8G8B8:				return E_Format_A8R8G8B8;
	case D3DFMT_X8R8G8B8:				return E_Format_X8R8G8B8;
	case D3DFMT_R32F:					return E_Format_R32F;
	// 64bit	
	case D3DFMT_A16B16G16R16F:			return E_Format_A16B16G16R16F;
	// DXT
	case D3DFMT_DXT1:					return E_Format_DXT1;
	case D3DFMT_DXT3:					return E_Format_DXT3;
	case D3DFMT_DXT5:					return E_Format_DXT5;
#endif
	default:
		//BIZ_ASSERT_FAIL("Unsupported d3d format");
		return E_Format_Unknown;
	}
}

}; // namespace D3D9

#if PLATFORM_XENON
void FlushCacheForGPU(void* __restrict ptr, int dataSize)
{
	for(int offset=0; offset < dataSize; offset+=128)
		__dcbf(offset, ptr);
}
#endif

}; // namespace Display
