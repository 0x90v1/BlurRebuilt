#include <StdAfx.h>
#include <core/threads.h>
#include <Network\FriendEnumerator.h>
#include <SystemProfile/Systemprofile.h>
#include "AsyncTask_xenon.h"
#include <core/maths/MathsUtil.h>

namespace NetPlay
{
	// constructor
	C_FriendsEnumerator::C_FriendsEnumerator() :m_Initialised(false), m_Enumerating(false), m_QuickEnumeration(true), m_NumFriendsEnumerated(0),
							m_pAsyncTask(NULL), m_UserIndex(-1), m_Xuid(0),
							m_EnumeratorHandle(NULL)
	{
	}


	// destructor
	C_FriendsEnumerator::~C_FriendsEnumerator()
	{
		Cleanup();
	}

	
	// Create all dynamic objects.. only needs to be called once prior to first use.
	// returns true if all dynamic objects were created or false if error occurred.
	bool C_FriendsEnumerator::Initialise(void)
	{
		bool ok = false;
		if (!m_Initialised)
		{
			m_pAsyncTask = new C_AsyncTask;
			if (m_pAsyncTask)
			{
				ZeroMemory(m_Friends, sizeof(m_Friends));
				m_NumFriendsEnumerated = 0;
				m_Initialised = true;
				ok = true;
			}
		}
		return ok;
	}


	// private function to release dynamic resources..
	void C_FriendsEnumerator::Cleanup(void)
	{
		StopEnumeration();

		if (m_pAsyncTask)
			delete m_pAsyncTask;
		m_pAsyncTask = NULL;

		m_QuickEnumeration = true;
		m_Enumerating = false;
		m_Initialised = false;
	}


	// Try and start asynchronous friend enumeration for a specific local player
	// returns true if successful or false if enumeration failed to start
	bool C_FriendsEnumerator::StartEnumeration(const SysProfile::FullProfile& inUser, const bool quickEnumeration)
	{
		bool ok = false;
		if (m_Initialised)
		{
			StopEnumeration();
			if (inUser.m_UserIndex >= 0 && inUser.m_UserIndex < XUSER_MAX_COUNT)
			{
				if (inUser.m_IsGuest)
				{
					LOGWARN(NetPlay::g_Log, "C_FriendsEnumerator::StartEnumeration() - Unable to enumerate friends of guest players.");
					return false; //Guest friends enumeration is not possible.
				}

				if (eXUserSigninState_SignedInToLive == XUserGetSigninState(inUser.m_UserIndex))
				{
					XUID x = 0;
					if (ERROR_SUCCESS == XUserGetXUID(inUser.m_UserIndex, &x))
					{
						if (XOnlineAreUsersIdentical(inUser.m_Uid, x))
						{
							if (m_pAsyncTask->Finished())
							{
								// flush old data..
								ZeroMemory(m_Friends, sizeof(m_Friends));
								m_NumFriendsEnumerated = 0;
								m_pAsyncTask->Close();

								// copy local user data
								m_UserIndex	=	inUser.m_UserIndex;
								m_Xuid		=	inUser.m_Uid;

								// create enumerator..
								DWORD bSize = 0;
								DWORD res = XFriendsCreateEnumerator(m_UserIndex, 0, MAX_FRIENDS, &bSize, &m_EnumeratorHandle);

								if (ERROR_SUCCESS == res)
								{
									assert(bSize == sizeof(m_Friends));
									res = XEnumerate(m_EnumeratorHandle, m_Friends, sizeof(m_Friends), NULL, &(*m_pAsyncTask));
									if (ERROR_IO_PENDING == res)
									{
										m_QuickEnumeration = quickEnumeration;
										m_Enumerating = true;
										ok = true;
									}
									else
									{
										LOGWARN(g_Log, "Failed to start friends enumeration.\n");
										m_pAsyncTask->Close();
										CloseHandle(m_EnumeratorHandle);
										m_EnumeratorHandle = NULL;
									}
								}
								else
									m_EnumeratorHandle = NULL; //XDK has a bug that sets an invalid handle value even tho the function failed, so just wipe value..
							}
							else
								LOGWARN(g_Log, "Can't start when previous friends enumeration operation still active\n");
						}
						else
							LOGWARN(g_Log, "Specified user doesnt match currently signed in user\n");
					}
				}
				else
					LOGWARN(g_Log, "Can't enumerate friends for offline players\n");
			}
		}
		return ok;
	}


