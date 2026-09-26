// functions to filter variance shadow maps

float4 DepthToVariance(float fDepth)
{
	return float4(fDepth, fDepth * fDepth, 0.0f, 0.0f);
}

float4 VarianceGaussianBlur_5Taps(sampler2D tShadowMap, float2 vTexCoord, float2 vPixelOffset)
{
	float4	vSamples01;
	float4	vSamples23;
	float2	vSamples4_;

	vSamples01.xy	= tex2D(tShadowMap, vTexCoord - 2.0f * vPixelOffset).xy;
	vSamples01.zw	= tex2D(tShadowMap, vTexCoord - 1.0f * vPixelOffset).xy;
	vSamples23.xy	= tex2D(tShadowMap, vTexCoord).xy;
	vSamples23.zw	= tex2D(tShadowMap, vTexCoord + 1.0f * vPixelOffset).xy;
	vSamples4_.xy	= tex2D(tShadowMap, vTexCoord + 2.0f * vPixelOffset).xy;

	vSamples01		= vSamples01 * float4(1.0f, 1.0f, 4.0f, 4.0f) / 16.0f;
	vSamples23		= vSamples23 * float4(6.0f, 6.0f, 4.0f, 4.0f) / 16.0f;
	vSamples4_		= vSamples4_ * float2(1.0f, 1.0f) / 16.0f;

	float4	vSamples0123	= vSamples01 + vSamples23;

	return float4(vSamples0123.xy + vSamples0123.zw + vSamples4_.xy, 0.0f, 0.0f);
}

float4 VarianceGaussianBlur_7Taps(sampler2D tShadowMap, float2 vTexCoord, float2 vPixelOffset)
{
	float4	vSamples01;
	float4	vSamples23;
	float4	vSamples45;
	float4	vSamples6_;

	vSamples01.xy	= tex2D(tShadowMap, vTexCoord - 3.0f * vPixelOffset).xy;
	vSamples01.zw	= tex2D(tShadowMap, vTexCoord - 2.0f * vPixelOffset).xy;
	vSamples23.xy	= tex2D(tShadowMap, vTexCoord - 1.0f * vPixelOffset).xy;
	vSamples23.zw	= tex2D(tShadowMap, vTexCoord).xy;
	vSamples45.xy	= tex2D(tShadowMap, vTexCoord + 1.0f * vPixelOffset).xy;
	vSamples45.zw	= tex2D(tShadowMap, vTexCoord + 2.0f * vPixelOffset).xy;
	vSamples6_.xy	= tex2D(tShadowMap, vTexCoord + 3.0f * vPixelOffset).xy;
	vSamples6_.zw	= 0.0f;

	vSamples01		= vSamples01 * float4(1.0f, 1.0f, 6.0f, 6.0f) / 64.0f;
	vSamples23		= vSamples23 * float4(15.0f, 15.0f, 20.0f, 20.0f) / 64.0f;
	vSamples45		= vSamples45 * float4(15.0f, 15.0f, 6.0f, 6.0f) / 64.0f;
	vSamples6_		= vSamples6_ * float4(1.0f, 1.0f, 1.0f, 1.0f) / 64.0f;

	float4	vSamples0123	= vSamples01 + vSamples23;
	float4	vSamples456_	= vSamples45 + vSamples6_;

	return float4(vSamples0123.xy + vSamples0123.zw + vSamples456_.xy + vSamples456_.zw, 0.0f, 0.0f);
}

float4 VarianceGaussianBlurFromDepth_5Taps(sampler2D tShadowMap, float2 vTexCoord, float2 vPixelOffset)
{
	float4	vSamples0123;
	float	vSamples4___;
	vSamples0123.x	= SampleDepthBuffer(tShadowMap, vTexCoord - 2.0f * vPixelOffset);
	vSamples0123.y	= SampleDepthBuffer(tShadowMap, vTexCoord - 1.0f * vPixelOffset);
	vSamples0123.z	= SampleDepthBuffer(tShadowMap, vTexCoord);
	vSamples0123.w	= SampleDepthBuffer(tShadowMap, vTexCoord + 1.0f * vPixelOffset);
	vSamples4___.x	= SampleDepthBuffer(tShadowMap, vTexCoord + 2.0f * vPixelOffset);

	float	fDepth	= dot(vSamples0123, float4(1.0f / 16.0f, 4.0f / 16.0f, 6.0f / 16.0f, 4.0f / 16.0f)) + vSamples4___.x * (1.0f / 16.0f);
	
	vSamples0123	= vSamples0123 * vSamples0123;
	vSamples4___	= vSamples4___ * vSamples4___;

	float	fDepth2	= dot(vSamples0123, float4(1.0f / 16.0f, 4.0f / 16.0f, 6.0f / 16.0f, 4.0f / 16.0f)) + vSamples4___.x * (1.0f / 16.0f);

	return float4(fDepth, fDepth2, 0.0f, 0.0f);
}

