// [Blur1Multi] .fxb: PS3 PS3FX -> PC PCFX (T076, decisions D03/D11). See X360Convert.h.
//
// The PS3 exporter (fx/source/targets/ps3fx.cpp, NglPs3Fx::IsBigEndian) writes the
// same FX::Effect graph as the PC and 360 ones (fx/include/fx/effect.h, version 9),
// big-endian, in a "PS3FX" block. All 820 retail PS3 effects parse to their last
// byte with the reader below. Field by field against the retail PC effects of the
// same name: names, structs, pass annotations, parameters, parameter data (the PS3
// sets the sRGB sampler bit as PC does) and vertex input semantics (already in the
// PC numbering; no usage shift) carry over; render states carry over except
// ColorWriteEnable, which the PS3 writes as a GCM colour mask (CELL_GCM_COLOR_MASK_*:
// A bit 24, R bit 16, G bit 8, B bit 0; ngl/ps3/ngl_ps3_gpu_types.h:629-634,
// Fx_export.cpp:879-885) and is mapped to the D3D mask (R 1, G 2, B 4, A 8).
//
// Programs are CGB blobs (RsxTranslate.h). Each is translated to HLSL, compiled for
// shader model 3 with D3DCompile, and its bindings rebuilt:
//  - Vertex: the exporter merged the bindings per parameter (ps3fx.cpp MergeBindings)
//    and left, in the CGB constant table, only literals the program needs
//    (StripDefaultValuesFromCGB); those become HLSL literals. Parameters placed at or
//    above c256 (sce-cgc allocates unregistered parameters downward from c467) move to
//    the lowest free D3D9 registers; everything else keeps its register.
//  - Fragment: RSX fragment programs have no constant registers; uniforms are float4
//    literals embedded in the ucode, patched per draw by name (fx/source/System.cpp
//    EvaluatePixelShaderBindings, FxContext.cpp PatchPixelShaderConstants). The PS3
//    bindings are per struct member (Offset in floats) and carry the register sce-cgc
//    listed (c[n]) or GPU_INVALID_PARAM. Each parameter becomes one binding with Offset
//    0 (the PC loader asserts it, fx/source/System.cpp:210) at its listed register when
//    it fits c0-c223, else at the lowest free run; every embedded copy the CGB map lists
//    for a bound member reads C[register]. Unbound copies stay literals, as they did on
//    the PS3 (nothing patched them).
//  - Samplers keep their texture unit. Dimension: the programs do not record it (a fetch
//    names only a unit), and the effect has no sampler type. Rule chosen by the user
//    (2026-09-25): a sampler is a cube when its packed AddressW (bits 6-8 of the sampler
//    state word, Fx_export.cpp WriteSamplerParameter) is non-zero, 2D otherwise. This rests
//    on how the retail effects were authored: in the source snapshot AddressW is set in
//    exactly three sampler declarations, all cube samplers (amax/game/Shaders/Common/
//    Lighting.h:66-78 g_tEnvironment, graphics/vehicle/include/vehicle/shaders/
//    vehicle_common.h:594-617 g_sVehicleEnvironmentTexture and g_sLightEnvironmentTexture),
//    and in the 820 PS3 effects AddressW is non-zero for exactly those three names (780,
//    960 and 560 bindings) and zero for all other sampler bindings. From the programs
//    alone, of 3,613 effect/sampler pairs 246 are proven to read at most 2 coordinates,
//    808 read 2 by swizzle pattern and 2,369 are undetermined. No PS3 fragment program
//    samples a volume texture, and no vertex program samples any texture.

#include <x360convert/X360Convert.h>

#include "ChunkFile.h"
#include "RsxTranslate.h"

#include <windows.h>
#include <d3dcompiler.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace X360Convert
{
	E_Result ConvertPs3Effect(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error,
		EffectStats* pStats)
	{
		return RsxTranslate::ConvertPs3Effect(in, inSize, out, error, pStats, NULL);
	}
}

namespace RsxTranslate
{
	using namespace X360Convert;

