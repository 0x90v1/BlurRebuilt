// [Blur1Multi] Xbox 360 -> PC conversion of level files (T072).
//
// C_LevelResource (gamesys/game_resource/source/LevelResource.cpp) opens
// level.level, then level.levelphysics and level.levelrender; the render
// file's scene streams level.levelstream (see SceneTranscode for the scene).
// Blocks the engine reads with a reader of its own are converted by running
// that reader, on a temporary object, over a C_FileBlock opened on the 360
// block through a C_ScalarSwapFile (ScalarSwapFile.h); the reader must consume
// the block exactly. Blocks with no reader of their own are converted here.

#include "StdAfx.h"

#include "EngineConverters.h"
#include "ScalarSwapFile.h"
#include "../source/ChunkFile.h"

#include <system/serialise/C_FileBlock.h>
#include <system/serialise/BinarySerialise.h>
#include <NavMesh/NavMesh.h>
#include <advertising/AdvertResource.h>
#include <world_render/LightList.h>
#include <world_render/LightSamplePoints.h>
#include <world_render/CubemapPoints.h>
#include <portal/System.h>
#include <game_resource/CrowdData.h>

#include <stdio.h>
#include <string.h>

namespace X360Convert
{
	// SceneTranscode.cpp (RenderingData library).
	bool TranscodePvsArchive(const unsigned char* in, size_t size, std::vector<unsigned char>& out, std::string& error);

	namespace
	{
		// world_render/source/RenderLevel.cpp E_LevelRenderFileVersion_EffectVariation
		const U32 kLevelRenderFileVersion_EffectVariation = 1;

		// Runs 'read' on a C_FileBlock opened over the whole 360 block (header
		// included) and appends the converted block.
		template <typename Reader>
		bool ConvertBlockWithReader(const Block& b, std::vector<unsigned char>& out, std::string& error, Reader read)
		{
			std::vector<unsigned char> converted;
			C_ScalarSwapFile file(b.header, b.size, b.id.c_str(), converted);
			{
				Sys::C_FileBlock block(&file);
				read(block);
			}
			char text[200];
			if (file.Failed())
			{
				sprintf_s(text, "'%s': %s", b.id.c_str(), file.Error().c_str());
				error = text;
				return false;
			}
			if (file.Consumed() != b.size || file.Skipped())
			{
				sprintf_s(text, "'%s': reader used %u of %u bytes (%u skipped)", b.id.c_str(),
					(unsigned)file.Consumed(), (unsigned)b.size, (unsigned)file.Skipped());
				error = text;
				return false;
			}
			out.insert(out.end(), converted.begin(), converted.end());
			return true;
		}

		// A block of U32 fields only.
		bool ConvertWordsBlock(const Block& b, std::vector<unsigned char>& out, std::string& error)
		{
			if (b.PayloadSize() % 4)
			{
				error = "'" + b.id + "' is not whole 32-bit words";
				return false;
			}
			const size_t at = BeginBlockLE(out, b, b.count);
			for (size_t i = 0; i < b.PayloadSize(); i += 4)
				PutLE32(out, BE32(b.payload + i));
			EndBlockLE(out, at);
			return true;
		}

		bool ConvertGravityBlock(const Block& b, std::vector<unsigned char>& out, std::string& error)
		{
			std::vector<unsigned char> payload;
			if (!ConvertGravityPayload(b.payload, b.PayloadSize(), payload, error))
			{
				error = "'" + b.id + "': " + error;
				return false;
			}
			const size_t at = BeginBlockLE(out, b, b.count);
			out.insert(out.end(), payload.begin(), payload.end());
			EndBlockLE(out, at);
			return true;
		}

		bool StartFile(const unsigned char* in, size_t inSize, const char* rootId, std::vector<unsigned char>& out,
			const unsigned char*& p, size_t& left, std::string& error)
		{
			out.clear();
			out.reserve(inSize + inSize / 2);
			if (inSize < 4 || memcmp(in, "360 ", 4) != 0)
			{
				error = std::string("not a 360 ") + rootId + " file";
				return false;
			}
			out.insert(out.end(), (const unsigned char*)"  CP", (const unsigned char*)"  CP" + 4);
			p = in + 4;
			left = inSize - 4;
			return true;
		}

