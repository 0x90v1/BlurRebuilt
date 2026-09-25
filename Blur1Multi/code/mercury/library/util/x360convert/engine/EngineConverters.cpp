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
#include <gravity/sdk/PhysicsSDK.h>
#include <gravity/importer/Importer.h>

#include "ScalarSwapFile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../source/ChunkFile.h"

namespace X360Convert
{
	// SceneTranscode.cpp (RenderingData library).
	bool TranscodeSceneArchive(const unsigned char* in, size_t size, std::vector<unsigned char>& out, std::string& error);

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

		// A block whose payload is one U32 read with BinaryDeserialiser (the model
		// and render Header blocks: the file version).
		bool ConvertVersionBlock(const Block& b, std::vector<unsigned char>& out, std::string& error)
		{
			if (b.PayloadSize() != 4)
			{
				error = "Header block of " + std::to_string((unsigned long long)b.PayloadSize()) + " bytes";
				return false;
			}
			const size_t at = BeginBlockLE(out, b, b.count);
			PutLE32(out, BE32(b.payload));
			EndBlockLE(out, at);
			return true;
		}

		// Swaps every 32-bit word of a payload (size must be a multiple of 4).
		bool PutSwapped32(const unsigned char* p, size_t n, std::vector<unsigned char>& out)
		{
			if (n % 4)
				return false;
			for (size_t i = 0; i < n; i += 4)
				PutLE32(out, BE32(p + i));
			return true;
		}

