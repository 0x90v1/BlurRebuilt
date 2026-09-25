// [Blur1Multi] RSX (PS3) vertex and fragment programs -> Direct3D 9 shader model 3 HLSL (T076).
//
// PS3 effects (.fxb, PS3FX block) carry each program as a CGB blob, the output of
// `sce-cgc -mcgb` (fx/source/targets/ps3fx.cpp CompileShader). This unit parses the
// blob and translates the RSX microcode into HLSL text for D3DCompile (vs_3_0 /
// ps_3_0). Ps3EffectConvert.cpp rebuilds the effect around it.
//
// Written from the documented NV40/RSX instruction layouts. References, used for
// bit positions, opcode numbers and hardware behaviour notes only (no code taken;
// RPCS3 is GPL-2.0):
//  - RPCS3 rpcs3/Emu/RSX/Program: RSXVertexProgram.h, RSXFragmentProgram.h,
//    Assembler/FPOpcodes.h, Assembler/FPToCFG.cpp (half-word order of FP words),
//    FragmentProgramDecompiler.cpp / VertexProgramDecompiler.cpp (semantics:
//    DIVSQ, RSQ, input clamping, condition codes, texture-coordinate 2D mask).
//  - Mesa src/gallium/drivers/nouveau/nv30: nvfx_shader.h (FP field list),
//    nv40_vertprog.h (VP output register numbers).
//  - NV_vertex_program / NV_vertex_program2 / NV_fragment_program specifications.
//
// CGB container (no SDK header in the tree; layout measured on all 7,574 program
// instances of the retail PS3 shaders-speed pak, every one parses to its last
// byte; big-endian throughout):
//   0x00 "CGB\0"            0x04 U32 0 (meaning unknown)
//   0x08 U16 ucode size     0x0a U8 0 = vertex, 1 = fragment   0x0b U8 3 (content flags; ps3fx.cpp:168 asserts CELL_CGB_CONSTANT_TABLE)
//   0x0c 16-byte program configuration:
//        VP: U16 attribute input mask, U8 temp count (unconfirmed), U8 0, U32 attribute output mask, 8 x 0
//        FP: U32 attribute input mask, U16 texCoordsInputMask, U16 texCoords2D, U16 texCoordsCentroid, U16 0, U32 fragmentControl
//   0x1c U32: VP 0; FP register count in the top byte (unconfirmed)
//   0x20 microcode, then (16-aligned) the constant table: U16 block size, U16 n,
//        U16 register[n], pad to 16, float4 value[n] (VP literals uploaded per draw;
//        empty in every FP). Block size = align16(2n) + 16n + 16 (ps3fx.cpp:229-233).
//   Parameter map: U16 byte size, U16 entry count, U16 N (table length in U16s),
//        entries {U16 type (0 in all), U16 name offset, S16 parent entry, S16 resource},
//        U16 table[N], NUL-first string table.
//        resource: VP uniform = constant register; VP input = attribute index (parent =
//        the input struct); FP sampler = texture unit; FP uniform = 0x400 | i with
//        table[i] = register (0xFFFF = none), table[i+1] = n, table[i+2..] = byte
//        offsets in the ucode of the n embedded copies (cellCgbMapGetFragmentUniformOffsets).
//
// Vertex program: 4 big-endian words per instruction, vector and scalar op dual-issued.
//   D0: addr_swz 0-1, cond swizzle w 2-3 z 4-5 y 6-7 x 8-9, cond 10-12 (lt 1, eq 2, gt 4),
//       cond_test 13, cond_update0 14, vec dst temp 15-20 (0x3f none), src0/1/2 abs 21-23,
//       addr_reg_sel 24, cond_reg_sel 25, saturate 26, index_input 27, cond_update1 29,
//       vec_result 30; branch address high bit 23
//   D1: src0 high 0-7, input 8-11, const 12-21, vec opcode 22-26, sca opcode 27-31
//   D2: src2 high 0-5 (branch address 0-5), src1 6-22, src0 low 23-31
//   D3: end 0, index_const 1, output 2-6 (0 HPOS, 1 COL0, 2 COL1, 3/4 BFC0/1, 5 FOGC,
//       6 PSZ, 7+n TEXn, 0x1f none), sca dst temp 7-12, vec mask w 13 z 14 y 15 x 16,
//       sca mask w 17 z 18 y 19 x 20, src2 low 21-31 (branch address low 29-31)
//   source (17 bits): type 0-1 (1 temp, 2 input, 3 const), temp 2-7, swizzle w 8-9 z 10-11
//       y 12-13 x 14-15, negate 16
//   ADD reads src0 and src2; scalar ops read src2.
// Fragment program: 16-byte instructions, each 32-bit word stored as two big-endian
//   halves, low half first. A source of type 2 means a float4 literal (same encoding)
//   follows the instruction in the next 16 bytes.
//   dst: end 0, reg 1-6, fp16 7, set_cond 8, mask 9-12, input attr 13-16 (WPOS, COL0, COL1,
//        FOGC, TEX0-9), texture unit 17-20, exp_tex 21, prec 22-23, opcode 24-29, no_dest 30, sat 31
//   src0: type 0-1 (0 temp, 1 input, 2 literal), reg 2-7, fp16 8, swizzle x 9-10 y 11-12 z 13-14
//        w 15-16, neg 17, exec if lt 18 eq 19 gt 20, cond swizzle 21-28, abs 29,
//        cond_mod_reg 30, cond_reg 31
//   src1: common 0-17, abs 18, precision of src0 19-21 / src1 22-24 / src2 25-27, scale 28-30,
//        opcode bit 6 at 31
//   src2: common 0-17, abs 18, addr 19-29, use index 30, perspective flag 31
//   H registers alias R storage: H(2n) = R(n).xy, H(2n+1) = R(n).zw (16-bit halves). The
//   translator keeps them as separate float4 variables and proves, per component read, that
//   no read crosses from one view to the other (a crossing is reported and fails).

