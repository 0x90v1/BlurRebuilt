// [Blur1Multi] RSX vertex / fragment program -> HLSL shader model 3 (T076). Layouts,
// references and the CGB container are described in RsxTranslate.h.
//
// Semantics that are not plain arithmetic, with their source:
//  - FP DIVSQ: a / sqrt(|b.x|), and 0 where a is 0 whatever b is (RPCS3 hardware note,
//    FragmentProgramDecompiler.cpp "DIVSQ is not compliant").
//  - FP RSQ: 1 / sqrt(|x|) ("RSQ ignores the sign of the inputs", same file); VP RSQ
//    likewise (NV_vertex_program: RSQ takes the absolute value).
//  - FP inputs COL0/COL1 are clamped to [0, 1]; a texture coordinate flagged in the
//    texCoords2D mask reads z = 0 and w = the position w (RPCS3 hardware tests).
//  - FP source flag src2 bit 31 ("disable perspective-correct interpolation", Mesa
//    nvfx_shader.h; RPCS3 multiplies by gl_FragCoord.w = 1/w): the input reads as
//    value / w. Every translated vertex program therefore also writes its position w
//    to TEXCOORD9 (kWInterpolator).
//  - FP condition codes: two registers; a write happens per component where the test
//    of the swizzled CC against 0 passes; set_cond copies the written value to CC.
//    KIL discards if any component passes.
//  - FP source precision modifiers: 2 clamp [-2, 2], 3 clamp [-1, 1], 4 saturate
//    (applied before abs), 1 (half) and 5 (no effect in RPCS3's hardware tests) ignored.
//    Destination precision 1 (half) is ignored: half registers are translated as float.
//  - FP output: control bit 0x40 exports R0 (and R2, R3, R4 to render targets 1-3);
//    otherwise H0 (H4, H6, H8).
//  - FENCT / FENCB: no operation.
//  - VP: ADD is src0 + src2; scalar ops read component x of the swizzled src2; both
//    halves of a dual-issued instruction read their sources before either writes.
//    VP LG2 clamps its input to 1e-10 as RPCS3 does; outputs start at (0, 0, 0, 1).
//  - VP BRI: forward branch when the condition passes; translated as a skipped block.

#include "RsxTranslate.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

namespace RsxTranslate
{
	namespace
	{
		unsigned Bits(unsigned v, unsigned lo, unsigned n) { return (v >> lo) & ((n >= 32) ? 0xffffffffu : ((1u << n) - 1)); }
		unsigned Be16(const unsigned char* p) { return (p[0] << 8) | p[1]; }
		unsigned Be32(const unsigned char* p) { return ((unsigned)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }
		unsigned Align16(unsigned v) { return (v + 15) & ~15u; }

		const char kLane[] = "xyzw";

		std::string Num(unsigned v)
		{
			char b[16];
			sprintf_s(b, "%u", v);
			return b;
		}

		// HLSL float literal that reads back to the same value.
		bool Literal(float f, std::string& out)
		{
			if (!(f == f) || fabsf(f) > 3.402823466e+38f)
				return false;
			char b[48];
			sprintf_s(b, "%.9g", f);
			out = b;
			if (out.find_first_of(".en") == std::string::npos)
				out += ".0";
			return true;
		}

		bool Literal4(const float* v, std::string& out)
		{
			std::string a[4];
			for (int i = 0; i < 4; ++i)
				if (!Literal(v[i], a[i]))
					return false;
			out = "float4(" + a[0] + ", " + a[1] + ", " + a[2] + ", " + a[3] + ")";
			return true;
		}

		std::string MaskString(unsigned mask)
		{
			std::string s;
			for (int c = 0; c < 4; ++c)
				if (mask & (1 << c))
					s += kLane[c];
			return s;
		}

		// Comparison of a condition register against 0 (lt 1, eq 2, gt 4).
		const char* CompareOp(unsigned cond)
		{
			switch (cond)
			{
			case 1: return "<";
			case 2: return "==";
			case 3: return "<=";
			case 4: return ">";
			case 5: return "!=";
			case 6: return ">=";
			default: return NULL;
			}
		}
	}

	VertexConstants::VertexConstants()
	{
		for (int i = 0; i < kRegisters; ++i)
		{
			d3dRegister[i] = -1;
			rangeEnd[i] = -1;
			literal[i] = false;
			value[i][0] = value[i][1] = value[i][2] = value[i][3] = 0.0f;
		}
	}

	int FragmentConstants::Find(unsigned offset) const
	{
		for (size_t i = 0; i < offsets.size(); ++i)
			if (offsets[i] == offset)
				return (int)registers[i];
		return -1;
	}

	FragmentSamplers::FragmentSamplers()
	{
		for (int i = 0; i < 16; ++i)
		{
			bound[i] = false;
			dim[i] = E_SamplerDim_2D;
		}
	}

	void Counters::memsetZero()
	{
		vpInstructions = fpInstructions = fpLiterals = fpPatchedLiterals = 0;
		unboundConstantReads = perspectiveReads = texcoord2DzwReads = cubeFetches = unboundSamplerFetches = 0;
		branches = conditionalWrites = kills = mrtOutputs = mrtSkipped = 0;
	}

	void Counters::Add(const Counters& o)
	{
		vpInstructions += o.vpInstructions; fpInstructions += o.fpInstructions;
		fpLiterals += o.fpLiterals; fpPatchedLiterals += o.fpPatchedLiterals;
		unboundConstantReads += o.unboundConstantReads; perspectiveReads += o.perspectiveReads;
		texcoord2DzwReads += o.texcoord2DzwReads; cubeFetches += o.cubeFetches;
		unboundSamplerFetches += o.unboundSamplerFetches; branches += o.branches;
		conditionalWrites += o.conditionalWrites; kills += o.kills; mrtOutputs += o.mrtOutputs; mrtSkipped += o.mrtSkipped;
	}

	bool ParseCgb(const unsigned char* p, size_t n, Program& prog, std::string& error)
	{
		if (n < 0x20 || memcmp(p, "CGB\0", 4) != 0)
		{
			error = "not a CGB program";
			return false;
		}
		prog.ucodeSize = Be16(p + 8);
		if (p[10] > 1)
		{
			error = "CGB program kind " + Num(p[10]);
			return false;
		}
		prog.fragment = p[10] == 1;
		memcpy(prog.config, p + 12, 16);
		prog.word1c = Be32(p + 0x1c);
		prog.ucode = p + 0x20;
		size_t at = 0x20 + Align16(prog.ucodeSize);
		if (at + 4 > n)
		{
			error = "CGB constant table past the end";
			return false;
		}
		const unsigned count = Be16(p + at + 2);
		const size_t values = at + Align16(4 + 2 * count);
		if (values + 16 * count > n)
		{
			error = "CGB constant values past the end";
			return false;
		}
		prog.literalRegisters.clear();
		prog.literalValues.clear();
		for (unsigned i = 0; i < count; ++i)
		{
			prog.literalRegisters.push_back(Be16(p + at + 4 + 2 * i));
			for (int c = 0; c < 4; ++c)
			{
				const unsigned bits = Be32(p + values + 16 * i + 4 * c);
				float f;
				memcpy(&f, &bits, 4);
				prog.literalValues.push_back(f);
			}
		}
		at = values + 16 * count;
		if (at + 6 > n)
		{
			error = "CGB parameter map past the end";
			return false;
		}
		const unsigned mapSize = Be16(p + at);
		const unsigned entries = Be16(p + at + 2);
		const unsigned tableLength = Be16(p + at + 4);
		const size_t tableAt = at + 6 + 8 * entries;
		const size_t stringsAt = tableAt + 2 * tableLength;
		if (at + mapSize != n || stringsAt > n)
		{
			error = "CGB parameter map size does not reach the end of the program";
			return false;
		}
		prog.table.clear();
		for (unsigned i = 0; i < tableLength; ++i)
			prog.table.push_back(Be16(p + tableAt + 2 * i));
		prog.map.clear();
		for (unsigned i = 0; i < entries; ++i)
		{
			const unsigned char* e = p + at + 6 + 8 * i;
			MapEntry m;
			m.type = Be16(e);
			const size_t nameAt = stringsAt + Be16(e + 2);
			if (nameAt >= n)
			{
				error = "CGB parameter name past the end";
				return false;
			}
			const unsigned char* end = (const unsigned char*)memchr(p + nameAt, 0, n - nameAt);
			if (!end)
			{
				error = "CGB parameter name not terminated";
				return false;
			}
			m.name.assign((const char*)p + nameAt, (const char*)end);
			m.parent = (short)Be16(e + 4);
			m.resource = (short)Be16(e + 6);
			m.fullName = m.name;
			prog.map.push_back(m);
		}
		for (size_t i = 0; i < prog.map.size(); ++i)
		{
			const int parent = prog.map[i].parent;
			if (parent >= 0 && parent < (int)prog.map.size())
				prog.map[i].fullName = prog.map[parent].fullName + "." + prog.map[i].name;
		}
		return true;
	}

