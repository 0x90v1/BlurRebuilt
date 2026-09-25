// [Blur1Multi] Xbox 360 and PS3 -> PC asset conversion (T072, T076, decision D11).
//
// Converts one file's bytes as they come out of an Xbox 360 or PS3 pak (or the
// loose files on the disc) into the bytes the PC build of Blur writes for the same
// asset, so the engine's PC readers can load it unchanged. Engine-independent:
// the runtime file-system layer and the standalone verifier
// (code/tools/x360convert-verify) share this code.
//
// Every converter is written from the engine's own serialisers for the PC side
// and from the console side's writers (the engine's PLATFORM_PS3 code, Blur2Test's
// decoders for the 360), and is checked byte for byte against the retail PC file
// of the same asset where one exists (docs/analysis.md "Xbox 360 -> PC conversion",
// "PS3 -> PC conversion"). The library keeps its first name, x360convert.
#pragma once

#include <stddef.h>
#include <string>
#include <vector>

namespace X360Convert
{
	// The console that wrote the data. Xbox 360 and PS3 share the big-endian chunk
	// files, XT flasks, localisation, cycles and skeletons byte for byte (docs/analysis.md
	// "PS3 -> PC conversion"); textures, GPU buffers, effects and audio banks differ.
	// One process serves one source: the runtime sets it once before the first file
	// is converted (GameSystems.cpp CreateFsLayers), the verifier from its arguments.
	enum E_Source
	{
		E_Source_Xbox360 = 0,
		E_Source_PS3,
	};
	void SetSource(E_Source source);
	E_Source GetSource();
	// The chunk files' platform FourCC for the current source: "360 " or "PS3 ".
	const char* SourceFourCC();

	enum E_Result
	{
		E_Result_NotHandled = 0,	// not a format this library converts: serve the bytes unchanged
		E_Result_Converted,			// 'out' holds the PC file
		E_Result_Failed,			// a format this library converts, but the input is not valid for it
	};

	// 'path' is the virtual path the engine asked for (used for the extension).
	E_Result ConvertFile(const char* path, const unsigned char* in, size_t inSize,
		std::vector<unsigned char>& out, std::string& error);

	// Whether ConvertFile handles files with this path at all (by extension), so a
	// caller can pass everything else straight through without reading it.
	bool HandlesPath(const char* path);

	// Individual converters.
	E_Result ConvertTexture(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error);
	E_Result ConvertFont(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error);
	E_Result ConvertBundle(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error);
	E_Result ConvertLoc(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error);
	E_Result ConvertFlask(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error);
	// .baf audio banks (PS3: the same 4-bit ADPCM as PC; big-endian headers, PCM16 samples
	// byte-swapped). Xbox 360 banks hold XMA and are not handled here (T075).
	E_Result ConvertBank(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error);
	// .dat audio object definitions (tag 'AOBJ'), from either console: every component's
	// fields swapped at their reader widths (AobjConvert.cpp). Other .dat files are not handled.
	E_Result ConvertAudioObject(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error);

	// The byte edits that turn a PS3 bank into a PC bank, for serving a streaming bank
	// without reading its payload: 'head' holds the file from offset 0 up to at least the
	// end of the last WAVE header (BankHeadersSize). The headers are replaced whole; the
	// DATA chunk headers in front of each payload get their U32 size swapped; PCM S16
	// payloads are swapped per sample.
	struct BankPatch
	{
		std::vector<unsigned char> headers;		// converted bytes for [0, headers.size())
		std::vector<unsigned> swap32;			// offsets of U32s to swap
		struct Range { unsigned offset, size; };
		std::vector<Range> swap16;				// even-aligned ranges of 16-bit samples
	};
	// Bytes PlanBank needs: false with 'needed' set when 'headSize' is not enough yet.
	bool BankHeadersSize(const unsigned char* head, size_t headSize, size_t& needed);
	E_Result PlanBank(const unsigned char* head, size_t headSize, size_t fileSize, BankPatch& patch, std::string& error);
	// Widens [offset, offset + size) so that no swapped U32 or sample straddles its ends;
	// ApplyBankPatch must be given whole units.
	void WidenForBankPatch(const BankPatch& patch, unsigned long long& offset, unsigned long long& size);
	// Applies the edits that fall in [offset, offset + size) to 'data' (the original bytes there).
	void ApplyBankPatch(const BankPatch& patch, unsigned long long offset, unsigned char* data, size_t size);

	E_Result ConvertParticleMesh(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error);

	// One serialised texture as Display::LoadTexture2D reads it (a .texture "Data"
	// payload, a font page): the Xenon header + GPU allocation at 'p' becomes
	// TextureDesc + levels appended to 'out'. 'consumed' is the 360 bytes used.
	E_Result ConvertTexturePayload(const unsigned char* p, size_t avail, size_t& consumed,
		std::vector<unsigned char>& out, std::string& error);

	// A texture as RenderingData's LodWriter<Texture> splits it for one lod
	// (renderingdata build/archive.cpp): the 64-byte Xenon header of the texture
	// from that lod's top level down, and separately the GPU data of its first
	// levels (all of them for lod 0, the first one for any later lod). The header
	// becomes a PC TextureDesc, the data the PC levels.
	E_Result ConvertTextureLodHeader(const unsigned char* header, size_t size, std::vector<unsigned char>& out, std::string& error);
	E_Result ConvertTextureLodData(const unsigned char* header, int lod, const unsigned char* data, size_t dataSize,
		std::vector<unsigned char>& out, std::string& error);

	// .fxb: the XEFX effect becomes a PCFX effect whose programs are the Xenos
	// programs recompiled to shader model 3 (EffectConvert.cpp). Slow (XenosRecomp
	// and D3DCompile per program): the runtime caches the results per source.
	struct EffectStats
	{
		unsigned programs, compiled, failed, unsupported;
		std::string firstFailure, firstFailureHlsl, firstUnsupported;
		EffectStats() : programs(0), compiled(0), failed(0), unsupported(0) {}
	};
	E_Result ConvertEffect(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error,
		EffectStats* pStats = NULL);

	// .fxb from a PS3 source: the PS3FX effect (the same graph) becomes a PCFX effect whose
	// programs are the RSX vertex and fragment programs (CGB blobs) translated to HLSL and
	// compiled for shader model 3 (Ps3EffectConvert.cpp, RsxTranslate.cpp). Cached per
	// source like ConvertEffect's output.
	E_Result ConvertPs3Effect(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error,
		EffectStats* pStats = NULL);
}
