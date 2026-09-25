// [Blur1Multi] The HLSL XenosRecomp's XENOS_RECOMP_D3D9 output includes: the
// helpers its recompiled programs call, for Direct3D 9 shader model 3
// (upstream shader_common.h for D3D12/Vulkan is kept beside it for reference).
// Kept as a C++ raw string so the recompiler can prepend it without files.
#pragma once

static const char* const g_xenosRecompD3D9Header = R"HLSL(
// XenosRecomp: FLT_MIN is asfloat(0xff7fffff), i.e. -FLT_MAX
#define FLT_MIN (-3.402823466e+38)
#define FLT_MAX (3.402823466e+38)

float4 tfetch1D(sampler s, float texCoord) { return tex1D(s, texCoord); }
float4 tfetch2D(sampler s, float2 texCoord) { return tex2D(s, texCoord); }
float4 tfetch3D(sampler s, float3 texCoord) { return tex3D(s, texCoord); }
float4 tfetch1DLod(sampler s, float texCoord) { return tex2Dlod(s, float4(texCoord, 0.0, 0.0, 0.0)); }
float4 tfetch2DLod(sampler s, float2 texCoord) { return tex2Dlod(s, float4(texCoord, 0.0, 0.0)); }
float4 tfetch3DLod(sampler s, float3 texCoord) { return tex3Dlod(s, float4(texCoord, 0.0)); }

// The Xenos cube instruction and the cube fetch that consumes it: the direction
// is kept (two slots, as upstream's CubeMapData) and sampled directly.
struct CubeMapData
{
    float3 direction0;
    float3 direction1;
    float index;
};

float4 cube(float4 value, inout CubeMapData cubeMapData)
{
    float index = cubeMapData.index;
    if (index < 0.5)
        cubeMapData.direction0 = value.xyz;
    else
        cubeMapData.direction1 = value.xyz;
    cubeMapData.index = index + 1.0;
    return float4(0.0, 0.0, 0.0, index);
}

float4 tfetchCube(sampler s, float3 texCoord, inout CubeMapData cubeMapData)
{
    return texCUBE(s, texCoord.z < 0.5 ? cubeMapData.direction0 : cubeMapData.direction1);
}

float4 tfetchCubeLod(sampler s, float3 texCoord, inout CubeMapData cubeMapData)
{
    return texCUBElod(s, float4(texCoord.z < 0.5 ? cubeMapData.direction0 : cubeMapData.direction1, 0.0));
}

float4 dst(float4 src0, float4 src1)
{
    float4 dest;
    dest.x = 1.0;
    dest.y = src0.y * src1.y;
    dest.z = src0.z;
    dest.w = src1.w;
    return dest;
}

float4 max4(float4 src0)
{
    return max(max(src0.x, src0.y), max(src0.z, src0.w));
}

// [Blur1Multi] DP2ADD as a multiply and two adds: d3dcompiler_47 folds "c - dot(abs(a), abs(a))"
// written as dot() into dp2add with one source negated and drops that source's abs modifier
// (softlightflare_ln: dp2add r0.x, r0_abs, -r0, c0.z, which whites out half of every light
// flare; found with apitrace, 2026-09-25). Same helper as RsxTranslate's Dot2 for PS3 programs.
float dp2add_xenos(float2 a, float2 b, float c)
{
    float2 p = a * b;
    return p.x + p.y + c;
}
)HLSL";
