#include "stdafx.h"
#include <system/xml/XmlDom.h>
#include <system/debug/log/logger.h>
#include <system/debug/log/subscribers/LogFileSubscriber.h>

#include <tools_shared/shim/Shim.h>
#include <tools_shared/ToolsApplication.h>
#include <tools_shared/VersionStrings.inl>

#include "Application.h"

C_Application::C_Application(
	const char *szName,
	const char *szSubtitle,
	const char *szLogger,
	const char *szDmpPath,
	bool bExecuter)
	:
	Tools::C_ToolsApplication(
		szName,
		szSubtitle,
		szLogger,
		szDmpPath,
		bExecuter)
{
}

C_Application::~C_Application()
{
}

#include <core/maths/storage.h>
#include <core/maths.h>
#include <Core/Maths/HVec/New/IncludeMinimal.h>
#include <Core/Maths/HVec/New/IncludeMinimal.inl>

#include "misc_shared/archive/archive.h"
#include "misc_shared/archive/btl.h"
#include "btl/utility/string.h"
#include "system/serialise/C_FileSystem.h"
#include "system/serialise/C_FileBlock.h"

struct C_GameObjectRecord
{
	Btl::String name;

	/// Write to stream
	template < typename T_Stream >
	FORCE_INLINE void Serialise( T_Stream& stream )
	{
		stream << name;
	}

	/// Read from stream
	template < typename T_Stream >
	FORCE_INLINE void Deserialise( T_Stream& stream )
	{
		stream >> name;
	}
};

struct C_GameObjectSample
{
	F32						time;
	MathsStorage::Float4*	pQuat;
	MathsStorage::Float3*	pPos;

	/// Write to stream
	template < typename T_Stream >
	FORCE_INLINE void Serialise( T_Stream& stream, U32 nActors)
	{
		stream << time;
		for (U32 i = 0; i < nActors; ++i)
		{
			pQuat[i].Serialise(stream);
			pPos[i].Serialise(stream);
		}
	}

	/// Read from stream
	template < typename T_Stream >
	FORCE_INLINE void Deserialise( T_Stream& stream, U32 nActors )
	{
		stream >> time;
		for (U32 i = 0; i < nActors; ++i)
		{
			pQuat[i].Deserialise(stream);
			pPos[i].Deserialise(stream);
		}
	}
};