#pragma once

#include <x360convert/X360Convert.h>

#include <string.h>
#include <string>
#include <vector>

namespace RsxTranslate
{
	struct MapEntry
	{
		unsigned type;
		std::string name;		// as stored
		std::string fullName;	// "parent.name" for struct members
		int parent;
		int resource;
	};

	struct Program
	{
		bool fragment;
		const unsigned char* ucode;
		unsigned ucodeSize;
		unsigned char config[16];
		unsigned word1c;
		std::vector<unsigned> literalRegisters;		// VP constant table
		std::vector<float> literalValues;			// 4 per register
		std::vector<MapEntry> map;
		std::vector<unsigned> table;

		// VP configuration
		unsigned VpInputMask() const { return (config[0] << 8) | config[1]; }
		unsigned VpOutputMask() const { return ((unsigned)config[4] << 24) | (config[5] << 16) | (config[6] << 8) | config[7]; }
		// FP configuration
		unsigned FpTexCoordsInputMask() const { return (config[4] << 8) | config[5]; }
		unsigned FpTexCoords2D() const { return (config[6] << 8) | config[7]; }
		unsigned FpTexCoordsCentroid() const { return (config[8] << 8) | config[9]; }
		unsigned FpControl() const { return ((unsigned)config[12] << 24) | (config[13] << 16) | (config[14] << 8) | config[15]; }
	};

	bool ParseCgb(const unsigned char* p, size_t n, Program& prog, std::string& error);

	// Vertex constant placement, per RSX constant register (0-467).
	struct VertexConstants
	{
		enum { kRegisters = 468 };
		int d3dRegister[kRegisters];	// -1: no binding covers it
		int rangeEnd[kRegisters];		// one past the last D3D register of the binding that covers it
		bool literal[kRegisters];		// from the CGB constant table
		float value[kRegisters][4];
		VertexConstants();
	};

	// Fragment constant placement: byte offset of an embedded literal in the ucode ->
	// the D3D register that replaces it (literals not listed stay literals).
	struct FragmentConstants
	{
		std::vector<unsigned> offsets;
		std::vector<unsigned> registers;
		int Find(unsigned offset) const;
	};

	enum E_SamplerDim { E_SamplerDim_2D, E_SamplerDim_Cube };

	struct FragmentSamplers
	{
		bool bound[16];
		E_SamplerDim dim[16];
		FragmentSamplers();
	};

	// Counters over translated programs (for the verify tool).
	struct Counters
	{
		unsigned vpInstructions, fpInstructions, fpLiterals, fpPatchedLiterals;
		unsigned unboundConstantReads, perspectiveReads, texcoord2DzwReads, cubeFetches, unboundSamplerFetches;
		unsigned branches, conditionalWrites, kills, mrtOutputs, mrtSkipped;
		Counters() { memsetZero(); }
		void memsetZero();
		void Add(const Counters& o);
	};

	// Name of the extra interpolator every translated vertex program writes: the
	// clip-space w of the position, which the fragment programs need for the
	// perspective flag and for 2D-masked texture coordinates.
	const unsigned kWInterpolator = 9;	// TEXCOORD9 (RSX has TEX0-TEX8 as program outputs)

	// Each returns false with 'error' set when the program uses something the
	// translator does not handle; 'notes' lists approximations taken (empty when exact).
	bool TranslateVertex(const Program& prog, const VertexConstants& constants, std::string& hlsl,
		std::string& error, std::string& notes, Counters& counters);
	bool TranslateFragment(const Program& prog, const FragmentConstants& constants, const FragmentSamplers& samplers,
		std::string& hlsl, std::string& error, std::string& notes, Counters& counters);

	// Ps3EffectConvert.cpp with the translated HLSL of every program kept (verify tool).
	struct EffectReport
	{
		std::vector<std::string> programNames;	// "technique/pass/vs|ps"
		std::vector<std::string> hlsl;
		std::vector<std::string> notes;
		Counters counters;
		unsigned cubeSamplers, samplers2D;
		unsigned vsBindingsMoved, psBindingsAllocated, psMapEntriesUnbound;
		EffectReport() : cubeSamplers(0), samplers2D(0), vsBindingsMoved(0), psBindingsAllocated(0), psMapEntriesUnbound(0) {}
	};

	// X360Convert::ConvertPs3Effect, with the report filled when 'pReport' is set.
	X360Convert::E_Result ConvertPs3Effect(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out,
		std::string& error, X360Convert::EffectStats* pStats, EffectReport* pReport);
}