	// ---------------------------------------------------------------------------------
	// Fragment programs

	namespace
	{
		enum
		{
			FP_NOP = 0x00, FP_MOV, FP_MUL, FP_ADD, FP_MAD, FP_DP3, FP_DP4, FP_DST, FP_MIN, FP_MAX, FP_SLT, FP_SGE,
			FP_SLE, FP_SGT, FP_SNE, FP_SEQ, FP_FRC, FP_FLR, FP_KIL, FP_PK4, FP_UP4, FP_DDX, FP_DDY, FP_TEX, FP_TXP,
			FP_TXD, FP_RCP, FP_RSQ, FP_EX2, FP_LG2, FP_LIT, FP_LRP, FP_STR, FP_SFL, FP_COS, FP_SIN, FP_PK2, FP_UP2,
			FP_POW, FP_PKB, FP_UPB, FP_PK16, FP_UP16, FP_BEM, FP_PKG, FP_UPG, FP_DP2A, FP_TXL, FP_TXB = 0x31,
			FP_TEXBEM = 0x33, FP_TXPBEM, FP_BEMLUM, FP_REFL, FP_TIMESWTEX, FP_DP2, FP_NRM, FP_DIV, FP_DIVSQ, FP_LIF,
			FP_FENCT, FP_FENCB, FP_BRK = 0x40, FP_CAL, FP_IFE, FP_LOOP, FP_REP, FP_RET
		};

		const char* FpOpName(unsigned op)
		{
			static const char* names[] = {
				"NOP", "MOV", "MUL", "ADD", "MAD", "DP3", "DP4", "DST", "MIN", "MAX", "SLT", "SGE", "SLE", "SGT", "SNE", "SEQ",
				"FRC", "FLR", "KIL", "PK4", "UP4", "DDX", "DDY", "TEX", "TXP", "TXD", "RCP", "RSQ", "EX2", "LG2", "LIT", "LRP",
				"STR", "SFL", "COS", "SIN", "PK2", "UP2", "POW", "PKB", "UPB", "PK16", "UP16", "BEM", "PKG", "UPG", "DP2A", "TXL",
				"op30", "TXB", "op32", "TEXBEM", "TXPBEM", "BEMLUM", "REFL", "TIMESWTEX", "DP2", "NRM", "DIV", "DIVSQ", "LIF", "FENCT",
				"FENCB", "op3f", "BRK", "CAL", "IFE", "LOOP", "REP", "RET" };
			return op < sizeof(names) / sizeof(names[0]) ? names[op] : "op?";
		}

		unsigned FpWord(const unsigned char* u, unsigned index)
		{
			const unsigned char* p = u + 4 * index;
			return (Be16(p + 2) << 16) | Be16(p);
		}

		struct FpInstruction
		{
			unsigned pc;			// slot index
			unsigned w[4];			// dst, src0, src1, src2
			bool hasLiteral;
			unsigned literalOffset;	// byte offset of the literal in the ucode
			float literal[4];

			unsigned Op() const { return Bits(w[0], 24, 6) | (Bits(w[2], 31, 1) << 6); }
		};

		class C_FragmentTranslator
		{
		public:
			C_FragmentTranslator(const Program& prog, const FragmentConstants& constants, const FragmentSamplers& samplers, Counters& counters)
				: m_prog(prog), m_constants(constants), m_samplers(samplers), m_counters(counters), m_needW(false), m_maxConstant(-1)
			{
				memset(m_usedR, 0, sizeof(m_usedR));
				memset(m_usedH, 0, sizeof(m_usedH));
				memset(m_writtenR, 0, sizeof(m_writtenR));
				memset(m_usedCC, 0, sizeof(m_usedCC));
				memset(m_inputs, 0, sizeof(m_inputs));
				memset(m_samplerUsed, 0, sizeof(m_samplerUsed));
				memset(m_storage, 0, sizeof(m_storage));
			}

			bool Run(std::string& hlsl, std::string& error, std::string& notes)
			{
				if (!Decode(error))
					return false;
				const unsigned control = m_prog.FpControl();
				if (control & 0xe)
					return Fail(error, "depth export (fragment control 0x" + Hex(control) + ")");
				for (size_t i = 0; i < m_ins.size(); ++i)
					if (!Instruction(m_ins[i], error))
						return false;
				return Build(hlsl, error, notes);
			}

		private:
			static bool Fail(std::string& error, const std::string& text)
			{
				error = text;
				return false;
			}

			static std::string Hex(unsigned v)
			{
				char b[16];
				sprintf_s(b, "%x", v);
				return b;
			}

			bool Decode(std::string& error)
			{
				const unsigned slots = m_prog.ucodeSize / 16;
				unsigned pc = 0;
				bool ended = false;
				while (pc < slots && !ended)
				{
					FpInstruction in;
					in.pc = pc;
					for (int k = 0; k < 4; ++k)
						in.w[k] = FpWord(m_prog.ucode, pc * 4 + k);
					in.hasLiteral = false;
					in.literalOffset = 0;
					++pc;
					for (int s = 1; s <= 3; ++s)
						if (Bits(in.w[s], 0, 2) == 2)
							in.hasLiteral = true;
					if (in.hasLiteral)
					{
						if (pc >= slots)
							return Fail(error, "literal past the end of the program");
						in.literalOffset = pc * 16;
						for (int c = 0; c < 4; ++c)
						{
							const unsigned bits = FpWord(m_prog.ucode, pc * 4 + c);
							memcpy(&in.literal[c], &bits, 4);
						}
						++pc;
					}
					ended = Bits(in.w[0], 0, 1) != 0;
					m_ins.push_back(in);
				}
				if (!ended)
					return Fail(error, "no end bit");
				return true;
			}

