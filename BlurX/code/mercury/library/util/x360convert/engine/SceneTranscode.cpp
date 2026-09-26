// [Blur1Multi] Xbox 360 and PS3 -> PC scene archive conversion. See SceneTranscode.h.
//
// Payload rules, all from the exporter in renderingdata/source/noninterface/build:
//  - PS3 (T076) writes the same grammar with PlatformType PS3, ZLIB_COMPRESSION in
//    place of XMEM (zlib-wrapped fragments, Compression::C_Deflate; the PS3 reader
//    uses the same Mercury zlib path, misc_shared compression/InflateTask.cpp), and
//    U8_4/U8_4N elements already in PC byte order: Scene::Finish(PS3) runs
//    ReverseU8_4 before the endian-swapping serialiser writes them as U32
//    (build/scenebuild.cpp, vertexbuffer.cpp VertexData::ReverseU8_4).
//  - Arch::Serialiser blocks (archive.cpp Serialise(VertexBuffer/IndexBuffer)):
//    360 writes XMEM_COMPRESSION when the payload is 128 bytes or more, PC writes
//    NO_COMPRESSION with the endian flag 0; alignment 16 on both.
//  - Vertices: each element's scalars go through the endian-swapping serialiser
//    at their own width (S16/F16 pairs and quads per 16 bits, F32 per 32 bits,
//    U8_4/U8_4N/SHHD_3N/SDDD_3N as one U32).
//  - Normals: Scene::Finish(PC) runs ChangeVertexElementType(SHHD_3N, U8_4N)
//    before anything else, i.e. PC's U8_4N normal is a function of the SHHD_3N
//    value the 360 file carries (vertexbuffer.cpp ChangeDataType).
//  - PS3 normals (T076): the PS3 effects are RSX programs translated to HLSL, which
//    read SHHD_3N as the RSX fetches it (CELL_GCM_VERTEX_CMP: signed normalised
//    11:11:10, w = 1), so every SHHD_3N element becomes F16_4 (x, y, z, 1). That
//    widens the vertex: VertexFormat DataSize/Offsets, VertexDeclaration offsets and
//    PrimitiveStream vertexOffset (bytes) move with it (ExpandShhd, PatchStreams).
//  - Indices: Scene::BuildStrips strips each index buffer lod with the ATG
//    stripper; 360/PS3 pass ATGTRISTRIPPER_USE_RESET_INDEX, which rewrites the
//    stitched strip through ConvertDegensToResetIndex (atg/AtgStrip.cpp). PC keeps
//    the stitched strip; each draw range covers one stripped lod.

#include "stdafx.h"

#include "SceneTranscode.h"
#include "KanzenTexture.h"	// [BlurX]
#include "LevelTranscode.h"

#include "renderingdata/noninterface/archive.inl"

#include <misc_shared/compression/xmemcompress.h>
#include <zlib.h>
#include <x360convert/X360Convert.h>
#include <world_render/WorldRender.h>
#include <world_render/pvs.h>

#include <algorithm>
#include <list>
#include <stdarg.h>
#include <stdio.h>

namespace Arch
{
	X360SceneTranscoder::X360SceneTranscoder(const U8* pIn, U32 inSize, U32 startAt, std::vector<U8>& out, X360Convert::SceneState& state)
		: m_in(const_cast<U8*>(pIn), inSize), m_pInBase(pIn), m_out(out), m_state(state), m_version(0), m_rawVersion(0), m_tagRawVersion(0), m_depth(0), m_failed(false), m_unconverted(false)
	{
		m_in.m_pCurBuffer += startAt;
	}

	void X360SceneTranscoder::Fail(const char* format, ...)
	{
		if (m_failed)
			return;
		char text[512];
		va_list args;
		va_start(args, format);
		vsprintf_s(text, format, args);
		va_end(args);
		char where[64];
		sprintf_s(where, " (input offset %u)", InPos());
		m_error = std::string(text) + where;
		m_failed = true;
	}

	void X360SceneTranscoder::Unconverted(const char* format, ...)
	{
		if (m_failed || m_unconverted)
			return;
		char text[512];
		va_list args;
		va_start(args, format);
		vsprintf_s(text, format, args);
		va_end(args);
		char where[64];
		sprintf_s(where, " (input offset %u)", InPos());
		m_error = std::string(text) + where;
		m_unconverted = true;
	}

	const U8* X360SceneTranscoder::InBytes(U32 n)
	{
		if (!m_failed && (U32)(m_in.m_pBufferEnd - m_in.m_pCurBuffer) < n)
			Fail("read of %u bytes past the end of the archive", n);
		if (m_failed)
		{
			if (m_zeros.size() < n + 1)
				m_zeros.assign(n + 1, 0);
			return &m_zeros[0];
		}
		const U8* p = m_in.m_pCurBuffer;
		m_in.m_pCurBuffer += n;
		return p;
	}

	U8 X360SceneTranscoder::InU8() { return *InBytes(1); }
	U16 X360SceneTranscoder::InU16() { const U8* p = InBytes(2); return (U16)((p[0] << 8) | p[1]); }
	U32 X360SceneTranscoder::InU32() { const U8* p = InBytes(4); return ((U32)p[0] << 24) | ((U32)p[1] << 16) | ((U32)p[2] << 8) | p[3]; }

	void X360SceneTranscoder::InAlign(U32 alignment)
	{
		if (alignment > 1)
		{
			U32 pos = InPos();
			InBytes(((pos + alignment - 1) / alignment) * alignment - pos);
		}
	}

	void X360SceneTranscoder::OutAlign(U32 alignment)
	{
		if (alignment > 1)
		{
			size_t pos = m_out.size();
			m_out.resize(((pos + alignment - 1) / alignment) * alignment, 0);
		}
	}

	void* X360SceneTranscoder::Read(void* pData, U32 nBytes)
	{
		const U8* p = InBytes(nBytes);
		OutBytes(p, nBytes);
		if (pData)
		{
			memcpy(pData, p, nBytes);
			return pData;
		}
		return const_cast<U8*>(p);
	}

	void X360SceneTranscoder::Align(U32 bytes)
	{
		InAlign(bytes);
		OutAlign(bytes);
	}
}

namespace X360Convert
{
	// [BlurX] Kanzen split textures. A Kanzen texture lod can be stored as N records
	// in a row in one resource block, each a Deserialiser block whose version-1
	// leading words are (part ordinal, part count) and whose compression type 3
	// segments carry rectangles absolute in the whole surface; the union tiles it.
	// Blur2Test found the same split on usa_amboy's vista01_diff (xbox_stage_decode.md
	// "The word at header + 4 is the part ordinal"; assemble_surface_strips joins
	// the parts when they tile the surface exactly, and binds part 0). Pass 1
	// gathers the parts; pass 2 writes the joined surface into part 0 and every
	// other part as Blur 1's empty lod. A group with parts missing from the archive
	// (Nov 10 dubai: vista01_diff part 0 of 4 only, Blur2Test "unresolved") is a
	// STOPGAP: above lod 0 written empty (the tier below serves), lod 0 decoded with
	// the uncovered blocks zero.
	struct KanzenSplitTexture
	{
		unsigned count;
		std::vector<unsigned char> have;
		std::vector<U8> header;
		bool consistent;
		std::string why, name;
		std::vector<KanzenSegment> segments;
		KanzenSplitTexture() : count(0), consistent(true) {}
	};

	// [BlurX] Blur 1 builds each texture lod as a new texture and copies the previous
	// lod's levels into it matched by level count (Display::D3D9 PCCopySurfaces: dst level
	// = src level + the difference of the counts; each copy is the source level's size).
	// A Kanzen lod 0 decoded short of its declared chain (the packed tail, KanzenTexture.h)
	// has fewer levels than its descriptor says, so every later lod of that texture must
	// declare as many fewer, or the pairing shifts and the copies overrun the smaller
	// surfaces (heap corruption in the D3D9 runtime, T030). The levels left out are those
	// of the undecoded tail; none is made up.
	void NoteLodTrim(Arch::X360SceneTranscoder& ar, size_t headerSizeAt, unsigned decoded)
	{
		SceneState& st = ar.State();
		const std::vector<U8>& o = ar.OutData();
		U32 declared = 0;
		if (headerSizeAt + 4 + 16 <= o.size())
			memcpy(&declared, &o[headerSizeAt + 4 + 12], 4);
		if (st.pass != 1 || declared <= decoded || st.textureIndex < 0)
			return;
		char key[32];
		sprintf_s(key, "%d/%d", st.block, st.textureIndex);
		st.kanzenLodTrim[key] = (int)(declared - decoded);
	}

	void ApplyLodTrim(Arch::X360SceneTranscoder& ar, size_t headerSizeAt)
	{
		SceneState& st = ar.State();
		if (st.kanzenLodTrim.empty() || st.textureIndex < 0 || ar.OutU16At(headerSizeAt) != 28)
			return;
		char key[32];
		sprintf_s(key, "%d/%d", st.block, st.textureIndex);
		std::map<std::string, int>::const_iterator it = st.kanzenLodTrim.find(key);
		if (it == st.kanzenLodTrim.end())
			return;
		const std::vector<U8>& o = ar.OutData();
		U32 levels = 0;
		memcpy(&levels, &o[headerSizeAt + 4 + 12], 4);
		if ((int)levels <= it->second)
		{
			ar.Unconverted("texture lod of block %d texture %d: %u levels, lod 0 left %d out", st.block, st.textureIndex, levels, it->second);
			return;
		}
		ar.PatchU32(headerSizeAt + 4 + 12, levels - it->second);
		if (st.countStats)
			++st.kanzenTrimmedLods;
	}

	// [BlurX] see SceneTranscode.h
	bool SceneTraceOn()
	{
		static const char* s_path = getenv("BLURX_SCENE_TRACE");
		return s_path && *s_path;
	}

	void SceneTrace(int depth, const char* what, unsigned raw, unsigned inPos)
	{
		static FILE* s_file = fopen(getenv("BLURX_SCENE_TRACE"), "a");
		if (s_file)
		{
			fprintf(s_file, "%*s%s v%u @%u\n", depth * 2, "", what, raw, inPos);
			fflush(s_file);	// a conversion that crashes keeps its last lines
		}
	}

	namespace
	{
		Debug::Logger& g_sceneLog = Debug::GetLogger("Blur1Multi.X360Convert");

		// An Arch::Serialiser block (serialiser.h) read from the 360 stream: its tag
		// and version have already gone to the output. Returns the logical payload.
		// [BlurX] 'kanzen' (optional): where Kanzen compression type 3 fragments go,
		// stored as they are (KanzenTexture.h); 'data' stays empty for them.
		void ReadSerialiserBlock(Arch::X360SceneTranscoder& ar, std::vector<U8>& data, U32& alignment,
			std::vector<X360Convert::KanzenSegment>* kanzen = 0)
		{
			// [BlurX] Kanzen version 1 (FUN_823b1c90) leads with two U32s (defaults
			// 0 and 1; meaning not established). Blur 1 has neither.
			ar.State().kanzenBlockWords[0] = 0;
			ar.State().kanzenBlockWords[1] = 1;
			if (ar.LastTagRawVersion() >= 1)
			{
				ar.State().kanzenBlockWords[0] = ar.InU32();
				ar.State().kanzenBlockWords[1] = ar.InU32();
			}
			const U32 size = ar.InU32();
			alignment = ar.InU32();
			const U32 compression = ar.InU32();
			ar.InU8();	// endian flag

			data.resize(size);
			if (compression == Arch::NO_COMPRESSION)
			{
				ar.InAlign(alignment);
				if (size)
					memcpy(&data[0], ar.InBytes(size), size);
			}
			else if (compression == Arch::XMEM_COMPRESSION)
			{
				const U32 count = ar.InU32();
				U32 at = 0;
				for (U32 i = 0; i < count; ++i)
				{
					const U32 originalSize = ar.InU32();
					const U32 compressedSize = ar.InU32();
					ar.InAlign(alignment);
					const U8* p = ar.InBytes(compressedSize);
					if (at + originalSize > size)
						ar.Fail("XMem fragments exceed the declared %u bytes", size);
					if (ar.GetFailed())
						return;
					const U32 got = Compression::XMemDecompress(const_cast<U8*>(p), compressedSize, &data[at], originalSize, 0);
					if (got != originalSize)
						ar.Unconverted("XMem fragment %u decoded to %u of %u bytes", i, got, originalSize);
					at += originalSize;
				}
				if (at != size)
					ar.Fail("XMem fragments give %u of %u bytes", at, size);
			}
			else if (compression == Arch::ZLIB_COMPRESSION && X360Convert::GetSource() == X360Convert::E_Source_PS3)
			{
				// the same fragment framing; each fragment a zlib stream (inflateInit)
				const U32 count = ar.InU32();
				U32 at = 0;
				for (U32 i = 0; i < count; ++i)
				{
					const U32 originalSize = ar.InU32();
					const U32 compressedSize = ar.InU32();
					ar.InAlign(alignment);
					const U8* p = ar.InBytes(compressedSize);
					if (at + originalSize > size)
						ar.Fail("zlib fragments exceed the declared %u bytes", size);
					if (ar.GetFailed())
						return;
					z_stream z;
					memset(&z, 0, sizeof z);
					z.next_in = const_cast<U8*>(p);
					z.avail_in = compressedSize;
					z.next_out = originalSize ? &data[at] : NULL;
					z.avail_out = originalSize;
					int result = inflateInit(&z);
					if (result == Z_OK)
					{
						result = inflate(&z, Z_FINISH);
						inflateEnd(&z);
					}
					if (result != Z_STREAM_END || z.total_out != originalSize)
						ar.Unconverted("zlib fragment %u decoded to %u of %u bytes (zlib %d)", i, (U32)z.total_out, originalSize, result);
					at += originalSize;
				}
				if (at != size)
					ar.Fail("zlib fragments give %u of %u bytes", at, size);
			}
			else if (compression == 3 && kanzen)
			{
				// [BlurX] Kanzen's MCT field planes: the fragment framing of types 1/2
				// (Kanzen FUN_823b1c90), decoded by the texture reader
				data.clear();
				const U32 count = ar.InU32();
				for (U32 i = 0; i < count && !ar.GetFailed(); ++i)
				{
					X360Convert::KanzenSegment segment;
					segment.logical = ar.InU32();
					const U32 stored = ar.InU32();
					ar.InAlign(alignment);
					const U8* p = ar.InBytes(stored);
					if (ar.GetFailed())
						return;
					segment.payload.assign(p, p + stored);
					kanzen->push_back(segment);
				}
			}
			else
				ar.Fail("unexpected Arch::Serialiser compression %u", compression);
		}

