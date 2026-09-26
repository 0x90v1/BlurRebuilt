// [BlurX] Kanzen -> Blur 1 normalisation (Xbox 360 byte order). See KanzenNormalise.h.

#include <assetcatalog/KanzenNormalise.h>

#include <string.h>
#include <ctype.h>
#include <stdio.h>

namespace KanzenNormalise
{
	namespace
	{
		bool EndsWith(const char* path, const char* ext)
		{
			const size_t n = strlen(path), m = strlen(ext);
			if (n < m)
				return false;
			for (size_t i = 0; i < m; ++i)
				if (tolower((unsigned char)path[n - m + i]) != ext[i])
					return false;
			return true;
		}

		// Big-endian reader that copies what it reads to 'out' unless told otherwise.
		class C_Stream
		{
		public:
			C_Stream(const unsigned char* p, size_t n, std::vector<unsigned char>* pOut)
				: m_p(p), m_n(n), m_at(0), m_pOut(pOut), m_ok(true) {}

			bool Ok() const { return m_ok; }
			size_t At() const { return m_at; }
			size_t Size() const { return m_n; }
			const unsigned char* Data() const { return m_p; }
			void Seek(size_t at) { m_at = at; }
			const std::string& Error() const { return m_error; }

			bool Fail(const char* what)
			{
				if (m_ok)
				{
					char buf[160];
					sprintf_s(buf, "%s (at 0x%x of 0x%x)", what, (unsigned)m_at, (unsigned)m_n);
					m_error = buf;
				}
				m_ok = false;
				return false;
			}

			const unsigned char* Take(size_t n, bool copy = true)
			{
				if (!m_ok || m_at + n > m_n)
				{
					Fail("read past the end");
					return NULL;
				}
				const unsigned char* p = m_p + m_at;
				m_at += n;
				if (copy && m_pOut)
					m_pOut->insert(m_pOut->end(), p, p + n);
				return p;
			}

			unsigned U32(bool copy = true)
			{
				const unsigned char* p = Take(4, copy);
				return p ? (unsigned)p[0] << 24 | (unsigned)p[1] << 16 | (unsigned)p[2] << 8 | p[3] : 0;
			}

			unsigned U16(bool copy = true)
			{
				const unsigned char* p = Take(2, copy);
				return p ? (unsigned)p[0] << 8 | p[1] : 0;
			}

			unsigned U8(bool copy = true)
			{
				const unsigned char* p = Take(1, copy);
				return p ? *p : 0;
			}

			void String()
			{
				const unsigned n = U32();
				Take(n);
			}

		private:
			const unsigned char* m_p;
			size_t m_n, m_at;
			std::vector<unsigned char>* m_pOut;
			bool m_ok;
			std::string m_error;
		};

		void PutU32(std::vector<unsigned char>& out, unsigned v)
		{
			out.push_back((unsigned char)(v >> 24)); out.push_back((unsigned char)(v >> 16));
			out.push_back((unsigned char)(v >> 8)); out.push_back((unsigned char)v);
		}

		void PutU16(std::vector<unsigned char>& out, unsigned v)
		{
			out.push_back((unsigned char)(v >> 8)); out.push_back((unsigned char)v);
		}

		unsigned GetU32(const unsigned char* p) { return (unsigned)p[0] << 24 | (unsigned)p[1] << 16 | (unsigned)p[2] << 8 | p[3]; }

		// One packed block of a cycle record (animation C_Cycle::ReadVaryingCurveKeys; Blur2Test
		// parse_xbox_cycle_prefix.packed_block): f32 x3, u32, f32, u16 x3, u16 x3, u8 x3 per count.
		void PackedBlock(C_Stream& s, const unsigned counts[6])
		{
			s.Take(12 * (size_t)counts[0]);
			s.Take(4 * (size_t)counts[1]);
			s.Take(4 * (size_t)counts[2]);
			s.Take(6 * (size_t)counts[3]);
			s.Take(6 * (size_t)counts[4]);
			s.Take(3 * (size_t)counts[5]);
		}

		// One segment's bytes as C_Cycle::Deserialise reads them (version 14/15).
		void SegmentRecord(C_Stream& s, const unsigned global[6], bool last)
		{
			PackedBlock(s, global);
			if (last)
				return;
			unsigned local[6];
			unsigned keyTimes = 0;
			for (int j = 0; j < 6; ++j)
			{
				local[j] = s.U16();
				keyTimes += local[j] + global[j];
			}
			s.Take(keyTimes);
			PackedBlock(s, local);
		}
	}

	bool Handles(const char* path)
	{
		return EndsWith(path, ".cycle") || EndsWith(path, ".skeleton") || EndsWith(path, ".model") || EndsWith(path, ".levelrender");
	}

