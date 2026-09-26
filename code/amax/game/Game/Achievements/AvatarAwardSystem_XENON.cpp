#include "stdafx.h"

#include <Game\Achievements\AvatarAwardSystem.h>

#include "Xlast/Amax.spa.h"

namespace Game
{

C_AvatarAwardSystem C_AvatarAwardSystem::s_instance;

void C_AvatarAwardSystem::Init()
{
	mAvatarAwardMapping[BLUR_AVATAR_AWARD_ALL_STICKERS_TEE]	=	AVATARASSETAWARD_AVATAR_ALLSTICKERSTEE;
	mAvatarAwardMapping[BLUR_AVATAR_AWARD_LEGEND_TEE]		=	AVATARASSETAWARD_AVATAR_LEGENDTEE;
	mAvatarAwardMapping[BLUR_AVATAR_AWARD_NINTH_BOSS_TEE]	=	AVATARASSETAWARD_AVATAR_NINTHBOSSTEE;
	mAvatarAwardMapping[BLUR_AVATAR_AWARD_RE_CHALLENGE_TEE]	=	AVATARASSETAWARD_AVATAR_RECHALLENGETEE;
	mAvatarAwardMapping[BLUR_AVATAR_AWARD_WRECK_TEE]		=	AVATARASSETAWARD_AVATAR_WRECKTEE;
}

void C_AvatarAwardSystem::Update()
{
	for(int a=0;a<mGiveAvatarAwardTasksStore.Size();)
	{
		if(XHasOverlappedIoCompleted(mGiveAvatarAwardTasksStore[a].mOverlapped))
		{
			CloseHandle(mGiveAvatarAwardTasksStore[a].mHandle);
			
			delete(mGiveAvatarAwardTasksStore[a].mOverlapped);
			delete(mGiveAvatarAwardTasksStore[a].xenon_avatar_asset);
			
			mGiveAvatarAwardTasksStore.EraseByIndex(a);
		}
		else
		{
			a++;
		}
	}
}

void C_AvatarAwardSystem::GiveAvatarAward(int user_index, BLUR_AVATAR_AWARD avatar_award)
{
	AvatarAwardOverlappedTracker tracker;

	tracker.xenon_avatar_asset = new XUSER_AVATARASSET;
	tracker.xenon_avatar_asset->dwUserIndex=user_index;
	tracker.xenon_avatar_asset->dwAwardId=mAvatarAwardMapping[avatar_award];

	tracker.mOverlapped = new XOVERLAPPED;
    ZeroMemory( tracker.mOverlapped, sizeof( XOVERLAPPED ) );

	tracker.mHandle = CreateEvent( NULL, FALSE, FALSE, NULL );
    tracker.mOverlapped->hEvent = tracker.mHandle;

	DWORD error=XUserAwardAvatarAssets(1,tracker.xenon_avatar_asset,tracker.mOverlapped);

	if(error != ERROR_IO_PENDING)
	{

	}

	mGiveAvatarAwardTasksStore.PushBack(tracker);
}

}; // namespace Game