	namespace
	{
		const unsigned kEffectVersion = 9;		// NGL_FX_VERSION
		const unsigned kRenderStates = 35;		// sizeof(nglFXRenderStates) / 4
		const unsigned kColorWriteEnable = 14;	// nglFXRenderStates word
		const unsigned kInvalidHandle = 0xffffffffu;	// GPU_INVALID_PARAM / GPU_INVALID_SAMPLER as written
		const unsigned kBindFloat = 0, kBindSampler = 3;	// NGLFX_BIND_FLOAT, NGLFX_BIND_SAMPLER
		const unsigned kParamSampler = 14;		// NGLFX_PARAM_SAMPLER
		const unsigned kVsRegisters = 256, kPsRegisters = 224;

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

		void PutString(std::vector<unsigned char>& o, const std::string& s)
		{
			PutLE32(o, (unsigned)s.size());
			o.insert(o.end(), s.begin(), s.end());
		}

		struct Binding
		{
			unsigned param, handle, type, source, offset, count;
			std::string name;	// as stored (the PS3 exporter includes the NUL)
			std::string Name() const { return std::string(name.c_str()); }
		};

		struct Function
		{
			unsigned size;
			std::vector<Binding> bindings;
			const unsigned char* code;
		};

		struct Pass
		{
			std::string name;
			unsigned states[kRenderStates];
			Function vs, ps;
			std::vector<unsigned char> annotations;	// already little-endian
		};

		struct Technique
		{
			std::string name;
			std::vector<Pass> passes;
			std::vector<unsigned char> annotations;
		};

		struct Parameter
		{
			std::string name, semantic;
			unsigned type, size, offset;
			unsigned char shared;
			std::vector<unsigned char> annotations;
		};

		struct Effect
		{
			unsigned version;
			std::string name;
			std::vector<Technique> techniques;
			std::vector<Parameter> parameters;
			std::vector<unsigned> data;
			std::vector<unsigned char> inputs;	// VtxInputDeclaration, little-endian
			std::vector<unsigned char> structs;	// Structs, little-endian
		};

		// Big-endian reader of the PS3 graph; annotation lists and the other parts the
		// converter does not change are re-emitted little-endian as they are read.
		class C_Reader
		{
		public:
			C_Reader(const unsigned char* p, size_t n) : m_p(p), m_n(n), m_at(0), m_ok(true) {}

			bool Ok() const { return m_ok; }
			bool AtEnd() const { return m_at == m_n; }
			size_t At() const { return m_at; }

			const unsigned char* Take(size_t n)
			{
				if (!m_ok || m_at + n > m_n)
				{
					m_ok = false;
					return NULL;
				}
				const unsigned char* p = m_p + m_at;
				m_at += n;
				return p;
			}

			unsigned Word()
			{
				const unsigned char* p = Take(4);
				return p ? BE32(p) : 0;
			}

			std::string String()
			{
				const unsigned n = Word();
				const unsigned char* p = Take(n);
				return p ? std::string((const char*)p, n) : std::string();
			}

			// Annotation: name, type, value raw (1/4/4/4/16 bytes by type, as the PC-hosted
			// exporter wrote them) or a string.
			void Annotation(std::vector<unsigned char>& o)
			{
				PutString(o, String());
				const unsigned type = Word();
				PutLE32(o, type);
				size_t raw = 0;
				switch (type)
				{
				case 1: raw = 1; break;
				case 2: case 3: case 4: raw = 4; break;
				case 5: raw = 16; break;
				case 6: PutString(o, String()); break;
				default: break;
				}
				if (raw)
				{
					const unsigned char* p = Take(raw);
					if (p)
						o.insert(o.end(), p, p + raw);
				}
			}

			void Annotations(std::vector<unsigned char>& o)
			{
				const unsigned n = Word();
				PutLE32(o, n);
				for (unsigned i = 0; m_ok && i < n; ++i)
					Annotation(o);
			}

			void Func(Function& f)
			{
				f.size = Word();
				const unsigned n = Word();
				for (unsigned i = 0; m_ok && i < n; ++i)
				{
					Binding b;
					b.param = Word(); b.handle = Word(); b.type = Word(); b.source = Word(); b.offset = Word(); b.count = Word();
					b.name = String();
					f.bindings.push_back(b);
				}
				f.code = f.size ? Take(f.size) : NULL;
			}