	E_Result Normalise(const char* path, const unsigned char* in, size_t size, std::vector<unsigned char>& out, std::string& error)
	{
		if (EndsWith(path, ".cycle"))
			return NormaliseCycle(in, size, out, error);
		if (EndsWith(path, ".skeleton"))
			return NormaliseSkeleton(in, size, out, error);
		if (EndsWith(path, ".model"))
			return NormaliseModel(in, size, out, error);
		if (EndsWith(path, ".levelrender"))
			return NormaliseLevelRender(in, size, out, error);
		return E_Result_NotHandled;
	}

	// Kanzen cycles (Blur2Test _tools/parse_xbox_cycle_prefix.py, _notes/xbox_stage_decode.md):
	// root class 0x10/0x11 is 0x0E/0x0F (C_Cycle versions 14/15: 0x11 and 0x0F carry the
	// joint transforms with scale) with three changes, all undone here:
	//  1. the channel table (C_Cycle::m_channelInfo, U16 = field index << 6 | pose field
	//     element) is a byte-size-prefixed list of [ULEB128 field index, U8 element];
	//  2. each segment header after the first is preceded by a U32, and the header table is
	//     followed by two zero U32s;
	//  3. the segments' bytes are wrapped: "ASCK", U32 total body bytes, U32 segment-data size
	//     (= the header's), U32 0, U32 segment count, some bytes, then one "SEGT" chunk per
	//     segment but the last and one "ENDS" (tag, U32 body size, body = the version 14/15
	//     segment bytes). The chunk chain is found as Blur2Test finds it: the one position
	//     from which exactly that chain ends at the end of the file.
	// Dropped (no place in Blur 1's cycle; meaning not established): the per-segment U32s
	// of (2) and the bytes between the ASCK header and the first chunk.
	E_Result NormaliseCycle(const unsigned char* in, size_t size, std::vector<unsigned char>& out, std::string& error)
	{
		if (size < 4)
			return E_Result_NotHandled;
		const unsigned root = GetU32(in);
		if (root != 0x10 && root != 0x11)
			return E_Result_NotHandled;

		out.clear();
		out.reserve(size);
		C_Stream s(in, size, &out);
		s.U32(false);
		PutU32(out, root == 0x11 ? 0x0F : 0x0E);
		const bool scale = root == 0x11;
		const size_t transformBytes = scale ? 40 : 28;

		const unsigned refs = s.U32();
		s.Take(transformBytes * refs);			// reference transforms
		s.Take(transformBytes * 2);				// trajectory first, final
		s.U32();								// trajectory root field index
		s.U32();								// frames
		s.Take(8);								// playback duration, frame rate
		const unsigned fields = s.U32();
		s.Take(fields);							// field types
		const unsigned names = s.U32();
		if (s.Ok() && names != fields)
		{
			error = "field name count differs from field count";
			return E_Result_Failed;
		}
		for (unsigned i = 0; s.Ok() && i < names; ++i)
			s.String();

		unsigned counts[11];
		for (int i = 0; i < 11; ++i)
			counts[i] = s.U16();
		const unsigned curves = counts[0] + counts[1];

		// (1) the channel table
		const unsigned tableBytes = s.U32(false);
		const size_t tableEnd = s.At() + tableBytes;
		for (unsigned i = 0; s.Ok() && i < curves; ++i)
		{
			unsigned field = 0, shift = 0, byte;
			do
			{
				byte = s.U8(false);
				field |= (byte & 0x7f) << shift;
				shift += 7;
			} while (s.Ok() && (byte & 0x80) && shift < 32);
			const unsigned element = s.U8(false);
			if (field >= 1024 || element >= 64)
			{
				s.Fail("channel does not fit Blur 1's U16 channel info");
				break;
			}
			PutU16(out, field << 6 | element);
		}
		if (s.Ok() && s.At() != tableEnd)
			s.Fail("channel table size differs from its entries");

		// constant curve data (unchanged)
		s.U32();								// constant curve data size
		s.Take(4 * 6 * (size_t)(counts[9] + counts[10]));
		s.Take(4 * 3 * (size_t)counts[2]);
		s.Take(4 * (size_t)counts[3]);
		s.Take(2 * 3 * (size_t)counts[4]);

		// (2) segment headers
		const unsigned segments = s.U32();
		const unsigned segmentBytes = s.U32();
		for (unsigned i = 0; s.Ok() && i < segments; ++i)
		{
			if (i > 0)
				s.U32(false);
			s.U32();							// start frame
			s.U16();							// frames
			s.U16();							// size
		}
		if (s.U32(false) != 0 || s.U32(false) != 0)
			s.Fail("segment header table trailer is not two zero words");
		if (!s.Ok())
		{
			error = s.Error();
			return E_Result_Failed;
		}

		// (3) unwrap the segments
		const size_t asck = s.At();
		if (segments == 0)
		{
			if (asck != size)
			{
				error = "bytes after a cycle with no segments";
				return E_Result_Failed;
			}
			return E_Result_Normalised;
		}
		if (size < asck + 20 || memcmp(in + asck, "ASCK", 4) != 0)
		{
			error = "no ASCK after the segment headers";
			return E_Result_Failed;
		}
		const unsigned total = GetU32(in + asck + 4);
		if (GetU32(in + asck + 8) != segmentBytes || GetU32(in + asck + 12) != 0 || GetU32(in + asck + 16) != segments)
		{
			error = "ASCK header disagrees with the segment headers";
			return E_Result_Failed;
		}
		std::vector<size_t> bodies, found;
		int solutions = 0;
		const char* first = segments == 1 ? "ENDS" : "SEGT";
		for (size_t start = asck + 20; start + 8 <= size; ++start)
		{
			if (memcmp(in + start, first, 4) != 0)
				continue;
			size_t at = start;
			bool valid = true;
			bodies.clear();
			for (unsigned i = 0; i < segments; ++i)
			{
				const char* tag = i + 1 == segments ? "ENDS" : "SEGT";
				if (at + 8 > size || memcmp(in + at, tag, 4) != 0)
				{
					valid = false;
					break;
				}
				const size_t bodySize = GetU32(in + at + 4);
				if (at + 8 + bodySize > size)
				{
					valid = false;
					break;
				}
				bodies.push_back(at + 8);
				bodies.push_back(bodySize);
				at += 8 + bodySize;
			}
			if (valid && at == size)
			{
				++solutions;
				found = bodies;
			}
		}
		if (solutions != 1)
		{
			char buf[96];
			sprintf_s(buf, "ASCK chunk chain has %d exact solutions", solutions);
			error = buf;
			return E_Result_Failed;
		}
		size_t sum = 0;
		for (size_t i = 0; i < found.size(); i += 2)
			sum += found[i + 1];
		if (sum != total)
		{
			error = "chunk bodies do not add up to the ASCK total";
			return E_Result_Failed;
		}

		// each body must walk to its end as a version 14/15 segment
		const unsigned global[6] = { counts[5], counts[6], counts[7], counts[8], counts[9], counts[10] };
		for (unsigned i = 0; i < segments; ++i)
		{
			const size_t body = found[2 * i], bodySize = found[2 * i + 1];
			C_Stream seg(in + body, bodySize, &out);
			SegmentRecord(seg, global, i + 1 == segments);
			if (!seg.Ok() || seg.At() != bodySize)
			{
				char buf[128];
				sprintf_s(buf, "segment %u does not walk as a Blur 1 segment (%s)", i, seg.Ok() ? "bytes left" : seg.Error().c_str());
				error = buf;
				return E_Result_Failed;
			}
		}
		return E_Result_Normalised;
	}

