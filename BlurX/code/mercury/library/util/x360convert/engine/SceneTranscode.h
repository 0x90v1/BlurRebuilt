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
// [BlurX] Kanzen (Blur 2 prototype, Nov 2010) scene archives carry objects at
// versions past Blur 1's maxima. Each such object is read by the engine's reader
// at Blur 1's maximum version and written with that version; what the newer
// version appends is consumed and dropped (X360Convert::KanzenFields). The
// fields that moved (ChunkInfo sizes, Arch::Deserialiser leading words) and the
// shared-texture list a version 2 Target::Scene no longer holds are handled in
// SceneTranscode.cpp. Blur 1 data never exceeds the maxima, so it converts as
// before. The version deltas: docs/analysis.md, "Kanzen scene archive".
#pragma once

#include <misc_shared/archive/archive.h>
#include <misc_shared/archive/btl.h>
#include <misc_shared/archive/serialiser.h>
#include <system/serialise/C_MemoryStream.h>
#include <renderingdata/target.h>
#include <renderingdata/streaming.h>

#include <map>
#include <memory>
#include <typeinfo>
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

		// [BlurX] Kanzen scene archives: the Target::Scene's version as the file
		// has it (0 until read), and how many objects were read past Blur 1's version.
		U16 kanzenScene;
		unsigned kanzenObjects;
		// A version 2+ Kanzen scene archive holds its shared-texture list after the
		// resource blocks, behind a U8 (Nov 10 envsphere.model: 1, then the list).
		// Pass 1 records where (0: none); pass 2 reads it at Blur 1's place in the
		// Scene. Kept across passes.
		U32 kanzenTexturesAt, kanzenTailEnd;
		bool kanzenTexturesInPlace;		// reading the list at its Blur 1 place now
		unsigned kanzenDanglingRanges;	// draw ranges past their index buffer, drawn empty
		unsigned kanzenPartialTiers;	// texture lods that ship part of their surface, written empty (pass 1 count)
		bool levelScene;				// [BlurX] a level's scene (C_LevelConversion): every dynamic material list kept
		U32 kanzenBlockWords[2];		// [BlurX] the two leading words of the last Kanzen version-1 Deserialiser block: part ordinal, part count
		int chunk;						// [BlurX] the levelstream chunk being read (-1: the scene archive)
		size_t passRowStart, passRowEnd;	// [BlurX] the render-pass row vector being read (RenderPassInfo overload)
		int passRowIndex;
		std::string kanzenPassNames;	// [BlurX] a version-3 Kanzen scene's render-pass names, in order
		S32 kanzenItemWord;				// [BlurX] the word after the lod of the last Kanzen LodReader<Resources>: its split part ordinal
		std::set<std::string> kanzenDropItems;	// [BlurX] "chunk/item" split-part copies (ordinal >= 1), dropped from chunks and the chunk table
		unsigned kanzenDroppedItems;
		// [BlurX] Kanzen lod 0 textures whose mip chain was decoded short of the header's
		// (the packed tail): "block/texture index" -> levels left out. Every later lod of
		// that texture declares that many fewer levels (see the LodReader<Texture> overload).
		std::map<std::string, int> kanzenLodTrim;
		int textureIndex;
		unsigned kanzenTrimmedLods;
		// [BlurX] Kanzen split textures (SceneTranscode.cpp, X360Convert::KanzenSplitTexture):
		// the groups gathered in pass 1 (parts of one group can sit in consecutive
		// levelstream chunks), the group of each part record by chunk and input
		// position (chunks convert again in any order), and pass 1's open groups
		std::map<std::string, std::shared_ptr<struct KanzenSplitTexture> > kanzenSplits;
		std::map<std::string, std::string> kanzenSplitRecord;
		std::map<std::string, unsigned> kanzenSplitNext;
		std::map<std::string, std::vector<std::string> > kanzenSplitOpen;	// per block/lod, oldest first
		unsigned kanzenSplitJoined, kanzenSplitIncomplete;

		SceneState() : pass(0), primitiveIndexBuffer(-1), block(-1), indexBufferCount(0), vertexBufferCount(0),
			primitiveDeclaration(-1), countStats(false), normalsRepacked(0), strips(0), reversedAmbiguous(0), trianglesChecked(0),
			kanzenScene(0), kanzenObjects(0), kanzenTexturesAt(0), kanzenTailEnd(0), kanzenTexturesInPlace(false), kanzenDanglingRanges(0), kanzenPartialTiers(0), levelScene(false), chunk(-1), kanzenItemWord(0), kanzenDroppedItems(0), textureIndex(-1), kanzenTrimmedLods(0), passRowStart(0), passRowEnd((size_t)-1), passRowIndex(-1), kanzenSplitJoined(0), kanzenSplitIncomplete(0) { kanzenBlockWords[0] = 0; kanzenBlockWords[1] = 1; }
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

