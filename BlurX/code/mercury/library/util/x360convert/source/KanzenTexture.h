// [BlurX] Kanzen (Blur 2 prototype) textures stored with Arch::Deserialiser
// compression type 3 ("kind 3", MCT field planes). Port of Blur2Test's decoder
// (_tools/extract_xbox_stage_materials.py: measure_xmem, nested_xmem_streams,
// scan_texture_blocks, rebuild_texture_block_levels, decode_tail_levels). Each
// fragment ("segment") of the block is an 88-byte control (destination rectangle
// at +80/+84) followed by size-prefixed XMem streams, one per BC field plane
// (DXT1: 4-byte endpoints, 4-byte selectors; DXT5: fields of 4, 4, 2 and 6 bytes
// in that order). The planes interleave back into BC blocks; each segment's
// blocks fill its rectangle of the top surface in row-major order, and the bytes
// after the rectangle carry the rectangle scaled to each lower level until a
// scaled rectangle is no longer block-aligned (the hardware's packed tail, not
// decoded). No engine dependencies.
#pragma once

#include <string>
#include <vector>

namespace X360Convert
{
	struct KanzenSegment
	{
		unsigned logical;					// the fragment's declared original size
		std::vector<unsigned char> payload;	// its stored bytes (from the aligned start)
	};

	// 'header' is the 64-byte big-endian Xenos texture header of the lod (fetch
	// constant at +0x1c). 'out' receives the levels as PC data, top first, in the
	// layout ConvertTextureLodData gives (linear BC blocks, endian undone): lod 0
	// every level decoded (at most the header's chain), a later lod its top level.
	// 'levels' is the number of levels in 'out'. False with 'error' set when the
	// data does not decode exactly. 'partial' (optional) is set when every segment
	// decoded but their rectangles tile only part of the surface: a tier that is a
	// partial residency transfer (Blur2Test extract_xbox_stage_materials.py
	// "unsupported_partial_stream_tiers": the three 1024-wide Dubai albedos), which
	// no reader decodes; the caller drops the tier. 'zeroFillMissing': such a surface
	// is decoded anyway, the blocks no segment covers left zero (a caller's stopgap;
	// the levels below it stop where the rectangles no longer tile them).
	bool ConvertKanzenTextureLod(const unsigned char* header, int lod, const std::vector<KanzenSegment>& segments,
		std::vector<unsigned char>& out, unsigned& levels, std::string& error, bool* partial = 0,
		bool zeroFillMissing = false);
}