			// Lanes of each operand (after its swizzle) that the operation reads.
			static unsigned OperandLanes(unsigned op, int operand, unsigned dstMask, bool cube)
			{
				switch (op)
				{
				case FP_MOV: case FP_FRC: case FP_FLR:
					return operand == 0 ? dstMask : 0;
				case FP_MUL: case FP_ADD: case FP_MIN: case FP_MAX: case FP_SLT: case FP_SGE: case FP_SLE:
				case FP_SGT: case FP_SNE: case FP_SEQ:
					return operand < 2 ? dstMask : 0;
				case FP_MAD: case FP_LRP:
					return dstMask;
				case FP_DP2: return operand < 2 ? 0x3 : 0;
				case FP_DP3: return operand < 2 ? 0x7 : 0;
				case FP_DP4: return operand < 2 ? 0xf : 0;
				case FP_DST: return operand == 0 ? 0x6 : operand == 1 ? 0xa : 0;
				case FP_NRM: return operand == 0 ? 0x7 : 0;
				case FP_RCP: case FP_RSQ: case FP_EX2: case FP_LG2: case FP_COS: case FP_SIN:
					return operand == 0 ? 0x1 : 0;
				case FP_POW: return operand < 2 ? 0x1 : 0;
				case FP_DIV: case FP_DIVSQ:
					return operand == 0 ? dstMask : operand == 1 ? 0x1 : 0;
				case FP_TEX:
					return operand == 0 ? (cube ? 0x7 : 0x3) : 0;
				case FP_TXB: case FP_TXL:
					return operand == 0 ? (cube ? 0x7 : 0x3) : operand == 1 ? 0x1 : 0;
				default:
					return 0;
				}
			}

			static unsigned Swizzle(unsigned src, int lane) { return Bits(src, 9 + 2 * lane, 2); }

			// Components of the source register that the lanes read.
			static unsigned SourceComponents(unsigned src, unsigned lanes)
			{
				unsigned comps = 0;
				for (int c = 0; c < 4; ++c)
					if (lanes & (1 << c))
						comps |= 1 << Swizzle(src, c);
				return comps;
			}

			// Temp register storage: R(n) component c; H(m) component c lives in
			// R(m / 2) component 2 * (m & 1) + c / 2. 1 = last written as R, 2 = as H.
			bool CheckRead(unsigned src, unsigned lanes, std::string& error)
			{
				if (Bits(src, 0, 2) != 0 || !lanes)
					return true;
				const unsigned reg = Bits(src, 2, 6);
				const bool half = Bits(src, 8, 1) != 0;
				const unsigned comps = SourceComponents(src, lanes);
				for (int c = 0; c < 4; ++c)
				{
					if (!(comps & (1 << c)))
						continue;
					const unsigned storageReg = half ? reg / 2 : reg;
					const unsigned storageComp = half ? 2 * (reg & 1) + c / 2 : c;
					const unsigned last = m_storage[storageReg][storageComp];
					if (last && last != (half ? 2u : 1u))
						return Fail(error, std::string(half ? "H" : "R") + Num(reg) + "." + kLane[c] +
							" read after the aliased " + (half ? "R" : "H") + " register was written");
				}
				return true;
			}

			void MarkWrite(bool half, unsigned reg, unsigned mask)
			{
				for (int c = 0; c < 4; ++c)
				{
					if (!(mask & (1 << c)))
						continue;
					if (half)
						m_storage[reg / 2][2 * (reg & 1) + c / 2] = 2;
					else
						m_storage[reg][c] = 1;
				}
			}

			std::string Temp(bool half, unsigned reg)
			{
				if (half)
					m_usedH[reg] = true;
				else
					m_usedR[reg] = true;
				return std::string(half ? "h" : "r") + Num(reg);
			}

			bool Source(const FpInstruction& in, int operand, unsigned lanes, std::string& out, std::string& error)
			{
				const unsigned src = in.w[1 + operand];
				const unsigned type = Bits(src, 0, 2);
				std::string base;
				unsigned precision = Bits(in.w[2], 19 + 3 * operand, 3);
				switch (type)
				{
				case 0:
					base = Temp(Bits(src, 8, 1) != 0, Bits(src, 2, 6));
					break;
				case 1:
				{
					const unsigned attr = Bits(in.w[0], 13, 4);
					if (Bits(in.w[3], 30, 1))
						return Fail(error, "indexed input read");
					const bool perspective = Bits(in.w[3], 31, 1) != 0;
					if (attr == 1 || attr == 2)
					{
						m_inputs[attr] = true;
						base = std::string("saturate(i.col") + (attr == 1 ? "0" : "1") + ")";
						precision = 0;
						if (perspective)
							return Fail(error, "perspective flag on a colour input");
					}
					else if (attr >= 4 && attr <= 12)
					{
						m_inputs[attr] = true;
						const unsigned n = attr - 4;
						const std::string name = "i.tex" + Num(n);
						const bool flagged2D = (m_prog.FpTexCoords2D() >> n) & 1;
						const unsigned comps = SourceComponents(src, lanes);
						if (perspective)
						{
							m_needW = true;
							++m_counters.perspectiveReads;
							base = flagged2D ? "float4(" + name + ".xy / i.w.x, 0.0, 1.0)" : "(" + name + " / i.w.x)";
						}
						else if (flagged2D && (comps & 0xc))
						{
							m_needW = true;
							++m_counters.texcoord2DzwReads;
							base = "float4(" + name + ".xy, 0.0, i.w.x)";
						}
						else
							base = name;
					}
					else
						return Fail(error, "fragment input attribute " + Num(attr));
					break;
				}
				case 2:
				{
					++m_counters.fpLiterals;
					const int reg = m_constants.Find(in.literalOffset);
					if (reg >= 0)
					{
						++m_counters.fpPatchedLiterals;
						base = "C[" + Num((unsigned)reg) + "]";
						if (reg > m_maxConstant)
							m_maxConstant = reg;
					}
					else if (!Literal4(in.literal, base))
						return Fail(error, "literal is not finite");
					break;
				}
				default:
					return Fail(error, "source type 3");
				}
				std::string e = base;
				std::string sw = ".";
				for (int c = 0; c < 4; ++c)
					sw += kLane[Swizzle(src, c)];
				if (sw != ".xyzw")
					e = "(" + e + ")" + sw;
				const bool abs = operand == 0 ? Bits(src, 29, 1) != 0 : Bits(src, 18, 1) != 0;
				switch (precision)
				{
				case 0: case 1: case 5: break;
				case 2: case 3: case 4: break;
				default: return Fail(error, "source precision " + Num(precision));
				}
				if (precision == 4)
					e = "saturate(" + e + ")";
				if (abs)
					e = "abs(" + e + ")";
				if (precision == 2)
					e = "clamp(" + e + ", -2.0, 2.0)";
				else if (precision == 3)
					e = "clamp(" + e + ", -1.0, 1.0)";
				if (Bits(src, 17, 1))
					e = "-(" + e + ")";
				out = "(" + e + ")";
				return true;
			}

			// Condition of src0's exec bits: "" when always, "false" when never.
			std::string Condition(const FpInstruction& in)
			{
				const unsigned s0 = in.w[1];
				const unsigned cond = Bits(s0, 18, 1) | (Bits(s0, 19, 1) << 1) | (Bits(s0, 20, 1) << 2);
				if (cond == 7)
					return "";
				if (cond == 0)
					return "false";
				const unsigned reg = Bits(s0, 31, 1);
				m_usedCC[reg] = true;
				std::string sw;
				for (int c = 0; c < 4; ++c)
					sw += kLane[Bits(s0, 21 + 2 * c, 2)];
				return "(cc" + Num(reg) + "." + sw + " " + CompareOp(cond) + " 0.0)";
			}

