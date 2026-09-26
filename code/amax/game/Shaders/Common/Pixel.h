#ifndef AMAX_SHADERS_COMMON_PIXEL_H
#define AMAX_SHADERS_COMMON_PIXEL_H

#include <Shader/Common/Debug.h>

C_PixelOutput MainPS(in C_VertexOutput vtxData) 
{ 
    C_PixelOutput output = (C_PixelOutput) 0;

	C_Material	mtlData		= EvaluateMaterial(vtxData);
	C_Surface	surfaceData	= EvaluateSurface(mtlData, vtxData);
	C_Lighting	lightData	= EvaluateLighting(mtlData, surfaceData, vtxData);

	real3	vDiffuse		= EvaluateDiffuse(mtlData, surfaceData, lightData, vtxData);
	real3	vSpecular		= EvaluateSpecular(mtlData, surfaceData, lightData, vtxData);

	output.m_vColour.xyz	= EvaluateColour(vDiffuse, vSpecular, mtlData, surfaceData);
	output.m_vColour.w		= EvaluateAlpha(mtlData);

#if defined(ENABLE_FOG) && !defined(LOD)
	output.m_vColour		= EvaluateDepthFog(output.m_vColour, surfaceData.m_fDistance);
#endif
	output.m_vColour		= EvaluateFinalColour(output.m_vColour);
#if !defined(ALPHA_TO_COVERAGE) && !defined(ALPHA_LUMINANCE)
	output.m_vColour.rgb	*= output.m_vColour.a;
#endif

#if defined(ENABLE_ALPHA_ENHANCE)
	output.m_vColour.a		= saturate(output.m_vColour.a * 1.10h);
#endif

#if !defined(CFG_CONVERTER)
#if !defined(DISABLE_LODFADE)
#if !defined(LODFADE_IN_VERTEX_CONSTANT)
	output.m_vColour.a		*= EvaluateLodFade(surfaceData);
#else
	output.m_vColour.a		*= saturate(vtxData.m_vNormal.w);
#endif
#endif
#endif

	output.m_vColour		= DebugColour(output.m_vColour, mtlData, vtxData);

	return output;
}

struct C_PixelOutputD
{
    float4	 m_vColour	: COLOR0;
    float4   m_fDepth   : COLOR1;
};

#if defined(ENABLE_PREPASS_DEPTHTEXTURE)
C_PixelOutputD PrepassPS(in C_VertexPrepassOutput vtxData)
{
    C_PixelOutputD output = (C_PixelOutputD) 0;
#else
C_PixelOutput PrepassPS(in C_VertexPrepassOutput vtxData)
{
    C_PixelOutput output = (C_PixelOutput) 0;
#endif

	C_Material	mtlData		= EvaluateMaterialPrepass(vtxData);
	C_Surface	surfaceData	= EvaluateSurfacePrepass(mtlData, vtxData);

#if defined(ENABLE_PREPASS_VIEWDEPTH)
	clip(mtlData.m_fAlpha - 0.5h);
	real fDepth255 = vtxData.m_vTexCoord.z * 255.0h;
#if defined(ENABLE_PREPASS)
	output.m_vColour = real4(surfaceData.m_vNormal.xy * 0.5h + 0.5h, floor(fDepth255) / 255.0h, frac(fDepth255));
#endif
#else

#if defined(OPAQUE_PASS)
	output.m_vColour = real4(surfaceData.m_vNormal * 0.5h + 0.5h, 1.0h);
#else
#if defined(ENABLE_PREPASS)
	output.m_vColour = real4(surfaceData.m_vNormal * 0.5h + 0.5h, mtlData.m_fAlpha);
#endif

#if !defined(DISABLE_LODFADE)
#ifndef LODFADE_IN_VERTEX_CONSTANT
	output.m_vColour.a *= saturate(EvaluateLodFade(surfaceData));
#else
	output.m_vColour.a *= saturate(vtxData.m_vNormal.w);
#endif
#endif

#endif // OPAQUE_PASS
 
#if defined(ENABLE_PREPASS_DEPTHTEXTURE)
    output.m_fDepth = mul( vtxData.m_vTexCoord.z,  1 / vtxData.m_vTexCoord.w );
#endif

#endif // ENABLE_PREPASS_VIEWDEPTH

    return output;
}

#endif // AMAX_SHADERS_COMMON_PIXEL_H
