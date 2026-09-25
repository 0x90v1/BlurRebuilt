// [Blur1Multi] XT flask databases (gamedata/xt/*.bin, *.xt): Xbox 360 -> PC.
//
// The exporter (common/xt2/source/tool/Export/XtFlaskExport.cpp CompileFlask,
// XtFlaskSwizzle.cpp) writes one layout for every platform and byte-swaps it for
// a big-endian target profile:
//   FlaskHeader: tag 'FLSK', version, then eight {U32 offset, U32 count} blobs
//     (types, bases, fields, strings, refs, records, components, data)
//   types      FlaskType      6 x U16
//   bases      FlaskBase      2 x U16          (per type, in type order: its bases)
//   fields     FlaskField     3 x U16, 2 x U8  (per type, in type order: its fields)
//   strings    bytes
//   refs       FlaskRef       U32 id, U16 type, U16 record
//   records    FlaskRecord    4 x U32 (ref, componentCount, dataBytes, stringBytes)
//   components FlaskComponent 3 x U16 (type, count, behaviour), records' components in order
//   data       per record: dataBytes of component data, then stringBytes of encoded strings
// Record data is swapped by SwizzleRecord: each component's elements, laid out
// one after another (each component's bytes rounded up to 4), through the
// type's bases and fields. The swap is its own inverse, so running the same walk
// with the file's (byte-swapped) tables gives the PC bytes. Enum values below
// are XtReflection.h's.

#include <x360convert/X360Convert.h>

#include "ChunkFile.h"

#include <stdio.h>
#include <string.h>

namespace X360Convert
{
	namespace
	{
		// XtTypeBehaviour
		enum { kTbAtom, kTbEnum, kTbFlags, kTbStruct };
		// XtFieldBehaviour (file values; weak pointers are exported as kFbPtr)
		enum { kFbAtom, kFbEnum, kFbFlags, kFbStruct, kFbPtr, kFbHandle, kFbLabel };
		// XtAtomType
		enum { kNotAtom, kBool, kI8, kI16, kI32, kI64, kU8, kU16, kU32, kU64, kF32, kF64, kSz8, kSz16, kLocId };
		// XtComponentBehaviour
		enum { kCbObject, kCbPtr, kCbHandle };

		struct Type { unsigned name, fieldCount, baseCount, behaviour, atom, size, firstField, firstBase; };
		struct Base { unsigned type, offset; };
		struct Field { unsigned name, type, offset, behaviour, array; };

		void Swap16(unsigned char* p) { unsigned char t = p[0]; p[0] = p[1]; p[1] = t; }
		void Swap32(unsigned char* p) { unsigned char t = p[0]; p[0] = p[3]; p[3] = t; t = p[1]; p[1] = p[2]; p[2] = t; }
		void Swap64(unsigned char* p) { for (int i = 0; i < 4; ++i) { unsigned char t = p[i]; p[i] = p[7 - i]; p[7 - i] = t; } }

		struct Flask
		{
			unsigned char* d;
			size_t n;
			std::vector<Type> types;
			std::vector<Base> bases;
			std::vector<Field> fields;
			std::string error;

			bool In(size_t at, size_t bytes) const { return at <= n && bytes <= n - at; }

			bool Fail(const char* text) { error = text; return false; }