			bool Instruction(const FpInstruction& in, std::string& error)
			{
				++m_counters.fpInstructions;
				const unsigned op = in.Op();
				const unsigned dst = in.w[0];
				const unsigned mask = Bits(dst, 9, 4);
				const bool noDest = Bits(dst, 30, 1) != 0;
				const bool setCond = Bits(dst, 8, 1) != 0;
				const unsigned unit = Bits(dst, 17, 4);
				const bool cube = m_samplers.dim[unit] == E_SamplerDim_Cube;
				if (Bits(dst, 21, 1))
					return Fail(error, "texture expand (_bx2)");

				if (op == FP_NOP || op == FP_FENCT || op == FP_FENCB)
					return true;

				// MOV Rn.m <- Rn.m (same register and view, identity swizzle on the written
				// lanes, no modifier, unconditional) copies the same bits: no operation. sce-cgc
				// emits it (RPCS3 notes "r0.xy = r0.xy" too), sometimes over storage that the
				// other view wrote last and nothing reads afterwards.
				if (op == FP_MOV && !noDest && !setCond && Bits(in.w[1], 0, 2) == 0 &&
					Bits(in.w[1], 2, 6) == Bits(dst, 1, 6) && Bits(in.w[1], 8, 1) == Bits(dst, 7, 1) &&
					!Bits(in.w[1], 17, 1) && !Bits(in.w[1], 29, 1) && !Bits(in.w[2], 19, 3) && !Bits(in.w[2], 28, 3) &&
					!Bits(dst, 31, 1) && Bits(dst, 22, 2) != 2 && Bits(dst, 22, 2) != 3 &&
					Bits(in.w[1], 18, 3) == 7)
				{
					bool identity = true;
					for (int c = 0; c < 4; ++c)
						if ((mask & (1 << c)) && Swizzle(in.w[1], c) != (unsigned)c)
							identity = false;
					if (identity)
						return true;
				}

				// Sources, with the aliasing proof for the lanes the operation reads.
				std::string s[3];
				for (int k = 0; k < 3; ++k)
				{
					const unsigned lanes = OperandLanes(op, k, mask, cube);
					if (!lanes)
						continue;
					if (!CheckRead(in.w[1 + k], lanes, error))
						return false;
					if (!Source(in, k, lanes, s[k], error))
						return false;
				}

				if (op == FP_KIL)
				{
					++m_counters.kills;
					const std::string c = Condition(in);
					if (c == "false")
						return true;
					if (c.empty())
						m_body += "\tdiscard;\n";
					else
						m_body += "\tif (any" + c + ") discard;\n";
					return true;
				}

				std::string v;
				switch (op)
				{
				case FP_MOV: v = s[0]; break;
				case FP_MUL: v = s[0] + " * " + s[1]; break;
				case FP_ADD: v = s[0] + " + " + s[1]; break;
				case FP_MAD: v = s[0] + " * " + s[1] + " + " + s[2]; break;
				// [Blur1Multi] DP2 goes through Dot2 (a multiply and an add): d3dcompiler_47 turns "c - dot(a.xy, b.xy)"
				// into dp2add with a negated source and drops that source's abs modifier (softlightflare_ln: 1 - dot(|h|, |h|)
				// compiled to dp2add r0_abs, -r0, 1, whiting out half of every light flare). See Dot2 below.
				case FP_DP2: v = "Dot2(" + s[0] + ", " + s[1] + ").xxxx"; m_needDot2 = true; break;
				case FP_DP3: v = "dot(" + s[0] + ".xyz, " + s[1] + ".xyz).xxxx"; break;
				case FP_DP4: v = "dot(" + s[0] + ", " + s[1] + ").xxxx"; break;
				case FP_DST: v = "float4(1.0, " + s[0] + ".y * " + s[1] + ".y, " + s[0] + ".z, " + s[1] + ".w)"; break;
				case FP_MIN: v = "min(" + s[0] + ", " + s[1] + ")"; break;
				case FP_MAX: v = "max(" + s[0] + ", " + s[1] + ")"; break;
				case FP_SLT: v = "(float4)(" + s[0] + " < " + s[1] + ")"; break;
				case FP_SGE: v = "(float4)(" + s[0] + " >= " + s[1] + ")"; break;
				case FP_SLE: v = "(float4)(" + s[0] + " <= " + s[1] + ")"; break;
				case FP_SGT: v = "(float4)(" + s[0] + " > " + s[1] + ")"; break;
				case FP_SNE: v = "(float4)(" + s[0] + " != " + s[1] + ")"; break;
				case FP_SEQ: v = "(float4)(" + s[0] + " == " + s[1] + ")"; break;
				case FP_SFL: v = "float4(0.0, 0.0, 0.0, 0.0)"; break;
				case FP_STR: v = "float4(1.0, 1.0, 1.0, 1.0)"; break;
				case FP_FRC: v = "frac(" + s[0] + ")"; break;
				case FP_FLR: v = "floor(" + s[0] + ")"; break;
				case FP_RCP: v = "(1.0 / " + s[0] + ".x).xxxx"; break;
				case FP_RSQ: v = "(1.0 / sqrt(abs(" + s[0] + ".x))).xxxx"; break;
				case FP_EX2: v = "exp2(" + s[0] + ".x).xxxx"; break;
				case FP_LG2: v = "log2(" + s[0] + ".x).xxxx"; break;
				case FP_COS: v = "cos(" + s[0] + ".x).xxxx"; break;
				case FP_SIN: v = "sin(" + s[0] + ".x).xxxx"; break;
				case FP_LRP: v = "lerp(" + s[2] + ", " + s[1] + ", " + s[0] + ")"; break;
				case FP_NRM: v = "normalize(" + s[0] + ".xyz).xyzz"; break;
				case FP_DIV: v = "(" + s[0] + " / " + s[1] + ".x)"; break;
				case FP_DIVSQ: v = "DivSq(" + s[0] + ", " + s[1] + ".x)"; m_needDivSq = true; break;
				case FP_TEX: case FP_TXB: case FP_TXL:
				{
					m_samplerUsed[unit] = true;
					if (!m_samplers.bound[unit])
						++m_counters.unboundSamplerFetches;
					const std::string sn = "S" + Num(unit);
					if (cube)
					{
						++m_counters.cubeFetches;
						if (op == FP_TEX) v = "texCUBE(" + sn + ", " + s[0] + ".xyz)";
						else if (op == FP_TXB) v = "texCUBEbias(" + sn + ", float4(" + s[0] + ".xyz, " + s[1] + ".x))";
						else v = "texCUBElod(" + sn + ", float4(" + s[0] + ".xyz, " + s[1] + ".x))";
					}
					else
					{
						if (op == FP_TEX) v = "tex2D(" + sn + ", " + s[0] + ".xy)";
						else if (op == FP_TXB) v = "tex2Dbias(" + sn + ", float4(" + s[0] + ".xy, 0.0, " + s[1] + ".x))";
						else v = "tex2Dlod(" + sn + ", float4(" + s[0] + ".xy, 0.0, " + s[1] + ".x))";
					}
					break;
				}
				default:
					return Fail(error, std::string("fragment opcode ") + FpOpName(op));
				}

				switch (Bits(in.w[2], 28, 3))
				{
				case 0: break;
				case 1: v = "(" + v + ") * 2.0"; break;
				case 2: v = "(" + v + ") * 4.0"; break;
				case 3: v = "(" + v + ") * 8.0"; break;
				case 5: v = "(" + v + ") / 2.0"; break;
				case 6: v = "(" + v + ") / 4.0"; break;
				case 7: v = "(" + v + ") / 8.0"; break;
				default: return Fail(error, "destination scale 4");
				}

				const std::string cond = Condition(in);
				if (cond == "false" || !mask)
					return true;
				if (!cond.empty())
					++m_counters.conditionalWrites;
				const std::string m = "." + MaskString(mask);

				if (noDest)
				{
					if (setCond)
					{
						const unsigned cc = Bits(in.w[1], 30, 1);
						m_usedCC[cc] = true;
						m_body += "\tt = " + v + ";\n";
						const std::string target = "cc" + Num(cc) + m;
						if (cond.empty())
							m_body += "\t" + target + " = t" + m + ";\n";
						else
							m_body += "\t" + target + " = (" + cond + ")" + m + " ? t" + m + " : " + target + ";\n";
					}
					return true;
				}

				if (Bits(dst, 31, 1))
					v = "saturate(" + v + ")";
				else
				{
					const unsigned prec = Bits(dst, 22, 2);
					const bool exempt = op == FP_NRM || op == FP_MAX || op == FP_MIN || op == FP_COS || op == FP_SIN ||
						op == FP_FRC || op == FP_LIT || op == FP_LG2;
					if (prec == 2 && !exempt)
						v = "clamp(" + v + ", -2.0, 2.0)";
					else if (prec == 3 && !exempt)
						v = "clamp(" + v + ", -1.0, 1.0)";
				}

				const bool half = Bits(dst, 7, 1) != 0;
				const unsigned reg = Bits(dst, 1, 6);
				const std::string target = Temp(half, reg) + m;
				m_body += "\tt = " + v + ";\n";
				if (cond.empty())
					m_body += "\t" + target + " = t" + m + ";\n";
				else
					m_body += "\t" + target + " = (" + cond + ")" + m + " ? t" + m + " : " + target + ";\n";
				MarkWrite(half, reg, mask);
				if (!half)
					m_writtenR[reg] = true;
				if (setCond)
				{
					const unsigned cc = Bits(in.w[1], 30, 1);
					m_usedCC[cc] = true;
					m_body += "\tcc" + Num(cc) + m + " = " + target + ";\n";
				}
				return true;
			}

