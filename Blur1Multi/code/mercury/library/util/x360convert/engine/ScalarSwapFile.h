// [Blur1Multi] A read-only Sys::I_File that converts as it is read (T072).
//
// Its buffer pointers stay null, so I_BinaryStream::Read hands every read to
// Underflow with the reader's exact target and size. Readers built on
// Sys::BinaryDeserialiser pull each scalar as one read of its own width, so an
// engine reader run over this file converts the bytes it reads:
//  - 2- and 4-byte reads are taken big-endian and returned little-endian;
//  - 1-byte reads, and reads of 8 bytes or more (C_FixedString block ids and
//    names, character arrays), are passed through as they are;
//  - a skip (a NULL read, or SetPosition forward) passes the bytes through and
//    is counted, since the reader did not say what they are.
// Everything returned to the reader is also appended to 'out', which is the PC
// file once the reader has consumed the whole input. Nested C_FileBlocks work:
// their headers are an 8-byte id, U32 size, U16 count and two bytes.
#pragma once

#include <system/serialise/I_File.h>

#include <string>
#include <vector>

namespace X360Convert
{
	class C_ScalarSwapFile : public Sys::I_File
	{
	public:
		// The first 'rawPrefix' bytes (a character tag read as a U32) pass through.
		C_ScalarSwapFile(const unsigned char* p, size_t n, const char* name, std::vector<unsigned char>& out, size_t rawPrefix = 0)
			: m_p(p), m_n(n), m_at(0), m_raw(rawPrefix), m_name(name), m_out(out), m_version(0), m_failed(false),
			  m_skipped(0), m_wide(0) {}

		const bool IsWriting() const { return false; }
		Btl::String Name() const { return m_name; }
		const bool GetEndianSwapFlag() const { return false; }
		void SetEndianSwapFlag(const bool) {}
		U32 GetVersionNumber() const { return m_version; }
		void SetVersionNumber(U32 v) { m_version = v; }
		const Sys::C_StreamPos GetPosition() const { return (U64)m_at; }
		const Sys::C_StreamSize Length() { return (U64)m_n; }
		bool Overflow() { return false; }

		void SetPosition(const Sys::C_StreamPos newPosition)
		{
			if (newPosition < m_at || newPosition > m_n)
			{
				Fail("seek outside the forward range");
				return;
			}
			const size_t n = (size_t)newPosition - m_at;
			m_out.insert(m_out.end(), m_p + m_at, m_p + m_at + n);
			m_skipped += n;
			m_at += n;
		}

		bool Underflow(UnderflowContext& uc)
		{
			const U32 n = uc.readSize;
			if (m_failed || m_at + n > m_n)
			{
				Fail("read past the end");
				return false;
			}
			const unsigned char* src = m_p + m_at;
			if (!uc.readTarget)
			{
				m_out.insert(m_out.end(), src, src + n);
				m_skipped += n;
			}
			else
			{
				if ((n == 2 || n == 4) && m_at + n > m_raw)
				{
					for (U32 i = 0; i < n; ++i)
						uc.readTarget[i] = src[n - 1 - i];
				}
				else
				{
					memcpy(uc.readTarget, src, n);
					if (n >= 8)
						m_wide += 1;
				}
				m_out.insert(m_out.end(), uc.readTarget, uc.readTarget + n);
			}
			m_at += n;
			uc.bytesRead = n;
			return true;
		}

		bool Failed() const { return m_failed; }
		const std::string& Error() const { return m_error; }
		size_t Consumed() const { return m_at; }
		size_t Skipped() const { return m_skipped; }
		unsigned WideReads() const { return m_wide; }

	private:
		void Fail(const char* what)
		{
			if (!m_failed)
			{
				char text[128];
				sprintf_s(text, "%s at byte %u of %u", what, (unsigned)m_at, (unsigned)m_n);
				m_error = text;
			}
			m_failed = true;
		}

		const unsigned char* m_p;
		size_t m_n, m_at, m_raw;
		Btl::String m_name;
		std::vector<unsigned char>& m_out;
		U32 m_version;
		bool m_failed;
		std::string m_error;
		size_t m_skipped;
		unsigned m_wide;
	};
}
