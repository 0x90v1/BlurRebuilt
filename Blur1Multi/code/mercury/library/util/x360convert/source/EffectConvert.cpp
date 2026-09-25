// [Blur1Multi] .fxb: Xbox 360 XEFX -> PC PCFX (T074, decisions D03/D11).
//
// FX::ImportFXB (graphics/fx/source/FXB.cpp) reads an int 'FXB2' (its byte
// order sets the file's), a root block, and in it the block for its platform
// ("PCFX" on PC, "XEFX" on Xenon), which holds an FX::Effect (fx/effect.h,
// version 9 here) read with BinaryDeserialiser: name, techniques (passes:
// render states, vertex and pixel Function, annotations), parameters,
// parameter data, vertex input semantics, structs. The graph converts field by
// field; each Function's microcode, a Xenos shader container on the 360, is
// recompiled to Direct3D 9 shader model 3 (XenosRecomp, vendor/xenosrecomp) and
// compiled with D3DCompile. The recompiled program keeps every constant on the
// register the Xenos constant table gives it, so the function's bindings
// (parameter -> register) carry over unchanged.
//
// Field widths, from the readers:
//  - strings (Util::C_String, SharedString::String): U32 length + characters;
//  - Btl::Vector: U32 count + elements;
//  - nglFXRenderStates: read raw (sizeof, 35 words) but written word by word
//    through the swapping serialiser, so each word is swapped;
//  - Annotation values: written raw (ar.Write(&Int, c), c = 1/4/4/16 bytes by
//    type) by the PC-hosted exporter, so they are the exporter's bytes on both
//    platforms and pass through;
//  - FunctionBinding: five S32 + U32 offset + string; Parameter: two strings,
//    three S32, U8 bShared, annotations.

#include <x360convert/X360Convert.h>

#include "ChunkFile.h"
#include "xenosrecomp_d3d9.h"

#include <windows.h>
#include <d3dcompiler.h>

#include <stdio.h>
#include <string.h>

namespace X360Convert
{
	namespace
	{
		const unsigned kEffectVersion = 9;		// NGL_FX_VERSION
		const unsigned kRenderStatesSize = 140;	// sizeof(nglFXRenderStates)

		// D3DCompile from the system's d3dcompiler_47.dll.
		typedef HRESULT(WINAPI* T_D3DCompile)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*, ID3DInclude*,
			LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);

		T_D3DCompile D3DCompileFunction()
		{
			static T_D3DCompile s_pCompile = NULL;
			static bool s_tried = false;
			if (!s_tried)
			{
				s_tried = true;
				HMODULE h = LoadLibraryA("d3dcompiler_47.dll");
				if (h)
					s_pCompile = (T_D3DCompile)GetProcAddress(h, "D3DCompile");
			}
			return s_pCompile;
		}

		class C_EffectConverter
		{
		public:
			C_EffectConverter(const unsigned char* p, size_t n, std::vector<unsigned char>& out, EffectStats& stats)
				: m_p(p), m_n(n), m_at(0), m_out(out), m_stats(stats), m_ok(true) {}

			bool Effect()
			{
				const unsigned version = Word();
				if (m_ok && version != kEffectVersion)
					return Fail("effect version " + std::to_string((unsigned long long)version));
				String();
				Vector(&C_EffectConverter::Technique);
				Vector(&C_EffectConverter::Parameter);
				Vector(&C_EffectConverter::WordItem);				// ParameterData
				Vector(&C_EffectConverter::InputSemantic);
				const unsigned structs = Word();
				for (unsigned i = 0; m_ok && i < structs; ++i)
					Struct();
				if (m_ok && m_at != m_n)
					return Fail("bytes left after the effect");
				return m_ok;
			}

			const std::string& Error() const { return m_error; }

		private:
			bool Fail(const std::string& what)
			{
				if (m_ok)
				{
					char where[48];
					sprintf_s(where, " (at %u of %u)", (unsigned)m_at, (unsigned)m_n);
					m_error = what + where;
				}
				m_ok = false;
				return false;
			}

			const unsigned char* Take(size_t n)
			{
				if (!m_ok || m_at + n > m_n)
				{
					Fail("read past the end");
					return NULL;
				}
				const unsigned char* p = m_p + m_at;
				m_at += n;
				return p;
			}

