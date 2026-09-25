// [Blur1Multi] Xbox 360 -> PC conversion of rendering-data scene archives (T072).
//
// A scene archive (the Scene block of a .model, and of level render files) is
// an Arch "ARCH" stream written by the PC-hosted exporter with endian swapping
// for the 360. Its grammar is whatever the engine's reader asks for, so the
// converter runs that reader (RenderingData::Target::Deserialise, instantiated
// for X360SceneTranscoder at the end of target/archive.cpp) over an archive
// that takes every scalar big-endian from the 360 bytes and appends it
// little-endian to the output. The GPU payloads the reader would hand to the
// device are converted by the overloads declared at the bottom, which follow
// the exporter (renderingdata/source/noninterface/build): per-element vertex
// swaps, SHHD_3N normals re-packed as U8_4N the way Scene::Finish does for PC,
// index strips re-stitched with degenerates the way the ATG stripper does
// without ATGTRISTRIPPER_USE_RESET_INDEX, and draw ranges moved to match.
// PS3 sources (T076) take the same path; there SHHD_3N becomes F16_4 (x, y, z, 1)
// for the translated RSX programs, and the vertex layouts, declarations and
// primitive-stream byte offsets widen to match (SceneState::VertexLayout).
#pragma once

#include <misc_shared/archive/archive.h>
#include <misc_shared/archive/btl.h>
#include <misc_shared/archive/serialiser.h>
#include <system/serialise/C_MemoryStream.h>
#include <renderingdata/target.h>
#include <renderingdata/streaming.h>

#include <map>
#include <set>
#include <string>
#include <vector>

namespace X360Convert
{
	// What the two passes over one scene archive share.
	struct SceneState
	{
		typedef std::pair<S32, S32> Range;		// (first index, index count) in one index buffer
		typedef std::pair<S32, S32> BufferId;	// ResourceIndex (block, index)

		// A draw range (RenderListNode::Primitive::Lod) as read, and where its
		// offset/count were written, so pass 2 can move them.
		struct LodRecord
		{
			size_t outPos;
			S32 offset, nIndices;
			S32 listIndex;				// Primitive::indexBufferIndex (index into the node's IndexBufferSource)
			BufferId buffer;			// resolved when the node is complete; (-1,-1) if not
		};

		int pass;
		std::vector<LodRecord> lods;
		std::vector<std::vector<size_t> > nodes;	// lod records of the RenderListNodes being read, innermost last
		S32 primitiveIndexBuffer;
		S32 block;									// resource block being read (ReadScene order)
		S32 indexBufferCount, vertexBufferCount;	// buffers read so far in that block

		// A levelstream chunk as the scene's StreamData describes it (ChunkInfo),
		// and where its offset and sizes were written.
		struct ChunkRecord
		{
			size_t outPos;				// ChunkInfo::m_offset in the output
			bool hasOriginalSize;		// ChunkInfo version > 0
			U64 offset, compressedSize, originalSize;
			std::vector<std::pair<S32, S32> > items;	// (block, lod)
		};
		std::vector<ChunkRecord> chunks;

		std::map<BufferId, std::vector<Range> > segments;			// pass 1 result: draw ranges per index buffer
		std::map<BufferId, std::map<Range, Range> > remap;			// pass 2: old range -> new range
		std::map<BufferId, std::map<S32, S32> > boundaries;			// pass 2: old segment boundary -> new
		std::set<BufferId> indexBuffersRead;						// every index buffer the file holds (all passes)

		// [Blur1Multi] PS3 sources (T076): every SHHD_3N element becomes F16_4, so
		// vertex layouts grow by 4 bytes per SHHD_3N element. What moves with them:
		// VertexFormat DataSize and Offsets, VertexDeclaration element offsets, and
		// PrimitiveStream::vertexOffset, a byte offset into its vertex buffer
		// (build/resourceblockbuild.cpp:409 'data.offset = offset * vertexSize').

		// One vertex layout as the console file has it and as the PC file gets it.
		struct VertexLayout
		{
			S32 stride, newStride;
			std::vector<S32> types, offsets, newOffsets;
		};
		std::map<BufferId, VertexLayout> vertexLayouts;			// every vertex buffer read; kept across passes

		// VertexDeclaration elements as read: (stream, offset, type) per scene declaration.
		struct DeclElement { S32 stream, offset, newOffset, type; };
		std::vector<std::vector<DeclElement> > declarations;

		// A PrimitiveStream as read, and where its vertexOffset was written.
		struct StreamRecord
		{
			size_t outPos;
			S32 vertexOffset;
			S32 listIndex;				// PrimitiveStream::vertexBuffer (index into the node's VertexBufferSource)
			S32 streamIndex;
			S32 declaration;			// the primitive's vertex declaration; -1 for instance streams
			BufferId buffer;			// resolved when the node is complete; (-1,-1) if not
		};
		std::vector<StreamRecord> streams;
		std::vector<std::vector<size_t> > streamNodes;	// stream records of the RenderListNodes being read
		S32 primitiveDeclaration;
		bool countStats;								// this pass produces the output: add to SceneTranscodeStats