		// The same block as the PC exporter writes it: uncompressed, endian flag 0.
		void WriteSerialiserBlock(Arch::X360SceneTranscoder& ar, const std::vector<U8>& data, U32 alignment)
		{
			ar.OutU32((U32)data.size());
			ar.OutU32(alignment);
			ar.OutU32(Arch::NO_COMPRESSION);
			ar.OutU8(0);
			ar.OutAlign(alignment);
			if (!data.empty())
				ar.OutBytes(&data[0], (U32)data.size());
		}

		inline void Swap16(U8* p) { std::swap(p[0], p[1]); }
		inline void Swap32(U8* p) { std::swap(p[0], p[3]); std::swap(p[1], p[2]); }

		// DataTypes::SHHD_3N -> F32_4 -> U8_4N, as VertexData::ChangeDataType does it.
		U32 RepackNormal(U32 shhd)
		{
			const S32 x = ((S32)(shhd << 21)) >> 21;
			const S32 y = ((S32)(shhd << 10)) >> 21;
			const S32 z = ((S32)shhd) >> 22;
			float fx = ((float) x) / 1023.0f;
			float fy = ((float) y) / 1023.0f;
			float fz = ((float) z) / 511.0f;
			fx = Maths::Clamped((fx + 1.f) * 0.5f, 0.f, 1.f);
			fy = Maths::Clamped((fy + 1.f) * 0.5f, 0.f, 1.f);
			fz = Maths::Clamped((fz + 1.f) * 0.5f, 0.f, 1.f);
			RenderingData::DataTypes::U8_4N n(0u);
			n.X(fx);
			n.Y(fy);
			n.Z(fz);
			n.W(0.f);
			U32 v;
			memcpy(&v, &n, 4);
			return v;
		}

		SceneTranscodeStats g_stats = {};

		// [Blur1Multi] Both consoles (T076, then the 360 in the same session). The
		// translated programs use SHHD_3N as the console fetched it: signed normalised
		// 11:11:10 with w = 1. PS3: CELL_GCM_VERTEX_CMP (GCMDevice.cpp:2966-2977;
		// fx/include/fx/targets/cell/gcm.h:109,118 == CELL_GCM_VERTEX_S11_11_10_NR).
		// Xbox 360: the Xenos 10_11_11 fetch, signed and normalised. Direct3D on the 360
		// patches each vfetch's format from the bound vertex declaration at draw time,
		// so the compiled microcode carries format 0 and XenosRecomp cannot decode it
		// per fetch (its D3D9-mode "x * 2 - 1" rule for format 16/17 never fired: every
		// 360 program read the U8_4N the converter wrote as a [0, 1] value, and vehicle
		// bodies, whose morph offsets are SHHD_3N, were thrown out of view). D3D9 has no
		// such type, so the element becomes F16_4 (x, y, z, 1) for both.
		bool ExpandsShhd()
		{
			return true;
		}

		// Bytes per element type (renderingdata/source/noninterface/vertexbuffer.cpp:9-24).
		U32 ElementSize(S32 type)
		{
			static const U32 sizes[] = { 4, 8, 4, 8, 4, 8, 12, 16, 4, 8, 4, 4, 4, 4 };
			return type >= 0 && type < (S32)(sizeof(sizes) / sizeof(sizes[0])) ? sizes[type] : 0;
		}

		// The PC layout of a vertex format: on PS3 every SHHD_3N element grows from
		// 4 to 8 bytes and every byte after it moves by 4.
		void BuildLayout(SceneState::VertexLayout& layout)
		{
			layout.newOffsets = layout.offsets;
			layout.newStride = layout.stride;
			if (!ExpandsShhd())
				return;
			for (size_t e = 0; e < layout.types.size(); ++e)
			{
				if (layout.types[e] != RenderingData::VERTEX_ELEMENT_TYPE_SHHD_3N)
					continue;
				layout.newStride += 4;
				for (size_t f = 0; f < layout.offsets.size(); ++f)
					if (layout.offsets[f] > layout.offsets[e])
						layout.newOffsets[f] += 4;
			}
		}

		// DataTypes::SHHD_3N (renderingdata/datatypes.h:333-367) is x:11, y:11, z:10
		// from bit 0; the exporter writes it as x * 1023, y * 1023, z * 511
		// (datatypes.h:340-343, 349-351) and decodes it with the same divisors
		// (datatypes.h:345-347), which is the scaling used here. w = 1 as RSX CMP
		// fetches it. -1024 and -512 decode to just under -1 and are counted.
		void ExpandShhd(U32 shhd, U8* out, unsigned& extreme)
		{
			const S32 x = ((S32)(shhd << 21)) >> 21;
			const S32 y = ((S32)(shhd << 10)) >> 21;
			const S32 z = ((S32)shhd) >> 22;
			if (x == -1024 || y == -1024 || z == -512)
				++extreme;
			const RenderingData::DataTypes::F16_4 v(((float) x) / 1023.0f, ((float) y) / 1023.0f, ((float) z) / 511.0f, 1.0f);
			memcpy(out, &v, 8);
		}

		// The triangles a strip draws, winding kept (rotated so the smallest index is
		// first), degenerates dropped. With 'reset' a restart index begins a new strip
		// and resets the winding parity, as on the 360; PC strips have none.
		void StripTriangles(const U32* idx, size_t n, U32 reset, std::vector<U64>& out)
		{
			out.clear();
			size_t start = 0;
			for (size_t i = 0; i < n; ++i)
			{
				if (idx[i] == reset)
				{
					start = i + 1;
					continue;
				}
				if (i - start < 2)
					continue;
				U32 a = idx[i - 2], b = idx[i - 1], c = idx[i];
				if ((i - start) & 1)
					std::swap(a, b);
				if (a == b || b == c || a == c)
					continue;
				while (a > b || a > c)
				{
					const U32 t = a; a = b; b = c; c = t;
				}
				out.push_back(((U64)a << 42) | ((U64)b << 21) | c);
			}
			std::sort(out.begin(), out.end());
		}

		// One stripped lod: undo ConvertDegensToResetIndex and stitch the strips the
		// way CStripper does without ATGTRISTRIPPER_USE_RESET_INDEX.
		//
		// A strip that sat at an odd position in the stitched output comes out of
		// ConvertDegensToResetIndex with its first index doubled (even length) or
		// reversed (odd length). The doubled form is recognisable; the reversed one
		// is the same bytes as a strip that sat at an even position, and is stitched
		// as such (same triangles and winding, possibly different bytes: counted).
		void Restitch(const U32* in, size_t n, U32 reset, std::vector<U32>& out, X360Convert::SceneState& state)
		{
			size_t start = 0;
			while (start < n)
			{
				size_t end = start;
				while (end < n && in[end] != reset)
					++end;
				if (end > start)
				{
					const U32* s = in + start;
					size_t len = end - start;
					bool cw = true;
					if (len >= 2 && s[0] == s[1])
					{
						++s;
						--len;
						cw = false;
					}
					else if (len & 1)
						++state.reversedAmbiguous;

					if (!out.empty())
					{
						const U32 last = out.back();
						if (s[0] != last)
						{
							out.push_back(last);
							out.push_back(s[0]);
						}
					}
					if (cw != !(out.size() & 1))
						out.push_back(s[0]);
					out.insert(out.end(), s, s + len);
					++state.strips;
					if (state.countStats)
						++g_stats.strips;
				}
				start = end + 1;
			}
		}
	}
}

namespace RenderingData
{
	void Deserialise(Arch::X360SceneTranscoder& ar, PlatformType& t)
	{
		const size_t at = ar.OutPos();
		S32 value;
		ar & value;
		const S32 expected = X360Convert::GetSource() == X360Convert::E_Source_PS3 ? PLATFORM_TYPE_PS3 : PLATFORM_TYPE_360;
		if (value != expected)
			ar.Fail("platform type %d where the console exporter writes %d", value, (int)expected);
		ar.PatchU32(at, PLATFORM_TYPE_PC);
		t = PLATFORM_TYPE_PC;
	}

	// Scene::Finish(PC): ChangeVertexElementType(SHHD_3N, U8_4N). The value keeps
	// the 360 type so the vertex buffer reader knows what it is converting.
	// [Blur1Multi] PS3 sources: SHHD_3N becomes F16_4 (ExpandShhd), in vertex
	// formats and vertex declarations alike.
	void Deserialise(Arch::X360SceneTranscoder& ar, VertexElementType& t)
	{
		const size_t at = ar.OutPos();
		S32 value;
		ar & value;
		t = (VertexElementType) value;
		if (t == VERTEX_ELEMENT_TYPE_SHHD_3N)
			ar.PatchU32(at, X360Convert::ExpandsShhd() ? VERTEX_ELEMENT_TYPE_F16_4 : VERTEX_ELEMENT_TYPE_U8_4N);
	}

	// RenderingData::Deserialise(Archive&, RenderListNode::Primitive&) in
	// target/archive.cpp, without the render-state registration.
	void Deserialise(Arch::X360SceneTranscoder& ar, RenderListNode::Primitive& t)
	{
		PrimitiveType primitiveType;
		BlendState blendState;

		ar & t.effect;
		ar & t.vertexDeclaration;
		ar & primitiveType;
		ar & t.indexBufferIndex;
		ar & t.flags;
		ar & t.baseMaterial;

		if (ar.GetVersion() > 0)
			ar & t.instanceMaterial;
		else
			t.instanceMaterial.data = -1;

		ar & blendState;

		ar.State().primitiveIndexBuffer = t.indexBufferIndex;
		ar & t.lods;
		ar.State().primitiveIndexBuffer = -1;

		ar.State().primitiveDeclaration = t.vertexDeclaration;
		ar & t.primitiveStreams;
		ar.State().primitiveDeclaration = -1;

		t.primitiveType = primitiveType;
		t.blendState = 0;
	}

	// [Blur1Multi] Deserialise(Archive&, RenderListNode::PrimitiveStream&)
	// (renderingdata/noninterface/archive.inl:793-806), with a record of the
	// stream so PatchStreams can move its vertexOffset, a byte offset into the
	// vertex buffer (build/resourceblockbuild.cpp:409), when the PS3 path widens
	// that buffer's vertices.
	void Deserialise(Arch::X360SceneTranscoder& ar, RenderListNode::PrimitiveStream& t)
	{
		X360Convert::SceneState& state = ar.State();
		S32 vertexBuffer, streamIndex;

		ar & vertexBuffer;
		ar & streamIndex;
		X360Convert::SceneState::StreamRecord r;
		r.outPos = ar.OutPos();
		ar & t.vertexOffset;
		ar & t.frequency;
		ar & t.material;

		t.vertexBuffer = vertexBuffer;
		t.streamIndex = streamIndex;

		r.vertexOffset = t.vertexOffset;
		r.listIndex = vertexBuffer;
		r.streamIndex = streamIndex;
		r.declaration = state.primitiveDeclaration;
		r.buffer = X360Convert::SceneState::BufferId(-1, -1);
		if (state.streamNodes.empty())
		{
			if (X360Convert::ExpandsShhd())
				ar.Fail("primitive stream outside a RenderListNode");
			return;
		}
		state.streamNodes.back().push_back(state.streams.size());
		state.streams.push_back(r);
	}

