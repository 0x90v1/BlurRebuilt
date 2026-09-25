// [Blur1Multi] Converters that run the engine's own deserialiser (T072).
//
// Some formats are plain serialised streams with no layout of their own: the
// reader pulls scalars one after another (C_Cycle::Deserialise for .cycle, C_Skeleton::Deserialise for .skeleton). For
// those the converter runs that exact reader over a transcoding stream: every
// scalar the reader asks for is taken from the 360 bytes big-endian and written
// to the output little-endian, so the output is what the PC reader consumes, by
// construction. Checked against the retail PC files with BLUR1MULTI_PAKDUMP.

#include "StdAfx.h"

#include "EngineConverters.h"

#include <system/serialise/Serialise.h>
#include <animation/cycle/Cycle.h>
#include <animation/cycle/CycleDeserialise.inl>
#include <animation/skeleton/Skeleton.h>
#include <animation/skeleton/SkeletonDeserialise.inl>

#include <stdio.h>
#include <string.h>

namespace X360Convert
{
	namespace
	{
		class C_TranscodeStream
		{
		public:
			C_TranscodeStream(const unsigned char* p, size_t n, std::vector<unsigned char>& out)
				: m_p(p), m_n(n), m_at(0), m_out(out), m_ok(true) {}

			// The scalar set of Sys::BinaryDeserialiser (system/serialise/BinarySerialise.h);
			// every other type goes through Deserialise(stream, object) as it does there.
			C_TranscodeStream& operator>>(U8& v)  { return Scalar(v); }
			C_TranscodeStream& operator>>(S8& v)  { return Scalar(v); }
			C_TranscodeStream& operator>>(char& v) { return Scalar(v); }
			C_TranscodeStream& operator>>(U16& v) { return Scalar(v); }
			C_TranscodeStream& operator>>(S16& v) { return Scalar(v); }
			C_TranscodeStream& operator>>(U32& v) { return Scalar(v); }
			C_TranscodeStream& operator>>(S32& v) { return Scalar(v); }
			C_TranscodeStream& operator>>(F32& v) { return Scalar(v); }
			C_TranscodeStream& operator>>(F64& v) { return Scalar(v); }
			C_TranscodeStream& operator>>(U64& v) { return Scalar(v); }
			C_TranscodeStream& operator>>(S64& v) { return Scalar(v); }

			template <typename T>
			C_TranscodeStream& operator>>(T& object)
			{
				using Sys::Deserialise;		// as inside Sys::BinaryDeserialiser; ADL still finds type-specific overloads
				Deserialise(*this, object);
				return *this;
			}

			template <typename T>
			C_TranscodeStream& Scalar(T& value)
			{
				unsigned char tmp[sizeof(T)];
				if (m_at + sizeof(T) > m_n)
				{
					m_ok = false;
					memset(&value, 0, sizeof(T));
					return *this;
				}
				for (size_t i = 0; i < sizeof(T); ++i)
					tmp[i] = m_p[m_at + sizeof(T) - 1 - i];
				memcpy(&value, tmp, sizeof(T));
				m_out.insert(m_out.end(), tmp, tmp + sizeof(T));
				m_at += sizeof(T);
				return *this;
			}

			// raw bytes (characters): no byte order
			bool Read(void* dst, U32 bytes)
			{
				if (m_at + bytes > m_n)
				{
					m_ok = false;
					memset(dst, 0, bytes);
					return false;
				}
				memcpy(dst, m_p + m_at, bytes);
				m_out.insert(m_out.end(), m_p + m_at, m_p + m_at + bytes);
				m_at += bytes;
				return true;
			}

			bool Ok() const { return m_ok; }
			size_t Consumed() const { return m_at; }

		private:
			const unsigned char* m_p;
			size_t m_n, m_at;
			std::vector<unsigned char>& m_out;
			bool m_ok;
		};

		bool EndsWith(const char* path, const char* ext)
		{
			const size_t n = strlen(path), m = strlen(ext);
			if (n < m)
				return false;
			for (size_t i = 0; i < m; ++i)
				if (tolower((unsigned char)path[n - m + i]) != ext[i])
					return false;
			return true;
		}

		E_Result ConvertCycle(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error)
		{
			out.clear();
			out.reserve(inSize);
			C_TranscodeStream stream(in, inSize, out);
			Animation::C_Cycle cycle;
			const bool read = cycle.Deserialise(stream, Animation::C_Cycle::kLoadFull);
			if (!read || !stream.Ok())
			{
				char msg[160];
				sprintf_s(msg, "C_Cycle::Deserialise did not accept the swapped stream (read %d, stopped at %u of %u bytes, fields %u)",
					read ? 1 : 0, (unsigned)stream.Consumed(), (unsigned)inSize, cycle.GetNumFields());
				error = msg;
				return E_Result_Failed;
			}
			if (stream.Consumed() != inSize)
			{
				error = "C_Cycle::Deserialise left bytes unread";
				return E_Result_Failed;
			}
			return E_Result_Converted;
		}

		E_Result ConvertSkeleton(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error)
		{
			out.clear();
			out.reserve(inSize);
			C_TranscodeStream stream(in, inSize, out);
			Animation::C_Skeleton skeleton;
			const bool read = skeleton.Deserialise(stream);
			if (!read || !stream.Ok() || stream.Consumed() != inSize)
			{
				char msg[160];
				sprintf_s(msg, "C_Skeleton::Deserialise did not accept the swapped stream (read %d, stopped at %u of %u bytes)",
					read ? 1 : 0, (unsigned)stream.Consumed(), (unsigned)inSize);
				error = msg;
				return E_Result_Failed;
			}
			return E_Result_Converted;
		}
	}

	bool EngineHandlesPath(const char* path)
	{
		return path && (EndsWith(path, ".cycle") || EndsWith(path, ".skeleton"));
	}

	E_Result EngineConvertFile(const char* path, const unsigned char* in, size_t inSize,
		std::vector<unsigned char>& out, std::string& error)
	{
		if (path && EndsWith(path, ".cycle"))
			return ConvertCycle(in, inSize, out, error);
		if (path && EndsWith(path, ".skeleton"))
			return ConvertSkeleton(in, inSize, out, error);
		return E_Result_NotHandled;
	}
}