			bool Read(Effect& e)
			{
				e.version = Word();
				e.name = String();
				const unsigned techniques = Word();
				for (unsigned t = 0; m_ok && t < techniques; ++t)
				{
					Technique tech;
					tech.name = String();
					const unsigned passes = Word();
					for (unsigned p = 0; m_ok && p < passes; ++p)
					{
						Pass pass;
						pass.name = String();
						for (unsigned i = 0; i < kRenderStates; ++i)
							pass.states[i] = Word();
						Func(pass.vs);
						Func(pass.ps);
						Annotations(pass.annotations);
						tech.passes.push_back(pass);
					}
					Annotations(tech.annotations);
					e.techniques.push_back(tech);
				}
				const unsigned parameters = Word();
				for (unsigned i = 0; m_ok && i < parameters; ++i)
				{
					Parameter p;
					p.name = String();
					p.semantic = String();
					p.type = Word(); p.size = Word(); p.offset = Word();
					const unsigned char* s = Take(1);
					p.shared = s ? *s : 0;
					Annotations(p.annotations);
					e.parameters.push_back(p);
				}
				const unsigned words = Word();
				for (unsigned i = 0; m_ok && i < words; ++i)
					e.data.push_back(Word());
				const unsigned inputs = Word();
				PutLE32(e.inputs, inputs);
				for (unsigned i = 0; m_ok && i < inputs; ++i)
				{
					PutLE32(e.inputs, Word());	// usage (already the PC numbering)
					PutLE32(e.inputs, Word());	// index
					Annotations(e.inputs);
				}
				const unsigned structs = Word();
				PutLE32(e.structs, structs);
				for (unsigned i = 0; m_ok && i < structs; ++i)
				{
					PutString(e.structs, String());
					const unsigned members = Word();
					PutLE32(e.structs, members);
					for (unsigned m = 0; m_ok && m < members; ++m)
					{
						PutString(e.structs, String());
						PutLE32(e.structs, Word());
						Annotations(e.structs);
					}
				}
				return m_ok;
			}

		private:
			const unsigned char* m_p;
			size_t m_n, m_at;
			bool m_ok;
		};

		void WriteFunction(std::vector<unsigned char>& o, const Function& f, const std::vector<unsigned char>& code)
		{
			PutLE32(o, (unsigned)code.size());
			PutLE32(o, (unsigned)f.bindings.size());
			for (size_t i = 0; i < f.bindings.size(); ++i)
			{
				const Binding& b = f.bindings[i];
				PutLE32(o, b.param); PutLE32(o, b.handle); PutLE32(o, b.type);
				PutLE32(o, b.source); PutLE32(o, b.offset); PutLE32(o, b.count);
				PutString(o, b.name);
			}
			o.insert(o.end(), code.begin(), code.end());
		}

		unsigned MapColorWriteEnable(unsigned gcm)
		{
			unsigned d3d = 0;
			if (gcm & 0x00010000) d3d |= 1;
			if (gcm & 0x00000100) d3d |= 2;
			if (gcm & 0x00000001) d3d |= 4;
			if (gcm & 0x01000000) d3d |= 8;
			return d3d;
		}

		// First run of 'count' free registers below 'limit'; -1 when there is none.
		int Allocate(std::vector<bool>& used, unsigned count, unsigned limit)
		{
			for (unsigned base = 0; base + count <= limit; ++base)
			{
				bool free = true;
				for (unsigned k = 0; k < count && free; ++k)
					free = !used[base + k];
				if (free)
				{
					for (unsigned k = 0; k < count; ++k)
						used[base + k] = true;
					return (int)base;
				}
			}
			return -1;
		}

		bool RangeFree(const std::vector<bool>& used, unsigned base, unsigned count, unsigned limit)
		{
			if (base + count > limit)
				return false;
			for (unsigned k = 0; k < count; ++k)
				if (used[base + k])
					return false;
			return true;
		}

		class C_Ps3EffectConverter
		{
		public:
			C_Ps3EffectConverter(Effect& e, EffectStats& stats, EffectReport* pReport)
				: m_e(e), m_stats(stats), m_pReport(pReport) {}

			bool Fail(std::string& error, const std::string& text)
			{
				error = text;
				return false;
			}

			// Registers a vertex program reads directly (not through a binding): kept free
			// when moving bindings.
			static void DirectReads(const Program& prog, std::vector<bool>& used)
			{
				for (unsigned pc = 0; pc < prog.ucodeSize / 16; ++pc)
				{
					const unsigned char* p = prog.ucode + 16 * pc;
					const unsigned d1 = BE32(p + 4), d2 = BE32(p + 8), d3 = BE32(p + 12);
					// Source types (bits 0-1 of each 17-bit source; 3 = constant), see RsxTranslate.h.
					const unsigned src0 = (d2 >> 23) & 3, src1 = (d2 >> 6) & 3, src2 = (d3 >> 21) & 3;
					const unsigned reg = (d1 >> 12) & 0x3ff;
					if ((src0 == 3 || src1 == 3 || src2 == 3) && !((d3 >> 1) & 1) && reg < kVsRegisters)
						used[reg] = true;
					if ((d3 & 1) != 0)
						break;
				}
			}

