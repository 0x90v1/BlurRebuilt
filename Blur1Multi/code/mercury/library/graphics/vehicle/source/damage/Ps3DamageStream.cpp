// [Blur1Multi] PS3 sources: the vehicle body's stream 3. See Ps3DamageStream.h.
//
// Line numbers "SPUDamageJob.cpp:n" refer to Blur1PC/code/mercury/library/graphics/
// vehicle/source/damage/SPU/SPUDamageJob.cpp, the SPU job this ports.
//
// Floating point: the SPU single-precision unit rounds every result towards zero and
// treats denormals as zero (Cell Broadband Engine Programming Handbook, SPU floating
// point). Run() sets that rounding (and denormal flushing) for its duration, so the
// scalar arithmetic below rounds as the job's did. Not reproduced: the SPU produces no
// infinities or NaNs (it saturates), and the Maths library's Normalised() on the SPU
// uses the reciprocal square root estimate with Newton-Raphson (here 1/sqrtf); both
// can differ in the last bits, which the F16 store usually hides.

#ifndef PS3DAMAGESTREAM_CORE_ONLY
#include "VehiclePCH.h"
#endif

#include "Ps3DamageStream.h"

#include <float.h>
#include <math.h>
#include <string.h>

namespace Ps3DamageJob
{
	namespace
	{
		float FromBits(U32x b) { float f; memcpy(&f, &b, 4); return f; }
		U32x ToBits(float f) { U32x b; memcpy(&b, &f, 4); return b; }

		// A double rounded to float towards zero (the SPU's conversions and FMA).
		float RoundTowardZero(double d)
		{
			float f = (float)d;
			if ((double)f != d && fabs((double)f) > fabs(d))
				f = nextafterf(f, 0.0f);
			return f;
		}

		U16x Half(const U8x* p) { return (U16x)(p[0] | (p[1] << 8)); }	// converted buffers are little-endian

		struct Vec4 { float x, y, z, w; };

		// load_unaligned_float16_4 (SPUDamageJob.cpp:64-105), four lanes.
		Vec4 LoadF16_4(const U8x* p)
		{
			Vec4 v;
			v.x = LoadFloat16(Half(p));
			v.y = LoadFloat16(Half(p + 2));
			v.z = LoadFloat16(Half(p + 4));
			v.w = LoadFloat16(Half(p + 6));
			return v;
		}

		// load_unaligned_SSHD_3N on the SHHD_3N word the converted F16_4 came from.
		Vec4 LoadSshd(const U8x* p)
		{
			U16x h[4];
			for (int i = 0; i < 4; ++i)
				h[i] = Half(p + 2 * i);
			float f[4];
			DecodeSshd3n(ShhdFromF16_4(h), f);
			Vec4 v = { f[0], f[1], f[2], f[3] };
			return v;
		}

		float Clamp(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }	// Maths::Clamp

		// IEEE half -> float, exact (to read back what the model converter stored).
		float LoadFloat16Ieee(U16x h)
		{
			const int e = (h >> 10) & 0x1f, m = h & 0x3ff;
			double v = e == 0 ? ldexp((double)m, -24) : ldexp((double)(m | 0x400), e - 25);
			return (float)((h & 0x8000) ? -v : v);
		}
	}

	float LoadFloat16(U16x h)
	{
		// sign << 16; (h & 0x7fff) << 13; + 0x38000000 when that is non-zero (:92-100).
		const U32x sign = (U32x)(h & 0x8000) << 16;
		const U32x significand = (U32x)(h & 0x7fff) << 13;
		const U32x bias = ((int)significand > 0) ? 0x38000000u : 0u;
		return FromBits((significand + bias) | sign);
	}

	U16x StoreFloat16(float f)
	{
		// Edge's F32 -> F16 (:125-148), one lane: the high 16 bits give the magnitude
		// tests; below 0x3800 (2^-15) the result is 0, from 0x4780 (2^16) up it is the
		// largest half 0x7BFF; else ((bits - 0x38000000) rotated left 3)'s high half, with
		// the input's sign.
		const U32x bits = ToBits(f);
		const U16x in = (U16x)(bits >> 16);
		const U16x nz = in & 0x7fff;
		if (nz < 0x3800)										// m01 = expMin > nz: result cleared
			return 0;
		U16x r;
		if (nz < 0x4780)										// x01 = expMax > nz
		{
			const U32x t = bits - 0x38000000u;
			const U32x rot = (t << 3) | (t >> 29);
			r = (U16x)(rot >> 16);
		}
		else
			r = 0x7BFF;
		return (U16x)((r & 0x7fff) | (in & 0x8000));			// spu_sel(r01, in01, signMask)
	}

