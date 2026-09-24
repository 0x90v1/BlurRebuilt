#include <StdAfx.h>
#include <Network\Securekey.h>
#include <SystemProfile\SystemProfile.h>
#include <SystemProfile\SystemProfileManager.h>
#include <GameLogic\RichPresenceBuilder.h>
#include <SystemProfile\GameInvite.h>
#include "..\..\Network\Xenon\AsyncTask_Xenon.h"

namespace SysProfile
{
	// static instance..
	C_Manager C_Manager::s_ProfileManager;

	// constructor
	C_Manager::C_Manager() : 
	m_Initialised(false), 
	m_RefCounter(0),
	m_ListenerHandle(NULL), 
	m_pGameInviteTask(NULL),
	m_GameInviteBeingSent(false),
	m_pProfileData(NULL),
	m_MaxUsers(XUSER_MAX_COUNT),
	m_SystemHudActive(false),
	m_pCritSection(NULL),
	m_PrimaryPadNo(-1),
	m_ActiveServiceState(eServiceStateUnknown),
	m_LatestServiceState(eServiceStateUnknown),
	m_AgeRequirement(-1),
	m_ReadUsersTimeout(0.0f)
	{
		m_PartyUserList.dwUserCount = 0;
	}


	// destructor..
	C_Manager::~C_Manager()
	{
		Shutdown();
	}


	// setup a listener to monitor profile events from the dashboard..
	bool C_Manager::Initialise(const S32 ageRequirement)
	{
		if (!m_Initialised)
		{
			m_pCritSection = new MT::Lock;
			if (m_pCritSection)
			{		
				m_pGameInviteTask = new NetPlay::C_AsyncTask;
				if (m_pGameInviteTask)
				{	
					m_pProfileData = new FullProfile[m_MaxUsers]; 
					if (m_pProfileData)
					{
						m_ListenerHandle = XNotifyCreateListener(XNOTIFY_SYSTEM|XNOTIFY_LIVE|XNOTIFY_PARTY);
						if (NULL != m_ListenerHandle && INVALID_HANDLE_VALUE != m_ListenerHandle)
						{
							ZeroMemory(m_pProfileData, sizeof(FullProfile)*XUSER_MAX_COUNT);
							XNotifyPositionUI(XNOTIFYUI_POS_BOTTOMRIGHT);
							m_Initialised = true;

							// initially poll system for default users.
							Notifications n;
							ReadUsers(n);

							return true;
						}
						else
							Shutdown();
					}
					else
						Shutdown();
				}
				else
					Shutdown();
			}
		}
		return false;
	}


	// release listener..
	void C_Manager::Shutdown(void)
	{
		if (NULL != m_ListenerHandle && INVALID_HANDLE_VALUE != m_ListenerHandle)
			CloseHandle(m_ListenerHandle);
		m_ListenerHandle = NULL;

		if (m_pProfileData)
			delete [] m_pProfileData;
		m_pProfileData = NULL;

		if (m_pGameInviteTask)
		{
			m_pGameInviteTask->Close();
			delete m_pGameInviteTask;
			m_pGameInviteTask = NULL;
		}

		if (m_pCritSection)
			delete m_pCritSection;
		m_pCritSection = NULL;
		
		m_ActiveServiceState = eServiceStateUnknown;
		m_LatestServiceState = eServiceStateUnknown;
		m_GameInviteBeingSent	= false;
		m_SystemHudActive	= false;
		m_Initialised		= false;
	}


	// Try and enter a critical section.. useful if you want to access multiple profiles at once via GetProfile() 
	void C_Manager::EnterCritical(void)
	{
		if (m_Initialised)
			m_pCritSection->Enter();
	}


	// Try an leave the critical section.. Must call after using EnterCritical() or bad stuff will happen !!
	void C_Manager::LeaveCritical(void)
	{
		if (m_Initialised)
			m_pCritSection->Exit();
	}


