#include "..\vsCommon.h"

float4x4	g_UiTransform						: register( CCV_UI_TRANSFORM );
float4		g_UiViewPosition					: register( CCV_UI_VIEW_POSITION );

float4x4	g_PolygonTransform					: register( CCV_POLYGON_TRANSFORM );
float4		g_PolygonColour						: register( CCV_POLYGON_COLOUR );

struct VS_INPUT
{
	float2 Position				: POSITION;
	float4 Normal				: COLOR0;
};

struct VS_OUTPUT
{
	float4 Colour				: COLOR0;
	float4 MyWorldPosition		: TEXCOORD3;
	float4 Position				: POSITION;
};

VS_OUTPUT main( VS_INPUT IN )
{
	VS_OUTPUT OUT = (VS_OUTPUT)0;

	IN.Position = IN.Position * float2(1.0f / 32767.0f, 1.0f / 32767.0f);
	IN.Normal.xy = (IN.Normal.xy - float2(127.0f, 127.0f)) * float2(1.0f / 127.0f, 1.0f / 127.0f);

	float4 newPos = float4( IN.Position.xy, 0.0f, 1.0f );
	float4 pos = mul( newPos, g_PolygonTransform );
	
	// **** THIS IS ALL TEMP
	// this is a bit ewww - push it down instead?
	// - or just make two shaders?
	bool is_2d = ( g_UiTransform[2][3] == 0.0f && g_UiTransform[3][3] == 1.0f );	
	
	float outline_size = 0.0f;
	
	if ( is_2d )
		outline_size = 1.0f;	// should this be sqrt(2) instead?
	else if ( !is_2d )
	{
		// this is fudge - figure it out properly?
		outline_size = length( g_UiViewPosition.xyz - pos.xyz ) * 0.0025f;
	}
	// *** END OF TEMP

	// transform the vector too
	float2 v = mul( float3( IN.Normal.xy, 0.0f ), (float4x3)g_PolygonTransform ).xy;
	v = normalize( v + 0.00001f ); // Temp bodge so we don't normalise a zero y component

#ifdef _FINNED_OUTER_
	v.x *= IN.Normal.w;
	v.y *= IN.Normal.w;
#endif

	pos.xy += ( v * outline_size * length( IN.Normal.xy ) );
	
	OUT.MyWorldPosition = pos;
	
	OUT.Position = mul( OUT.MyWorldPosition, g_UiTransform );

	OUT.Colour = g_PolygonColour;
	OUT.Colour.a *= IN.Normal.z;
	
	return OUT;
}
