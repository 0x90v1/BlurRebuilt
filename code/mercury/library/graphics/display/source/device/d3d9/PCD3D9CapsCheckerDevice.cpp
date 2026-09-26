#include "stdafx.h"
#include "Display/Device/D3D9/PCD3D9CapsChecker.h"

#ifdef CFG_ALLOW_OPTIMIZE_OFF
//#pragma optimize( , off ) // CFG_ALLOW_OPTIMIZE_OFF
#endif

//lint -e685 -e568 -e574 -e650 // stop lint warnings

namespace Display
{
namespace D3D9
{

bool D3DCheckDeviceCaps(const D3DCAPS9& d3dCaps)
{
	bool supported = true;

	if(d3dCaps.DeviceType < 1)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : DeviceType (%i) < 1\n", d3dCaps.DeviceType);
		supported = false;
	}
	//if(d3dCaps.AdapterOrdinal < 0)
	//{
		//LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : AdapterOrdinal (%i) < 0\n", d3dCaps.AdapterOrdinal);
		//supported = false;
	//}
	if(!(d3dCaps.Caps & D3DCAPS_READ_SCANLINE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  Caps & D3DCAPS_READ_SCANLINE\n");
		supported = false;
	}
	if(!(d3dCaps.Caps2 & D3DCAPS2_FULLSCREENGAMMA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  Caps2 & D3DCAPS2_FULLSCREENGAMMA\n");
		supported = false;
	}
	if(!(d3dCaps.Caps2 & D3DCAPS2_CANCALIBRATEGAMMA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  Caps2 & D3DCAPS2_CANCALIBRATEGAMMA\n");
		supported = false;
	}
	if(!(d3dCaps.Caps2 & D3DCAPS2_RESERVED))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  Caps2 & D3DCAPS2_RESERVED\n");
		supported = false;
	}
	if(!(d3dCaps.Caps2 & D3DCAPS2_CANMANAGERESOURCE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  Caps2 & D3DCAPS2_CANMANAGERESOURCE\n");
		supported = false;
	}
	if(!(d3dCaps.Caps2 & D3DCAPS2_DYNAMICTEXTURES))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  Caps2 & D3DCAPS2_DYNAMICTEXTURES\n");
		supported = false;
	}
	if(!(d3dCaps.Caps2 & D3DCAPS2_CANAUTOGENMIPMAP))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  Caps2 & D3DCAPS2_CANAUTOGENMIPMAP\n");
		supported = false;
	}
	if(!(d3dCaps.Caps2 & D3DCAPS2_CANSHARERESOURCE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  Caps2 & D3DCAPS2_CANSHARERESOURCE\n");
		supported = false;
	}
	if(!(d3dCaps.Caps3 & D3DCAPS3_RESERVED))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  Caps3 & D3DCAPS3_RESERVED\n");
		supported = false;
	}
	if(!(d3dCaps.Caps3 & D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  Caps3 & D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD\n");
		supported = false;
	}
	if(!(d3dCaps.Caps3 & D3DCAPS3_LINEAR_TO_SRGB_PRESENTATION))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  Caps3 & D3DCAPS3_LINEAR_TO_SRGB_PRESENTATION\n");
		supported = false;
	}
	if(!(d3dCaps.Caps3 & D3DCAPS3_COPY_TO_VIDMEM))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  Caps3 & D3DCAPS3_COPY_TO_VIDMEM\n");
		supported = false;
	}
	if(!(d3dCaps.Caps3 & D3DCAPS3_COPY_TO_SYSTEMMEM))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  Caps3 & D3DCAPS3_COPY_TO_SYSTEMMEM\n");
		supported = false;
	}
	if(!(d3dCaps.PresentationIntervals & D3DPRESENT_INTERVAL_DEFAULT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PresentationIntervals & D3DPRESENT_INTERVAL_DEFAULT\n");
		supported = false;
	}
	if(!(d3dCaps.PresentationIntervals & D3DPRESENT_INTERVAL_ONE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PresentationIntervals & D3DPRESENT_INTERVAL_ONE\n");
		supported = false;
	}
	if(!(d3dCaps.PresentationIntervals & D3DPRESENT_INTERVAL_TWO))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PresentationIntervals & D3DPRESENT_INTERVAL_TWO\n");
		supported = false;
	}
	if(!(d3dCaps.PresentationIntervals & D3DPRESENT_INTERVAL_THREE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PresentationIntervals & D3DPRESENT_INTERVAL_THREE\n");
		supported = false;
	}
	if(!(d3dCaps.PresentationIntervals & D3DPRESENT_INTERVAL_FOUR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PresentationIntervals & D3DPRESENT_INTERVAL_FOUR\n");
		supported = false;
	}
	if(!(d3dCaps.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE\n");
		supported = false;
	}
	if(!(d3dCaps.CursorCaps & D3DCURSORCAPS_COLOR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  CursorCaps & D3DCURSORCAPS_COLOR\n");
		supported = false;
	}
	if(!(d3dCaps.CursorCaps & D3DCURSORCAPS_LOWRES))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  CursorCaps & D3DCURSORCAPS_LOWRES\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_EXECUTESYSTEMMEMORY))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_EXECUTESYSTEMMEMORY\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_EXECUTEVIDEOMEMORY))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_EXECUTEVIDEOMEMORY\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_TLVERTEXSYSTEMMEMORY))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_TLVERTEXSYSTEMMEMORY\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_TLVERTEXVIDEOMEMORY))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_TLVERTEXVIDEOMEMORY\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_TEXTURESYSTEMMEMORY))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_TEXTURESYSTEMMEMORY\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_TEXTUREVIDEOMEMORY))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_TEXTUREVIDEOMEMORY\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_DRAWPRIMTLVERTEX))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_DRAWPRIMTLVERTEX\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_CANRENDERAFTERFLIP))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_CANRENDERAFTERFLIP\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_DRAWPRIMITIVES2))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_DRAWPRIMITIVES2\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_SEPARATETEXTUREMEMORIES))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_SEPARATETEXTUREMEMORIES\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_DRAWPRIMITIVES2EX))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_DRAWPRIMITIVES2EX\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_CANBLTSYSTONONLOCAL))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_CANBLTSYSTONONLOCAL\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_HWRASTERIZATION))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_HWRASTERIZATION\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_PUREDEVICE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_PUREDEVICE\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_QUINTICRTPATCHES))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_QUINTICRTPATCHES\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_RTPATCHES))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_RTPATCHES\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_RTPATCHHANDLEZERO))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_RTPATCHHANDLEZERO\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps & D3DDEVCAPS_NPATCHES))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps & D3DDEVCAPS_NPATCHES\n");
		supported = false;
	}
	if(!(d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_MASKZ))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PrimitiveMiscCaps & D3DPMISCCAPS_MASKZ\n");
		supported = false;
	}
	if(!(d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_CULLNONE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PrimitiveMiscCaps & D3DPMISCCAPS_CULLNONE\n");
		supported = false;
	}
	if(!(d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_CULLCW))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PrimitiveMiscCaps & D3DPMISCCAPS_CULLCW\n");
		supported = false;
	}
	if(!(d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_CULLCCW))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PrimitiveMiscCaps & D3DPMISCCAPS_CULLCCW\n");
		supported = false;
	}
	if(!(d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_COLORWRITEENABLE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PrimitiveMiscCaps & D3DPMISCCAPS_COLORWRITEENABLE\n");
		supported = false;
	}
	if(!(d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_CLIPPLANESCALEDPOINTS))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PrimitiveMiscCaps & D3DPMISCCAPS_CLIPPLANESCALEDPOINTS\n");
		supported = false;
	}
	if(!(d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_CLIPTLVERTS))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PrimitiveMiscCaps & D3DPMISCCAPS_CLIPTLVERTS\n");
		supported = false;
	}
	if(!(d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_TSSARGTEMP))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PrimitiveMiscCaps & D3DPMISCCAPS_TSSARGTEMP\n");
		supported = false;
	}
	if(!(d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_BLENDOP))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PrimitiveMiscCaps & D3DPMISCCAPS_BLENDOP\n");
		supported = false;
	}
	if(!(d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_NULLREFERENCE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PrimitiveMiscCaps & D3DPMISCCAPS_NULLREFERENCE\n");
		supported = false;
	}
	if(!(d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_INDEPENDENTWRITEMASKS))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PrimitiveMiscCaps & D3DPMISCCAPS_INDEPENDENTWRITEMASKS\n");
		supported = false;
	}
	if(!(d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_PERSTAGECONSTANT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PrimitiveMiscCaps & D3DPMISCCAPS_PERSTAGECONSTANT\n");
		supported = false;
	}
	if(!(d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_FOGANDSPECULARALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PrimitiveMiscCaps & D3DPMISCCAPS_FOGANDSPECULARALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_SEPARATEALPHABLEND))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PrimitiveMiscCaps & D3DPMISCCAPS_SEPARATEALPHABLEND\n");
		supported = false;
	}
	if(!(d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PrimitiveMiscCaps & D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS\n");
		supported = false;
	}
	if(!(d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PrimitiveMiscCaps & D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING\n");
		supported = false;
	}
	if(!(d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_FOGVERTEXCLAMPED))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PrimitiveMiscCaps & D3DPMISCCAPS_FOGVERTEXCLAMPED\n");
		supported = false;
	}
	if(!(d3dCaps.PrimitiveMiscCaps & D3DPMISCCAPS_POSTBLENDSRGBCONVERT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  PrimitiveMiscCaps & D3DPMISCCAPS_POSTBLENDSRGBCONVERT\n");
		supported = false;
	}
	if(!(d3dCaps.RasterCaps & D3DPRASTERCAPS_DITHER))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  RasterCaps & D3DPRASTERCAPS_DITHER\n");
		supported = false;
	}
	if(!(d3dCaps.RasterCaps & D3DPRASTERCAPS_ZTEST))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  RasterCaps & D3DPRASTERCAPS_ZTEST\n");
		supported = false;
	}
	if(!(d3dCaps.RasterCaps & D3DPRASTERCAPS_FOGVERTEX))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  RasterCaps & D3DPRASTERCAPS_FOGVERTEX\n");
		supported = false;
	}
	if(!(d3dCaps.RasterCaps & D3DPRASTERCAPS_FOGTABLE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  RasterCaps & D3DPRASTERCAPS_FOGTABLE\n");
		supported = false;
	}
	if(!(d3dCaps.RasterCaps & D3DPRASTERCAPS_MIPMAPLODBIAS))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  RasterCaps & D3DPRASTERCAPS_MIPMAPLODBIAS\n");
		supported = false;
	}
	if(!(d3dCaps.RasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  RasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR\n");
		supported = false;
	}
	if(!(d3dCaps.RasterCaps & D3DPRASTERCAPS_FOGRANGE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  RasterCaps & D3DPRASTERCAPS_FOGRANGE\n");
		supported = false;
	}
	if(!(d3dCaps.RasterCaps & D3DPRASTERCAPS_ANISOTROPY))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  RasterCaps & D3DPRASTERCAPS_ANISOTROPY\n");
		supported = false;
	}
	if(!(d3dCaps.RasterCaps & D3DPRASTERCAPS_WBUFFER))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  RasterCaps & D3DPRASTERCAPS_WBUFFER\n");
		supported = false;
	}
	if(!(d3dCaps.RasterCaps & D3DPRASTERCAPS_WFOG))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  RasterCaps & D3DPRASTERCAPS_WFOG\n");
		supported = false;
	}
	if(!(d3dCaps.RasterCaps & D3DPRASTERCAPS_ZFOG))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  RasterCaps & D3DPRASTERCAPS_ZFOG\n");
		supported = false;
	}
	if(!(d3dCaps.RasterCaps & D3DPRASTERCAPS_COLORPERSPECTIVE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  RasterCaps & D3DPRASTERCAPS_COLORPERSPECTIVE\n");
		supported = false;
	}
	if(!(d3dCaps.RasterCaps & D3DPRASTERCAPS_SCISSORTEST))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  RasterCaps & D3DPRASTERCAPS_SCISSORTEST\n");
		supported = false;
	}
	if(!(d3dCaps.RasterCaps & D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  RasterCaps & D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS\n");
		supported = false;
	}
	if(!(d3dCaps.RasterCaps & D3DPRASTERCAPS_DEPTHBIAS))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  RasterCaps & D3DPRASTERCAPS_DEPTHBIAS\n");
		supported = false;
	}
	if(!(d3dCaps.RasterCaps & D3DPRASTERCAPS_MULTISAMPLE_TOGGLE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  RasterCaps & D3DPRASTERCAPS_MULTISAMPLE_TOGGLE\n");
		supported = false;
	}
	if(!(d3dCaps.ZCmpCaps & D3DPCMPCAPS_NEVER))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  ZCmpCaps & D3DPCMPCAPS_NEVER\n");
		supported = false;
	}
	if(!(d3dCaps.ZCmpCaps & D3DPCMPCAPS_LESS))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  ZCmpCaps & D3DPCMPCAPS_LESS\n");
		supported = false;
	}
	if(!(d3dCaps.ZCmpCaps & D3DPCMPCAPS_EQUAL))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  ZCmpCaps & D3DPCMPCAPS_EQUAL\n");
		supported = false;
	}
	if(!(d3dCaps.ZCmpCaps & D3DPCMPCAPS_LESSEQUAL))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  ZCmpCaps & D3DPCMPCAPS_LESSEQUAL\n");
		supported = false;
	}
	if(!(d3dCaps.ZCmpCaps & D3DPCMPCAPS_GREATER))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  ZCmpCaps & D3DPCMPCAPS_GREATER\n");
		supported = false;
	}
	if(!(d3dCaps.ZCmpCaps & D3DPCMPCAPS_NOTEQUAL))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  ZCmpCaps & D3DPCMPCAPS_NOTEQUAL\n");
		supported = false;
	}
	if(!(d3dCaps.ZCmpCaps & D3DPCMPCAPS_GREATEREQUAL))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  ZCmpCaps & D3DPCMPCAPS_GREATEREQUAL\n");
		supported = false;
	}
	if(!(d3dCaps.ZCmpCaps & D3DPCMPCAPS_ALWAYS))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  ZCmpCaps & D3DPCMPCAPS_ALWAYS\n");
		supported = false;
	}
	if(!(d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_ZERO))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  SrcBlendCaps & D3DPBLENDCAPS_ZERO\n");
		supported = false;
	}
	if(!(d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_ONE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  SrcBlendCaps & D3DPBLENDCAPS_ONE\n");
		supported = false;
	}
	if(!(d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_SRCCOLOR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  SrcBlendCaps & D3DPBLENDCAPS_SRCCOLOR\n");
		supported = false;
	}
	if(!(d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_INVSRCCOLOR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  SrcBlendCaps & D3DPBLENDCAPS_INVSRCCOLOR\n");
		supported = false;
	}
	if(!(d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_SRCALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  SrcBlendCaps & D3DPBLENDCAPS_SRCALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_INVSRCALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  SrcBlendCaps & D3DPBLENDCAPS_INVSRCALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_DESTALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  SrcBlendCaps & D3DPBLENDCAPS_DESTALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_INVDESTALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  SrcBlendCaps & D3DPBLENDCAPS_INVDESTALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_DESTCOLOR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  SrcBlendCaps & D3DPBLENDCAPS_DESTCOLOR\n");
		supported = false;
	}
	if(!(d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_INVDESTCOLOR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  SrcBlendCaps & D3DPBLENDCAPS_INVDESTCOLOR\n");
		supported = false;
	}
	if(!(d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_SRCALPHASAT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  SrcBlendCaps & D3DPBLENDCAPS_SRCALPHASAT\n");
		supported = false;
	}
	if(!(d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_BOTHSRCALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  SrcBlendCaps & D3DPBLENDCAPS_BOTHSRCALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_BOTHINVSRCALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  SrcBlendCaps & D3DPBLENDCAPS_BOTHINVSRCALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_BLENDFACTOR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  SrcBlendCaps & D3DPBLENDCAPS_BLENDFACTOR\n");
		supported = false;
	}
	if(!(d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_SRCCOLOR2))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  SrcBlendCaps & D3DPBLENDCAPS_SRCCOLOR2\n");
		supported = false;
	}
	if(!(d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_INVSRCCOLOR2))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  SrcBlendCaps & D3DPBLENDCAPS_INVSRCCOLOR2\n");
		supported = false;
	}
	if(!(d3dCaps.DestBlendCaps & D3DPBLENDCAPS_ZERO))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DestBlendCaps & D3DPBLENDCAPS_ZERO\n");
		supported = false;
	}
	if(!(d3dCaps.DestBlendCaps & D3DPBLENDCAPS_ONE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DestBlendCaps & D3DPBLENDCAPS_ONE\n");
		supported = false;
	}
	if(!(d3dCaps.DestBlendCaps & D3DPBLENDCAPS_SRCCOLOR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DestBlendCaps & D3DPBLENDCAPS_SRCCOLOR\n");
		supported = false;
	}
	if(!(d3dCaps.DestBlendCaps & D3DPBLENDCAPS_INVSRCCOLOR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DestBlendCaps & D3DPBLENDCAPS_INVSRCCOLOR\n");
		supported = false;
	}
	if(!(d3dCaps.DestBlendCaps & D3DPBLENDCAPS_SRCALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DestBlendCaps & D3DPBLENDCAPS_SRCALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.DestBlendCaps & D3DPBLENDCAPS_INVSRCALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DestBlendCaps & D3DPBLENDCAPS_INVSRCALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.DestBlendCaps & D3DPBLENDCAPS_DESTALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DestBlendCaps & D3DPBLENDCAPS_DESTALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.DestBlendCaps & D3DPBLENDCAPS_INVDESTALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DestBlendCaps & D3DPBLENDCAPS_INVDESTALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.DestBlendCaps & D3DPBLENDCAPS_DESTCOLOR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DestBlendCaps & D3DPBLENDCAPS_DESTCOLOR\n");
		supported = false;
	}
	if(!(d3dCaps.DestBlendCaps & D3DPBLENDCAPS_INVDESTCOLOR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DestBlendCaps & D3DPBLENDCAPS_INVDESTCOLOR\n");
		supported = false;
	}
	if(!(d3dCaps.DestBlendCaps & D3DPBLENDCAPS_SRCALPHASAT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DestBlendCaps & D3DPBLENDCAPS_SRCALPHASAT\n");
		supported = false;
	}
	if(!(d3dCaps.DestBlendCaps & D3DPBLENDCAPS_BOTHSRCALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DestBlendCaps & D3DPBLENDCAPS_BOTHSRCALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.DestBlendCaps & D3DPBLENDCAPS_BOTHINVSRCALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DestBlendCaps & D3DPBLENDCAPS_BOTHINVSRCALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.DestBlendCaps & D3DPBLENDCAPS_BLENDFACTOR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DestBlendCaps & D3DPBLENDCAPS_BLENDFACTOR\n");
		supported = false;
	}
	if(!(d3dCaps.DestBlendCaps & D3DPBLENDCAPS_SRCCOLOR2))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DestBlendCaps & D3DPBLENDCAPS_SRCCOLOR2\n");
		supported = false;
	}
	if(!(d3dCaps.DestBlendCaps & D3DPBLENDCAPS_INVSRCCOLOR2))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DestBlendCaps & D3DPBLENDCAPS_INVSRCCOLOR2\n");
		supported = false;
	}
	if(!(d3dCaps.AlphaCmpCaps & D3DPCMPCAPS_NEVER))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  AlphaCmpCaps & D3DPCMPCAPS_NEVER\n");
		supported = false;
	}
	if(!(d3dCaps.AlphaCmpCaps & D3DPCMPCAPS_LESS))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  AlphaCmpCaps & D3DPCMPCAPS_LESS\n");
		supported = false;
	}
	if(!(d3dCaps.AlphaCmpCaps & D3DPCMPCAPS_EQUAL))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  AlphaCmpCaps & D3DPCMPCAPS_EQUAL\n");
		supported = false;
	}
	if(!(d3dCaps.AlphaCmpCaps & D3DPCMPCAPS_LESSEQUAL))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  AlphaCmpCaps & D3DPCMPCAPS_LESSEQUAL\n");
		supported = false;
	}
	if(!(d3dCaps.AlphaCmpCaps & D3DPCMPCAPS_GREATER))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  AlphaCmpCaps & D3DPCMPCAPS_GREATER\n");
		supported = false;
	}
	if(!(d3dCaps.AlphaCmpCaps & D3DPCMPCAPS_NOTEQUAL))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  AlphaCmpCaps & D3DPCMPCAPS_NOTEQUAL\n");
		supported = false;
	}
	if(!(d3dCaps.AlphaCmpCaps & D3DPCMPCAPS_GREATEREQUAL))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  AlphaCmpCaps & D3DPCMPCAPS_GREATEREQUAL\n");
		supported = false;
	}
	if(!(d3dCaps.AlphaCmpCaps & D3DPCMPCAPS_ALWAYS))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  AlphaCmpCaps & D3DPCMPCAPS_ALWAYS\n");
		supported = false;
	}
	if(!(d3dCaps.ShadeCaps & D3DPSHADECAPS_COLORGOURAUDRGB))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  ShadeCaps & D3DPSHADECAPS_COLORGOURAUDRGB\n");
		supported = false;
	}
	if(!(d3dCaps.ShadeCaps & D3DPSHADECAPS_SPECULARGOURAUDRGB))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  ShadeCaps & D3DPSHADECAPS_SPECULARGOURAUDRGB\n");
		supported = false;
	}
	if(!(d3dCaps.ShadeCaps & D3DPSHADECAPS_ALPHAGOURAUDBLEND))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  ShadeCaps & D3DPSHADECAPS_ALPHAGOURAUDBLEND\n");
		supported = false;
	}
	if(!(d3dCaps.ShadeCaps & D3DPSHADECAPS_FOGGOURAUD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  ShadeCaps & D3DPSHADECAPS_FOGGOURAUD\n");
		supported = false;
	}
	if(!(d3dCaps.TextureCaps & D3DPTEXTURECAPS_PERSPECTIVE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureCaps & D3DPTEXTURECAPS_PERSPECTIVE\n");
		supported = false;
	}
	if(!(d3dCaps.TextureCaps & D3DPTEXTURECAPS_POW2))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureCaps & D3DPTEXTURECAPS_POW2\n");
		supported = false;
	}
	if(!(d3dCaps.TextureCaps & D3DPTEXTURECAPS_ALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureCaps & D3DPTEXTURECAPS_ALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureCaps & D3DPTEXTURECAPS_SQUAREONLY\n");
		supported = false;
	}
	if(!(d3dCaps.TextureCaps & D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureCaps & D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE\n");
		supported = false;
	}
	if(!(d3dCaps.TextureCaps & D3DPTEXTURECAPS_ALPHAPALETTE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureCaps & D3DPTEXTURECAPS_ALPHAPALETTE\n");
		supported = false;
	}
	if(!(d3dCaps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL\n");
		supported = false;
	}
	if(!(d3dCaps.TextureCaps & D3DPTEXTURECAPS_PROJECTED))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureCaps & D3DPTEXTURECAPS_PROJECTED\n");
		supported = false;
	}
	if(!(d3dCaps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureCaps & D3DPTEXTURECAPS_CUBEMAP\n");
		supported = false;
	}
	if(!(d3dCaps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP\n");
		supported = false;
	}
	if(!(d3dCaps.TextureCaps & D3DPTEXTURECAPS_MIPMAP))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureCaps & D3DPTEXTURECAPS_MIPMAP\n");
		supported = false;
	}
	if(!(d3dCaps.TextureCaps & D3DPTEXTURECAPS_MIPVOLUMEMAP))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureCaps & D3DPTEXTURECAPS_MIPVOLUMEMAP\n");
		supported = false;
	}
	if(!(d3dCaps.TextureCaps & D3DPTEXTURECAPS_MIPCUBEMAP))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureCaps & D3DPTEXTURECAPS_MIPCUBEMAP\n");
		supported = false;
	}
	if(!(d3dCaps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP_POW2))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureCaps & D3DPTEXTURECAPS_CUBEMAP_POW2\n");
		supported = false;
	}
	if(!(d3dCaps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP_POW2))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP_POW2\n");
		supported = false;
	}
	if(!(d3dCaps.TextureCaps & D3DPTEXTURECAPS_NOPROJECTEDBUMPENV))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureCaps & D3DPTEXTURECAPS_NOPROJECTEDBUMPENV\n");
		supported = false;
	}
	if(!(d3dCaps.TextureFilterCaps & D3DPTFILTERCAPS_MINFPOINT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureFilterCaps & D3DPTFILTERCAPS_MINFPOINT\n");
		supported = false;
	}
	if(!(d3dCaps.TextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR\n");
		supported = false;
	}
	if(!(d3dCaps.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC\n");
		supported = false;
	}
	if(!(d3dCaps.TextureFilterCaps & D3DPTFILTERCAPS_MINFPYRAMIDALQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureFilterCaps & D3DPTFILTERCAPS_MINFPYRAMIDALQUAD\n");
		supported = false;
	}
	if(!(d3dCaps.TextureFilterCaps & D3DPTFILTERCAPS_MINFGAUSSIANQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureFilterCaps & D3DPTFILTERCAPS_MINFGAUSSIANQUAD\n");
		supported = false;
	}
	if(!(d3dCaps.TextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT\n");
		supported = false;
	}
	if(!(d3dCaps.TextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR\n");
		supported = false;
	}
	if(!(d3dCaps.TextureFilterCaps & D3DPTFILTERCAPS_CONVOLUTIONMONO))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureFilterCaps & D3DPTFILTERCAPS_CONVOLUTIONMONO\n");
		supported = false;
	}
	if(!(d3dCaps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT\n");
		supported = false;
	}
	if(!(d3dCaps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR\n");
		supported = false;
	}
	if(!(d3dCaps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC\n");
		supported = false;
	}
	if(!(d3dCaps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureFilterCaps & D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD\n");
		supported = false;
	}
	if(!(d3dCaps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFGAUSSIANQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureFilterCaps & D3DPTFILTERCAPS_MAGFGAUSSIANQUAD\n");
		supported = false;
	}
	if(!(d3dCaps.CubeTextureFilterCaps & D3DPTFILTERCAPS_MINFPOINT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  CubeTextureFilterCaps & D3DPTFILTERCAPS_MINFPOINT\n");
		supported = false;
	}
	if(!(d3dCaps.CubeTextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  CubeTextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR\n");
		supported = false;
	}
	if(!(d3dCaps.CubeTextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  CubeTextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC\n");
		supported = false;
	}
	if(!(d3dCaps.CubeTextureFilterCaps & D3DPTFILTERCAPS_MINFPYRAMIDALQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  CubeTextureFilterCaps & D3DPTFILTERCAPS_MINFPYRAMIDALQUAD\n");
		supported = false;
	}
	if(!(d3dCaps.CubeTextureFilterCaps & D3DPTFILTERCAPS_MINFGAUSSIANQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  CubeTextureFilterCaps & D3DPTFILTERCAPS_MINFGAUSSIANQUAD\n");
		supported = false;
	}
	if(!(d3dCaps.CubeTextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  CubeTextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT\n");
		supported = false;
	}
	if(!(d3dCaps.CubeTextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  CubeTextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR\n");
		supported = false;
	}
	if(!(d3dCaps.CubeTextureFilterCaps & D3DPTFILTERCAPS_CONVOLUTIONMONO))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  CubeTextureFilterCaps & D3DPTFILTERCAPS_CONVOLUTIONMONO\n");
		supported = false;
	}
	if(!(d3dCaps.CubeTextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  CubeTextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT\n");
		supported = false;
	}
	if(!(d3dCaps.CubeTextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  CubeTextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR\n");
		supported = false;
	}
	if(!(d3dCaps.CubeTextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  CubeTextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC\n");
		supported = false;
	}
	if(!(d3dCaps.CubeTextureFilterCaps & D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  CubeTextureFilterCaps & D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD\n");
		supported = false;
	}
	if(!(d3dCaps.CubeTextureFilterCaps & D3DPTFILTERCAPS_MAGFGAUSSIANQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  CubeTextureFilterCaps & D3DPTFILTERCAPS_MAGFGAUSSIANQUAD\n");
		supported = false;
	}
	if(!(d3dCaps.VolumeTextureFilterCaps & D3DPTFILTERCAPS_MINFPOINT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VolumeTextureFilterCaps & D3DPTFILTERCAPS_MINFPOINT\n");
		supported = false;
	}
	if(!(d3dCaps.VolumeTextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VolumeTextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR\n");
		supported = false;
	}
	if(!(d3dCaps.VolumeTextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VolumeTextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC\n");
		supported = false;
	}
	if(!(d3dCaps.VolumeTextureFilterCaps & D3DPTFILTERCAPS_MINFPYRAMIDALQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VolumeTextureFilterCaps & D3DPTFILTERCAPS_MINFPYRAMIDALQUAD\n");
		supported = false;
	}
	if(!(d3dCaps.VolumeTextureFilterCaps & D3DPTFILTERCAPS_MINFGAUSSIANQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VolumeTextureFilterCaps & D3DPTFILTERCAPS_MINFGAUSSIANQUAD\n");
		supported = false;
	}
	if(!(d3dCaps.VolumeTextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VolumeTextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT\n");
		supported = false;
	}
	if(!(d3dCaps.VolumeTextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VolumeTextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR\n");
		supported = false;
	}
	if(!(d3dCaps.VolumeTextureFilterCaps & D3DPTFILTERCAPS_CONVOLUTIONMONO))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VolumeTextureFilterCaps & D3DPTFILTERCAPS_CONVOLUTIONMONO\n");
		supported = false;
	}
	if(!(d3dCaps.VolumeTextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VolumeTextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT\n");
		supported = false;
	}
	if(!(d3dCaps.VolumeTextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VolumeTextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR\n");
		supported = false;
	}
	if(!(d3dCaps.VolumeTextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VolumeTextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC\n");
		supported = false;
	}
	if(!(d3dCaps.VolumeTextureFilterCaps & D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VolumeTextureFilterCaps & D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD\n");
		supported = false;
	}
	if(!(d3dCaps.VolumeTextureFilterCaps & D3DPTFILTERCAPS_MAGFGAUSSIANQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VolumeTextureFilterCaps & D3DPTFILTERCAPS_MAGFGAUSSIANQUAD\n");
		supported = false;
	}
	if(!(d3dCaps.TextureAddressCaps & D3DPTADDRESSCAPS_WRAP))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureAddressCaps & D3DPTADDRESSCAPS_WRAP\n");
		supported = false;
	}
	if(!(d3dCaps.TextureAddressCaps & D3DPTADDRESSCAPS_MIRROR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureAddressCaps & D3DPTADDRESSCAPS_MIRROR\n");
		supported = false;
	}
	if(!(d3dCaps.TextureAddressCaps & D3DPTADDRESSCAPS_CLAMP))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureAddressCaps & D3DPTADDRESSCAPS_CLAMP\n");
		supported = false;
	}
	if(!(d3dCaps.TextureAddressCaps & D3DPTADDRESSCAPS_BORDER))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureAddressCaps & D3DPTADDRESSCAPS_BORDER\n");
		supported = false;
	}
	if(!(d3dCaps.TextureAddressCaps & D3DPTADDRESSCAPS_INDEPENDENTUV))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureAddressCaps & D3DPTADDRESSCAPS_INDEPENDENTUV\n");
		supported = false;
	}
	if(!(d3dCaps.TextureAddressCaps & D3DPTADDRESSCAPS_MIRRORONCE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureAddressCaps & D3DPTADDRESSCAPS_MIRRORONCE\n");
		supported = false;
	}
	if(!(d3dCaps.VolumeTextureAddressCaps & D3DPTADDRESSCAPS_WRAP))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VolumeTextureAddressCaps & D3DPTADDRESSCAPS_WRAP\n");
		supported = false;
	}
	if(!(d3dCaps.VolumeTextureAddressCaps & D3DPTADDRESSCAPS_MIRROR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VolumeTextureAddressCaps & D3DPTADDRESSCAPS_MIRROR\n");
		supported = false;
	}
	if(!(d3dCaps.VolumeTextureAddressCaps & D3DPTADDRESSCAPS_CLAMP))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VolumeTextureAddressCaps & D3DPTADDRESSCAPS_CLAMP\n");
		supported = false;
	}
	if(!(d3dCaps.VolumeTextureAddressCaps & D3DPTADDRESSCAPS_BORDER))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VolumeTextureAddressCaps & D3DPTADDRESSCAPS_BORDER\n");
		supported = false;
	}
	if(!(d3dCaps.VolumeTextureAddressCaps & D3DPTADDRESSCAPS_INDEPENDENTUV))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VolumeTextureAddressCaps & D3DPTADDRESSCAPS_INDEPENDENTUV\n");
		supported = false;
	}
	if(!(d3dCaps.VolumeTextureAddressCaps & D3DPTADDRESSCAPS_MIRRORONCE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VolumeTextureAddressCaps & D3DPTADDRESSCAPS_MIRRORONCE\n");
		supported = false;
	}
	if(!(d3dCaps.LineCaps & D3DLINECAPS_TEXTURE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  LineCaps & D3DLINECAPS_TEXTURE\n");
		supported = false;
	}
	if(!(d3dCaps.LineCaps & D3DLINECAPS_ZTEST))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  LineCaps & D3DLINECAPS_ZTEST\n");
		supported = false;
	}
	if(!(d3dCaps.LineCaps & D3DLINECAPS_BLEND))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  LineCaps & D3DLINECAPS_BLEND\n");
		supported = false;
	}
	if(!(d3dCaps.LineCaps & D3DLINECAPS_ALPHACMP))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  LineCaps & D3DLINECAPS_ALPHACMP\n");
		supported = false;
	}
	if(!(d3dCaps.LineCaps & D3DLINECAPS_FOG))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  LineCaps & D3DLINECAPS_FOG\n");
		supported = false;
	}
	if(!(d3dCaps.LineCaps & D3DLINECAPS_ANTIALIAS))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  LineCaps & D3DLINECAPS_ANTIALIAS\n");
		supported = false;
	}
	if(d3dCaps.MaxTextureWidth < 2048)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxTextureWidth (%i) < 2048\n", d3dCaps.MaxTextureWidth);
		supported = false;
	}
	if(d3dCaps.MaxTextureHeight < 2048)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxTextureHeight (%i) < 2048\n", d3dCaps.MaxTextureHeight);
		supported = false;
	}
	if(d3dCaps.MaxVolumeExtent < 1024)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxVolumeExtent (%i) < 1024\n", d3dCaps.MaxVolumeExtent);
		supported = false;
	}
	if(d3dCaps.MaxTextureRepeat < 2048)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxTextureRepeat (%i) < 2048\n", d3dCaps.MaxTextureRepeat);
		supported = false;
	}
	if(d3dCaps.MaxTextureAspectRatio < 2048)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxTextureAspectRatio (%i) < 2048\n", d3dCaps.MaxTextureAspectRatio);
		supported = false;
	}
	if(d3dCaps.MaxAnisotropy < 16)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxAnisotropy (%i) < 16\n", d3dCaps.MaxAnisotropy);
		supported = false;
	}
	if(d3dCaps.MaxVertexW < 536870912)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxVertexW (%f) < 10000000000.000000\n", d3dCaps.MaxVertexW);
		supported = false;
	}
	//if(d3dCaps.GuardBandLeft < 0)
	//{
		//LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : GuardBandLeft (%f) < -960.000000\n", d3dCaps.GuardBandLeft);
		//supported = false;
	//}
	//if(d3dCaps.GuardBandTop < 0)
	//{
		//LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : GuardBandTop (%f) < -960.000000\n", d3dCaps.GuardBandTop);
		//supported = false;
	//}
	//if(d3dCaps.GuardBandRight < 0)
	//{
		//LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : GuardBandRight (%f) < 2880.000000\n", d3dCaps.GuardBandRight);
		//supported = false;
	//}
	//if(d3dCaps.GuardBandBottom < 0)
	//{
		//LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : GuardBandBottom (%f) < 2880.000000\n", d3dCaps.GuardBandBottom);
		//supported = false;
	//}
	//if(d3dCaps.ExtentsAdjust < 0)
	//{
		//LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : ExtentsAdjust (%f) < 0.000000\n", d3dCaps.ExtentsAdjust);
		//supported = false;
	//}
	if(!(d3dCaps.StencilCaps & D3DSTENCILCAPS_KEEP))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StencilCaps & D3DSTENCILCAPS_KEEP\n");
		supported = false;
	}
	if(!(d3dCaps.StencilCaps & D3DSTENCILCAPS_ZERO))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StencilCaps & D3DSTENCILCAPS_ZERO\n");
		supported = false;
	}
	if(!(d3dCaps.StencilCaps & D3DSTENCILCAPS_REPLACE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StencilCaps & D3DSTENCILCAPS_REPLACE\n");
		supported = false;
	}
	if(!(d3dCaps.StencilCaps & D3DSTENCILCAPS_INCRSAT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StencilCaps & D3DSTENCILCAPS_INCRSAT\n");
		supported = false;
	}
	if(!(d3dCaps.StencilCaps & D3DSTENCILCAPS_DECRSAT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StencilCaps & D3DSTENCILCAPS_DECRSAT\n");
		supported = false;
	}
	if(!(d3dCaps.StencilCaps & D3DSTENCILCAPS_INVERT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StencilCaps & D3DSTENCILCAPS_INVERT\n");
		supported = false;
	}
	if(!(d3dCaps.StencilCaps & D3DSTENCILCAPS_INCR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StencilCaps & D3DSTENCILCAPS_INCR\n");
		supported = false;
	}
	if(!(d3dCaps.StencilCaps & D3DSTENCILCAPS_DECR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StencilCaps & D3DSTENCILCAPS_DECR\n");
		supported = false;
	}
	if(!(d3dCaps.StencilCaps & D3DSTENCILCAPS_TWOSIDED))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StencilCaps & D3DSTENCILCAPS_TWOSIDED\n");
		supported = false;
	}
	if(!(d3dCaps.FVFCaps & D3DFVFCAPS_TEXCOORDCOUNTMASK))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  FVFCaps & D3DFVFCAPS_TEXCOORDCOUNTMASK\n");
		supported = false;
	}
	if(!(d3dCaps.FVFCaps & D3DFVFCAPS_DONOTSTRIPELEMENTS))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  FVFCaps & D3DFVFCAPS_DONOTSTRIPELEMENTS\n");
		supported = false;
	}
	if(!(d3dCaps.FVFCaps & D3DFVFCAPS_PSIZE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  FVFCaps & D3DFVFCAPS_PSIZE\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_DISABLE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_DISABLE\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_SELECTARG1))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_SELECTARG1\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_SELECTARG2))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_SELECTARG2\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_MODULATE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_MODULATE\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_MODULATE2X))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_MODULATE2X\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_MODULATE4X))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_MODULATE4X\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_ADD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_ADD\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_ADDSIGNED))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_ADDSIGNED\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_ADDSIGNED2X))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_ADDSIGNED2X\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_SUBTRACT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_SUBTRACT\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_ADDSMOOTH))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_ADDSMOOTH\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_BLENDDIFFUSEALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_BLENDDIFFUSEALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_BLENDTEXTUREALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_BLENDTEXTUREALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_BLENDFACTORALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_BLENDFACTORALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_BLENDTEXTUREALPHAPM))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_BLENDTEXTUREALPHAPM\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_BLENDCURRENTALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_BLENDCURRENTALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_PREMODULATE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_PREMODULATE\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_BUMPENVMAP))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_BUMPENVMAP\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_BUMPENVMAPLUMINANCE))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_BUMPENVMAPLUMINANCE\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_DOTPRODUCT3))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_DOTPRODUCT3\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_MULTIPLYADD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_MULTIPLYADD\n");
		supported = false;
	}
	if(!(d3dCaps.TextureOpCaps & D3DTEXOPCAPS_LERP))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  TextureOpCaps & D3DTEXOPCAPS_LERP\n");
		supported = false;
	}
	if(d3dCaps.MaxTextureBlendStages < 8)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxTextureBlendStages (%i) < 8\n", d3dCaps.MaxTextureBlendStages);
		supported = false;
	}
	if(d3dCaps.MaxSimultaneousTextures < 8)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxSimultaneousTextures (%i) < 8\n", d3dCaps.MaxSimultaneousTextures);
		supported = false;
	}
	if(!(d3dCaps.VertexProcessingCaps & D3DVTXPCAPS_TEXGEN))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexProcessingCaps & D3DVTXPCAPS_TEXGEN\n");
		supported = false;
	}
	if(!(d3dCaps.VertexProcessingCaps & D3DVTXPCAPS_MATERIALSOURCE7))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexProcessingCaps & D3DVTXPCAPS_MATERIALSOURCE7\n");
		supported = false;
	}
	if(!(d3dCaps.VertexProcessingCaps & D3DVTXPCAPS_DIRECTIONALLIGHTS))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexProcessingCaps & D3DVTXPCAPS_DIRECTIONALLIGHTS\n");
		supported = false;
	}
	if(!(d3dCaps.VertexProcessingCaps & D3DVTXPCAPS_POSITIONALLIGHTS))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexProcessingCaps & D3DVTXPCAPS_POSITIONALLIGHTS\n");
		supported = false;
	}
	if(!(d3dCaps.VertexProcessingCaps & D3DVTXPCAPS_LOCALVIEWER))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexProcessingCaps & D3DVTXPCAPS_LOCALVIEWER\n");
		supported = false;
	}
	if(!(d3dCaps.VertexProcessingCaps & D3DVTXPCAPS_TWEENING))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexProcessingCaps & D3DVTXPCAPS_TWEENING\n");
		supported = false;
	}
	if(!(d3dCaps.VertexProcessingCaps & D3DVTXPCAPS_TEXGEN_SPHEREMAP))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexProcessingCaps & D3DVTXPCAPS_TEXGEN_SPHEREMAP\n");
		supported = false;
	}
	if(!(d3dCaps.VertexProcessingCaps & D3DVTXPCAPS_NO_TEXGEN_NONLOCALVIEWER))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexProcessingCaps & D3DVTXPCAPS_NO_TEXGEN_NONLOCALVIEWER\n");
		supported = false;
	}
	if(d3dCaps.MaxActiveLights < 8)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxActiveLights (%i) < 8\n", d3dCaps.MaxActiveLights);
		supported = false;
	}
	if(d3dCaps.MaxUserClipPlanes < 6)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxUserClipPlanes (%i) < 6\n", d3dCaps.MaxUserClipPlanes);
		supported = false;
	}
	if(d3dCaps.MaxVertexBlendMatrices < 4)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxVertexBlendMatrices (%i) < 4\n", d3dCaps.MaxVertexBlendMatrices);
		supported = false;
	}
	if(d3dCaps.MaxVertexBlendMatrixIndex < 37)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxVertexBlendMatrixIndex (%i) < 37\n", d3dCaps.MaxVertexBlendMatrixIndex);
		supported = false;
	}
	if(d3dCaps.MaxPointSize < 0)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxPointSize (%f) < 256.000000\n", d3dCaps.MaxPointSize);
		supported = false;
	}
	if(d3dCaps.MaxPrimitiveCount < 1048575)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxPrimitiveCount (%i) < 1048575\n", d3dCaps.MaxPrimitiveCount);
		supported = false;
	}
	if(d3dCaps.MaxVertexIndex < 16777215)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxVertexIndex (%i) < 16777215\n", d3dCaps.MaxVertexIndex);
		supported = false;
	}
	if(d3dCaps.MaxStreams < 16)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxStreams (%i) < 16\n", d3dCaps.MaxStreams);
		supported = false;
	}
	if(d3dCaps.MaxStreamStride < 508)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxStreamStride (%i) < 508\n", d3dCaps.MaxStreamStride);
		supported = false;
	}
	//if(d3dCaps.VertexShaderVersion < -130560)
	//{
		//LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : VertexShaderVersion (%i) < -130560\n", d3dCaps.VertexShaderVersion);
		//supported = false;
	//}
	if(d3dCaps.MaxVertexShaderConst < 256)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxVertexShaderConst (%i) < 256\n", d3dCaps.MaxVertexShaderConst);
		supported = false;
	}
	//if(d3dCaps.PixelShaderVersion < -65024)
	//{
		//LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : PixelShaderVersion (%i) < -65024\n", d3dCaps.PixelShaderVersion);
		//supported = false;
	//}
	if(d3dCaps.PixelShader1xMaxValue < -536870912)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : PixelShader1xMaxValue (%f) < 340282346638528860000000000000000000000.000000\n", d3dCaps.PixelShader1xMaxValue);
		supported = false;
	}
	if(!(d3dCaps.DevCaps2 & D3DDEVCAPS2_STREAMOFFSET))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps2 & D3DDEVCAPS2_STREAMOFFSET\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps2 & D3DDEVCAPS2_DMAPNPATCH))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps2 & D3DDEVCAPS2_DMAPNPATCH\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps2 & D3DDEVCAPS2_ADAPTIVETESSRTPATCH))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps2 & D3DDEVCAPS2_ADAPTIVETESSRTPATCH\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps2 & D3DDEVCAPS2_ADAPTIVETESSNPATCH))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps2 & D3DDEVCAPS2_ADAPTIVETESSNPATCH\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps2 & D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps2 & D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps2 & D3DDEVCAPS2_PRESAMPLEDDMAPNPATCH))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps2 & D3DDEVCAPS2_PRESAMPLEDDMAPNPATCH\n");
		supported = false;
	}
	if(!(d3dCaps.DevCaps2 & D3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DevCaps2 & D3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET\n");
		supported = false;
	}
	if(d3dCaps.MaxNpatchTessellationLevel < 0)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxNpatchTessellationLevel (%f) < 1.000000\n", d3dCaps.MaxNpatchTessellationLevel);
		supported = false;
	}
	//if(d3dCaps.Reserved5 < 0)
	//{
		//LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : Reserved5 (%i) < 0\n", d3dCaps.Reserved5);
		//supported = false;
	//}
	//if(d3dCaps.MasterAdapterOrdinal < 0)
	//{
		//LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MasterAdapterOrdinal (%i) < 0\n", d3dCaps.MasterAdapterOrdinal);
		//supported = false;
	//}
	//if(d3dCaps.AdapterOrdinalInGroup < 0)
	//{
		//LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : AdapterOrdinalInGroup (%i) < 0\n", d3dCaps.AdapterOrdinalInGroup);
		//supported = false;
	//}
	if(d3dCaps.NumberOfAdaptersInGroup < 1)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : NumberOfAdaptersInGroup (%i) < 1\n", d3dCaps.NumberOfAdaptersInGroup);
		supported = false;
	}
	if(!(d3dCaps.DeclTypes & D3DDTCAPS_UBYTE4))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DeclTypes & D3DDTCAPS_UBYTE4\n");
		supported = false;
	}
	if(!(d3dCaps.DeclTypes & D3DDTCAPS_UBYTE4N))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DeclTypes & D3DDTCAPS_UBYTE4N\n");
		supported = false;
	}
	if(!(d3dCaps.DeclTypes & D3DDTCAPS_SHORT2N))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DeclTypes & D3DDTCAPS_SHORT2N\n");
		supported = false;
	}
	if(!(d3dCaps.DeclTypes & D3DDTCAPS_SHORT4N))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DeclTypes & D3DDTCAPS_SHORT4N\n");
		supported = false;
	}
	if(!(d3dCaps.DeclTypes & D3DDTCAPS_USHORT2N))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DeclTypes & D3DDTCAPS_USHORT2N\n");
		supported = false;
	}
	if(!(d3dCaps.DeclTypes & D3DDTCAPS_USHORT4N))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DeclTypes & D3DDTCAPS_USHORT4N\n");
		supported = false;
	}
	if(!(d3dCaps.DeclTypes & D3DDTCAPS_UDEC3))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DeclTypes & D3DDTCAPS_UDEC3\n");
		supported = false;
	}
	if(!(d3dCaps.DeclTypes & D3DDTCAPS_DEC3N))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DeclTypes & D3DDTCAPS_DEC3N\n");
		supported = false;
	}
	if(!(d3dCaps.DeclTypes & D3DDTCAPS_FLOAT16_2))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DeclTypes & D3DDTCAPS_FLOAT16_2\n");
		supported = false;
	}
	if(!(d3dCaps.DeclTypes & D3DDTCAPS_FLOAT16_4))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  DeclTypes & D3DDTCAPS_FLOAT16_4\n");
		supported = false;
	}
	if(d3dCaps.NumSimultaneousRTs < 4)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : NumSimultaneousRTs (%i) < 4\n", d3dCaps.NumSimultaneousRTs);
		supported = false;
	}
	if(!(d3dCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MINFPOINT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StretchRectFilterCaps & D3DPTFILTERCAPS_MINFPOINT\n");
		supported = false;
	}
	if(!(d3dCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MINFLINEAR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StretchRectFilterCaps & D3DPTFILTERCAPS_MINFLINEAR\n");
		supported = false;
	}
	if(!(d3dCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StretchRectFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC\n");
		supported = false;
	}
	if(!(d3dCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MINFPYRAMIDALQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StretchRectFilterCaps & D3DPTFILTERCAPS_MINFPYRAMIDALQUAD\n");
		supported = false;
	}
	if(!(d3dCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MINFGAUSSIANQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StretchRectFilterCaps & D3DPTFILTERCAPS_MINFGAUSSIANQUAD\n");
		supported = false;
	}
	if(!(d3dCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MIPFPOINT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StretchRectFilterCaps & D3DPTFILTERCAPS_MIPFPOINT\n");
		supported = false;
	}
	if(!(d3dCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StretchRectFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR\n");
		supported = false;
	}
	if(!(d3dCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_CONVOLUTIONMONO))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StretchRectFilterCaps & D3DPTFILTERCAPS_CONVOLUTIONMONO\n");
		supported = false;
	}
	if(!(d3dCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MAGFPOINT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StretchRectFilterCaps & D3DPTFILTERCAPS_MAGFPOINT\n");
		supported = false;
	}
	if(!(d3dCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StretchRectFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR\n");
		supported = false;
	}
	if(!(d3dCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StretchRectFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC\n");
		supported = false;
	}
	if(!(d3dCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StretchRectFilterCaps & D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD\n");
		supported = false;
	}
	if(!(d3dCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MAGFGAUSSIANQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  StretchRectFilterCaps & D3DPTFILTERCAPS_MAGFGAUSSIANQUAD\n");
		supported = false;
	}
	if(d3dCaps.VS20Caps.Caps < 0)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : VS20Caps.Caps (%i) < 0\n", d3dCaps.VS20Caps.Caps);
		supported = false;
	}
	if(d3dCaps.VS20Caps.DynamicFlowControlDepth < 0)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : VS20Caps.DynamicFlowControlDepth (%i) < 0\n", d3dCaps.VS20Caps.DynamicFlowControlDepth);
		supported = false;
	}
	if(d3dCaps.VS20Caps.NumTemps < 16)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : VS20Caps.NumTemps (%i) < 16\n", d3dCaps.VS20Caps.NumTemps);
		supported = false;
	}
	if(d3dCaps.VS20Caps.StaticFlowControlDepth < 1)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : VS20Caps.StaticFlowControlDepth (%i) < 1\n", d3dCaps.VS20Caps.StaticFlowControlDepth);
		supported = false;
	}
	if(d3dCaps.PS20Caps.Caps < 0)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : PS20Caps.Caps (%i) < 0\n", d3dCaps.PS20Caps.Caps);
		supported = false;
	}
	if(d3dCaps.PS20Caps.DynamicFlowControlDepth < 0)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : PS20Caps.DynamicFlowControlDepth (%i) < 0\n", d3dCaps.PS20Caps.DynamicFlowControlDepth);
		supported = false;
	}
	if(d3dCaps.PS20Caps.NumTemps < 12)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : PS20Caps.NumTemps (%i) < 12\n", d3dCaps.PS20Caps.NumTemps);
		supported = false;
	}
	if(d3dCaps.PS20Caps.StaticFlowControlDepth < 0)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : PS20Caps.StaticFlowControlDepth (%i) < 0\n", d3dCaps.PS20Caps.StaticFlowControlDepth);
		supported = false;
	}
	if(d3dCaps.PS20Caps.NumInstructionSlots < 96)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : PS20Caps.NumInstructionSlots (%i) < 96\n", d3dCaps.PS20Caps.NumInstructionSlots);
		supported = false;
	}
	if(!(d3dCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MINFPOINT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexTextureFilterCaps & D3DPTFILTERCAPS_MINFPOINT\n");
		supported = false;
	}
	if(!(d3dCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexTextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR\n");
		supported = false;
	}
	if(!(d3dCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexTextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC\n");
		supported = false;
	}
	if(!(d3dCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MINFPYRAMIDALQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexTextureFilterCaps & D3DPTFILTERCAPS_MINFPYRAMIDALQUAD\n");
		supported = false;
	}
	if(!(d3dCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MINFGAUSSIANQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexTextureFilterCaps & D3DPTFILTERCAPS_MINFGAUSSIANQUAD\n");
		supported = false;
	}
	if(!(d3dCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexTextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT\n");
		supported = false;
	}
	if(!(d3dCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexTextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR\n");
		supported = false;
	}
	if(!(d3dCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_CONVOLUTIONMONO))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexTextureFilterCaps & D3DPTFILTERCAPS_CONVOLUTIONMONO\n");
		supported = false;
	}
	if(!(d3dCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexTextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT\n");
		supported = false;
	}
	if(!(d3dCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexTextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR\n");
		supported = false;
	}
	if(!(d3dCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexTextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC\n");
		supported = false;
	}
	if(!(d3dCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexTextureFilterCaps & D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD\n");
		supported = false;
	}
	if(!(d3dCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MAGFGAUSSIANQUAD))
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure :  VertexTextureFilterCaps & D3DPTFILTERCAPS_MAGFGAUSSIANQUAD\n");
		supported = false;
	}
	if(d3dCaps.MaxVShaderInstructionsExecuted < -1)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxVShaderInstructionsExecuted (%i) < -1\n", d3dCaps.MaxVShaderInstructionsExecuted);
		supported = false;
	}
	if(d3dCaps.MaxPShaderInstructionsExecuted < 0)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxPShaderInstructionsExecuted (%i) < 0\n", d3dCaps.MaxPShaderInstructionsExecuted);
		supported = false;
	}
	if(d3dCaps.MaxVertexShader30InstructionSlots < 0)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxVertexShader30InstructionSlots (%i) < 0\n", d3dCaps.MaxVertexShader30InstructionSlots);
		supported = false;
	}
	if(d3dCaps.MaxPixelShader30InstructionSlots < 0)
	{
		LOGINFO(g_Log, "*** D3DDeviceCapsCheckFailure : MaxPixelShader30InstructionSlots (%i) < 0\n", d3dCaps.MaxPixelShader30InstructionSlots);
		supported = false;
	}

	return supported;
}

}; // namespace D3D9
}; // namespace Display
