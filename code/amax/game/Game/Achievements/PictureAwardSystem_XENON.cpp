#include "stdafx.h"

#include <Game\Achievements\PictureAwardSystem.h>

#include "Xlast/Amax.spa.h"

namespace Game
{

C_PictureAwardSystem C_PictureAwardSystem::s_instance;

void C_PictureAwardSystem::Init()
{
	mPictureAwardMapping[BLUR_PICTURE_AWARD_GOLD]		=	GAMER_PICTURE_GOLDMEDAL;
	mPictureAwardMapping[BLUR_PICTURE_AWARD_HARD_GOLD]	=	GAMER_PICTURE_GOLDMEDAL2;
}

void C_PictureAwardSystem::Update()
{
	for(int a=0;a<mGivePictureAwardTasksStore.Size();)
	{
		if(XHasOverlappedIoCompleted(mGivePictureAwardTasksStore[a].mOverlapped))
		{
			CloseHandle(mGivePictureAwardTasksStore[a].mHandle);
			
			delete(mGivePictureAwardTasksStore[a].mOverlapped);
			
			mGivePictureAwardTasksStore.EraseByIndex(a);
		}
		else
		{
			a++;
		}
	}
}

void C_PictureAwardSystem::GivePictureAward(int user_index, BLUR_PICTURE_AWARD picture_award)
{
	PictureAwardOverlappedTracker tracker;

	tracker.mPicture=mPictureAwardMapping[picture_award];

	tracker.mOverlapped = new XOVERLAPPED;
    ZeroMemory( tracker.mOverlapped, sizeof( XOVERLAPPED ) );

	tracker.mHandle = CreateEvent( NULL, FALSE, FALSE, NULL );
    tracker.mOverlapped->hEvent = tracker.mHandle;
	
	DWORD error=XUserAwardGamerPicture(user_index,tracker.mPicture,0,tracker.mOverlapped);

	if(error != ERROR_IO_PENDING)
	{

	}

	mGivePictureAwardTasksStore.PushBack(tracker);
}

}; // namespace Game