	// Kanzen skeletons (Blur2Test _tools/decode_xbox_skeleton.py; Blur 1's reader is
	// animation/skeleton/SkeletonDeserialise.inl). Root class 8 differs from version 7 in
	// its header only: U32 8, U16 joints, U16 a, U32 b, U32 c where version 7 has U32 7,
	// U32 joints; the names list holds joints + b names. Joints, parents, local base poses
	// (10 floats), inverse world matrices (12 floats) are unchanged. The constraint block
	// (C_SkeletonConstraints: U16 version, U16 constraints, U16 driven keys, U8[8] counts)
	// carries version 5 where Blur 1 writes 4; with no constraints the two are the same
	// bytes, so only empty blocks are accepted (the cut-scene skeletons with constraints
	// are not decoded, by Blur2Test either). Dropped: the b extra names (not joints; their
	// use is not established). Refused: a or c non-zero (layout not established).
	E_Result NormaliseSkeleton(const unsigned char* in, size_t size, std::vector<unsigned char>& out, std::string& error)
	{
		if (size < 16 || GetU32(in) != 8)
			return E_Result_NotHandled;
		out.clear();
		out.reserve(size);
		C_Stream s(in, size, &out);
		s.U32(false);
		const unsigned joints = s.U16(false);
		const unsigned a = s.U16(false);
		const unsigned b = s.U32(false);
		const unsigned c = s.U32(false);
		if (a != 0 || c != 0)
		{
			error = "class 8 header counts a/c are not zero (layout not established)";
			return E_Result_Failed;
		}
		PutU32(out, 7);
		PutU32(out, joints);
		s.Take(2 * (size_t)joints);				// parent indices
		s.Take(40 * (size_t)joints);			// local base poses with scale
		s.Take(48 * (size_t)joints);			// inverse world base pose matrices
		s.U16();								// constraint block version (5)
		const unsigned constraints = s.U16();
		const unsigned drivenKeys = s.U16();
		unsigned typed = 0;
		for (int i = 0; i < 8; ++i)
			typed += s.U8();
		if (s.Ok() && (constraints || drivenKeys || typed))
		{
			error = "skeleton has constraints (version 5 constraint layout not established)";
			return E_Result_Failed;
		}
		const unsigned names = s.U32(false);
		if (s.Ok() && names != joints + b)
		{
			error = "name count is not joints + extra names";
			return E_Result_Failed;
		}
		PutU32(out, joints);
		for (unsigned i = 0; s.Ok() && i < names; ++i)
		{
			const bool keep = i < joints;
			const unsigned n = s.U32(keep);
			s.Take(n, keep);
		}
		if (!s.Ok() || s.At() != size)
		{
			error = s.Ok() ? "bytes after the joint names" : s.Error();
			return E_Result_Failed;
		}
		return E_Result_Normalised;
	}