		// C_LevelResource::LoadLevelData: Level { Header, MdlDat, NavDat, Advert },
		// then MtaDbg (left out, as for models).
		E_Result ConvertLevel(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error)
		{
			const unsigned char* p;
			size_t left;
			if (!StartFile(in, inSize, "level", out, p, left, error))
				return E_Result_Failed;
			while (left)
			{
				Block b;
				if (!ReadBlockBE(p, left, b))
				{
					error = "bad block header";
					return E_Result_Failed;
				}
				if (b.id == "Level")
				{
					std::vector<Block> kids;
					if (!ChildrenBE(b, kids))
					{
						error = "Level children do not fill it";
						return E_Result_Failed;
					}
					const size_t at = BeginBlockLE(out, b, b.count);
					for (size_t i = 0; i < kids.size(); ++i)
					{
						const Block& k = kids[i];
						bool ok;
						if (k.id == "Header")
							ok = ConvertWordsBlock(k, out, error);
						else if (k.id == "MdlDat")
							ok = ConvertMdlDat(k, out, error);
						else if (k.id == "NavDat")
							ok = ConvertBlockWithReader(k, out, error, [](Sys::C_FileBlock& block)
							{
								AI::C_NavMesh navMesh;
								navMesh.LoadMesh(block);
							});
						else if (k.id == "Advert")
							ok = ConvertBlockWithReader(k, out, error, [](Sys::C_FileBlock& block)
							{
								Sys::BinaryDeserialiser<Ut::NoEndianSwap> ar(block);
								Advertising::AdvertResource resource;
								ar >> resource;
							});
						else
						{
							error = "unknown Level child '" + k.id + "'";
							ok = false;
						}
						if (!ok)
							return E_Result_Failed;
					}
					EndBlockLE(out, at);
				}
				else if (b.id != "MtaDbg")
				{
					error = "unknown level block '" + b.id + "'";
					return E_Result_Failed;
				}
				p += b.size;
				left -= b.size;
			}
			return E_Result_Converted;
		}

		// C_PhysicsLevel::Load: Physic { Header {U32 version, S32 routes}, Main,
		// Routes { Route... } }; Main and each Route are Gravity data.
		E_Result ConvertLevelPhysics(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error)
		{
			const unsigned char* p;
			size_t left;
			if (!StartFile(in, inSize, "levelphysics", out, p, left, error))
				return E_Result_Failed;
			Block root;
			std::vector<Block> kids;
			if (!ReadBlockBE(p, left, root) || root.id != "Physic" || root.size != left || !ChildrenBE(root, kids))
			{
				error = "not a Physic block tree";
				return E_Result_Failed;
			}
			const size_t at = BeginBlockLE(out, root, root.count);
			for (size_t i = 0; i < kids.size(); ++i)
			{
				const Block& k = kids[i];
				bool ok = true;
				if (k.id == "Header")
					ok = ConvertWordsBlock(k, out, error);
				else if (k.id == "Main")
					ok = ConvertGravityBlock(k, out, error);
				else if (k.id == "Routes")
				{
					std::vector<Block> routes;
					if (!ChildrenBE(k, routes))
					{
						error = "Routes children do not fill it";
						return E_Result_Failed;
					}
					const size_t rat = BeginBlockLE(out, k, k.count);
					for (size_t r = 0; ok && r < routes.size(); ++r)
					{
						if (routes[r].id != "Route")
						{
							error = "Routes holds '" + routes[r].id + "'";
							ok = false;
						}
						else
							ok = ConvertGravityBlock(routes[r], out, error);
					}
					EndBlockLE(out, rat);
				}
				else
				{
					error = "unknown Physic child '" + k.id + "'";
					ok = false;
				}
				if (!ok)
					return E_Result_Failed;
			}
			EndBlockLE(out, at);
			return E_Result_Converted;
		}

	}

