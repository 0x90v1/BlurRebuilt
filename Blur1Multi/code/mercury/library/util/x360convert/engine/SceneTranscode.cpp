// [Blur1Multi] Xbox 360 -> PC scene archive conversion. See SceneTranscode.h.
//
// Payload rules, all from the exporter in renderingdata/source/noninterface/build:
//  - Arch::Serialiser blocks (archive.cpp Serialise(VertexBuffer/IndexBuffer)):
//    360 writes XMEM_COMPRESSION when the payload is 128 bytes or more, PC writes
//    NO_COMPRESSION with the endian flag 0; alignment 16 on both.
//  - Vertices: each element's scalars go through the endian-swapping serialiser
//    at their own width (S16/F16 pairs and quads per 16 bits, F32 per 32 bits,
//    U8_4/U8_4N/SHHD_3N/SDDD_3N as one U32).
//  - Normals: Scene::Finish(PC) runs ChangeVertexElementType(SHHD_3N, U8_4N)
//    before anything else, i.e. PC's U8_4N normal is a function of the SHHD_3N
//    value the 360 file carries (vertexbuffer.cpp ChangeDataType).
//  - Indices: Scene::BuildStrips strips each index buffer lod with the ATG
//    stripper; 360/PS3 pass ATGTRISTRIPPER_USE_RESET_INDEX, which rewrites the
//    stitched strip through ConvertDegensToResetIndex (atg/AtgStrip.cpp). PC keeps
//    the stitched strip; each draw range covers one stripped lod.

#include "stdafx.h"

#include "SceneTranscode.h"

#include "renderingdata/noninterface/archive.inl"

#include <misc_shared/compression/xmemcompress.h>
#include <x360convert/X360Convert.h>
#include <world_render/WorldRender.h>

#include <algorithm>
#include <stdarg.h>
#include <stdio.h>

namespace Arch
{
	X360SceneTranscoder::X360SceneTranscoder(const U8* pIn, U32 inSize, U32 startAt, std::vector<U8>& out, X360Convert::SceneState& state)
		: m_in(const_cast<U8*>(pIn), inSize), m_pInBase(pIn), m_out(out), m_state(state), m_version(0), m_failed(false), m_unconverted(false)
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
	namespace
	{
		Debug::Logger& g_sceneLog = Debug::GetLogger("Blur1Multi.X360Convert");

		// An Arch::Serialiser block (serialiser.h) read from the 360 stream: its tag
		// and version have already gone to the output. Returns the logical payload.
		void ReadSerialiserBlock(Arch::X360SceneTranscoder& ar, std::vector<U8>& data, U32& alignment)
		{
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
		if (value != PLATFORM_TYPE_360)
			ar.Fail("platform type %d where the 360 exporter writes %d", value, (int)PLATFORM_TYPE_360);
		ar.PatchU32(at, PLATFORM_TYPE_PC);
		t = PLATFORM_TYPE_PC;
	}