float4 VarianceGaussianBlurFromDepth_7Taps(sampler2D tShadowMap, float2 vTexCoord, float2 vPixelOffset)
{
	float4	vSamples0123;
	float3	vSamples456_;
	vSamples0123.x	= SampleDepthBuffer(tShadowMap, vTexCoord - 3.0f * vPixelOffset);
	vSamples0123.y	= SampleDepthBuffer(tShadowMap, vTexCoord - 2.0f * vPixelOffset);
	vSamples0123.z	= SampleDepthBuffer(tShadowMap, vTexCoord - 1.0f * vPixelOffset);
	vSamples0123.w	= SampleDepthBuffer(tShadowMap, vTexCoord - 0.0f * vPixelOffset);
	vSamples456_.x	= SampleDepthBuffer(tShadowMap, vTexCoord + 1.0f * vPixelOffset);
	vSamples456_.y	= SampleDepthBuffer(tShadowMap, vTexCoord + 2.0f * vPixelOffset);
	vSamples456_.z	= SampleDepthBuffer(tShadowMap, vTexCoord + 3.0f * vPixelOffset);

	float	fDepth	= dot(vSamples0123, float4(1.0f / 64.0f, 6.0f / 64.0f, 15.0f / 64.0f, 20.0f / 64.0f))
					+ dot(vSamples456_.xyz, float3(15.0f / 64.0f, 6.0f / 64.0f, 1.0f / 64.0f));
	
	vSamples0123	= vSamples0123 * vSamples0123;
	vSamples456_	= vSamples456_ * vSamples456_;

	float	fDepth2	= dot(vSamples0123, float4(1.0f / 64.0f, 6.0f / 64.0f, 15.0f / 64.0f, 20.0f / 64.0f))
					+ dot(vSamples456_.xyz, float3(15.0f / 64.0f, 6.0f / 64.0f, 1.0f / 64.0f));

	return float4(fDepth, fDepth2, 0.0f, 0.0f);
}

#if defined(XB2)

float4 VarianceGaussianBlurXFromDepth_5Taps(sampler2D tShadowMap, float2 vTexCoord)
{
	float4	vSamples0123;
	float	vSamples4___;
    asm
    {
        tfetch2D vSamples0123.x___, vTexCoord, tShadowMap, OffsetX = -2.0, MinFilter=point, MagFilter=point
        tfetch2D vSamples0123._x__, vTexCoord, tShadowMap, OffsetX = -1.0, MinFilter=point, MagFilter=point
        tfetch2D vSamples0123.__x_, vTexCoord, tShadowMap, OffsetX = -0.0, MinFilter=point, MagFilter=point
        tfetch2D vSamples0123.___x, vTexCoord, tShadowMap, OffsetX = +1.0, MinFilter=point, MagFilter=point
        tfetch2D vSamples4___.x___, vTexCoord, tShadowMap, OffsetX = +2.0, MinFilter=point, MagFilter=point
    };

	float	fDepth	= dot(vSamples0123, float4(1.0f / 16.0f, 4.0f / 16.0f, 6.0f / 16.0f, 4.0f / 16.0f)) + vSamples4___.x * (1.0f / 16.0f);
	
	vSamples0123	= vSamples0123 * vSamples0123;
	vSamples4___	= vSamples4___ * vSamples4___;

	float	fDepth2	= dot(vSamples0123, float4(1.0f / 16.0f, 4.0f / 16.0f, 6.0f / 16.0f, 4.0f / 16.0f)) + vSamples4___.x * (1.0f / 16.0f);

	return float4(fDepth, fDepth2, 0.0f, 0.0f);
}