	namespace
	{
		struct C_Block
		{
			const unsigned char* at;	// the 16-byte header
			size_t size;				// with the header
			unsigned count;
			bool Is(const char* id) const
			{
				char name[9] = { 0 };
				memcpy(name, at, 8);
				return strcmp(name, id) == 0;
			}
			const unsigned char* Payload() const { return at + 16; }
			size_t PayloadSize() const { return size - 16; }
		};

		// The blocks that fill [p, p + n) exactly; false when they do not.
		bool Blocks(const unsigned char* p, size_t n, std::vector<C_Block>& out)
		{
			out.clear();
			size_t at = 0;
			while (at < n)
			{
				if (n - at < 16)
					return false;
				C_Block b;
				b.at = p + at;
				b.size = GetU32(p + at + 8);
				b.count = (unsigned)p[at + 12] << 8 | p[at + 13];
				if (b.size < 16 || b.size > n - at)
					return false;
				out.push_back(b);
				at += b.size;
			}
			return true;
		}

		// A block header (Blur 1's C_BlockHeader, big-endian here): id, size, count and
		// the 2 bytes that follow, as the source block has them; the size is patched by EndBlock.
		size_t BeginBlock(std::vector<unsigned char>& out, const C_Block& like, const char* id, unsigned count)
		{
			const size_t at = out.size();
			char name[8] = { 0 };
			memcpy(name, id, strlen(id) < 8 ? strlen(id) : 8);
			out.insert(out.end(), (const unsigned char*)name, (const unsigned char*)name + 8);
			PutU32(out, 0);
			PutU16(out, count);
			out.insert(out.end(), like.at + 14, like.at + 16);
			return at;
		}

		void EndBlock(std::vector<unsigned char>& out, size_t at)
		{
			const unsigned size = (unsigned)(out.size() - at);
			out[at + 8] = (unsigned char)(size >> 24);
			out[at + 9] = (unsigned char)(size >> 16);
			out[at + 10] = (unsigned char)(size >> 8);
			out[at + 11] = (unsigned char)size;
		}

		void CopyBlock(std::vector<unsigned char>& out, const C_Block& b)
		{
			out.insert(out.end(), b.at, b.at + b.size);
		}

		// Half::F16::F16(F32) (misc_shared/source/misc_shared/Types/Float16.cpp): the
		// engine's conversion, truncating, which is how Blur 1's exporter wrote halves.
		unsigned FloatBitsToHalf(unsigned f)
		{
			const unsigned sign = f >> 31, exponent = (f >> 23) & 0xff, mantissa = f & 0x7fffff;
			unsigned h = sign << 15;
			if (exponent == 0xff)
			{
				h |= 31u << 10;
				if (mantissa)
					h |= 1;
			}
			else if (exponent != 0)
			{
				const int e = (int)exponent - 127;
				if (e < -14)
				{
					const unsigned shift = (unsigned)(-14 - e);
					if (shift < 10)
						h |= ((0x800000 | mantissa) >> (13 + shift)) & 0x3ff;
					else if (shift == 10)
						h |= 1;
				}
				else if (e > 15)
					h |= 31u << 10;
				else
					h |= (unsigned)(e + 15) << 10 | (mantissa >> 13);
			}
			return h;
		}