	// check listener for events and user changes..
	void C_Manager::Poll(GameInvite& outGameInvite, Notifications& notifications, const F32 frameTime)
	{
		outGameInvite.m_Active = false;
		
		if (!m_Initialised)
			return;

		DWORD	notification = 0;
		ULONG	parameter = 0;
		U32		i =	0;

		if(XNotifyGetNext(m_ListenerHandle, 0, &notification, &parameter))
		{
			switch (notification)
			{
			// HUD has either be brought up or shutdown
			case XN_SYS_UI:
				m_SystemHudActive = (parameter) ? true : false;
				break;

			case XN_FRIENDS_FRIEND_ADDED:
			case XN_FRIENDS_FRIEND_REMOVED:
			case XN_FRIENDS_PRESENCE_CHANGED:
				notifications.m_FriendsUpdated = true;
				break;

			// user profiles have changed..
			case XN_SYS_SIGNINCHANGED:  
				if (m_ReadUsersTimeout == 0.0f)
					m_ReadUsersTimeout = 1.0f;
				else
				{
					m_ReadUsersTimeout = 0.0f;
					ReadUsers(notifications);
					notifications.m_SignInChanged = true;
				}
				break;

			case XN_PARTY_MEMBERS_CHANGED:
				if (XPartyGetUserList(&m_PartyUserList) == XPARTY_E_NOT_IN_PARTY)
					m_PartyUserList.dwUserCount = 0;
				break;

			case XN_LIVE_LINK_STATE_CHANGED:
				if (parameter == FALSE)
				{
					notifications.m_NetworkCableRemoved = true;
				}
				break;

			case XN_LIVE_CONNECTIONCHANGED:
				switch(parameter)
				{
				case XONLINE_S_LOGON_CONNECTION_ESTABLISHED:
					notifications.m_ServiceConnectionEstablished = true;
					m_ActiveServiceState = m_LatestServiceState = eServiceStateConnected;
					break;
				case XONLINE_E_LOGON_KICKED_BY_DUPLICATE_LOGON:
					notifications.m_DuplicateSignIn = true;
					m_ActiveServiceState = m_LatestServiceState = eServiceStateDisconnected;
					break;
				default:
					notifications.m_ServiceConnectionLost = true;
					m_ActiveServiceState = m_LatestServiceState = eServiceStateDisconnected;
					break;
				}
				break;

			case XN_SYS_MUTELISTCHANGED:
				notifications.m_MuteInfoChanged = true;
				break;

			// specific player has accepted a game invite..
			case XN_LIVE_INVITE_ACCEPTED:
				// [Will B]
				// It appears that the XUserGetSigninState() function is broke and when its called in the initialise function, it is reading the profile state as offline when its online
				// Ignore the check until MS sort out their SDK
				//if (eProfileStateOnline == m_pProfileData[parameter].m_State)
				//{
					outGameInvite.m_Active = (XInviteGetAcceptedInfo(parameter, &(outGameInvite.m_InviteData)) == ERROR_SUCCESS) ? true : false;
				//}
				break;

			// dlc installed
			case XN_LIVE_CONTENT_INSTALLED:
				notifications.m_NewContentInstalled = true;
				break;

			// MU inserted or removed..
			case XN_SYS_STORAGEDEVICESCHANGED:
				notifications.m_StorageDeviceChanged = true;
				break;

			// profile settings for user have been modified.
			case XN_SYS_PROFILESETTINGCHANGED:
				notifications.m_ProfileSettingsChanged = true;

				// rescan gamertags, since they can change while the game is running..
				for (i=0; i<XUSER_MAX_COUNT; i++)
				{
					if(eXUserSigninState_NotSignedIn != XUserGetSigninState(i))
					{
						char temp_name[XUSER_NAME_SIZE];
						XUserGetName(i, temp_name, XUSER_NAME_SIZE);
						temp_name[XUSER_NAME_SIZE-1] = 0;
						swprintf_s(m_pProfileData[i].m_Name,SYSTEM_PROFILE_NAME_LENGTH, L"%S",temp_name);
						m_pProfileData[i].m_Name[SYSTEM_PROFILE_NAME_LENGTH-1] = 0;
					}
				}
				break;

			case XN_SYS_INPUTDEVICESCHANGED:
				notifications.m_InputDeviceChanged = true;			// parameter is invalid
				break;

			case XN_SYS_INPUTDEVICECONFIGCHANGED:
			{
				notifications.m_DeviceConfigChanged |= 1<<parameter;
				break;
			}
			

			default:
				//LOGINFO( GameSys::g_Log,  "Un handled notification ID 0x%x\n", notification);
				break;
			}
		}

		if (m_ReadUsersTimeout > 0.0f)
		{
			m_ReadUsersTimeout -= frameTime;

			if (m_ReadUsersTimeout < 0.0f)
			{
				m_ReadUsersTimeout = 0.0f;
				ReadUsers(notifications);
				notifications.m_SignInChanged = true;
			}
		}
	}