S32 C_Application::Main(S32 argc, char **argv, Progress::I_ProgressBar *pProgress)
{
	{
		Btl::Vector<C_GameObjectRecord> myActorTable;
		myActorTable.PushBack();
		myActorTable.Back().name = "car1";
		myActorTable.PushBack();
		myActorTable.Back().name = "car2";

		Btl::Vector<MathsStorage::Float4> actorQuats;

		Maths::C_Quaternion car1Quat;
		actorQuats.PushBack();
		Maths::Store(car1Quat, &actorQuats.Back());
		Maths::C_Quaternion car2Quat;
		actorQuats.PushBack();
		Maths::Store(car2Quat, &actorQuats.Back());
		
		Btl::Vector<MathsStorage::Float3> actorTrans;
		Maths::HVec3 car1Pos;
		actorTrans.PushBack();
		Maths::Store(car1Pos, &actorTrans.Back());
		Maths::HVec3 car2Pos;
		actorTrans.PushBack();
		Maths::Store(car2Pos, &actorTrans.Back());

		BIZ_ASSERT(actorQuats.Size() == actorTrans.Size());
		if (actorQuats.Size() == 0)
			return 0;

		U32 nActors = (U32)actorQuats.Size();


		//WRITE FILE
		const char* filename = "z:/mytest.objsample";
		Sys::I_File* pFile = Sys::C_FileSystem::Instance().Open(
			filename, 
			Sys::E_FileAccess_Write);
		if (!pFile)
			return -1;

		//Header
		{
			Sys::C_FileBlock headerBlock(pFile, "objsmp");
			{
				Arch::BinaryOutputArchive ar(headerBlock, Ut::NoEndianSwap::swap );
				U32 version = 1;
				ar << version;
			}

			//Table describing actors.
			{
				Sys::C_FileBlock gameDescBlock(&headerBlock, "objdesc");
				Arch::BinaryOutputArchive ar(gameDescBlock, Ut::NoEndianSwap::swap );
				ar << nActors;
				for (U32 i = 0; i < nActors; ++i)
					myActorTable[i].Serialise(ar);
			}

			//Data per frame.

			{
				Sys::C_StreamPos dataStart;
				U32 nFrames = 0;
				Sys::C_FileBlock gameObjectBlock(&headerBlock, "frames");
				U32 kFrameNotSetPlaceholder = 0xffffffff;
				{
					Arch::BinaryOutputArchive ar(gameObjectBlock, Ut::NoEndianSwap::swap );
					//Write placeholder nframes
					dataStart = gameObjectBlock.GetPosition();
					ar << kFrameNotSetPlaceholder;

					//THIS IS THE BIT WE CALL PER FRAME
					{
						C_GameObjectSample myFrameTestData;
						myFrameTestData.time = 0.f;
						myFrameTestData.pQuat = &actorQuats[0];
						myFrameTestData.pPos = &actorTrans[0];
						myFrameTestData.Serialise(ar, nActors);
						++nFrames;
					}

					//END BIT WE CALL PER FRAME
				}
				//Write num frames in a ninja style.
				{
					Sys::C_StreamPos currentPos = gameObjectBlock.GetPosition();
					gameObjectBlock.SetPosition(dataStart);
					gameObjectBlock.Write(&nFrames, sizeof(U32));
					gameObjectBlock.SetPosition(currentPos);
				}
			}
	
		}
		SAFE_DELETE_PTR(pFile);
	}
	
	//READ FILE
	{
		Btl::Vector<C_GameObjectRecord> myActorTable;
		Btl::Vector<MathsStorage::Float4> actorQuats;
		Btl::Vector<MathsStorage::Float3> actorTrans;
		Btl::Vector<C_GameObjectSample> samples;

		const char* filename = "z:/mytest.objsample";
		Sys::I_File* pFile = Sys::C_FileSystem::Instance().Open(
			filename, 
			Sys::E_FileAccess_Read);
		if (!pFile)
			return -1;

		//Header
		{
			Sys::C_FileBlock headerBlock(pFile);
			{
				Arch::BinaryInputArchive ar(headerBlock);
				U32 version;
				ar >> version;
			}

			//Table describing actors.
			U32 nActors = 0;
			{
				Sys::C_FileBlock gameDescBlock(&headerBlock);
				if (!strcmp(gameDescBlock.GetBlockHeader().m_Id, "objdesc"))
				{
					Arch::BinaryInputArchive ar(gameDescBlock);
					ar >> nActors;
					myActorTable.Resize(nActors);
					actorQuats.Resize(nActors);
					actorTrans.Resize(nActors);
					for (U32 i = 0; i < nActors; ++i)
						myActorTable[i].Deserialise(ar);
				}
			}

			//Data per frame.
			{
				Sys::C_FileBlock gameObjectBlock(&headerBlock);
				if (!strcmp(gameObjectBlock.GetBlockHeader().m_Id, "frames"))
				{
					Arch::BinaryInputArchive ar(gameObjectBlock);
					U32 nFrames;
					ar >> nFrames;
					samples.Resize(nFrames);
					for (U32 i = 0; i < nFrames; ++i)
					{
						samples[i].pQuat = &actorQuats[i];
						samples[i].pPos = &actorTrans[i];
						samples[i].Deserialise(ar, nActors);
					}
				}
			}
		}
		SAFE_DELETE_PTR(pFile);
	}

	return 0;
}

bool C_Application::ParseCommandLineOptions(
	int argc, 
	char* argv[], 
	C_Options& options)
{
	return true;
}