namespace X360Convert
{
	// [BlurX] Kanzen additions per type; defined after X360SceneTranscoder.
	template<typename T> struct KanzenFields;
	// [BlurX] BLURX_SCENE_TRACE=<file>: one line per object read (depth, type, the
	// file's version, input offset), appended to <file>. SceneTranscode.cpp.
	bool SceneTraceOn();
	void SceneTrace(int depth, const char* what, unsigned raw, unsigned inPos);
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

			// [BlurX] a type Kanzen no longer writes at this place: synthesised, nothing read
			if (X360Convert::KanzenFields<T_NC>::Absent(*this))
			{
				X360Convert::KanzenFields<T_NC>::WriteAbsent(*this);
				return *this;
			}

			U16 version = 0;
			U16 raw = 0;

			if (SerialisationInfo<T>::GetSerialisationType() != Arch::SERIALISATION_TYPE_PRIMITIVE)
			{
				U16 test; *this >> test;
				if (test != 'AR')
					Fail("missing 'AR' object tag");
				raw = ClampedVersion<T_NC>();
				version = raw > X360Convert::KanzenFields<T_NC>::Blur1Max ? (U16)X360Convert::KanzenFields<T_NC>::Blur1Max : raw;
			}
			X360Convert::KanzenFields<T_NC>::Begin(*this, raw);
			if (X360Convert::SceneTraceOn())
				X360Convert::SceneTrace(m_depth, SerialisationInfo<T_NC>::GetGUID() ? SerialisationInfo<T_NC>::GetGUID() : typeid(T_NC).name(), raw, InPos());
			++m_depth;

			U16 currentVersion = m_version;
			U16 currentRaw = m_rawVersion;
			m_version = version;
			m_rawVersion = raw;

			Deserialise(*this, const_cast<T_NC&>(obj));

			// [BlurX] what a Kanzen version appends to the type
			if (raw > version)
				X360Convert::KanzenFields<T_NC>::Skip(*this, raw);

			--m_depth;
			m_version = currentVersion;
			m_rawVersion = currentRaw;
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
		// [BlurX] the version of the object being read as the file has it (above
		// GetVersion() in a Kanzen archive), and of the last object TagFor read
		U16 GetRawVersion() const { return m_rawVersion; }
		U16 LastTagRawVersion() const { return m_tagRawVersion; }
		void SetVersion(U16 version) { m_version = version; }
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
		// [BlurX] Moves the input to an offset inside it (nothing written).
		void InSeek(U32 pos) { if (m_pInBase + pos <= m_in.m_pBufferEnd) m_in.m_pCurBuffer = const_cast<U8*>(m_pInBase) + pos; else Fail("seek to %u past the input", pos); }