			// SwizzleField / SwizzleStruct, bounded to the record's data [lo, hi)
			bool SwapField(unsigned behaviour, unsigned array, unsigned typeIndex, size_t at, size_t lo, size_t hi, int depth)
			{
				if (depth > 64)
					return Fail("struct nesting too deep");
				if (array)
				{
					if (at < lo || at + 8 > hi) return Fail("array outside its record");
					Swap16(d + at); Swap16(d + at + 2); Swap32(d + at + 4);
					return true;
				}
				switch (behaviour)
				{
				case kFbLabel:
					return true;
				case kFbPtr:
					if (at < lo || at + 4 > hi) return Fail("pointer outside its record");
					Swap16(d + at); Swap16(d + at + 2);
					return true;
				case kFbHandle:
				case kFbEnum:
				case kFbFlags:
					if (at < lo || at + 4 > hi) return Fail("value outside its record");
					Swap32(d + at);
					return true;
				case kFbAtom:
				{
					if (typeIndex >= types.size()) return Fail("atom field with a bad type");
					const unsigned atom = types[typeIndex].atom;
					if (atom == kNotAtom || atom > kLocId) return Fail("unknown atom type");
					const bool wide = atom == kI64 || atom == kU64 || atom == kF64;
					if (at < lo || at + (wide ? 8 : 4) > hi) return Fail("atom outside its record");
					if (wide) Swap64(d + at); else Swap32(d + at);
					return true;
				}
				case kFbStruct:
				{
					if (typeIndex >= types.size()) return Fail("struct field with a bad type");
					const Type& t = types[typeIndex];
					for (unsigned b = 0; b < t.baseCount; ++b)
					{
						const Base& base = bases[t.firstBase + b];
						if (!SwapField(kFbStruct, 0, base.type, at + base.offset, lo, hi, depth + 1))
							return false;
					}
					for (unsigned f = 0; f < t.fieldCount; ++f)
					{
						const Field& fl = fields[t.firstField + f];
						if (!SwapField(fl.behaviour, fl.array, fl.type, at + fl.offset, lo, hi, depth + 1))
							return false;
					}
					return true;
				}
				default:
					return Fail("unknown field behaviour");
				}
			}
		};
	}

