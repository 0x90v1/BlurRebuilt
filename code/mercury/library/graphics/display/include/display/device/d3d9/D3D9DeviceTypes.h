#ifndef DISPLAY_DEVICE_D3D9_D3D9_DEVICETYPES_H
#define DISPLAY_DEVICE_D3D9_D3D9_DEVICETYPES_H

#ifdef _MSC_VER
#pragma once
#endif

#if PLATFORM_XENON
#include <xgraphics.h>
#endif

#include <core/maths/mathsutil.h>

#include "Display/Device/DeviceTypes.h"
#include "Display/Device/D3D9/D3D9DeviceCaps.h"

namespace Display
{
namespace D3D9
{

INLINE BOOL D3D9Bool(const E_Bool boolean)
{
	return boolean == E_Bool_True ? TRUE : FALSE;
}

INLINE E_Bool D3D9Bool(const BOOL boolean)
{
	return boolean == TRUE ? E_Bool_True : E_Bool_False;
}

INLINE D3DBLEND D3D9Blend(const E_Blend blend)
{
	#ifdef PLATFORM_PC
	assert(C_DeviceCaps::Blend(blend));
	#endif
	switch(blend)
	{
	case E_Blend_Zero:
		return D3DBLEND_ZERO;
	case E_Blend_One:
		return D3DBLEND_ONE;
	case E_Blend_SrcColour:
		return D3DBLEND_SRCCOLOR;
	case E_Blend_InvSrcColour:
		return D3DBLEND_INVSRCCOLOR;
	case E_Blend_SrcAlpha:
		return D3DBLEND_SRCALPHA;
	case E_Blend_InvSrcAlpha:
		return D3DBLEND_INVSRCALPHA;
	case E_Blend_DstAlpha:
		return D3DBLEND_DESTALPHA;
	case E_Blend_InvDstAlpha:
		return D3DBLEND_INVDESTALPHA;
	case E_Blend_DstColour:
		return D3DBLEND_DESTCOLOR;
	case E_Blend_InvDstColour:
		return D3DBLEND_INVDESTCOLOR;
	default:
		BIZ_ASSERT_FAIL("Unsupported blend");
		return D3DBLEND_FORCE_DWORD;
	}
}

INLINE E_Blend D3D9Blend(const D3DBLEND blend)
{
	switch(blend)
	{
	case D3DBLEND_ZERO:
		return E_Blend_Zero;
	case D3DBLEND_ONE:
		return E_Blend_One;
	case D3DBLEND_SRCCOLOR:
		return E_Blend_SrcColour;
	case D3DBLEND_INVSRCCOLOR:
		return E_Blend_InvSrcColour;
	case D3DBLEND_SRCALPHA:
		return E_Blend_SrcAlpha;
	case D3DBLEND_INVSRCALPHA:
		return E_Blend_InvSrcAlpha;
	case D3DBLEND_DESTALPHA:
		return E_Blend_DstAlpha;
	case D3DBLEND_INVDESTALPHA:
		return E_Blend_InvDstAlpha;
	case D3DBLEND_DESTCOLOR:
		return E_Blend_DstColour;
	case D3DBLEND_INVDESTCOLOR:
		return E_Blend_InvDstColour;
	default:
		BIZ_ASSERT_FAIL("Unsupported blend");
		return E_Blend_Unknown;
	}
}

INLINE D3DBLENDOP D3D9BlendOp(const E_BlendOp blendOp)
{
	switch(blendOp)
	{
	case E_BlendOp_Add:
		return D3DBLENDOP_ADD;
	case E_BlendOp_Subtract:
		return D3DBLENDOP_SUBTRACT;
	case E_BlendOp_RevSubtract:
		return D3DBLENDOP_REVSUBTRACT;
	case E_BlendOp_Min:
		return D3DBLENDOP_MIN;
	case E_BlendOp_Max:
		return D3DBLENDOP_MAX;
	default:
		BIZ_ASSERT_FAIL("Unsupported blend op");
		return D3DBLENDOP_FORCE_DWORD;
	}
}

INLINE E_BlendOp D3D9BlendOp(const D3DBLENDOP blendOp)
{
	switch(blendOp)
	{
	case D3DBLENDOP_ADD:
		return E_BlendOp_Add;	
	case D3DBLENDOP_SUBTRACT:
		return E_BlendOp_Subtract;	
	case D3DBLENDOP_REVSUBTRACT:
		return E_BlendOp_RevSubtract;
	case D3DBLENDOP_MIN:
		return E_BlendOp_Min;	
	case D3DBLENDOP_MAX:
		return E_BlendOp_Max;
	default:
		BIZ_ASSERT_FAIL("Unsupported blend op");
		return E_BlendOp_Unknown;
	}
}

INLINE D3DCMPFUNC D3D9CmpFunc(const E_CmpFunc cmpFunc)
{
	#ifdef PLATFORM_PC
	assert(C_DeviceCaps::CmpFunc(cmpFunc));
	#endif
	switch(cmpFunc)
	{
	case E_CmpFunc_Never:
		return D3DCMP_NEVER;	
	case E_CmpFunc_Less:
		return D3DCMP_LESS;	
	case E_CmpFunc_Equal:
		return D3DCMP_EQUAL;	
	case E_CmpFunc_LessEqual:
		return D3DCMP_LESSEQUAL;	
	case E_CmpFunc_Greater:
		return D3DCMP_GREATER;
	case E_CmpFunc_NotEqual:
		return D3DCMP_NOTEQUAL;	
	case E_CmpFunc_GreaterEqual:
		return D3DCMP_GREATEREQUAL;	
	case E_CmpFunc_Always:
		return D3DCMP_ALWAYS;	
	default:
		BIZ_ASSERT_FAIL("Unsupported cmp func");
		return D3DCMP_FORCE_DWORD;
	}
}

INLINE E_CmpFunc D3D9CmpFunc(const D3DCMPFUNC cmpFunc)
{
	switch(cmpFunc)
	{
	case D3DCMP_NEVER:
		return E_CmpFunc_Never;
	case D3DCMP_LESS:
		return E_CmpFunc_Less;	
	case D3DCMP_EQUAL:
		return E_CmpFunc_Equal;	
	case D3DCMP_LESSEQUAL:
		return E_CmpFunc_LessEqual;	
	case D3DCMP_GREATER:
		return E_CmpFunc_Greater;
	case D3DCMP_NOTEQUAL:
		return E_CmpFunc_NotEqual;
	case D3DCMP_GREATEREQUAL:
		return E_CmpFunc_GreaterEqual;
	case D3DCMP_ALWAYS:
		return E_CmpFunc_Always;
	default:
		BIZ_ASSERT_FAIL("Unsupported cmp func");
		return E_CmpFunc_Unknown;
	}
}

INLINE D3DCUBEMAP_FACES D3D9CubeTextureFace(const E_CubeTextureFace face)
{
	switch(face)
	{
	case E_CubeTextureFace_PosX:
		return D3DCUBEMAP_FACE_POSITIVE_X;	
	case E_CubeTextureFace_PosY:
		return D3DCUBEMAP_FACE_POSITIVE_Y;
	case E_CubeTextureFace_PosZ:
		return D3DCUBEMAP_FACE_POSITIVE_Z;
	case E_CubeTextureFace_NegX:
		return D3DCUBEMAP_FACE_NEGATIVE_X;
	case E_CubeTextureFace_NegY:
		return D3DCUBEMAP_FACE_NEGATIVE_Y;
	case E_CubeTextureFace_NegZ:
		return D3DCUBEMAP_FACE_NEGATIVE_Z;	
	default:
		BIZ_ASSERT_FAIL("Unsupported cube texture face");
		return D3DCUBEMAP_FACE_FORCE_DWORD;
	}
}

INLINE D3DCULL D3D9CullMode(const E_CullMode cullMode)
{
	switch(cullMode)
	{
	case E_CullMode_None:
		return D3DCULL_NONE;
	case E_CullMode_Cw:
		return D3DCULL_CW;
	case E_CullMode_Ccw:
		return D3DCULL_CCW;
	default:
		BIZ_ASSERT_FAIL("Unsupported cull mode");
		return D3DCULL_FORCE_DWORD;
	}
}

INLINE E_CullMode D3D9CullMode(const D3DCULL cullMode)
{
	switch(cullMode)
	{
	case D3DCULL_NONE:
		return E_CullMode_None;	
	case D3DCULL_CW:
		return E_CullMode_Cw;	
	case D3DCULL_CCW:
		return E_CullMode_Ccw;	
	default:
		BIZ_ASSERT_FAIL("Unsupported cull mode");
		return E_CullMode_Unknown;
	}
}

INLINE D3DFILLMODE D3D9FillMode(const E_FillMode fillMode)
{
	switch(fillMode)
	{
	case E_FillMode_Point:
		return D3DFILL_POINT;
	case E_FillMode_Wireframe:
		return D3DFILL_WIREFRAME;
	case E_FillMode_Solid:
		return D3DFILL_SOLID;
	default:
		BIZ_ASSERT_FAIL("Unsupported fill mode");
		return D3DFILL_FORCE_DWORD;
	}
}

INLINE D3DFORMAT D3D9Format(const E_Format format)
{
	switch(format)
	{
	case E_Format_DXT1:
		return D3DFMT_DXT1;
	case E_Format_DXT3:
		return D3DFMT_DXT3;
	case E_Format_DXT5:
		return D3DFMT_DXT5;
#ifdef PLATFORM_XENON
	case E_Format_DXN:
		return D3DFMT_DXN;
#endif
	case E_Format_L8:
		return D3DFMT_L8;
	case E_Format_A8:
		return D3DFMT_A8;
	case E_Format_A8L8:
		return D3DFMT_A8L8;
	case E_Format_L16:
		return D3DFMT_L16;
	case E_Format_A8R8G8B8:
		return D3DFMT_A8R8G8B8;
	case E_Format_X8R8G8B8:
		return D3DFMT_X8R8G8B8;
	case E_Format_G16R16:
		return D3DFMT_G16R16;
	case E_Format_G16R16F:
		return D3DFMT_G16R16F;
	case E_Format_A16B16G16R16:
		return D3DFMT_A16B16G16R16;
	case E_Format_A16B16G16R16F:
		return D3DFMT_A16B16G16R16F;
	case E_Format_R16F:
		return D3DFMT_R16F;
	case E_Format_R32F:
		return D3DFMT_R32F;
	case E_Format_A32B32G32R32F:
		return D3DFMT_A32B32G32R32F;
	case E_Format_A2R10G10B10:
		return D3DFMT_A2R10G10B10;
#ifdef PLATFORM_XENON
	case E_Format_A2B10G10R10F:
		return D3DFMT_A2B10G10R10F_EDRAM;
#endif
	case E_Format_D16:
		return D3DFMT_D16;
	case E_Format_D24S8:
		return D3DFMT_D24S8;
	case E_Format_D24FS8:
		return D3DFMT_D24FS8;
	case E_Format_Index16:
		return D3DFMT_INDEX16;
	case E_Format_Index32:
		return D3DFMT_INDEX32;
	default:
		BIZ_ASSERT_FAIL("Unsupported format");
		return D3DFMT_FORCE_DWORD;
	}
}

INLINE D3DFORMAT D3D9Format(const E_Format format, const U32 flags)
{
#ifdef PLATFORM_XENON
	switch(format)
	{
	case E_Format_DXT1:
		return flags & E_Format_Linear ? D3DFMT_LIN_DXT1 : D3DFMT_DXT1;
	case E_Format_DXT3:
		return flags & E_Format_Linear ? D3DFMT_LIN_DXT3 : D3DFMT_DXT3;
	case E_Format_DXT5:
		return flags & E_Format_Linear ? D3DFMT_LIN_DXT5 : D3DFMT_DXT5;
	case E_Format_DXN:
		return flags & E_Format_Linear ? D3DFMT_LIN_DXN  : D3DFMT_DXN;
	case E_Format_L16:
		return flags & E_Format_Linear ? D3DFMT_LIN_L16 : D3DFMT_L16;
	case E_Format_A8:
		return flags & E_Format_Linear ? D3DFMT_LIN_A8 : D3DFMT_A8;
	case E_Format_L8:
		return flags & E_Format_Linear ? D3DFMT_LIN_L8 : D3DFMT_L8;
	case E_Format_A8R8G8B8:
		return flags & E_Format_Linear ? D3DFMT_LIN_A8R8G8B8 : D3DFMT_A8R8G8B8;
	case E_Format_X8R8G8B8:
		return flags & E_Format_Linear ? D3DFMT_LIN_X8R8G8B8 : D3DFMT_X8R8G8B8;
	case E_Format_G16R16:
		return flags & E_Format_Linear ? D3DFMT_LIN_G16R16 : D3DFMT_G16R16;
	case E_Format_G16R16F:
		return flags & E_Format_Linear ? D3DFMT_LIN_G16R16F : D3DFMT_G16R16F;
	case E_Format_A16B16G16R16:
		return flags & E_Format_Linear ? D3DFMT_LIN_A16B16G16R16 : D3DFMT_A16B16G16R16;
	case E_Format_A16B16G16R16F:
		return flags & E_Format_Linear ? D3DFMT_LIN_A16B16G16R16F : D3DFMT_A16B16G16R16F;
	case E_Format_A2B10G10R10F:
		return D3DFMT_A2B10G10R10F_EDRAM;
	case E_Format_R32F:
		return flags & E_Format_Linear ? D3DFMT_LIN_R32F : D3DFMT_R32F;
	case E_Format_A32B32G32R32F:
		return flags & E_Format_Linear ? D3DFMT_LIN_A32B32G32R32F : D3DFMT_A32B32G32R32F;
	case E_Format_D16:
		return flags & E_Format_Linear ? D3DFMT_LIN_D16 : D3DFMT_D16;
	case E_Format_D24S8:
		return flags & E_Format_Linear ? D3DFMT_LIN_D24S8 : D3DFMT_D24S8;
	case E_Format_D24FS8:
		return flags & E_Format_Linear ? D3DFMT_LIN_D24FS8 : D3DFMT_D24FS8;
	case E_Format_Index16:
		return D3DFMT_INDEX16;
	case E_Format_Index32:
		return D3DFMT_INDEX32;
	default:
		BIZ_ASSERT_FAIL("Unsupported format");
		return D3DFMT_FORCE_DWORD;
	}
#else
	return D3D9Format(format);
#endif
}

INLINE D3DFORMAT D3D9RenderTextureFormat(const D3DFORMAT d3dFormat, const U32 flags)
{
#ifdef PLATFORM_XENON
	if(d3dFormat == D3DFMT_A2B10G10R10F_EDRAM)
		return D3DFMT_A16B16G16R16F;
	else
	if(d3dFormat == D3DFMT_A16B16G16R16F && (flags & E_Usage_Expand))
		return D3DFMT_A16B16G16R16F_EXPAND;
	else
	if(d3dFormat == D3DFMT_A16B16G16R16F && (flags & E_Usage_Uint))
		return D3DFMT_A16B16G16R16_UINT;
	else
		return d3dFormat;
#endif
}

INLINE D3DMULTISAMPLE_TYPE D3D9Multisample(const E_Multisample multisample)
{
	switch(multisample)
	{
	case E_Multisample_None:
		return D3DMULTISAMPLE_NONE;
	case E_Multisample_2:
		return D3DMULTISAMPLE_2_SAMPLES;
	case E_Multisample_4:
		return D3DMULTISAMPLE_4_SAMPLES;
	#ifndef PLATFORM_XENON
	case E_Multisample_8:
		return D3DMULTISAMPLE_8_SAMPLES;
	case E_Multisample_16:
		return D3DMULTISAMPLE_16_SAMPLES;
	#endif
	default:
		BIZ_ASSERT_FAIL("Unsupported multisample");
		return D3DMULTISAMPLE_FORCE_DWORD ;
	}
}

INLINE UINT D3D9PresentInterval(const E_PresentInterval presentInterval)
{
	switch(presentInterval)
	{
	case E_PresentInterval_Immediate:
		return D3DPRESENT_INTERVAL_IMMEDIATE;
	case E_PresentInterval_One:
		return D3DPRESENT_INTERVAL_ONE;
	case E_PresentInterval_Two:
		return D3DPRESENT_INTERVAL_TWO;
	case E_PresentInterval_Three:
		return D3DPRESENT_INTERVAL_THREE;
	case E_PresentInterval_Default:
		return D3DPRESENT_INTERVAL_DEFAULT;
	default:
		BIZ_ASSERT_FAIL("Unsupported present interval");
		return D3DPRESENT_INTERVAL_DEFAULT;
	}
}

INLINE D3DPRIMITIVETYPE D3D9PrimitiveType(const E_PrimitiveType primitiveType)
{
	switch(primitiveType)
	{
	case E_PrimitiveType_PointList:
		return D3DPT_POINTLIST;
	case E_PrimitiveType_LineList:
		return D3DPT_LINELIST;
	case E_PrimitiveType_LineStrip:
		return D3DPT_LINESTRIP;
	case E_PrimitiveType_TriangleList:
		return D3DPT_TRIANGLELIST;
	case E_PrimitiveType_TriangleStrip:
		return D3DPT_TRIANGLESTRIP;
	case E_PrimitiveType_TriangleFan:
		return D3DPT_TRIANGLEFAN;
	case E_PrimitiveType_QuadList:
	#ifdef PLATFORM_XENON
		return D3DPT_QUADLIST;
	#else
		return D3D9PrimitiveType(E_PrimitiveType_PointList); // turn quad list into points
	#endif
#ifdef DEVICE_XB2D3D9
	case E_PrimitiveType_RectList:
		return D3DPT_RECTLIST;
#endif
	default:
		BIZ_ASSERT_FAIL("Unsupported primitive type");
		return D3DPT_FORCE_DWORD;
	}
}

INLINE DWORD D3D9PrimitiveCount(const E_PrimitiveType primitiveType, const U32 indexCount)
{
	switch(primitiveType)
	{
	case E_PrimitiveType_PointList:
		return indexCount;
	case E_PrimitiveType_LineList:
		return indexCount / 2;
	case E_PrimitiveType_LineStrip:
		return indexCount / 2;
	case E_PrimitiveType_TriangleList:
		return indexCount / 3;
	case E_PrimitiveType_TriangleStrip:
		return indexCount - 2;
	case E_PrimitiveType_TriangleFan:
		return indexCount - 2;
	default:
		BIZ_ASSERT_FAIL("Unsupported primitive type");
		return 0;
	}
}

INLINE D3DPOOL D3D9Pool(const U32 flags)
{
#ifdef PLATFORM_XENON
	return 0;
#else
	if(flags & E_Usage_Video)
		return D3DPOOL_DEFAULT;
	else
	if(flags & E_Usage_System)
		return D3DPOOL_SYSTEMMEM;
	else
		return D3DPOOL_MANAGED;
#endif
}

INLINE DWORD D3D9Resolve(const E_Resolve resolve)
{
#ifdef PLATFORM_XENON
	switch(resolve)
	{
	case E_Resolve_Colour:
		return D3DRESOLVE_RENDERTARGET0;
	case E_Resolve_Colour_1:
		return D3DRESOLVE_RENDERTARGET1;
	case E_Resolve_Colour_2:
		return D3DRESOLVE_RENDERTARGET2;
	case E_Resolve_Colour_3:
		return D3DRESOLVE_RENDERTARGET3;
	case E_Resolve_DepthStencil:
		return D3DRESOLVE_DEPTHSTENCIL;
	default:
		return 0;
	}
#else
	return 0;
#endif
}

INLINE D3DSTENCILOP D3D9StencilOp(const E_StencilOp stencilOp)
{
	switch(stencilOp)
	{
	case E_StencilOp_Keep:
		return D3DSTENCILOP_KEEP;
	case E_StencilOp_Zero:
		return D3DSTENCILOP_ZERO;
	case E_StencilOp_Replace:
		return D3DSTENCILOP_REPLACE;
	case E_StencilOp_IncrSat:
		return D3DSTENCILOP_INCRSAT;
	case E_StencilOp_DecrSat:
		return D3DSTENCILOP_DECRSAT;
	case E_StencilOp_Invert:
		return D3DSTENCILOP_INVERT;
	case E_StencilOp_Incr:
		return D3DSTENCILOP_INCR;
	case E_StencilOp_Decr:
		return D3DSTENCILOP_DECR;
	default:
		BIZ_ASSERT_FAIL("Unsupported stencil op");
		return D3DSTENCILOP_FORCE_DWORD;
	}
}

INLINE D3DTEXTUREADDRESS D3D9TextureAddress(const E_TextureAddress textureAddress)
{
	switch(textureAddress)
	{
	case E_TextureAddress_Wrap:
		return D3DTADDRESS_WRAP;
	case E_TextureAddress_Clamp:
		return D3DTADDRESS_CLAMP;
	case E_TextureAddress_Mirror:
		return D3DTADDRESS_MIRROR;
	case E_TextureAddress_Border:
		return D3DTADDRESS_BORDER;
	default:
		BIZ_ASSERT_FAIL("Unsupported texture address");
		return D3DTADDRESS_FORCE_DWORD;
	}
}

INLINE D3DTEXTUREFILTERTYPE D3D9TextureFilter(const U32 textureFilter)
{
	switch(textureFilter)
	{
	case E_TextureFilter_None:
		return D3DTEXF_NONE;
	case E_TextureFilter_Point:
		return D3DTEXF_POINT;
	case E_TextureFilter_Linear:
		return D3DTEXF_LINEAR;
	case E_TextureFilter_Anisotropic:
		return D3DTEXF_ANISOTROPIC;
	default:
		BIZ_ASSERT_FAIL("Unsupported texture filter");
		return D3DTEXF_FORCE_DWORD;
	}
}

INLINE U32 D3D9Texture2DSize(const U32 w, const U32 h, const D3DFORMAT format)
{
#ifdef PLATFORM_XENON
	return XGSetTextureHeader(w, h, 1, 0, format, 0, 0, 0, 0, NULL, NULL, NULL);
#else
	switch(format)
	{
	case D3DFMT_DXT1:
		return Maths::Align(Maths::Max(1,((int)w+3)/4)*Maths::Max(1,((int)h+3)/4)*8, 4);
	case D3DFMT_DXT2:
	case D3DFMT_DXT3:
	case D3DFMT_DXT4:
	case D3DFMT_DXT5:
		return Maths::Align(Maths::Max(1,((int)w+3)/4)*Maths::Max(1,((int)h+3)/4)*16, 4);
	case D3DFMT_R3G3B2:
	case D3DFMT_A8:
	case D3DFMT_L8:
	case D3DFMT_P8:
	case D3DFMT_A4L4:
		return Maths::Align(w*h, 4);
	case D3DFMT_R5G6B5:
	case D3DFMT_X1R5G5B5:
	case D3DFMT_A1R5G5B5:
	case D3DFMT_A4R4G4B4:
	case D3DFMT_X4R4G4B4:
	case D3DFMT_A8R3G3B2:
	case D3DFMT_A8P8:
	case D3DFMT_A8L8:
	case D3DFMT_V8U8:
	case D3DFMT_L6V5U5:
	case D3DFMT_R8G8_B8G8:
	case D3DFMT_G8R8_G8B8:
	case D3DFMT_D16_LOCKABLE:
	case D3DFMT_D15S1:
	case D3DFMT_D16:
	case D3DFMT_L16:
	case D3DFMT_R16F:
	case D3DFMT_CxV8U8:
		return Maths::Align(2*w*h, 4);
	case D3DFMT_R8G8B8:
		return Maths::Align(3*w*h, 4);
	case D3DFMT_A8R8G8B8:
	case D3DFMT_X8R8G8B8:
	case D3DFMT_A8B8G8R8:
	case D3DFMT_X8B8G8R8:
	case D3DFMT_A2R10G10B10:
	case D3DFMT_A2B10G10R10:
	case D3DFMT_G16R16:
	case D3DFMT_X8L8V8U8:
	case D3DFMT_Q8W8V8U8:
	case D3DFMT_V16U16:
	case D3DFMT_A2W10V10U10:
	case D3DFMT_D32:
	case D3DFMT_D24S8:
	case D3DFMT_D24X8:
	case D3DFMT_D24X4S4:
	case D3DFMT_D32F_LOCKABLE:
	case D3DFMT_D24FS8:
	case D3DFMT_G16R16F:
	case D3DFMT_R32F:
		return 4*w*h;
	case D3DFMT_A16B16G16R16:
	case D3DFMT_Q16W16V16U16:
	case D3DFMT_A16B16G16R16F:
	case D3DFMT_G32R32F:
		return 8*w*h;
	case D3DFMT_A32B32G32R32F:
		return 16*w*h;
	case D3DFMT_MULTI2_ARGB8:
	case D3DFMT_VERTEXDATA:
	case D3DFMT_INDEX16:
	case D3DFMT_INDEX32:
	case D3DFMT_UYVY:
	case D3DFMT_YUY2:
	default:
		BIZ_ASSERT_FAIL("Unsupported texture format.");
		return 0;
	};
#endif
}

INLINE U32 D3D9Texture2DSize(U32 width, U32 height, const U32 levels, const D3DFORMAT format)
{
	U32 size = 0;
	for(U32 level = 0; level < levels; level++)
	{
		size += D3D9Texture2DSize(width, height, format);
		if(width > 1) width >>= 1;
		if(height > 1) height >>= 1;
	}
	return size;
}

INLINE U32 D3D9Texture3DSize(U32 width, U32 height, U32 depth, const U32 levels, const D3DFORMAT format)
{
	U32 size = 0;
	for(U32 level = 0; level < levels; level++)
	{
		size += D3D9Texture2DSize(width, height, format);
		if(width > 1) width >>= 1;
		if(height > 1) height >>= 1;
	}
	return size * depth;
}

INLINE U32 D3D9TextureCubeSize(U32 width, const U32 levels, const D3DFORMAT format)
{
	U32 size = 0;
	for(U32 level = 0; level < levels; level++)
	{
		size += D3D9Texture2DSize(width, width, format);
		if(width > 1) width >>= 1;
	}
	return size * 6;
}

INLINE DWORD D3D9TextureUsage(const U32 flags)
{
#ifdef PLATFORM_XENON
	return 0;
#else
	if(flags & E_Usage_Colour || flags & E_Usage_DepthStencil)
		return D3DUSAGE_RENDERTARGET;
	else
		return 0;
#endif
}

INLINE DWORD D3D9Usage(const U32 flags)
{
#ifdef PLATFORM_XENON
	return 0;
#else
	if(flags & E_Usage_Dynamic)
		return D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
	else
		return D3DUSAGE_WRITEONLY;
#endif
}

INLINE D3DDECLTYPE D3D9VertexElementType(const E_VertexElementType vertexElementType)
{
	switch(vertexElementType)
	{
	case E_VertexElementType_F32_1:
		return D3DDECLTYPE_FLOAT1;
	case E_VertexElementType_F32_2:
		return D3DDECLTYPE_FLOAT2;
	case E_VertexElementType_F32_3:
		return D3DDECLTYPE_FLOAT3;
	case E_VertexElementType_F32_4:
		return D3DDECLTYPE_FLOAT4;
	case E_VertexElementType_Colour:
		return D3DDECLTYPE_D3DCOLOR;	
	case E_VertexElementType_U8_4:
		#ifdef PLATFORM_PC
		assert(C_DeviceCaps::VertexElementType(vertexElementType));
		#endif
		return D3DDECLTYPE_UBYTE4;
	case E_VertexElementType_S16_2:
		return D3DDECLTYPE_SHORT2;
	case E_VertexElementType_S16_4:
		return D3DDECLTYPE_SHORT4;
#ifdef DEVICE_XB2D3D9
	case E_VertexElementType_U16_2:
		return D3DDECLTYPE_USHORT2;
	case E_VertexElementType_U16_4:
		return D3DDECLTYPE_USHORT4;
#endif
	case E_VertexElementType_S16_2N:
		return D3DDECLTYPE_SHORT2N;
	case E_VertexElementType_U8_4N:
		#ifdef PLATFORM_PC
		assert(C_DeviceCaps::VertexElementType(vertexElementType));
		#endif
		return D3DDECLTYPE_UBYTE4N;
	case E_VertexElementType_F16_2:
		#ifdef PLATFORM_PC
		assert(C_DeviceCaps::VertexElementType(vertexElementType));
		#endif
		return D3DDECLTYPE_FLOAT16_2;
	case E_VertexElementType_F16_4:
		#ifdef PLATFORM_PC
		assert(C_DeviceCaps::VertexElementType(vertexElementType));
		#endif
		return D3DDECLTYPE_FLOAT16_4;
#ifdef DEVICE_XB2D3D9
	case E_VertexElementType_U10_3N:
		return D3DDECLTYPE_UDEC3N;
	case E_VertexElementType_U10_4N:
		return D3DDECLTYPE_UDEC4N;
#elif defined (PLATFORM_PC_TODO) 
	case E_VertexElementType_U10_3N:
	case E_VertexElementType_U10_4N:
		return D3DDECLTYPE_UBYTE4N;
#endif
#ifdef DEVICE_XB2D3D9
	case E_VertexElementType_SHHD_3:
		return D3DDECLTYPE_HEND3;
	case E_VertexElementType_SHHD_3N:
		return D3DDECLTYPE_HEND3N;
	case E_VertexElementType_U32_1:
		return D3DDECLTYPE_UINT1;
#endif
	default:
		BIZ_ASSERT_FAIL("Unsupported vertex element type");
		return D3DDECLTYPE_UNUSED;
	}
}

INLINE D3DDECLUSAGE D3D9VertexElementUsage(const E_VertexElementUsage vertexElementUsage)
{
	switch(vertexElementUsage)
	{
	case E_VertexElementUsage_Position:
		return D3DDECLUSAGE_POSITION;
	case E_VertexElementUsage_BlendWeight:
		return D3DDECLUSAGE_BLENDWEIGHT;
	case E_VertexElementUsage_BlendIndices:
		return D3DDECLUSAGE_BLENDINDICES;
	case E_VertexElementUsage_Normal:
		return D3DDECLUSAGE_NORMAL;
	case E_VertexElementUsage_PSize:
		return D3DDECLUSAGE_PSIZE;
	case E_VertexElementUsage_TexCoord:
		return D3DDECLUSAGE_TEXCOORD;
	case E_VertexElementUsage_Tangent:
		return D3DDECLUSAGE_TANGENT;
	case E_VertexElementUsage_Binormal:
		return D3DDECLUSAGE_BINORMAL;
	case E_VertexElementUsage_Colour:
		return D3DDECLUSAGE_COLOR;
	case E_VertexElementUsage_Fog:
		return D3DDECLUSAGE_FOG;
	case E_VertexElementUsage_TessFactor:
		return D3DDECLUSAGE_TESSFACTOR;
	case E_VertexElementUsage_Depth:
		return D3DDECLUSAGE_DEPTH;
	case E_VertexElementUsage_Sample:
		return D3DDECLUSAGE_SAMPLE;
#ifdef DEVICE_PCD3D9
	case E_VertexElementUsage_PositionT:
		return D3DDECLUSAGE_POSITIONT;
#endif
	default:
		BIZ_ASSERT_FAIL("Unsupported vertex element usage");
		return D3DDECLUSAGE_POSITION;
	}
}

}; // namespace D3D9
}; // namespace Display

#endif // DISPLAY_DEVICE_D3D9_D3D9_DEVICETYPES_H
