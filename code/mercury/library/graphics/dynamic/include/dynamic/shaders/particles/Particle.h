
struct VS_OUTPUT
{
	float4 position			: POSITION0;

#ifdef POINTSPRITE
	float  pointsize		: PSIZE;
	
	#ifndef XB2
		float4 psuv			: TEXCOORD0;
		float4 uv			: TEXCOORD1;
	#else
		#ifdef PS_3_0
			float2 psuv		: SPRITETEXCOORD;
		#endif
			float4 uv		: TEXCOORD0;
	#endif
#else
	float4 uv : TEXCOORD0;
#endif

#ifdef NORMALMAP
	float3 light			: TEXCOORD1;
#endif

#ifdef REFRACT
	float4 screenUV			: TEXCOORD1;
#endif

#ifdef DEPTH
	float4 positionUV		: TEXCOORD1;
#endif

	float4 worldPositionAndOffset	: TEXCOORD2;
	float4 colour					: TEXCOORD3;
};