	void Deserialise(Arch::X360SceneTranscoder& ar, RenderListNode::Primitive::Lod& t)
	{
		X360Convert::SceneState& state = ar.State();
		X360Convert::SceneState::LodRecord r;
		r.outPos = ar.OutPos();
		r.listIndex = state.primitiveIndexBuffer;
		r.buffer = X360Convert::SceneState::BufferId(-1, -1);

		ar & t.offset;
		ar & t.nIndices;
		ar & t.minIndex;
		ar & t.nVertices;

		r.offset = t.offset;
		r.nIndices = t.nIndices;
		if (state.nodes.empty())
			ar.Fail("draw range outside a RenderListNode");
		state.nodes.back().push_back(state.lods.size());
		state.lods.push_back(r);
	}

	// [BlurX] A render-list node's per-pass primitive ranges (survey: BLURX_PASS_TRACE
	// counts primitives per row index).
	//
	// Kanzen level nodes carry 7 rows where Blur 1's carry RENDER_PASS_MAX (6): Kanzen's
	// render lists are Opaque, AlphaTest, Overlay, BackFront, PostResolve, PostResolveAlpha
	// (the six built-in names of scene versions below 3, Blur2Test xbox_stage_decode.md
	// "Render-list names by scene version"), then row 6, which holds what Blur 1's row 5,
	// RENDER_PASS_RACIFICATION, holds (Nov 10 spa_barcelona_oval scene: 46 primitives in 6
	// nodes in row 6, none in row 5; Aug 31's Blur 1-layout oval: 442 in its row 5). Blur 1
	// walks the rows until they cover every primitive and maps each through a 6-entry
	// table (CachedRenderListNode.cpp), so a seventh row indexes past it (the race-load
	// crash in C_CachedRenderListNode::Gather). The PostResolveAlpha row is erased when
	// empty and row 6 becomes row 5; a PostResolveAlpha row with content has no Blur 1
	// pass and fails the file.
	void Deserialise(Arch::X360SceneTranscoder& ar, RenderListNode::RenderPassInfo& t)
	{
		X360Convert::SceneState& st = ar.State();
		const size_t tagAt = ar.OutPos() - 4;	// 'AR' + U16 version, written before this
		if (tagAt == st.passRowEnd)
			++st.passRowIndex;
		else
		{
			st.passRowIndex = 0;
			st.passRowStart = tagAt;
		}
		Serialise(ar, t);
		st.passRowEnd = ar.OutPos();
		static const bool s_trace = getenv("BLURX_PASS_TRACE") != NULL;
		if (s_trace && st.pass == 1)
			X360Convert::SceneTrace(0, "PASSROW", (unsigned)t.nPrimitives, (unsigned)t.primitiveOffset);
		if (st.passRowIndex != 6)
			return;
		const std::vector<U8>& o = ar.OutData();
		const size_t rowSize = st.passRowEnd - tagAt;
		const size_t countAt = st.passRowStart - 4;
		U32 count = 0, row5[6] = { 0 };
		memcpy(&count, &o[countAt], 4);
		const size_t row5At = st.passRowStart + 5 * rowSize;
		memcpy(row5, &o[row5At + 4], sizeof row5);
		if (count != 7 || rowSize != 4 + sizeof row5)
		{
			ar.Fail("render pass rows: 7th row where the vector holds %u (row size %u)", count, (unsigned)rowSize);
			return;
		}
		if (row5[0] || row5[2] || row5[4])
		{
			ar.Fail("Kanzen PostResolveAlpha row holds %u primitives, %u instances, %u batches (no Blur 1 pass)", row5[0], row5[2], row5[4]);
			return;
		}
		ar.OutErase(row5At, rowSize);
		ar.PatchU32(countAt, 6);
		st.passRowEnd -= rowSize;
		st.passRowIndex = -1;
		++st.kanzenObjects;
	}

	// The generic ChunkInfo reader, then a record of the chunk: its items and
	// where its offset and sizes went, so a level conversion can move them.
	void Deserialise(Arch::X360SceneTranscoder& ar, ChunkInfo& t)
	{
		const size_t infoAt = ar.OutPos();	// [BlurX] split-part items (below)
		X360Convert::SceneState::ChunkRecord c;
		if (ar.GetRawVersion() >= 2)
		{
			// [BlurX] Kanzen version 2 (FUN_823c6ed0): U64 offset, then U32
			// compressed size, U32 original size (0: stored uncompressed), U32 derived
			// (not established; dropped). Read at version 0, whose U64 compressed size
			// covers the two sizes, and written as Blur 1's version 1: U64 compressed,
			// U64 original, the original equal to the compressed when stored, as
			// Blur 1's version 0 reader sets it.
			ar.SetVersion(0);
			Serialise(ar, t);
			ar.SetVersion(1);
			const U64 sizes = t.GetCompressedSize();
			const U64 compressed = sizes >> 32;
			const U64 original = (sizes & 0xffffffffu) ? (sizes & 0xffffffffu) : compressed;
			ar.InU32();
			ar.OutTruncate(ar.OutPos() - 8);
			ar.OutBytes(&compressed, 8);
			ar.OutBytes(&original, 8);
			c.hasOriginalSize = true;
			c.outPos = ar.OutPos() - 24;
			c.offset = t.GetOffset();
			c.compressedSize = compressed;
			c.originalSize = original;
		}
		else
		{
		Serialise(ar, t);
		c.hasOriginalSize = ar.GetVersion() > 0;
		c.outPos = ar.OutPos() - (c.hasOriginalSize ? 24 : 16);
		c.offset = t.GetOffset();
		c.compressedSize = t.GetCompressedSize();
		c.originalSize = t.GetOriginalSize();
		}
		for (S32 i = 0; i < t.GetNumItems(); ++i)
			c.items.push_back(std::make_pair(t.GetItem(i).block, t.GetItem(i).lod));
		// [BlurX] the split-part copies dropped from this chunk (ReadChunkPass) leave its item
		// list too: find the list in the output (S32 count, then per item 'AR', U16 version,
		// S32 block, S32 lod) and erase them; offsets and sizes are recorded after this.
		X360Convert::SceneState& st = ar.State();
		if (st.pass >= 2 && !st.kanzenDropItems.empty() && t.GetNumItems() > 0)
		{
			const int chunkIndex = (int)st.chunks.size();
			std::vector<int> drop;
			for (S32 i = 0; i < t.GetNumItems(); ++i)
			{
				char key[32];
				sprintf_s(key, "%d/%d", chunkIndex, i);
				if (st.kanzenDropItems.count(key))
					drop.push_back(i);
			}
			if (!drop.empty())
			{
				const std::vector<U8>& o = ar.OutData();
				auto le32 = [&o](size_t at) -> U32 { U32 v = 0; if (at + 4 <= o.size()) memcpy(&v, &o[at], 4); return v; };
				const size_t n = (size_t)t.GetNumItems();
				size_t found = (size_t)-1;
				for (size_t p = infoAt; p + 4 + 12 * n <= c.outPos && found == (size_t)-1; ++p)
				{
					if (le32(p) != (U32)n)
						continue;
					bool match = true;
					for (size_t i = 0; i < n && match; ++i)
					{
						const size_t q = p + 4 + 12 * i;
						match = o[q] == 0x52 && o[q + 1] == 0x41 && (S32)le32(q + 4) == t.GetItem((S32)i).block && (S32)le32(q + 8) == t.GetItem((S32)i).lod;
					}
					if (match)
						found = p;
				}
				if (found == (size_t)-1)
					ar.Fail("chunk %d: its item list was not found in the output", chunkIndex);
				else
				{
					for (size_t k = drop.size(); k-- > 0; )
						ar.OutErase(found + 4 + 12 * (size_t)drop[k], 12);
					ar.PatchU32(found, (U32)(n - drop.size()));
					c.outPos -= 12 * drop.size();
					st.kanzenDroppedItems += (unsigned)drop.size();
				}
			}
		}
		ar.State().chunks.push_back(c);
	}

	// [BlurX] Kanzen material element types. Kanzen's size tables have 18 types
	// (Blur 1: 11), every one past Blur 1's 4 bytes in memory. The ones Nov 10 data
	// uses, 11 (g_vCodeDamageScale, g_vScrollTextureX, g_fAnimatedNeonX, g_vColour
	// ...), 13 (g_mPalette) and 16 (g_LocalSpaceSHNode), are archived per component
	// as Blur 1 archives MATERIAL_ELEMENT_TYPE_DATA_INDEX: a MaterialIndex object
	// ('AR', U16 version 0, S32 index, S32 list index; g_mPalette runs count the
	// index up from 1, the list index is 1). Retail Blur 1 X360 models carry the
	// same parameters as type 9, DATA_INDEX (g_LocalSpaceSHNode count 1, g_mPalette
	// count 1-28, g_vCodeDamageScale count 1), so these are written as type 9 (the
	// MaterialElementType overload below). Why Kanzen gives them three types is not
	// established. Any other type past Blur 1's has no reading; Blur 1 has no place for it
	// elements: they are dropped from the material type (later offsets and the
	// data size move down) and their data is consumed without output. The material
	// list's total data size is left as the file has it (it sizes an allocation,
	// so the dropped bytes only leave it larger).
	namespace
	{
		// the Kanzen element types written as Blur 1's DATA_INDEX (see above)
		// 14: Blur2Test xbox_stage_decode.md ("Survey topology refresh", "The palette form of the
		// pool join"): the same tagged reference, {S32 index, S32 process type 3} into the
		// level's runtime pool (g_mWorld / g_mPalette of _test_events, Miami's InstanceReset);
		// the pool is the scene's third dynamic material list, kept for level scenes (T030).
		// 15: the same tagged reference (Blur2Test "types 11, 13, and 15 ... serialize as the
		// same tagged 12-byte reference form"; "The Advert placement record": g_tAlbedo of the
		// advert_lm_ln draws, process type 3 plus an index into the runtime data pool)
		bool KanzenDataIndexType(S32 type) { return type == 11 || type == 13 || type == 14 || type == 15 || type == 16; }

		bool IsKanzenElement(S32 type) { return type >= MATERIAL_ELEMENT_TYPE_MAX; }

		U32 KanzenBe32(const U8* p) { return (U32)p[0] << 24 | (U32)p[1] << 16 | (U32)p[2] << 8 | p[3]; }

		// [BlurX] Split texture parts are separate allocations: their Xenos fetch constants
		// (lod header + 0x1c) differ in the base address (dword 1 bits 12-31) and the mip
		// address (dword 5 bits 12-31). Everything else (format, endian, size, swizzle,
		// filters, mip range, dimension, packing) must agree.
		bool SameSplitDescriptor(const U8* a, const U8* b)
		{
			if ((KanzenBe32(a + 0x20) & 0xfff) != (KanzenBe32(b + 0x20) & 0xfff))
				return false;
			for (int o = 0x24; o <= 0x2c; o += 4)
				if (KanzenBe32(a + o) != KanzenBe32(b + o))
					return false;
			return (KanzenBe32(a + 0x30) & 0xfff) == (KanzenBe32(b + 0x30) & 0xfff);
		}

		// Little-endian reads of what the transcoder has just written.
		U32 OutLE32(const std::vector<U8>& o, size_t at) { U32 v = 0; if (at + 4 <= o.size()) memcpy(&v, &o[at], 4); return v; }
	}

	// [BlurX] Kanzen types 11, 13, 14, 15 and 16 are Blur 1's DATA_INDEX (see above).
	void Deserialise(Arch::X360SceneTranscoder& ar, MaterialElementType& t)
	{
		const size_t at = ar.OutPos();
		S32 value;
		ar & value;
		if (KanzenDataIndexType(value))
		{
			value = MATERIAL_ELEMENT_TYPE_DATA_INDEX;
			ar.PatchU32(at, (U32)value);
			++ar.State().kanzenObjects;
		}
		t = (MaterialElementType)value;
	}