	void DecodeSshd3n(U32x word, float out[4])
	{
		// (word << {21, 10, 0}) & {0xFFE00000, 0xFFE00000, 0xFFC00000}, as signed / 2^31,
		// then * {0x3F801002, 0x3F801002, 0x3F802008, 0} + {0x3A001002, 0x3A001002, 0x3A802008, 1.0}
		// in one fused multiply-add (:212-243).
		static const U32x shift[3] = { 21, 10, 0 };
		static const U32x mask[3] = { 0xFFE00000u, 0xFFE00000u, 0xFFC00000u };
		static const U32x scale[3] = { 0x3F801002u, 0x3F801002u, 0x3F802008u };
		static const U32x bias[3] = { 0x3A001002u, 0x3A001002u, 0x3A802008u };
		for (int i = 0; i < 3; ++i)
		{
			const int v = (int)((word << shift[i]) & mask[i]);
			const double f = (double)v / 2147483648.0;			// exact: at most 11 significant bits
			out[i] = RoundTowardZero(f * (double)FromBits(scale[i]) + (double)FromBits(bias[i]));
		}
		out[3] = 1.0f;
	}

	U32x ShhdFromF16_4(const U16x* halves)
	{
		// The converter wrote (x / 1023, y / 1023, z / 511, 1) (SceneTranscode.cpp ExpandShhd);
		// consecutive x differ by 1/1023, more than two F16 steps (2^-11 below 1), so the
		// nearest integer is the original.
		const float x = LoadFloat16Ieee(halves[0]), y = LoadFloat16Ieee(halves[1]), z = LoadFloat16Ieee(halves[2]);
		const int ix = (int)floor((double)x * 1023.0 + 0.5);
		const int iy = (int)floor((double)y * 1023.0 + 0.5);
		const int iz = (int)floor((double)z * 511.0 + 0.5);
		return ((U32x)ix & 0x7ff) | (((U32x)iy & 0x7ff) << 11) | (((U32x)iz & 0x3ff) << 22);
	}

	float Saturate(float f)
	{
		// si_cfltu(v, 32) then si_cuflt(v, 32) (:167-175): to unsigned 0.32 fixed point,
		// saturating and truncating, and back, rounding towards zero.
		if (!(f > 0.0f))
			return 0.0f;
		const double scaled = (double)f * 4294967296.0;
		const double fixed = scaled >= 4294967295.0 ? 4294967295.0 : floor(scaled);
		return RoundTowardZero(fixed / 4294967296.0);
	}

