#ifndef SHADER_COMMON_DEBUG_H
#define SHADER_COMMON_DEBUG_H

float4 DebugColour(in float4 vColour, in C_Material mtlData, in C_VertexOutput vtxData)
{
#if !defined(CFG_SPEED)
#if !defined(CFG_CONVERTER)
	if (length(g_vDebugColour) > 0.0)
		vColour = g_vDebugColour;

#if defined(PLATFORM_XENON)

#if defined(ENABLE_LIGHTMAP)
	if(g_DebugToggles.m_bShowLumels)
	{
		#if defined(BASIS_LIGHTING)
		#define LIGHTMAP_SIZE (512.0f * 2.0f)
		#else
		#define LIGHTMAP_SIZE (512.0f)
		#endif

		float2 grid = frac(vtxData.m_vTexCoord.zw * LIGHTMAP_SIZE);

		if((grid.x > 0.9) || (grid.y > 0.9))
		{
			vColour.rgb = float3(0.0f, 0.0f, 1.0f);
		}
	}
#endif // ENABLE_LIGHTMAP

#if defined(ENABLE_TANGENTNORMAL)
	if(g_DebugToggles.m_bShowTangentNormal)
	{
		vColour.rgb = mtlData.m_vTangentNormal * 0.5f + 0.5f;
	}
#endif // ENABLE_TANGENTNORMAL

#if defined(ENABLE_TANGENTNORMAL)
	if(g_DebugToggles.m_bShowWorldTangent)
	{
		vColour.rgb = vtxData.m_vTangent.xyz * 0.5f + 0.5f;
	}
#endif // ENABLE_TANGENTNORMAL

#if defined(ENABLE_SPECULARMAP) && defined(ENABLE_SPECULAR)
	if(g_DebugToggles.m_bShowSpecular)
	{
		real3 vSpecularSample = tex2D(SpecularSampler, vtxData.m_vTexCoord.xy).rgb;
		vColour.rgb = pow(vSpecularSample, 1.0f / 2.2f);
	}
#endif // ENABLE_SPECULARMAP
#if defined(ENABLE_MATERIALMAP) && defined(ENABLE_SPECULAR)
	if(g_DebugToggles.m_bShowSpecular)
	{
		real3 vSpecularSample = tex2D(MaterialSampler, vtxData.m_vTexCoord.xy).rgb;
		vColour.rgb = pow(vSpecularSample, 1.0f / 2.2f);
	}
#endif // ENABLE_MATERIALMAP

	if(g_DebugToggles.m_bNoLighting)
	{
		vColour = EvaluateFinalColour( float4( mtlData.m_vAlbedo, 1.0f ));
	}

#if defined(ENABLE_LIGHTMAP)
#if defined(NIGHT_LIGHTING) || defined(DIRECTIONAL_LIGHTING)
	if(g_DebugToggles.m_bShowLightmapRange)
	{
		real3 lightmapColour=1.0f;
#if defined(NIGHT_LIGHTING)
		RGBLightmapNormalisedGamma(LightmapSampler, vtxData.m_vTexCoord.zw, lightmapColour);
#else
		RGBLightmapNormalisedGamma(Lightmap0Sampler, vtxData.m_vTexCoord.zw, lightmapColour);		
#endif
		float lightmapIntensity = dot(lightmapColour, float3(0.33333f, 0.33333f, 0.33333f));
		vColour.rgb = lerp( float3(0.0f, 0.0f, 1.0f), float3(0.0f, 1.0f, 0.0f), lightmapIntensity );

		float band = frac( lightmapIntensity * 8.0f);
		if(band > 0.95f)
		{
			vColour.r += 1.0f;
		}
	}
#endif
#endif // ENABLE_LIGHTMAP

#endif // PLATFORM_XENON
#endif // MAYA || XSI
#endif // CFG_SPEED

	return vColour;
}

#endif // SHADER_COMMON_DEBUG_H