			bool Build(std::string& hlsl, std::string& error, std::string& notes)
			{
				const bool fullOutputs = (m_prog.FpControl() & 0x40) != 0;
				// Colour outputs: R0, R2, R3, R4 (32-bit export) or H0, H4, H6, H8.
				const unsigned outputRegs[4] = { fullOutputs ? 0u : 0u, fullOutputs ? 2u : 4u, fullOutputs ? 3u : 6u, fullOutputs ? 4u : 8u };
				bool outputs[4] = { true, false, false, false };
				// Render targets 1-3: only when the exported register's final content came
				// through the exported view (a register that ends up holding half-register
				// temporaries was a temporary, not an output). The retail PS3 programs write
				// R2-R4 in every technique, prepass included (35 of 391 prepass programs
				// end with R2 written as R; retail PC writes oC1 only in prepass techniques),
				// so these are mostly temporaries; exporting them is harmless where the
				// engine binds one target, which D3D9 ignores.
				for (int k = 1; k < 4; ++k)
				{
					const unsigned reg = outputRegs[k];
					bool any = false, same = true;
					for (int c = 0; c < 4; ++c)
					{
						const unsigned last = fullOutputs ? m_storage[reg][c] : m_storage[reg / 2][2 * (reg & 1) + c / 2];
						if (last)
						{
							any = true;
							if (last != (fullOutputs ? 1u : 2u))
								same = false;
						}
					}
					// GCM render-target sets are contiguous (CELL_GCM_SURFACE_TARGET_0, MRT1 = 0-1,
					// MRT2 = 0-2, MRT3 = 0-3), and D3D9 requires contiguous COLOR outputs: an
					// output after a missing one is never exported.
					outputs[k] = any && same && outputs[k - 1];
					if (any && !outputs[k])
						++m_counters.mrtSkipped;
				}
				for (int k = 0; k < 4; ++k)
				{
					if (!outputs[k])
						continue;
					if (k)
						++m_counters.mrtOutputs;
					// The exported register must have been written through the same view.
					const unsigned reg = outputRegs[k];
					for (int c = 0; c < 4; ++c)
					{
						const unsigned storageReg = fullOutputs ? reg : reg / 2;
						const unsigned storageComp = fullOutputs ? c : 2 * (reg & 1) + c / 2;
						const unsigned last = m_storage[storageReg][storageComp];
						if (last && last != (fullOutputs ? 1u : 2u))
						{
							return Fail(error, "exported colour register last written through the aliased view");
						}
					}
					Temp(!fullOutputs, reg);
				}

				unsigned inputCount = 0;
				std::string in = "struct PS_IN\n{\n";
				if (m_inputs[1]) { in += "\tfloat4 col0 : COLOR0;\n"; ++inputCount; }
				if (m_inputs[2]) { in += "\tfloat4 col1 : COLOR1;\n"; ++inputCount; }
				for (unsigned a = 4; a <= 12; ++a)
					if (m_inputs[a])
					{
						in += "\tfloat4 tex" + Num(a - 4) + " : TEXCOORD" + Num(a - 4) + ";\n";
						++inputCount;
					}
				if (m_needW)
				{
					in += "\tfloat4 w : TEXCOORD" + Num(kWInterpolator) + ";\n";
					++inputCount;
				}
				if (inputCount == 0)
					in += "\tfloat4 unused : TEXCOORD0;\n";
				in += "};\n\n";
				if (inputCount > 10)
					return Fail(error, "more than 10 fragment inputs");

				hlsl = "// RSX fragment program translated for Direct3D 9 (Blur1Multi RsxTranslate)\n";
				if (m_maxConstant >= 0)
				{
					if (m_maxConstant >= 224)
						return Fail(error, "fragment constant register " + Num((unsigned)m_maxConstant));
					// Sized to the highest register read, as in the vertex translator.
					hlsl += "float4 C[" + Num((unsigned)m_maxConstant + 1) + "] : register(c0);\n";
				}
				for (unsigned u = 0; u < 16; ++u)
					if (m_samplerUsed[u])
						hlsl += std::string(m_samplers.dim[u] == E_SamplerDim_Cube ? "samplerCUBE" : "sampler2D") + " S" + Num(u) + " : register(s" + Num(u) + ");\n";
				hlsl += "\n";
				if (m_needDot2)
					hlsl += "float Dot2(float4 a, float4 b)\n{\n\tfloat2 p = a.xy * b.xy;\n\treturn p.x + p.y;\n}\n\n";
				if (m_needDivSq)
					hlsl += "float4 DivSq(float4 a, float b)\n{\n\tfloat4 q = a / sqrt(abs(b));\n\treturn (abs(a) > 0.0) ? q : a;\n}\n\n";
				hlsl += in;
				hlsl += "struct PS_OUT\n{\n";
				for (int k = 0; k < 4; ++k)
					if (outputs[k])
						hlsl += "\tfloat4 c" + Num(k) + " : COLOR" + Num(k) + ";\n";
				hlsl += "};\n\nPS_OUT main(PS_IN i)\n{\n\tfloat4 t;\n";
				for (unsigned r = 0; r < 64; ++r)
				{
					if (m_usedR[r]) hlsl += "\tfloat4 r" + Num(r) + " = 0.0;\n";
					if (m_usedH[r]) hlsl += "\tfloat4 h" + Num(r) + " = 0.0;\n";
				}
				for (int c = 0; c < 2; ++c)
					if (m_usedCC[c])
						hlsl += "\tfloat4 cc" + Num(c) + " = 0.0;\n";
				hlsl += m_body;
				hlsl += "\tPS_OUT o;\n";
				for (int k = 0; k < 4; ++k)
					if (outputs[k])
						hlsl += "\to.c" + Num(k) + " = " + (fullOutputs ? "r" : "h") + Num(outputRegs[k]) + ";\n";
				hlsl += "\treturn o;\n}\n";
				return true;
			}