			bool VertexBindings(Function& f, const Program& prog, VertexConstants& vc, std::string& error)
			{
				for (size_t i = 0; i < prog.literalRegisters.size(); ++i)
				{
					const unsigned reg = prog.literalRegisters[i];
					if (reg >= (unsigned)VertexConstants::kRegisters)
						return Fail(error, "literal register out of range");
					vc.literal[reg] = true;
					memcpy(vc.value[reg], &prog.literalValues[4 * i], 16);
				}
				std::vector<Binding> in = f.bindings, out;
				std::vector<bool> used(kVsRegisters, false);
				std::vector<int> placed(in.size(), -1);
				for (size_t i = 0; i < in.size(); ++i)
				{
					const Binding& b = in[i];
					if (b.type != kBindFloat)
						return Fail(error, "vertex binding type " + std::to_string(b.type));
					if (b.handle == kInvalidHandle)
						continue;
					if (RangeFree(used, b.handle, b.count, kVsRegisters))
					{
						for (unsigned k = 0; k < b.count; ++k)
							used[b.handle + k] = true;
						placed[i] = (int)b.handle;
					}
				}
				// Registers read without a binding stay out of the moved ranges.
				std::vector<bool> reserved = used;
				DirectReads(prog, reserved);
				for (size_t i = 0; i < in.size(); ++i)
				{
					const Binding& b = in[i];
					if (b.handle == kInvalidHandle || placed[i] >= 0)
						continue;
					placed[i] = Allocate(reserved, b.count, kVsRegisters);
					if (placed[i] < 0)
						return Fail(error, "no free vertex constant registers for a binding of " + std::to_string(b.count));
					if (m_pReport)
						++m_pReport->vsBindingsMoved;
				}
				for (size_t i = 0; i < in.size(); ++i)
				{
					const Binding& b = in[i];
					if (placed[i] < 0)
						continue;
					for (unsigned k = 0; k < b.count; ++k)
						if (b.handle + k < (unsigned)VertexConstants::kRegisters)
						{
							vc.d3dRegister[b.handle + k] = placed[i] + (int)k;
							vc.rangeEnd[b.handle + k] = placed[i] + (int)b.count;
						}
					Binding nb = b;
					nb.handle = (unsigned)placed[i];
					nb.source = 0;
					nb.offset = 0;
					nb.name.clear();
					out.push_back(nb);
				}
				f.bindings = out;
				return true;
			}