	void Deserialise(Arch::X360SceneTranscoder& ar, MaterialType& t)
	{
		const size_t start = ar.OutPos();
		Serialise(ar, t);	// the engine's reader: t keeps every element, Kanzen ones too
		bool kanzen = false;
		for (S32 e = 0; e < t.GetNumElements(); ++e)
			kanzen |= IsKanzenElement(t.GetElement(e).type);
		if (!kanzen || ar.GetFailed())
			return;

		// Rewrite what was written from 'start': Name (U32 length, chars), elements
		// ('AR', U16, U32 count, then per element 'AR', U16, name, S32 type, S32
		// count), U32 data size, U32 max align, offsets ('AR', U16, U32 count, U16
		// each), S32 effect, S32 list index.
		std::vector<U8> in;
		ar.CopyOut(start, in);
		size_t at = 0;
		std::vector<U8> out;
		const U32 nameLength = OutLE32(in, at);
		out.insert(out.end(), in.begin() + at, in.begin() + at + 4 + nameLength);
		at += 4 + nameLength;
		out.insert(out.end(), in.begin() + at, in.begin() + at + 4);	// vector tag and version
		at += 4;
		const U32 nElements = OutLE32(in, at);
		at += 4;
		const size_t countAt = out.size();
		out.resize(out.size() + 4);
		std::vector<bool> keep(nElements, true);
		std::vector<U32> sizes(nElements, 0);
		U32 kept = 0, dropped = 0;
		for (U32 e = 0; e < nElements && e < (U32)t.GetNumElements(); ++e)
		{
			const U32 elementName = OutLE32(in, at + 4);
			const size_t length = 4 + 4 + elementName + 4 + 4;
			const MaterialElement& element = t.GetElement(e);
			if (IsKanzenElement(element.type))
			{
				keep[e] = false;
				sizes[e] = 4 * element.count;
				dropped += sizes[e];
			}
			else
			{
				out.insert(out.end(), in.begin() + at, in.begin() + at + length);
				++kept;
			}
			at += length;
		}
		memcpy(&out[countAt], &kept, 4);
		const U32 dataSize = OutLE32(in, at) - dropped;
		out.insert(out.end(), (const U8*)&dataSize, (const U8*)&dataSize + 4);
		at += 4;
		out.insert(out.end(), in.begin() + at, in.begin() + at + 4);	// max align
		at += 4;
		out.insert(out.end(), in.begin() + at, in.begin() + at + 4);	// offsets tag and version
		at += 4;
		const U32 nOffsets = OutLE32(in, at);
		at += 4;
		const U32 keptOffsets = nOffsets == nElements ? kept : nOffsets;
		out.insert(out.end(), (const U8*)&keptOffsets, (const U8*)&keptOffsets + 4);
		for (U32 e = 0; e < nOffsets; ++e)
		{
			U16 offset = 0;
			memcpy(&offset, &in[at + 2 * e], 2);
			if (nOffsets == nElements && !keep[e])
				continue;
			// every element stored before this one in the data that was dropped
			U32 before = 0;
			for (U32 d = 0; d < nElements && nOffsets == nElements; ++d)
			{
				U16 other = 0;
				memcpy(&other, &in[at + 2 * d], 2);
				if (!keep[d] && other < offset)
					before += sizes[d];
			}
			const U16 moved = (U16)(offset - before);
			out.insert(out.end(), (const U8*)&moved, (const U8*)&moved + 2);
		}
		if (nOffsets != nElements)
			ar.Fail("material type %s: %u offsets for %u elements", t.GetName().Chars(), nOffsets, nElements);
		at += 2 * nOffsets;
		out.insert(out.end(), in.begin() + at, in.end());	// effect, list index
		ar.OutTruncate(start);
		ar.OutBytes(out.empty() ? 0 : &out[0], (U32)out.size());
		ar.State().kanzenObjects += nElements - kept;
	}

	// DeserialiseMaterial (target/archive.cpp) for the transcoder: the same reads per
	// element (MaterialIndex elements as objects), and Kanzen elements consumed with
	// nothing written. Nothing is allocated: the transcoder keeps no material data.
	void DeserialiseMaterial(Arch::X360SceneTranscoder& ar, Material& material, const MaterialType& materialType)
	{
		for (int e = 0; e < materialType.GetNumElements() && !ar.GetFailed(); e++)
		{
			const MaterialElement& element = materialType.GetElement(e);
			for (int c = 0; c < element.count && !ar.GetFailed(); c++)
			{
				switch (element.type)
				{
					case MATERIAL_ELEMENT_TYPE_S8: { S8 v; ar >> v; break; }
					case MATERIAL_ELEMENT_TYPE_U8: { U8 v; ar >> v; break; }
					case MATERIAL_ELEMENT_TYPE_S16: { S16 v; ar >> v; break; }
					case MATERIAL_ELEMENT_TYPE_U16: { U16 v; ar >> v; break; }
					case MATERIAL_ELEMENT_TYPE_S32:
					case MATERIAL_ELEMENT_TYPE_TEXTURE_INDEX:
					case MATERIAL_ELEMENT_TYPE_TEXTURE_PTR: { S32 v; ar >> v; break; }
					case MATERIAL_ELEMENT_TYPE_U32: { U32 v; ar >> v; break; }
					case MATERIAL_ELEMENT_TYPE_F32: { F32 v; ar >> v; break; }
					case MATERIAL_ELEMENT_TYPE_MATERIAL_INDEX:
					case MATERIAL_ELEMENT_TYPE_DATA_INDEX: { MaterialIndex v; ar >> v; break; }
					default:
						{
							// the bytes that follow, for working the layout out from the log
							char bytes[3 * 32 + 1] = "";
							const U32 at = ar.InPos();
							const U8* p = ar.InBytes(32);
							for (int b = 0; b < 32; ++b)
								sprintf_s(bytes + 3 * b, sizeof(bytes) - 3 * b, "%02x ", p[b]);
							ar.InSeek(at);
							ar.Fail("material element type %d (%s, count %d) has no reading; next bytes %s", (int)element.type, element.name.Chars(), element.count, bytes);
						}
				}
			}
		}
	}

	namespace Target
	{
		void Deserialise(Arch::X360SceneTranscoder& ar, IndexBuffer& t)
		{
			X360Convert::SceneState& state = ar.State();
			const X360Convert::SceneState::BufferId id(state.block, state.indexBufferCount++);
			state.indexBuffersRead.insert(id);

			const size_t sizeAt = ar.OutPos();
			S32 size;
			IndexType indexType;
			ar & size;
			ar & indexType;

			if (indexType != INDEX_TYPE_U16 && indexType != INDEX_TYPE_U32)
			{
				ar.Fail("index type %d", (int)indexType);
				return;
			}
			const U32 width = indexType == INDEX_TYPE_U16 ? 2 : 4;
			const U32 reset = indexType == INDEX_TYPE_U16 ? 0xffff : 0xffffffff;

			std::vector<U8> data;
			U32 alignment = 16;
			if (ar.GetVersion() > 0)
			{
				ar.TagFor<Arch::Deserialiser>();
				X360Convert::ReadSerialiserBlock(ar, data, alignment);
			}
			else
			{
				data.resize(size * width);
				if (!data.empty())
					memcpy(&data[0], ar.InBytes((U32)data.size()), data.size());
			}
			if (data.size() != (size_t)size * width)
				ar.Fail("index buffer holds %u bytes for %d indices", (unsigned)data.size(), size);
			if (ar.GetFailed())
				return;

			std::vector<U32> in(size);
			for (S32 i = 0; i < size; ++i)
			{
				const U8* p = &data[i * width];
				in[i] = width == 2 ? (U32)((p[0] << 8) | p[1]) : (((U32)p[0] << 24) | ((U32)p[1] << 16) | ((U32)p[2] << 8) | p[3]);
			}

			std::vector<U32> result;
			if (state.pass == 1)
				result = in;
			else
			{
				// Each draw range is one stripped lod; rebuild them one by one.
				std::vector<X360Convert::SceneState::Range> ranges = state.segments[id];
				std::map<X360Convert::SceneState::Range, X360Convert::SceneState::Range>& remap = state.remap[id];
				std::map<S32, S32>& boundaries = state.boundaries[id];
				S32 at = 0;
				std::vector<X360Convert::SceneState::Range> overlapping;	// [BlurX]
				for (size_t r = 0; r <= ranges.size(); ++r)
				{
					const S32 first = r < ranges.size() ? ranges[r].first : size;
					const S32 count = r < ranges.size() ? ranges[r].second : 0;
					if (first < at)
					{
						// [BlurX] Kanzen strip draws share index data: a range that starts
						// inside the previous one gets its own re-stitched copy after the
						// buffer's other data (Nov 10 vehicles; Blur 1 ranges never overlap)
						if (remap.find(ranges[r]) == remap.end())
						{
							if (first + count > size || count < 0)
							{
								remap[ranges[r]] = X360Convert::SceneState::Range((S32)result.size(), 0);
								++state.kanzenDanglingRanges;
							}
							else
								overlapping.push_back(ranges[r]);
						}
						continue;
					}
					// [BlurX] A draw range past the end of its buffer: Kanzen vehicles' coarse
					// lods (slots 2-3) name indices the file does not hold (Blur2Test: those
					// slots "do not resolve"). They become empty draws, recorded in the log;
					// Blur 1 data has none.
					if (r < ranges.size() && (first > size || count < 0 || first + count > size))
					{
						remap[ranges[r]] = X360Convert::SceneState::Range((S32)result.size(), 0);
						++state.kanzenDanglingRanges;
						continue;
					}
					if (first > at)
					{
						// indices no draw range covers: kept as one run of strips
						boundaries[at] = (S32)result.size();
						X360Convert::Restitch(&in[at], first - at, reset, result, state);
					}
					if (r == ranges.size())
						break;
					const S32 newFirst = (S32)result.size();
					std::vector<U32> lod;
					X360Convert::Restitch(&in[first], count, reset, lod, state);

					// The draw must produce exactly the triangles it did on the 360.
					std::vector<U64> before, after;
					X360Convert::StripTriangles(&in[first], count, reset, before);
					X360Convert::StripTriangles(lod.empty() ? NULL : &lod[0], lod.size(), 0xffffffff, after);
					if (before != after)
						ar.Unconverted("draw range (%d,%d) of index buffer (%d,%d): %u triangles on the 360, %u re-stitched",
							first, count, id.first, id.second, (unsigned)before.size(), (unsigned)after.size());
					state.trianglesChecked += (unsigned)before.size();
					if (state.countStats)
						X360Convert::g_stats.trianglesChecked += (unsigned)before.size();
					result.insert(result.end(), lod.begin(), lod.end());
					remap[ranges[r]] = X360Convert::SceneState::Range(newFirst, (S32)lod.size());
					boundaries[first] = newFirst;
					at = first + count;
				}
				// [BlurX] the overlapping ranges, each re-stitched on its own
				for (size_t o = 0; o < overlapping.size(); ++o)
				{
					const S32 first = overlapping[o].first, count = overlapping[o].second;
					std::vector<U32> lod;
					X360Convert::Restitch(&in[first], count, reset, lod, state);
					std::vector<U64> before, after;
					X360Convert::StripTriangles(&in[first], count, reset, before);
					X360Convert::StripTriangles(lod.empty() ? NULL : &lod[0], lod.size(), 0xffffffff, after);
					if (before != after)
						ar.Unconverted("draw range (%d,%d) of index buffer (%d,%d): %u triangles on the 360, %u re-stitched",
							first, count, id.first, id.second, (unsigned)before.size(), (unsigned)after.size());
					state.trianglesChecked += (unsigned)before.size();
					remap[overlapping[o]] = X360Convert::SceneState::Range((S32)result.size(), (S32)lod.size());
					result.insert(result.end(), lod.begin(), lod.end());
				}
				boundaries[size] = (S32)result.size();
				ar.PatchU32(sizeAt, (U32)result.size());
			}

			std::vector<U8> le(result.size() * width);
			for (size_t i = 0; i < result.size(); ++i)
			{
				if (width == 2)
				{
					const U16 v = (U16)result[i];
					memcpy(&le[i * 2], &v, 2);
				}
				else
					memcpy(&le[i * 4], &result[i], 4);
			}

			if (ar.GetVersion() > 0)
			{
				// the tag and version of the Deserialiser were written by TagFor
				X360Convert::WriteSerialiserBlock(ar, le, alignment);
			}
			else if (!le.empty())
				ar.OutBytes(&le[0], (U32)le.size());
		}

