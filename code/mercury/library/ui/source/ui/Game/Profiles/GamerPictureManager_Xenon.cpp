
#include "stdafx.h"
#include "UI/Game/Profiles/GamerPictureManager_XENON.h"

#include <core/threads/Atomic.h>

namespace UI
{

const bool C_GamerPictureManager_Xenon::GetLocalPicture( const S32 sPadIndex )
{
	if ( sPadIndex >= m_LocalGamerPictures.Size() )
		return false;

	Display::C_LockedRect rect = m_LocalGamerPictures[sPadIndex]->m_Texture.Lock( 0 );

	if ( rect.GetBits() == NULL ) 
	{
		LOGERROR( g_Log, "Failed to lock rect" ); 
		return false;
	}

	bool small = m_LocalGamerPictures[sPadIndex]->m_Texture.GetHeight() < 64;

	Display::Device()->AcquireThreadOwnership(); // prevent TCR # 022 as following code can stall for a while
	Display::Device()->GetD3DDevice()->Suspend();

	DWORD dwErr = XUserReadGamerPicture( sPadIndex,				// User index
										 small,					// 32x32 or 64x64
										 rect.GetBits(),		// texture buffer
										 rect.GetPitch(),		// texture pitch
										 m_LocalGamerPictures[sPadIndex]->m_Texture.GetHeight(),	// texture height
										 0 );					// Pointer to XOVERLAPPED, or NULL to work synchronously

	Display::Device()->GetD3DDevice()->Resume();
	Display::Device()->ReleaseThreadOwnership();

	m_LocalGamerPictures[sPadIndex]->m_Texture.Unlock( 0 );
	m_LocalGamerPictures[sPadIndex]->m_Ref = sPadIndex;
	m_LocalGamerPictures[sPadIndex]->m_Free	= false;
	m_LocalGamerPictures[sPadIndex]->m_Active = ( dwErr == ERROR_SUCCESS );

	return ( dwErr == ERROR_SUCCESS );
}


const bool C_GamerPictureManager_Xenon::GetRemotePictures()
{
	if ( !m_RemoteGamerPictures.Size() || !m_RemoteJobList.Size() )
		return false;

	XUID uids[MAX_JOB_SIZE]; // max we can ever request;

	for ( S32 i = 0; i < m_RemoteJobList.Size(); i++ )
		uids[i] = m_RemoteJobList[i].user.m_Uid;

	// Need to request the settings size in the first call so we can allocate enough space to contain the data
	S32 settingsSize = 1;
	DWORD settingId = XPROFILE_GAMERCARD_PICTURE_KEY; 
	DWORD cbResults = 0;
	DWORD dwErr = 0;

	S32 sPadIndex = SysProfile::C_Manager::Instance().GetPrimaryPadNo();

	dwErr = XUserReadProfileSettingsByXuid(	0,							// The title id. Zero for current title
											sPadIndex,					// User index
											m_RemoteJobList.Size(),		// Number of XUID values in the array
											uids,						// Array of XUIDs
											settingsSize,				// Number of settings in the array
											&settingId,					// Array of settings to retrieve
											&cbResults,					// Size in bytes of the space needed to retrieve settings
											0,							// Pointer to results buffer. Null to request size
											0 );						// Pointer to XOVERLAPPED, or NULL to work synchronously

	BIZ_ASSERT( dwErr == ERROR_INSUFFICIENT_BUFFER );
	BIZ_ASSERT( cbResults );

	XUSER_READ_PROFILE_SETTING_RESULT* pResults = ( XUSER_READ_PROFILE_SETTING_RESULT* )( new BYTE[cbResults] );

	// Read the actual data now.
	dwErr = XUserReadProfileSettingsByXuid( 0,							// The title id. Zero for current title
											sPadIndex,					// User index
											m_RemoteJobList.Size(),		// Number of XUID values in the array
											uids, 						// Array of XUIDs
											settingsSize,				// Number of settings in the array
											&settingId,					// Array pf settings to retrieve
											&cbResults,					// Size in bytes of the space needed to retrieve settings
											pResults,					// Pointer to results buffer. Null to request size
											0 );						// Pointer to XOVERLAPPED, or NULL to work synchronously

	BIZ_ASSERT( dwErr != ERROR_INSUFFICIENT_BUFFER );

	// Incase we lost connection to live
	if ( dwErr == ERROR_SUCCESS )
	{
		for ( S32 i = 0; i < m_RemoteJobList.Size(); i++ )
		{
			const S32 picIndex = m_RemoteJobList[i].slot;

			Display::C_LockedRect rect = m_RemoteGamerPictures[picIndex]->m_Texture.Lock( 0 );

			if ( rect.GetBits() == NULL ) 
			{
				LOGERROR( g_Log, "Failed to lock rect" ); 
				continue;
			}

			dwErr = XUserReadGamerPictureByKey( &pResults->pSettings[i].data, 
												false, 
												rect.GetBits(), 
												rect.GetPitch(), 
												m_RemoteGamerPictures[picIndex]->m_Texture.GetHeight(), 
												0 );

			// Unlock texture and free up memory
			m_RemoteGamerPictures[picIndex]->m_Texture.Unlock( 0 );

			if ( dwErr == ERROR_SUCCESS )
			{
				MT::MemoryFence();

				m_RemoteGamerPictures[picIndex]->m_Active = true;
			}
		}
	}

	SAFE_DELETE_ARY( pResults );
	m_RemoteJobList.Clear();
	
	return ( dwErr == ERROR_SUCCESS ) ? true : false;
}

}; // namespace UI