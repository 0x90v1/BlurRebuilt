#ifndef LOGICALFS_PAKFS_PAK_HEADER_H
#define LOGICALFS_PAKFS_PAK_HEADER_H

#ifdef _MSC_VER
#pragma once
#endif

namespace Pak
{

struct C_PreHeader
{
	U32 fourCc;
	U32 version;
	U32 sectorSize;
};

struct C_PakHeader
{
	C_PakHeader() 
	{
		codec = 0;
		fatEntryType = 0;
		codecFlags = 0;
		pathSeparator = 0;
		unused = 0;
		metaStrOffset = 0;
		fileTime = 0;
		memset(wholeFileIntegrityHash, 0, sizeof(wholeFileIntegrityHash));
	}

    template <typename ARCHIVE>  
    void Serialise(ARCHIVE& ar)  
    {  
		ar.Write(wholeFileIntegrityHash, sizeof(wholeFileIntegrityHash));
		ar << codec;
		ar << fatEntryType;
		ar << pathSeparator;
		ar << unused;
		ar << codecFlags;
		ar << metaStrOffset;
		ar << fileTime;
    }

    template <typename ARCHIVE>  
    void Deserialise(ARCHIVE& ar)  
    {  
		ar.Read(wholeFileIntegrityHash, sizeof(wholeFileIntegrityHash));
		ar >> codec;
		ar >> fatEntryType;
		ar >> pathSeparator;
		ar >> unused;
		ar >> codecFlags;
		ar >> metaStrOffset;
		ar >> fileTime;
    }

	U8				wholeFileIntegrityHash[20];
	U8				codec;
	U8				fatEntryType;
	char			pathSeparator;
	U8				unused;
	U32				codecFlags;
	U32				metaStrOffset;
	U64				fileTime;
};
}

#endif