		// Kanzen LS (light-sample volume) version 3 -> Blur 1 version 1
		// (WorldRender::C_LightSamplePoints::Read, LightSamplePoints.cpp). Layouts from
		// Blur2Test (_notes/xbox_stage_decode.md, "The light-sample volume read from
		// Kanzen_Speed.xex" and "Record mode 3 ..."; _tools/export_xbox_light_samples.py):
		//   Header  Blur 1: U32 version (1), S32 light model (2 = RGBSH, 12 halves; -1 none)
		//           Kanzen: U32 version (3), U32 mode (2: 12 halves; 3: then U32 halves per record)
		//   LSPoint Blur 1: S32 n, n x (3 halves position, read and discarded; U32 LSData index)
		//           Kanzen v3 (the reader: "if (version < 2)" reads the halves): n x U32 index
		//   LSData  n records; mode 2 = Blur 1's RGBSH record (12 halves), copied
		//   LSTree  Blur 1: S32 planes, planes x C_Plane (16 bytes), S32 positions,
		//           positions x (U32 LSPoint index, 3 halves), S32 nodes, node stream
		//           Kanzen: the same with each position as 3 F32
		// Mode 3 records ([L1 xyz, L0] monochrome, then colour.rgb and a lighting scale,
		// consumed as g_SHNode[2] by Kanzen's pixel programs) have no Blur 1 light model:
		// the volume is written as Blur 1's "unknown lightsampledata format" (model -1),
		// which the reader takes as no light samples. STOPGAP: those levels light props
		// without samples until Kanzen's light-sample consumer is ported.
		bool NormaliseLightSamples(const C_Block& ls, std::vector<unsigned char>& out, bool& changed, std::string& error)
		{
			std::vector<C_Block> kids;
			if (!Blocks(ls.Payload(), ls.PayloadSize(), kids) || kids.empty() || !kids[0].Is("Header") || kids[0].PayloadSize() < 4)
			{
				error = "LS: children do not fill it, or no Header first";
				return false;
			}
			const C_Block& header = kids[0];
			const unsigned version = GetU32(header.Payload());
			if (version < 3)
			{
				CopyBlock(out, ls);		// Blur 1's layout (Aug 31, retail)
				return true;
			}
			if (version != 3)
			{
				char msg[64];
				sprintf_s(msg, "LS version %u (Kanzen 3 known)", version);
				error = msg;
				return false;
			}
			changed = true;
			const unsigned mode = header.PayloadSize() >= 8 ? GetU32(header.Payload() + 4) : 0xffffffffu;
			const size_t lsAt = BeginBlock(out, ls, "LS", mode == 2 && header.PayloadSize() == 8 ? 4 : 1);
			const size_t hAt = BeginBlock(out, header, "Header", 0);
			PutU32(out, 1);		// E_LightSamplePointsFileVersion
			if (mode != 2 || header.PayloadSize() != 8)
			{
				PutU32(out, 0xffffffffu);	// E_LightSampleDataType_Unknown (see above)
				EndBlock(out, hAt);
				EndBlock(out, lsAt);
				return true;
			}
			PutU32(out, 2);		// E_LightSampleDataType_RGBSH
			EndBlock(out, hAt);

			const C_Block* point = NULL;
			const C_Block* data = NULL;
			const C_Block* tree = NULL;
			for (size_t i = 1; i < kids.size(); ++i)
			{
				if (kids[i].Is("LSPoint")) point = &kids[i];
				else if (kids[i].Is("LSData")) data = &kids[i];
				else if (kids[i].Is("LSTree")) tree = &kids[i];
				else
				{
					error = "LS: unknown child";
					return false;
				}
			}
			if (!point || !data || !tree || point->PayloadSize() < 4 || data->PayloadSize() < 4 || tree->PayloadSize() < 4)
			{
				error = "LS: LSPoint, LSData or LSTree missing";
				return false;
			}
			const unsigned points = GetU32(point->Payload());
			if (point->PayloadSize() != 4 + (size_t)points * 4)
			{
				error = "LS: LSPoint is not a U32 index per point";
				return false;
			}
			const unsigned records = GetU32(data->Payload());
			if (data->PayloadSize() != 4 + (size_t)records * 24)
			{
				error = "LS: LSData is not 12 halves per record";
				return false;
			}

			// LSTree: planes, positions (index + 3 F32), nodes
			const unsigned char* t = tree->Payload();
			const size_t tn = tree->PayloadSize();
			const unsigned planes = GetU32(t);
			size_t at = 4 + (size_t)planes * 16;
			if (at + 4 > tn)
			{
				error = "LS: LSTree planes past its end";
				return false;
			}
			const unsigned positions = GetU32(t + at);
			const size_t rowsAt = at + 4;
			at = rowsAt + (size_t)positions * 16;
			if (positions != points || at + 4 > tn)
			{
				error = "LS: LSTree position table does not match LSPoint";
				return false;
			}
			// the node stream (C_BSPPointTree::ReadNode): -1 leaf + 2 words, 1 branch + 1 word + 2 nodes, 0 null
			const size_t nodesAt = at;
			at += 4;
			unsigned pending = 1;
			while (pending)
			{
				if (at + 4 > tn)
				{
					error = "LS: LSTree node stream past its end";
					return false;
				}
				const unsigned tag = GetU32(t + at);
				at += 4;
				--pending;
				if (tag == 0xffffffffu)
					at += 8;
				else if (tag == 1)
				{
					at += 4;
					pending += 2;
				}
				else if (tag != 0)
				{
					error = "LS: LSTree node tag not -1, 0 or 1";
					return false;
				}
			}
			if (at != tn)
			{
				error = "LS: bytes after the LSTree node stream";
				return false;
			}

			// each point's position from the tree row that names it (Blur 1's reader
			// discards LSPoint's positions; they are written as Blur 1's exporter did)
			std::vector<unsigned> halves((size_t)points * 3, 0);
			std::vector<unsigned char> seen(points, 0);
			for (unsigned r = 0; r < positions; ++r)
			{
				const unsigned char* row = t + rowsAt + (size_t)r * 16;
				const unsigned index = GetU32(row);
				if (index >= points || seen[index])
				{
					error = "LS: LSTree rows do not name each point once";
					return false;
				}
				seen[index] = 1;
				for (int c = 0; c < 3; ++c)
					halves[(size_t)index * 3 + c] = FloatBitsToHalf(GetU32(row + 4 + 4 * c));
			}

			const size_t pAt = BeginBlock(out, *point, "LSPoint", 0);
			PutU32(out, points);
			for (unsigned i = 0; i < points; ++i)
			{
				for (int c = 0; c < 3; ++c)
					PutU16(out, halves[(size_t)i * 3 + c]);
				out.insert(out.end(), point->Payload() + 4 + (size_t)i * 4, point->Payload() + 8 + (size_t)i * 4);
			}
			EndBlock(out, pAt);
			CopyBlock(out, *data);
			const size_t tAt = BeginBlock(out, *tree, "LSTree", 0);
			out.insert(out.end(), t, t + rowsAt);
			for (unsigned r = 0; r < positions; ++r)
			{
				const unsigned char* row = t + rowsAt + (size_t)r * 16;
				out.insert(out.end(), row, row + 4);
				for (int c = 0; c < 3; ++c)
					PutU16(out, FloatBitsToHalf(GetU32(row + 4 + 4 * c)));
			}
			out.insert(out.end(), t + nodesAt, t + tn);
			EndBlock(out, tAt);
			EndBlock(out, lsAt);
			return true;
		}