float4 VarianceGaussianBlurXFromDepth_7Taps(sampler2D tShadowMap, float2 vTexCoord)
{
	float4	vSamples0123;
	float3	vSamples456_;
    asm
    {
        tfetch2D vSamples0123.x___, vTexCoord, tShadowMap, OffsetX = -3.0, MinFilter=point, MagFilter=point
        tfetch2D vSamples0123._x__, vTexCoord, tShadowMap, OffsetX = -2.0, MinFilter=point, MagFilter=point
        tfetch2D vSamples0123.__x_, vTexCoord, tShadowMap, OffsetX = -1.0, MinFilter=point, MagFilter=point
        tfetch2D vSamples0123.___x, vTexCoord, tShadowMap, OffsetX = -0.0, MinFilter=point, MagFilter=point
        tfetch2D vSamples456_.x___, vTexCoord, tShadowMap, OffsetX = +1.0, MinFilter=point, MagFilter=point
        tfetch2D vSamples456_._x__, vTexCoord, tShadowMap, OffsetX = +2.0, MinFilter=point, MagFilter=point
        tfetch2D vSamples456_.__x_, vTexCoord, tShadowMap, OffsetX = +3.0, MinFilter=point, MagFilter=point
    };

	float	fDepth	= dot(vSamples0123, float4(1.0f / 64.0f, 6.0f / 64.0f, 15.0f / 64.0f, 20.0f / 64.0f))
					+ dot(vSamples456_.xyz, float3(15.0f / 64.0f, 6.0f / 64.0f, 1.0f / 64.0f));
	
	vSamples0123	= vSamples0123 * vSamples0123;
	vSamples456_	= vSamples456_ * vSamples456_;

	float	fDepth2	= dot(vSamples0123, float4(1.0f / 64.0f, 6.0f / 64.0f, 15.0f / 64.0f, 20.0f / 64.0f))
					+ dot(vSamples456_.xyz, float3(15.0f / 64.0f, 6.0f / 64.0f, 1.0f / 64.0f));

	return float4(fDepth, fDepth2, 0.0f, 0.0f);
}

float4 VarianceGaussianBlurY_5Taps(sampler2D tShadowMap, float2 vTexCoord)
{
	float4	vSamples01;
	float4	vSamples23;
    asm
    {
        tfetch2D vSamples01.xy__, vTexCoord, tShadowMap, OffsetY = +1.5, MinFilter=linear, MagFilter=linear
        tfetch2D vSamples01.__xy, vTexCoord, tShadowMap, OffsetY = +0.5, MinFilter=linear, MagFilter=linear
        tfetch2D vSamples23.xy__, vTexCoord, tShadowMap, OffsetY = -0.5, MinFilter=linear, MagFilter=linear
        tfetch2D vSamples23.__xy, vTexCoord, tShadowMap, OffsetY = -1.5, MinFilter=linear, MagFilter=linear
    };

	vSamples01		= vSamples01 * float4(2.0f, 2.0f, 6.0f, 6.0f) / 16.0f;
	vSamples23		= vSamples23 * float4(6.0f, 6.0f, 2.0f, 2.0f) / 16.0f;

	float4	vSamples0123	= vSamples01 + vSamples23;
    
	return float4(vSamples0123.xy + vSamples0123.zw, 0.0f, 0.0f);
}

float4 VarianceGaussianBlurY_7Taps(sampler2D tShadowMap, float2 vTexCoord)
{
	float4	vSamples01;
	float4	vSamples23;
	float4	vSamples45;
    asm
    {
        tfetch2D vSamples01.xy__, vTexCoord, tShadowMap, OffsetY = -2.5, MinFilter=linear, MagFilter=linear
        tfetch2D vSamples01.__xy, vTexCoord, tShadowMap, OffsetY = -1.5, MinFilter=linear, MagFilter=linear
        tfetch2D vSamples23.xy__, vTexCoord, tShadowMap, OffsetY = -0.5, MinFilter=linear, MagFilter=linear
        tfetch2D vSamples23.__xy, vTexCoord, tShadowMap, OffsetY = +0.5, MinFilter=linear, MagFilter=linear
        tfetch2D vSamples45.xy__, vTexCoord, tShadowMap, OffsetY = +1.5, MinFilter=linear, MagFilter=linear
        tfetch2D vSamples45.__xy, vTexCoord, tShadowMap, OffsetY = +2.5, MinFilter=linear, MagFilter=linear
    };

	vSamples01		= vSamples01 * float4(2.0f, 2.0f, 10.0f, 10.0f) / 64.0f;
	vSamples23		= vSamples23 * float4(20.0f, 20.0f, 20.0f, 20.0f) / 64.0f;
	vSamples45		= vSamples45 * float4(10.0f, 10.0f, 2.0f, 2.0f) / 64.0f;

	float4	vSamples012345	= vSamples01 + vSamples23 + vSamples45;
    
	return float4(vSamples012345.xy + vSamples012345.zw, 0.0f, 0.0f);
}

#endif
