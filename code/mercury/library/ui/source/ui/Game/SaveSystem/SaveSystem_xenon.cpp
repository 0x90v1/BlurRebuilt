#include "stdafx.h"

#include "ui/Game/Profiles/ProfileInterface.h"
#include "ui/Game/SaveSystem/SaveSystem.h"

#include "ui/UiInterface.h"
#include "ui/GameSaveSystem.h"
#include "ui/Graphics/Localisation/loc.h"

#ifdef CFG_ALLOW_OPTIMIZE_OFF
#pragma optimize( "", off )
#endif

namespace UI
{

// -----------------
// C_AsyncLoadThread
// -----------------
void C_AsyncLoadThread::ThreadMain()
{
	C_SaveInfoInterface* pSaveInfoInterface = C_SaveSystem::Instance().SaveInfoInterface();

	if ( !pSaveInfoInterface )
		return;

	gGameSaveSystem.SetUserIndex( C_ProfileInterface::Instance().GetPadProfile( m_sPadNo ).m_UserIndex );
	gGameSaveSystem.SetSaveRoot( pSaveInfoInterface->m_SaveRoot.Chars() );

	if ( ( pSaveInfoInterface->m_eType == E_UiSaveInfoType_Enumerate ) && ( pSaveInfoInterface->m_sEnumeration == -1 ) )
	{
		// set the device id
		gGameSaveSystem.mDeviceID = C_ProfileInterface::Instance().DeviceId( m_sPadNo );

		bool bSuccess = gGameSaveSystem.Enumerate( false );
		
		if ( !bSuccess )
		{
			// failed for some reason - check the error code and close
			S32 sError = gGameSaveSystem.GetLastError();

			if ( sError == ERROR_DEVICE_NOT_CONNECTED )
				m_eErrorCode = E_UiGameProfile_Error_DeviceNotConnected;
			else
				m_eErrorCode = 	E_UiGameProfile_Error_CannotEnumerateFiles;
		}
	}
	else
	{
		S32 sNumFiles = ( pSaveInfoInterface->m_eType == E_UiSaveInfoType_Enumerate ) ? 1 : pSaveInfoInterface->m_FileInfo.Size();

		for ( S32 i = 0; i < sNumFiles; i++ )
		{
			bool bSuccess = false;

			if ( pSaveInfoInterface->m_eType == E_UiSaveInfoType_Enumerate )
				bSuccess = gGameSaveSystem.LoadSave( gGameSaveSystem.mContentData[ pSaveInfoInterface->m_sEnumeration ].szFileName, gGameSaveSystem.mContentData[ pSaveInfoInterface->m_sEnumeration ].szDisplayName );
			else
				bSuccess = gGameSaveSystem.LoadSave( pSaveInfoInterface->m_FileInfo[ i ].m_FileName.Chars(), pSaveInfoInterface->m_FileInfo[ i ].m_DisplayName );

			if ( !bSuccess )
			{
				// failed for some reason - check the error code and close
				S32 sError = gGameSaveSystem.GetLastError();

				if ( sError == ERROR_DEVICE_NOT_CONNECTED )
					m_eErrorCode = E_UiGameProfile_Error_DeviceNotConnected;
				else 
				{
					if ( sError == ERROR_FILE_CORRUPT )
						m_eErrorCode = E_UiGameProfile_Error_CannotReadFile;
					else
						m_eErrorCode = E_UiGameProfile_Error_FileDoesNotExist;
				}

				//gGameSaveSystem.CloseSave(); // PR - don't need this here
			}
			else
			{
				Sys::C_OpenParams params;
				params.m_IsCritical = false;

				Sys::I_File* pFile = Sys::C_FileSystem::Instance().Open(
					pSaveInfoInterface->m_FileInfo[ i ].m_InternalName.Chars(),
					Sys::E_FileAccess_Read, params );

				if ( !pFile )
				{
					// failed to open the file
					gGameSaveSystem.CloseSave();
					m_eErrorCode = E_UiGameProfile_Error_CannotReadFile;
				}
				else
				{
					const U32 uBufferSize = (U32)pFile->Length(); //pSaveInfoInterface->m_FileInfo[ i ].m_uFileSize;

					// read and process
					U8* pBuffer = new U8[ uBufferSize ];	// use fixed data size
					memset( pBuffer, 0xfd, uBufferSize );	// debug clear

					bool bReadSucceeded = pFile->Read( pBuffer, uBufferSize );

					if ( bReadSucceeded )
						bReadSucceeded = pSaveInfoInterface->Load( m_sPadNo, pBuffer, uBufferSize, pSaveInfoInterface->m_FileInfo[ i ].m_sUserData );	// load in

					SAFE_DELETE_PTR( pFile );
					SAFE_DELETE_ARY( pBuffer );

					gGameSaveSystem.CloseSave();

					if ( !bReadSucceeded )	// failed to read in the variables - out-of-date file
						m_eErrorCode = E_UiGameProfile_Error_CannotReadFile;
					else
						m_eErrorCode = E_UiGameProfile_Error_Okay;
				}
			}

			// check for an optional file
			if ( ( m_eErrorCode == E_UiGameProfile_Error_FileDoesNotExist ) && pSaveInfoInterface->m_FileInfo[ i ].m_bOptional )
				m_eErrorCode = E_UiGameProfile_Error_Okay;

			if ( m_eErrorCode != E_UiGameProfile_Error_Okay )
				break;
		}
	}

	// store away the device id
	C_ProfileInterface::Instance().DeviceId( m_sPadNo, gGameSaveSystem.mDeviceID );

	// do a last check to see if people are daft enough to pull the MU!
	if( XContentGetDeviceState( C_ProfileInterface::Instance().DeviceId( m_sPadNo ), NULL ) == ERROR_DEVICE_NOT_CONNECTED )
		m_eErrorCode = E_UiGameProfile_Error_DeviceNotConnected;

	pSaveInfoInterface->LoadFinished();

#ifndef CFG_GOLD
	// force error code
	if ( g_SaveSystemForceErrorCode != E_UiGameProfile_Error_None )
	{
		if ( ( g_SaveSystemForceErrorCode == E_UiGameProfile_Error_Okay )
			|| ( g_SaveSystemForceErrorCode == E_UiGameProfile_Error_FileDoesNotExist )
			|| ( g_SaveSystemForceErrorCode == E_UiGameProfile_Error_CannotReadFile )
			|| ( g_SaveSystemForceErrorCode == E_UiGameProfile_Error_CannotEnumerateFiles )
			|| ( g_SaveSystemForceErrorCode == E_UiGameProfile_Error_DeviceNotConnected ) )
		{
			m_eErrorCode = g_SaveSystemForceErrorCode;
		}
	}
#endif
}

// -----------------
// C_AsyncSaveThread
// -----------------
void C_AsyncSaveThread::ThreadMain()
{
	C_SaveInfoInterface* pSaveInfoInterface = C_SaveSystem::Instance().SaveInfoInterface();

	if ( !pSaveInfoInterface )
		return;

	gGameSaveSystem.SetUserIndex( C_ProfileInterface::Instance().GetPadProfile( m_sPadNo ).m_UserIndex );
	gGameSaveSystem.SetSaveRoot( pSaveInfoInterface->m_SaveRoot.Chars() );

	// set the device id
	gGameSaveSystem.mDeviceID = C_ProfileInterface::Instance().DeviceId( m_sPadNo );

	// check that our device is connected
	if( XContentGetDeviceState( C_ProfileInterface::Instance().DeviceId( m_sPadNo ), NULL ) == ERROR_DEVICE_NOT_CONNECTED )
		m_eErrorCode = E_UiGameProfile_Error_DeviceNotConnected;
	else
	{
		// check how much free space we have
		XDEVICE_DATA deviceData;
		DWORD ret = XContentGetDeviceData( C_ProfileInterface::Instance().DeviceId( m_sPadNo ), &deviceData );

		if ( ret != ERROR_SUCCESS )
			m_eErrorCode = E_UiGameProfile_Error_CannotWriteFile;
		else
		{
			// do we have enough free space?
			if ( deviceData.ulDeviceFreeBytes < (ULONGLONG)C_SaveSystem::Instance().GetCurrentSaveSize() )
				m_eErrorCode = E_UiGameProfile_Error_NoFreeSpace;
			else
			{
				// save out
				for ( S32 i = 0; i < pSaveInfoInterface->m_FileInfo.Size(); i++ )
				{
					S32 sError = gGameSaveSystem.CreateSave( pSaveInfoInterface->m_FileInfo[ i ].m_FileName.Chars(), pSaveInfoInterface->m_FileInfo[ i ].m_DisplayName, pSaveInfoInterface->m_bRoamable, pSaveInfoInterface->m_bCheckOverwrites );

					if ( sError != GAMESAVE_SYSTEM_OK )
					{
						// failed for some reason - check the error code and close
						if ( sError == GAMESAVE_SYSTEM_DEVICE_DISCONNECTED )
							m_eErrorCode = E_UiGameProfile_Error_DeviceNotConnected;
						if ( sError == GAMESAVE_SYSTEM_FILE_EXISTS )
							m_eErrorCode = E_UiGameProfile_Error_FileExists;
						else
							m_eErrorCode = E_UiGameProfile_Error_CannotWriteFile;

						if ( m_eErrorCode != E_UiGameProfile_Error_FileExists )
							gGameSaveSystem.CloseSave();
					}
					else
					{
						bool bWriteSucceeded = false;

						Sys::C_OpenParams params;
						params.m_IsCritical = false;

						Sys::I_File* pFile = Sys::C_FileSystem::Instance().Open( 
							pSaveInfoInterface->m_FileInfo[ i ].m_InternalName.Chars(), 
							Sys::E_FileAccess_Write, params );

						if ( !pFile )
						{
							// failed to open the file
							gGameSaveSystem.CloseSave();
							m_eErrorCode = E_UiGameProfile_Error_CannotWriteFile;
						}
						else
						{
							const U32 uBufferSize = pSaveInfoInterface->m_FileInfo[ i ].m_uFileSize;

							// process and write
							U8* pBuffer = new U8[ uBufferSize ];	// use fixed data size
							memset( pBuffer, 0xfe, uBufferSize );	// debug clear

							// save out
							if ( pSaveInfoInterface->Save( m_sPadNo, pBuffer, uBufferSize, pSaveInfoInterface->m_FileInfo[ i ].m_sUserData ) )
							{
								if ( !pFile->Write( pBuffer, uBufferSize ) )
									m_eErrorCode = E_UiGameProfile_Error_CannotWriteFile;

								pFile->Flush();	
							}
							else
								m_eErrorCode = E_UiGameProfile_Error_CannotWriteFile;			

							SAFE_DELETE_PTR( pFile );
							SAFE_DELETE_ARY( pBuffer );

							gGameSaveSystem.Flush();
							gGameSaveSystem.CloseSave();
						}
					}

					if ( m_eErrorCode != E_UiGameProfile_Error_Okay )
						break;
				}

				// do a last check to see if people are daft enough to pull the MU!
				if( XContentGetDeviceState( C_ProfileInterface::Instance().DeviceId( m_sPadNo ), NULL ) == ERROR_DEVICE_NOT_CONNECTED )
					m_eErrorCode = E_UiGameProfile_Error_DeviceNotConnected;
			}
		}
	}

	pSaveInfoInterface->SaveFinished();

#ifndef CFG_GOLD
	// force error code
	if ( g_SaveSystemForceErrorCode != E_UiGameProfile_Error_None )
	{
		if ( ( g_SaveSystemForceErrorCode == E_UiGameProfile_Error_Okay )
			|| ( g_SaveSystemForceErrorCode == E_UiGameProfile_Error_NoFreeSpace )
			|| ( g_SaveSystemForceErrorCode == E_UiGameProfile_Error_FileExists )
			|| ( g_SaveSystemForceErrorCode == E_UiGameProfile_Error_CannotWriteFile )
			|| ( g_SaveSystemForceErrorCode == E_UiGameProfile_Error_DeviceNotConnected ) )
		{
			m_eErrorCode = g_SaveSystemForceErrorCode;
		}
	}
#endif
}

// -------------------
// C_AsyncDeleteThread
// -------------------
void C_AsyncDeleteThread::ThreadMain()
{
	C_SaveInfoInterface* pSaveInfoInterface = C_SaveSystem::Instance().SaveInfoInterface();

	if ( !pSaveInfoInterface )
		return;

	for ( S32 i = 0; i < pSaveInfoInterface->m_FileInfo.Size(); i++ )
	{
		gGameSaveSystem.SetUserIndex( C_ProfileInterface::Instance().GetPadProfile( m_sPadNo ).m_UserIndex );
		gGameSaveSystem.SetSaveRoot( pSaveInfoInterface->m_SaveRoot.Chars() );

		// set the device id
		gGameSaveSystem.mDeviceID = C_ProfileInterface::Instance().DeviceId( m_sPadNo );

		if ( gGameSaveSystem.DeleteSave( pSaveInfoInterface->m_FileInfo[ i ].m_FileName.Chars(), pSaveInfoInterface->m_FileInfo[ i ].m_DisplayName ) )
			m_eErrorCode = E_UiGameProfile_Error_Okay;
		else
		{
			// check error code
			S32 sError = gGameSaveSystem.GetLastError();

			if ( sError != ERROR_FILE_NOT_FOUND )
				m_eErrorCode = E_UiGameProfile_Error_CannotDeleteFile;
		}

		//gGameSaveSystem.CloseSave(); // PR - don't need this here

		if ( m_eErrorCode != E_UiGameProfile_Error_Okay )
			break;
	}

	pSaveInfoInterface->DeleteFinished();

#ifndef CFG_GOLD
	// force error code
	if ( g_SaveSystemForceErrorCode != E_UiGameProfile_Error_None )
	{
		if ( ( g_SaveSystemForceErrorCode == E_UiGameProfile_Error_Okay )
			|| ( g_SaveSystemForceErrorCode == E_UiGameProfile_Error_CannotDeleteFile ) )
		{
			m_eErrorCode = g_SaveSystemForceErrorCode;
		}
	}
#endif
}

// ------------
// C_SaveSystem
// ------------
S32 C_SaveSystem::GetCurrentSaveSize()
{
	C_SaveInfoInterface* pSaveInfoInterface = C_SaveSystem::Instance().SaveInfoInterface();

	if ( !pSaveInfoInterface )
		return 0;

	ULONGLONG uSpaceRequired = 0;

	for ( S32 i = 0; i < pSaveInfoInterface->m_FileInfo.Size(); i++ )
		uSpaceRequired += XContentCalculateSize( (ULONGLONG)pSaveInfoInterface->m_FileInfo[ i ].m_uFileSize, /*directories*/ 0 );

	return (S32)uSpaceRequired;
}

}; // namespace UI

#ifdef CFG_ALLOW_OPTIMIZE_OFF
#pragma optimize( "", on )
#endif