			unsigned Word()
			{
				const unsigned char* p = Take(4);
				if (!p)
					return 0;
				const unsigned v = BE32(p);
				PutLE32(m_out, v);
				return v;
			}

			void Byte()
			{
				const unsigned char* p = Take(1);
				if (p)
					m_out.push_back(*p);
			}

			void Raw(size_t n)
			{
				const unsigned char* p = Take(n);
				if (p)
					m_out.insert(m_out.end(), p, p + n);
			}

			std::string String()
			{
				const unsigned n = Word();
				const unsigned char* p = Take(n);
				if (!p)
					return std::string();
				m_out.insert(m_out.end(), p, p + n);
				return std::string((const char*)p, (const char*)p + n);
			}

			void Vector(void (C_EffectConverter::*item)())
			{
				const unsigned n = Word();
				for (unsigned i = 0; m_ok && i < n; ++i)
					(this->*item)();
			}

			void WordItem() { Word(); }

			void Annotation()
			{
				String();
				const unsigned type = Word();
				switch (type)
				{
				case 1: Raw(1); break;				// BOOL
				case 2: case 3: case 4: Raw(4); break;	// INT, FLOAT, VECTOR (4 bytes, as written)
				case 5: Raw(16); break;				// MATRIX (16 bytes, as written)
				case 6: String(); break;			// STRING
				default: break;						// NONE: no value
				}
			}

			void Annotations() { Vector(&C_EffectConverter::Annotation); }

			void Binding()
			{
				for (int i = 0; i < 6; ++i)			// ParameterIdx, Handle, Type, Source, Offset, Count
					Word();
				String();
			}

			// FX::Function: U32 cMicrocode, bindings, microcode.
			void Function()
			{
				const unsigned char* pSize = Take(4);
				if (!pSize)
					return;
				const unsigned size = BE32(pSize);
				const size_t sizeAt = m_out.size();
				PutLE32(m_out, size);
				Vector(&C_EffectConverter::Binding);
				const unsigned char* microcode = Take(size);
				if (!microcode || size == 0)
					return;

				std::string hlsl, unsupported, error;
				bool isPixelShader = false;
				if (!XenosRecompD3D9::Recompile(microcode, size, hlsl, isPixelShader, unsupported, error))
				{
					Fail("program: " + error);
					return;
				}
				++m_stats.programs;
				// [Blur1Multi] Debugging aid: BLUR1MULTI_XENOS_HLSL=<dir> writes every recompiled
				// program as <dir>/xenos_<n>_<vs|ps>.hlsl (n counts programs in this process).
				if (const char* dumpDir = getenv("BLUR1MULTI_XENOS_HLSL"))
				{
					static unsigned s_dumped = 0;
					char path[1024];
					sprintf_s(path, "%s/xenos_%05u_%s.hlsl", dumpDir, s_dumped++, isPixelShader ? "ps" : "vs");
					FILE* f = NULL;
					if (fopen_s(&f, path, "wb") == 0 && f)
					{
						fwrite(hlsl.data(), 1, hlsl.size(), f);
						fclose(f);
					}
				}
				if (!unsupported.empty())
				{
					++m_stats.unsupported;
					if (m_stats.firstUnsupported.empty())
						m_stats.firstUnsupported = unsupported;
				}

				T_D3DCompile compile = D3DCompileFunction();
				if (!compile)
				{
					Fail("d3dcompiler_47.dll not available");
					return;
				}
				ID3DBlob* pCode = NULL;
				ID3DBlob* pErrors = NULL;
				const HRESULT hr = compile(hlsl.data(), hlsl.size(), "xenos", NULL, NULL, "main",
					isPixelShader ? "ps_3_0" : "vs_3_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pCode, &pErrors);
				if (FAILED(hr) || !pCode)
				{
					++m_stats.failed;
					std::string messages = pErrors ? std::string((const char*)pErrors->GetBufferPointer(), pErrors->GetBufferSize()) : std::string("no messages");
					if (m_stats.firstFailure.empty())
					{
						m_stats.firstFailure = messages;
						m_stats.firstFailureHlsl = hlsl;
					}
					if (pErrors)
						pErrors->Release();
					if (pCode)
						pCode->Release();
					Fail("fxc: " + messages.substr(0, 300));
					return;
				}
				if (pErrors)
					pErrors->Release();
				const unsigned char* bytecode = (const unsigned char*)pCode->GetBufferPointer();
				const unsigned bytecodeSize = (unsigned)pCode->GetBufferSize();
				SetLE32(m_out, sizeAt, bytecodeSize);
				m_out.insert(m_out.end(), bytecode, bytecode + bytecodeSize);
				pCode->Release();
				++m_stats.compiled;
			}

			// FX::Pass: name, render states, vertex and pixel function, annotations.
			// The last render state, HiZEnable, is a Xenos state: the 360 exporter
			// writes 2 where the PC exporter writes 0 (every pass of the 820 retail
			// effects that exist on both platforms).
			void Pass()
			{
				String();
				for (unsigned i = 0; m_ok && i < kRenderStatesSize / 4; ++i)
				{
					const size_t at = m_out.size();
					Word();
					if (m_ok && i == kRenderStatesSize / 4 - 1)
						SetLE32(m_out, at, 0);
				}
				Function();
				Function();
				Annotations();
			}

			void Technique()
			{
				String();
				Vector(&C_EffectConverter::Pass);
				Annotations();
			}

			void Parameter()
			{
				String();							// Name
				String();							// Semantic
				Word();								// Type
				Word();								// Size
				Word();								// Offset
				Byte();								// bShared
				Annotations();
			}

			// FX::InputSemantic. The PC exporter (fx/source/targets/pcfx.cpp) shifts
			// D3DDECLUSAGE_COLOR and above down by one ("our output doesn't have
			// POSITIONT") to E_FXSemantic; the 360 exporter writes the usage as it is.
			void InputSemantic()
			{
				const size_t at = m_out.size();
				const unsigned usage = Word();
				if (m_ok && usage >= 10 && usage != 0xffffffff)
					SetLE32(m_out, at, usage - 1);
				Word();
				Annotations();
			}

			void StructMember()
			{
				String();
				Word();
				Annotations();
			}

			void Struct()
			{
				String();
				Vector(&C_EffectConverter::StructMember);
			}

			const unsigned char* m_p;
			size_t m_n, m_at;
			std::vector<unsigned char>& m_out;
			EffectStats& m_stats;
			bool m_ok;
			std::string m_error;
		};