	// WorldRender::C_RenderLevel::Read: Render { Header, Scene, LS, Lights,
	// PVSData, CubePt2, Portals, Crowd }; 'scene' is the converted Scene payload.
	E_Result ConvertLevelRender(const unsigned char* in, size_t inSize, const std::vector<unsigned char>& scene,
		std::vector<unsigned char>& out, std::string& error)
	{
			const unsigned char* p;
			size_t left;
			if (!StartFile(in, inSize, "levelrender", out, p, left, error))
				return E_Result_Failed;
			Block root;
			std::vector<Block> kids;
			if (!ReadBlockBE(p, left, root) || root.id != "Render" || root.size != left || !ChildrenBE(root, kids))
			{
				error = "not a Render block tree";
				return E_Result_Failed;
			}
			const size_t at = BeginBlockLE(out, root, root.count);
			for (size_t i = 0; i < kids.size(); ++i)
			{
				const Block& k = kids[i];
				bool ok;
				if (k.id == "Header")
					ok = ConvertBlockWithReader(k, out, error, [](Sys::C_FileBlock& block)
					{
						Sys::BinaryDeserialiser<Ut::NoEndianSwap> ar(block);
						U32 fileVersion;
						ar >> fileVersion;
						if (fileVersion >= kLevelRenderFileVersion_EffectVariation)
						{
							Util::C_String effectVariation;
							ar >> effectVariation;
						}
					});
				else if (k.id == "Scene")
				{
					// converted with the levelstream (C_LevelConversion)
					const size_t sat = BeginBlockLE(out, k, k.count);
					out.insert(out.end(), scene.begin(), scene.end());
					EndBlockLE(out, sat);
					ok = true;
				}
				else if (k.id == "LS")
					ok = ConvertBlockWithReader(k, out, error, [](Sys::C_FileBlock& block)
					{
						WorldRender::C_LightSamplePoints points;
						points.Read(&block);
					});
				else if (k.id == "Lights")
					ok = ConvertBlockWithReader(k, out, error, [](Sys::C_FileBlock& block)
					{
						WorldRender::C_LightList lights;
						lights.Read(&block);
					});
				else if (k.id == "PVSData")
				{
					std::vector<unsigned char> pvs;
					ok = TranscodePvsArchive(k.payload, k.PayloadSize(), pvs, error);
					if (ok)
					{
						const size_t pat = BeginBlockLE(out, k, k.count);
						out.insert(out.end(), pvs.begin(), pvs.end());
						EndBlockLE(out, pat);
					}
					else
						error = "PVSData: " + error;
				}
				else if (k.id == "CubePt2")
					ok = ConvertBlockWithReader(k, out, error, [](Sys::C_FileBlock& block)
					{
						Sys::BinaryDeserialiser<Ut::NoEndianSwap> ar(block);
						WorldRender::C_LocalisedCubemapData cubemaps;
						ar >> cubemaps;
					});
				else if (k.id == "Portals")
					ok = ConvertBlockWithReader(k, out, error, [](Sys::C_FileBlock& block)
					{
						Portal::C_System portals;
						portals.LoadFromFile(block);
					});
				else if (k.id == "Crowd")
					ok = ConvertBlockWithReader(k, out, error, [](Sys::C_FileBlock& block)
					{
						GameResource::C_CrowdData crowd;
						crowd.Read(&block);
					});
				else
				{
					error = "unknown Render child '" + k.id + "'";
					ok = false;
				}
				if (!ok)
					return E_Result_Failed;
			}
			EndBlockLE(out, at);
			return E_Result_Converted;
		}

	namespace
	{
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
	}

	bool LevelHandlesPath(const char* path)
	{
		return path && (EndsWith(path, ".level") || EndsWith(path, ".levelphysics"));
	}

	E_Result LevelConvertFile(const char* path, const unsigned char* in, size_t inSize,
		std::vector<unsigned char>& out, std::string& error)
	{
		if (!path)
			return E_Result_NotHandled;
		if (EndsWith(path, ".level"))
			return ConvertLevel(in, inSize, out, error);
		if (EndsWith(path, ".levelphysics"))
			return ConvertLevelPhysics(in, inSize, out, error);
		return E_Result_NotHandled;
	}
}
