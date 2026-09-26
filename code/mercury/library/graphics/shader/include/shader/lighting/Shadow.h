#ifndef SHADER_LIGHTING_SHADOW
#define SHADER_LIGHTING_SHADOW

float ShadowMapPoint(sampler ShadowMapSampler, float3 vShadowCoord)
{
	float fDepthValue = tex2D(ShadowMapSampler, vShadowCoord.xy).r;
	float fDepthCompare = step(vShadowCoord.z, fDepthValue);

	return fDepthCompare;
}

float ShadowMapHardware(sampler ShadowMapSampler, float3 vShadowCoord)
{
	return tex2D(ShadowMapSampler, vShadowCoord);
}

float ShadowBilinear(sampler ShadowMapSampler, float3 vShadowCoord, float fShadowSize, float fShadowInvSize)
{
	float4 vDepthValues;
#if PLATFORM_XENON
	asm
	{
		tfetch2D vDepthValues.x___, vShadowCoord.xy, ShadowMapSampler, OffsetX = +0.0f, OffsetY = +0.0f
		tfetch2D vDepthValues._x__, vShadowCoord.xy, ShadowMapSampler, OffsetX = +1.0f, OffsetY = +0.0f
		tfetch2D vDepthValues.__x_, vShadowCoord.xy, ShadowMapSampler, OffsetX = +0.0f, OffsetY = +1.0f
		tfetch2D vDepthValues.___x, vShadowCoord.xy, ShadowMapSampler, OffsetX = +1.0f, OffsetY = +1.0f
		sge vDepthValues.xyzw, vDepthValues.xyzw, vShadowCoord.zzzz
	};
#else
	vDepthValues.x = tex2D(ShadowMapSampler, vShadowCoord.xy + float2(0.0f, 0.0f)).r;
	vDepthValues.y = tex2D(ShadowMapSampler, vShadowCoord.xy + float2(fShadowInvSize, 0.0f)).r;
	vDepthValues.z = tex2D(ShadowMapSampler, vShadowCoord.xy + float2(0.0f, fShadowInvSize)).r;
	vDepthValues.w = tex2D(ShadowMapSampler, vShadowCoord.xy + float2(fShadowInvSize, fShadowInvSize)).r;
	vDepthValues = step(vShadowCoord.z, vDepthValues);
#endif

	float2 vShadowFrac = frac(vShadowCoord.xy * fShadowSize);
	float2 vShadowLerp = lerp(vDepthValues.xz, vDepthValues.yw, vShadowFrac.x);

	return lerp(vShadowLerp.x, vShadowLerp.y, vShadowFrac.y);
}