		void Deserialise(Arch::X360SceneTranscoder& ar, VertexBuffer& t)
		{
			X360Convert::SceneState& state = ar.State();
			const X360Convert::SceneState::BufferId id(state.block, state.vertexBufferCount++);

			// [Blur1Multi] The VertexFormat, read field by field (Serialise(VertexFormat&),
			// renderingdata/noninterface/archive.inl:246-252: DataSize, Elements, Offsets)
			// so the PS3 path can move DataSize and Offsets for its F16_4 elements.
			X360Convert::SceneState::VertexLayout layout;
			std::vector<VertexFormat::VertexElement> elements;
			std::vector<size_t> offsetAt;
			ar.TagFor<VertexFormat>();
			const size_t strideAt = ar.OutPos();
			ar >> layout.stride;
			ar.TagFor<Btl::Vector<VertexFormat::VertexElement> >();
			S32 nElements;
			ar >> nElements;
			if (nElements < 0 || nElements > 64)
				ar.Fail("vertex format with %d elements", nElements);
			for (S32 e = 0; e < nElements && !ar.GetFailed(); ++e)
			{
				VertexFormat::VertexElement element;
				ar >> element;
				elements.push_back(element);
				layout.types.push_back(element.type);
			}
			ar.TagFor<Btl::Vector<U16> >();
			S32 nOffsets;
			ar >> nOffsets;
			if (nOffsets != nElements)
				ar.Fail("vertex format with %d elements and %d offsets", nElements, nOffsets);
			for (S32 e = 0; e < nOffsets && !ar.GetFailed(); ++e)
			{
				offsetAt.push_back(ar.OutPos());
				U16 offset;
				ar >> offset;
				layout.offsets.push_back(offset);
			}
			if (ar.GetFailed())
				return;
			X360Convert::BuildLayout(layout);
			for (S32 e = 0; e < nElements; ++e)
			{
				if (layout.offsets[e] + X360Convert::ElementSize(layout.types[e]) > (U32)layout.stride)
					ar.Fail("vertex element %d of type %d at %d outside a %d-byte vertex", e, layout.types[e], layout.offsets[e], layout.stride);
				if (layout.newOffsets[e] != layout.offsets[e])
					ar.PatchU16(offsetAt[e], (U16)layout.newOffsets[e]);
			}
			if (layout.newStride != layout.stride)
				ar.PatchU32(strideAt, (U32)layout.newStride);
			state.vertexLayouts[id] = layout;

			S32 size;
			ar & size;

			const U32 stride = layout.stride;
			const U32 bufferSize = size * stride;

			std::vector<U8> data;
			U32 alignment = 16;
			if (ar.GetVersion() > 0)
			{
				ar.TagFor<Arch::Deserialiser>();
				X360Convert::ReadSerialiserBlock(ar, data, alignment);
			}
			else
			{
				data.resize(bufferSize);
				if (bufferSize)
					memcpy(&data[0], ar.InBytes(bufferSize), bufferSize);
			}
			if (data.size() != bufferSize)
				ar.Fail("vertex buffer holds %u bytes for %d vertices of %u", (unsigned)data.size(), size, stride);
			if (ar.GetFailed())
				return;

			const bool expand = layout.newStride != layout.stride;
			for (S32 v = 0; v < size; ++v)
			{
				for (int e = 0; e < nElements; ++e)
				{
					U8* p = &data[v * stride + layout.offsets[e]];
					switch (layout.types[e])
					{
						case VERTEX_ELEMENT_TYPE_S16_2:
						case VERTEX_ELEMENT_TYPE_S16_2N:
						case VERTEX_ELEMENT_TYPE_F16_2:
							X360Convert::Swap16(p); X360Convert::Swap16(p + 2);
							break;
						case VERTEX_ELEMENT_TYPE_S16_4:
						case VERTEX_ELEMENT_TYPE_S16_4N:
						case VERTEX_ELEMENT_TYPE_F16_4:
							X360Convert::Swap16(p); X360Convert::Swap16(p + 2); X360Convert::Swap16(p + 4); X360Convert::Swap16(p + 6);
							break;
						case VERTEX_ELEMENT_TYPE_F32_4: X360Convert::Swap32(p + 12);	// fall through
						case VERTEX_ELEMENT_TYPE_F32_3: X360Convert::Swap32(p + 8);	// fall through
						case VERTEX_ELEMENT_TYPE_F32_2: X360Convert::Swap32(p + 4);	// fall through
						case VERTEX_ELEMENT_TYPE_F32_1: X360Convert::Swap32(p);
							break;
						case VERTEX_ELEMENT_TYPE_U8_4:
						case VERTEX_ELEMENT_TYPE_U8_4N:
							// PS3: reversed by the exporter, then written as a BE U32: PC order already
							if (X360Convert::GetSource() != X360Convert::E_Source_PS3)
								X360Convert::Swap32(p);
							break;
						case VERTEX_ELEMENT_TYPE_SDDD_3N:
							X360Convert::Swap32(p);
							break;
						case VERTEX_ELEMENT_TYPE_SHHD_3N:
						{
							// PS3: expanded to F16_4 below, from these big-endian bytes
							if (X360Convert::ExpandsShhd())
								break;
							const U32 shhd = ((U32)p[0] << 24) | ((U32)p[1] << 16) | ((U32)p[2] << 8) | p[3];
							const U32 n = X360Convert::RepackNormal(shhd);
							memcpy(p, &n, 4);
							++state.normalsRepacked;
							if (state.countStats)
								++X360Convert::g_stats.normalsRepacked;
							break;
						}
						default:
							ar.Unconverted("vertex element type %d", (int)layout.types[e]);
					}
				}
			}

			// [Blur1Multi] PS3: every byte moves to its new offset, and each SHHD_3N
			// becomes an F16_4 (ExpandShhd). Padding bytes move with the element before them.
			if (expand)
			{
				std::vector<S32> byteShift(stride, -1);		// -1: inside an SHHD_3N element
				for (U32 b = 0; b < stride; ++b)
				{
					S32 shift = 0;
					bool inside = false;
					for (int e = 0; e < nElements; ++e)
						if (layout.types[e] == VERTEX_ELEMENT_TYPE_SHHD_3N)
						{
							if ((U32)layout.offsets[e] + 4 <= b)
								shift += 4;
							else if ((U32)layout.offsets[e] <= b)
								inside = true;
						}
					if (!inside)
						byteShift[b] = shift;
				}
				unsigned extreme = 0;
				std::vector<U8> expanded(size * layout.newStride);
				for (S32 v = 0; v < size; ++v)
				{
					const U8* src = &data[v * stride];
					U8* dst = &expanded[v * layout.newStride];
					for (U32 b = 0; b < stride; ++b)
						if (byteShift[b] >= 0)
							dst[b + byteShift[b]] = src[b];
					for (int e = 0; e < nElements; ++e)
						if (layout.types[e] == VERTEX_ELEMENT_TYPE_SHHD_3N)
						{
							const U8* p = src + layout.offsets[e];
							X360Convert::ExpandShhd(((U32)p[0] << 24) | ((U32)p[1] << 16) | ((U32)p[2] << 8) | p[3], dst + layout.newOffsets[e], extreme);
						}
				}
				data.swap(expanded);
				if (state.countStats)
				{
					for (int e = 0; e < nElements; ++e)
						if (layout.types[e] == VERTEX_ELEMENT_TYPE_SHHD_3N)
						{
							X360Convert::g_stats.shhdExpanded += size;
							X360Convert::g_stats.byUsage[elements[e].usage & 15][elements[e].usageIndex & 7]++;
						}
					X360Convert::g_stats.shhdExtreme += extreme;
				}
			}

			if (ar.GetVersion() > 0)
				X360Convert::WriteSerialiserBlock(ar, data, alignment);
			else if (!data.empty())
				ar.OutBytes(&data[0], (U32)data.size());
		}

		// Target::Deserialise(Archive&, VertexDeclaration&) without the device
		// declaration; the element types go through the VertexElementType overload.
		//
		// [Blur1Multi] PS3: an element's offset grows by 4 for every SHHD_3N element
		// before it in the same stream, as the vertex formats' offsets do (BuildLayout);
		// PatchStreams checks each primitive's declaration against its vertex buffer.
		void Deserialise(Arch::X360SceneTranscoder& ar, VertexDeclaration& t)
		{
			typedef X360Convert::SceneState::DeclElement DeclElement;
			std::vector<DeclElement> elements;
			std::vector<size_t> offsetAt;
			S32 nElements;
			ar & nElements;
			for (int i = 0; i < nElements && !ar.GetFailed(); i++)
			{
				const size_t at = ar.OutPos();
				RenderingData::VertexDeclaration::VertexElement element;
				ar & element;
				// Serialise(VertexDeclaration::VertexElement&) (archive.inl:256-265):
				// the object tag (if any), S16 stream, S16 offset, ...
				const size_t fields = at + (ar.OutU16At(at) == 'AR' ? 4 : 0);
				if ((S16)ar.OutU16At(fields) != element.stream || (S16)ar.OutU16At(fields + 2) != element.offset)
					ar.Fail("vertex declaration element %d not where Serialise(VertexElement) writes it", i);
				offsetAt.push_back(fields + 2);
				DeclElement d;
				d.stream = element.stream;
				d.offset = element.offset;
				d.newOffset = element.offset;
				d.type = element.type;
				elements.push_back(d);
			}
			if (X360Convert::ExpandsShhd())
				for (size_t i = 0; i < elements.size(); ++i)
				{
					for (size_t j = 0; j < elements.size(); ++j)
						if (elements[j].type == VERTEX_ELEMENT_TYPE_SHHD_3N && elements[j].stream == elements[i].stream && elements[j].offset < elements[i].offset)
							elements[i].newOffset += 4;
					if (elements[i].newOffset != elements[i].offset)
						ar.PatchU16(offsetAt[i], (U16)elements[i].newOffset);
				}
			ar.State().declarations.push_back(elements);
		}

		// Target::Deserialise(Archive&, Texture&): a whole texture as Writer<Texture>
		// writes it (NglXeTex / NglPcTex HEADER_AND_DATA behind a byte count).
		void Deserialise(Arch::X360SceneTranscoder& ar, Texture& t)
		{
			PlatformType platformType;
			ar & platformType;

			Btl::String name;
			ar & name;
			SamplerState samplerState;
			ar & samplerState;
			U32 hashValue;
			ar & hashValue;

			const size_t sizeAt = ar.OutPos();
			S32 size;
			ar & size;
			if (size <= 0)
				return;

			const U8* p = ar.InBytes(size);
			if (ar.GetFailed())
				return;
			std::vector<U8> pc;
			size_t consumed = 0;
			std::string error;
			if (X360Convert::ConvertTexturePayload(p, size, consumed, pc, error) != X360Convert::E_Result_Converted || consumed != (size_t)size)
			{
				ar.Unconverted("scene texture '%s': %s", name.Chars(), error.empty() ? "data does not fill the record" : error.c_str());
				return;
			}
			ar.OutBytes(&pc[0], (U32)pc.size());
			ar.PatchU32(sizeAt, (U32)pc.size());
		}