	// Continue the asynchronous process of enumerating friends.
	// returns true when finished.
	bool C_FriendsEnumerator::ContinueEnumeration(bool& errorOccured)
	{
		errorOccured = true;
		bool done = true;
		assert(m_Initialised);
		if (m_Initialised)
		{
			if (m_Enumerating)
			{
				if (!m_pAsyncTask->Finished())
				{
					done = false;
				}
				else
				{
					errorOccured = (m_pAsyncTask->ErrorCode() != ERROR_SUCCESS);

					//this means it succeeded but there are no friends
					if(m_pAsyncTask->ErrorCode() == 0x80070012)
					{
						errorOccured=false;
					}

					U32 tempNumFriends = 0;
					if (m_pAsyncTask->GetBytesRead(tempNumFriends))
						m_NumFriendsEnumerated = tempNumFriends;
					
					StopEnumeration();
				}
			}
		}
		return done;
	}

	
	// terminate any active friends enumerations.
	void C_FriendsEnumerator::StopEnumeration(void)
	{
		if (!m_Initialised)
			return;
				
		m_pAsyncTask->Close();

		if (m_EnumeratorHandle != NULL && m_EnumeratorHandle != INVALID_HANDLE_VALUE)
			CloseHandle(m_EnumeratorHandle);
		m_EnumeratorHandle = NULL;

		m_Enumerating = false;
	}


	//extract enumerated friends
	U32 C_FriendsEnumerator::ExtractFriends(SysProfile::UserIdentifier* pOutput, const U32 maxOutput, const bool fullFriendsOnly)
	{
		if (!m_Initialised)
			return 0;

		assert(pOutput);
		assert(maxOutput);

		if (NULL == pOutput || 0 == maxOutput)
			return 0;

		if (m_Enumerating)
		{
			LOGWARN(g_Log, "can't extract friends while still enumerating !!\n");
			return 0;
		}

		U32 numExtracted = 0;
		for (U32 i=0; i<m_NumFriendsEnumerated; i++)
		{
			if (numExtracted == maxOutput)
				return numExtracted;

			// quick enumeration returns only online friends.
			if (m_QuickEnumeration)
			{
				if (!(m_Friends[i].dwFriendState&XONLINE_FRIENDSTATE_FLAG_ONLINE))
					continue;
			}

			if (fullFriendsOnly)
			{
				if (m_Friends[i].dwFriendState&XONLINE_FRIENDSTATE_FLAG_SENTREQUEST || m_Friends[i].dwFriendState&XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST)
					continue; // not full friend so dont add to output
			}

			pOutput[numExtracted].m_IsGuest	=	false;
			pOutput[numExtracted++].m_Uid	=	m_Friends[i].xuid;
		}

		return numExtracted;
	}

	// extract more information on each friend.
	U32 C_FriendsEnumerator::ExtractFriends(FriendProfile* pOutput, const U32 maxOutput, const bool fullFriendsOnly)
	{
		if (!m_Initialised)
			return 0;

		assert(pOutput);
		assert(maxOutput);

		if (NULL == pOutput || 0 == maxOutput)
			return 0;

		if (m_Enumerating)
		{
			LOGWARN(g_Log, "can't extract friends while still enumerating !!\n");
			return 0;
		}

		U32 numExtracted = 0;
		for (U32 i=0; i<m_NumFriendsEnumerated; i++)
		{
			if (numExtracted < maxOutput)
			{
				// quick enumeration returns only online friends.
				if (m_QuickEnumeration)
				{
					if (!(m_Friends[i].dwFriendState&XONLINE_FRIENDSTATE_FLAG_ONLINE))
						continue; // offline
				}

				if (fullFriendsOnly)
				{
					if (m_Friends[i].dwFriendState&XONLINE_FRIENDSTATE_FLAG_SENTREQUEST || m_Friends[i].dwFriendState&XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST)
						continue; // not full friend so dont add to output
				}

				FriendProfile profile;
				profile.m_CanInvite				=	(m_Friends[i].dwFriendState&XONLINE_FRIENDSTATE_FLAG_ONLINE) ? true : false;
				profile.m_FriendState			=	(m_Friends[i].dwFriendState&XONLINE_FRIENDSTATE_FLAG_ONLINE) ? eFriendOnline : eFriendOffline;
				profile.m_Profile.m_Uid			=	m_Friends[i].xuid;
				profile.m_Profile.m_IsGuest		=	false;
				swprintf_s(profile.m_Profile.m_Name, L"%S",m_Friends[i].szGamertag);

				RichPresence rp;

				// copy display string
				memcpy(rp.m_DisplayString, m_Friends[i].wszRichPresence, MAX_RICHPRESENCE_SIZE);
				
				// is the player in a joinable session ?
				if (m_Friends[i].dwFriendState&XONLINE_FRIENDSTATE_FLAG_JOINABLE || m_Friends[i].dwFriendState&XONLINE_FRIENDSTATE_FLAG_JOINABLE_FRIENDS_ONLY)
				{
					U64 value = 0;
					for (U32 z=0; z<8; z++)
						value += m_Friends[i].sessionID.ab[z];
			
					if (value != 0)
					{
						rp.m_JoinInProgressPossible = true;
						rp.m_OnlineSessionId = m_Friends[i].sessionID;
					}
				}

				profile.m_RichPresence = rp;

				pOutput[numExtracted++] = profile;
			}
		}

		return numExtracted;
	}
}
