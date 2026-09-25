// [Blur1Multi] PS3 sources: the vehicle body's stream 3 (T076 follow-up).
//
// The PS3 vehicle effects (288 of the veh_* family) take the body's position and
// normal from vertex stream 3 (vertex input declaration: TANGENT and BINORMAL,
// annotation stream 3, F16_4). The PS3 build creates that stream per body vertex
// buffer (VehicleModel.cpp:255-351, 16 bytes per vertex) and fills it with the SPU
// job SpuDamage::DamageTask (SPUDamageJob.cpp:642-801), kicked from C_DamageLayer
// (InitSPUDamage DamageLayer.cpp:149-262, FinaliseSPUDamageJob :775-812) through
// SpuDamage::C_ContextManager (SPUDamageContext.cpp:43-67, one job per frame). The
// PC build has none of it and deforms in the vertex shader instead, so for a PS3
// source the body read zeros from an unbound stream and was not drawn.
//
// This unit is the PC counterpart, selected at run time for PS3 sources:
//  - the stream-3 buffers and dynamic primitive streams (CreateStreams /
//    DestroyStreams: VehicleModel.cpp:255-351 / :478-486);
//  - the per-layer damage context and its scheduling (Init, Finalise, the forceRun
//    triggers, the one-job-per-frame limit);
//  - a CPU port of DamageTask's vertex loop (Ps3DamageJob::Run), step by step from
//    the SPU source, with scalar rewrites of the SPU-only helpers
//    (load_unaligned_float16_4, load_unaligned_SSHD_3N, float32_to_float16,
//    saturate) that give their results, and the SPU's single-precision rounding
//    (towards zero, denormals flushed) during the job.
// Input vertices are the converted (PC) body buffers: the model converter writes
// every PS3 SHHD_3N element as F16_4 (x/1023, y/1023, z/511, 1); the job reads the
// SHHD_3N integers back from them (exact: neighbouring values are further apart
// than two F16 steps) and decodes them as the SPU did.
//
// The core (namespace Ps3DamageJob) is engine-independent so an offline harness can
// compile it with PS3DAMAGESTREAM_CORE_ONLY defined.

#pragma once

#include <stddef.h>

namespace Ps3DamageJob
{
	typedef unsigned char U8x;
	typedef unsigned short U16x;
	typedef unsigned int U32x;

	const U32x kNone = 0xffffffffu;			// UINT_MAX: element absent (DamageLayer.cpp:162-163)
	const U32x kOutputStride = 0x10;		// DamageLayer.cpp:799, VehicleModel.cpp:318
	const U32x kControlPointFloats = 6 * 3 * 4;	// SPUDamageJob.cpp:665 (fetch size)
	const int kLayers = 3, kNodesPerLayer = 0x40;

	// One body vertex buffer: the converted stream-0 bytes and where the job's
	// elements are (byte offsets in the vertex, kNone when absent).
	struct Buffer
	{
		const U8x* input;
		U32x stride, count;
		U32x position;			// F16_4 (the job reads it at +0: SPUDamageJob.cpp:718)
		U32x normal;			// F16_4 converted from SHHD_3N (the job reads +8: :724)
		U32x positionOffset;	// TEXCOORD2, F16_4 converted from SHHD_3N (DamageLayer.cpp:208-210)
		U32x normalOffset;		// TEXCOORD3, likewise (:211-213)
		U32x controlPoint;		// TEXCOORD4, U8_4 (:214-216)
		U32x colour;			// COLOR, U8_4N; the first byte is the ambient occlusion (:219-223)
		U8x* output;			// count * kOutputStride bytes, little-endian F16 halves
	};

	struct Node
	{
		float deformLevel;
		float offset[3];
		float scratchLevel;
	};

	struct Input
	{
		const Node* nodes;				// kLayers * kNodesPerLayer (C_NodeLayer::m_Nodes)
		float heightProfile[8];			// SPUDamageJob.cpp:655 (8 floats fetched)
		float bounds[4];				// DamageLayer.cpp:250-253
		const float* controlPoints;		// kControlPointFloats floats
		bool noDeformation;				// SPUDamageContext.cpp:59 (g_noSpuDeformation)
	};

	// DamageTask's vertex loop (SPUDamageJob.cpp:666-790) over the buffers.
	void Run(const Input& in, const Buffer* buffers, int bufferCount);

	// The SPU helpers, for the harness.
	float LoadFloat16(U16x h);						// load_unaligned_float16_4, one lane (:64-105)
	U16x StoreFloat16(float f);						// float32_to_float16, one lane (:125-148)
	void DecodeSshd3n(U32x word, float out[4]);	// load_unaligned_SSHD_3N (:212-243)
	U32x ShhdFromF16_4(const U16x* halves);			// the SHHD_3N word the converter's F16_4 came from
	float Saturate(float f);						// saturate (:167-175)
}

#ifndef PS3DAMAGESTREAM_CORE_ONLY

namespace Vehicle
{
	class C_VehicleModel;
	class C_DamageLayer;
	class C_NodeLayer;
}

namespace Ps3DamageStream
{
	// True when the running game source is a PS3 disc.
	bool Active();

	// VehicleModel.cpp:255-351: stream 3 for every body primitive, one 16-byte-per-vertex
	// buffer per body vertex buffer; the body buffers' bytes are kept for the job.
	void CreateStreams(Vehicle::C_VehicleModel* pModel);
	// VehicleModel.cpp:478-486.
	void DestroyStreams(Vehicle::C_VehicleModel* pModel);

	// C_DamageLayer's context (constructor, DamageLayer.cpp:131-143; destructor :285-293).
	void Allocate(const Vehicle::C_DamageLayer* pLayer);
	void Free(const Vehicle::C_DamageLayer* pLayer);

	// pContext->forceRun = true / forceRunThisFrame = true.
	void SetForceRun(const Vehicle::C_DamageLayer* pLayer);
	void SetForceRunThisFrame(const Vehicle::C_DamageLayer* pLayer);

	// Start of C_DamageLayer::Update: the PS3 zeroed the job count once per game-world
	// update (GameWorld.cpp:2205-2207, SPUDamageContext.cpp:126-129); here a frame starts
	// when a layer that already updated since the last start updates again.
	void BeginLayerUpdate(const Vehicle::C_DamageLayer* pLayer);

	// InitSPUDamage (DamageLayer.cpp:149-262) up to, not including, its FinaliseSPUDamageJob:
	// finds the elements in the body's declarations and sets forceRun.
	bool IsInitialised(const Vehicle::C_DamageLayer* pLayer);
	void Init(const Vehicle::C_DamageLayer* pLayer, Vehicle::C_VehicleModel* pModel, const float* pControlPointData,
		const float bounds[4]);

	// FinaliseSPUDamageJob (DamageLayer.cpp:775-812) with C_ContextManager::FinaliseContext
	// (SPUDamageContext.cpp:43-67): when forceRun is set, runs the job now unless one has
	// run this frame. Returns true when forceRun was set on entry (the caller then does
	// the PS3's "if (!forceRun) SetUpdateVolumeTexture(false)" with ForceRunPending).
	bool Finalise(const Vehicle::C_DamageLayer* pLayer, Vehicle::C_VehicleModel* pModel,
		Vehicle::C_NodeLayer* pLayers, const float* pHeightProfile, size_t heightProfileSize);
	bool ForceRunPending(const Vehicle::C_DamageLayer* pLayer);
}

#endif
