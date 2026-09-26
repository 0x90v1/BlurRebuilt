// shared parameters
shared float4	g_Luminance : register(c27);
shared texture	g_LuminanceTexture;

#define		g_fGain				g_Luminance.x
#define		g_fGamma			g_Luminance.x
#define		g_fLuminanceScale	g_Luminance.y
#define		g_fExposureLow		g_Luminance.z
#define		g_fExposureHigh		g_Luminance.w

// shared samplers
sampler LuminanceSampler =
sampler_state
{
	Texture = <g_LuminanceTexture>;
	MipFilter = POINT;
	MinFilter = POINT;
	MagFilter = POINT;
};

// constants
static const float3	vLuminance			= float3(0.2125f, 0.7154f, 0.0721f);
static const float	fDelta				= 0.0001f;
static const float	fFrameBufferDelta	= 0.004f;

// functions
float GetLogLuminance(float3 colour)
{
	return log(dot(colour, vLuminance) + fDelta);
}

float GetLogLuminanceScaled(float3 colour)
{
	return log(dot(colour, vLuminance) + fDelta) * g_fLuminanceScale;
}

float GetLuminanceScaled(float3 colour)
{
	// make sure the lowest value written to the alpha channel is 1/255
	return max((dot(colour, vLuminance) + fDelta) * g_fLuminanceScale, fFrameBufferDelta);
}

#if PLATFORM_XENON

// does HDR in the shader but still writes alpha as normal
// mainly for use with 7e3 or 6e4 compressed float surfaces
#ifdef HDR

float4 GetExposure(float4 vRGBA, float fAlpha)
{
	float4	vColour		= 0.0f;
	float	fLuminance	= tex2D(LuminanceSampler, float2(0.5f, 0.5f)).r;
	
	vColour.rgb = vRGBA.rgb * (g_fExposureLow / (fLuminance + 0.001f));
	vColour.rgb = pow(vColour.rgb, g_fGamma);

	#ifdef ALPHA_AND_ADDITIVE
		vColour = float4(vColour.rgb * vRGBA.a, (1.0f - fAlpha) * vRGBA.a); 
	#else
		#ifdef ALPHA_PREMULTIPLY
			vColour = float4(vColour.rgb * vRGBA.a, vRGBA.a);
		#else
			vColour.a = vRGBA.a;
		#endif
	#endif

	return vColour;
}

#endif

// uses a single U32 rendertarget for HDR tonemapping
#ifdef HDRU32

float4 GetExposure(float4 vRGBA, float fAlpha)
{
	float4	vColour		= 0.0f;
	float	fLuminance	= tex2D(LuminanceSampler, float2(0.5f, 0.5f)).r;
	
#ifdef ALPHA
	// exposes value but doesnt contribute to luminance
	vColour.rgb = vRGBA.rgb * (g_fExposureLow / (fLuminance + 0.001f));
	vColour.rgb = pow(vColour.rgb, g_fGamma);

	// need to do alpha premultiply after gamma
	#ifdef ALPHA_AND_ADDITIVE
		vColour = float4(vColour.rgb * vRGBA.a, (1.0f - fAlpha) * vRGBA.a); 
	#else
		#ifdef ALPHA_PREMULTIPLY
			vColour = float4(vColour.rgb * vRGBA.a, vRGBA.a);
		#else
			vColour.a = vRGBA.a;
		#endif
	#endif
#else
	// exposes value and writes luminance to alpha channel
	vColour.a =  GetLuminanceScaled(vRGBA.rgb);
	vColour.rgb = vRGBA.rgb * (g_fExposureLow / (fLuminance + 0.001f));
	vColour.rgb = pow(vColour.rgb, g_fGamma);
#endif

	return vColour;
}
#endif

#ifndef HDR
#ifndef HDRU32
#ifndef HDRU32X2

float4 GetExposure(float4 vRGBA, float fAlpha)
{
	// apply gain
	float4	vColour = vRGBA * g_fGain;

	// need to do alpha premultiply after gamma
	#ifdef ALPHA_AND_ADDITIVE
		vColour = float4(vRGBA.rgb * vRGBA.a, (1.0f - fAlpha) * vRGBA.a);
	#else
		#ifdef ALPHA_PREMULTIPLY
			vColour = float4(vRGBA.rgb * vRGBA.a, vRGBA.a);
		#else
			vColour.a = vRGBA.a;
		#endif
	#endif

	return vColour;
}

#endif
#endif
#endif

#else // PLATFORM_XENON

float4 GetExposure(float4 vColour, float fAlpha)
{
	return float4(vColour.rgb, fAlpha);
}

#endif

float4 GetExposure(float4 vColour)
{
	return GetExposure(vColour, 1.0f);
}