		// Output only (little-endian).
		void OutU8(U8 v) { m_out.push_back(v); }
		void OutU16(U16 v) { OutBytes(&v, 2); }
		void OutU32(U32 v) { OutBytes(&v, 4); }
		void OutBytes(const void* p, U32 n) { m_out.insert(m_out.end(), (const U8*)p, (const U8*)p + n); }
		void OutAlign(U32 alignment);
		size_t OutPos() const { return m_out.size(); }
		void PatchU32(size_t at, U32 v) { memcpy(&m_out[at], &v, 4); }
		void PatchU16(size_t at, U16 v) { memcpy(&m_out[at], &v, 2); }
		void OutTruncate(size_t size) { if (size < m_out.size()) m_out.resize(size); }	// [BlurX]
		void OutErase(size_t at, size_t n) { if (at + n <= m_out.size()) m_out.erase(m_out.begin() + at, m_out.begin() + at + n); }	// [BlurX]
		const std::vector<U8>& OutData() const { return m_out; }	// [BlurX]
		void CopyOut(size_t from, std::vector<U8>& to) const { to.assign(m_out.begin() + (from < m_out.size() ? from : m_out.size()), m_out.end()); }	// [BlurX]
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
				version = ClampedVersion<T>();	// [BlurX]
			}
			return version;
		}

		// [BlurX] Input only: an object's 'AR' tag and version, its version returned.
		U16 SkipTag()
		{
			if (InU16() != 'AR')
				Fail("missing 'AR' object tag");
			return InU16();
		}

		X360Convert::SceneState& State() { return m_state; }

	private:
		// [BlurX] Reads an object's version, writes Blur 1's maximum for T in its
		// place when the file's is above it (a Kanzen archive) and returns the file's;
		// m_tagRawVersion keeps it. A version with no Kanzen reading fails the archive.
		template<typename T>
		U16 ClampedVersion()
		{
			const size_t at = OutPos();
			U16 version;
			*this >> version;
			m_tagRawVersion = version;
			// Only types with a Kanzen reading are clamped, to the Blur 1 maximum their
			// KanzenFields names (a type whose SerialisationInfo a translation unit does
			// not see reports version 0 there); every other type keeps the file's
			// version, as in Blur1Multi's transcoder.
			const U16 max = (U16)X360Convert::KanzenFields<T>::Blur1Max;
			if (version > max)
			{
				if (!X360Convert::KanzenFields<T>::Known(version))
					Fail("%s version %u, past Blur 1's %u, has no Kanzen reading", SerialisationInfo<T>::GetGUID() ? SerialisationInfo<T>::GetGUID() : "object", version, max);
				PatchU16(at, max);
				++m_state.kanzenObjects;
			}
			return version;
		}

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
		U16 m_rawVersion;		// [BlurX]
		U16 m_tagRawVersion;	// [BlurX]
		int m_depth;			// [BlurX] object nesting, for SceneTrace
		bool m_failed;
		bool m_unconverted;
		std::string m_error;
		std::vector<U8> m_zeros;
	};
}

namespace X360Convert
{
	// [BlurX] What a Kanzen scene archive adds to a type past Blur 1's version.
	// Known(v): whether version v has a reading here. Skip(ar, v): consume, input
	// only, the fields versions above Blur 1's maximum append. Begin: called with
	// the file's version before the engine reader runs. Absent/WriteAbsent: the
	// object is not in the file at this place and is written from nothing.
	// Blur1Max: Blur 1's version of the type (its SERIALISATION_VERSION); objects
	// above it are clamped. 0xFFFF for types with no Kanzen reading.
	template<typename T>
	struct KanzenFields
	{
		enum { Blur1Max = 0xFFFF };
		static bool Known(U16) { return false; }
		static void Skip(Arch::X360SceneTranscoder&, U16) {}
		static void Begin(Arch::X360SceneTranscoder&, U16) {}
		static bool Absent(Arch::X360SceneTranscoder&) { return false; }
		static void WriteAbsent(Arch::X360SceneTranscoder&) {}
	};

	struct KanzenFieldsBase
	{
		static void Begin(Arch::X360SceneTranscoder&, U16) {}
		static bool Absent(Arch::X360SceneTranscoder&) { return false; }
		static void WriteAbsent(Arch::X360SceneTranscoder&) {}
	};

	// Target::Scene (Kanzen FUN_823ce978): version 2 drops the shared-texture
	// list (KanzenFields<SharedTextureList>); version 3 appends the render-pass
	// names, U32 count + length-prefixed strings.
	template<>
	struct KanzenFields<RenderingData::Target::Scene> : KanzenFieldsBase
	{
		enum { Blur1Max = 1 };
		static bool Known(U16 v) { return v <= 3; }
		static void Begin(Arch::X360SceneTranscoder& ar, U16 v) { ar.State().kanzenScene = v; }
		static void Skip(Arch::X360SceneTranscoder& ar, U16 v)
		{
			if (v >= 3)
			{
				const U32 n = ar.InU32();
				std::string names;
				for (U32 i = 0; i < n && !ar.GetFailed(); ++i)
				{
					const U32 len = ar.InU32();
					const U8* p = ar.InBytes(len);
					if (p)
						names.append(" ").append((const char*)p, len);
				}
				ar.State().kanzenPassNames = names;
			}
		}
	};

