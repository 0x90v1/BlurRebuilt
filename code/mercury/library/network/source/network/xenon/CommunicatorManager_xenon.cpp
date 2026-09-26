#include <StdAfx.h>
#include <Network\CommunicatorManager.h>
#include <SystemProfile/SystemProfile.h>
#include <Network/CommunicatorVoiceBuffer.h>
#include "../Shared/Remotetalker.h"


#if defined ( USE_XHV2 )
namespace Audio
{
	namespace Core
	{
		// global exposed by the audio renderer.. Audio\source\core\renderer.cpp
		extern IXAudio2* g_pXAudio2;
	}
}
#endif

namespace Communicator
{
	// static instance..
	C_Core	C_Core::s_CommunicatorCore;


	// constructor
	C_Core::C_Core() :	m_MaxLocalTalkers(0), m_MaxRemoteTalkers(0),
						m_LocalRegisteredIndexes(0), m_pRemoteTalkerStorage(NULL), m_pRemoteTalkerList(NULL),
						m_pRemoteTalkerPool(NULL), m_Initialised(false), m_pXHV(NULL)
	{
	}


	// destructor 
	C_Core::~C_Core()
	{
		Shutdown();
	}


	/// call once to initialise all dynamic resources and startup voice engine.
	bool C_Core::Initialise(const U32 maxLocalPlayers, const U32 maxRemotePlayers)
	{
		if (m_Initialised)
			return true; // already initialised once.

		if (0 == maxLocalPlayers)
			return false; // must have local players, and some storage for the voice data..

		m_MaxLocalTalkers	=	(maxLocalPlayers <= XUSER_MAX_COUNT) ? maxLocalPlayers : XUSER_MAX_COUNT;
		m_MaxRemoteTalkers	=	maxRemotePlayers;

		// allocate dynamic memory if required for remote talkers.
		if (m_MaxRemoteTalkers)
		{
			m_pRemoteTalkerStorage	=	new C_RemoteTalker[m_MaxRemoteTalkers];
			m_pRemoteTalkerList		=	reinterpret_cast<void*> (new C_RemoteTalkerList);
			m_pRemoteTalkerPool		=	reinterpret_cast<void*> (new C_RemoteTalkerList);

			if (NULL == m_pRemoteTalkerStorage || NULL == m_pRemoteTalkerList || NULL == m_pRemoteTalkerPool)
			{
				// tonk, cant allocate memory..
				Cleanup();
				return false;
			}

			// setup pool objects
			C_RemoteTalkerList* pPool = reinterpret_cast<C_RemoteTalkerList*> (m_pRemoteTalkerPool);
			for (U32 i=0; i<m_MaxRemoteTalkers; i++)
				pPool->PushBack(m_pRemoteTalkerStorage[i]);
		}

		// go on ya wazzle... start up.
		if (!CreateEngine())
			Cleanup();
		else
			m_Initialised = true;

		return m_Initialised;
	}


	// release all resources
	void C_Core::Shutdown(void)
	{
		UnRegisterAllPlayers();
		Cleanup();
	}


	// register local player for voice communications.
	bool C_Core::RegisterLocalTalker(const SysProfile::FullProfile& localTalker, const bool enforcePrivileges)
	{
		if (!m_Initialised)
			return false;
		
		// gamer profile must be valid for voice..
		switch (localTalker.m_State)
		{
		case SysProfile::eProfileStateOffline:
		case SysProfile::eProfileStateOnline:
		case SysProfile::eProfileStateBlagged:
			break;
		default:
			return false;
			break;
		}

		// validate userindex
		if (localTalker.m_UserIndex >= XUSER_MAX_COUNT)
		{
			LOGFATAL( g_Log,  "Communicator::C_Core::RegisterLocalTalker() - Invalid profile user index");
			return false;
		}

		// is user already registered..
		if (m_LocalRegisteredIndexes&(1<<localTalker.m_UserIndex))
			return true;

		// make sure player has voice priviliges
		if (enforcePrivileges)
		{
			if (!localTalker.m_CommunicationsPrivilege)
			{
				if (!localTalker.m_CommunicationsFriendsOnlyPrivilege)
					return false; // local player cant use voice at all !!
			}
		}

		// try and register..
		if (S_OK == m_pXHV->RegisterLocalTalker(localTalker.m_UserIndex))
		{
			m_LocalRegisteredIndexes |= (1<<localTalker.m_UserIndex);
			bool ok = SetLocalMode(localTalker.m_UserIndex, eModeNotActive);	// start up in not active state..
		}
		else
		{
			LOGWARN( g_Log, "Unable to register local talker %d", localTalker.m_UserIndex);
			return false;
		}
	
		return true;
	}


