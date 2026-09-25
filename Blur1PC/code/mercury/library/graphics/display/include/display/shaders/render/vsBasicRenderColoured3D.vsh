#include "..\\vsConstants.h"
#include "..\\vsCommon.h"

float4x4 g_WorldMatrix		: register(CCV_DISPLAY_WORLD_MATRIX);
float4x4 g_ViewProjMatrix	: register(CCV_DISPLAY_VIEWPROJ_MATRIX);

struct VS_INPUT
{
	float3 Position			: POSITION;
	float4 Colour0			: COLOR0;
};

struct VS_OUTPUT
{
	float4 Position			: POSITION;
	float4 Colour0			: COLOR0;
};

VS_OUTPUT main(VS_INPUT IN)
{
	VS_OUTPUT OUT = (VS_OUTPUT)0;
	
	float4 position = float4(IN.Position, 1.f);
	
	position = mul(position, g_WorldMatrix);
	position = mul(position, g_ViewProjMatrix);
	
	OUT.Position = position;
	
	OUT.Colour0 = IN.Colour0;
	
	return OUT;
}