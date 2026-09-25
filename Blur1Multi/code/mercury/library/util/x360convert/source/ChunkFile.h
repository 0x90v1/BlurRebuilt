// [Blur1Multi] Chunk (C_FileBlock) trees for the Xbox 360 -> PC converters.
//
// A block is a 16-byte C_BlockHeader followed by its payload
// (system/serialise/C_FileBlock.h): char id[8], U32 size (header included),
// U16 child count, U8 E_StreamHeaderType, U8 compression type. C_FileBlock
// reads the header through the stream's endian flag, so on 360 files size and
// count are big-endian; the id and the two U8 fields are bytes.
#pragma once

#include <string.h>
#include <string>
#include <vector>

namespace X360Convert
{
	inline unsigned BE16(const unsigned char* p) { return (p[0] << 8) | p[1]; }
	inline unsigned BE32(const unsigned char* p) { return ((unsigned)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }

	inline void PutLE16(std::vector<unsigned char>& o, unsigned v) { o.push_back((unsigned char)v); o.push_back((unsigned char)(v >> 8)); }
	inline void PutLE32(std::vector<unsigned char>& o, unsigned v) { for (int i = 0; i < 4; ++i) o.push_back((unsigned char)(v >> (8 * i))); }
	inline void SetLE32(std::vector<unsigned char>& o, size_t at, unsigned v) { for (int i = 0; i < 4; ++i) o[at + i] = (unsigned char)(v >> (8 * i)); }

	const size_t kBlockHeaderSize = 16;

	struct Block
	{
		std::string id;
		const unsigned char* header;	// the 16 header bytes
		const unsigned char* payload;	// payload start (header + 16)
		size_t size;					// whole block, header included
		unsigned count;					// child count as stored
		size_t PayloadSize() const { return size - kBlockHeaderSize; }
	};

	// Reads the big-endian block header at 'p' (with 'avail' bytes available).
	inline bool ReadBlockBE(const unsigned char* p, size_t avail, Block& b)
	{
		if (avail < kBlockHeaderSize)
			return false;
		char id[9];
		memcpy(id, p, 8);
		id[8] = 0;
		b.id = id;
		b.header = p;
		b.payload = p + kBlockHeaderSize;
		b.size = BE32(p + 8);
		b.count = BE16(p + 12);
		return b.size >= kBlockHeaderSize && b.size <= avail;
	}

	// Children of a block: consecutive blocks filling its payload.
	inline bool ChildrenBE(const Block& parent, std::vector<Block>& out)
	{
		out.clear();
		const unsigned char* p = parent.payload;
		size_t left = parent.PayloadSize();
		while (left)
		{
			Block c;
			if (!ReadBlockBE(p, left, c))
				return false;
			out.push_back(c);
			p += c.size;
			left -= c.size;
		}
		return true;
	}

	// Appends a PC (little-endian) header for a block with the same id, type and
	// compression bytes as 'like'; returns its offset so the size can be patched.
	inline size_t BeginBlockLE(std::vector<unsigned char>& o, const Block& like, unsigned count)
	{
		size_t at = o.size();
		o.insert(o.end(), like.header, like.header + 8);
		PutLE32(o, 0);
		PutLE16(o, count);
		o.push_back(like.header[14]);
		o.push_back(like.header[15]);
		return at;
	}

	inline void EndBlockLE(std::vector<unsigned char>& o, size_t at)
	{
		SetLE32(o, at + 8, (unsigned)(o.size() - at));
	}
}