		// Kanzen CubePt3 -> Blur 1 CubePt2 (WorldRender::C_LocalisedCubemapData::Deserialise:
		// Locations {HVec3, string}, the zones' KdTree {nodes (2 U32), mailboxes (U32),
		// zones (HMat43, S32)}, the localised cubemap path, the default cubemap name).
		// Kanzen's reader (FUN_8247e050, location reader FUN_8247c8d8) reads the same four
		// containers and two strings with one more U32 after each location's name (stored
		// at location +0x60) and one U32 after the strings (stored at +0x98); both are
		// dropped (meaning not established). Checked on every CubePt3 of the prototype
		// discs (137, 5 with locations): each walks to its end exactly.
		bool NormaliseCubemaps(const C_Block& b, std::vector<unsigned char>& out, std::string& error)
		{
			const unsigned char* p = b.Payload();
			const size_t n = b.PayloadSize();
			size_t at = 0;
			bool ok = true;
			std::vector<unsigned char> body;
			auto take = [&](size_t k, bool keep) -> const unsigned char*
			{
				if (!ok || at + k > n) { ok = false; return NULL; }
				const unsigned char* q = p + at;
				if (keep)
					body.insert(body.end(), q, q + k);
				at += k;
				return q;
			};
			auto u32 = [&](bool keep) -> unsigned { const unsigned char* q = take(4, keep); return q ? GetU32(q) : 0; };
			auto str = [&]() { const unsigned len = u32(true); take(len, true); };
			const unsigned locations = u32(true);
			for (unsigned i = 0; ok && i < locations; ++i)
			{
				take(12, true);		// HVec3
				str();
				u32(false);			// Kanzen: dropped
			}
			const unsigned nodes = u32(true);
			take((size_t)nodes * 8, true);
			const unsigned mailboxes = u32(true);
			take((size_t)mailboxes * 4, true);
			const unsigned zones = u32(true);
			take((size_t)zones * 52, true);
			str();
			str();
			u32(false);				// Kanzen: dropped
			if (!ok || at != n)
			{
				error = "CubePt3 does not walk to its end";
				return false;
			}
			const size_t cAt = BeginBlock(out, b, "CubePt2", b.count);
			out.insert(out.end(), body.begin(), body.end());
			EndBlock(out, cAt);
			return true;
		}
	}