			bool FragmentBindings(Function& f, const Program& prog, FragmentConstants& fc, FragmentSamplers& fs, std::string& error)
			{
				std::vector<Binding> out;
				// Samplers keep their unit.
				for (size_t i = 0; i < f.bindings.size(); ++i)
				{
					const Binding& b = f.bindings[i];
					if (b.type == kBindFloat)
						continue;
					if (b.type != kBindSampler)
						return Fail(error, "fragment binding type " + std::to_string(b.type));
					if (b.handle >= 16)
						return Fail(error, "sampler binding without a texture unit");
					if (b.param >= m_e.parameters.size())
						return Fail(error, "binding parameter index out of range");
					const Parameter& p = m_e.parameters[b.param];
					if (p.type != kParamSampler || p.offset / 4 + 1 >= m_e.data.size())
						return Fail(error, "sampler binding to a non-sampler parameter");
					const unsigned state = m_e.data[p.offset / 4 + 1];
					const bool cube = ((state >> 6) & 7) != 0;
					fs.bound[b.handle] = true;
					fs.dim[b.handle] = cube ? E_SamplerDim_Cube : E_SamplerDim_2D;
					if (m_pReport)
						++(cube ? m_pReport->cubeSamplers : m_pReport->samplers2D);
					Binding nb = b;
					nb.source = 0;
					nb.offset = 0;
					nb.count = 1;
					nb.name.clear();
					out.push_back(nb);
				}

				// Float bindings grouped by parameter.
				std::vector<unsigned> params;
				for (size_t i = 0; i < f.bindings.size(); ++i)
					if (f.bindings[i].type == kBindFloat)
					{
						bool seen = false;
						for (size_t k = 0; k < params.size(); ++k)
							seen |= params[k] == f.bindings[i].param;
						if (!seen)
							params.push_back(f.bindings[i].param);
					}
				std::vector<bool> used(kPsRegisters, false);
				std::vector<int> base(params.size(), -1);
				std::vector<unsigned> span(params.size(), 0);
				for (size_t pi = 0; pi < params.size(); ++pi)
				{
					bool listed = true;
					int b0 = -1;
					for (size_t i = 0; i < f.bindings.size(); ++i)
					{
						const Binding& b = f.bindings[i];
						if (b.type != kBindFloat || b.param != params[pi])
							continue;
						if (b.offset % 4)
							return Fail(error, "fragment binding offset not a whole register");
						if (b.offset / 4 + b.count > span[pi])
							span[pi] = b.offset / 4 + b.count;
						if (b.handle == kInvalidHandle || b.handle < b.offset / 4)
						{
							listed = false;
							continue;
						}
						const int thisBase = (int)(b.handle - b.offset / 4);
						if (b0 >= 0 && b0 != thisBase)
							listed = false;
						b0 = thisBase;
					}
					if (listed && b0 >= 0 && RangeFree(used, (unsigned)b0, span[pi], kPsRegisters))
					{
						for (unsigned k = 0; k < span[pi]; ++k)
							used[b0 + k] = true;
						base[pi] = b0;
					}
				}
				for (size_t pi = 0; pi < params.size(); ++pi)
				{
					if (base[pi] >= 0)
						continue;
					base[pi] = Allocate(used, span[pi], kPsRegisters);
					if (base[pi] < 0)
						return Fail(error, "no free fragment constant registers");
					if (m_pReport)
						++m_pReport->psBindingsAllocated;
				}

				// Embedded copies of each bound member -> its register.
				for (size_t mi = 0; mi < prog.map.size(); ++mi)
				{
					const MapEntry& m = prog.map[mi];
					if (m.resource < 0 || !(m.resource & 0x400))
						continue;
					const unsigned t = m.resource & 0x3ff;
					if (t + 1 >= prog.table.size() || t + 2 + prog.table[t + 1] > prog.table.size())
						return Fail(error, "fragment offset table index out of range");
					// Binding for this member: the exact name, or element k of an array bound as "name[0]".
					int reg = -1;
					for (size_t i = 0; i < f.bindings.size() && reg < 0; ++i)
					{
						const Binding& b = f.bindings[i];
						if (b.type != kBindFloat)
							continue;
						const std::string bn = b.Name();
						unsigned element = 0;
						bool match = bn == m.fullName;
						if (!match)
						{
							const size_t br = m.fullName.rfind('[');
							if (br != std::string::npos && bn.size() > 3 && bn.compare(bn.size() - 3, 3, "[0]") == 0 &&
								bn.compare(0, bn.size() - 3, m.fullName, 0, br) == 0 && bn.size() - 3 == br)
							{
								element = (unsigned)atoi(m.fullName.c_str() + br + 1);
								match = element < b.count;
							}
						}
						if (!match)
							continue;
						for (size_t pi = 0; pi < params.size(); ++pi)
							if (params[pi] == b.param)
								reg = base[pi] + (int)(b.offset / 4 + element);
					}
					if (reg < 0)
					{
						if (m_pReport)
							++m_pReport->psMapEntriesUnbound;
						continue;
					}
					const unsigned n = prog.table[t + 1];
					for (unsigned k = 0; k < n; ++k)
					{
						fc.offsets.push_back(prog.table[t + 2 + k]);
						fc.registers.push_back((unsigned)reg);
					}
				}

				for (size_t pi = 0; pi < params.size(); ++pi)
				{
					Binding nb;
					nb.param = params[pi];
					nb.handle = (unsigned)base[pi];
					nb.type = kBindFloat;
					nb.source = 0;
					nb.offset = 0;
					nb.count = span[pi];
					out.push_back(nb);
				}
				f.bindings = out;
				return true;
			}