	void Run(const Input& in, const Buffer* buffers, int bufferCount)
	{
		if (in.noDeformation)		// SPUDamageJob.cpp:667: the vertex loop only runs with deformation
			return;

		unsigned int oldControl = 0;
		_controlfp_s(&oldControl, 0, 0);
		unsigned int unused;
		_controlfp_s(&unused, _RC_CHOP | _DN_FLUSH, _MCW_RC | _MCW_DN);

		const Node* const nodes = in.nodes;
		const float* const b = in.bounds;
		for (int iBuffer = 0; iBuffer < bufferCount; ++iBuffer)
		{
			const Buffer& buf = buffers[iBuffer];
			if (!buf.input || !buf.output)		// :674-676
				continue;
			for (U32x iVertex = 0; iVertex < buf.count; ++iVertex)
			{
				const U8x* const addr = buf.input + iVertex * buf.stride;

				// :718-728: position (F16_4) and normal (SSHD_3N), w = 0.
				const Vec4 rawPos = LoadF16_4(addr + buf.position);
				float px = rawPos.x, py = rawPos.y, pz = rawPos.z, pw = 0.0f;
				const Vec4 rawNormal = LoadSshd(addr + buf.normal);
				float nx = rawNormal.x, ny = rawNormal.y, nz = rawNormal.z, nw = 0.0f;

				// GetDamageUV (:177-184).
				const float u = Clamp((px + b[0] / 2.0f) / b[0], 0.0f, 1.0f);
				const float v = Clamp((pz + b[2] / 2.0f) / b[2], 0.0f, 1.0f);

				// EvaluateDamageInfoTexture (:186-217).
				float infoX, infoY;
				{
					const U32x iu = (U32x)(int)(u * 7.f);
					const U32x iv = (U32x)(int)(v * 7.f);
					const U32x i = (iv << 3) + iu;
					const U32x j = (iv << 3) + ((iu + 1) % 7);
					const float yVal = (v - ((float)(iv) / 7.f)) / (((float)((iv + 1) % 7) / 7.f) - ((float)(iv) / 7.f));
					float maxDeform = 0.0f;
					for (int iLayer = 0; iLayer < kLayers; iLayer++)
					{
						const float a = nodes[iLayer * kNodesPerLayer + i].deformLevel;
						const float c = nodes[iLayer * kNodesPerLayer + j].deformLevel;
						const float level = a * (1 - yVal) + c * (yVal);
						if (level > maxDeform)
							maxDeform = level;
					}
					infoY = maxDeform;
					infoX = Clamp((py / (in.heightProfile[iv < 8 ? iv : 7] * b[1])), 0.0f, 1.0f);
				}

				// EvaluateVertexDamage (u, v, infoX) (:279-331).
				float dx, dy, dz, dw;
				{
					const U32x x_dim = 7, y_dim = 7, z_dim = 2;
					const float x = u * (float)x_dim;
					const float y = v * (float)y_dim;
					const float z = infoX * (float)z_dim;
					int ix = (int)floor(x);
					int iy = (int)floor(y);
					int iz = (int)floor(z);
					const float fx = x - (float)ix;
					const float fy = y - (float)iy;
					const float fz = z - (float)iz;
					const float omfx = 1.0f - fx;
					const float omfy = 1.0f - fy;
					const float omfz = 1.0f - fz;
					ix = (ix < 0) ? 0 : ((ix > (int)x_dim) ? (int)x_dim : ix);
					iy = (iy < 0) ? 0 : ((iy > (int)y_dim) ? (int)y_dim : iy);
					iz = (iz < 0) ? 0 : ((iz > (int)z_dim) ? (int)z_dim : iz);
					const U32x ixp1 = ((ix + 1) < 0) ? 0 : (((ix + 1) > (int)x_dim) ? x_dim : (U32x)(ix + 1));
					const U32x iyp1 = ((iy + 1) < 0) ? 0 : (((iy + 1) > (int)y_dim) ? y_dim : (U32x)(iy + 1));
					const U32x izp1 = ((iz + 1) < 0) ? 0 : (((iz + 1) > (int)z_dim) ? z_dim : (U32x)(iz + 1));
					const U32x cx[2] = { (U32x)ix, ixp1 }, cy[2] = { (U32x)iy, iyp1 }, cz[2] = { (U32x)iz, izp1 };
					// Corner order and weights as written (:309-325).
					static const int corner[8][3] = { {0,0,0}, {1,0,0}, {0,1,0}, {1,1,0}, {0,0,1}, {1,0,1}, {0,1,1}, {1,1,1} };
					const float wx[2] = { omfx, fx }, wy[2] = { omfy, fy }, wz[2] = { omfz, fz };
					float rx = 0, ry = 0, rz = 0, scratch = 0.f;
					for (int k = 0; k < 8; ++k)
					{
						const Node& n = nodes[cz[corner[k][2]] * kNodesPerLayer + (cy[corner[k][1]] << 3) + cx[corner[k][0]]];
						const float weight = wx[corner[k][0]] * wy[corner[k][1]] * wz[corner[k][2]];
						rx = rx + n.offset[0] * weight;
						ry = ry + n.offset[1] * weight;
						rz = rz + n.offset[2] * weight;
					}
					for (int k = 0; k < 8; ++k)
					{
						const Node& n = nodes[cz[corner[k][2]] * kNodesPerLayer + (cy[corner[k][1]] << 3) + cx[corner[k][0]]];
						const float weight = wx[corner[k][0]] * wy[corner[k][1]] * wz[corner[k][2]];
						scratch = scratch + n.scratchLevel * weight;
					}
					dx = rx; dy = ry; dz = rz; dw = scratch;
				}

				// Control-point deformation (:733-735, ControlPointDamage :246-257).
				if (buf.controlPoint != kNone)
				{
					const U8x* cp = addr + buf.controlPoint;
					const float* t1 = &in.controlPoints[cp[0] * 4 < kControlPointFloats ? cp[0] * 4 : 0];
					const float* t2 = &in.controlPoints[cp[1] * 4 < kControlPointFloats ? cp[1] * 4 : 0];
					const float weight = (float)cp[2] / 254.0f;
					dx += t1[0] + (t2[0] - t1[0]) * weight;
					dy += t1[1] + (t2[1] - t1[1]) * weight;
					dz += t1[2] + (t2[2] - t1[2]) * weight;
					dw += 0.0f;			// blendTgt.W(0)
				}

				// Morph target deformation (:737-750).
				if (buf.positionOffset != kNone)
				{
					Vec4 posOffset = LoadSshd(addr + buf.positionOffset);
					posOffset.w = 0.0f;
					Vec4 normalOffset = LoadSshd(addr + buf.normalOffset);
					normalOffset.w = 0.0f;
					dx += posOffset.x * infoY;
					dy += posOffset.y * infoY;
					dz += posOffset.z * infoY;
					dw += posOffset.w * infoY;
					nx = nx + normalOffset.x * infoY;
					ny = ny + normalOffset.y * infoY;
					nz = nz + normalOffset.z * infoY;
					nw = nw + normalOffset.w * infoY;
					const float inv = 1.0f / sqrtf(nx * nx + ny * ny + nz * nz + nw * nw);	// Normalised()
					nx *= inv; ny *= inv; nz *= inv; nw *= inv;
				}

				// :752-754.
				px += dx; py += dy; pz += dz; pw += dw;
				pw = pw * 2.f;

				// CalculateRearLightAttenuation (:259-265) with the colour's first byte (:757).
				const U8x ao = buf.colour != kNone ? addr[buf.colour] : 0;
				const float rearLightFactor = Saturate(Saturate(-nz) - 0.6f) / (1.f - 0.6f);
				const float rearLightFactor2 = rearLightFactor * rearLightFactor * Saturate(-(pz * 0.5f));
				const U32x iRearLightAttenuation = (U32x)(rearLightFactor2 * (float)ao);

				// :759-760.
				nw = (0.996f < infoY ? 0.996f : infoY) + (float)iRearLightAttenuation;

				// float32_to_float16(localPos, localNormal) (:762).
				U8x* out = buf.output + iVertex * kOutputStride;
				const float lanes[8] = { px, py, pz, pw, nx, ny, nz, nw };
				for (int k = 0; k < 8; ++k)
				{
					const U16x h = StoreFloat16(lanes[k]);
					out[2 * k] = (U8x)h;
					out[2 * k + 1] = (U8x)(h >> 8);
				}
			}
		}

		_controlfp_s(&unused, oldControl, _MCW_RC | _MCW_DN);
	}
}