		// Target::Deserialise(Archive&, LodReader<Texture>&): one lod of a resource
		// block texture as LodWriter<Texture> writes it.
		void Deserialise(Arch::X360SceneTranscoder& ar, LodReader<Texture>& t)
		{
			const U32 recordAt = ar.InPos();	// [BlurX] split texture part identity
			PlatformType platformType;
			ar & platformType;

			Btl::String name;
			U32 hashValue = 0;
			if (t.m_lod == 0)
			{
				ar & name;
				SamplerState samplerState;
				ar & samplerState;
				ar & hashValue;
			}

			const size_t presentAt = ar.OutPos();	// [BlurX] a partial Kanzen tier is rewritten as present = 0
			U32 present;
			ar >> present;
			if (!present)
				return;
			const bool paired = (present > 1);

			const size_t headerSizeAt = ar.OutPos();
			S32 headerSize;
			ar & headerSize;

			std::vector<U8> header;
			if (headerSize > 0)
			{
				const U8* p = ar.InBytes(headerSize);
				if (ar.GetFailed())
					return;
				header.assign(p, p + headerSize);
				std::vector<U8> pc;
				std::string error;
				if (X360Convert::ConvertTextureLodHeader(p, headerSize, pc, error) != X360Convert::E_Result_Converted)
				{
					ar.Unconverted("texture '%s' lod %d header: %s", name.Chars(), t.m_lod, error.c_str());
					ar.OutBytes(p, headerSize);
				}
				else
				{
					ar.OutBytes(&pc[0], (U32)pc.size());
					ar.PatchU32(headerSizeAt, (U32)pc.size());
				}
			}

			if (paired)
			{
				ar & t.m_source;
				return;
			}

			std::vector<U8> data;
			std::vector<X360Convert::KanzenSegment> kanzen;	// [BlurX] compression type 3
			U32 alignment = 16;
			size_t dataSizeAt = 0;
			if (ar.GetVersion() == 0)
			{
				dataSizeAt = ar.OutPos();
				S32 dataSize;
				ar & dataSize;
				if (dataSize > 0)
				{
					const U8* p = ar.InBytes(dataSize);
					if (!ar.GetFailed())
						data.assign(p, p + dataSize);
				}
			}
			else
			{
				ar.TagFor<Arch::Deserialiser>();
				X360Convert::ReadSerialiserBlock(ar, data, alignment, &kanzen);
			}
			if (ar.GetFailed())
				return;

			std::vector<U8> pc;
			// [BlurX] a Kanzen split texture part (X360Convert::KanzenSplitTexture)
			std::shared_ptr<X360Convert::KanzenSplitTexture> split;
			if (!kanzen.empty() && ar.State().kanzenBlockWords[1] > 1 && header.size() == 64)
			{
				X360Convert::SceneState& st = ar.State();
				const unsigned count = st.kanzenBlockWords[1];
				const unsigned ordinal = st.kanzenBlockWords[0];
				char record[48];
				sprintf_s(record, "%d@%u", st.chunk, recordAt);
				std::string key;
				if (st.pass == 1)
				{
					// parts follow part 0 in the same resource block and lod, possibly in the next chunks
					char group[48];
					sprintf_s(group, "%d/%d", st.block, t.m_lod);
					std::vector<std::string>& open = st.kanzenSplitOpen[group];
					if (ordinal == 0)
					{
						char k[64];
						sprintf_s(k, "%s/%u", group, st.kanzenSplitNext[group]++);
						key = k;
						open.push_back(key);
					}
					else
					{
						// several split textures of one block and lod can interleave: the part
						// joins the latest open group of the same descriptor (addresses aside),
						// part count and name that has the part before it and not this one
						for (size_t i = open.size(); i-- > 0 && key.empty(); )
						{
							const std::shared_ptr<X360Convert::KanzenSplitTexture>& o = st.kanzenSplits[open[i]];
							if (o && o->count == count && ordinal < count && o->have[ordinal - 1] && !o->have[ordinal]
								&& o->name == name.Chars() && SameSplitDescriptor(&o->header[0], &header[0]))
								key = open[i];
						}
						if (key.empty())
						{
							ar.Unconverted("texture '%s' lod %d: split part %u of %u matches no open part group", name.Chars(), t.m_lod, ordinal, count);
							return;
						}
					}
					st.kanzenSplitRecord[record] = key;
				}
				else
				{
					std::map<std::string, std::string>::iterator r = st.kanzenSplitRecord.find(record);
					if (r == st.kanzenSplitRecord.end())
					{
						ar.Unconverted("texture lod %d: split part %u of %u not seen in pass 1", t.m_lod, ordinal, count);
						return;
					}
					key = r->second;
				}
				std::shared_ptr<X360Convert::KanzenSplitTexture>& g = st.kanzenSplits[key];
				if (st.pass == 1)
				{
					if (!g)
					{
						g.reset(new X360Convert::KanzenSplitTexture);
						g->count = count;
						g->have.assign(count, 0);
						g->header = header;
						g->name = name.Chars();
					}
					// the parts' descriptors agree past the fetch constant's base address word
					char why[160] = "";
					if (g->count != count)
						sprintf_s(why, "part %u says %u parts, part 0 said %u", ordinal, count, g->count);
					else if (ordinal >= count)
						sprintf_s(why, "part ordinal %u of %u", ordinal, count);
					else if (g->have[ordinal])
						sprintf_s(why, "part %u twice (record %s)", ordinal, record);
					else if (!SameSplitDescriptor(&g->header[0], &header[0]))
						sprintf_s(why, "part %u descriptor differs from part 0's (%08x %08x %08x %08x %08x vs %08x %08x %08x %08x %08x)", ordinal,
							KanzenBe32(&g->header[0x20]), KanzenBe32(&g->header[0x24]), KanzenBe32(&g->header[0x28]), KanzenBe32(&g->header[0x2c]), KanzenBe32(&g->header[0x30]),
							KanzenBe32(&header[0x20]), KanzenBe32(&header[0x24]), KanzenBe32(&header[0x28]), KanzenBe32(&header[0x2c]), KanzenBe32(&header[0x30]));
					if (why[0])
					{
						if (g->consistent)
							g->why = why;
						g->consistent = false;
					}
					else
					{
						g->have[ordinal] = 1;
						g->segments.insert(g->segments.end(), kanzen.begin(), kanzen.end());
					}
				}
				split = g;
				if (ordinal != 0 || st.pass == 1 || !split)
				{
					// every part but 0 (and part 0 in pass 1, whose output is not kept) is Blur 1's empty lod
					ar.OutTruncate(presentAt);
					ar.OutU32(0);
					return;
				}
			}
			if (split)
			{
				X360Convert::SceneState& st = ar.State();
				bool whole = split->consistent;
				for (size_t i = 0; i < split->have.size(); ++i)
					whole = whole && split->have[i] != 0;
				if (!split->consistent)
				{
					ar.Unconverted("texture '%s' lod %d: split parts do not agree: %s", name.Chars(), t.m_lod, split->why.c_str());
					return;
				}
				if (!whole && t.m_lod > 0)
				{
					ar.OutTruncate(presentAt);
					ar.OutU32(0);
					if (st.countStats)
						++st.kanzenSplitIncomplete;
					return;
				}
				std::string error;
				unsigned levels = 0;
				if (!X360Convert::ConvertKanzenTextureLod(&header[0], t.m_lod, split->segments, pc, levels, error, NULL, !whole))
				{
					ar.Unconverted("texture '%s' lod %d (split into %u parts): %s", name.Chars(), t.m_lod, split->count, error.c_str());
					return;
				}
				if (st.countStats)
				{
					if (whole)
						++st.kanzenSplitJoined;
					else
						++st.kanzenSplitIncomplete;
				}
				if (t.m_lod == 0)
				{
					if (ar.OutU16At(headerSizeAt) != 28)
						ar.Unconverted("texture '%s': PC header of unexpected size", name.Chars());
					X360Convert::NoteLodTrim(ar, headerSizeAt, levels);
					ar.PatchU32(headerSizeAt + 4 + 12, levels);
				}
			}
			else if (!kanzen.empty())
			{
				// [BlurX] lod 0 carries the levels decoded (KanzenTexture.h); the PC
				// header written above states that count
				std::string error;
				unsigned levels = 0;
				bool partial = false;
				if (header.size() != 64)
					ar.Unconverted("texture '%s' lod %d: compression type 3 with a %u-byte header", name.Chars(), t.m_lod, (unsigned)header.size());
				else if (!X360Convert::ConvertKanzenTextureLod(&header[0], t.m_lod, kanzen, pc, levels, error, &partial))
				{
					if (partial && t.m_lod > 0)
					{
						// [BlurX] A tier above the base that ships part of its surface (a
						// partial residency transfer; Blur2Test stops the chain there and
						// keeps the tiers below). Written as Blur 1's own empty lod (present
						// 0, what LodWriter<Texture> writes when a lod has no level), so the
						// engine's finest level of the texture is the tier below.
						ar.OutTruncate(presentAt);
						ar.OutU32(0);
						if (ar.State().pass == 1)
							++ar.State().kanzenPartialTiers;
						return;
					}
					ar.Unconverted("texture '%s' lod %d (compression type 3): %s", name.Chars(), t.m_lod, error.c_str());
				}
				else if (t.m_lod == 0)
				{
					// the PC header: S32 size, then width, height, depth, levels
					if (ar.OutU16At(headerSizeAt) != 28)
						ar.Unconverted("texture '%s': PC header of unexpected size", name.Chars());
					X360Convert::NoteLodTrim(ar, headerSizeAt, levels);
					ar.PatchU32(headerSizeAt + 4 + 12, levels);
				}
			}
			else if (!data.empty())
			{
				std::string error;
				if (header.empty())
					ar.Unconverted("texture '%s' lod %d has data but no header", name.Chars(), t.m_lod);
				else if (X360Convert::ConvertTextureLodData(&header[0], t.m_lod, &data[0], data.size(), pc, error) != X360Convert::E_Result_Converted)
					ar.Unconverted("texture '%s' lod %d data: %s", name.Chars(), t.m_lod, error.c_str());
			}

			if (t.m_lod > 0 && headerSize > 0)
				X360Convert::ApplyLodTrim(ar, headerSizeAt);	// [BlurX]
			if (ar.GetVersion() == 0)
			{
				if (!pc.empty())
				{
					ar.OutBytes(&pc[0], (U32)pc.size());
					ar.PatchU32(dataSizeAt, (U32)pc.size());
				}
			}
			else
				X360Convert::WriteSerialiserBlock(ar, pc, alignment);
		}

		// Target::Deserialise(Archive&, LodReader<ResourceBlock::Resources>&) without
		// the resource allocator (not thread-safe; the engine serialises its users):
		// buffers are read into temporaries, nodes into a local vector.
		void Deserialise(Arch::X360SceneTranscoder& ar, LodReader<ResourceBlock::Resources>& t)
		{
			PlatformType platformType;
			ar & platformType;

			S32 lod;
			ar & lod;
			if (lod != t.m_lod)
				ar.Fail("resource block lod %d where %d was expected", lod, t.m_lod);

			// [BlurX] Kanzen version 1 (FUN_823cc660) reads a U32 after the lod (its
			// default comes from the reader; meaning not established) and reads the
			// lod 0 buffers and nodes only when it is below 1. Blur 1's reader always
			// reads them at lod 0: a block that omits them gets them written empty.
			S32 kanzenWord = 0;
			if (ar.GetRawVersion() >= 1)
				kanzenWord = (S32)ar.InU32();
			ar.State().kanzenItemWord = kanzenWord;	// [BlurX] split-part copies (ReadChunkPass)
			if (lod == 0 && kanzenWord >= 1)
			{
				for (int i = 0; i < 3; ++i)
				{
					ar.OutU16('AR');
					ar.OutU16(0);
					ar.OutU32(0);
				}
			}
			else if (lod == 0)
			{
				// ResourceBlock::Resources::VectorType<T>::Result: a tagged Btl::Vector of
				// AutoPointers (no tag, a SerialisationType byte, then the object).
				ar.TagFor<ResourceBlock::Resources::VectorType<VertexBuffer>::Result>();
				S32 n;
				ar >> n;
				for (S32 i = 0; i < n && !ar.GetFailed(); ++i)
				{
					Arch::SerialisationType type;
					ar >> type;
					if (type == Arch::SERIALISATION_TYPE_UNKNOWN)
						continue;
					if (type == Arch::SERIALISATION_TYPE_POLYMORPHIC)
						ar.Fail("polymorphic vertex buffer");
					VertexBuffer buffer;
					ar >> buffer;
				}

				ar.TagFor<ResourceBlock::Resources::VectorType<IndexBuffer>::Result>();
				ar >> n;
				for (S32 i = 0; i < n && !ar.GetFailed(); ++i)
				{
					Arch::SerialisationType type;
					ar >> type;
					if (type == Arch::SERIALISATION_TYPE_UNKNOWN)
						continue;
					if (type == Arch::SERIALISATION_TYPE_POLYMORPHIC)
						ar.Fail("polymorphic index buffer");
					IndexBuffer buffer;
					ar >> buffer;
				}

				Btl::Vector<Btl::AutoPointer<Node> > nodes;
				ar & nodes;
			}

			S32 nTextures;
			ar & nTextures;
			static const bool s_texTrace = getenv("BLURX_TEXLIST_TRACE") != NULL;	// [BlurX] T030 survey
			std::string texTrace;
			for (S32 i = 0; i < nTextures && !ar.GetFailed(); ++i)
			{
				Texture texture;
				LodReader<Texture> textureLod(0, 0, texture, lod);
				ar.State().textureIndex = i;	// [BlurX] lod level trims
				ar.State().kanzenBlockWords[0] = 0;
				ar.State().kanzenBlockWords[1] = 1;
				ar & textureLod;
				if (s_texTrace)
				{
					char b[24];
					sprintf_s(b, " %u/%u", ar.State().kanzenBlockWords[0], ar.State().kanzenBlockWords[1]);
					texTrace += b;
				}
			}
			if (s_texTrace && ar.State().pass == 1)
				LOGINFO(Debug::GetLogger("Blur1Multi.X360Convert"), "texlist chunk %d block %d lod %d word %d: %d textures:%s", ar.State().chunk, ar.State().block, lod, ar.State().kanzenItemWord, nTextures, texTrace.c_str());
		}

		// Target::Deserialise(Archive&, SharedTextureList&) without the shared
		// texture manager.
		void Deserialise(Arch::X360SceneTranscoder& ar, SharedTextureList& t)
		{
			PlatformType platformType;
			ar & platformType;

			S32 nTextures;
			ar & nTextures;
			for (int i = 0; i < nTextures; i++)
			{
				Btl::String name;
				U32 hash;
				ar & name;
				ar & hash;
				Texture texture;
				ar & texture;
			}

		}
	}
}

namespace X360Convert
{
	namespace
	{
		// "ARCH", U32 1 in the writer's byte order, U32 swap flag
		// (Arch::BinaryInputArchive). The 360 exporter ran on PC with swapping on.
		bool CheckArchiveHeader(const U8* in, size_t size, std::string& error)
		{
			if (size < 20 || memcmp(in, "ARCH", 4) != 0)
			{
				error = "not an ARCH stream";
				return false;
			}
			U32 one, swap;
			memcpy(&one, in + 4, 4);
			memcpy(&swap, in + 8, 4);
			if ((one != 1) == (swap != 0))
			{
				error = "archive is not endian-swapped (already PC data?)";
				return false;
			}
			return true;
		}