	// is a system profile active for a specific user.. get profile data
	bool C_Manager::GetProfile(const U32 userIndex, FullProfile& outProfileData)
	{
		bool ret = false;
		
		// as default copy a blank profile..
		FullProfile blank;
		outProfileData = blank;

		if (m_Initialised)
		{
			BIZ_ASSERT(userIndex < XUSER_MAX_COUNT);
			if (m_pProfileData[userIndex].m_State != eProfileStateNotActive)
			{
				outProfileData = m_pProfileData[userIndex];
				ret = true;
			}
		}
		return ret;
	}


	// monitor a profile for changes... if profile changes from offline to online or vice versa then reflect this change in the passed in profile.
	// a profile changing from off line to online wont triggere ProfilesChanged() notification since the user has not changed.
	void C_Manager::MonitorProfile(FullProfile& activeProfile)
	{
		switch (activeProfile.m_State)
		{
		case SysProfile::eProfileStateOffline:
		case SysProfile::eProfileStateOnline:
			if (activeProfile.m_UserIndex < m_MaxUsers)
			{
				const SysProfile::FullProfile& stored = m_pProfileData[activeProfile.m_UserIndex];
				switch (stored.m_State)
				{
				case SysProfile::eProfileStateOffline:
				case SysProfile::eProfileStateOnline:
					if (stored.m_Ref == activeProfile.m_Ref)
						activeProfile = stored; // same user so update profile data.. status may have changed.. also name may have changed.
				break;

				default:
					break;
				}
			}
			else
				BIZ_ASSERT_FAIL("MonitorProfile");
			break;

		default:
			break;
		}
	}


	// whan on system profiles are available we can setup blagged ones..
	bool C_Manager::CreateBlaggedProfile(const U32 userIndex, const WCHAR* pDefaultName, FullProfile& outProfileData)
	{
		if (!m_Initialised)
			return false;

		if (userIndex >= m_MaxUsers)
		{
			BIZ_ASSERT_FAIL("CreateBlaggedProfile");
			return false; // user index out of range.
		}

		// start with an empty profile.
		FullProfile blank;
		outProfileData = blank;

		outProfileData.m_Ref				=	++m_RefCounter;
		outProfileData.m_LastModifiedTick	=	GetTickCount();
		outProfileData.m_State				=	SysProfile::eProfileStateBlagged;
		outProfileData.m_UserIndex			=	userIndex;
		outProfileData.m_IsGuest			=	false;

		// privileges..
		outProfileData.m_ValidPrivileges						=	true;							
		outProfileData.m_OnlineMultiplayerPrivilege				=	false;				
		outProfileData.m_ScoreboardPrivilege					=	false;
		outProfileData.m_ParentalLocked							=	false;
		outProfileData.m_CommunicationsPrivilege				=	false;					
		outProfileData.m_CommunicationsFriendsOnlyPrivilege		=	false;		
		outProfileData.m_ProfileViewingPrivilege				=	false;					
		outProfileData.m_ProfileViewingFriendsOnlyPrivilege		=	false;		
		outProfileData.m_ViewUserContentPrivilege				=	true;				
		outProfileData.m_ViewUserContentFriendsOnlyPrivilege	=	false;		
		outProfileData.m_PurchaseContentPrivilege				=	false;				

		// user Identifier..
		outProfileData.m_Uid	=	0;

		BIZ_ASSERT(SYSTEM_PROFILE_NAME_LENGTH > 0); // WTF !!

		// player name
		if (pDefaultName)
			_snwprintf_s(outProfileData.m_Name, lengthof(outProfileData.m_Name), _TRUNCATE, pDefaultName );
		else
			_snwprintf_s(outProfileData.m_Name, lengthof(outProfileData.m_Name), _TRUNCATE, L"P(%d)",userIndex+1);
		return true;
	}