	// change mode for players communicator
	bool C_Core::SetLocalMode(const U8 userIndex, const eCommunicatorMode newMode)
	{
		if (!m_Initialised)
			return false;

		// validate userindex
		if (userIndex >= XUSER_MAX_COUNT)
		{
			LOGFATAL( g_Log, "Communicator::C_Core::SetLocalMode() - Invalid profile user index");
			return false;
		}

		if (!(m_LocalRegisteredIndexes&(1<<userIndex)))
			return false; // specified user is not registered..

		HRESULT hr;
		HRESULT hr1;
		XHV_PROCESSING_MODE LocalModes[] = {XHV_LOOPBACK_MODE, XHV_VOICECHAT_MODE};
		bool ok = true;

		switch (newMode)
		{
		case eModeNotActive:
			hr = m_pXHV->StopLocalProcessingModes(userIndex, LocalModes, 2);
			if (S_OK != hr)
				ok = false;
			break;

		case eModeNormal:
			hr = m_pXHV->StopLocalProcessingModes(userIndex, (PXHV_PROCESSING_MODE)&XHV_LOOPBACK_MODE, 1);
			hr1 = m_pXHV->StartLocalProcessingModes(userIndex, (PXHV_PROCESSING_MODE)&XHV_VOICECHAT_MODE, 1);
			if (S_OK != hr || S_OK != hr1)
				ok = false;
			break;

		case eModeLoopback:
			hr = m_pXHV->StopLocalProcessingModes(userIndex, (PXHV_PROCESSING_MODE)&XHV_VOICECHAT_MODE, 1);
			hr1 = m_pXHV->StartLocalProcessingModes(userIndex, (PXHV_PROCESSING_MODE)&XHV_LOOPBACK_MODE, 1);
			if (S_OK != hr || S_OK != hr1)
				ok = false;
			break;

		default:
			LOGFATAL( g_Log, "Communicator::C_Core::SetLocalMode() - Invalid Communicator mode supplied");
			return false;
		break;
		}

		if (!ok)
		{
			LOGWARN( g_Log, "Unable to set local communicator mode for user %d", userIndex);
		}

		return ok;
	}

	
	// Attempt to unregister a local player from voice communication encoding
	bool C_Core::UnRegisterLocalTalker(const U8 userIndex)
	{
		if (!m_Initialised)
			return false;

		// validate userindex
		if (userIndex >= XUSER_MAX_COUNT)
		{
			LOGFATAL( g_Log,  "Communicator::C_Core::UnRegisterLocalTalker() - Invalid profile user index");
			return false;
		}

		if (!(m_LocalRegisteredIndexes&(1<<userIndex)))
			return false; // specified user is not registered..

		// try and unregister
		if (S_OK == m_pXHV->UnregisterLocalTalker(userIndex))
		{
			m_LocalRegisteredIndexes &= ~(1<<userIndex);

			if (m_MaxRemoteTalkers > 0)
			{
				// iterate through all remote talkers and clear the mute data
				C_RemoteTalkerList* pList = reinterpret_cast<C_RemoteTalkerList*> (m_pRemoteTalkerList);

				C_RemoteTalkerList::Iterator it;
				for (it = pList->Begin(); it != pList->End(); ++it)
					it->m_LocalMuted &= ~(1<<userIndex);
			}
		}
		else
			return false;

		return true;
	}