			const Program& m_prog;
			const FragmentConstants& m_constants;
			const FragmentSamplers& m_samplers;
			Counters& m_counters;
			std::vector<FpInstruction> m_ins;
			std::string m_body;
			bool m_usedR[64], m_usedH[64], m_writtenR[64], m_usedCC[2], m_inputs[16], m_samplerUsed[16];
			unsigned m_storage[64][4];
			bool m_needW;
			bool m_needDivSq = false;
			bool m_needDot2 = false; // [Blur1Multi]
			int m_maxConstant;
		};
	}

	bool TranslateFragment(const Program& prog, const FragmentConstants& constants, const FragmentSamplers& samplers,
		std::string& hlsl, std::string& error, std::string& notes, Counters& counters)
	{
		if (!prog.fragment)
		{
			error = "not a fragment program";
			return false;
		}
		C_FragmentTranslator t(prog, constants, samplers, counters);
		return t.Run(hlsl, error, notes);
	}

	// ---------------------------------------------------------------------------------
	// Vertex programs

	namespace
	{
		enum
		{
			VEC_NOP = 0, VEC_MOV, VEC_MUL, VEC_ADD, VEC_MAD, VEC_DP3, VEC_DPH, VEC_DP4, VEC_DST, VEC_MIN, VEC_MAX,
			VEC_SLT, VEC_SGE, VEC_ARL, VEC_FRC, VEC_FLR, VEC_SEQ, VEC_SFL, VEC_SGT, VEC_SLE, VEC_SNE, VEC_STR, VEC_SSG,
			VEC_TXL = 0x19
		};
		enum
		{
			SCA_NOP = 0, SCA_MOV, SCA_RCP, SCA_RCC, SCA_RSQ, SCA_EXP, SCA_LOG, SCA_LIT, SCA_BRA, SCA_BRI, SCA_CAL,
			SCA_CLI, SCA_RET, SCA_LG2, SCA_EX2, SCA_SIN, SCA_COS, SCA_BRB, SCA_CLB, SCA_PSH, SCA_POP
		};

		// GCM attribute index -> D3D9 input semantic (E_VertexElementUsage for GCM,
		// display2/include/display2/context/GCMContextSPU.h:480-493).
		const char* AttributeSemantic(unsigned attr)
		{
			static const char* table[16] = {
				"POSITION0", "BLENDWEIGHT0", "NORMAL0", "COLOR0", NULL, "FOG0", "PSIZE0", "BLENDINDICES0",
				"TEXCOORD0", "TEXCOORD1", "TEXCOORD2", "TEXCOORD3", "TEXCOORD4", "TEXCOORD5", "TANGENT0", "BINORMAL0" };
			return attr < 16 ? table[attr] : NULL;
		}

		class C_VertexTranslator
		{
		public:
			C_VertexTranslator(const Program& prog, const VertexConstants& constants, Counters& counters)
				: m_prog(prog), m_constants(constants), m_counters(counters), m_maxConstant(-1)
			{
				memset(m_usedR, 0, sizeof(m_usedR));
				memset(m_inputs, 0, sizeof(m_inputs));
				memset(m_outputs, 0, sizeof(m_outputs));
				memset(m_usedCC, 0, sizeof(m_usedCC));
				memset(m_usedA, 0, sizeof(m_usedA));
			}

			bool Run(std::string& hlsl, std::string& error, std::string& notes)
			{
				const unsigned count = m_prog.ucodeSize / 16;
				bool ended = false;
				for (unsigned pc = 0; pc < count && !ended; ++pc)
				{
					while (!m_openTargets.empty() && m_openTargets.back() == pc)
					{
						m_openTargets.pop_back();
						m_body += "\t}\n";
					}
					unsigned d[4];
					for (int k = 0; k < 4; ++k)
						d[k] = Be32(m_prog.ucode + 16 * pc + 4 * k);
					if (!Instruction(pc, d, error))
						return false;
					ended = Bits(d[3], 0, 1) != 0;
				}
				if (!ended)
					return Fail(error, "no end bit");
				if (!m_openTargets.empty())
					return Fail(error, "branch target past the end of the program");
				return Build(hlsl, error, notes);
			}

		private:
			static bool Fail(std::string& error, const std::string& text)
			{
				error = text;
				return false;
			}

			static unsigned SourceBits(const unsigned* d, int n)
			{
				if (n == 0) return Bits(d[2], 23, 9) | (Bits(d[1], 0, 8) << 9);
				if (n == 1) return Bits(d[2], 6, 17);
				return Bits(d[3], 21, 11) | (Bits(d[2], 0, 6) << 11);
			}

			bool Source(const unsigned* d, int n, std::string& out, std::string& error)
			{
				const unsigned src = SourceBits(d, n);
				std::string base;
				switch (Bits(src, 0, 2))
				{
				case 1:
				{
					const unsigned reg = Bits(src, 2, 6);
					m_usedR[reg] = true;
					base = "r" + Num(reg);
					break;
				}
				case 2:
				{
					if (Bits(d[0], 27, 1))
						return Fail(error, "indexed vertex input");
					const unsigned attr = Bits(d[1], 8, 4);
					if (!AttributeSemantic(attr))
						return Fail(error, "vertex attribute " + Num(attr));
					m_inputs[attr] = true;
					base = "i.a" + Num(attr);
					break;
				}
				case 3:
				{
					const unsigned reg = Bits(d[1], 12, 10);
					if (reg >= (unsigned)VertexConstants::kRegisters)
						return Fail(error, "constant register " + Num(reg));
					if (Bits(d[3], 1, 1))
					{
						const int mapped = m_constants.d3dRegister[reg];
						if (mapped < 0)
							return Fail(error, "relative constant read from unbound register " + Num(reg));
						const unsigned a = Bits(d[0], 24, 1);
						m_usedA[a] = true;
						base = "C[a" + Num(a) + "." + kLane[Bits(d[0], 0, 2)] + " + " + Num((unsigned)mapped) + "]";
						if (mapped > m_maxConstant)
							m_maxConstant = mapped;
						// A relative read may reach any register of its binding: the array
						// must cover the whole binding.
						if (m_constants.rangeEnd[reg] - 1 > m_maxConstant)
							m_maxConstant = m_constants.rangeEnd[reg] - 1;
					}
					else if (m_constants.literal[reg])
					{
						if (!Literal4(m_constants.value[reg], base))
							return Fail(error, "constant literal is not finite");
					}
					else if (m_constants.d3dRegister[reg] >= 0)
					{
						const int mapped = m_constants.d3dRegister[reg];
						base = "C[" + Num((unsigned)mapped) + "]";
						if (mapped > m_maxConstant)
							m_maxConstant = mapped;
					}
					else
					{
						if (reg >= 256)
							return Fail(error, "unbound constant register " + Num(reg));
						++m_counters.unboundConstantReads;
						m_notes += "reads unbound constant c" + Num(reg) + "; ";
						base = "C[" + Num(reg) + "]";
						if ((int)reg > m_maxConstant)
							m_maxConstant = reg;
					}
					break;
				}
				default:
					return Fail(error, "vertex source type 0");
				}
				std::string e = base;
				std::string sw = ".";
				sw += kLane[Bits(src, 14, 2)];
				sw += kLane[Bits(src, 12, 2)];
				sw += kLane[Bits(src, 10, 2)];
				sw += kLane[Bits(src, 8, 2)];
				if (sw != ".xyzw")
					e = "(" + e + ")" + sw;
				if (Bits(d[0], 21 + n, 1))
					e = "abs(" + e + ")";
				if (Bits(src, 16, 1))
					e = "-(" + e + ")";
				out = "(" + e + ")";
				return true;
			}

