#include "SharedActorsPch.h"

#include <actor_system/ActorList.h>
#include <actor_system/ActorMessages.h>
#include <shared_actors/SharedActorUtils.h>


namespace SharedActorUtils
{

C_ActorLimitList::C_ActorLimitList(U32 maxEntries, Actor::C_ActorList *pActorList):
				m_pActorList(pActorList),m_MaxEntries(maxEntries),m_CurrentSlot(0) 
{
	m_ActorHandleList = new Actor::C_ActorHandle[m_MaxEntries];
}

C_ActorLimitList::~C_ActorLimitList()
{
	SAFE_DELETE_ARY(m_ActorHandleList);
}


bool	C_ActorLimitList::AddActor(Actor::C_ActorHandle newActor)
{
	BIZ_ASSERT(m_pActorList);

	// Kill Old Actor
	m_pActorList->SendMessageToActor(m_ActorHandleList[m_CurrentSlot], Actor::E_ActorMsg_Kill);

	m_ActorHandleList[m_CurrentSlot] = newActor;
	m_CurrentSlot++;
	if(m_CurrentSlot == m_MaxEntries)
		m_CurrentSlot = 0;
	return true;
}
	
void* HandleToUserData(const Actor::C_ActorHandle& actorHandle)
{
	StaticAssert(sizeof(Actor::C_ActorHandle) == sizeof(void*), we_want_to_reinterpret_these_bits);
	return *reinterpret_cast<void* const*>(&actorHandle);
}

Actor::C_ActorHandle HandleFromUserData(const void* pUserData)
{
	StaticAssert(sizeof(Actor::C_ActorHandle) == sizeof(void*), we_want_to_reinterpret_these_bits);
	return *reinterpret_cast<const Actor::C_ActorHandle*>(&pUserData);
}


} // namespace SharedActorUtils