#ifndef PS3DAMAGESTREAM_CORE_ONLY

#include "Vehicle/VehicleModel.h"
#include "Vehicle/Damage/DamageLayer.h"
#include "Vehicle/Damage/NodeLayer.h"
#include "Vehicle/Damage/Node.h"
#include "world_render/ModelInstance.h"
#include <SceneGraph/renderingdata/RenderListNode.h>
#define RENDERINGDATA_ALLOW_INCLUDE
#include <renderingdata/renderingdata.h>
#undef RENDERINGDATA_ALLOW_INCLUDE

#include <gamesource/GameSource.h>

#include <map>
#include <mutex>
#include <set>
#include <vector>

namespace Ps3DamageStream
{
	namespace
	{
		Debug::Logger& g_Log = Debug::GetLogger("mercury.vehicle.damage");

		// Per body vertex buffer (index in the render list node).
		struct ModelStreams
		{
			std::vector<Vehicle::C_VehicleVertexBuffer*> outputs;	// m_damageOffsets (VehicleModel.h:389-392)
			std::vector<std::vector<Ps3DamageJob::U8x> > inputs;	// the body buffer bytes, read once
		};

		// SpuDamage::C_DamageContext: the fields the vertex loop and the scheduling use.
		struct Context
		{
			bool initialised, forceRun, forceRunThisFrame;
			std::vector<Ps3DamageJob::Buffer> buffers;	// one per used vertex buffer (DamageLayer.cpp:230-245)
			std::vector<int> bufferIndex;				// render-list vertex buffer of each
			float bounds[4];
			const float* pControlPointData;
			Context() : initialised(false), forceRun(false), forceRunThisFrame(false), pControlPointData(NULL)
			{
				bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0f;
			}
		};

		std::mutex g_lock;
		std::map<const Vehicle::C_VehicleModel*, ModelStreams> g_models;
		std::map<const Vehicle::C_DamageLayer*, Context> g_contexts;
		std::set<const Vehicle::C_DamageLayer*> g_updatedThisFrame;
		unsigned g_runThisFrame = 0;
		const unsigned kMaxPerFrame = 1;		// SPUDamageContext.cpp:11
		const unsigned kMaxVertexBuffers = 128;	// SpuDamage::maxNumberOfVertexBuffers (SPURepairContext.h:6)