float ShadowMapPCF(sampler ShadowMapSampler, float3 vShadowCoord, float fShadowSize, float fShadowInvSize)
{
#if PLATFORM_XENON	
	float3 vDepthValues012;
	float3 vDepthValues345;
	float3 vDepthValues678;
	asm
	{
		tfetch2D vDepthValues012.x___, vShadowCoord.xy, ShadowMapSampler, OffsetX = -1.0f, OffsetY = -1.0f
		tfetch2D vDepthValues012._x__, vShadowCoord.xy, ShadowMapSampler, OffsetX = -1.0f, OffsetY =  0.0f
		tfetch2D vDepthValues012.__x_, vShadowCoord.xy, ShadowMapSampler, OffsetX = -1.0f, OffsetY = +1.0f
		sge vDepthValues012.xyz, vDepthValues012.xyz, vShadowCoord.zzz
		tfetch2D vDepthValues345.x___, vShadowCoord.xy, ShadowMapSampler, OffsetX =  0.0f, OffsetY = -1.0f
		tfetch2D vDepthValues345._x__, vShadowCoord.xy, ShadowMapSampler, OffsetX =  0.0f, OffsetY =  0.0f
		tfetch2D vDepthValues345.__x_, vShadowCoord.xy, ShadowMapSampler, OffsetX =  0.0f, OffsetY = +1.0f
		sge vDepthValues345.xyz, vDepthValues345.xyz, vShadowCoord.zzz
		tfetch2D vDepthValues678.x___, vShadowCoord.xy, ShadowMapSampler, OffsetX = +1.0f, OffsetY = -1.0f
		tfetch2D vDepthValues678._x__, vShadowCoord.xy, ShadowMapSampler, OffsetX = +1.0f, OffsetY =  0.0f
		tfetch2D vDepthValues678.__x_, vShadowCoord.xy, ShadowMapSampler, OffsetX = +1.0f, OffsetY = +1.0f
		sge vDepthValues678.xyz, vDepthValues678.xyz, vShadowCoord.zzz
	};
	
	float2 vShadowFrac = frac(vShadowCoord.xy * fShadowSize);
	
	float3 vShadowLerp1 = lerp(vDepthValues012, vDepthValues345, vShadowFrac.x);
	float3 vShadowLerp2 = lerp(vDepthValues345, vDepthValues678, vShadowFrac.x);
	
	float4 vShadowLerp;
	vShadowLerp.xy = lerp(vShadowLerp1.xy, vShadowLerp1.yz, vShadowFrac.yy);
	vShadowLerp.zw = lerp(vShadowLerp2.xy, vShadowLerp2.yz, vShadowFrac.yy);

	return dot(vShadowLerp, float4(0.25f, 0.25f, 0.25f, 0.25f));
#else
	float3 vDepthValues012;
	vDepthValues012.x = tex2D(ShadowMapSampler, vShadowCoord.xy + float2(-fShadowInvSize, -fShadowInvSize)).r;
	vDepthValues012.y = tex2D(ShadowMapSampler, vShadowCoord.xy + float2(-fShadowInvSize,  0.0f)).r;
	vDepthValues012.z = tex2D(ShadowMapSampler, vShadowCoord.xy + float2(-fShadowInvSize, +fShadowInvSize)).r;
	vDepthValues012 = step(vShadowCoord.z, vDepthValues012);

	float3 vDepthValues345;
	vDepthValues345.x = tex2D(ShadowMapSampler, vShadowCoord.xy + float2(0.0f, -fShadowInvSize)).r;
	vDepthValues345.y = tex2D(ShadowMapSampler, vShadowCoord.xy).r;
	vDepthValues345.z = tex2D(ShadowMapSampler, vShadowCoord.xy + float2(0.0f, +fShadowInvSize)).r;
	vDepthValues345 = step(vShadowCoord.z, vDepthValues345);

	float3 vDepthValues678;
	vDepthValues678.x = tex2D(ShadowMapSampler, vShadowCoord.xy + float2(+fShadowInvSize, -fShadowInvSize)).r;
	vDepthValues678.y = tex2D(ShadowMapSampler, vShadowCoord.xy + float2(+fShadowInvSize,  0.0f)).r;
	vDepthValues678.z = tex2D(ShadowMapSampler, vShadowCoord.xy + float2(+fShadowInvSize, +fShadowInvSize)).r;
	vDepthValues678 = step(vShadowCoord.z, vDepthValues678);

	float2 vShadowFrac = frac(vShadowCoord.xy * fShadowSize);

	float3 vShadowLerp1 = lerp(vDepthValues012, vDepthValues345, vShadowFrac.x);
	float3 vShadowLerp2 = lerp(vDepthValues345, vDepthValues678, vShadowFrac.x);
	
	float4 vShadowLerp;
	vShadowLerp.xy = lerp(vShadowLerp1.xy, vShadowLerp1.yz, vShadowFrac.yy);
	vShadowLerp.zw = lerp(vShadowLerp2.xy, vShadowLerp2.yz, vShadowFrac.yy);

	return dot(vShadowLerp, float4(0.25f, 0.25f, 0.25f, 0.25f));
#endif
}

