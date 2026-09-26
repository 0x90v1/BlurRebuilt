#include "LogicalFsPch.h"

#include <PakFs/Impl/PakDefinitions.h>
#include <PakFs/Impl/IPakCompress.h>
#include <PakFs/Impl/PakIoMediator.h>
#include <PakFs/Impl/PakCompressRaw.h>
#include <PakFs/Impl/PakSha1.h>

namespace Pak
{

C_PakCompressRaw::C_PakCompressRaw(U32 flags)
{
	m_pMediator = 0;
	m_physicalStartPos = 0;
	m_physicalReadPos = 0;
	m_fileAccessAttributes = 0;
	//Physical size of compressed data.
	m_dataPhysicalLength = 0;
}

C_PakCompressRaw::~C_PakCompressRaw()
{
}

bool C_PakCompressRaw::Open(
	C_PakIoMediator* pMediator, 
	Sys::C_StreamPos physicalStartPos, 
	const U32 fileAccessAttributes,
	Sha1::C_HashContext* pHashContext)
{
	m_physicalStartPos = physicalStartPos;
	m_physicalReadPos = m_physicalStartPos;
	m_pMediator = pMediator;
	m_fileAccessAttributes = fileAccessAttributes;

	m_pHashContext = pHashContext;

	switch(m_fileAccessAttributes)
	{
		case Sys::E_FileAccess_Read:
			return m_pMediator->Read(m_dataPhysicalLength, &m_physicalReadPos);
#ifdef CFG_CONVERTER
		case Sys::E_FileAccess_Write:
			//Write space for physical length
			return m_pMediator->Write(m_dataPhysicalLength);
#endif
		default:
			break;
	}
		
	LOGERROR(g_log, "Failed to open uncompressed pak!");
	return false;
}

bool C_PakCompressRaw::Restart(U32 streamToLogicalPos)
{
	if (streamToLogicalPos >= m_dataPhysicalLength)
	{
		BIZ_ASSERT_FAIL("Invalid logical pos!");
		return false;
	}

	m_physicalReadPos = m_physicalStartPos+sizeof(U32)+streamToLogicalPos;
	return true;
}

U32 C_PakCompressRaw::GetPhysicalLength() const
{
	return m_dataPhysicalLength;
}

bool C_PakCompressRaw::Close()
{
	switch(m_fileAccessAttributes)
	{
		case Sys::E_FileAccess_Read:
			return true;
#ifdef CFG_CONVERTER
		case Sys::E_FileAccess_Write:
			U32 logicalPos = GetLogicalPos(m_pMediator->GetWritePosition());
			//The U32 term below is the physical length.
			U32 sectorSize = m_pMediator->GetSectorSize();
			U32 nrToWrite = sectorSize - ((logicalPos+sizeof(U32)) % sectorSize);
			U8 zeroBuffer[PAK_SECTOR_SIZE_MAX] = {0};
			m_pMediator->Write((U8*)zeroBuffer, nrToWrite);
			m_dataPhysicalLength = logicalPos;
			//Write physical length.
			{
				Sys::C_StreamPos pos = m_pMediator->GetWritePosition();
				m_pMediator->SetWritePosition(m_physicalStartPos);
				m_pMediator->Write(m_dataPhysicalLength);
				m_pMediator->SetWritePosition(pos);
			}
			return true;
#endif
	}

	LOGERROR(g_log, "Failed to close uncompressed pak!");
	return false;
}

#ifdef CFG_CONVERTER

bool C_PakCompressRaw::Write(U8* buffer, U32 nrBytes)
{
	if (m_pHashContext)
		m_pHashContext->UpdateHash(buffer, nrBytes);

	return m_pMediator->Write(buffer, nrBytes);
}

#endif

bool C_PakCompressRaw::Read(U8* buffer, U32 nrBytes)
{
	U32 logicalPos = GetLogicalPos(m_physicalReadPos);
	if (logicalPos + nrBytes >= m_dataPhysicalLength)
		//Read remainder of file.
		nrBytes = m_dataPhysicalLength-logicalPos;
	
	bool result = m_pMediator->Read(buffer, nrBytes, &m_physicalReadPos);

	if (result && m_pHashContext)
		m_pHashContext->UpdateHash(buffer, nrBytes);

	return result;
}

U32 C_PakCompressRaw::GetLogicalPos(Sys::C_StreamPos physicalPos) const
{
	return (U32)(physicalPos - m_physicalStartPos)-sizeof(U32);
}


}