	E_Result ConvertFlask(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error)
	{
		if (inSize < 72 || memcmp(in, "FLSK", 4) != 0)
		{
			error = "not a big-endian flask";
			return E_Result_Failed;
		}
		out.assign(in, in + inSize);
		Flask fl;
		fl.d = &out[0];
		fl.n = out.size();
		for (size_t o = 0; o < 72; o += 4)
			Swap32(fl.d + o);
		unsigned h[18];
		for (int i = 0; i < 18; ++i)
			h[i] = fl.d[i * 4] | (fl.d[i * 4 + 1] << 8) | (fl.d[i * 4 + 2] << 16) | ((unsigned)fl.d[i * 4 + 3] << 24);
		if (h[1] != 2)
		{
			error = "flask version is not 2";
			return E_Result_Failed;
		}
		const unsigned typeOff = h[2], typeCount = h[3], baseOff = h[4], baseCount = h[5], fieldOff = h[6], fieldCount = h[7];
		const unsigned refOff = h[10], refCount = h[11], recOff = h[12], recCount = h[13], compOff = h[14], compCount = h[15];
		const unsigned dataOff = h[16], dataBytes = h[17];
		if (!fl.In(typeOff, typeCount * 12) || !fl.In(baseOff, baseCount * 4) || !fl.In(fieldOff, fieldCount * 8) ||
			!fl.In(refOff, refCount * 8) || !fl.In(recOff, recCount * 16) || !fl.In(compOff, compCount * 6) || !fl.In(dataOff, dataBytes))
		{
			error = "flask blob outside the file";
			return E_Result_Failed;
		}

		// schema tables
		unsigned nextField = 0, nextBase = 0;
		for (unsigned i = 0; i < typeCount; ++i)
		{
			unsigned char* p = fl.d + typeOff + i * 12;
			for (int k = 0; k < 6; ++k)
				Swap16(p + k * 2);
			Type t;
			t.name = p[0] | (p[1] << 8); t.fieldCount = p[2] | (p[3] << 8); t.baseCount = p[4] | (p[5] << 8);
			t.behaviour = p[6] | (p[7] << 8); t.atom = p[8] | (p[9] << 8); t.size = p[10] | (p[11] << 8);
			t.firstField = nextField; t.firstBase = nextBase;
			nextField += t.fieldCount; nextBase += t.baseCount;
			fl.types.push_back(t);
		}
		if (nextField != fieldCount || nextBase != baseCount)
		{
			error = "type table does not account for every base and field";
			return E_Result_Failed;
		}
		for (unsigned i = 0; i < baseCount; ++i)
		{
			unsigned char* p = fl.d + baseOff + i * 4;
			Swap16(p); Swap16(p + 2);
			Base b = { (unsigned)(p[0] | (p[1] << 8)), (unsigned)(p[2] | (p[3] << 8)) };
			fl.bases.push_back(b);
		}
		for (unsigned i = 0; i < fieldCount; ++i)
		{
			unsigned char* p = fl.d + fieldOff + i * 8;
			Swap16(p); Swap16(p + 2); Swap16(p + 4);
			Field f = { (unsigned)(p[0] | (p[1] << 8)), (unsigned)(p[2] | (p[3] << 8)), (unsigned)(p[4] | (p[5] << 8)), p[6], p[7] };
			fl.fields.push_back(f);
		}
		for (unsigned i = 0; i < refCount; ++i)
		{
			unsigned char* p = fl.d + refOff + i * 8;
			Swap32(p); Swap16(p + 4); Swap16(p + 6);
		}
		for (unsigned i = 0; i < compCount; ++i)
		{
			unsigned char* p = fl.d + compOff + i * 6;
			Swap16(p); Swap16(p + 2); Swap16(p + 4);
		}

		// records and their data
		size_t data = dataOff;
		unsigned comp = 0;
		for (unsigned r = 0; r < recCount; ++r)
		{
			unsigned char* p = fl.d + recOff + r * 16;
			for (int k = 0; k < 4; ++k)
				Swap32(p + k * 4);
			const unsigned components = p[4] | (p[5] << 8) | (p[6] << 16) | ((unsigned)p[7] << 24);
			const unsigned recData = p[8] | (p[9] << 8) | (p[10] << 16) | ((unsigned)p[11] << 24);
			const unsigned recStrings = p[12] | (p[13] << 8) | (p[14] << 16) | ((unsigned)p[15] << 24);
			if (comp + components > compCount || !fl.In(data, (size_t)recData + recStrings))
			{
				error = "record data or components outside the flask";
				return E_Result_Failed;
			}
			const size_t lo = data, hi = data + recData;
			size_t at = lo;
			for (unsigned c = 0; c < components; ++c, ++comp)
			{
				const unsigned char* cp = fl.d + compOff + comp * 6;
				const unsigned type = cp[0] | (cp[1] << 8), count = cp[2] | (cp[3] << 8), behaviour = cp[4] | (cp[5] << 8);
				unsigned elementBytes, fieldBehaviour;
				if (behaviour == kCbObject)
				{
					if (type >= fl.types.size())
					{
						error = "component with a bad type";
						return E_Result_Failed;
					}
					elementBytes = fl.types[type].size;
					switch (fl.types[type].behaviour)
					{
					case kTbAtom: fieldBehaviour = kFbAtom; break;
					case kTbEnum: fieldBehaviour = kFbEnum; break;
					case kTbFlags: fieldBehaviour = kFbFlags; break;
					case kTbStruct: fieldBehaviour = kFbStruct; break;
					default: error = "component type with an unknown behaviour"; return E_Result_Failed;
					}
				}
				else if (behaviour == kCbPtr) { elementBytes = 4; fieldBehaviour = kFbPtr; }
				else if (behaviour == kCbHandle) { elementBytes = 4; fieldBehaviour = kFbHandle; }
				else
				{
					error = "unknown component behaviour";
					return E_Result_Failed;
				}
				for (unsigned e = 0; e < count; ++e)
					if (!fl.SwapField(fieldBehaviour, 0, type, at + (size_t)e * elementBytes, lo, hi, 0))
					{
						char msg[160];
						sprintf_s(msg, "record %u component %u: %s", r, c, fl.error.c_str());
						error = msg;
						return E_Result_Failed;
					}
				at += ((size_t)elementBytes * count + 3) & ~(size_t)3;
			}
			if (at != hi)
			{
				error = "record components do not fill its data";
				return E_Result_Failed;
			}
			data = hi + recStrings;		// strings: encoded bytes, no byte order
		}
		if (comp != compCount || data != (size_t)dataOff + dataBytes)
		{
			error = "records do not account for every component and data byte";
			return E_Result_Failed;
		}
		return E_Result_Converted;
	}
}