			bool Compile(const std::string& hlsl, bool pixel, std::vector<unsigned char>& code, std::string& error)
			{
				T_D3DCompile compile = D3DCompileFunction();
				if (!compile)
					return Fail(error, "d3dcompiler_47.dll not available");
				ID3DBlob* pCode = NULL;
				ID3DBlob* pErrors = NULL;
				const HRESULT hr = compile(hlsl.data(), hlsl.size(), "rsx", NULL, NULL, "main",
					pixel ? "ps_3_0" : "vs_3_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pCode, &pErrors);
				if (FAILED(hr) || !pCode)
				{
					++m_stats.failed;
					const std::string messages = pErrors ? std::string((const char*)pErrors->GetBufferPointer(), pErrors->GetBufferSize()) : std::string("no messages");
					if (m_stats.firstFailure.empty())
					{
						m_stats.firstFailure = messages;
						m_stats.firstFailureHlsl = hlsl;
					}
					if (pErrors) pErrors->Release();
					if (pCode) pCode->Release();
					return Fail(error, "fxc: " + messages.substr(0, 300));
				}
				if (pErrors)
					pErrors->Release();
				const unsigned char* p = (const unsigned char*)pCode->GetBufferPointer();
				code.assign(p, p + pCode->GetBufferSize());
				pCode->Release();
				// fxc accepts literals (def) past the shader model 3 constant file; the
				// runtime does not create such a shader. Check every def register.
				const unsigned limit = pixel ? kPsRegisters : kVsRegisters;
				const unsigned* tok = (const unsigned*)&code[0];
				const size_t count = code.size() / 4;
				for (size_t i = 1; i < count;)
				{
					const unsigned t = tok[i];
					if (t == 0x0000ffff)
						break;
					if ((t & 0xffff) == 0xfffe)			// comment: length in bits 16-30
					{
						i += 1 + ((t >> 16) & 0x7fff);
						continue;
					}
					const unsigned length = (t >> 24) & 0xf;	// shader model 2+: tokens after the opcode
					if ((t & 0xffff) == 0x51 && i + 1 < count)	// D3DSIO_DEF
					{
						const unsigned dst = tok[i + 1];
						const unsigned type = ((dst >> 28) & 7) | ((dst >> 8) & 0x18);
						if (type == 2 && (dst & 0x7ff) >= limit)	// D3DSPR_CONST
						{
							++m_stats.failed;
							return Fail(error, "fxc placed a literal at c" + std::to_string(dst & 0x7ff));
						}
					}
					i += 1 + length;
				}
				++m_stats.compiled;
				return true;
			}

			bool Program1(Function& f, bool pixel, const std::string& where, std::vector<unsigned char>& code, std::string& error)
			{
				code.clear();
				if (!f.size)
				{
					f.bindings.clear();
					return true;
				}
				Program prog;
				if (!ParseCgb(f.code, f.size, prog, error))
					return Fail(error, where + ": " + error);
				if (prog.fragment != pixel)
					return Fail(error, where + ": program kind does not match its slot");
				++m_stats.programs;
				std::string hlsl, notes;
				Counters counters;
				bool ok;
				if (pixel)
				{
					FragmentConstants fc;
					FragmentSamplers fs;
					if (!FragmentBindings(f, prog, fc, fs, error))
						return Fail(error, where + ": " + error);
					ok = TranslateFragment(prog, fc, fs, hlsl, error, notes, counters);
				}
				else
				{
					VertexConstants* vc = new VertexConstants;
					ok = VertexBindings(f, prog, *vc, error) && TranslateVertex(prog, *vc, hlsl, error, notes, counters);
					delete vc;
				}
				if (!ok)
				{
					++m_stats.failed;
					if (m_stats.firstFailure.empty())
						m_stats.firstFailure = where + ": " + error;
					return Fail(error, where + ": " + error);
				}
				if (!notes.empty())
				{
					++m_stats.unsupported;
					if (m_stats.firstUnsupported.empty())
						m_stats.firstUnsupported = where + ": " + notes;
				}
				if (m_pReport)
				{
					m_pReport->programNames.push_back(where);
					m_pReport->hlsl.push_back(hlsl);
					m_pReport->notes.push_back(notes);
					m_pReport->counters.Add(counters);
				}
				if (!Compile(hlsl, pixel, code, error))
					return Fail(error, where + ": " + error);
				return true;
			}

			bool Convert(std::vector<unsigned char>& o, std::string& error)
			{
				PutLE32(o, m_e.version);
				PutString(o, m_e.name);
				PutLE32(o, (unsigned)m_e.techniques.size());
				for (size_t t = 0; t < m_e.techniques.size(); ++t)
				{
					Technique& tech = m_e.techniques[t];
					PutString(o, tech.name);
					PutLE32(o, (unsigned)tech.passes.size());
					for (size_t p = 0; p < tech.passes.size(); ++p)
					{
						Pass& pass = tech.passes[p];
						PutString(o, pass.name);
						for (unsigned i = 0; i < kRenderStates; ++i)
							PutLE32(o, i == kColorWriteEnable ? MapColorWriteEnable(pass.states[i]) : pass.states[i]);
						const std::string where = tech.name + "/" + std::to_string((unsigned long long)p);
						std::vector<unsigned char> vsCode, psCode;
						if (!Program1(pass.vs, false, where + "/vs", vsCode, error) ||
							!Program1(pass.ps, true, where + "/ps", psCode, error))
							return false;
						WriteFunction(o, pass.vs, vsCode);
						WriteFunction(o, pass.ps, psCode);
						o.insert(o.end(), pass.annotations.begin(), pass.annotations.end());
					}
					o.insert(o.end(), tech.annotations.begin(), tech.annotations.end());
				}
				PutLE32(o, (unsigned)m_e.parameters.size());
				for (size_t i = 0; i < m_e.parameters.size(); ++i)
				{
					const Parameter& p = m_e.parameters[i];
					PutString(o, p.name);
					PutString(o, p.semantic);
					PutLE32(o, p.type); PutLE32(o, p.size); PutLE32(o, p.offset);
					o.push_back(p.shared);
					o.insert(o.end(), p.annotations.begin(), p.annotations.end());
				}
				PutLE32(o, (unsigned)m_e.data.size());
				for (size_t i = 0; i < m_e.data.size(); ++i)
					PutLE32(o, m_e.data[i]);
				o.insert(o.end(), m_e.inputs.begin(), m_e.inputs.end());
				o.insert(o.end(), m_e.structs.begin(), m_e.structs.end());
				return true;
			}

		private:
			Effect& m_e;
			EffectStats& m_stats;
			EffectReport* m_pReport;
		};
	}

