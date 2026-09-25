// [Blur1Multi] .particlemesh: Xbox 360 -> PC.
//
// Written by C_UserParticleContainer::Serialise with BinarySerialiserAutoEndian
// (graphics/dynamic/include/dynamic/particle/tools_shared/UserParticleContainer.inl),
// so the 360 file is the PC layout with every scalar big-endian:
//   "PtlData": U32 file version, then block "UsrVar3":
//     U32 variant count; per variant (UserShape3DVariants.inl):
//       name (U32 length + characters), U32 vertex count, F32 scale, U32 shape count,
//       shapes x vertices x C_Vertex (14 F32: position 3, normal 3, uv 2, tangent 3, binormal 3)

#include <x360convert/X360Convert.h>

#include "ChunkFile.h"

namespace X360Convert
{
	namespace
	{
		E_Result Fail(std::string& error, const std::string& text)
		{
			error = text;
			return E_Result_Failed;
		}

		struct Reader
		{
			const unsigned char* p;
			size_t n, at;
			bool U32(unsigned& v) { if (at + 4 > n) return false; v = BE32(p + at); at += 4; return true; }
		};
	}

	E_Result ConvertParticleMesh(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error)
	{
		Block root;
		if (!ReadBlockBE(in, inSize, root) || root.id != "PtlData" || root.size != inSize || root.PayloadSize() < 4)
			return Fail(error, "not a big-endian PtlData block");
		out.clear();
		out.reserve(inSize);
		const size_t rootAt = BeginBlockLE(out, root, root.count);
		PutLE32(out, BE32(root.payload));				// file version
		Block child;
		if (root.PayloadSize() > 4)
		{
			if (!ReadBlockBE(root.payload + 4, root.PayloadSize() - 4, child) || child.size != root.PayloadSize() - 4)
				return Fail(error, "PtlData does not hold exactly one child block");
			if (child.id != "UsrVar3")
				return Fail(error, "unknown block '" + child.id + "' in PtlData");
			const size_t at = BeginBlockLE(out, child, child.count);
			Reader r = { child.payload, child.PayloadSize(), 0 };
			unsigned variants;
			if (!r.U32(variants))
				return Fail(error, "UsrVar3 has no variant count");
			PutLE32(out, variants);
			for (unsigned v = 0; v < variants; ++v)
			{
				unsigned len, vertices, scale, shapes;
				if (!r.U32(len) || r.at + len > r.n)
					return Fail(error, "variant name runs past the block");
				PutLE32(out, len);
				out.insert(out.end(), r.p + r.at, r.p + r.at + len);
				r.at += len;
				if (!r.U32(vertices) || !r.U32(scale) || !r.U32(shapes))
					return Fail(error, "variant header runs past the block");
				PutLE32(out, vertices);
				PutLE32(out, scale);
				PutLE32(out, shapes);
				const size_t floats = (size_t)shapes * vertices * 14;
				if (r.at + floats * 4 > r.n)
					return Fail(error, "variant vertices run past the block");
				for (size_t f = 0; f < floats; ++f)
				{
					PutLE32(out, BE32(r.p + r.at));
					r.at += 4;
				}
			}
			if (r.at != r.n)
				return Fail(error, "UsrVar3 has bytes after its last variant");
			EndBlockLE(out, at);
		}
		EndBlockLE(out, rootAt);
		return E_Result_Converted;
	}
}
