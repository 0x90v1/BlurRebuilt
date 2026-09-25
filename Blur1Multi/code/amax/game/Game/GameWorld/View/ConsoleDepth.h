// [Blur1Multi] Scene depth for console sources (T076). See ConsoleDepth.cpp.
#pragma once

struct IDirect3DDevice9;
struct IDirect3DTexture9;

namespace ConsoleDepth
{
	// Writes post-projection depth (clip z / clip w, what the PC prepass writes into its R32F
	// target 1) into 'pDst' (R32F, render target usage) from the console lighting prepass
	// target 'pGBuffer' (normal.xy, then clip w / g_vNearFar.w packed in z and w as
	// floor(d * 255) / 255 and frac(d * 255); amax/game/Shaders/Common/Pixel.h).
	// z / w = a + b / w along the camera axis (a, b from the view's camera-to-device matrix,
	// see ProjectionTerms); 'depthScale' is g_vNearFar.w. Pixels the prepass did not write
	// (still the 0xFFFFFFFF clear) get 1.0, the value the PC target's clear converts to.
	// Device state is saved and restored around the pass. False if the pass cannot run.
	bool Resolve(IDirect3DDevice9* pDevice, IDirect3DTexture9* pGBuffer, IDirect3DTexture9* pDst,
		float a, float b, float depthScale);

	// a and b of z / w = a + b / w from a row-vector camera-to-device matrix (clip = v * m;
	// display ViewSettings.cpp TransformWorldToWindow): clip w and clip z are both linear in
	// the camera-space depth coordinate, whichever axis and sign the camera looks along.
	void ProjectionTerms(const float m[4][4], float& a, float& b);
}