		Context* Find(const Vehicle::C_DamageLayer* pLayer)
		{
			std::map<const Vehicle::C_DamageLayer*, Context>::iterator it = g_contexts.find(pLayer);
			return it == g_contexts.end() ? NULL : &it->second;
		}
	}

	bool Active()
	{
		return GameSource::Current().m_Platform == GameSource::E_Platform_PS3;
	}

	void CreateStreams(Vehicle::C_VehicleModel* pModel)
	{
		WorldRender::C_ModelInstance* pInstance = pModel->GetModelInstance(0);
		if (!pInstance)
			return;
		const RenderingData::Target::RenderListNode* pNode = pInstance->GetRenderListNode();
		const U32 iNumberOfVertexBuffers = pNode->GetNumVertexBuffers();

		ModelStreams streams;
		streams.outputs.resize(iNumberOfVertexBuffers, NULL);
		streams.inputs.resize(iNumberOfVertexBuffers);

		// One stream-3 primitive stream per body primitive (VehicleModel.cpp:264-293).
		for (U32 iCurrentPrimitive = 0; iCurrentPrimitive < (U32)pNode->GetNumPrimitives(); ++iCurrentPrimitive)
		{
			const S16 iVertexBufferIndex = pNode->GetPrimitive(iCurrentPrimitive).primitiveStreams[0].vertexBuffer;
			const S32 iVertexBufferOffset = pNode->GetPrimitive(iCurrentPrimitive).primitiveStreams[0].vertexOffset;
			const S32 iVertexStride = pNode->GetVertexBuffers(iVertexBufferIndex).vertexSize;
			RenderingData::Target::PrimitiveStream stream;
			stream.streamIndex = 3;
			stream.vertexBuffer = iVertexBufferIndex;
			stream.vertexOffset = (iVertexBufferOffset / iVertexStride) << 4;
			pInstance->AddDynamicPrimitiveStreams(stream);
		}

		// One 16-byte-per-vertex buffer per body vertex buffer (VehicleModel.cpp:295-349),
		// initialised as the PS3 did: x = y = z = 0.0f and the U32 0x80000000, i.e. as the
		// F16_4 pair the RSX reads, halves 0, 0, 0, 0, 0, 0, 0x8000, 0.
		for (U32 iCurrentVertexBuffer = 0; iCurrentVertexBuffer < iNumberOfVertexBuffers; ++iCurrentVertexBuffer)
		{
			const RenderingData::Target::VertexBufferData& vbInfo = pNode->GetVertexBuffers(iCurrentVertexBuffer);
			RenderingData::Target::VertexBufferData vbData;
			vbData.pBuffer = NULL;
			vbData.vertexSize = 0;
			if (vbInfo.pBuffer && vbInfo.vertexSize > 0)
			{
				const U32 iSize = (U32)vbInfo.pBuffer->GetSize();
				const U32 iStride = (U32)vbInfo.vertexSize;
				const U32 iNumberOfVerts = iSize / iStride;
				Vehicle::C_VehicleVertexBuffer* pBuffer = Vehicle::C_VehicleVertexBufferManager::GetInstance().CreateVertexBuffer(iNumberOfVerts * 16);
				if (pBuffer)
				{
					U8* p = (U8*)pBuffer->m_Data.Lock();
					if (p)
					{
						memset(p, 0, iNumberOfVerts * 16);
						for (U32 v = 0; v < iNumberOfVerts; ++v)
							p[16 * v + 13] = 0x80;		// half 6 = 0x8000, little-endian
						pBuffer->m_Data.Unlock();
					}
					streams.outputs[iCurrentVertexBuffer] = pBuffer;
					vbData.pBuffer = &pBuffer->m_Data;
					vbData.vertexSize = 0x10;

					// The job reads the body buffer: keep its bytes (managed pool, read once).
					Display2::C_VertexBuffer* pSource = const_cast<Display2::C_VertexBuffer*>(vbInfo.pBuffer);
					const U8* pIn = (const U8*)pSource->Lock(0, 0, D3DLOCK_READONLY);
					if (pIn)
					{
						streams.inputs[iCurrentVertexBuffer].assign(pIn, pIn + iNumberOfVerts * iStride);
						pSource->Unlock();
					}
				}
				else
					LOGERROR(g_Log, "PS3 damage stream: failed to create the stream-3 buffer of %u vertices\n", iNumberOfVerts);
			}
			pInstance->AddDynamicVertexBuffer(vbData);	// a dummy where there is no buffer, to match indices
		}

		std::lock_guard<std::mutex> hold(g_lock);
		g_models[pModel] = streams;
	}