	// register a remote talker so we can decode any voice packets they send.
	bool C_Core::RegisterRemoteTalker(const SysProfile::UserIdentifier& remoteTalker)
	{
		if (!m_Initialised)
			return false;

		if (0 == m_MaxRemoteTalkers)
			return false;	// we specified no remote players on startup
		
		if (0 == remoteTalker.m_Uid)
			return false; // blagged XUID..

		C_RemoteTalkerList* pPool = reinterpret_cast<C_RemoteTalkerList*> (m_pRemoteTalkerPool);
		C_RemoteTalkerList* pList = reinterpret_cast<C_RemoteTalkerList*> (m_pRemoteTalkerList);

		// check for duplicates..
		C_RemoteTalkerList::Iterator it;
		for (it = pList->Begin(); it != pList->End(); ++it)
		{
			if (XOnlineAreUsersIdentical(it->m_UserData.m_Uid, remoteTalker.m_Uid))
				return true; // remote talker already added to list
		}

		if (pPool->Empty())
			return false; // not enough open slots top add new player to remote talkers list

		if (SUCCEEDED(m_pXHV->RegisterRemoteTalker(remoteTalker.m_Uid, NULL, NULL, NULL)))
		{
			XHV_PROCESSING_MODE mode = XHV_VOICECHAT_MODE;
			if (SUCCEEDED(m_pXHV->StartRemoteProcessingModes(remoteTalker.m_Uid, &mode, 1)))
			{
				// push newly added remote talker into the list.
				it = pPool->Begin();
				pPool->Erase(pPool->Begin());
				it->m_LocalMuted	= 0;		// not muted by default.
				it->m_RemoteMuted	= 0;
				it->m_NeedsUpdate	= false;
				it->m_UserData		= remoteTalker;
				pList->PushBack(*it);
			}
			else
			{
				LOGWARN( g_Log, "Unable to change processing mode for remote talker");

				// remove remote talker and return 
				m_pXHV->UnregisterRemoteTalker(remoteTalker.m_Uid);
				return false;
			}
		}
		else
		{
			LOGWARN( g_Log, "Unable to register remote talker");
			return false;
		}

		return true;
	}


	// unregister a remote talker to stop voice being decoded for this player.
	bool C_Core::UnRegisterRemoteTalker(const SysProfile::UserIdentifier& remoteTalker)
	{
		if (!m_Initialised)
			return false;
		
		if (0 == m_MaxRemoteTalkers)
			return false;	// we specified no remote players on startup.

		C_RemoteTalkerList* pPool = reinterpret_cast<C_RemoteTalkerList*> (m_pRemoteTalkerPool);
		C_RemoteTalkerList* pList = reinterpret_cast<C_RemoteTalkerList*> (m_pRemoteTalkerList);

		// find specified player
		C_RemoteTalkerList::Iterator it;
		for (it = pList->Begin(); it != pList->End(); ++it)
		{
			if (XOnlineAreUsersIdentical(it->m_UserData.m_Uid, remoteTalker.m_Uid))	
			{
				if (SUCCEEDED(m_pXHV->UnregisterRemoteTalker(remoteTalker.m_Uid)))
				{
					// remove player from list and give back to pool.
					C_RemoteTalkerList::Iterator cur = it;
					it = pList->Erase(cur);
					pPool->PushBack(*cur);
					return true;
				}
				return false;
			}
		}

		return false;
	}


	bool C_Core::IsRemoteTalkerRegistered(const SysProfile::UserIdentifier& remoteTalker)
	{
		if (!m_Initialised)
			return false;

		if (0 == m_MaxRemoteTalkers)
			return false;	// we specified no remote players on startup.

		C_RemoteTalkerList* pPool = reinterpret_cast<C_RemoteTalkerList*> (m_pRemoteTalkerPool);
		C_RemoteTalkerList* pList = reinterpret_cast<C_RemoteTalkerList*> (m_pRemoteTalkerList);

		// check for duplicates..
		C_RemoteTalkerList::Iterator it;
		for (it = pList->Begin(); it != pList->End(); ++it)
		{
			if (XOnlineAreUsersIdentical(it->m_UserData.m_Uid, remoteTalker.m_Uid))
				return true; // remote talker already added to list
		}

		return false;
	}


	// Unregister all players both local and remote.
	bool C_Core::UnRegisterAllPlayers(void)
	{
		if (!m_Initialised)
			return false;

		bool needToCloseEngine = false;

		// unregister remote first
		if (m_MaxRemoteTalkers)
		{
			C_RemoteTalkerList* pPool = reinterpret_cast<C_RemoteTalkerList*> (m_pRemoteTalkerPool);
			C_RemoteTalkerList* pList = reinterpret_cast<C_RemoteTalkerList*> (m_pRemoteTalkerList);

			C_RemoteTalkerList::Iterator it;
			for (it = pList->Begin(); it != pList->End();)
			{
				if (S_OK != m_pXHV->UnregisterRemoteTalker(it->m_UserData.m_Uid))
					needToCloseEngine = true;

				// remove player from list and give back to pool.
				C_RemoteTalkerList::Iterator cur = it;
				it = pList->Erase(cur);
				pPool->PushBack(*cur);
			}
		}

		// unregister local..
		U32 p;
		for (p=0; p<XUSER_MAX_COUNT; p++)
		{
			if (m_LocalRegisteredIndexes&(1<<p))
			{
				if (S_OK != m_pXHV->UnregisterLocalTalker(p))
					needToCloseEngine = true;
			}
		}

		// set no local players currently registered anymore..
		m_LocalRegisteredIndexes = 0;

		assert(!needToCloseEngine);
		if (needToCloseEngine)
		{
			if (!CreateEngine())
			{
				// cant re create engine so release all resources.
				Cleanup();
				return false;
			}
		}

		return true;
	}


