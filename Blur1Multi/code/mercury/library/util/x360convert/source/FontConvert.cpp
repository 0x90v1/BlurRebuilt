// [Blur1Multi] .font: Xbox 360 -> PC.
//
// Render::Font::C_FontResource::Load (graphics/render/source/font/Font.cpp) reads
// a "Font" block holding:
//   "Header":  U16 major, U16 minor version
//   "Data":    U16 character count, then C_CharInfo records
//              (U16 unicode, U16 texture page, F32 offset, height, width, UMin, VMax, UMax, VMin)
//   "Texture": U16 page count, then each page as Display::LoadTexture2D reads it
// Scalars are byte-swapped; pages go through ConvertTexturePayload.
// PS3 fonts have the same blocks; ConvertTexturePayload reads their pages as GCM
// textures when the source is PS3 (Ps3Texture.h). The retail debug font's page is
// a swizzled 512x512 A8 and converts to the retail PC file byte for byte.

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
	}

	E_Result ConvertFont(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error)
	{
		Block root;
		if (!ReadBlockBE(in, inSize, root) || root.id != "Font" || root.size != inSize)
			return Fail(error, "not a big-endian Font block tree");
		std::vector<Block> children;
		if (!ChildrenBE(root, children))
			return Fail(error, "bad child blocks");

		out.clear();
		out.reserve(inSize);
		const size_t rootAt = BeginBlockLE(out, root, root.count);
		for (size_t i = 0; i < children.size(); ++i)
		{
			const Block& b = children[i];
			const unsigned char* p = b.payload;
			const size_t n = b.PayloadSize();
			const size_t at = BeginBlockLE(out, b, b.count);
			if (b.id == "Header")
			{
				if (n != 4)
					return Fail(error, "font Header is not two U16s");
				PutLE16(out, BE16(p));
				PutLE16(out, BE16(p + 2));
			}
			else if (b.id == "Data")
			{
				if (n < 2)
					return Fail(error, "font Data has no character count");
				const unsigned count = BE16(p);
				if (n != 2 + (size_t)count * 32)
					return Fail(error, "font Data size does not match its character count");
				PutLE16(out, count);
				for (unsigned c = 0; c < count; ++c)
				{
					const unsigned char* r = p + 2 + c * 32;
					PutLE16(out, BE16(r));
					PutLE16(out, BE16(r + 2));
					for (unsigned f = 0; f < 7; ++f)
						PutLE32(out, BE32(r + 4 + f * 4));
				}
			}
			else if (b.id == "Texture")
			{
				if (n < 2)
					return Fail(error, "font Texture has no page count");
				const unsigned pages = BE16(p);
				PutLE16(out, pages);
				size_t off = 2;
				for (unsigned pg = 0; pg < pages; ++pg)
				{
					size_t consumed = 0;
					const E_Result r = ConvertTexturePayload(p + off, n - off, consumed, out, error);
					if (r != E_Result_Converted)
						return r;
					off += consumed;
				}
				if (off != n)
					return Fail(error, "font pages do not fill the Texture block");
			}
			else
				return Fail(error, "unknown block '" + b.id + "' in a font");
			EndBlockLE(out, at);
		}
		EndBlockLE(out, rootAt);
		return E_Result_Converted;
	}
}
