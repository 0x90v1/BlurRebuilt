#include "..\vsCommon.h"

float4x4	g_UiTransform						: register( CCV_UI_TRANSFORM );

float4x4	g_PolygonTransform					: register( CCV_POLYGON_TRANSFORM );
float4x4	g_PolygonGradientColourTransform	: register( CCV_POLYGON_COLOUR_TRANSFORM );

struct VS_INPUT
{
	float2 Position				: POSITION;
	float4 Normal				: NORMAL;
};

struct VS_OUTPUT
{
	float4 Colour				: COLOR;
	float4 Position				: POSITION;
};

VS_OUTPUT main( VS_INPUT IN )
{
	VS_OUTPUT OUT = (VS_OUTPUT)0;

	// Calc pos	
	float4 pos = mul( float4( IN.Position.xy, 0.0f, 1.0f ), g_PolygonTransform );
	OUT.Position = mul( pos, g_UiTransform );

	// Calc colour (positions in it's own 0-1 space)
//	float4 p = mul( float4( IN.Position.xy, 0.0f, 1.0f ), g_PolygonTransform );
	OUT.Colour = mul( float4( IN.Position.xy, 0.0f, 1.0f ), g_PolygonGradientColourTransform );
	
	
	return OUT;
}