	namespace
	{
		// Kanzen crowd data (Render/Crowd) versions 7-9 -> Blur 1 version 6
		// (GameResource::C_CrowdData::Read, E_CrowdFileVersion_SHNodeData). Kanzen's reader
		// is recovered from the Nov 25 Kanzen_Release.xex (flat image; C_CrowdData::Read
		// FUN_82889558, element readers: character ref FUN_8288a800, instance FUN_8288bb78,
		// FUN_8288ab80, block ref FUN_8288bf40, block grid FUN_8288c6d0 with cells
		// FUN_8288bda8; decompiles in BlurX/_work/xex/nov25r_crowd*.c). Kanzen, by version v:
		//   S32 v; U32 seed; v>6: 3 x F32, a direction: (0,0,-1) in every level of the Nov
		//   4, 9 and 10 discs, which is the m_ZDirection Blur 1's constructor sets (it is
		//   not in Blur 1's file); (0,0,1) only in levels\legacyarcelona_gracia, not a level
		//   S32 n; n x U32 behaviour hashes                       (Blur 1: same)
		//   S32 n; n x {U32, U16 behaviour, U16, U16}             (Blur 1: U16 behaviour)
		//   S32 n; n x {3 F32, 3 F32, F32, U32, U32, U32 [, v==8: U32]}  (40 bytes; Blur 1: without the v8 word)
		//   v>5: S32 n; n x {U32, U32}                            (Blur 1: none)
		//   S32 n; n x block ref {U32, U32, U8 k, k x U32, 6 F32, U32}   (Blur 1: same)
		//   S32 n; n x {U16, U16} route counts                    (Blur 1: same)
		//   v>3: U32; 6<v<9: S32 n x {U32, string}; v==8: S32 n x {U32 k, k x U32, string}
		//   block grid {U16, 2 F32, 2 F32, S32 n, n x {S32 k, k x U16}}  (Blur 1: same)
		//   (Blur 1 version 6 then reads S32 n SH node records; Kanzen has none: 0)
		// Dropped, meaning not established: the three floats, the character ref's first
		// U32 and last two U16s, the instance's version-8 word, the U32-pair list, the
		// version-4+ word and the two string lists. Every Crowd block of the Nov 4, 9 and
		// 10 discs walks to its end exactly under this reading.
		bool NormaliseCrowd(const C_Block& b, std::vector<unsigned char>& out, std::string& error)
		{
			C_Stream s(b.Payload(), b.PayloadSize(), NULL);
			std::vector<unsigned char> body;
			const unsigned version = s.U32(false);
			if (version > 9)
			{
				char msg[48];
				sprintf_s(msg, "Crowd version %u (Kanzen 7-9 known)", version);
				error = msg;
				return false;
			}
			PutU32(body, 6);
			PutU32(body, s.U32(false));								// seed
			if (version > 6)
				s.Take(12, false);
			auto copy = [&](size_t n) { const unsigned char* p = s.Take(n, false); if (p) body.insert(body.end(), p, p + n); };
			auto count = [&]() -> unsigned { const unsigned n = s.U32(false); PutU32(body, n); return (int)n < 0 ? 0 : n; };
			unsigned n = count();									// behaviour hashes
			copy((size_t)n * 4);
			n = count();											// character refs
			for (unsigned i = 0; i < n && s.Ok(); ++i)
			{
				s.U32(false);
				PutU16(body, s.U16(false));
				s.U16(false);
				s.U16(false);
			}
			n = count();											// instances
			for (unsigned i = 0; i < n && s.Ok(); ++i)
			{
				copy(40);
				if (version == 8)
					s.U32(false);
			}
			n = s.U32(false);										// Kanzen-only U32 pairs
			s.Take((size_t)((int)n < 0 ? 0 : n) * 8, false);
			n = count();											// block refs
			for (unsigned i = 0; i < n && s.Ok(); ++i)
			{
				copy(8);
				const unsigned char* k = s.Take(1, false);
				if (!k)
					break;
				body.push_back(*k);
				copy((size_t)*k * 4 + 24 + 4);
			}
			n = count();											// route counts
			copy((size_t)n * 4);
			s.U32(false);
			if (version > 6 && version < 9)
			{
				n = s.U32(false);
				for (unsigned i = 0; i < n && s.Ok(); ++i)
				{
					s.U32(false);
					s.Take(s.U32(false), false);
				}
				if (version == 8)
				{
					n = s.U32(false);
					for (unsigned i = 0; i < n && s.Ok(); ++i)
					{
						s.Take((size_t)s.U32(false) * 4, false);
						s.Take(s.U32(false), false);
					}
				}
			}
			copy(2 + 16);											// block grid
			n = count();
			for (unsigned i = 0; i < n && s.Ok(); ++i)
			{
				const unsigned k = count();
				copy((size_t)k * 2);
			}
			PutU32(body, 0);										// no SH node records
			if (!s.Ok() || s.At() != s.Size())
			{
				char msg[96];
				sprintf_s(msg, "Crowd version %u does not walk to its end (%s)", version, s.Ok() ? "bytes left" : s.Error().c_str());
				error = msg;
				return false;
			}
			const size_t cAt = BeginBlock(out, b, "Crowd", b.count);
			out.insert(out.end(), body.begin(), body.end());
			EndBlock(out, cAt);
			return true;
		}
	}