	E_Result ConvertPs3Effect(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out,
		std::string& error, EffectStats* pStats, EffectReport* pReport)
	{
		EffectStats localStats;
		EffectStats& stats = pStats ? *pStats : localStats;

		if (inSize < 4 || memcmp(in, "FXB2", 4) != 0)
		{
			error = "not a big-endian FXB2 file";
			return E_Result_Failed;
		}
		Block root;
		std::vector<Block> kids;
		if (!ReadBlockBE(in + 4, inSize - 4, root) || !ChildrenBE(root, kids))
		{
			error = "bad FXB block tree";
			return E_Result_Failed;
		}
		const Block* pPs3 = NULL;
		for (size_t i = 0; i < kids.size(); ++i)
			if (kids[i].id == "PS3FX")
				pPs3 = &kids[i];
		if (!pPs3)
		{
			error = "no PS3FX block";
			return E_Result_Failed;
		}

		Effect effect;
		C_Reader reader(pPs3->payload, pPs3->PayloadSize());
		if (!reader.Read(effect) || !reader.AtEnd())
		{
			error = "PS3FX effect does not parse to the end of its block";
			return E_Result_Failed;
		}
		if (effect.version != kEffectVersion)
		{
			error = "effect version " + std::to_string((unsigned long long)effect.version);
			return E_Result_Failed;
		}

		out.clear();
		out.reserve(inSize * 2);
		PutLE32(out, ('F' << 24) | ('X' << 16) | ('B' << 8) | '2');	// the PC exporter's int 'FXB2'
		const size_t rootAt = BeginBlockLE(out, root, 1);
		const size_t pcfxAt = out.size();
		out.insert(out.end(), (const unsigned char*)"PCFX\0\0\0\0", (const unsigned char*)"PCFX\0\0\0\0" + 8);
		PutLE32(out, 0);
		PutLE16(out, pPs3->count);
		out.push_back(pPs3->header[14]);
		out.push_back(pPs3->header[15]);

		C_Ps3EffectConverter converter(effect, stats, pReport);
		if (!converter.Convert(out, error))
			return E_Result_Failed;
		EndBlockLE(out, pcfxAt);
		EndBlockLE(out, rootAt);
		return E_Result_Converted;
	}
}
