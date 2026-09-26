#include "stdafx.h"

#include <Game\Achievements\AchievementSystem.h>
#include <Game\Messaging\GameflowMessages.h>

namespace Game
{

C_AchievementSystem C_AchievementSystem::s_instance;

void C_AchievementSystem::Init()
{


}

WCHAR *C_AchievementSystem::GetAchievementName(U32 achievement)
{
	return L"";
}

void C_AchievementSystem::GetAchievementDetails(int user_index)
{

}

void C_AchievementSystem::Update()
{


}

void C_AchievementSystem::InitialiseSubscription( Messaging::C_MessageSubscriberSystem* pSubscriberSystem )
{
}

bool C_AchievementSystem::MessageHandler(const  Messaging::C_MessageBase* pMessage)
{
	return false;
}

bool C_AchievementSystem::HasAchievement(int user_index,BLUR_ACHIEVEMENT achievement)
{
	return false;
}

void C_AchievementSystem::GiveAchievement(int user_index,BLUR_ACHIEVEMENT achievement)
{



}

}; // namespace Game