	// Not at its Blur 1 place in a version 2+ Kanzen Target::Scene: read from the
	// end of the archive where pass 1 found it (SceneState::kanzenTexturesAt), or
	// written as Blur 1's empty list.
	template<>
	struct KanzenFields<RenderingData::Target::SharedTextureList>
	{
		enum { Blur1Max = 0xFFFF };
		static bool Known(U16) { return false; }
		static void Skip(Arch::X360SceneTranscoder&, U16) {}
		static void Begin(Arch::X360SceneTranscoder&, U16) {}
		static bool Absent(Arch::X360SceneTranscoder& ar) { return ar.State().kanzenScene >= 2 && !ar.State().kanzenTexturesInPlace; }
		static void WriteAbsent(Arch::X360SceneTranscoder& ar)
		{
			X360Convert::SceneState& state = ar.State();
			if (state.kanzenTexturesAt)
			{
				const U32 resume = ar.InPos();
				ar.InSeek(state.kanzenTexturesAt);
				state.kanzenTexturesInPlace = true;
				RenderingData::Target::SharedTextureList list;
				ar >> list;
				state.kanzenTexturesInPlace = false;
				ar.InSeek(resume);
				return;
			}
			ar.OutU16('AR');
			ar.OutU16(Arch::SerialisationInfo<RenderingData::Target::SharedTextureList>::GetVersion());
			ar.OutU32(RenderingData::PLATFORM_TYPE_PC);
			ar.OutU32(0);	// no textures
		}
	};

	// Target::Scene's dynamic material lists. Blur 1 scenes hold one (C_Model::
	// GetDynamicMaterialInput uses the scene's list only when there is exactly one;
	// otherwise the model gets no elements, "needs re-exporting", and vehicle code
	// dereferences a missing world matrix). A version 2+ Kanzen scene holds more:
	// the first as Blur 1's (Element, AnimatedParam types), then empty ones (Nov 10
	// audi_r8v10_stock.model: a list with no types). The empty lists are dropped;
	// a non-empty extra list has no Blur 1 reading and fails the file.
	template<>
	struct KanzenFields<Btl::Vector<Btl::AutoPointer<RenderingData::Target::MaterialList> > > : KanzenFieldsBase
	{
		enum { Blur1Max = 0xFFFF };
		static bool Known(U16) { return false; }
		static void Skip(Arch::X360SceneTranscoder&, U16) {}
		static bool Absent(Arch::X360SceneTranscoder& ar) { return ar.State().kanzenScene >= 2; }
		// defined in SceneTranscodeInstantiate.inl, beside the engine's MaterialList reader
		static void WriteAbsent(Arch::X360SceneTranscoder& ar);
	};

	// CullNode (FUN_823be9c8): version 3 + U8 (meaning not established).
	template<>
	struct KanzenFields<RenderingData::CullNode> : KanzenFieldsBase
	{
		enum { Blur1Max = 2 };
		static bool Known(U16 v) { return v <= 3; }
		static void Skip(Arch::X360SceneTranscoder& ar, U16) { ar.InU8(); }
	};

	// RenderListNode::Primitive (FUN_823c3e18): version 2 + 2 x U16.
	template<>
	struct KanzenFields<RenderingData::RenderListNode::Primitive> : KanzenFieldsBase
	{
		enum { Blur1Max = 1 };
		static bool Known(U16 v) { return v <= 2; }
		static void Skip(Arch::X360SceneTranscoder& ar, U16) { ar.InU16(); ar.InU16(); }
	};

	// RenderListNode::Instance (FUN_823c3020): version 1 + U32, version 2 + 2 x U16.
	template<>
	struct KanzenFields<RenderingData::RenderListNode::Instance> : KanzenFieldsBase
	{
		enum { Blur1Max = 0 };
		static bool Known(U16 v) { return v <= 2; }
		static void Skip(Arch::X360SceneTranscoder& ar, U16 v)
		{
			ar.InU32();
			if (v >= 2) { ar.InU16(); ar.InU16(); }
		}
	};