		// statistics for the log
		unsigned normalsRepacked, strips, reversedAmbiguous, trianglesChecked;

		SceneState() : pass(0), primitiveIndexBuffer(-1), block(-1), indexBufferCount(0), vertexBufferCount(0),
			primitiveDeclaration(-1), countStats(false), normalsRepacked(0), strips(0), reversedAmbiguous(0), trianglesChecked(0) {}
	};

	// [Blur1Multi] Running totals over every scene converted in this process, for
	// verification harnesses and the log.
	struct SceneTranscodeStats
	{
		unsigned normalsRepacked;		// SHHD_3N -> U8_4N (Xbox 360 sources)
		unsigned shhdExpanded;			// SHHD_3N -> F16_4 element values (PS3 sources)
		unsigned shhdExtreme;			// of those, components at the negative extreme (-1024 or -512)
		unsigned strips;
		unsigned trianglesChecked;
		unsigned streamsPatched;		// PrimitiveStream vertex offsets moved
		unsigned streamsWithoutBuffer;	// PrimitiveStreams whose VertexBufferSource is (-1,-1) (run-time buffers)
		unsigned streamsBlockAbsent;		// PrimitiveStreams whose vertex buffer's resource block the file never holds
		unsigned rangesBlockAbsent;		// draw ranges whose index buffer's resource block the file never holds
		unsigned declElementsChecked;	// declaration elements matched against their vertex buffer's layout
		unsigned byUsage[16][8];		// SHHD_3N vertex-format elements expanded, by usage and usage index
	};
	const SceneTranscodeStats& GetSceneTranscodeStats();

	// Runs ReadScene's reads (WorldRender::ReadScene) over a 360 Scene block
	// payload; 'out' receives the PC payload. Returns false with 'error' set if
	// the stream is not a 360 scene archive or a payload cannot be converted.
	bool TranscodeSceneArchive(const unsigned char* in, size_t size, std::vector<unsigned char>& out, std::string& error);

	// Registers X360SceneTranscoder streamers for the polymorphic scene nodes
	// (defined in SceneTranscodeInstantiate.inl, inside target/archive.cpp).
	void RegisterSceneTranscodeStreamers();
}

namespace Arch
{
	// Mirrors BinaryInputArchive_t (misc_shared/archive/binaryarchive.h) member for
	// member, so the engine's deserialisers compile against it unchanged.
	class X360SceneTranscoder
	{
	public:
		X360SceneTranscoder(const U8* pIn, U32 inSize, U32 startAt, std::vector<U8>& out, X360Convert::SceneState& state);

		// ---- the BinaryInputArchive_t interface

		// Raw bytes (characters, opaque blocks): copied unchanged.
		void* Read(void* pData, U32 nBytes);
		void Align(U32 bytes);

#define __X360_TRANSCODER_ATOM(TYPE) \
		X360SceneTranscoder& operator >> (TYPE& t) {Atom(t); return *this;} \
		X360SceneTranscoder& operator >> (const TYPE& t) {Atom(const_cast<TYPE&>(t)); return *this;}

		__X360_TRANSCODER_ATOM(U8)
		__X360_TRANSCODER_ATOM(S8)
		__X360_TRANSCODER_ATOM(char)
		__X360_TRANSCODER_ATOM(bool)
		__X360_TRANSCODER_ATOM(U16)
		__X360_TRANSCODER_ATOM(S16)
		__X360_TRANSCODER_ATOM(U32)
		__X360_TRANSCODER_ATOM(S32)
		__X360_TRANSCODER_ATOM(F32)
		__X360_TRANSCODER_ATOM(F64)
		__X360_TRANSCODER_ATOM(U64)
		__X360_TRANSCODER_ATOM(S64)

#undef __X360_TRANSCODER_ATOM

		template<typename T>
		X360SceneTranscoder& operator >> (T& obj)
		{
			typedef typename Btl::StripConst<T>::Result T_NC;

			U16 version = 0;

			if (SerialisationInfo<T>::GetSerialisationType() != Arch::SERIALISATION_TYPE_PRIMITIVE)
			{
				U16 test; *this >> test;
				if (test != 'AR')
					Fail("missing 'AR' object tag");
				*this >> version;
			}

			U16 currentVersion = m_version;
			m_version = version;

			Deserialise(*this, const_cast<T_NC&>(obj));

			m_version = currentVersion;
			return *this;
		}

		template<typename T>
		X360SceneTranscoder& operator >> (T*& obj)
		{
			Deserialise(*this, obj);
			return *this;
		}

		X360SceneTranscoder& operator >> (char*& p)
		{
			U32 n;
			*this >> n;
			p = new char[n + 1];
			Read(p, n);
			p[n] = 0;
			return *this;
		}

		X360SceneTranscoder& operator >> (SerialisationType& t)
		{
			U8 temp;
			*this >> temp;
			t = (SerialisationType) temp;
			return *this;
		}

		template<typename T>
		X360SceneTranscoder& operator >> (Nvp<T>& obj)
		{
			*this >> obj.value;
			return *this;
		}

