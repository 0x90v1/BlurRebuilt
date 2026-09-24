#include "..\\vsConstants.h"
#include "..\\vsCommon.h"

float4 g_PositionUvVector	: register(CCV_DISPLAY_POSITION_UV_VECTOR);

struct VS_INPUT
{
	float2 Position			: POSITION;
	#ifdef USE_VERTEXCOLOUR
	float4 Colour			: COLOR0;
	#endif
	float2 Uv				: TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 Position			: POSITION;
	#ifdef USE_VERTEXCOLOUR
	float4 Colour			: COLOR0;
	#endif
	float2 Texcoord0		: TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT IN)
{
	VS_OUTPUT OUT;
	
	OUT.Position = float4(IN.Position, 0.f, 1.f);
	OUT.Texcoord0 = IN.Uv;
	#ifdef USE_VERTEXCOLOUR
	OUT.Colour = IN.Colour;
	#endif
	
	return OUT; 
}