	// StreamData (FUN_823cc288): version 4 + object {string, U64}; version 5 +
	// vector of objects {U32 count, U32s}; version 6 + object {U32 count, U32s}.
	// Nov 10 envsphere.model (version 6): the objects' U32 lists have no tag of
	// their own, only the object's.
	template<>
	struct KanzenFields<RenderingData::StreamData> : KanzenFieldsBase
	{
		enum { Blur1Max = 3 };
		static bool Known(U16 v) { return v <= 6; }
		static void SkipU32List(Arch::X360SceneTranscoder& ar)
		{
			ar.SkipTag();
			const U32 n = ar.InU32();
			ar.InBytes(n * 4);
		}
		static void Skip(Arch::X360SceneTranscoder& ar, U16 v)
		{
			ar.SkipTag();
			ar.InBytes(ar.InU32());
			ar.InBytes(8);
			if (v >= 5)
			{
				ar.SkipTag();
				const U32 n = ar.InU32();
				for (U32 i = 0; i < n && !ar.GetFailed(); ++i)
					SkipU32List(ar);
			}
			if (v >= 6)
				SkipU32List(ar);
		}
	};

	// LodReader<ResourceBlock::Resources> (FUN_823cc660): version 1 adds a U32
	// after the lod, read in SceneTranscode.cpp's overload; nothing appended.
	template<>
	struct KanzenFields<RenderingData::Target::LodReader<RenderingData::Target::ResourceBlock::Resources> > : KanzenFieldsBase
	{
		enum { Blur1Max = 0 };
		static bool Known(U16 v) { return v <= 1; }
		static void Skip(Arch::X360SceneTranscoder&, U16) {}
	};

	// ChunkInfo (FUN_823c6ed0): version 2 moves its sizes (U32s); read in
	// SceneTranscode.cpp's ChunkInfo overload, nothing appended.
	template<>
	struct KanzenFields<RenderingData::ChunkInfo> : KanzenFieldsBase
	{
		enum { Blur1Max = 1 };
		static bool Known(U16 v) { return v <= 2; }
		static void Skip(Arch::X360SceneTranscoder&, U16) {}
	};

	// ChunkInfo::Item (FUN_823c6ed0 walks the items 12 bytes apart; Blur 1's are
	// 8: S32 block, S32 lod): version 1 + S32 (0 in every item of the Nov 10
	// levels read so far; Blur 1's serialiser has Item::size commented out at
	// this place; meaning not established, dropped). Level scenes only: model
	// scenes have no chunk items.
	template<>
	struct KanzenFields<RenderingData::ChunkInfo::Item> : KanzenFieldsBase
	{
		enum { Blur1Max = 0 };
		static bool Known(U16 v) { return v <= 1; }
		static void Skip(Arch::X360SceneTranscoder& ar, U16) { ar.InU32(); }
	};

	// Arch::Deserialiser (FUN_823b1c90): version 1 leads with two U32s, consumed
	// by ReadSerialiserBlock (SceneTranscode.cpp).
	template<>
	struct KanzenFields<Arch::Deserialiser> : KanzenFieldsBase
	{
		enum { Blur1Max = 0 };
		static bool Known(U16 v) { return v <= 1; }
		static void Skip(Arch::X360SceneTranscoder&, U16) {}
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
	void Deserialise(Arch::X360SceneTranscoder& ar, RenderListNode::RenderPassInfo& t);	// [BlurX] pass rows
	// [BlurX] Kanzen material element types (past Blur 1's MATERIAL_ELEMENT_TYPE_MAX)
	// are dropped from material types and their data skipped (SceneTranscode.cpp).
	void Deserialise(Arch::X360SceneTranscoder& ar, MaterialElementType& t);
	void Deserialise(Arch::X360SceneTranscoder& ar, MaterialType& t);
	void DeserialiseMaterial(Arch::X360SceneTranscoder& ar, Material& material, const MaterialType& materialType);

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