	bool C_Manager::SetPresence(const U32 userIndex, RacerNet::eRichPresence presence, bool joinInProgress)
	{
		if (!m_Initialised)
			return false;

		if (userIndex > XUSER_MAX_COUNT)
		{
			LOGWARN(NetPlay::g_Log, "C_Manager::SetPresence() FAILED - userIndex out of range.");
			return false;
		}
		
		const FullProfile& user = m_pProfileData[userIndex];
		if (SysProfile::eProfileStateBlagged == user.m_State || user.m_IsGuest || !user.Active())
			return false; // no presence for offline users or guests

		Game::C_RichPresenceBuilder::Instance().BuildRichPresence(userIndex, presence);

		return true;
	}

	bool C_Manager::StartSendGameInvite(const U32 userIndex, const UserIdentifier* pRecipients, const U32 numRecipients, NetPlay::SessionSecurity& sessionData, WCHAR* inviteText)
	{
		if (!m_Initialised || NULL == pRecipients || 0 == numRecipients)
			return false;
		
		if (m_GameInviteBeingSent)
		{
			LOGWARN(NetPlay::g_Log, "SysProfile::StartSendGameInvite() - called when still sending previous invite.");
			return false;
		}

		// kill previous invite task if needed.
		m_pGameInviteTask->Close();
		m_GameInviteBeingSent = false;

		if (userIndex >= XUSER_MAX_COUNT)
			return false;

		const SysProfile::FullProfile& fp = m_pProfileData[userIndex];
		if (eProfileStateOnline != fp.m_State || fp.m_IsGuest)
			return false; // player not able to actually send invites !

		//make sure we only send to non guests and also not to player who are signed in on this console !
		// if local users change while sending invite then the invite will be cancelled.
		U32 added = 0;
		for (U32 i=0; i<numRecipients; i++)
		{
			if (pRecipients[i].m_IsGuest)
				continue;

			bool isLocal = false;
			for (U32 z=0; z<m_MaxUsers; z++)
			{
				const FullProfile& user = m_pProfileData[z];
				if (user.Active() && user.m_Uid == pRecipients[i].m_Uid)
					isLocal = true;
			}

			if (isLocal) // ignore this user as you dont need to send invites to players on the same console. 
				continue;

			// add player to invite list.
			m_InvitedFriends[added++] = pRecipients[i].m_Uid;
		}

		if (0 == added) // no valid users in the invite list ?.
			return false; 

		const DWORD res = XInviteSend(userIndex, added, m_InvitedFriends, inviteText, &(*m_pGameInviteTask));
		if (ERROR_IO_PENDING != res)
			return false;

		m_GameInviteBeingSent = true;
		return true;
	}

	bool C_Manager::StartSendExternalPartyInvite(const U32 userIndex, const U8 minRemote)
	{
		if (!m_Initialised)
			return false;
		if (m_GameInviteBeingSent)
		{
			LOGWARN(NetPlay::g_Log, "SysProfile::StartSendExternalPartyInvite() - called when still sending previous invite.");
			return false;
		}

		// kill previous invite task if needed.
		m_pGameInviteTask->Close();
		m_GameInviteBeingSent = false;

		if (userIndex >= XUSER_MAX_COUNT)
			return false;

		const SysProfile::FullProfile& fp = m_pProfileData[userIndex];
		if (eProfileStateOnline != fp.m_State || fp.m_IsGuest)
			return false; // player not able to actually send invites !

		// is this local player in the external party session
		bool friendsMode = true;
		if (IsInExternalParty(minRemote))
		{
			// make 100% sure local specified local player is in external session.
			for (DWORD i=0; i<m_PartyUserList.dwUserCount && friendsMode; i++)
			{
				if (XOnlineAreUsersIdentical(fp.m_Uid, m_PartyUserList.Users[i].Xuid))
					friendsMode = false;
			}
		}

		if (friendsMode)
		{
			// specified player is not in an external party so just bring up his friends gui.
			if (XShowFriendsUI(userIndex) == ERROR_SUCCESS)
				return true;
		}
		else
		{
			// send a single game invite to the whole party.
			const DWORD res = XPartySendGameInvites(userIndex, &(*m_pGameInviteTask));
			if (ERROR_IO_PENDING != res)
				return false;

			m_GameInviteBeingSent = true;
		}

		return m_GameInviteBeingSent;
	}