			// "" = always, "false" = never, else a bool4 expression.
			std::string Condition(const unsigned* d)
			{
				const unsigned cond = Bits(d[0], 10, 3);
				if (cond == 0)
					return "false";
				if (!Bits(d[0], 13, 1) || cond == 7)
					return "";
				const unsigned cc = Bits(d[0], 25, 1);
				m_usedCC[cc] = true;
				std::string sw;
				sw += kLane[Bits(d[0], 8, 2)];
				sw += kLane[Bits(d[0], 6, 2)];
				sw += kLane[Bits(d[0], 4, 2)];
				sw += kLane[Bits(d[0], 2, 2)];
				return "(cc" + Num(cc) + "." + sw + " " + CompareOp(cond) + " 0.0)";
			}

			void Assign(const std::string& target, const std::string& value, const std::string& m, const std::string& cond)
			{
				if (cond.empty())
					m_body += "\t" + target + m + " = " + value + m + ";\n";
				else
					m_body += "\t" + target + m + " = (" + cond + ")" + m + " ? " + value + m + " : " + target + m + ";\n";
			}

			bool Instruction(unsigned pc, const unsigned* d, std::string& error)
			{
				++m_counters.vpInstructions;
				const unsigned vec = Bits(d[1], 22, 5);
				const unsigned sca = Bits(d[1], 27, 5);
				const bool sat = Bits(d[0], 26, 1) != 0;
				const unsigned output = Bits(d[3], 2, 5);
				const bool vecResult = Bits(d[0], 30, 1) != 0;
				const bool ccUpdate = Bits(d[0], 14, 1) || Bits(d[0], 29, 1);
				const std::string cond = Condition(d);
				if (!cond.empty() && cond != "false")
					++m_counters.conditionalWrites;

				// Read every source of both halves before any write.
				std::string vecValue, scaValue;
				if (vec != VEC_NOP)
				{
					std::string s0, s1, s2;
					const bool use0 = true;
					const bool use1 = vec == VEC_MUL || vec == VEC_MAD || vec == VEC_DP3 || vec == VEC_DPH || vec == VEC_DP4 ||
						vec == VEC_DST || vec == VEC_MIN || vec == VEC_MAX || vec == VEC_SLT || vec == VEC_SGE || vec == VEC_SEQ ||
						vec == VEC_SGT || vec == VEC_SLE || vec == VEC_SNE;
					const bool use2 = vec == VEC_ADD || vec == VEC_MAD;
					if (vec == VEC_SFL || vec == VEC_STR)
						;
					else
					{
						if (use0 && !Source(d, 0, s0, error)) return false;
						if (use1 && !Source(d, 1, s1, error)) return false;
						if (use2 && !Source(d, 2, s2, error)) return false;
					}
					switch (vec)
					{
					case VEC_MOV: vecValue = s0; break;
					case VEC_MUL: vecValue = s0 + " * " + s1; break;
					case VEC_ADD: vecValue = s0 + " + " + s2; break;
					case VEC_MAD: vecValue = s0 + " * " + s1 + " + " + s2; break;
					case VEC_DP3: vecValue = "dot(" + s0 + ".xyz, " + s1 + ".xyz).xxxx"; break;
					case VEC_DPH: vecValue = "(dot(" + s0 + ".xyz, " + s1 + ".xyz) + " + s1 + ".w).xxxx"; break;
					case VEC_DP4: vecValue = "dot(" + s0 + ", " + s1 + ").xxxx"; break;
					case VEC_DST: vecValue = "float4(1.0, " + s0 + ".y * " + s1 + ".y, " + s0 + ".z, " + s1 + ".w)"; break;
					case VEC_MIN: vecValue = "min(" + s0 + ", " + s1 + ")"; break;
					case VEC_MAX: vecValue = "max(" + s0 + ", " + s1 + ")"; break;
					case VEC_SLT: vecValue = "(float4)(" + s0 + " < " + s1 + ")"; break;
					case VEC_SGE: vecValue = "(float4)(" + s0 + " >= " + s1 + ")"; break;
					case VEC_SEQ: vecValue = "(float4)(" + s0 + " == " + s1 + ")"; break;
					case VEC_SGT: vecValue = "(float4)(" + s0 + " > " + s1 + ")"; break;
					case VEC_SLE: vecValue = "(float4)(" + s0 + " <= " + s1 + ")"; break;
					case VEC_SNE: vecValue = "(float4)(" + s0 + " != " + s1 + ")"; break;
					case VEC_SFL: vecValue = "float4(0.0, 0.0, 0.0, 0.0)"; break;
					case VEC_STR: vecValue = "float4(1.0, 1.0, 1.0, 1.0)"; break;
					case VEC_FRC: vecValue = "frac(" + s0 + ")"; break;
					case VEC_FLR: vecValue = "floor(" + s0 + ")"; break;
					case VEC_SSG: vecValue = "sign(" + s0 + ")"; break;
					case VEC_ARL: vecValue = "floor(" + s0 + ")"; break;
					default:
						return Fail(error, "vertex vector opcode " + Num(vec));
					}
				}
				if (sca != SCA_NOP && sca != SCA_BRI)
				{
					std::string s;
					if (!Source(d, 2, s, error))
						return false;
					const std::string x = s + ".x";
					switch (sca)
					{
					case SCA_MOV: scaValue = x + ".xxxx"; break;
					case SCA_RCP: scaValue = "(1.0 / " + x + ").xxxx"; break;
					case SCA_RCC: scaValue = "clamp(1.0 / " + x + ", 5.42101e-20, 1.884467e19).xxxx"; break;
					case SCA_RSQ: scaValue = "(1.0 / sqrt(abs(" + x + "))).xxxx"; break;
					case SCA_LG2: scaValue = "log2(max(" + x + ", 1e-10)).xxxx"; break;
					case SCA_EX2: scaValue = "exp2(" + x + ").xxxx"; break;
					case SCA_SIN: scaValue = "sin(" + x + ").xxxx"; break;
					case SCA_COS: scaValue = "cos(" + x + ").xxxx"; break;
					default:
						return Fail(error, "vertex scalar opcode " + Num(sca));
					}
				}
				if (!vecValue.empty())
					m_body += "\ttv = " + std::string(sat ? "saturate(" + vecValue + ")" : vecValue) + ";\n";
				if (!scaValue.empty())
					m_body += "\tts = " + std::string(sat ? "saturate(" + scaValue + ")" : scaValue) + ";\n";

				if (cond != "false")
				{
					if (!vecValue.empty())
					{
						const unsigned mask = (Bits(d[3], 16, 1)) | (Bits(d[3], 15, 1) << 1) | (Bits(d[3], 14, 1) << 2) | (Bits(d[3], 13, 1) << 3);
						const unsigned temp = Bits(d[0], 15, 6);
						const std::string m = mask == 0xf ? "" : "." + MaskString(mask);
						bool wrote = false;
						if (mask)
						{
							if (vec == VEC_ARL)
							{
								if (temp > 1)
									return Fail(error, "address register " + Num(temp));
								m_usedA[temp] = true;
								Assign("a" + Num(temp), "(int4)tv", m, cond);
								wrote = true;
							}
							else
							{
								if (vecResult && output != 0x1f)
								{
									if (!OutputName(output, error))
										return false;
									Assign("o" + Num(output), "tv", m, cond);
									wrote = true;
								}
								if (temp != 0x3f)
								{
									m_usedR[temp] = true;
									Assign("r" + Num(temp), "tv", m, cond);
									wrote = true;
								}
							}
							if (!wrote && ccUpdate)
							{
								const unsigned cc = Bits(d[0], 25, 1);
								m_usedCC[cc] = true;
								Assign("cc" + Num(cc), "tv", m, cond);
								wrote = true;
							}
							else if (wrote && ccUpdate)
								return Fail(error, "condition update together with a register write");
						}
					}
					if (!scaValue.empty())
					{
						const unsigned mask = (Bits(d[3], 20, 1)) | (Bits(d[3], 19, 1) << 1) | (Bits(d[3], 18, 1) << 2) | (Bits(d[3], 17, 1) << 3);
						const unsigned temp = Bits(d[3], 7, 6);
						const std::string m = mask == 0xf ? "" : "." + MaskString(mask);
						bool wrote = false;
						if (mask)
						{
							if (!vecResult && output != 0x1f)
							{
								if (!OutputName(output, error))
									return false;
								Assign("o" + Num(output), "ts", m, cond);
								wrote = true;
							}
							if (temp != 0x3f)
							{
								m_usedR[temp] = true;
								Assign("r" + Num(temp), "ts", m, cond);
								wrote = true;
							}
							if (!wrote && ccUpdate)
							{
								const unsigned cc = Bits(d[0], 25, 1);
								m_usedCC[cc] = true;
								Assign("cc" + Num(cc), "ts", m, cond);
							}
							else if (wrote && ccUpdate && vecValue.empty())
								return Fail(error, "condition update together with a register write");
						}
					}
				}

				if (sca == SCA_BRI)
				{
					++m_counters.branches;
					const unsigned target = (Bits(d[0], 23, 1) << 9) | (Bits(d[2], 0, 6) << 3) | Bits(d[3], 29, 3);
					if (target <= pc)
						return Fail(error, "backward branch");
					if (!m_openTargets.empty() && target > m_openTargets.back())
						return Fail(error, "overlapping branches");
					if (cond == "false")
						return true;
					if (cond.empty())
						m_body += "\tif (false)\n\t{\n";
					else
						m_body += "\tif (!any" + cond + ")\n\t{\n";
					m_openTargets.push_back(target);
				}
				return true;
			}