	// change a remote players decoding priority
	bool C_Core::SetDecodingPriority(const U8 userIndex, const SysProfile::UserIdentifier& remoteTalker, const U8 decodePriority, const bool muteRemoteTalker)
	{
		if (!m_Initialised)
			return false;
		
		if (0 == m_MaxRemoteTalkers)
			return false;	// we specified no remote players on startup.

		// validate userindex
		if (userIndex >= XUSER_MAX_COUNT)
		{
			LOGFATAL( g_Log,  "Communicator::C_Core::SetDecodingPriority() - Invalid profile user index");
			return false;
		}

		if (!(m_LocalRegisteredIndexes&(1<<userIndex)))
			return false; // specified user is not registered..

		XHV_PLAYBACK_PRIORITY priority = decodePriority;

		C_RemoteTalkerList* pList = reinterpret_cast<C_RemoteTalkerList*> (m_pRemoteTalkerList);

		// find specified player
		C_RemoteTalkerList::Iterator it;
		for (it = pList->Begin(); it != pList->End(); ++it)
		{
			if (XOnlineAreUsersIdentical(it->m_UserData.m_Uid, remoteTalker.m_Uid))
			{
				it->m_Priority = decodePriority;
				
				// check muted status, we dont want to mess with decode priority is player is muted.
				if (LocalMuted(userIndex, remoteTalker) || RemoteMuted(userIndex, remoteTalker))
					return true;

				// change decode priority for OS..
				if (m_pXHV->SetPlaybackPriority(remoteTalker.m_Uid, userIndex, priority) == S_OK)
					return true;

				LOGWARN( g_Log, "unable to set voice decoding priority for remote player");
				return false;
			}
		}

		return false;
	}


	// does a specific player have a communicator connected.
	bool C_Core::CommunicatorConnected(const U8 userIndex)
	{
		if (!m_Initialised)
			return false;

		// validate userindex
		if (userIndex >= XUSER_MAX_COUNT)
		{
			LOGFATAL( g_Log,  "Communicator::C_Core::CommunicatorConnected() - Invalid profile user index");
			return false;
		}

		if (!(m_LocalRegisteredIndexes&(1<<userIndex)))
			return false; // specified user is not registered..

		return m_pXHV->IsHeadsetPresent(userIndex) ? true : false;
	}

	
	// is a registered local player currently talking..
	bool C_Core::IsLocalTalking(const U8 userIndex)
	{
		if (!m_Initialised)
			return false;

		// validate userindex
		if (userIndex >= XUSER_MAX_COUNT)
		{
			LOGFATAL( g_Log,  "Invalid profile user index");
			return false;
		}

		if (!(m_LocalRegisteredIndexes&(1<<userIndex)))
			return false; // specified user is not registered..

		return m_pXHV->IsLocalTalking(userIndex) ? true : false;
	}


	// is a registered remote player talking..
	bool C_Core::IsRemoteTalking(const SysProfile::UserIdentifier& remoteTalker)
	{
		if (!m_Initialised)
			return false;

		if (0 == m_MaxRemoteTalkers)
			return false;

		C_RemoteTalkerList* pList = reinterpret_cast<C_RemoteTalkerList*> (m_pRemoteTalkerList);

		// find specified player
		C_RemoteTalkerList::Iterator it;
		for (it = pList->Begin(); it != pList->End(); ++it)
		{
			if (XOnlineAreUsersIdentical(it->m_UserData.m_Uid, remoteTalker.m_Uid))	
			{
				if (it->m_RemoteMuted != m_LocalRegisteredIndexes && it->m_LocalMuted != m_LocalRegisteredIndexes)
					return m_pXHV->IsRemoteTalking(remoteTalker.m_Uid) ? true : false;

				return false;
			}
		}
		return false;
	}