		void PutArchiveHeader(std::vector<U8>& out)
		{
			const U32 header[3] = { 0x48435241 /* "ARCH" */, 1, 0 };
			out.insert(out.end(), (const U8*)header, (const U8*)header + 12);
		}

		// One pass of WorldRender::ReadScene over the 360 bytes. Returns false with
		// 'error' set on failure.
		bool ReadScenePass(const U8* in, size_t size, std::vector<U8>& out, SceneState& state, std::string& error)
		{
			out.clear();
			out.reserve(size * 2);
			if (!CheckArchiveHeader(in, size, error))
				return false;
			PutArchiveHeader(out);

			Arch::X360SceneTranscoder ar((const U8*)in, (U32)size, 12, out, state);
			state.declarations.clear();	// Scene::m_vertexDeclarations, read below
			state.chunk = -1;			// [BlurX]

			// BinaryInputArchive_t::Open
			ar.Read(0, 4);
			S32 archiveVersion;
			ar >> archiveVersion;
			if (archiveVersion != 0)
				ar.Fail("archive version %d", archiveVersion);

			S32 version = -1;
			ar >> version;

			RenderingData::Target::Scene scene;
			ar >> scene;

			S32 nBlocks;
			if (version == 0)
			{
				bool loadBlocks;
				ar >> loadBlocks;
				nBlocks = loadBlocks ? scene.GetNumResourceBlocks() : 0;
			}
			else
				ar >> nBlocks;

			for (int i = 0; i < nBlocks; i++)
			{
				state.block = i;
				state.indexBufferCount = 0;
				state.vertexBufferCount = 0;
				ar >> scene.GetResourceBlock(i);
			}

			// [BlurX] A version 2+ Kanzen scene: U8, then (when 1) the shared-texture
			// list, after the blocks. Pass 1 reads it here and records where; later
			// passes have read it at its Blur 1 place in the Scene and skip it.
			if (state.kanzenScene >= 2 && !ar.GetFailed())
			{
				if (state.kanzenTailEnd)
					ar.InSeek(state.kanzenTailEnd);
				else
				{
					const U8 hasTextures = ar.InU8();
					if (hasTextures > 1)
						ar.Fail("Kanzen shared-texture flag %u", hasTextures);
					else if (hasTextures)
					{
						state.kanzenTexturesAt = ar.InPos();
						state.kanzenTexturesInPlace = true;
						RenderingData::Target::SharedTextureList list;
						ar >> list;
						state.kanzenTexturesInPlace = false;
					}
					state.kanzenTailEnd = ar.InPos();
				}
			}

			if (!ar.GetFailed() && ar.InPos() != size)
				ar.Fail("%u bytes left after the last resource block", (unsigned)(size - ar.InPos()));
			error = ar.Error();
			return !ar.GetFailed() && !ar.GetUnconverted();
		}

		// One levelstream chunk (StreamData::UpdateAsynch, STATE_ASYNCH_LOAD): an
		// archive holding S32 nItems and then one LodReader<Resources> per item.
		bool ReadChunkPass(const U8* in, size_t size, const std::vector<std::pair<S32, S32> >& items,
			std::vector<U8>& out, SceneState& state, std::string& error)
		{
			out.clear();
			out.reserve(size + size / 2);
			if (!CheckArchiveHeader(in, size, error))
				return false;
			PutArchiveHeader(out);

			Arch::X360SceneTranscoder ar((const U8*)in, (U32)size, 12, out, state);
			ar.Read(0, 4);
			S32 archiveVersion;
			ar >> archiveVersion;
			S32 nItems;
			ar >> nItems;
			const size_t nItemsAt = ar.OutPos() - 4;	// [BlurX]
			if (nItems != (S32)items.size())
				ar.Fail("chunk holds %d items where the scene lists %u", nItems, (unsigned)items.size());

			S32 dropped = 0;
			for (S32 i = 0; i < nItems && !ar.GetFailed(); ++i)
			{
				state.block = items[i].first;
				state.indexBufferCount = 0;
				state.vertexBufferCount = 0;
				state.kanzenItemWord = 0;
				const size_t itemAt = ar.OutPos();
				RenderingData::Target::ResourceBlock::Resources resources;
				RenderingData::Target::LodReader<RenderingData::Target::ResourceBlock::Resources> reader(0, 0, resources, items[i].second);
				ar >> reader;
				// [BlurX] A Kanzen split-part copy of a (block, lod) item: its textures are
				// joined into part 0's item (X360Convert::KanzenSplitTexture), and Blur 1
				// loads a block's lod from one chunk only (StreamData::UpdateAsynch asserts
				// each item raises the block's lod). Left out of the chunk and of the scene's
				// chunk table (ChunkInfo overload).
				if (state.kanzenItemWord >= 1)
				{
					ar.OutTruncate(itemAt);
					++dropped;
					if (state.pass == 1)
					{
						char key[32];
						sprintf_s(key, "%d/%d", state.chunk, i);
						state.kanzenDropItems.insert(key);
					}
				}
			}
			if (dropped)
				ar.PatchU32(nItemsAt, (U32)(nItems - dropped));

			// The rest of the chunk is never read (the engine reads nItems items).
			if (!ar.GetFailed() && ar.InPos() < size)
				ar.Read(0, (U32)(size - ar.InPos()));
			error = ar.Error();
			return !ar.GetFailed() && !ar.GetUnconverted();
		}

		// After pass 1: every draw range, grouped per index buffer.
		bool CollectSegments(SceneState& state, std::string& error)
		{
			for (size_t i = 0; i < state.lods.size(); ++i)
			{
				const SceneState::LodRecord& r = state.lods[i];
				if (r.buffer.second < 0)
				{
					char text[128];
					sprintf_s(text, "draw range %u has no index buffer (list index %d)", (unsigned)i, r.listIndex);
					error = text;
					return false;
				}
				if (r.nIndices > 0)
					state.segments[r.buffer].push_back(SceneState::Range(r.offset, r.nIndices));
			}
			for (std::map<SceneState::BufferId, std::vector<SceneState::Range> >::iterator it = state.segments.begin(); it != state.segments.end(); ++it)
			{
				std::sort(it->second.begin(), it->second.end());
				it->second.erase(std::unique(it->second.begin(), it->second.end()), it->second.end());
			}
			return true;
		}

		// [Blur1Multi] After pass 2, PS3 sources: move every PrimitiveStream vertex
		// offset read in this pass to its buffer's widened vertices, and check each
		// primitive's vertex declaration against that buffer's layout (the element
		// the declaration names must be in the buffer, same type, same new offset).
		bool PatchStreams(SceneState& state, std::vector<U8>& out, std::string& error)
		{
			if (!ExpandsShhd())
				return true;
			char text[256];
			for (size_t i = 0; i < state.streams.size(); ++i)
			{
				const SceneState::StreamRecord& r = state.streams[i];
				if (r.buffer == SceneState::BufferId(-1, -1))
				{
					// VertexBufferSource (-1,-1): no scene buffer; the node's slot is left
					// empty (target/render.cpp:335-347) for a buffer supplied at run time.
					// Nothing to move, unless its declaration stream would widen.
					if (r.declaration >= 0 && r.declaration < (S32)state.declarations.size())
					{
						const std::vector<SceneState::DeclElement>& decl = state.declarations[r.declaration];
						for (size_t d = 0; d < decl.size(); ++d)
							if (decl[d].stream == r.streamIndex && (decl[d].type == RenderingData::VERTEX_ELEMENT_TYPE_SHHD_3N || decl[d].newOffset != decl[d].offset))
							{
								sprintf_s(text, "vertex declaration %d changes element %u of stream %d, which has no scene vertex buffer",
									r.declaration, (unsigned)d, r.streamIndex);
								error = text;
								return false;
							}
					}
					if (state.countStats)
						++g_stats.streamsWithoutBuffer;
					continue;
				}
				std::map<SceneState::BufferId, SceneState::VertexLayout>::const_iterator it = state.vertexLayouts.find(r.buffer);
				if (it == state.vertexLayouts.end() && r.buffer.first >= 0)
				{
					// A resource block the file never holds (no ChunkInfo item names it, and
					// the scene does not carry it): the node's slot never gets a buffer
					// (target/render.cpp:335-376) and the engine skips the primitive
					// (target/context.cpp:486-496), so the offset is never used. A block the
					// file does hold must have this buffer.
					std::map<SceneState::BufferId, SceneState::VertexLayout>::const_iterator b =
						state.vertexLayouts.lower_bound(SceneState::BufferId(r.buffer.first, -1));
					if (b == state.vertexLayouts.end() || b->first.first != r.buffer.first)
					{
						if (state.countStats)
							++g_stats.streamsBlockAbsent;
						continue;
					}
				}
				if (it == state.vertexLayouts.end())
				{
					sprintf_s(text, "primitive stream %u (stream %d, list index %d, declaration %d, vertex offset %d) has no vertex buffer read (%d,%d)",
						(unsigned)i, r.streamIndex, r.listIndex, r.declaration, r.vertexOffset, r.buffer.first, r.buffer.second);
					error = text;
					return false;
				}
				const SceneState::VertexLayout& layout = it->second;
				if (layout.newStride != layout.stride)
				{
					if (r.vertexOffset % layout.stride)
					{
						sprintf_s(text, "vertex offset %d of vertex buffer (%d,%d) is not whole %d-byte vertices", r.vertexOffset, r.buffer.first, r.buffer.second, layout.stride);
						error = text;
						return false;
					}
					const S32 offset = r.vertexOffset / layout.stride * layout.newStride;
					memcpy(&out[r.outPos], &offset, 4);
					if (state.countStats)
						++g_stats.streamsPatched;
				}
				if (r.declaration < 0)
					continue;
				if (r.declaration >= (S32)state.declarations.size())
				{
					sprintf_s(text, "primitive stream %u names vertex declaration %d of %u", (unsigned)i, r.declaration, (unsigned)state.declarations.size());
					error = text;
					return false;
				}
				const std::vector<SceneState::DeclElement>& decl = state.declarations[r.declaration];
				for (size_t d = 0; d < decl.size(); ++d)
				{
					if (decl[d].stream != r.streamIndex)
						continue;
					size_t e = 0;
					while (e < layout.offsets.size() && layout.offsets[e] != decl[d].offset)
						++e;
					if (e == layout.offsets.size() || layout.types[e] != decl[d].type || layout.newOffsets[e] != decl[d].newOffset)
					{
						sprintf_s(text, "vertex declaration %d element %u (stream %d, offset %d, type %d) does not match vertex buffer (%d,%d)",
							r.declaration, (unsigned)d, decl[d].stream, decl[d].offset, decl[d].type, r.buffer.first, r.buffer.second);
						error = text;
						return false;
					}
					if (state.countStats)
						++g_stats.declElementsChecked;
				}
			}
			return true;
		}

		// After pass 2: move every draw range read in this pass to its re-stitched lod.
		bool PatchLods(SceneState& state, std::vector<U8>& out, std::string& error)
		{
			for (size_t i = 0; i < state.lods.size(); ++i)
			{
				const SceneState::LodRecord& r = state.lods[i];
				S32 offset = r.offset, count = r.nIndices;
				if (count > 0)
				{
					std::map<SceneState::Range, SceneState::Range>& m = state.remap[r.buffer];
					std::map<SceneState::Range, SceneState::Range>::const_iterator it = m.find(SceneState::Range(r.offset, r.nIndices));
					if (it == m.end())
					{
						// [Blur1Multi] An index buffer the file never holds: its resource
						// block is in no ChunkInfo item and not in the scene, so the engine
						// never binds it (target/render.cpp:358-364, 512-518) and skips the
						// primitive (target/context.cpp:422-425). The range is never drawn;
						// it keeps the console values.
						if (state.indexBuffersRead.find(r.buffer) == state.indexBuffersRead.end())
						{
							if (state.countStats)
								++g_stats.rangesBlockAbsent;
							continue;
						}
						char text[160];
						sprintf_s(text, "draw range (%d,%d) of index buffer (%d,%d) was not re-stitched", r.offset, r.nIndices, r.buffer.first, r.buffer.second);
						error = text;
						return false;
					}
					offset = it->second.first;
					count = it->second.second;
				}
				else
				{
					std::map<S32, S32>& b = state.boundaries[r.buffer];
					std::map<S32, S32>::const_iterator it = b.find(r.offset);
					if (it != b.end())
						offset = it->second;
				}
				memcpy(&out[r.outPos], &offset, 4);
				memcpy(&out[r.outPos + 4], &count, 4);
			}
			return PatchStreams(state, out, error);
		}