	// Scene::Finish(PC): ChangeVertexElementType(SHHD_3N, U8_4N). The value keeps
	// the 360 type so the vertex buffer reader knows what it is converting.
	void Deserialise(Arch::X360SceneTranscoder& ar, VertexElementType& t)
	{
		const size_t at = ar.OutPos();
		S32 value;
		ar & value;
		t = (VertexElementType) value;
		if (t == VERTEX_ELEMENT_TYPE_SHHD_3N)
			ar.PatchU32(at, VERTEX_ELEMENT_TYPE_U8_4N);
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

		ar & t.primitiveStreams;

		t.primitiveType = primitiveType;
		t.blendState = 0;
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

	namespace Target
	{
		void Deserialise(Arch::X360SceneTranscoder& ar, IndexBuffer& t)
		{
			X360Convert::SceneState& state = ar.State();
			const X360Convert::SceneState::BufferId id(state.block, state.indexBufferCount++);

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
				for (size_t r = 0; r <= ranges.size(); ++r)
				{
					const S32 first = r < ranges.size() ? ranges[r].first : size;
					const S32 count = r < ranges.size() ? ranges[r].second : 0;
					if (first < at)
					{
						if (remap.find(ranges[r]) == remap.end())
							ar.Unconverted("overlapping draw ranges in index buffer (%d,%d)", id.first, id.second);
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
					result.insert(result.end(), lod.begin(), lod.end());
					remap[ranges[r]] = X360Convert::SceneState::Range(newFirst, (S32)lod.size());
					boundaries[first] = newFirst;
					at = first + count;
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
			state.vertexBufferCount++;

			S32 size;
			VertexFormat vertexFormat;
			ar & vertexFormat;
			ar & size;

			const U32 stride = vertexFormat.GetDataSize();
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

			for (S32 v = 0; v < size; ++v)
			{
				for (int e = 0; e < vertexFormat.GetNumVertexElements(); ++e)
				{
					U8* p = &data[v * stride + vertexFormat.GetOffset(e)];
					switch (vertexFormat.GetVertexElement(e).type)
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
						case VERTEX_ELEMENT_TYPE_SDDD_3N:
							X360Convert::Swap32(p);
							break;
						case VERTEX_ELEMENT_TYPE_SHHD_3N:
						{
							const U32 shhd = ((U32)p[0] << 24) | ((U32)p[1] << 16) | ((U32)p[2] << 8) | p[3];
							const U32 n = X360Convert::RepackNormal(shhd);
							memcpy(p, &n, 4);
							++state.normalsRepacked;
							break;
						}
						default:
							ar.Unconverted("vertex element type %d", (int)vertexFormat.GetVertexElement(e).type);
					}
				}
			}

			if (ar.GetVersion() > 0)
				X360Convert::WriteSerialiserBlock(ar, data, alignment);
			else if (!data.empty())
				ar.OutBytes(&data[0], (U32)data.size());
		}

		// Target::Deserialise(Archive&, VertexDeclaration&) without the device
		// declaration; the element types go through the VertexElementType overload.
		void Deserialise(Arch::X360SceneTranscoder& ar, VertexDeclaration& t)
		{
			S32 nElements;
			ar & nElements;
			for (int i = 0; i < nElements; i++)
			{
				RenderingData::VertexDeclaration::VertexElement element;
				ar & element;
			}
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
			PlatformType platformType;
			ar & platformType;

			Btl::String name;
			if (t.m_lod == 0)
			{
				ar & name;
				SamplerState samplerState;
				ar & samplerState;
				U32 hashValue;
				ar & hashValue;
			}

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
				X360Convert::ReadSerialiserBlock(ar, data, alignment);
			}
			if (ar.GetFailed())
				return;

			std::vector<U8> pc;
			if (!data.empty())
			{
				std::string error;
				if (header.empty())
					ar.Unconverted("texture '%s' lod %d has data but no header", name.Chars(), t.m_lod);
				else if (X360Convert::ConvertTextureLodData(&header[0], t.m_lod, &data[0], data.size(), pc, error) != X360Convert::E_Result_Converted)
					ar.Unconverted("texture '%s' lod %d data: %s", name.Chars(), t.m_lod, error.c_str());
			}

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
		// One pass of WorldRender::ReadScene over the 360 bytes. Returns false with
		// 'error' set on failure.
		bool ReadScenePass(const U8* in, size_t size, std::vector<U8>& out, SceneState& state, std::string& error)
		{
			out.clear();
			out.reserve(size * 2);

			// Arch::BinaryInputArchive: "ARCH", U32 1 in the writer's byte order, U32
			// swap flag. The 360 exporter ran on PC with endian swapping on.
			if (size < 20 || memcmp(in, "ARCH", 4) != 0)
			{
				error = "not an ARCH stream";
				return false;
			}
			U32 one, swap;
			memcpy(&one, in + 4, 4);
			memcpy(&swap, in + 8, 4);
			const bool endianSwap = (one != 1) != (swap != 0);
			if (!endianSwap)
			{
				error = "archive is not endian-swapped (already PC data?)";
				return false;
			}
			const U32 header[3] = { 0x48435241 /* "ARCH" */, 1, 0 };
			out.insert(out.end(), (const U8*)header, (const U8*)header + 12);

			Arch::X360SceneTranscoder ar((const U8*)in, (U32)size, 12, out, state);

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

			if (!ar.GetFailed() && ar.InPos() != size)
				ar.Fail("%u bytes left after the last resource block", (unsigned)(size - ar.InPos()));
			error = ar.Error();
			return !ar.GetFailed() && !ar.GetUnconverted();
		}
	}

	bool TranscodeSceneArchive(const unsigned char* in, size_t size, std::vector<unsigned char>& out, std::string& error)
	{
		RegisterSceneTranscodeStreamers();

		SceneState state;
		{
			state.pass = 1;
			if (!ReadScenePass(in, size, out, state, error))
				return false;

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

			std::vector<std::vector<size_t> >().swap(state.nodes);
			state.lods.clear();
			state.pass = 2;
			if (!ReadScenePass(in, size, out, state, error))
				return false;
		}

		// Move every draw range to its re-stitched lod.
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

		LOGDEBUG(g_sceneLog, "X360Convert: scene %u -> %u bytes, %u draw ranges, %u strips (%u odd-length), %u normals repacked",
			(unsigned)size, (unsigned)out.size(), (unsigned)state.lods.size(), state.strips, state.reversedAmbiguous, state.normalsRepacked);
		return true;
	}
}