	U32 C_Core::AudioFrameSize(void) const
	{
		return XHV_VOICECHAT_MODE_PACKET_SIZE;
	}

	// encode voice for a local player
	bool C_Core::GetLocalVoiceData(const U8 userIndex, U8* pOutputBuffer, U32 outputBufferSize, U32& numBytesEncoded)
	{
		// default this. 
		numBytesEncoded = 0;

		if (!m_Initialised)
			return false;

		// validate userindex
		if (userIndex >= XUSER_MAX_COUNT)
		{
			LOGERROR( g_Log,  "Communicator::C_Core::GetLocalVoiceData() - Invalid profile user index");
			return false;
		}

		if (!(m_LocalRegisteredIndexes&(1<<userIndex)))
			return false; // specified user is not registered..

		if (NULL == pOutputBuffer)
			return false; // wtf.. no buffer specified.

		if (outputBufferSize < AudioFrameSize())
		{
			LOGWARN( g_Log,  "Communicator::C_Core::GetLocalVoiceData() - Buffer size too small to hold a single encoded voice packet");
			return false;
		} 

		// peek to see if the playewr has voice data awaiting so we dont enter critical section.
		if (m_pXHV->GetDataReadyFlags() & (1<<userIndex))
		{
			// data ready to be encoded for player so grab it.
			DWORD bytes = outputBufferSize;
			DWORD packets = 0;
			if (S_OK == m_pXHV->GetLocalChatData(userIndex, pOutputBuffer, &bytes, &packets))
				numBytesEncoded = bytes;
		}

		return true;
	}

	bool C_Core::GetLocalVoiceData(const U8 userIndex, C_VoiceBuffer& outputBuffer, U32& numBytesEncoded)
	{
		// default this. 
		numBytesEncoded = 0;

		if (!m_Initialised)
			return false;

		if (!outputBuffer.IsInitialised())
			return false;

		// validate userindex
		if (userIndex >= XUSER_MAX_COUNT)
		{
			LOGFATAL( g_Log,  "Communicator::C_Core::GetLocalVoiceData() - Invalid profile user index");
			return false;
		}

		if (!(m_LocalRegisteredIndexes&(1<<userIndex)))
			return false; // specified user is not registered..

		const U32 remaining = outputBuffer.Capacity() - outputBuffer.Size();
		if(remaining < XHV_VOICECHAT_MODE_PACKET_SIZE)
		{
			LOGWARN( g_Log,  "Communicator::C_Core::GetLocalVoiceData() - Not enough space remaining in buffer to encode new voice into.");
			return false; // not enough room remaining in buffer to hold a single voice chat packet.
		}

		if (m_pXHV->GetDataReadyFlags() & (1<<userIndex))
		{
			DWORD bytes = remaining;
			DWORD packets = 0;
			if (S_OK == m_pXHV->GetLocalChatData(userIndex, &outputBuffer.m_pData[outputBuffer.m_CurrentSize], &bytes, &packets))
			{
				numBytesEncoded = bytes;
				outputBuffer.m_CurrentSize += bytes;
			}
		}
		return true;
	}

	// submit remote voice data for decoding and playback.
	bool C_Core::SubmitRemoteVoiceData(const SysProfile::UserIdentifier& remoteTalker, U8* pData, const U32 dataSize, U32* pDataSizeDecoded)
	{
		if(pDataSizeDecoded)
			*pDataSizeDecoded = dataSize;

		if (!m_Initialised)
			return false;

		if (0 == m_MaxRemoteTalkers)
			return false;	// we specified no remote players on startup.

		if (0 == dataSize || NULL == pData)
			return false;

		C_RemoteTalkerList* pList = reinterpret_cast<C_RemoteTalkerList*> (m_pRemoteTalkerList);

		// find specified player
		C_RemoteTalkerList::Iterator it;
		for (it = pList->Begin(); it != pList->End(); ++it)
		{
			if (XOnlineAreUsersIdentical(it->m_UserData.m_Uid, remoteTalker.m_Uid))
			{
				DWORD size = dataSize;
				if (S_OK == m_pXHV->SubmitIncomingChatData(remoteTalker.m_Uid, pData, &size))
				{
					if(pDataSizeDecoded)
						*pDataSizeDecoded = size;

					return true;
				}
			}
		}

		return false;
	}


