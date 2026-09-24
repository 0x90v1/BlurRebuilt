/*---------------------------------------------------------------------------------------
	File	: vsPolygonTrail.vsh

	Desc	: 
---------------------------------------------------------------------------------------*/

#include "..\vsCommon.h"

float4x4	g_UiTransform						: register( CCV_UI_TRANSFORM );
float4x4	g_PolygonTransform					: register( CCV_POLYGON_TRANSFORM );

float4		g_PolygonColour						: register( CCV_POLYGON_COLOUR );
float4		g_TextureVector						: register( CCV_POLYGON_TEXTURE_VECTOR );
float4		g_WaveSettings						: register( CCV_POLYGON_WAVE );

// For usability.
#define		g_Time								g_WaveSettings.x 
#define		g_Intensity							g_WaveSettings.y 
#define		g_Frequency							g_WaveSettings.z 

struct VS_INPUT
{
	float2 Position				: POSITION;
	float2 TexCoord0			: TEXCOORD0;
	float4 Normal				: NORMAL;
};


struct VS_OUTPUT
{
	float4	Position			: POSITION;
	float4	Colour				: COLOR0;
	float2	TexCoord0			: TEXCOORD0;
	//float4	WorldPosition		: TEXCOORD0;
};


VS_OUTPUT main( VS_INPUT IN )
{
	VS_OUTPUT OUT = (VS_OUTPUT)0;

	float4 pos = float4( IN.Position.xy, 0.0f, 1.0f );
	
	IN.Normal = normalize( IN.Normal );

	pos.x += ( sin( ( IN.TexCoord0.x * g_Frequency ) + g_Time ) * g_Intensity ) * abs( IN.Normal.x );
	pos.y += ( sin( ( IN.TexCoord0.x * g_Frequency ) + g_Time ) * g_Intensity ) * abs( IN.Normal.y );

	/*
	pos.y += sin( ( pos.x * g_Frequency ) + g_Time ) * g_Intensity;
	pos.y += sin( ( pos.x * g_Frequency * 0.3f ) + cos( g_Time ) ) * g_Intensity ;
	pos.y += sin( ( pos.x * cos( g_Frequency ) * 0.3f ) + g_Time + 3.2f ) * g_Intensity;
	*/

	// Pass to the pixel shader
	OUT.Position = mul( mul( pos, g_PolygonTransform ), g_UiTransform );
	OUT.TexCoord0 = g_TextureVector.xy + (IN.TexCoord0 * g_TextureVector.zw);
	OUT.Colour = g_PolygonColour;
	
	return OUT;
}