	void DestroyStreams(Vehicle::C_VehicleModel* pModel)
	{
		std::lock_guard<std::mutex> hold(g_lock);
		std::map<const Vehicle::C_VehicleModel*, ModelStreams>::iterator it = g_models.find(pModel);
		if (it == g_models.end())
			return;
		for (size_t i = 0; i < it->second.outputs.size(); ++i)
			if (it->second.outputs[i])
				Vehicle::C_VehicleVertexBufferManager::GetInstance().DestroyVertexBuffer(it->second.outputs[i]);
		g_models.erase(it);
	}

	void Allocate(const Vehicle::C_DamageLayer* pLayer)
	{
		std::lock_guard<std::mutex> hold(g_lock);
		g_contexts[pLayer] = Context();
	}

	void Free(const Vehicle::C_DamageLayer* pLayer)
	{
		std::lock_guard<std::mutex> hold(g_lock);
		g_contexts.erase(pLayer);
		g_updatedThisFrame.erase(pLayer);
	}

	void SetForceRun(const Vehicle::C_DamageLayer* pLayer)
	{
		std::lock_guard<std::mutex> hold(g_lock);
		if (Context* c = Find(pLayer))
			c->forceRun = true;
	}

	void SetForceRunThisFrame(const Vehicle::C_DamageLayer* pLayer)
	{
		std::lock_guard<std::mutex> hold(g_lock);
		if (Context* c = Find(pLayer))
			c->forceRunThisFrame = true;
	}

	bool ForceRunPending(const Vehicle::C_DamageLayer* pLayer)
	{
		std::lock_guard<std::mutex> hold(g_lock);
		Context* c = Find(pLayer);
		return c && c->forceRun;
	}

	void BeginLayerUpdate(const Vehicle::C_DamageLayer* pLayer)
	{
		std::lock_guard<std::mutex> hold(g_lock);
		if (g_updatedThisFrame.count(pLayer))
		{
			g_updatedThisFrame.clear();
			g_runThisFrame = 0;		// C_ContextManager::ZeroRunJobsCount
		}
		g_updatedThisFrame.insert(pLayer);
	}

	bool IsInitialised(const Vehicle::C_DamageLayer* pLayer)
	{
		std::lock_guard<std::mutex> hold(g_lock);
		Context* c = Find(pLayer);
		return c && c->initialised;
	}