		// C_ModelData::Read (gamesys/game_resource/source/ModelData.cpp): child blocks
		// read with BinaryDeserialiser. Header, Models, Element and Constr hold only
		// 32-bit fields (S32, F32 matrices and bounds); StrTab is Util::C_StringTable,
		// Ut::Vector<U32> offsets then Ut::Vector<char> (U32 count before each).
	}

	bool ConvertMdlDat(const Block& mdl, std::vector<unsigned char>& out, std::string& error)
		{
			std::vector<Block> kids;
			if (!ChildrenBE(mdl, kids))
			{
				error = "MdlDat children do not fill it";
				return false;
			}
			const size_t at = BeginBlockLE(out, mdl, mdl.count);
			for (size_t i = 0; i < kids.size(); ++i)
			{
				const Block& k = kids[i];
				const size_t kat = BeginBlockLE(out, k, k.count);
				const unsigned char* p = k.payload;
				const size_t n = k.PayloadSize();
				if (k.id == "StrTab")
				{
					if (n < 4)
					{
						error = "StrTab too short";
						return false;
					}
					const unsigned offsets = BE32(p);
					const size_t charsAt = 4 + (size_t)offsets * 4;
					if (charsAt + 4 > n || charsAt + 4 + BE32(p + charsAt) != n)
					{
						error = "StrTab sizes do not fill it";
						return false;
					}
					PutSwapped32(p, charsAt + 4, out);
					out.insert(out.end(), p + charsAt + 4, p + n);
				}
				else if (k.id == "Header" || k.id == "Models" || k.id == "Element" || k.id == "Constr")
				{
					if (!PutSwapped32(p, n, out))
					{
						error = "MdlDat '" + k.id + "' is not whole 32-bit words";
						return false;
					}
				}
				else
				{
					error = "unknown MdlDat child '" + k.id + "'";
					return false;
				}
				EndBlockLE(out, kat);
			}
			EndBlockLE(out, at);
			return true;
		}

	namespace
	{
		bool ConvertCollBlock(const Block& coll, std::vector<unsigned char>& out, std::string& error)
		{
			std::vector<unsigned char> payload;
			if (!ConvertGravityPayload(coll.payload, coll.PayloadSize(), payload, error))
			{
				error = "Coll: " + error;
				return false;
			}
			const size_t at = BeginBlockLE(out, coll, coll.count);
			out.insert(out.end(), payload.begin(), payload.end());
			EndBlockLE(out, at);
			return true;
		}

		// A block the converter cannot convert yet, copied with a PC header when
		// BLUR1MULTI_X360_PARTIAL is set (verification dumps only: the engine cannot
		// read it).
		bool CopyUnconverted(const Block& b, std::vector<unsigned char>& out, std::string& error)
		{
			if (!getenv("BLUR1MULTI_X360_PARTIAL"))
			{
				error = "'" + b.id + "' block not converted yet";
				return false;
			}
			const size_t at = BeginBlockLE(out, b, b.count);
			out.insert(out.end(), b.payload, b.payload + b.PayloadSize());
			EndBlockLE(out, at);
			return true;
		}

		// C_RenderModel::Read: Render { Header, Scene }; the Scene payload is the
		// rendering-data archive WorldRender::ReadScene reads.
		bool ConvertRenderBlock(const Block& render, std::vector<unsigned char>& out, std::string& error);

		// C_ModelResource hands C_RenderModel::Read the model's Render block, and
		// Read opens a Render block inside it.
		bool ConvertModelRenderBlock(const Block& outer, std::vector<unsigned char>& out, std::string& error)
		{
			std::vector<Block> kids;
			if (!ChildrenBE(outer, kids))
			{
				error = "model Render block children do not fill it";
				return false;
			}
			const size_t at = BeginBlockLE(out, outer, outer.count);
			for (size_t i = 0; i < kids.size(); ++i)
			{
				if (kids[i].id != "Render")
				{
					error = "model Render block holds '" + kids[i].id + "'";
					return false;
				}
				if (!ConvertRenderBlock(kids[i], out, error))
					return false;
			}
			EndBlockLE(out, at);
			return true;
		}

		bool ConvertRenderBlock(const Block& render, std::vector<unsigned char>& out, std::string& error)
		{
			std::vector<Block> kids;
			if (!ChildrenBE(render, kids))
			{
				error = "Render block children do not fill it";
				return false;
			}
			const size_t at = BeginBlockLE(out, render, render.count);
			for (size_t i = 0; i < kids.size(); ++i)
			{
				const Block& k = kids[i];
				if (k.id == "Header")
				{
					if (!ConvertVersionBlock(k, out, error))
						return false;
				}
				else if (k.id == "Scene")
				{
					std::vector<unsigned char> scene;
					if (!TranscodeSceneArchive(k.payload, k.PayloadSize(), scene, error))
					{
						error = "Scene: " + error;
						return false;
					}
					const size_t sat = BeginBlockLE(out, k, k.count);
					out.insert(out.end(), scene.begin(), scene.end());
					EndBlockLE(out, sat);
				}
				else
				{
					error = "unknown Render child '" + k.id + "'";
					return false;
				}
			}
			EndBlockLE(out, at);
			return true;
		}

		// C_ModelResource::LoadModelData: platform FourCC, Model { Header, MdlDat,
		// Render, Coll }, then MtaDbg. Older exports have no FourCC and start with
		// the Model block (50 retail 360 models, the same 50 on PC).
		E_Result ConvertModel(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error)
		{
			out.clear();
			out.reserve(inSize * 2);
			size_t skip = 0;
			if (inSize >= 4 && memcmp(in, SourceFourCC(), 4) == 0)
			{
				out.insert(out.end(), (const unsigned char*)"  CP", (const unsigned char*)"  CP" + 4);
				skip = 4;
			}
			else if (inSize < 8 || memcmp(in, "Model\0\0\0", 8) != 0)
			{
				error = "not a console model";
				return E_Result_Failed;
			}

			const unsigned char* p = in + skip;
			size_t left = inSize - skip;
			while (left)
			{
				Block b;
				if (!ReadBlockBE(p, left, b))
				{
					error = "bad block header";
					return E_Result_Failed;
				}
				if (b.id == "Model")
				{
					std::vector<Block> kids;
					if (!ChildrenBE(b, kids))
					{
						error = "Model block children do not fill it";
						return E_Result_Failed;
					}
					const size_t at = BeginBlockLE(out, b, b.count);
					for (size_t i = 0; i < kids.size(); ++i)
					{
						const Block& k = kids[i];
						bool ok;
						if (k.id == "Header")
							ok = ConvertVersionBlock(k, out, error);
						else if (k.id == "Render")
							ok = ConvertModelRenderBlock(k, out, error);
						else if (k.id == "MdlDat")
							ok = ConvertMdlDat(k, out, error);
						else if (k.id == "Coll")
							ok = ConvertCollBlock(k, out, error);
						else
							ok = CopyUnconverted(k, out, error);
						if (!ok)
							return E_Result_Failed;
					}
					EndBlockLE(out, at);
				}
				else if (b.id == "MtaDbg")
				{
					// Export log, read only by non-GOLD builds for logging when bytes
					// remain after the model (C_ModelResource). Not converted: its strings
					// are raw reads whose layout the scalar stream cannot tell apart.
					if (getenv("BLUR1MULTI_X360_PARTIAL") && !CopyUnconverted(b, out, error))
						return E_Result_Failed;
				}
				else if (!CopyUnconverted(b, out, error))
					return E_Result_Failed;
				p += b.size;
				left -= b.size;
			}
			return E_Result_Converted;
		}
	}

	// C_ImportScene::Import (physics/gravity/source/importer/Importer.cpp) reads a
	// Gravity payload (model Coll, level Main and Route blocks) through
	// BinaryDeserialiser<NoEndianSwap>, scalars only (the triangle stream too:
	// C_TriangleHeader/AABBTreeTriangle::DeSerialise), so the importer itself runs
	// over a C_ScalarSwapFile and the recorded bytes are the PC payload. The
	// leading "Head" tag is read as a U32 whose four characters both exporters
	// wrote as they are.
	bool ConvertGravityPayload(const unsigned char* p, size_t n, std::vector<unsigned char>& out, std::string& error)
	{
		const size_t tag = (n >= 4 && memcmp(p, "Head", 4) == 0) ? 4 : 0;
		if (!tag)
		{
			error = "Gravity data without the 'Head' tag (unversioned)";
			return false;
		}
		C_ScalarSwapFile file(p, n, "x360:Gravity", out, tag);
		Gravity::C_ImportScene* pScene = Gravity::C_PhysicsSDK::PhysicsSDK()->CreateImportScene();
		const bool imported = pScene->Import(&file);
		Gravity::C_PhysicsSDK::PhysicsSDK()->ReleaseImportScene(pScene);
		char text[160];
		if (file.Failed())
		{
			error = file.Error();
			return false;
		}
		if (!imported || file.Consumed() != n || file.Skipped() || file.WideReads())
		{
			sprintf_s(text, "importer %s after %u of %u bytes (%u skipped, %u wide reads)", imported ? "stopped" : "failed",
				(unsigned)file.Consumed(), (unsigned)n, (unsigned)file.Skipped(), file.WideReads());
			error = text;
			return false;
		}
		return true;
	}

	bool EngineHandlesPath(const char* path)
	{
		return path && (EndsWith(path, ".cycle") || EndsWith(path, ".skeleton") || EndsWith(path, ".model") || LevelHandlesPath(path));
	}

	E_Result EngineConvertFile(const char* path, const unsigned char* in, size_t inSize,
		std::vector<unsigned char>& out, std::string& error)
	{
		if (path && EndsWith(path, ".cycle"))
			return ConvertCycle(in, inSize, out, error);
		if (path && EndsWith(path, ".skeleton"))
			return ConvertSkeleton(in, inSize, out, error);
		if (path && EndsWith(path, ".model"))
			return ConvertModel(in, inSize, out, error);
		if (LevelHandlesPath(path))
			return LevelConvertFile(path, in, inSize, out, error);
		return E_Result_NotHandled;
	}
}
