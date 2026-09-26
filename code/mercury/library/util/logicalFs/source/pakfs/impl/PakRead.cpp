#include "LogicalFsPch.h"

#include <System/Serialise/BinarySerialiseAutoEndian.h>
#include <System/Serialise/C_MemoryStream.h>

#include <Misc_Shared/Uid/FourCc.h>
#include <Misc_Shared/DataStructure/StringTable.h>

#include <PakFs/Impl/PakDefinitions.h>
#include <PakFs/Impl/PakFile.h>
#include <PakFs/Impl/PakFatEntry.h>
#include <PakFs/Impl/PakIoMediator.h>
#include <PakFs/Impl/PakSha1.h>
#include <PakFs/Impl/PakUtil.h>
#include <PakFs/Impl/PakRead.h>

namespace Pak
{

bool ReadPreHeader(C_PakFile& pak, C_PreHeader& preHeader, Sys::C_StreamPos& versionPos, Sys::C_StreamPos* pPhysicalPos)
{
	if (!pak.pMediator->Read(preHeader.fourCc, pPhysicalPos)) return false;
	versionPos = *pPhysicalPos;
	if (!pak.pMediator->Read(preHeader.version, pPhysicalPos)) return false;
	if (!pak.pMediator->Read(preHeader.sectorSize, pPhysicalPos)) return false;

	 //PAK_FOURCC_U32;
	if (preHeader.fourCc != PAK_FOURCC_U32)
	{
		Ut::DoEndianSwap::EndianSwap(preHeader.fourCc);
		if (preHeader.fourCc == PAK_FOURCC_U32)
		{
			Ut::DoEndianSwap::EndianSwap(preHeader.version);
			Ut::DoEndianSwap::EndianSwap(preHeader.sectorSize);
			pak.pMediator->SetEndianSwapFlag(true);
		}
		else
		{
			LOGERROR(g_log, "Failed to read pak. Given file is not a pak!");
			return false;
		}
	}

	pak.pMediator->SetSectorSize(preHeader.sectorSize);
	return true;
}

bool ReadHeader(C_PakFile& pak, Sys::C_StreamPos& headerStartPos, Sys::C_StreamPos* pPhysicalPos)
{
	U32 headerSize;
	if (!pak.pMediator->Read(headerSize, pPhysicalPos)) return false;
	if (headerSize != sizeof(C_PakHeader))
	{
		LOGERROR(g_log, "Pak header size mismatch!");
		return false;
	}

	headerStartPos = *pPhysicalPos;
	if (!pak.pMediator->Read(&pak.header, headerSize, pPhysicalPos)) return false;

	//In place endian fixup.
	if (pak.pMediator->GetEndianSwapFlag())
	{
		Sys::C_MemoryStream memStream((U8*)&pak.header, headerSize);
		memStream.SetEndianSwapFlag(pak.pMediator->GetEndianSwapFlag());
		Sys::BinaryDeserialiserAutoEndian ar(memStream);
		ar >> pak.header;
	}

	return true;
}

bool ReadStringTable(C_PakFile& pak, Sys::C_StreamPos* pPhysicalPos)
{
	U8 sectorBuf[PAK_SECTOR_SIZE_MAX];
	//String table.
	{
		if (!pak.pMediator->Read(pak.stringTable.dataSize, pPhysicalPos)) return false;
		pak.stringTable.pData = new U8[pak.stringTable.dataSize];
		if (!pak.pMediator->Read(pak.stringTable.pData, pak.stringTable.dataSize, pPhysicalPos)) return false;
		PakCryptDecrypt(pak.stringTable.pData, pak.stringTable.dataSize);
		U32 padding = pak.pMediator->GetSectorSize() - (*pPhysicalPos % pak.pMediator->GetSectorSize());
		//Consume sector padding.
		if (!pak.pMediator->Read(sectorBuf, padding, pPhysicalPos)) return false;
	}

	return true;
}

bool ReadFat(C_PakFile& pak, Sys::C_StreamPos& fatStartPos, Sys::C_StreamPos* pPhysicalPos)
{
	pak.pMediator->Read(pak.nFatEntries, pPhysicalPos);
	U32 filePakFatSize;
	pak.pMediator->Read(filePakFatSize, pPhysicalPos);

	U32 fatEntrySize = GetFatEntrySize((E_FatEntryType)pak.header.fatEntryType);
	U32 fatSize = fatEntrySize * pak.nFatEntries;
	if (filePakFatSize != fatSize)
	{
		LOGERROR(g_log, "FAT size mismatch!");
		return false;
	}

	pak.pFatEntries = new U8[filePakFatSize];
	fatStartPos = *pPhysicalPos;
	if (!pak.pMediator->Read(pak.pFatEntries, filePakFatSize, pPhysicalPos)) return false;

	//In place endian fixup.
	if (pak.pMediator->GetEndianSwapFlag())
	{
		Sys::C_MemoryStream memStream((U8*)pak.pFatEntries, filePakFatSize);
		memStream.SetEndianSwapFlag(pak.pMediator->GetEndianSwapFlag());
		Sys::BinaryDeserialiserAutoEndian ar(memStream);

		//Perform fixup for appropriate derived structure type.
		if (pak.header.fatEntryType == E_FatEntryType_Normal)
			for (U32 i = 0; i < pak.nFatEntries; ++i)
			{
				C_PakFatEntry& fatEntry = ((C_PakFatEntry*)pak.pFatEntries)[i];
				ar >> fatEntry;
			}
		else if (pak.header.fatEntryType == E_FatEntryType_Secure)
			for (U32 i = 0; i < pak.nFatEntries; ++i)
			{
				C_PakFatEntrySecure& fatEntry = ((C_PakFatEntrySecure*)pak.pFatEntries)[i];
				ar >> fatEntry;
			}
		else
		{
			LOGERROR(g_log, "Unknown FAT entry type!");
			return false;
		}
	}

	return true;
}

}