		template<typename T>
		X360SceneTranscoder& operator & (T& obj)
		{
			typedef typename Btl::StripConst<T>::Result T_NC;
			*this >> const_cast<T_NC&>(obj);
			return *this;
		}

		template<typename T>
		X360SceneTranscoder& operator & (T*& obj)
		{
			*this >> obj;
			return *this;
		}

		template<typename T>
		X360SceneTranscoder& operator & (Nvp<T> obj)
		{
			*this >> obj.value;
			return *this;
		}

		U16 GetVersion() const { return m_version; }
		const Sys::I_BinaryStream& Stream() const { return m_in; }
		Sys::I_BinaryStream& Stream() { return m_in; }
		bool GetEndianSwap() const { return false; }
		void SetFailed() { Fail("the reader flagged the archive as failed"); }
		bool GetFailed() { return m_failed; }

		// ---- converter side

		// Records the first failure and drains: every later read yields zero bytes,
		// so the engine's reader runs to its end with empty counts. Nothing is thrown
		// through it (it holds the resource and FX serialise mutexes while reading).
		void Fail(const char* format, ...);
		// A payload was consumed exactly but could not be converted: records the
		// first such error and carries on reading in step with the input.
		void Unconverted(const char* format, ...);
		bool GetUnconverted() const { return m_unconverted; }
		const std::string& Error() const { return m_error; }

		// Input only (big-endian), nothing written.
		U8 InU8();
		U16 InU16();
		U32 InU32();
		const U8* InBytes(U32 n);
		void InAlign(U32 alignment);
		U32 InPos() const { return (U32)(m_in.m_pCurBuffer - m_pInBase); }

		// Output only (little-endian).
		void OutU8(U8 v) { m_out.push_back(v); }
		void OutU16(U16 v) { OutBytes(&v, 2); }
		void OutU32(U32 v) { OutBytes(&v, 4); }
		void OutBytes(const void* p, U32 n) { m_out.insert(m_out.end(), (const U8*)p, (const U8*)p + n); }
		void OutAlign(U32 alignment);
		size_t OutPos() const { return m_out.size(); }
		void PatchU32(size_t at, U32 v) { memcpy(&m_out[at], &v, 4); }
		void PatchU16(size_t at, U16 v) { memcpy(&m_out[at], &v, 2); }
		U16 OutU16At(size_t at) const { U16 v = 0; if (at + 2 <= m_out.size()) memcpy(&v, &m_out[at], 2); return v; }

		// The tag and version operator>> would read for a T, moved to the output.
		template<typename T>
		U16 TagFor()
		{
			U16 version = 0;
			if (SerialisationInfo<T>::GetSerialisationType() != Arch::SERIALISATION_TYPE_PRIMITIVE)
			{
				U16 test; *this >> test;
				if (test != 'AR')
					Fail("missing 'AR' object tag");
				*this >> version;
			}
			return version;
		}

		X360Convert::SceneState& State() { return m_state; }

	private:
		template<typename T>
		void Atom(T& t)
		{
			const U8* p = InBytes(sizeof(T));
			U8 le[sizeof(T)];
			for (U32 i = 0; i < sizeof(T); ++i)
				le[i] = p[sizeof(T) - 1 - i];
			memcpy(&t, le, sizeof(T));
			OutBytes(le, sizeof(T));
		}

		Sys::C_MemoryStream m_in;
		const U8* m_pInBase;
		std::vector<U8>& m_out;
		X360Convert::SceneState& m_state;
		U16 m_version;
		bool m_failed;
		bool m_unconverted;
		std::string m_error;
		std::vector<U8> m_zeros;
	};
}

// Payload overloads: preferred to the engine's templates for this archive type,
// so nothing reaches the device while converting (SceneTranscode.cpp).
namespace RenderingData
{
	void Deserialise(Arch::X360SceneTranscoder& ar, PlatformType& t);
	void Deserialise(Arch::X360SceneTranscoder& ar, VertexElementType& t);
	void Deserialise(Arch::X360SceneTranscoder& ar, RenderListNode::Primitive& t);
	void Deserialise(Arch::X360SceneTranscoder& ar, RenderListNode::Primitive::Lod& t);
	void Deserialise(Arch::X360SceneTranscoder& ar, RenderListNode::PrimitiveStream& t);
	void Deserialise(Arch::X360SceneTranscoder& ar, ChunkInfo& t);

	namespace Target
	{
		void Deserialise(Arch::X360SceneTranscoder& ar, LodReader<ResourceBlock::Resources>& t);
		void Deserialise(Arch::X360SceneTranscoder& ar, IndexBuffer& t);
		void Deserialise(Arch::X360SceneTranscoder& ar, VertexBuffer& t);
		void Deserialise(Arch::X360SceneTranscoder& ar, VertexDeclaration& t);
		void Deserialise(Arch::X360SceneTranscoder& ar, Texture& t);
		void Deserialise(Arch::X360SceneTranscoder& ar, LodReader<Texture>& t);
		void Deserialise(Arch::X360SceneTranscoder& ar, SharedTextureList& t);
	}
}