float ShadowMapHardwarePCF(sampler ShadowMapSampler, float3 vShadowCoord, float fShadowSize, float fShadowInvSize)
{
	float fHalfPixelSize = 0.5f * fShadowInvSize;

	float4 vDepthValues;
	vDepthValues.x = tex2D(ShadowMapSampler, vShadowCoord.xyz + float3(-fHalfPixelSize, -fHalfPixelSize, 0.0f)).r;
	vDepthValues.y = tex2D(ShadowMapSampler, vShadowCoord.xyz + float3(-fHalfPixelSize, +fHalfPixelSize, 0.0f)).r;
	vDepthValues.z = tex2D(ShadowMapSampler, vShadowCoord.xyz + float3(+fHalfPixelSize, -fHalfPixelSize, 0.0f)).r;
	vDepthValues.w = tex2D(ShadowMapSampler, vShadowCoord.xyz + float3(+fHalfPixelSize, +fHalfPixelSize, 0.0f)).r;

	float2 vShadowFrac = frac(vShadowCoord.xy * fShadowSize);
	float2 vShadowLerp = lerp(vDepthValues.xz, vDepthValues.yw, vShadowFrac.x);

	return lerp(vShadowLerp.x, vShadowLerp.y, vShadowFrac.y);
}

float ShadowMapVariance(sampler ShadowMapSampler, float3 vShadowCoord, float fPower = 16.0f)
{
	float2	vSample		= tex2D(ShadowMapSampler, vShadowCoord.xy).rg;
	float	fAvgZ		= vSample.r;
	float	fAvgZ2		= vSample.g;
	float	fVariance	= (fAvgZ2) - (fAvgZ * fAvgZ);

	float	fMean		= fAvgZ;
	float	fD			= vShadowCoord.z - fMean;
	float	fPMax		= fVariance / (fVariance + fD * fD);

	return (vShadowCoord.z > fAvgZ) ? pow(fPMax, fPower) : 1.0f;
}

float ShadowMapExponential(sampler ShadowMapSampler, float3 vShadowCoord, float fK, float fQ)
{
	float fDepth = tex2D(ShadowMapSampler, vShadowCoord.xy).r;
	return saturate(fQ - exp(fK * (vShadowCoord.z - fDepth)));
}

float ShadowMapExponentialBilinear(sampler ShadowMapSampler, float3 vShadowCoord, float fShadowSize, float fShadowInvSize, float fK, float fQ)
{
	float4 vDepthValues;
#if PLATFORM_XENON
	asm
	{
		tfetch2D vDepthValues.x___, vShadowCoord.xy, ShadowMapSampler, OffsetX = +0.0f, OffsetY = +0.0f
		tfetch2D vDepthValues._x__, vShadowCoord.xy, ShadowMapSampler, OffsetX = +1.0f, OffsetY = +0.0f
		tfetch2D vDepthValues.__x_, vShadowCoord.xy, ShadowMapSampler, OffsetX = +0.0f, OffsetY = +1.0f
		tfetch2D vDepthValues.___x, vShadowCoord.xy, ShadowMapSampler, OffsetX = +1.0f, OffsetY = +1.0f
	};
#else
	vDepthValues.x = tex2D(ShadowMapSampler, vShadowCoord.xy).r;
	vDepthValues.y = tex2D(ShadowMapSampler, vShadowCoord.xy + float2(fShadowInvSize, 0.0f)).r;
	vDepthValues.z = tex2D(ShadowMapSampler, vShadowCoord.xy + float2(0.0f, fShadowInvSize)).r;
	vDepthValues.w = tex2D(ShadowMapSampler, vShadowCoord.xy + float2(fShadowInvSize, fShadowInvSize)).r;
#endif

	float2 vShadowFrac = frac(vShadowCoord.xy * fShadowSize);
	float2 vShadowLerp = lerp(vDepthValues.xz, vDepthValues.yw, vShadowFrac.x);
	float fDepth = lerp(vShadowLerp.x, vShadowLerp.y, vShadowFrac.y);

	return saturate(fQ - exp(fK * (vShadowCoord.z - fDepth)));
}

#endif // SHADER_LIGHTING_SHADOW
