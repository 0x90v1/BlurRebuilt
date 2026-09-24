#include "stdafx.h"

#include <Game\Achievements\AchievementSystem.h>

#include <Gamelogic\SocialNetworking\SocialNetworkManager.h>
#include <ui/Game/Profiles/ProfileInterface.h>
#include <Game\Messaging\GameflowMessages.h>

#include "Xlast/Amax.spa.h"

namespace Game
{

C_AchievementSystem C_AchievementSystem::s_instance;

void C_AchievementSystem::Init()
{
	//not the prettiest bit of code, but it gets a job done
	mAchievementMapping[BLUR_ACHIEVEMENT_ALL_ROUNDER]			=	ACHIEVEMENT_ACHIEVEMENT_ALLROUNDER;
	mAchievementMapping[BLUR_ACHIEVEMENT_BARGE_MASTER]			=	ACHIEVEMENT_ACHIEVEMENT_BARGEMASTER;
	mAchievementMapping[BLUR_ACHIEVEMENT_BEAT_BIZARRE]			=	ACHIEVEMENT_ACHIEVEMENT_BEATBIZARRE;
	mAchievementMapping[BLUR_ACHIEVEMENT_BETTER_ENEMIES]		=	ACHIEVEMENT_ACHIEVEMENT_FRIENDDEMANDS;	
	mAchievementMapping[BLUR_ACHIEVEMENT_BLUR_PARTY]			=	ACHIEVEMENT_ACHIEVEMENT_SPLITSCREEN;
	mAchievementMapping[BLUR_ACHIEVEMENT_BOLT_MASTER]			=	ACHIEVEMENT_ACHIEVEMENT_BOLTMASTER;
	mAchievementMapping[BLUR_ACHIEVEMENT_BREAK_YOU]				=	ACHIEVEMENT_ACHIEVEMENT_WRECKBOSS;		
	mAchievementMapping[BLUR_ACHIEVEMENT_CHALLENGE_MASTER]		=	ACHIEVEMENT_ACHIEVEMENT_LEADERBOARDCHALLENGE;
	mAchievementMapping[BLUR_ACHIEVEMENT_CHALLENGER]			=	ACHIEVEMENT_ACHIEVEMENT_MPCHALLENGES;
	mAchievementMapping[BLUR_ACHIEVEMENT_CHECKPOINT_MASTER]		=	ACHIEVEMENT_ACHIEVEMENT_CHECKPOINT;
	mAchievementMapping[BLUR_ACHIEVEMENT_COMPLETIST]			=	ACHIEVEMENT_ACHIEVEMENT_COMPLETIST;
	mAchievementMapping[BLUR_ACHIEVEMENT_CONE_GATES]			=	ACHIEVEMENT_ACHIEVEMENT_FIRSTFANRUN;		
	mAchievementMapping[BLUR_ACHIEVEMENT_DOMINATOR]				=	ACHIEVEMENT_ACHIEVEMENT_MPAWARDS;
	mAchievementMapping[BLUR_ACHIEVEMENT_FAN_BOY]				=	ACHIEVEMENT_ACHIEVEMENT_BEATRHYMER;
	mAchievementMapping[BLUR_ACHIEVEMENT_FAN_DEMAND_MASTER]		=	ACHIEVEMENT_ACHIEVEMENT_FANDEMANDMASTER;
	mAchievementMapping[BLUR_ACHIEVEMENT_FIRST_TEST]			=	ACHIEVEMENT_ACHIEVEMENT_UNLOCKFIRSTBOSS;
	mAchievementMapping[BLUR_ACHIEVEMENT_FOR_THE_FANS]			=	ACHIEVEMENT_ACHIEVEMENT_FANRUNS;
	mAchievementMapping[BLUR_ACHIEVEMENT_PHOTO_FINISH]			=	ACHIEVEMENT_ACHIEVEMENT_PHOTOFINISH;
	mAchievementMapping[BLUR_ACHIEVEMENT_GETTING_STARTED] 		=	ACHIEVEMENT_ACHIEVEMENT_FIRSTWRECK; 
	mAchievementMapping[BLUR_ACHIEVEMENT_GIANT_KILLER]			=	ACHIEVEMENT_ACHIEVEMENT_GIANTKILLER;
	mAchievementMapping[BLUR_ACHIEVEMENT_GLOBETROTTER]			=	ACHIEVEMENT_ACHIEVEMENT_GLOBETROTTER;
	mAchievementMapping[BLUR_ACHIEVEMENT_GOING_GREY]			=	ACHIEVEMENT_ACHIEVEMENT_BEATCARVER;
	mAchievementMapping[BLUR_ACHIEVEMENT_LEARNING_THE_ROPES]	=	ACHIEVEMENT_ACHIEVEMENT_BEATSHANNON;
	mAchievementMapping[BLUR_ACHIEVEMENT_LIGHTS_OUT_MEATBALL]	=	ACHIEVEMENT_ACHIEVEMENT_HUMILIATEBOSSES;
	mAchievementMapping[BLUR_ACHIEVEMENT_MASTER_OF_THE_MACHINE]	=	ACHIEVEMENT_ACHIEVEMENT_GOLDHARUNA;
	mAchievementMapping[BLUR_ACHIEVEMENT_MEET_THE_MITCHELLS]	=	ACHIEVEMENT_ACHIEVEMENT_BEATGAMEHARD;
	mAchievementMapping[BLUR_ACHIEVEMENT_MINE_MASTER]			=	ACHIEVEMENT_ACHIEVEMENT_MINEMASTER;
	mAchievementMapping[BLUR_ACHIEVEMENT_MODDER]				=	ACHIEVEMENT_ACHIEVEMENT_MODDER;
	mAchievementMapping[BLUR_ACHIEVEMENT_MOTOR_MAN]				=	ACHIEVEMENT_ACHIEVEMENT_MOTORMASH;
	mAchievementMapping[BLUR_ACHIEVEMENT_NITRO_MASTER]			=	ACHIEVEMENT_ACHIEVEMENT_NITROMASTER;
	mAchievementMapping[BLUR_ACHIEVEMENT_NOT_SO_PRETTY_BOY]		=	ACHIEVEMENT_ACHIEVEMENT_BEATKHAN;
	mAchievementMapping[BLUR_ACHIEVEMENT_PAINT_JOB]				=	ACHIEVEMENT_ACHIEVEMENT_PAINTJOB;
	mAchievementMapping[BLUR_ACHIEVEMENT_RACE_MASTER]			=	ACHIEVEMENT_ACHIEVEMENT_RACE;
	mAchievementMapping[BLUR_ACHIEVEMENT_RANK_FIFTY]			=	ACHIEVEMENT_ACHIEVEMENT_RANK50;
	mAchievementMapping[BLUR_ACHIEVEMENT_REPAIR_MASTER]			=	ACHIEVEMENT_ACHIEVEMENT_REPAIRMASTER;
	mAchievementMapping[BLUR_ACHIEVEMENT_SHIELD_MASTER]			=	ACHIEVEMENT_ACHIEVEMENT_SHIELDMASTER;
	mAchievementMapping[BLUR_ACHIEVEMENT_SHOCK_MASTER]			=	ACHIEVEMENT_ACHIEVEMENT_SHOCKMASTER;
	mAchievementMapping[BLUR_ACHIEVEMENT_SHOW_WHOS_BOSS]		=	ACHIEVEMENT_ACHIEVEMENT_BOSSDEMANDS;	
	mAchievementMapping[BLUR_ACHIEVEMENT_SHUNT_MASTER]			=	ACHIEVEMENT_ACHIEVEMENT_SHUNTMASTER;
	mAchievementMapping[BLUR_ACHIEVEMENT_SKILLS]				=	ACHIEVEMENT_ACHIEVEMENT_FIRSTEVADE;
	mAchievementMapping[BLUR_ACHIEVEMENT_SOCIALISER]			=	ACHIEVEMENT_ACHIEVEMENT_SOCIALISER;
	mAchievementMapping[BLUR_ACHIEVEMENT_SPEED_DEMON]			=	ACHIEVEMENT_ACHIEVEMENT_SPEEDDEMON;
	mAchievementMapping[BLUR_ACHIEVEMENT_SWING_OF_THINGS]		=	ACHIEVEMENT_ACHIEVEMENT_FIRSTFANPAR;	
	mAchievementMapping[BLUR_ACHIEVEMENT_SYBERIAN_EXPRESS]		=	ACHIEVEMENT_ACHIEVEMENT_BEATDRAKE;
	mAchievementMapping[BLUR_ACHIEVEMENT_TALONTED]				=	ACHIEVEMENT_ACHIEVEMENT_BEATTALON;		
	mAchievementMapping[BLUR_ACHIEVEMENT_THE_REAL_HUSTLE]		=	ACHIEVEMENT_ACHIEVEMENT_BEATSHANNONFINAL;
	mAchievementMapping[BLUR_ACHIEVEMENT_THORNY_ISSUE]			=	ACHIEVEMENT_ACHIEVEMENT_BEATNATALYA;
	mAchievementMapping[BLUR_ACHIEVEMENT_TOURIST]				=	ACHIEVEMENT_ACHIEVEMENT_TOURIST;
	mAchievementMapping[BLUR_ACHIEVEMENT_TWIN_TROUBLE]			=	ACHIEVEMENT_ACHIEVEMENT_BEATTWINS;
	mAchievementMapping[BLUR_ACHIEVEMENT_WRECKING_MACHINE]		=	ACHIEVEMENT_ACHIEVEMENT_DESTRUCTION;

	mhSchedulerGroup = Scheduler::CreateGroup();
}																	

void C_AchievementSystem::InitialiseSubscription( Messaging::C_MessageSubscriberSystem* pSubscriberSystem )
{
}

bool C_AchievementSystem::MessageHandler(const  Messaging::C_MessageBase* pMessage)
{
	return false;
}

WCHAR *C_AchievementSystem::GetAchievementName(BLUR_ACHIEVEMENT achievement)
{
	if (mProcessingTask)
		return L"";

	XACHIEVEMENT_DETAILS* achievements = ( XACHIEVEMENT_DETAILS* )mAchievementDetails;

	for(int a=0;a<BLUR_ACHIEVEMENT_MAX;a++)
	{
		if(achievements[a].dwId == mAchievementMapping[achievement])
		{
			return achievements[a].pwszLabel;
		}
	}

	return L"";
}

U32 C_AchievementSystem::GetAchievementScore(BLUR_ACHIEVEMENT achievement)
{
	if (mProcessingTask)
		return 0;

	XACHIEVEMENT_DETAILS* achievements = ( XACHIEVEMENT_DETAILS* )mAchievementDetails;

	for(int a=0;a<BLUR_ACHIEVEMENT_MAX;a++)
	{
		if(achievements[a].dwId == mAchievementMapping[achievement])
		{
			return achievements[a].dwCred;
		}
	}

	return 0;
}

void DeferredGetAchievementDetails(Scheduler::Task* pTask)
{
	PERF_EVENT("GetAchievementDetailsTask");
	C_AchievementSystem* pSystem = *(C_AchievementSystem**)pTask->ParamData;
	pSystem->GetAchievementDetailsDeferred();
}

JQ_DECLARE_PPU_MODULE(GetAchievementDetailsTask, DeferredGetAchievementDetails);

void C_AchievementSystem::GetAchievementDetailsDeferred()
{
	HANDLE hEnum;
	DWORD buffer_size;
    DWORD items;

	HRESULT hr = XUserCreateAchievementEnumerator(0, mUserIndex, INVALID_XUID, 
		XACHIEVEMENT_DETAILS_LABEL | XACHIEVEMENT_DETAILS_UNACHIEVED, 0,
		BLUR_ACHIEVEMENT_MAX,&buffer_size,&hEnum);

	if (hr == ERROR_SUCCESS)
	{
		if (XEnumerate( hEnum, mAchievementDetails, XACHIEVEMENT_SIZE_FULL * BLUR_ACHIEVEMENT_MAX, &items, NULL) != ERROR_SUCCESS)
		{
			BIZ_ASSERT_FAIL("Unable to get Achievement Details");
		}
	}

	CloseHandle( hEnum );

	mProcessingTask = false;
}

void C_AchievementSystem::GetAchievementDetails(int user_index)
{
	if (mProcessingTask)
		return;

	mProcessingTask = true;

	mUserIndex = user_index;

	BIZ_ASSERT(mhSchedulerGroup != 0); // make sure you set the scheduler group before running the update
	C_AchievementSystem* pSystem = this;
	Scheduler::AddTask(GetAchievementDetailsTask, 0, 0, mhSchedulerGroup, 0, &pSystem, sizeof(pSystem));
}

void C_AchievementSystem::Update()
{
	if (mProcessingTask)
		return;

	for(int a=0;a<mGiveAchievementTasksStore.Size();)
	{
		if(XHasOverlappedIoCompleted(mGiveAchievementTasksStore[a].mOverlapped))
		{
			if(mGiveAchievementTasksStore[a].mOverlapped->dwExtendedError == 0)
			{
				C_SocialNetworkManager::Instance().AddOneOff(AMAX_SOCIAL_NETWORKING_ITEM_TYPE_ACHIEVEMENT, mGiveAchievementTasksStore[a].xenon_achievement->dwAchievementId);
			}

			CloseHandle(mGiveAchievementTasksStore[a].mHandle);
			
			delete(mGiveAchievementTasksStore[a].mOverlapped);
			delete(mGiveAchievementTasksStore[a].xenon_achievement);
			
			mGiveAchievementTasksStore.EraseByIndex(a);
		}
		else
		{
			a++;
		}
	}
}

bool C_AchievementSystem::HasAchievement(int user_index,BLUR_ACHIEVEMENT achievement)
{
	if (mProcessingTask)
		return false;

	XACHIEVEMENT_DETAILS* achievements = ( XACHIEVEMENT_DETAILS* )mAchievementDetails;
	
	for(int a=0;a<BLUR_ACHIEVEMENT_MAX;a++)
	{
		if(achievements[a].dwId == mAchievementMapping[achievement])
		{
			return (achievements[a].ftAchieved.dwHighDateTime > 0);
		}
	}

	return false;
}

void C_AchievementSystem::GiveAchievement(int user_index,BLUR_ACHIEVEMENT achievement)
{
	if (mProcessingTask)
		return;

	// Check that we have a valid profile for the pad and not a blagged/guest or inactive profile.
	SysProfile::FullProfile& profile = UI::C_ProfileInterface::Instance().GetPadProfile(user_index);
	if( profile.m_IsGuest || profile.m_State == SysProfile::eProfileStateNotActive || profile.m_State == SysProfile::eProfileStateBlagged )
		return;

	AchievementOverlappedTracker tracker;

	tracker.xenon_achievement = new XUSER_ACHIEVEMENT;
	tracker.xenon_achievement->dwUserIndex=user_index;
	tracker.xenon_achievement->dwAchievementId=mAchievementMapping[achievement];

	tracker.mOverlapped = new XOVERLAPPED;
    ZeroMemory( tracker.mOverlapped, sizeof( XOVERLAPPED ) );

	tracker.mHandle = CreateEvent( NULL, FALSE, FALSE, NULL );
    tracker.mOverlapped->hEvent = tracker.mHandle;

	DWORD error=XUserWriteAchievements(1,tracker.xenon_achievement,tracker.mOverlapped);

	XACHIEVEMENT_DETAILS* achievements = ( XACHIEVEMENT_DETAILS* )mAchievementDetails;
	for(int a=0;a<BLUR_ACHIEVEMENT_MAX;a++)
	{
		if(achievements[a].dwId == mAchievementMapping[achievement])
		{
			achievements[a].ftAchieved.dwHighDateTime = 1;	//	Update our Achievement details so that we know the profile has been awarded this.
			achievements[a].ftAchieved.dwLowDateTime = 1;
		}
	}

	if(error != ERROR_IO_PENDING)
	{

	}

	mGiveAchievementTasksStore.PushBack(tracker);
}

}; // namespace Game
