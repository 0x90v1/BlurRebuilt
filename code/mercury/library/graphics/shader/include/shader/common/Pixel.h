#ifndef SHADER_PIXEL_H
#define SHADER_PIXEL_H

C_PixelOutput MainPS(in C_VertexOutput vtxData) 
{ 
    C_PixelOutput output = (C_PixelOutput) 0;

	C_Material	mtlData		= EvaluateMaterial(vtxData);
	C_Surface	surfaceData	= EvaluateSurface(mtlData, vtxData);
	C_Lighting	lightData	= EvaluateLighting(mtlData, surfaceData, vtxData);

	real3	vDiffuse		= EvaluateDiffuse(mtlData, surfaceData, lightData, vtxData);
	real3	vSpecular		= EvaluateSpecular(mtlData, surfaceData, lightData, vtxData);

	output.m_vColour.xyz	= EvaluateColour(vDiffuse, vSpecular, mtlData, surfaceData, lightData);
	output.m_vColour.w		= EvaluateAlpha(mtlData);

	output.m_vColour		= EvaluateFog(output.m_vColour, surfaceData);
	output.m_vColour		= EvaluateFinalColour(output.m_vColour);
#if !defined(ALPHA_TO_COVERAGE)
	output.m_vColour.xyz	*= output.m_vColour.w;
#endif

	output.m_vColour.a		*= vtxData.m_vNormal.a;

#ifndef CFG_SPEED
#if !defined(CFG_CONVERTER)

#ifdef PLATFORM_XENON

	if (length(g_vDebugColour) > 0.0)
		output.m_vColour = g_vDebugColour;

#if defined(ENABLE_LIGHTMAP)
	if(g_DebugToggles.m_bShowLumels)
	{
		#ifdef BASIS_LIGHTING
		#define LIGHTMAP_SIZE (512.0f * 2.0f)
		#else
		#define LIGHTMAP_SIZE (512.0f)
		#endif

		float2 grid = frac(vtxData.m_vTexCoord.zw * LIGHTMAP_SIZE);

		if((grid.x > 0.9) || (grid.y > 0.9))
		{
			output.m_vColour.rgb = float3(0.0f, 0.0f, 1.0f);
		}
	}
#endif // ENABLE_LIGHTMAP

#if defined(ENABLE_TANGENTNORMAL)
	if(g_DebugToggles.m_bShowTangentNormal)
	{
		output.m_vColour.rgb = mtlData.m_vTangentNormal * 0.5f + 0.5f;
	}
#endif // ENABLE_TANGENTNORMAL

#if defined(ENABLE_TANGENTNORMAL)
	if(g_DebugToggles.m_bShowWorldTangent)
	{
		output.m_vColour.rgb = vtxData.m_vTangent.xyz * 0.5f + 0.5f;
	}
#endif // ENABLE_TANGENTNORMAL

#if defined(ENABLE_SPECULARMAP) && defined(ENABLE_SPECULAR)
	if(g_DebugToggles.m_bShowSpecular)
	{
		real3 vSpecularSample = tex2D(SpecularSampler, vtxData.m_vTexCoord.xy).rgb;
		output.m_vColour.rgb = pow(vSpecularSample, 1.0f / 2.2f);
	}
#endif // ENABLE_TANGENTNORMAL

	if(g_DebugToggles.m_bNoLighting)
	{
		output.m_vColour		= EvaluateFinalColour( float4( mtlData.m_vAlbedo, 1.0f ));
	}
	

#endif // PLATFORM_XENON
#endif // MAYA || XSI
#endif // CFG_SPEED

	//output.m_vColour.rgb = saturate( lightData.m_vDirection.xyz * 0.5f + 0.5f );

    return output;
}

#endif // SHADER_PIXEL_H
