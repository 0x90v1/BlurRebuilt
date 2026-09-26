#ifndef DISPLAY_DEVICE_D3D9_D3D9_DEVICE_CAPS_INL
#define DISPLAY_DEVICE_D3D9_D3D9_DEVICE_CAPS_INL

#ifdef _MSC_VER
#pragma once
#endif

namespace Display
{
namespace D3D9
{

INLINE const bool C_DeviceCaps::Blend(const E_Blend blend)
{
	switch(blend)
	{
	case E_Blend_Zero:
		return (m_d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_ZERO) && (m_d3dCaps.DestBlendCaps & D3DPBLENDCAPS_ZERO) ? true : false;
	case E_Blend_One:
		return (m_d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_ONE) && (m_d3dCaps.DestBlendCaps & D3DPBLENDCAPS_ONE) ? true : false;
	case E_Blend_SrcColour:
		return (m_d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_SRCCOLOR) && (m_d3dCaps.DestBlendCaps & D3DPBLENDCAPS_SRCCOLOR) ? true : false;
	case E_Blend_InvSrcColour:
		return (m_d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_INVSRCCOLOR) && (m_d3dCaps.DestBlendCaps & D3DPBLENDCAPS_INVSRCCOLOR) ? true : false;
	case E_Blend_SrcAlpha:
		return (m_d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_SRCALPHA) && (m_d3dCaps.DestBlendCaps & D3DPBLENDCAPS_SRCALPHA) ? true : false;
	case E_Blend_InvSrcAlpha:
		return (m_d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_INVSRCALPHA) && (m_d3dCaps.DestBlendCaps & D3DPBLENDCAPS_INVSRCALPHA) ? true : false;
	case E_Blend_DstAlpha:
		return (m_d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_DESTALPHA) && (m_d3dCaps.DestBlendCaps & D3DPBLENDCAPS_DESTALPHA) ? true : false;
	case E_Blend_InvDstAlpha:
		return (m_d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_INVDESTALPHA) && (m_d3dCaps.DestBlendCaps & D3DPBLENDCAPS_INVDESTALPHA) ? true : false;
	case E_Blend_DstColour:
		return (m_d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_DESTCOLOR) && (m_d3dCaps.DestBlendCaps & D3DPBLENDCAPS_DESTCOLOR) ? true : false;
	case E_Blend_InvDstColour:
		return (m_d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_INVDESTCOLOR) && (m_d3dCaps.DestBlendCaps & D3DPBLENDCAPS_INVDESTCOLOR) ? true : false;
	default:
		return true;
	}
}

INLINE const bool C_DeviceCaps::CmpFunc(const E_CmpFunc cmpFunc)
{
	switch(cmpFunc)
	{
	case E_CmpFunc_Never:
		return m_d3dCaps.ZCmpCaps & D3DPCMPCAPS_NEVER ? true : false;
	case E_CmpFunc_Less:
		return m_d3dCaps.ZCmpCaps & D3DPCMPCAPS_LESS ? true : false;
	case E_CmpFunc_Equal:
		return m_d3dCaps.ZCmpCaps & D3DPCMPCAPS_EQUAL ? true : false;
	case E_CmpFunc_LessEqual:
		return m_d3dCaps.ZCmpCaps & D3DPCMPCAPS_LESSEQUAL ? true : false;
	case E_CmpFunc_Greater:
		return m_d3dCaps.ZCmpCaps & D3DPCMPCAPS_GREATER ? true : false;
	case E_CmpFunc_NotEqual:
		return m_d3dCaps.ZCmpCaps & D3DPCMPCAPS_NOTEQUAL ? true : false;
	case E_CmpFunc_GreaterEqual:
		return m_d3dCaps.ZCmpCaps & D3DPCMPCAPS_GREATEREQUAL ? true : false;
	case E_CmpFunc_Always:
		return m_d3dCaps.ZCmpCaps & D3DPCMPCAPS_ALWAYS ? true : false;
	default:
		return true;
	}
}

INLINE const bool C_DeviceCaps::MipMapLodBias()
{
	return m_d3dCaps.RasterCaps & D3DPRASTERCAPS_MIPMAPLODBIAS ? true : false;
}

INLINE const bool C_DeviceCaps::OcclusionQueries()
{
	return m_d3dCaps.RasterCaps & D3DPRASTERCAPS_ZTEST ? true : false;
}

INLINE const bool C_DeviceCaps::VertexElementType(const E_VertexElementType vertexElementType)
{
	switch(vertexElementType)
	{
	case E_VertexElementType_U8_4:
		return m_d3dCaps.DeclTypes & D3DDTCAPS_UBYTE4 ? true : false;
	case E_VertexElementType_U8_4N:
		return m_d3dCaps.DeclTypes & D3DDTCAPS_UBYTE4N ? true : false;
	case E_VertexElementType_F16_2:
		return m_d3dCaps.DeclTypes & D3DDTCAPS_FLOAT16_2 ? true : false;
	case E_VertexElementType_F16_4:
		return m_d3dCaps.DeclTypes & D3DDTCAPS_FLOAT16_4 ? true : false;
	default:
		return true;
	}
}

INLINE const DWORD C_DeviceCaps::VertexProcessing()
{
#ifdef PLATFORM_XENON
	return 0;
#else
	if(m_d3dCaps.VertexProcessingCaps &  D3DVTXPCAPS_LOCALVIEWER)
	if(m_d3dCaps.VertexProcessingCaps &  D3DVTXPCAPS_MATERIALSOURCE7)
	if(m_d3dCaps.VertexProcessingCaps &~ D3DVTXPCAPS_NO_TEXGEN_NONLOCALVIEWER)
	if(m_d3dCaps.VertexProcessingCaps &  D3DVTXPCAPS_POSITIONALLIGHTS)
	if(m_d3dCaps.VertexProcessingCaps &  D3DVTXPCAPS_TEXGEN)
	if(m_d3dCaps.VertexProcessingCaps &  D3DVTXPCAPS_TEXGEN_SPHEREMAP)
	return D3DCREATE_HARDWARE_VERTEXPROCESSING;
	return D3DCREATE_SOFTWARE_VERTEXPROCESSING;
#endif
}

INLINE const bool C_DeviceCaps::NonPowerOfTwoTextures()
{
#ifdef PLATFORM_PC
	if(	(m_d3dCaps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) ||
		(m_d3dCaps.TextureCaps & D3DPTEXTURECAPS_POW2))
	{
		return false;
	}
	else
	{
		return true;
	}
#else
	return true;
#endif
}

INLINE const bool C_DeviceCaps::StretchRect(const E_TextureFilter textureFilter)
{
#ifdef PLATFORM_PC
	switch(textureFilter)
	{
	case E_TextureFilter_Point:
		if(	(m_d3dCaps.DevCaps2 & D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES) &&
			(m_d3dCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MINFLINEAR) && 
			(m_d3dCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MINFLINEAR))
		{
			return true;
		}
		else
		{
			return false;
		}
	case E_TextureFilter_Linear:
		if(	(m_d3dCaps.DevCaps2 & D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES) &&
			(m_d3dCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MINFLINEAR) && 
			(m_d3dCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR))
		{
			return true;
		}
		else
		{
			return false;
		}
	default:
		return false;
	}
#else
	return true;
#endif
}

}; // namespace D3D9
}; // namespace Display

#endif // DISPLAY_DEVICE_D3D9_D3D9_DEVICE_CAPS_INL
