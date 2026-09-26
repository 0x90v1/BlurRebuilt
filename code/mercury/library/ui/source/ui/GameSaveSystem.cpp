#include "stdafx.h"

#include "ui/GameSaveSystem.h"
#include "ui/Game/UIScreenControl.h"

#include "ui/Game/Gadgets/ButtonController.h"
#include "ui/Game/Gadgets/ButtonMessages.h"

#ifdef CFG_ALLOW_OPTIMIZE_OFF_UI
#pragma optimize("", off)
#endif

namespace UI
{

C_GameSaveSystem gGameSaveSystem;

void C_GameSaveSystem::Update()
{
#if defined(PLATFORM_XENON)
	if(mCheckforDeviceLocation)
	{
		if(XHasOverlappedIoCompleted(&mDeviceOverlapped))
		{
			if ( mDeviceID == XCONTENTDEVICE_ANY )
			{
				_UI_MESSAGE( E_UiMsg_SaveLocationNotFound );
			}
			else
			{
				_UI_MESSAGE( E_UiMsg_SaveLocationFound );
			}

			mCheckforDeviceLocation = false;
		}
	}

	if(mEnumeratingSaves)
	{
		if(XHasOverlappedIoCompleted(&mEnumerateOverlapped))
		{
			CloseHandle( mEnumerateHandle );

			mContentCount=(int)mEnumerateOverlapped.InternalHigh;

			LOGDEBUG( g_Log, "Got %d Save Games",mContentCount);
			for(U32 a=0;a<mContentCount;a++)
			{
				LOGDEBUG( g_Log, "%d: %S (%s)",a,mContentData[a].szDisplayName,mContentData[a].szFileName);
			}

			_UI_MESSAGE( E_UiMsg_SaveEnumerationFinished );

			mEnumeratingSaves = false;
		}
	}
#else
	if ( mCheckforDeviceLocation )
	{
		_UI_MESSAGE( E_UiMsg_SaveLocationNotFound );
		mCheckforDeviceLocation = false;
	}

	if ( mEnumeratingSaves )
	{
		_UI_MESSAGE( E_UiMsg_SaveEnumerationFinished );
		mEnumeratingSaves = false;
	}
#endif
}

void C_GameSaveSystem::StartXenonLocationUI( const S32 sDataSize, const S32 sDeviceSelectorFlag )
{
#if defined(PLATFORM_XENON)
	ULARGE_INTEGER dataSize;
	dataSize.HighPart = 0;
	dataSize.LowPart = ( sDataSize == -1 ) ? 1024 * 500 : sDataSize;

	DWORD contentFlag = ( sDeviceSelectorFlag == -1 ) ? XCONTENTFLAG_NONE: (DWORD)sDeviceSelectorFlag;

	memset( &mDeviceOverlapped, 0, sizeof( XOVERLAPPED ) );

	mDeviceID = XCONTENTDEVICE_ANY;
	DWORD error = XShowDeviceSelectorUI( mUserIndex, XCONTENTTYPE_SAVEDGAME, contentFlag, dataSize, &mDeviceID, &mDeviceOverlapped );

	mCheckforDeviceLocation = true;
#endif
}

S32 C_GameSaveSystem::CreateSave(const char *filename, const WCHAR *display_name, const bool roamable, const bool check_overwrites)
{
#if defined(PLATFORM_XENON)
	//assert(mOpen == false);

	ZeroMemory( &mSaveContentData, sizeof(XCONTENT_DATA) );
	wcsncpy(mSaveContentData.szDisplayName, display_name, 128);
	lstrcpy(mSaveContentData.szFileName, filename);
	mSaveContentData.dwContentType = XCONTENTTYPE_SAVEDGAME;
	mSaveContentData.DeviceID = mDeviceID;

	DWORD flags;
	if(check_overwrites)
	{
		flags=XCONTENTFLAG_CREATENEW;
	}
	else
	{
		flags=XCONTENTFLAG_CREATEALWAYS;
	}

	if(!roamable)
	{
		flags|=XCONTENTFLAG_NOPROFILE_TRANSFER;
	}
	else
	{
		flags|=XCONTENTFLAG_ALLOWPROFILE_TRANSFER;
	}

	mLastError=XContentCreate( mUserIndex, mSaveRoot, &mSaveContentData,flags, NULL, NULL, NULL);

	if(check_overwrites && mLastError == ERROR_ALREADY_EXISTS)
	{
		return GAMESAVE_SYSTEM_FILE_EXISTS;
	}
    
	if(mLastError != ERROR_SUCCESS && mLastError != ERROR_ALREADY_EXISTS)
	{
		if(mLastError == ERROR_NO_SUCH_USER)
		{
			LOGDEBUG( g_Log, "Failed to save profile because you aren't logged in");
		}
		else if(mLastError == ERROR_DEVICE_NOT_CONNECTED)
		{
			LOGDEBUG( g_Log, "Failed to save profile because device was disconnected");
			return GAMESAVE_SYSTEM_DEVICE_DISCONNECTED;
		}
		else
		{
			LOGDEBUG( g_Log, "Failed to save profile because of error %d",mLastError);
		}
		return GAMESAVE_SYSTEM_ERROR;
	}
#endif
	mOpen = true;

	return GAMESAVE_SYSTEM_OK;
}

bool C_GameSaveSystem::LoadSave(const char *filename, const WCHAR *display_name)
{
#if defined(PLATFORM_XENON)
	//assert(mOpen == false);

	ZeroMemory( &mSaveContentData, sizeof(XCONTENT_DATA) );
	wcsncpy(mSaveContentData.szDisplayName, display_name, 128);
	lstrcpy(mSaveContentData.szFileName, filename);
	mSaveContentData.dwContentType = XCONTENTTYPE_SAVEDGAME;
	mSaveContentData.DeviceID = mDeviceID;

	mLastError=XContentCreate( mUserIndex, mSaveRoot, &mSaveContentData, XCONTENTFLAG_OPENEXISTING, NULL, NULL, NULL);
   
	if(mLastError == ERROR_ALREADY_EXISTS)		//it already exists so we're fine
	{
		mOpen = true;
		return true;
	}
	else if(mLastError != ERROR_SUCCESS)		//there must be an error
	{
		if(mLastError == ERROR_NO_SUCH_USER)
		{
			LOGDEBUG( g_Log,"Failed to load profile because you aren't logged in");
		}
		else if ( mLastError == ERROR_PATH_NOT_FOUND )
		{
			LOGDEBUG( g_Log,"Failed to load profile - path does not exist" );
		}
		else if ( mLastError == ERROR_FILE_NOT_FOUND )
		{
			LOGDEBUG( g_Log,"Failed to load profile - file does not exist" );
		}
		else
		{
			LOGDEBUG( g_Log,"Failed to load profile because of error %d",mLastError);
		}
		return false;
	}
#endif
	//must be ERROR_SUCCESS then (a new save)
	mOpen = true;
	return true;
}

bool C_GameSaveSystem::DeleteSave(const char *filename, const WCHAR *display_name)
{
#if defined(PLATFORM_XENON)
	XCONTENT_DATA con;

	ZeroMemory( &con, sizeof(XCONTENT_DATA) );
	wcsncpy(con.szDisplayName, display_name, 128);
	lstrcpy(con.szFileName, filename);
	con.dwContentType = XCONTENTTYPE_SAVEDGAME;
	con.DeviceID = mDeviceID;

	mLastError=XContentDelete(mUserIndex,&con,NULL);

	if(mLastError != ERROR_SUCCESS)		//there must be an error
	{
		LOGDEBUG( g_Log,"Failed to delete profile because of error", mLastError);
		return false;
	}
#endif
	return true;
}

bool C_GameSaveSystem::CloseSave()
{
#if defined(PLATFORM_XENON)
	//assert(mOpen == true);
	//not asserting anymore as there could have been a problem i.e. pulling an MU
	
	mLastError = XContentClose(mSaveRoot, NULL);
	
	if (mLastError != ERROR_SUCCESS)		//there must be an error
	{
		LOGWARN(g_Log, "Failed to close save because of error %d", mLastError);
		return false;
	}
#endif

	mOpen = false;
	return true;
}


bool C_GameSaveSystem::Enumerate( const bool bAsync )
{
#if defined(PLATFORM_XENON)
	DWORD cbBuffer;

	DWORD dwRet;

	memset(&mContentData,0,sizeof(XCONTENT_DATA)*MAX_SAVE_ENUMERATIONS);
	memset(&mEnumerateOverlapped,0,sizeof(XOVERLAPPED));

	dwRet = XContentCreateEnumerator( mUserIndex,mDeviceID,XCONTENTTYPE_SAVEDGAME,0,MAX_SAVE_ENUMERATIONS,&cbBuffer,&mEnumerateHandle);

	if( dwRet != ERROR_SUCCESS )
	{
		LOGERROR(g_Log,"Could not Create Content Enumerator");
		return false;
	}

	assert( cbBuffer == (sizeof( XCONTENT_DATA )*MAX_SAVE_ENUMERATIONS) );

	if ( bAsync )
	{
		mLastError = XEnumerate( mEnumerateHandle, mContentData, sizeof( mContentData ), NULL, &mEnumerateOverlapped );

		if ( mLastError != ERROR_IO_PENDING )
		{
			LOGERROR( g_Log, "Could not enumerate save games, call to XEnumerate failed" );
			return false;
		}

		mEnumeratingSaves = true;
	}
	else
	{
		mLastError = XEnumerate( mEnumerateHandle, mContentData, sizeof( mContentData ), &mContentCount, NULL );

		if ( mLastError != ERROR_SUCCESS && mLastError != ERROR_NO_MORE_FILES )
		{
			LOGERROR( g_Log, "Could not enumerate save games, call to XEnumerate failed" );
			return false;
		}

		CloseHandle( mEnumerateHandle );

		_UI_MESSAGE( E_UiMsg_SaveEnumerationFinished );
	}
	return true;
#endif
	return false;
}

bool C_GameSaveSystem::Flush()
{
#if defined(PLATFORM_XENON)
	DWORD err = XContentFlush(mSaveRoot,NULL);

	if (err != ERROR_SUCCESS)
		return false;

	return true;
#endif
	return true;
}

ULONGLONG C_GameSaveSystem::SpaceLeftOnCurrentDevice(int *error)
{
	ULONGLONG bytes=0;
#if defined(PLATFORM_XENON)
	XDEVICE_DATA data;
	DWORD err=XContentGetDeviceData(mDeviceID,&data);

	if(err == ERROR_SUCCESS)
	{
		bytes=data.ulDeviceFreeBytes;
		if(error)
			*error=err;
	}
	else
	{
		LOGERROR(g_Log,"DeviceID %d has Error %d",mDeviceID,err);
		//assert(!"Could not find how much space is left on the device");
		if(error)
			*error=err;
	}

#endif
	return bytes;
}

}; // namespace UI

#ifdef CFG_ALLOW_OPTIMIZE_OFF_UI
#pragma optimize("", on) 
#endif