	bool C_Manager::ContinueSendGameInvite(bool& errorOccurred)
	{
		errorOccurred = true;
		if (!m_Initialised)
			return true;

		if (!m_GameInviteBeingSent)
			return true;

		if (!m_pGameInviteTask->Finished())
			return false;

		errorOccurred = (m_pGameInviteTask->ErrorCode() != ERROR_SUCCESS) ? true : false;

		m_pGameInviteTask->Close();
		m_GameInviteBeingSent = false;
		return true;
	}

	bool C_Manager::ProcessGameInvite(const U32 userIndex, const UserIdentifier& inviteSender, const bool accepted)
	{
		if (!m_Initialised)
			return false;

		// not required to accept / decline invites from the agme code for xbox360
		LOGWARN(NetPlay::g_Log, "SysProfile::StartProcessInvite() - no need to accept / decline invites from the game code for xbox 360.");
		return false;
	}

	bool C_Manager::GetDemonwareUserId(const U32 userIndex, bdOnlineUserID& demonwareUserId)
	{
		demonwareUserId = 0;

		if (m_Initialised)
		{
			FullProfile fp;
			if (GetProfile(userIndex, fp))
			{
				if (SysProfile::eProfileStateOnline == fp.m_State && !fp.m_IsGuest)
				{
					demonwareUserId = fp.m_Uid; // copy of XUID on xenon..
					return true;
				}
			}
		}

		return false;
	}