	// Has a local talker curently muted the specific remote talker ?
	// returns true if muted or false if not
	bool C_Core::LocalMuted(const U8 userIndex, const SysProfile::UserIdentifier& remoteTalker)
	{
		if (!m_Initialised)
			return false;

		if (userIndex< XUSER_MAX_COUNT)
		{
			if (m_LocalRegisteredIndexes&(1<<userIndex))
			{
				BOOL m = false;
				if (XUserMuteListQuery(userIndex, remoteTalker.m_Uid, &m) == ERROR_SUCCESS)
					return (m == TRUE) ? true : false;
			}
		}

		return false;
	}


	// has the remote talker muted me
	// returns true if local player has been muted by remote talker otherwise false
	bool C_Core::RemoteMuted(const U8 userIndex, const SysProfile::UserIdentifier& remoteTalker)
	{
		if (!m_Initialised)
			return false;

		if (userIndex< XUSER_MAX_COUNT)
		{
			if (m_LocalRegisteredIndexes&(1<<userIndex))
			{
				C_RemoteTalkerList* pList = reinterpret_cast<C_RemoteTalkerList*> (m_pRemoteTalkerList);
				C_RemoteTalkerList::Iterator it;
				for (it = pList->Begin(); it != pList->End(); ++it)
				{
					if (XOnlineAreUsersIdentical(remoteTalker.m_Uid, it->m_UserData.m_Uid))
					{
						if (it->m_RemoteMuted&(1<<userIndex))
							return true;
						return false;
					}
				}
			}
		}
		return false;
	}


	// once we receive a message that we have been muted/un muted by a local player this is called to update our voice manager so it knows if it
	// should decode any incoming vocie packets from this remote player.
	// remoteTalker		:	remote player who has changed his mute info
	// localMutedUserIndex	:	user index of the local player who is affected by the remote players mute change
	// mute				:	true if we should mute or false if we should unmute
	// forceUpdate			:	if true then the mute info will take effect straight away otherwise the mute wil only take effect on the next call to MuteUpdate()
	void C_Core::SetMutedRemotely(const SysProfile::UserIdentifier& remoteTalker, const U8 localMutedUserIndex, const bool mute, const bool forceUpdate)
	{
		if (!m_Initialised)
			return;

		if (localMutedUserIndex < XUSER_MAX_COUNT)
		{
			if (m_LocalRegisteredIndexes&(1<<localMutedUserIndex))
			{
				C_RemoteTalkerList* pList = reinterpret_cast<C_RemoteTalkerList*> (m_pRemoteTalkerList);
				C_RemoteTalkerList::Iterator it;
				for (it = pList->Begin(); it != pList->End(); ++it)
				{
					if (XOnlineAreUsersIdentical(remoteTalker.m_Uid, it->m_UserData.m_Uid))
					{
						if (mute)
						{
							if (!(it->m_RemoteMuted&(1<<localMutedUserIndex)))
							{
								it->m_NeedsUpdate = true;
								it->m_RemoteMuted |= (1<<localMutedUserIndex);
								if (forceUpdate)
									MuteUpdate();
							}
						}
						else
						{
							if (it->m_RemoteMuted&(1<<localMutedUserIndex))
							{
								it->m_NeedsUpdate = true;
								it->m_RemoteMuted &= ~(1<<localMutedUserIndex);
								if (forceUpdate)
									MuteUpdate();
							}
						}
						return;
					}
				}
			}
		}
	}