	void Init(const Vehicle::C_DamageLayer* pLayer, Vehicle::C_VehicleModel* pModel, const float* pControlPointData,
		const float bounds[4])
	{
		WorldRender::C_ModelInstance* pInstance = pModel->GetModelInstance(0);
		std::lock_guard<std::mutex> hold(g_lock);
		Context* c = Find(pLayer);
		if (!c)
			return;
		c->initialised = true;
		std::map<const Vehicle::C_VehicleModel*, ModelStreams>::iterator itModel = g_models.find(pModel);
		if (!pInstance || itModel == g_models.end())
			return;
		const RenderingData::Target::RenderListNode* pRenderList = pInstance->GetRenderListNode();
		const U32 iNumberOfVertexBuffers = pRenderList->GetNumVertexBuffers();

		// Element offsets per vertex buffer (DamageLayer.cpp:158-226). The PS3 read the
		// position and normal at fixed offsets 0 and 8 (SPUDamageJob.cpp:718, :724); the
		// converted buffers are read where their declaration puts them.
		std::vector<U32> positionOffsets(iNumberOfVertexBuffers, UINT_MAX), normalOffsets(iNumberOfVertexBuffers, UINT_MAX),
			controlPointOffsets(iNumberOfVertexBuffers, UINT_MAX), colourOffsets(iNumberOfVertexBuffers, UINT_MAX),
			vertexPositions(iNumberOfVertexBuffers, UINT_MAX), vertexNormals(iNumberOfVertexBuffers, UINT_MAX);
		std::vector<bool> usable(iNumberOfVertexBuffers, true);
		for (U32 iPrim = 0; iPrim < (U32)pRenderList->GetNumPrimitives(); iPrim++)
		{
			const RenderingData::RenderListNode::Primitive& prim = pRenderList->GetPrimitive(iPrim);
			S32 iVtxBuffer = -1;
			for (int iStream = 0; iStream < prim.primitiveStreams.Size(); iStream++)
				if (prim.primitiveStreams[iStream].streamIndex == 0)
				{
					iVtxBuffer = prim.primitiveStreams[iStream].vertexBuffer;
					break;
				}
			if (iVtxBuffer < 0 || iVtxBuffer >= (S32)iNumberOfVertexBuffers)
				continue;
			if (colourOffsets[iVtxBuffer] != UINT_MAX)
				continue;

			const Display2::C_VertexDeclaration* pVtxDecl = pRenderList->GetVertexDeclaration(prim.vertexDeclaration);
			Display2::C_VertexElement elements[Display2::C_VertexDeclaration::m_MaxElementCount + 1];
			const S32 count = pVtxDecl ? pVtxDecl->GetElementCount() : 0;
			if (count <= 0 || count > (S32)Display2::C_VertexDeclaration::m_MaxElementCount || !pVtxDecl->GetElements(count, elements))
				continue;
			for (int iElement = 0; iElement < count; iElement++)
			{
				const U8 iUsage = elements[iElement].GetUsage();
				if (iUsage == Display2::E_VertexElementUsage_TexCoord)
				{
					switch (elements[iElement].GetUsageIndex())
					{
					default:
						break;
					case 2:	positionOffsets[iVtxBuffer] = elements[iElement].GetOffset(); break;
					case 3: normalOffsets[iVtxBuffer] = elements[iElement].GetOffset(); break;
					case 4: controlPointOffsets[iVtxBuffer] = elements[iElement].GetOffset(); break;
					}
				}
				if (iUsage == Display2::E_VertexElementUsage_Colour)
				{
					colourOffsets[iVtxBuffer] = elements[iElement].GetOffset();
					break;
				}
			}
			// [Blur1Multi] position and normal of stream 0, and the types the conversion must have given.
			for (int iElement = 0; iElement < count; iElement++)
			{
				const Display2::C_VertexElement& e = elements[iElement];
				if (e.GetStream() != 0)
					continue;
				const bool f16x4 = e.GetType() == Display2::E_VertexElementType_F16_4;
				if (e.GetUsage() == Display2::E_VertexElementUsage_Position && e.GetUsageIndex() == 0)
				{
					vertexPositions[iVtxBuffer] = e.GetOffset();
					usable[iVtxBuffer] = usable[iVtxBuffer] && f16x4;
				}
				if (e.GetUsage() == Display2::E_VertexElementUsage_Normal && e.GetUsageIndex() == 0)
				{
					vertexNormals[iVtxBuffer] = e.GetOffset();
					usable[iVtxBuffer] = usable[iVtxBuffer] && f16x4;
				}
				if (e.GetUsage() == Display2::E_VertexElementUsage_TexCoord && (e.GetUsageIndex() == 2 || e.GetUsageIndex() == 3))
					usable[iVtxBuffer] = usable[iVtxBuffer] && f16x4;
			}
		}

		// DamageLayer.cpp:229-245, and the output buffers of FinaliseSPUDamageJob (:785-801).
		ModelStreams& streams = itModel->second;
		c->buffers.clear();
		c->bufferIndex.clear();
		const U32 nVtxBuffers = iNumberOfVertexBuffers > kMaxVertexBuffers ? kMaxVertexBuffers : iNumberOfVertexBuffers;
		for (U32 iCurrentVB = 0; iCurrentVB < nVtxBuffers; ++iCurrentVB)
		{
			const RenderingData::Target::VertexBufferData& vbInfo = pRenderList->GetVertexBuffers(iCurrentVB);
			if (!vbInfo.pBuffer)
				continue;
			Ps3DamageJob::Buffer b;
			memset(&b, 0, sizeof(b));
			const std::vector<Ps3DamageJob::U8x>& bytes = streams.inputs[iCurrentVB];
			const bool haveElements = vertexPositions[iCurrentVB] != UINT_MAX && vertexNormals[iCurrentVB] != UINT_MAX &&
				(positionOffsets[iCurrentVB] == UINT_MAX) == (normalOffsets[iCurrentVB] == UINT_MAX);
			if (!usable[iCurrentVB] || !haveElements)
				LOGERROR(g_Log, "PS3 damage stream: body vertex buffer %u has no F16_4 position/normal as converted; not deformed\n", iCurrentVB);
			if (!bytes.empty() && streams.outputs[iCurrentVB] && usable[iCurrentVB] && haveElements)
			{
				b.input = &bytes[0];
				b.stride = (U32)vbInfo.vertexSize;
				b.count = (U32)bytes.size() / b.stride;
				b.position = vertexPositions[iCurrentVB];
				b.normal = vertexNormals[iCurrentVB];
				b.positionOffset = positionOffsets[iCurrentVB];
				b.normalOffset = normalOffsets[iCurrentVB];
				b.controlPoint = controlPointOffsets[iCurrentVB];
				b.colour = colourOffsets[iCurrentVB];
			}
			c->buffers.push_back(b);
			c->bufferIndex.push_back((int)iCurrentVB);
		}
		for (int i = 0; i < 4; ++i)
			c->bounds[i] = bounds[i];
		c->pControlPointData = pControlPointData;
		c->forceRun = true;				// DamageLayer.cpp:255-256
		c->forceRunThisFrame = false;
	}