		// 'final': this pass's output is the one kept (counted in SceneTranscodeStats).
		void StartPass(SceneState& state, int pass, bool final = false)
		{
			state.pass = pass;
			state.countStats = final;
			state.lods.clear();
			std::vector<std::vector<size_t> >().swap(state.nodes);
			state.streams.clear();
			std::vector<std::vector<size_t> >().swap(state.streamNodes);
			state.chunks.clear();
		}
	}

	const SceneTranscodeStats& GetSceneTranscodeStats()
	{
		return g_stats;
	}

	bool TranscodeSceneArchive(const unsigned char* in, size_t size, std::vector<unsigned char>& out, std::string& error)
	{
		RegisterSceneTranscodeStreamers();
		if (SceneTraceOn())
			SceneTrace(0, "=== scene archive, bytes:", (unsigned)size, 0);	// [BlurX]

		SceneState state;
		StartPass(state, 1);
		if (!ReadScenePass(in, size, out, state, error) || !CollectSegments(state, error))
			return false;
		StartPass(state, 2, true);
		if (!ReadScenePass(in, size, out, state, error) || !PatchLods(state, out, error))
			return false;
		if (state.kanzenDanglingRanges)	// [BlurX]
			LOGINFO(g_sceneLog, "X360Convert: %u draw ranges past the end of their index buffer drawn empty (Kanzen coarse lods)", state.kanzenDanglingRanges);
		if (state.kanzenPartialTiers)	// [BlurX]
			LOGINFO(g_sceneLog, "X360Convert: %u texture lods shipping part of their surface written empty (Kanzen partial tiers)", state.kanzenPartialTiers);
		if (state.kanzenSplitJoined || state.kanzenSplitIncomplete)	// [BlurX]
			LOGINFO(g_sceneLog, "X360Convert: %u split texture lods joined, %u with parts missing", state.kanzenSplitJoined, state.kanzenSplitIncomplete);

		LOGDEBUG(g_sceneLog, "X360Convert: scene %u -> %u bytes, %u draw ranges, %u strips (%u odd-length), %u normals repacked",
			(unsigned)size, (unsigned)out.size(), (unsigned)state.lods.size(), state.strips, state.reversedAmbiguous, state.normalsRepacked);
		return true;
	}

	struct C_LevelConversion::Impl
	{
		struct Chunk
		{
			U64 inOffset;
			U32 inSize;
			std::vector<std::pair<S32, S32> > items;
			U64 outOffset;
			U32 outSize;
		};

		CRITICAL_SECTION lock;
		I_ChunkSource* pSource;
		SceneState state;
		std::vector<Chunk> chunks;
		std::vector<U8> scene;
		U64 streamLength;

		// the chunks converted most recently (a chunk read spans one chunk)
		std::list<std::pair<size_t, std::vector<U8> > > recent;

		Impl() : pSource(0), streamLength(0) { InitializeCriticalSection(&lock); }
		~Impl() { delete pSource; DeleteCriticalSection(&lock); }

		// Pass 2 with 'patch' moves the chunk's draw ranges; that needs every index
		// buffer's re-stitched ranges, so the layout pass converts without it (the
		// patch changes no size).
		bool ConvertChunk(size_t i, int pass, bool patch, std::vector<U8>& out, std::string& error)
		{
			const Chunk& c = chunks[i];
			std::vector<U8> in;
			if (!pSource->Read(c.inOffset, c.inSize, in) || in.size() != c.inSize)
			{
				char text[96];
				sprintf_s(text, "cannot read chunk %u (%u bytes)", (unsigned)i, c.inSize);
				error = text;
				return false;
			}
			StartPass(state, pass, patch);
			state.chunk = (int)i;	// [BlurX] split texture keys
			if (!ReadChunkPass(&in[0], in.size(), c.items, out, state, error))
			{
				char text[48];
				sprintf_s(text, "chunk %u: ", (unsigned)i);
				error = text + error;
				return false;
			}
			if (patch && !PatchLods(state, out, error))
			{
				char text[48];
				sprintf_s(text, "chunk %u: ", (unsigned)i);
				error = text + error;
				return false;
			}
			return true;
		}
	};

	C_LevelConversion::C_LevelConversion() : m_pImpl(new Impl) {}
	C_LevelConversion::~C_LevelConversion() { delete m_pImpl; }

	const std::vector<unsigned char>& C_LevelConversion::Scene() const { return m_pImpl->scene; }
	unsigned long long C_LevelConversion::StreamLength() const { return m_pImpl->streamLength; }

	bool C_LevelConversion::Prepare(const unsigned char* sceneIn, size_t sceneSize, I_ChunkSource* source, std::string& error)
	{
		RegisterSceneTranscodeStreamers();
		Impl& m = *m_pImpl;
		m.pSource = source;
		SceneState& state = m.state;
		state.levelScene = true;	// [BlurX]

		// Pass 1: the scene (its chunk table and any draw ranges), then every chunk.
		StartPass(state, 1);
		std::vector<U8> scratch;
		if (!ReadScenePass(sceneIn, sceneSize, scratch, state, error))
		{
			error = "scene: " + error;
			return false;
		}
		std::vector<SceneState::LodRecord> lods = state.lods;
		const std::vector<SceneState::ChunkRecord> chunkRecords = state.chunks;
		for (size_t i = 0; i < chunkRecords.size(); ++i)
		{
			Impl::Chunk c;
			c.inOffset = chunkRecords[i].offset;
			c.inSize = (U32)chunkRecords[i].compressedSize;
			c.items = chunkRecords[i].items;
			c.outOffset = 0;
			c.outSize = 0;
			m.chunks.push_back(c);
		}
		for (size_t i = 0; i < m.chunks.size(); ++i)
		{
			if (!m.ConvertChunk(i, 1, false, scratch, error))
				return false;
			lods.insert(lods.end(), state.lods.begin(), state.lods.end());
		}
		state.lods.swap(lods);
		if (!CollectSegments(state, error))
			return false;

		// Pass 2 over every chunk: the PC stream layout (and every index buffer's
		// re-stitched ranges, which nodes in other chunks and the scene refer to).
		U64 at = 0;
		for (size_t i = 0; i < m.chunks.size(); ++i)
		{
			if (!m.ConvertChunk(i, 2, false, scratch, error))
				return false;
			at = (at + 15) & ~(U64)15;
			m.chunks[i].outOffset = at;
			m.chunks[i].outSize = (U32)scratch.size();
			at += scratch.size();
		}
		m.streamLength = at;

		// Pass 2 over the scene, with the chunk table moved to the PC layout.
		StartPass(state, 2, true);
		if (!ReadScenePass(sceneIn, sceneSize, m.scene, state, error) || !PatchLods(state, m.scene, error))
		{
			error = "scene: " + error;
			return false;
		}
		if (state.chunks.size() != m.chunks.size())
		{
			error = "scene chunk table changed between passes";
			return false;
		}
		for (size_t i = 0; i < state.chunks.size(); ++i)
		{
			const SceneState::ChunkRecord& r = state.chunks[i];
			const U64 offset = m.chunks[i].outOffset, size = m.chunks[i].outSize;
			memcpy(&m.scene[r.outPos], &offset, 8);
			memcpy(&m.scene[r.outPos + 8], &size, 8);
			if (r.hasOriginalSize)
				memcpy(&m.scene[r.outPos + 16], &size, 8);
		}

		LOGINFO(g_sceneLog, "X360Convert: level scene %u -> %u bytes, %u chunks, stream %u -> %u bytes, %u strips (%u odd-length), %u normals repacked, %u triangles checked",
			(unsigned)sceneSize, (unsigned)m.scene.size(), (unsigned)m.chunks.size(),
			m.chunks.empty() ? 0u : (unsigned)(m.chunks.back().inOffset + m.chunks.back().inSize), (unsigned)m.streamLength,
			state.strips, state.reversedAmbiguous, state.normalsRepacked, state.trianglesChecked);
		if (state.kanzenDanglingRanges)	// [BlurX]
			LOGINFO(g_sceneLog, "X360Convert: level: %u draw ranges past the end of their index buffer drawn empty (Kanzen coarse lods)", state.kanzenDanglingRanges);
		if (state.kanzenPartialTiers)	// [BlurX]
			LOGINFO(g_sceneLog, "X360Convert: level: %u texture lods shipping part of their surface written empty (Kanzen partial tiers)", state.kanzenPartialTiers);
		if (!state.kanzenSplits.empty())	// [BlurX]
		{
			unsigned joined = 0, missing = 0, parts = 0;
			for (std::map<std::string, std::shared_ptr<KanzenSplitTexture> >::const_iterator it = state.kanzenSplits.begin(); it != state.kanzenSplits.end(); ++it)
			{
				bool whole = it->second->consistent;
				for (size_t i = 0; i < it->second->have.size(); ++i)
					whole = whole && it->second->have[i] != 0;
				++(whole ? joined : missing);
				parts += it->second->count;
			}
			LOGINFO(g_sceneLog, "X360Convert: level: %u split texture lods (%u parts) joined, %u with parts missing (STOPGAP: empty above lod 0, uncovered blocks zero at lod 0); %u split-part chunk items dropped",
				joined, parts, missing, (unsigned)state.kanzenDropItems.size());
			LOGINFO(g_sceneLog, "X360Convert: level: %u textures with lod 0 decoded short of its chain; later lods trimmed to match", (unsigned)state.kanzenLodTrim.size());
			LOGINFO(g_sceneLog, "X360Convert: level: render passes:%s", state.kanzenPassNames.c_str());
		}
		return true;
	}

	bool C_LevelConversion::ReadStream(unsigned long long offset, void* dst, unsigned size, std::string& error)
	{
		Impl& m = *m_pImpl;
		U8* p = (U8*)dst;
		memset(p, 0, size);
		EnterCriticalSection(&m.lock);
		bool ok = true;
		for (size_t i = 0; ok && i < m.chunks.size(); ++i)
		{
			const Impl::Chunk& c = m.chunks[i];
			const U64 begin = c.outOffset, end = c.outOffset + c.outSize;
			if (end <= offset || begin >= offset + size)
				continue;
			const std::vector<U8>* pBytes = 0;
			for (std::list<std::pair<size_t, std::vector<U8> > >::iterator it = m.recent.begin(); it != m.recent.end(); ++it)
				if (it->first == i)
				{
					pBytes = &it->second;
					break;
				}
			if (!pBytes)
			{
				m.recent.push_front(std::make_pair(i, std::vector<U8>()));
				if (!m.ConvertChunk(i, 2, true, m.recent.front().second, error) || m.recent.front().second.size() != c.outSize)
				{
					if (error.empty())
						error = "chunk converted to a different size than when the stream was laid out";
					m.recent.pop_front();
					ok = false;
					break;
				}
				pBytes = &m.recent.front().second;
				while (m.recent.size() > 4)
					m.recent.pop_back();
			}
			const U64 from = begin > offset ? begin : offset;
			const U64 to = end < offset + size ? end : offset + size;
			memcpy(p + (from - offset), &(*pBytes)[(size_t)(from - begin)], (size_t)(to - from));
		}
		LeaveCriticalSection(&m.lock);
		return ok;
	}

	// C_RenderLevel::Read's PVSData block: an Arch archive holding one
	// Pvs::Target::PvsData (world_render/pvs.h), through the same archive.
	bool TranscodePvsArchive(const unsigned char* in, size_t size, std::vector<unsigned char>& out, std::string& error)
	{
		out.clear();
		if (size < 20 || memcmp(in, "ARCH", 4) != 0)
		{
			error = "not an ARCH stream";
			return false;
		}
		U32 one, swap;
		memcpy(&one, in + 4, 4);
		memcpy(&swap, in + 8, 4);
		if ((one != 1) == (swap != 0))
		{
			// The retail 360 levels' PVS archives were written without swapping (the
			// header says so and BinaryInputArchive swaps on the 360 while reading):
			// they already are the PC bytes.
			out.assign(in, in + size);
			return true;
		}
		const U32 header[3] = { 0x48435241 /* "ARCH" */, 1, 0 };
		out.insert(out.end(), (const U8*)header, (const U8*)header + 12);
		SceneState state;
		state.pass = 2;
		Arch::X360SceneTranscoder ar(in, (U32)size, 12, out, state);
		ar.Read(0, 4);
		S32 archiveVersion;
		ar >> archiveVersion;
		Pvs::Target::PvsData pvs;
		ar >> pvs;
		if (!ar.GetFailed() && ar.InPos() != size)
			ar.Fail("%u bytes left after the PVS data", (unsigned)(size - ar.InPos()));
		error = ar.Error();
		return !ar.GetFailed() && !ar.GetUnconverted();
	}
}