			bool OutputName(unsigned output, std::string& error)
			{
				if (output == 0 || output == 1 || output == 2 || (output >= 7 && output <= 15))
				{
					m_outputs[output] = true;
					return true;
				}
				return Fail(error, "vertex output register " + Num(output));
			}

			static std::string OutputSemantic(unsigned output)
			{
				if (output == 0) return "POSITION";
				if (output == 1) return "COLOR0";
				if (output == 2) return "COLOR1";
				return "TEXCOORD" + Num(output - 7);
			}

			bool Build(std::string& hlsl, std::string& error, std::string& notes)
			{
				if (!m_outputs[0])
					return Fail(error, "no position output");
				unsigned outputs = 1;	// the w interpolator
				for (int k = 0; k < 16; ++k)
					if (m_outputs[k])
						++outputs;
				if (outputs > 12)
					return Fail(error, "more than 12 vertex outputs");
				if (m_maxConstant >= 256)
					return Fail(error, "vertex constant register " + Num((unsigned)m_maxConstant));

				hlsl = "// RSX vertex program translated for Direct3D 9 (Blur1Multi RsxTranslate)\n";
				// The array ends at the highest register the program reads: fxc places the
				// program's literals (def) after it. Declared whole (C[256]), fxc kept all 256
				// registers for every program with relative addressing and put the literals at
				// c256 and up, past the 256 vertex constants of shader model 3 (591 programs:
				// every skinned and SpeedTree vertex program).
				if (m_maxConstant >= 0)
					hlsl += "float4 C[" + Num((unsigned)m_maxConstant + 1) + "] : register(c0);\n";
				hlsl += "\nstruct VS_IN\n{\n";
				bool anyInput = false;
				for (unsigned a = 0; a < 16; ++a)
					if (m_inputs[a])
					{
						hlsl += "\tfloat4 a" + Num(a) + " : " + AttributeSemantic(a) + ";\n";
						anyInput = true;
					}
				if (!anyInput)
					hlsl += "\tfloat4 a0 : POSITION0;\n";
				hlsl += "};\n\nstruct VS_OUT\n{\n";
				for (unsigned k = 0; k < 16; ++k)
					if (m_outputs[k])
						hlsl += "\tfloat4 o" + Num(k) + " : " + OutputSemantic(k) + ";\n";
				hlsl += "\tfloat4 w : TEXCOORD" + Num(kWInterpolator) + ";\n";
				hlsl += "};\n\nVS_OUT main(VS_IN i)\n{\n\tfloat4 tv, ts;\n";
				for (unsigned r = 0; r < 64; ++r)
					if (m_usedR[r])
						hlsl += "\tfloat4 r" + Num(r) + " = 0.0;\n";
				for (int a = 0; a < 2; ++a)
					if (m_usedA[a])
						hlsl += "\tint4 a" + Num(a) + " = 0;\n";
				for (int c = 0; c < 2; ++c)
					if (m_usedCC[c])
						hlsl += "\tfloat4 cc" + Num(c) + " = 0.0;\n";
				for (unsigned k = 0; k < 16; ++k)
					if (m_outputs[k])
						hlsl += "\tfloat4 o" + Num(k) + " = float4(0.0, 0.0, 0.0, 1.0);\n";
				hlsl += m_body;
				hlsl += "\tVS_OUT o;\n";
				for (unsigned k = 0; k < 16; ++k)
					if (m_outputs[k])
						hlsl += "\to.o" + Num(k) + " = o" + Num(k) + ";\n";
				hlsl += "\to.w = float4(o0.w, 0.0, 0.0, 1.0);\n\treturn o;\n}\n";
				notes += m_notes;
				return true;
			}

			const Program& m_prog;
			const VertexConstants& m_constants;
			Counters& m_counters;
			std::string m_body, m_notes;
			std::vector<unsigned> m_openTargets;
			bool m_usedR[64], m_inputs[16], m_outputs[16], m_usedCC[2], m_usedA[2];
			int m_maxConstant;
		};
	}

	bool TranslateVertex(const Program& prog, const VertexConstants& constants, std::string& hlsl,
		std::string& error, std::string& notes, Counters& counters)
	{
		if (prog.fragment)
		{
			error = "not a vertex program";
			return false;
		}
		C_VertexTranslator t(prog, constants, counters);
		return t.Run(hlsl, error, notes);
	}
}