	/// call when you want to update the any mute information to take effect on remote players. 
	void C_Core::MuteUpdate(void)
	{
		if (!m_Initialised)
			return ;

		const DWORD mutePriority = XHV_PLAYBACK_PRIORITY_NEVER;

		C_RemoteTalkerList* pList = reinterpret_cast<C_RemoteTalkerList*> (m_pRemoteTalkerList);

		for (U32 i=0; i<XUSER_MAX_COUNT; i++)
		{
			if (m_LocalRegisteredIndexes&(1<<i))
			{
				// local player is registered. so iterate through remote player list and see who we have muted
				C_RemoteTalkerList::Iterator it;
				for (it = pList->Begin(); it != pList->End(); ++it)
				{
					if (!(it->m_LocalMuted&(1<<i)))
					{
						if (LocalMuted(i, it->m_UserData))
						{
							it->m_NeedsUpdate = true;
							it->m_LocalMuted |= (1<<i); 
						}
					}
					else
					{
						if (!LocalMuted(i, it->m_UserData))
						{
							it->m_NeedsUpdate = true;
							it->m_LocalMuted &= ~(1<<i);  
						}
					}
				}
			}
		}

		C_RemoteTalkerList::Iterator it;
		for (it = pList->Begin(); it != pList->End(); ++it)
		{
			if (it->m_NeedsUpdate)
			{
				for (U8 i=0; i<XUSER_MAX_COUNT; i++)
				{
					if (it->m_LocalMuted&(1<<i) || it->m_RemoteMuted&(1<<i))
					{
						// mute
						if (m_pXHV->SetPlaybackPriority(it->m_UserData.m_Uid, i, mutePriority) == S_OK)
							it->m_NeedsUpdate = false;
					}
					else
					{
						// unmute.
						if (m_pXHV->SetPlaybackPriority(it->m_UserData.m_Uid, i, it->m_Priority) == S_OK)
							it->m_NeedsUpdate = false;
					}
				}
			}
		}	
	}

	// private function to create XHV engine....
	bool C_Core::CreateEngine(void)
	{
		// if engine already exixts shut it down before re-creating..
		if (m_pXHV)
			m_pXHV->Release();
		m_pXHV = NULL;

		// try and enable the voice high level voice engine.
		XHV_INIT_PARAMS params = {0};

		// Set the maximum number of talkers
		params.dwMaxLocalTalkers				= m_MaxLocalTalkers;
		params.dwMaxRemoteTalkers				= m_MaxRemoteTalkers;

	   // Set the processing modes
		XHV_PROCESSING_MODE LocalModes[]		= {XHV_LOOPBACK_MODE, XHV_VOICECHAT_MODE};
		XHV_PROCESSING_MODE RemoteModes[]		= {XHV_VOICECHAT_MODE};
	
		params.localTalkerEnabledModes			= LocalModes;
		params.remoteTalkerEnabledModes			= RemoteModes;
		params.dwNumLocalTalkerEnabledModes		= ARRAYSIZE(LocalModes);
		params.dwNumRemoteTalkerEnabledModes	= ARRAYSIZE(RemoteModes);

		// tell xhv to ignore privileges.. game will enforce these
		#if _XDK_VER>1838 
			params.bRelaxPrivileges				=	true;
		#endif

#if !defined ( USE_XHV2 ) 
		if (S_OK != XHVCreateEngine(&params, NULL, &m_pXHV))
			return false;
		LOGDEBUG( g_Log,  "Communicator::C_Core::CreateEngine() - Voice Communicator Engine Initialised. <Xaudio>");
#else
			params.pXAudio2                     = Audio::Core::g_pXAudio2; 
			if (S_OK != XHV2CreateEngine(&params, NULL, &m_pXHV))
				return false;
			LOGDEBUG( g_Log,  "Communicator::C_Core::CreateEngine() - Voice Communicator Engine Initialised. <Xaudio2>");
#endif

		return true; // yippee..
   	}


	// private cleanup function.. Releases all dynamic resources and shuts down voice engine
	void C_Core::Cleanup(void)
	{
		UnRegisterAllPlayers();

		C_RemoteTalkerList* pPool = reinterpret_cast<C_RemoteTalkerList*> (m_pRemoteTalkerPool);
		C_RemoteTalkerList* pList = reinterpret_cast<C_RemoteTalkerList*> (m_pRemoteTalkerList);

		// release pool
		if (pPool)
		{
			pPool->Clear();
			delete pPool;
			m_pRemoteTalkerPool = NULL;
		}

		// release list
		if (pList)
		{
			pList->Clear();
			delete pList;
			m_pRemoteTalkerList = NULL;
		}

		// delete storage
		if (m_pRemoteTalkerStorage)
		{
			delete [] m_pRemoteTalkerStorage;
			m_pRemoteTalkerStorage = NULL;
		}

		if (m_pXHV)
		{
			m_pXHV->Release(); 
			LOGDEBUG(g_Log,  "Communicator::C_Core::Cleanup() - Voice Communicator Engine Closed.");
		}
		
		m_pXHV							=	NULL;
		m_Initialised					=	false;
		m_MaxLocalTalkers				=	0;
		m_MaxRemoteTalkers				=	0;
		m_LocalRegisteredIndexes		=	0;
	}
}