		E_Result Fail(std::string& error, const std::string& text)
		{
			error = text;
			return E_Result_Failed;
		}
	}

	E_Result ConvertEffect(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error, EffectStats* pStats)
	{
		EffectStats localStats;
		EffectStats& stats = pStats ? *pStats : localStats;

		// int 'FXB2' written big-endian: the bytes "FXB2"
		if (inSize < 4 || memcmp(in, "FXB2", 4) != 0)
			return Fail(error, "not a big-endian FXB2 file");
		Block root;
		std::vector<Block> kids;
		if (!ReadBlockBE(in + 4, inSize - 4, root) || !ChildrenBE(root, kids))
			return Fail(error, "bad FXB block tree");

		const Block* pXefx = NULL;
		for (size_t i = 0; i < kids.size(); ++i)
			if (kids[i].id == "XEFX")
				pXefx = &kids[i];
		if (!pXefx)
			return Fail(error, "no XEFX block");

		out.clear();
		out.reserve(inSize);
		PutLE32(out, ('F' << 24) | ('X' << 16) | ('B' << 8) | '2');	// the PC exporter's int 'FXB2'
		const size_t rootAt = BeginBlockLE(out, root, 1);
		const size_t pcfxAt = out.size();
		out.insert(out.end(), (const unsigned char*)"PCFX\0\0\0\0", (const unsigned char*)"PCFX\0\0\0\0" + 8);
		PutLE32(out, 0);
		PutLE16(out, pXefx->count);
		out.push_back(pXefx->header[14]);
		out.push_back(pXefx->header[15]);

		C_EffectConverter converter(pXefx->payload, pXefx->PayloadSize(), out, stats);
		if (!converter.Effect())
			return Fail(error, converter.Error());
		EndBlockLE(out, pcfxAt);
		EndBlockLE(out, rootAt);
		return E_Result_Converted;
	}
}