	// Kanzen level render files: "360 ", Render { Header, Scene, LS, Lights, PVSData,
	// CubePt3, [Portals,] Crowd, Weather }. Kanzen's level loader (FUN_8261b878) hands
	// Header, Scene, Lights, LS, PVSData, CubePt3 and Portals to the render level
	// (FUN_8247c0e8, Blur 1's C_RenderLevel::Read with CubePt3 for CubePt2) and every
	// other block to a registry of level components by name (FUN_8261b750).
	//  - Scene: converted by BlurX's scene transcoder (x360convert/engine/SceneTranscode*).
	//  - LS: NormaliseLightSamples; CubePt3: NormaliseCubemaps.
	//  - Weather: a registered Kanzen component with no Blur 1 counterpart (Header,
	//    Zones; contents not established): dropped.
	//  - Header, Lights, PVSData, Portals, Crowd: kept; Blur1Multi's converters run
	//    Blur 1's readers over them and refuse any that does not read exactly.
	E_Result NormaliseLevelRender(const unsigned char* in, size_t size, std::vector<unsigned char>& out, std::string& error)
	{
		if (size < 4 || memcmp(in, "360 ", 4) != 0)
			return E_Result_NotHandled;
		std::vector<C_Block> top;
		if (!Blocks(in + 4, size - 4, top) || top.size() != 1 || !top[0].Is("Render"))
		{
			error = "not a Render block tree";
			return E_Result_Failed;
		}
		std::vector<C_Block> kids;
		if (!Blocks(top[0].Payload(), top[0].PayloadSize(), kids))
		{
			error = "Render children do not fill it";
			return E_Result_Failed;
		}
		out.clear();
		out.reserve(size);
		out.insert(out.end(), in, in + 4);
		const size_t rAt = BeginBlock(out, top[0], "Render", 0);
		bool changed = false;
		unsigned count = 0;
		for (size_t i = 0; i < kids.size(); ++i)
		{
			const C_Block& k = kids[i];
			bool ok = true;
			if (k.Is("LS"))
				ok = NormaliseLightSamples(k, out, changed, error);
			else if (k.Is("CubePt3"))
			{
				ok = NormaliseCubemaps(k, out, error);
				changed = true;
			}
			else if (k.Is("Weather"))
			{
				changed = true;
				continue;
			}
			else if (k.Is("Crowd") && k.PayloadSize() >= 4 && GetU32(k.Payload()) > 6)
			{
				ok = NormaliseCrowd(k, out, error);
				changed = true;
			}
			else
				CopyBlock(out, k);
			if (!ok)
				return E_Result_Failed;
			++count;
		}
		if (!changed)
			return E_Result_NotHandled;
		EndBlock(out, rAt);
		// Render's count field stays as the file has it (0 on every disc; the engine walks by size)
		out[rAt + 12] = top[0].at[12];
		out[rAt + 13] = top[0].at[13];
		(void)count;
		return E_Result_Normalised;
	}

	// Kanzen models: "360 ", then blocks (16-byte header: 8-byte id, U32 size with the
	// header, U16 child count, 2 bytes). The Model block holds a fifth child, SmpVol,
	// between Render and Coll (Nov 10 envsphere.model: 167 bytes). Blur 1's reader
	// (C_ModelResource::LoadModelData) walks Model's children by id and has no SmpVol;
	// it is dropped here (contents not established) and Model's size and count follow.
	// The scene archive inside Render is converted by BlurX's scene transcoder.
	E_Result NormaliseModel(const unsigned char* in, size_t size, std::vector<unsigned char>& out, std::string& error)
	{
		const size_t header = 16;
		if (size < 4 || memcmp(in, "360 ", 4) != 0)
			return E_Result_NotHandled;
		out.clear();
		bool dropped = false;
		out.insert(out.end(), in, in + 4);
		size_t at = 4;
		while (at < size)
		{
			if (size - at < header)
			{
				error = "model block header past the end";
				return E_Result_Failed;
			}
			const size_t blockSize = GetU32(in + at + 8);
			if (blockSize < header || blockSize > size - at)
			{
				error = "model block size past the end";
				return E_Result_Failed;
			}
			if (memcmp(in + at, "Model\0\0\0", 8) != 0)
			{
				out.insert(out.end(), in + at, in + at + blockSize);
				at += blockSize;
				continue;
			}
			const size_t modelAt = out.size();
			out.insert(out.end(), in + at, in + at + header);
			unsigned count = (unsigned)in[at + 12] << 8 | in[at + 13];
			size_t kid = at + header;
			const size_t end = at + blockSize;
			while (kid < end)
			{
				const size_t kidSize = end - kid >= header ? GetU32(in + kid + 8) : 0;
				if (kidSize < header || kidSize > end - kid)
				{
					error = "Model children do not fill it";
					return E_Result_Failed;
				}
				// Coll: a Gravity stream, "Head" + U32 version. Blur 1 writes 10, Kanzen
				// 11 (read by BlurX's copy of gravity/sdk/BodyHelper.h). A later version
				// is not known: such a model is refused (the profile's next disc serves it)
				// before Blur 1's importer mis-sizes an allocation on it.
				if (memcmp(in + kid, "Coll\0\0\0\0", 8) == 0 && kidSize >= header + 8 &&
					memcmp(in + kid + header, "Head", 4) == 0 && GetU32(in + kid + header + 4) > 11)
				{
					char msg[96];
					sprintf_s(msg, "Gravity version %u in Coll (BlurX reads 10 and 11)", GetU32(in + kid + header + 4));
					error = msg;
					return E_Result_Failed;
				}
				if (memcmp(in + kid, "SmpVol\0\0", 8) == 0)
				{
					dropped = true;
					--count;
				}
				else
					out.insert(out.end(), in + kid, in + kid + kidSize);
				kid += kidSize;
			}
			const unsigned newSize = (unsigned)(out.size() - modelAt);
			out[modelAt + 8] = (unsigned char)(newSize >> 24);
			out[modelAt + 9] = (unsigned char)(newSize >> 16);
			out[modelAt + 10] = (unsigned char)(newSize >> 8);
			out[modelAt + 11] = (unsigned char)newSize;
			out[modelAt + 12] = (unsigned char)(count >> 8);
			out[modelAt + 13] = (unsigned char)count;
			at += blockSize;
		}
		return dropped ? E_Result_Normalised : E_Result_NotHandled;
	}
}