	bool Finalise(const Vehicle::C_DamageLayer* pLayer, Vehicle::C_VehicleModel* pModel,
		Vehicle::C_NodeLayer* pLayers, const float* pHeightProfile, size_t heightProfileSize)
	{
		std::vector<Ps3DamageJob::Buffer> buffers;
		Ps3DamageJob::Input in;
		std::vector<Ps3DamageJob::Node> nodes;
		{
			std::lock_guard<std::mutex> hold(g_lock);
			Context* c = Find(pLayer);
			if (!c || !c->forceRun)
				return false;

			// C_ContextManager::FinaliseContext (SPUDamageContext.cpp:56-65).
			if (!(g_runThisFrame < kMaxPerFrame || c->forceRunThisFrame))
				return true;
			c->forceRun = false;
			c->forceRunThisFrame = false;
			g_runThisFrame++;

			std::map<const Vehicle::C_VehicleModel*, ModelStreams>::iterator itModel = g_models.find(pModel);
			if (itModel == g_models.end() || !pLayers)
				return true;
			buffers = c->buffers;
			for (size_t i = 0; i < buffers.size(); ++i)
			{
				Vehicle::C_VehicleVertexBuffer* pOut = itModel->second.outputs[c->bufferIndex[i]];
				buffers[i].output = (pOut && buffers[i].input) ? (Ps3DamageJob::U8x*)pOut->m_Data.Lock() : NULL;
			}
			memcpy(in.bounds, c->bounds, sizeof(in.bounds));
			in.controlPoints = c->pControlPointData;
		}

		// The lattice as the job's DMA took it (damageLayerPtr, DamageLayer.cpp:785; C_NodeLayer x 3).
		nodes.resize(Ps3DamageJob::kLayers * Ps3DamageJob::kNodesPerLayer);
		for (int l = 0; l < Ps3DamageJob::kLayers; ++l)
			for (int n = 0; n < Ps3DamageJob::kNodesPerLayer; ++n)
			{
				const Vehicle::C_Node& src = pLayers[l].m_Nodes[n];
				Ps3DamageJob::Node& dst = nodes[l * Ps3DamageJob::kNodesPerLayer + n];
				dst.deformLevel = src.m_DeformLevel;
				dst.offset[0] = src.m_OffsetVector.X();
				dst.offset[1] = src.m_OffsetVector.Y();
				dst.offset[2] = src.m_OffsetVector.Z();
				dst.scratchLevel = src.m_ScratchLevel;
			}
		in.nodes = &nodes[0];
		for (int i = 0; i < 8; ++i)
			in.heightProfile[i] = (pHeightProfile && (size_t)i < heightProfileSize) ? pHeightProfile[i] : 0.0f;
		float controlPoints[Ps3DamageJob::kControlPointFloats];
		if (in.controlPoints)
			memcpy(controlPoints, in.controlPoints, sizeof(controlPoints));	// Spu::FetchAlloc, SPUDamageJob.cpp:665
		else
			memset(controlPoints, 0, sizeof(controlPoints));
		in.controlPoints = controlPoints;
		in.noDeformation = false;		// g_noSpuDeformation ("Vehicle.Damage.NoDeform") defaults to false

		Ps3DamageJob::Run(in, buffers.empty() ? NULL : &buffers[0], (int)buffers.size());

		std::lock_guard<std::mutex> hold(g_lock);
		std::map<const Vehicle::C_VehicleModel*, ModelStreams>::iterator itModel = g_models.find(pModel);
		if (itModel != g_models.end())
		{
			Context* c = Find(pLayer);
			for (size_t i = 0; c && i < buffers.size(); ++i)
				if (buffers[i].output)
					itModel->second.outputs[c->bufferIndex[i]]->m_Data.Unlock();
		}
		return true;
	}
}

#endif