	// private function sued for reading user profiles from the system.
	void C_Manager::ReadUsers(Notifications& notifications)
	{
	    #if _XDK_VER >= 3424
		char temp_name[XUSER_NAME_SIZE];
		#else
		char temp_name[XONLINE_GAMERTAG_SIZE];
		#endif
		
		U8 numOnline = 0;
		U8 numOffline = 0;
		BOOL privTemp;
		XUSER_SIGNIN_INFO signInInfo;

		SysProfile::FullProfile blankProfile;

		for (U32 i=0; i<m_MaxUsers; i++)
		{
			ZeroMemory(&signInInfo, sizeof(signInInfo));

			FullProfile& pro = m_pProfileData[i];
			const bool userCurrentlyActive = pro.m_State != eProfileStateNotActive ? true : false;

			switch (XUserGetSigninState(i))
			{
				case eXUserSigninState_SignedInToLive:    
					numOnline++;
					if (!userCurrentlyActive)
					{
						pro.m_Ref =	++m_RefCounter;
						notifications.m_ProfilesChanged = true;
					}

					// profile has been modified so record tick count.
					pro.m_LastModifiedTick = GetTickCount();

					pro.m_State = eProfileStateOnline;
					pro.m_UserIndex = i;
					XUserGetXUID(i, &pro.m_Uid);
					
	                #if _XDK_VER >= 3424
					XUserGetName(i, temp_name, XUSER_NAME_SIZE);
					temp_name[XUSER_NAME_SIZE-1] = 0;
	                #else
					XUserGetName(i, temp_name, XONLINE_GAMERTAG_SIZE);
					temp_name[XONLINE_GAMERTAG_SIZE-1] = 0;
					#endif
					
					swprintf(pro.m_Name, L"%S",temp_name);

					// read privileges..
					pro.m_ValidPrivileges = true;

					XUserCheckPrivilege(i, XPRIVILEGE_MULTIPLAYER_SESSIONS, &privTemp);
					pro.m_OnlineMultiplayerPrivilege = privTemp ? true :false;
					
					// always have access to scoreboards on xbox
					pro.m_ScoreboardPrivilege = true;

					// never get full parental lockout
					pro.m_ParentalLocked = false;

					XUserCheckPrivilege(i, XPRIVILEGE_COMMUNICATIONS, &privTemp);
					pro.m_CommunicationsPrivilege = privTemp ? true : false;
					
					if (!pro.m_CommunicationsPrivilege) 
					{
						XUserCheckPrivilege(i, XPRIVILEGE_COMMUNICATIONS_FRIENDS_ONLY, &privTemp);
						pro.m_CommunicationsFriendsOnlyPrivilege = privTemp ? true : false;
					}
					else
						pro.m_CommunicationsFriendsOnlyPrivilege = false;

					XUserCheckPrivilege(i, XPRIVILEGE_PROFILE_VIEWING, &privTemp);
					pro.m_ProfileViewingPrivilege = privTemp ? true : false;
					
					if (!pro.m_ProfileViewingPrivilege)
					{
						XUserCheckPrivilege(i, XPRIVILEGE_PROFILE_VIEWING_FRIENDS_ONLY, &privTemp);
						pro.m_ProfileViewingFriendsOnlyPrivilege = privTemp ? true : false;
					}
					else
						pro.m_ProfileViewingFriendsOnlyPrivilege = false;
					
					XUserCheckPrivilege(i, XPRIVILEGE_USER_CREATED_CONTENT, &privTemp);
					pro.m_ViewUserContentPrivilege = privTemp ? true : false;
					
					if (!pro.m_ViewUserContentPrivilege)
					{
						XUserCheckPrivilege(i, XPRIVILEGE_USER_CREATED_CONTENT_FRIENDS_ONLY, &privTemp);
						pro.m_ViewUserContentFriendsOnlyPrivilege = privTemp ? true : false;  
					}
					else
						pro.m_ViewUserContentFriendsOnlyPrivilege = false;

					XUserCheckPrivilege(i, XPRIVILEGE_PURCHASE_CONTENT, &privTemp);
					pro.m_PurchaseContentPrivilege = privTemp ? true : false;
					
					// check for guest..
					pro.m_IsGuest = false; 
					switch (XUserGetSigninInfo(i, 0, &signInInfo))
					{	
					case ERROR_SUCCESS:
						pro.m_IsGuest = signInInfo.dwGuestNumber == 0 ? false : true;
						break;

					default:
						//LOGINFO(NetPlay::g_Log, "CLEAR: player %d,", i);
						m_pProfileData[i] = blankProfile;
						notifications.m_ProfilesChanged = true;
						break;
					}

					break;
				
				case eXUserSigninState_SignedInLocally:
					numOffline++;
					if (!userCurrentlyActive)
					{
						pro.m_Ref =	++m_RefCounter;
						notifications.m_ProfilesChanged = true;
					}

					// profile has been modified so record tick count.
					pro.m_LastModifiedTick = GetTickCount();

					pro.m_State = eProfileStateOffline;
					pro.m_UserIndex = i;
					XUserGetXUID(i, &pro.m_Uid);
					
					#if _XDK_VER >= 3424
					XUserGetName(i, temp_name, XUSER_NAME_SIZE);
					temp_name[XUSER_NAME_SIZE-1] = 0;
					#else
					XUserGetName(i, temp_name, XONLINE_GAMERTAG_SIZE);
					temp_name[XONLINE_GAMERTAG_SIZE-1] = 0;
					#endif
					
					swprintf(pro.m_Name, L"%S",temp_name);

					// read privileges..
					pro.m_ValidPrivileges = true;

					XUserCheckPrivilege(i, XPRIVILEGE_MULTIPLAYER_SESSIONS, &privTemp);
					pro.m_OnlineMultiplayerPrivilege = privTemp ? true :false;

					XUserCheckPrivilege(i, XPRIVILEGE_COMMUNICATIONS, &privTemp);
					pro.m_CommunicationsPrivilege = privTemp ? true : false;

					if (!pro.m_CommunicationsPrivilege)
					{
						XUserCheckPrivilege(i, XPRIVILEGE_COMMUNICATIONS_FRIENDS_ONLY, &privTemp);
						pro.m_CommunicationsFriendsOnlyPrivilege = privTemp ? true : false;
					}
					else
						pro.m_CommunicationsFriendsOnlyPrivilege = false;

					XUserCheckPrivilege(i, XPRIVILEGE_PROFILE_VIEWING, &privTemp);
					pro.m_ProfileViewingPrivilege = privTemp ? true : false;

					if (!pro.m_ProfileViewingPrivilege)
					{
						XUserCheckPrivilege(i, XPRIVILEGE_PROFILE_VIEWING_FRIENDS_ONLY, &privTemp);
						pro.m_ProfileViewingFriendsOnlyPrivilege = privTemp ? true : false;
					}
					else
						pro.m_ProfileViewingFriendsOnlyPrivilege = false;

					XUserCheckPrivilege(i, XPRIVILEGE_USER_CREATED_CONTENT, &privTemp);
					pro.m_ViewUserContentPrivilege = privTemp ? true : false;

					if (!pro.m_ViewUserContentPrivilege)
					{
						XUserCheckPrivilege(i, XPRIVILEGE_USER_CREATED_CONTENT_FRIENDS_ONLY, &privTemp);
						pro.m_ViewUserContentFriendsOnlyPrivilege = privTemp ? true : false;
					}
					else
						pro.m_ViewUserContentFriendsOnlyPrivilege = false;

					XUserCheckPrivilege(i, XPRIVILEGE_PURCHASE_CONTENT, &privTemp);
					pro.m_PurchaseContentPrivilege = privTemp ? true : false;

					// overwrite some settings... since offline users can use voice + video in system link game sessions..
					pro.m_CommunicationsPrivilege = true;
					pro.m_CommunicationsFriendsOnlyPrivilege = false;
					pro.m_ScoreboardPrivilege = false;
					pro.m_ParentalLocked = false;

					// check for guest..
					pro.m_IsGuest = false;
					switch (XUserGetSigninInfo(i, 0, &signInInfo))
					{	
					case ERROR_SUCCESS:
						pro.m_IsGuest = signInInfo.dwGuestNumber == 0 ? false : true;
						break;
					default:
						LOGINFO(NetPlay::g_Log, "INVALID: player %d,", i);
						m_pProfileData[i] = blankProfile;
						notifications.m_ProfilesChanged = true;
						break;
					}
						
					break;

				default:
					
					//LOGINFO(NetPlay::g_Log, "CLEAR: player %d,", i);

					// reset profile data for this player..
					m_pProfileData[i] = blankProfile;
					if (userCurrentlyActive)
						notifications.m_ProfilesChanged = true;
					break;
			}
		}

		// if suers have changed then clear the game invite request (if it was in the process of being sent)
		if (notifications.m_ProfilesChanged)
		{
			m_pGameInviteTask->Close();
			m_GameInviteBeingSent = false;
		}

		//LOGINFO(g_Log,  "<< System Profile Change >> %d:Online, %d:Ofline\n", numOnline, numOffline);
	}

	void  C_Manager::DemonwarePlayerStatusUpdate(bdOnlineUserID userID, bdNChar8 *userName, bdUByte8 status)
	{
		if (!m_Initialised)
			return;
		// not used on xbox360.
	}

	void C_Manager::DemonwareSessionInvite(bdOnlineUserID senderID, char *senderName, bdSecurityID& secID, void *attachment, U32 attachmentSize)
	{
		if (!m_Initialised)
			return;

		// not used on xbox360. invites go through the guide.
	}

	void C_Manager::DemonwareRichPresenceUpdate(bdOnlineUserID userID, char *userName, void *data, U32 dataSize)
	{
		if (!m_Initialised)
			return;

		// not used on xbox360. presence data goes through the guide.
	}

	bool C_Manager::IsInExternalChat(const UserIdentifier& local, const UserIdentifier& remote)
	{
		if (!m_Initialised)
			return false;
	
		if (local == remote)
			return false;

		U32 numInPartyChat = 0;
		for (DWORD i=0; i<m_PartyUserList.dwUserCount; i++)
		{
			if (XOnlineAreUsersIdentical(local.m_Uid, m_PartyUserList.Users[i].Xuid) && m_PartyUserList.Users[i].dwFlags&XPARTY_USER_ISINPARTYVOICE)
				numInPartyChat += 1;
			else if (XOnlineAreUsersIdentical(remote.m_Uid, m_PartyUserList.Users[i].Xuid) && m_PartyUserList.Users[i].dwFlags&XPARTY_USER_ISINPARTYVOICE)
				numInPartyChat += 1;
		}
		return numInPartyChat == 2 ? true : false;
	}

	bool C_Manager::IsInExternalParty(const U8 minRemote)
	{
		if (!m_Initialised)
			return false;
		U32 local = 0;
		U32 remote = 0;

		for (DWORD i=0; i<m_PartyUserList.dwUserCount; i++)
		{
			if (m_PartyUserList.Users[i].dwFlags&XPARTY_USER_ISLOCAL)
				local += 1;
			else
				remote += 1;
		}

		if (local >= 1 && remote >= minRemote)
			return true;

		return false;
	}
}