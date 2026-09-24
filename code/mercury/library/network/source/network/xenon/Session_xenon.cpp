#include <StdAfx.h>
#include <Network\Session.h>
#include <Network\Securekey.h>
#include <SystemProfile\SystemProfile.h>
#include <SystemProfile\GameInvite.h>
#include <Network\SessionParameter.h>
#include <Network\SessionEnumerator.h>
#include <Network\SessionDeletion.h>
#include "AsyncTask_Xenon.h"
#include "SessionCommand_xenon.h"  


/*
static void Dump(XSESSION_INFO x)
{
	// dump session Keys to test..
	bool done = false;
	U32 count = 0;
	U32 totalBytesToPrint = sizeof(XNKID);
	U8* p = (U8*)(&(x.sessionID));
	char buff[128];
	LOGINFO(NetPlay::g_Log, "DUMP START");
	while (!done)
	{
		ZeroMemory(&buff, sizeof(buff));
		for (U32 i=0; i<20; i++)
		{
			if (count++ < totalBytesToPrint)
				buff[i] = *p++;
			else
				done = true;

			LOGINFO(NetPlay::g_Log, "%s", buff);
		}
	}

	LOGINFO(NetPlay::g_Log, "DUMP END\n\n");

}
*/

namespace NetPlay
{
	// constructor
	C_Session::C_Session() :	m_Initialised(false), m_SessionState(eStateNotActive),
								m_SessionParameters(0), m_PublicSlots(0), m_PrivateSlots(0), m_UserIndex(0), m_SessionHandle(NULL), 
								m_CommandStorageSize(0), m_SessionCreated(false),
								m_Locked(false), m_QoSEnabled(false), m_pCommandStorage(NULL), m_pCommandPool(NULL), m_pCommandList(NULL), 
								m_SessionNonce(0), m_pArbitrationRegistrationResults(NULL), m_pSkillModifiers(NULL), m_Migrating(false)
	{ 
	}
  
 
	// destructor
	C_Session::~C_Session()
	{
		CleanUp();
	}

  
	// attempt to initialize the object.
	bool C_Session::Initialise(const U32 maxPlayers, const U32 commandQueueSize)
	{
		if (m_Initialised)
		{
			LOGWARN( g_Log, "Trying to re-initialize a C_Session is not possible");
			return true;
		}
		
		if (0 == maxPlayers)
			return false;

		// store the maximun number of players we can create a session for..
		m_MaxPlayers = maxPlayers;

		ZeroMemory(&m_SessionInfo, sizeof(m_SessionInfo));
		ZeroMemory(&m_MigrationInfo, sizeof(m_MigrationInfo));

		// if we request 0 commands then allocate a default amount.
		m_CommandStorageSize		= (commandQueueSize != 0) ? commandQueueSize : 32;
	
		// allocate storage for changing session trueskill..
		m_pSkillModifiers = new XUID[m_MaxPlayers];

		//const U32 allocatedStorageSize = sizeof(C_SessionCommand)*m_CommandStorageSize;

		//allocate commands storage and linked lists..
		m_pCommandStorage = new C_SessionCommand[m_CommandStorageSize];
		m_pCommandPool = reinterpret_cast<void*> (new C_SessionCommandList);
		m_pCommandList = reinterpret_cast<void*> (new C_SessionCommandList);

		// allocate arbitration storage buffers..
		if (m_pCommandStorage && m_pCommandPool && m_pCommandList && m_pSkillModifiers)
		{
			for (U32 i=0; i<m_MaxPlayers; i++)
				m_pSkillModifiers[i] = 0;
				
			// put all normal command objects into pool 
			C_SessionCommandList* pPool = reinterpret_cast<C_SessionCommandList*> (m_pCommandPool);
			for (U32 i=0; i< m_CommandStorageSize; i++)
				pPool->PushBack(m_pCommandStorage[i]);

			m_Initialised = true;
		}
		else
			CleanUp();
	
		return m_Initialised;
	}

	bool C_Session::SetParameter(const U8 userIndex, const SessionParameter& data)
	{
		if (!m_Initialised)
			return false;
		
		if (userIndex >= XUSER_MAX_COUNT)
		{
			LOGFATAL( g_Log, "C_Session::SetParameter() - userIndex out or range");
			return false;
		}

		// pretty sure the local user must have a valid gamer profile before we can use any of the session API calls.
		const XUSER_SIGNIN_STATE s = XUserGetSigninState(userIndex);

		if (eXUserSigninState_NotSignedIn == s)
		{
			LOGWARN( g_Log, "C_Session::SetParameter() - specified user does not have a gamer profile signed in");
			return false;
		}

		bool paramSet = false;
		if (SessionParameter::eTypeProperty == data.m_ParameterType)
		{
			// try and set the property
			switch(data.m_Data.m_PropertyData.m_ValueType)
			{
			case SessionParameter::Int32:
				XUserSetProperty(userIndex, data.m_Data.m_PropertyData.m_Id, sizeof(S32), &data.m_Data.m_PropertyData.m_Value.m_S32);
				paramSet = true;
				break;
			case SessionParameter::Int64:
				XUserSetProperty(userIndex, data.m_Data.m_PropertyData.m_Id, sizeof(S64), &data.m_Data.m_PropertyData.m_Value.m_S64);
				paramSet = true;
				break;
			case SessionParameter::Float64:
				XUserSetProperty(userIndex, data.m_Data.m_PropertyData.m_Id, sizeof(F64), &data.m_Data.m_PropertyData.m_Value.m_F64);
				paramSet = true;
				break;
			case SessionParameter::Float32:
				XUserSetProperty(userIndex, data.m_Data.m_PropertyData.m_Id, sizeof(F32), &data.m_Data.m_PropertyData.m_Value.m_F32);
				paramSet = true;
				break;
			case SessionParameter::None:
				paramSet = true;
				break;
			default:
				LOGFATAL( g_Log, "C_Session::SetParameter() - trying to set an unsupported property type");
				break;
			}
		}
		else
		{
			// try and set the context.
			XUserSetContext(userIndex, data.m_Data.m_ContextData.m_Id, data.m_Data.m_ContextData.m_Value);
			if (X_CONTEXT_GAME_MODE == data.m_Data.m_ContextData.m_Id)
			{
				LOGINFO(g_Log, "C_Session : Setting XLAST game MODE to %d", data.m_Data.m_ContextData.m_Value);
			}
			if (X_CONTEXT_GAME_TYPE == data.m_Data.m_ContextData.m_Id)
			{
				LOGINFO(g_Log, "C_Session : Setting XLAST game TYPE to %d", data.m_Data.m_ContextData.m_Value);
			}
			paramSet = true;
		}
		return paramSet;
	}

	bool C_Session::Poll(void)
	{
		if (!m_Initialised)
			return true;
		
		if (eStateError != m_SessionState)
		{
			// next process any normal session commands..
			C_SessionCommandList* pList = reinterpret_cast<C_SessionCommandList*> (m_pCommandList);
			C_SessionCommandList* pPool = reinterpret_cast<C_SessionCommandList*> (m_pCommandPool);
			if (!pList->Empty())
			{
				bool startNextCommand = false;
				C_SessionCommandList::Iterator i = pList->Begin();
				if (i->m_Active)
				{
					// command is currently active so pump asynchronous task..
					if (i->m_AsyncTask.Finished())
					{
						bool commandSuccess = false;
						if (i->m_AsyncTask.ErrorCode() == ERROR_SUCCESS)
							commandSuccess = true;

						
						// if in a ranked sesion and the End Command fails, ignore it.
						// since ranked sessions can only be played once we dont need to throw an error if arbitration fails because of differing results
						if (C_SessionCommand::eCommandEndSession == i->m_Command && m_SessionParameters&SESSION_CREATE_USES_ARBITRATION)
						{
							if (i->m_AsyncTask.ErrorCode() != ERROR_SUCCESS)
							{
								LOGINFO(g_Log, "XSessionEnd() failed.. error code 0x%x", i->m_AsyncTask.ErrorCode());
							}
							commandSuccess = true;
						}
							

						if (commandSuccess)
						{
							i->m_AsyncTask.Close();

							// command completed successfully.. modify object data to reflect certain commands have completed.
							switch (i->m_Command)
							{
							case C_SessionCommand::eCommandCreateSession:
								m_SessionState		= eStateRunning;	// yippee session is now available..
								m_SessionCreated	= true;				// we must delete the session and not just close the handle..
								break;
							
							case C_SessionCommand::eCommandAddPlayer:
								//LOGINFO(g_Log, "Add complete");
								{
									SessionCommandNotification* p = reinterpret_cast<SessionCommandNotification*> (i->m_CommandData.m_Add.m_pNotification);
									if (p)
									{
										p->Reset();
										p->m_Finished = true;
										p->m_Id = i->m_CommandData.m_Add.m_NotificationId;
									}
								}
								break;

							case C_SessionCommand::eCommandRemovePlayer:
								//LOGINFO(g_Log, "Remove complete");
								{
									SessionCommandNotification* p = reinterpret_cast<SessionCommandNotification*> (i->m_CommandData.m_Remove.m_pNotification);
									if (p)
									{
										p->Reset();
										p->m_Finished = true;
										p->m_Id = i->m_CommandData.m_Remove.m_NotificationId;
									}
								}
								break;

							case C_SessionCommand::eCommandModifySession:
								{
									SessionCommandNotification* p = reinterpret_cast<SessionCommandNotification*> (i->m_CommandData.m_Modify.m_pNotification);
									if (p)
									{
										p->Reset();
										p->m_Finished = true;
										p->m_Id = i->m_CommandData.m_Modify.m_NotificationId;
									}
								}
								break;

							case C_SessionCommand::eCommandArbitrationRegistration:
								{
									SessionCommandNotification* p = reinterpret_cast<SessionCommandNotification*> (i->m_CommandData.m_ArbitrationRegistration.m_pNotification);
									if (p)
									{
										p->Reset();
										p->m_Finished = true;
										p->m_Id = i->m_CommandData.m_ArbitrationRegistration.m_NotificationId;
									}

								}
								break;

							case C_SessionCommand::eCommandMigrateSession:
								{
									SessionCommandNotification* p = reinterpret_cast<SessionCommandNotification*> (i->m_CommandData.m_Migrate.m_pNotification);
									if (p)
									{
										p->Reset();
										p->m_Finished = true;
										p->m_Id = i->m_CommandData.m_Migrate.m_NotificationId;
									}
								}
								m_Migrating = false;

								// copy migrated session to mains ession..
								m_SessionInfo = m_MigrationInfo;

								//Dump(m_MigrationInfo);
								break;

							case C_SessionCommand::eCommandModifySkill:
								{
									SessionCommandNotification* p = reinterpret_cast<SessionCommandNotification*> (i->m_CommandData.m_ModifySkill.m_pNotification);
									if (p)
									{
										p->Reset();
										p->m_Finished = true;
										p->m_Id = i->m_CommandData.m_ModifySkill.m_NotificationId;
									}
								}
								break;

							case C_SessionCommand::eCommandStartSession:
								// yippee session is now playing
								m_SessionState = eStatePlaying;	

								// release arbitration registration results if they where used.. since they shouldnt be referenced after starting the session.
								if (m_pArbitrationRegistrationResults)
									delete [] m_pArbitrationRegistrationResults;
								m_pArbitrationRegistrationResults = NULL;
								break;

							case C_SessionCommand::eCommandEndSession:
								m_Locked = false;				
								m_SessionState = eStateRunning;	// yippee session is now idle again
								break;

							default:
								LOGFATAL(g_Log, "C_Session : Invalid Session Command Type : %d", i->m_Command);
								break;
							}

							// now give finished command back to pool and flag next command to start
							C_SessionCommandList::Iterator cur = i;
							i = pList->Erase(cur);
							pPool->PushBack(*cur);
							startNextCommand = true;
						}
						else
						{
							LOGWARN( g_Log, "C_Session : Command failed. Type :%d  ErrorCode : 0x%x", i->m_Command, i->m_AsyncTask.ErrorCode());
							// crap.. command failed. 
							i->m_AsyncTask.Close();
							m_SessionState = eStateError;	// stop us adding new commands to the queue..

							// release arbitration registration buffer if it exixts
							if (m_pArbitrationRegistrationResults)
								delete [] m_pArbitrationRegistrationResults;
							m_pArbitrationRegistrationResults = NULL;
							
							// give all active commands back to the pool..
							while (!pList->Empty())
							{
								C_SessionCommandList::Iterator f = pList->Begin();
								pList->Erase(f);
								pPool->PushBack(*f);
							}
						}
					}
				}
				else
					startNextCommand = true;

				if (startNextCommand)
				{
					if (!pList->Empty())
					{
						// get head of list and start next command..
						DWORD retCode = ERROR_SUCCESS;

						i = pList->Begin();
						switch (i->m_Command)
						{
						case C_SessionCommand::eCommandCreateSession:
							// delay creation if sessions are still being deleted.
							if (C_SessionDeletion::Instance().ActiveCount()  == 0)
							{
								retCode = XSessionCreate(m_SessionParameters, m_UserIndex, m_PublicSlots, m_PrivateSlots, &m_SessionNonce, &m_SessionInfo, &i->m_AsyncTask, &m_SessionHandle);
								if (ERROR_IO_PENDING == retCode)
									i->m_Active = true;
								else
								{
									// flag error and flush list.. session handle wont be valid if this function fails..
									m_SessionState = eStateError;
									while (!pList->Empty())
									{
										C_SessionCommandList::Iterator f = pList->Begin();
										pList->Erase(f);
										pPool->PushBack(*f);
									}
								}
							}
							else
							{
								LOGWARN(g_Log, "C_Session::Poll() - Delaying session creation until previous sessions are deleted.");
							}
							break;

						case C_SessionCommand::eCommandAddPlayer:
							retCode = XSessionJoinRemote(m_SessionHandle, i->m_CommandData.m_Add.m_NumToAdd, &(i->m_CommandData.m_Add.m_Xuids[0]), &(i->m_CommandData.m_Add.m_PrivateSlots[0]), &(i->m_AsyncTask));
							if (ERROR_IO_PENDING == retCode)
								i->m_Active = true;
							else
								m_SessionState = eStateError; // flag error.. no more commands will work in this state
							break;

						case C_SessionCommand::eCommandRemovePlayer:
							retCode = XSessionLeaveRemote(m_SessionHandle, i->m_CommandData.m_Remove.m_NumToRemove, &(i->m_CommandData.m_Remove.m_Xuids[0]), &(i->m_AsyncTask));
							if (ERROR_IO_PENDING == retCode)
								i->m_Active = true;
							else
								m_SessionState = eStateError; // flag error.. no more commands will work in this state
							break;

						case C_SessionCommand::eCommandModifySession:
							retCode = XSessionModify(m_SessionHandle, i->m_CommandData.m_Modify.m_SessionFlags, i->m_CommandData.m_Modify.m_PublicSlotCount, i->m_CommandData.m_Modify.m_PrivateSlotCount, &(i->m_AsyncTask));
							if (ERROR_IO_PENDING == retCode)
								i->m_Active = true;
							else
								m_SessionState = eStateError; // flag error.. no more commands will work in this state
							break;

						case C_SessionCommand::eCommandArbitrationRegistration:
							{
								assert(m_pArbitrationRegistrationResults == NULL);	// should have no allocated buffer when we get here..
								
								// work out size of buffer required to hold arbitration registration results..
								DWORD regBufferSize = 0;
								const DWORD ret = XSessionArbitrationRegister(m_SessionHandle, 0, i->m_CommandData.m_ArbitrationRegistration.m_ArbitrationNonce, &regBufferSize, NULL, NULL);
								if (ERROR_INSUFFICIENT_BUFFER == ret)
								{
									m_pArbitrationRegistrationResults = new BYTE[regBufferSize];
									if (m_pArbitrationRegistrationResults)
									{
										retCode = XSessionArbitrationRegister(m_SessionHandle, 0, i->m_CommandData.m_ArbitrationRegistration.m_ArbitrationNonce, &regBufferSize, (XSESSION_REGISTRATION_RESULTS*)m_pArbitrationRegistrationResults, &(i->m_AsyncTask));
										if (ERROR_IO_PENDING == retCode)
											i->m_Active = true;
										else
										{
											delete [] m_pArbitrationRegistrationResults;
											m_pArbitrationRegistrationResults = NULL;
											m_SessionState = eStateError; // flag error.. no more commands will work in this state
										}
									}
									else
										m_SessionState = eStateError; // flag error.. no more commands will work in this state
								}
								else
									m_SessionState = eStateError; // flag error.. no more commands will work in this state
							}
							break;

						case C_SessionCommand::eCommandMigrateSession:						
#if _XDK_VER >= 4929
							retCode = XSessionMigrateHost(m_SessionHandle, i->m_CommandData.m_Migrate.m_NewHost ? m_UserIndex : XUSER_INDEX_NONE, &m_MigrationInfo, &(i->m_AsyncTask));
							if (ERROR_IO_PENDING == retCode)
								i->m_Active = true;
							else
								m_SessionState = eStateError; // flag error.. no more commands will work in this state
#endif
							break;

						case C_SessionCommand::eCommandModifySkill:
#if _XDK_VER >= 4929
							retCode = XSessionModifySkill(m_SessionHandle, i->m_CommandData.m_ModifySkill.m_NumPlayers, m_pSkillModifiers, &(i->m_AsyncTask));
							if (ERROR_IO_PENDING == retCode)
								i->m_Active = true;
							else
								m_SessionState = eStateError; // flag error.. no more commands will work in this state
#endif
							break;

						case C_SessionCommand::eCommandStartSession:
							LOGINFO(g_Log, "C_Session : XSessionStart() called");
							retCode = XSessionStart(m_SessionHandle, 0, &(i->m_AsyncTask));
							if (ERROR_IO_PENDING == retCode)
								i->m_Active = true;
							else
								m_SessionState = eStateError; // flag error.. no more commands will work in this state
							break;

						case C_SessionCommand::eCommandEndSession:
							LOGINFO(g_Log, "C_Session : XSessionEnd() called");
							retCode = XSessionEnd(m_SessionHandle, &(i->m_AsyncTask));
							if (ERROR_IO_PENDING == retCode)
								i->m_Active = true;
							else
								m_SessionState = eStateError; // flag error.. no more commands will work in this state
							break;

						default:
							LOGFATAL( g_Log, "C_Session::Poll() - Invalid Command Type : %d", i->m_Command);
							break;
						}
					}
				}			
			}
		}
		return m_SessionState != eStateError;
	}

	bool C_Session::Create(const U8 userIndex, const U32 sessionFlags, const SessionParameter* pSessionParameters, const U32 numSessionParameters, const SlotConfig& slotConfig)
	{
		if (!m_Initialised)
			return false;
		
		if (!C_SessionDeletion::Instance().IsActive())
		{
			LOGFATAL(g_Log, "can't create online sessions until the C_SessionDeletion class has been configured.");
			return false;
		}

		if (C_SessionDeletion::Instance().Remaining() == 0)
		{
			LOGERROR(g_Log, "can't create online sessions as there is no available deletion slot within C_SessionDeletion.");
			return false;
		}

		if (!slotConfig.Validate(m_MaxPlayers))
		{
			LOGWARN(g_Log, "C_Session::Create()  - Unable to create session, slot configuration is invalid.");
			return false;
		}

		if (eStateNotActive != m_SessionState)
			return false; // can't create new session until previous one is deleted.

		if (userIndex >= XUSER_MAX_COUNT)
		{
			LOGFATAL( g_Log, "C_Session::Create() - userIndex out or range");
			return false;
		}

		// pretty sure the local user must have a valid gamer profile before we can use any of the session API calls.
		const XUSER_SIGNIN_STATE s = XUserGetSigninState(userIndex);
		if (eXUserSigninState_SignedInToLive != s)
		{
			LOGWARN( g_Log, "C_Session::Create() - specified user is not connected to Xbox Live");
			return false;
		}

		// allocate a command object and setup for creating session.
		C_SessionCommandList* pList = reinterpret_cast<C_SessionCommandList*> (m_pCommandList);
		C_SessionCommandList* pPool = reinterpret_cast<C_SessionCommandList*> (m_pCommandPool);
		if (pPool->Empty())
			return false;

		// if we are hosting a new session then zero out this structure otherwise it will be already filled in prior to entering this function.
		if (m_SessionParameters&SESSION_CREATE_HOST)
			ZeroMemory(&m_SessionInfo, sizeof(m_SessionInfo));
		ZeroMemory(&m_MigrationInfo, sizeof(m_MigrationInfo));

		// null out handle as it will be created when we call XSessionCreate()
		m_SessionHandle		=	NULL;

		m_UserIndex			=	userIndex;
		m_PublicSlots		=	slotConfig.m_MaxPublic;
		m_PrivateSlots		=	slotConfig.m_MaxPrivate;
		m_SessionParameters	=	sessionFlags;

		C_SessionCommandList::Iterator it = pPool->Begin();
		pPool->Erase(pPool->Begin());
		it->m_Command		=	C_SessionCommand::eCommandCreateSession;
		it->m_Active		=	false;
		pList->PushBack(*it);

		// flag session state as now creating;
		m_SessionState =	eStateCreating;
		
		// set all session parameters (contexts and properties) if any have been speicfied before session is actually created on the host box..
		if (NULL != pSessionParameters && numSessionParameters > 0)
		{
			for (U32 i=0; i<numSessionParameters; i++)
				SetParameter(userIndex, pSessionParameters[i]);
		}

		return true;
	}

	bool C_Session::CreatePeer(const U8 userIndex, const U32 sessionFlags, const SessionParameter* pSessionParameters, const U32 numSessionParameters, const SessionSecurity& securityData, const SlotConfig& slotConfig)
	{
		SysProfile::GameInvite inv;
		ZeroMemory(&inv, sizeof(inv));

		// blag a gamem invite and use that version of create peer
		inv.m_Active								= true;
		inv.m_InviteData.hostInfo.sessionID			= securityData.m_Id;
		inv.m_InviteData.hostInfo.keyExchangeKey	= securityData.m_Key;

		return CreatePeer(userIndex, sessionFlags,pSessionParameters, numSessionParameters, inv, slotConfig);
	}

	bool C_Session::CreatePeer(const U8 userIndex, const U32 sessionFlags, const SessionParameter* pSessionParameters, const U32 numSessionParameters, const C_SessionEnumerator& availableSessions, const U32 sessionIndexToJoin, const SlotConfig& slotConfig)
	{
		if (!m_Initialised)
			return false;

		if (userIndex >= XUSER_MAX_COUNT)
		{
			LOGFATAL( g_Log, "C_Session::Create() - userIndex out or range");
			return false;
		}

		// pretty sure the local user must have a valid gamer profile before we can use any of the session API calls.
		const XUSER_SIGNIN_STATE s = XUserGetSigninState(userIndex);
		if (eXUserSigninState_SignedInToLive != s)
		{
			LOGWARN( g_Log, "C_Session::CreatePeer() - specified user does is not connected to Xbox Live");
			return false;
		}

		// peers cant create sessions if the specify the hosting flag...
		if (sessionFlags&SESSION_CREATE_HOST)
		{
			LOGFATAL(g_Log,"C_Session::CreatePeer() - Can't create a peer session with the host flag specified");
			return false;
		}
  
		// can't create new session until previous one is deleted.
		if (eStateNotActive != m_SessionState)
			return false; 

		// have we actually enumerated sessions from server ?
		if (sessionIndexToJoin >= availableSessions.m_NumSessionEnumerated)
			return false;

		// is session data still valid
		if (availableSessions.m_SearchState != C_SessionEnumerator::eSearchNotActive || NULL == availableSessions.m_pEnumeratedSessionData)
			return false;

		const XSESSION_SEARCHRESULT_HEADER* pSearch = (XSESSION_SEARCHRESULT_HEADER*)availableSessions.m_pEnumeratedSessionData;

		m_SessionNonce = 0;

		// secure key details..
		m_SessionInfo	=	pSearch->pResults[sessionIndexToJoin].info;
		
		return Create(userIndex, sessionFlags, pSessionParameters, numSessionParameters, slotConfig);
	}

	bool C_Session::CreatePeer(const U8 userIndex, const U32 sessionFlags, const SessionParameter* pSessionParameters, const U32 numSessionParameters, SysProfile::GameInvite& inviteData, const SlotConfig& slotConfig)
	{
		if (!m_Initialised)
			return false;

		if (userIndex >= XUSER_MAX_COUNT)
		{
			LOGFATAL( g_Log, "C_Session::CreatePeer() - userIndex out or range");
			return false;
		}

		// pretty sure the local user must have a valid gamer profile before we can use any of the session API calls.
		const XUSER_SIGNIN_STATE s = XUserGetSigninState(userIndex);
		if (eXUserSigninState_SignedInToLive != s)
		{
			LOGWARN( g_Log, "C_Session::Create() - specified user is not connected to Xbox Live");
			return false;
		}

		// peers cant create sessions if the specify the hosting flag...
		if (sessionFlags&SESSION_CREATE_HOST)
		{
			LOGFATAL(g_Log,"C_Session::CreatePeer() - Can't create a peer session with the host flag specified");
			return false;
		}
  
		// can't create new session until previous one is deleted.
		if (eStateNotActive != m_SessionState)
			return false; 

		if (!inviteData.m_Active)
			return false;

		// set session data on what we want to join.
		m_SessionInfo = inviteData.m_InviteData.hostInfo;
		m_SessionNonce = 0;

		return Create(userIndex, sessionFlags, pSessionParameters, numSessionParameters, slotConfig);
	}

	bool C_Session::EnableQos(const U32 qosBandwidth, U8* pData, const U32 dataSize)
	{
		if (!m_Initialised)
			return false;

		if (!m_SessionCreated)
			return false;
		
		if (m_Migrating)
			return false;   // cant enable QoS while we are migrating session.

		S32 res = 0;
		if (pData && dataSize)
			res = XNetQosListen(&m_SessionInfo.sessionID, (const BYTE*)pData, dataSize, qosBandwidth, XNET_QOS_LISTEN_ENABLE|XNET_QOS_LISTEN_SET_BITSPERSEC|XNET_QOS_LISTEN_SET_DATA);
		else
			res = XNetQosListen(&m_SessionInfo.sessionID, NULL, 0, qosBandwidth, XNET_QOS_LISTEN_ENABLE|XNET_QOS_LISTEN_SET_BITSPERSEC);
			
		if (0 == res)
		{
			m_QoSEnabled = true;
			return true;
		}

		return false;
	}
	
	bool C_Session::DisableQos(void)
	{
		if (!m_Initialised)
			return false;

		if (!m_QoSEnabled)
			return false;

		if (!m_SessionCreated)
			return false;

		if (XNetQosListen(&m_SessionInfo.sessionID, NULL, 0, 0, XNET_QOS_LISTEN_DISABLE) == 0)
		{
			m_QoSEnabled = false;
			return true;
		}
		
		return false;
	}

	bool C_Session::Modify(const U32 modifiedSessionFlags, const SlotConfig& slotConfig, const SessionParameter* pSessionParameters, const U32 numSessionParameters, SessionCommandNotification* pCompleteNotification)
	{
		if (!m_Initialised)
			return false;
		
		if (eStateNotActive == m_SessionState || eStateError == m_SessionState || !m_SessionCreated)
			return false;  // session not in a  state where we can modify

		if (!slotConfig.Validate(m_MaxPlayers))
		{
			LOGWARN(g_Log, "C_Session::Modify() - Unable to modify session as slot configuation is invalid.");
			return false;
		}
	
		C_SessionCommandList* pList = reinterpret_cast<C_SessionCommandList*> (m_pCommandList);
		C_SessionCommandList* pPool = reinterpret_cast<C_SessionCommandList*> (m_pCommandPool);

		if (pPool->Empty())
		{
			m_SessionState = eStateError;
			return false;
		}

		// set new session parameters, this will be sent to server when modify command is executed.
		if (0 != modifiedSessionFlags)
			m_SessionParameters =  modifiedSessionFlags;

		// allocate object and configure
		C_SessionCommandList::Iterator i = pPool->Begin();
		pPool->Erase(pPool->Begin());
		i->m_Command								=	C_SessionCommand::eCommandModifySession;
		i->m_CommandData.m_Modify.m_PublicSlotCount	=	slotConfig.m_MaxPublic;
		i->m_CommandData.m_Modify.m_PrivateSlotCount=	slotConfig.m_MaxPrivate;
		i->m_CommandData.m_Modify.m_SessionFlags	=	m_SessionParameters;
		if (NULL == pCompleteNotification)
		{
			i->m_CommandData.m_Modify.m_pNotification	=	NULL;
			i->m_CommandData.m_Modify.m_NotificationId	=	0;
		}
		else
		{
			i->m_CommandData.m_Modify.m_pNotification	=	pCompleteNotification;
			i->m_CommandData.m_Modify.m_NotificationId	=	pCompleteNotification->m_Id;
		}

		// update session attributes.
		for (U32 i=0; i<numSessionParameters; i++)
			SetParameter(m_UserIndex, pSessionParameters[i]);

		m_PublicSlots	= slotConfig.m_MaxPublic;
		m_PrivateSlots	= slotConfig.m_MaxPrivate;

		// clear the passed in notifiction structure.
		if (NULL != pCompleteNotification)
			pCompleteNotification->Reset();

		LOGINFO(g_Log, "C_Session::Modify() - params:%d public:%d private:%d", modifiedSessionFlags, m_PublicSlots, m_PrivateSlots);

		// add to list for asynchronous processing.
		pList->PushBack(*i);
		return true;
	}


	// Starts the process of deleting the active session.
	void C_Session::Delete(void)
	{
		if (!m_Initialised)
			return;

		if (eStateNotActive == m_SessionState)
			return; // no session currently running.

		// stiop Qos Listener if its active.
		DisableQos();

		if (!m_SessionCreated)
		{
			// stop all active tasks and close session handle, since session was not successfully created.
			C_SessionCommandList* pList = reinterpret_cast<C_SessionCommandList*> (m_pCommandList);
			C_SessionCommandList* pPool = reinterpret_cast<C_SessionCommandList*> (m_pCommandPool);
			while (!pList->Empty())
			{
				C_SessionCommandList::Iterator i = pList->Begin();
				if (i->m_Active)
					i->m_AsyncTask.Close();

				// give back to pool;
				C_SessionCommandList::Iterator c = i;
				i = pList->Erase(c);
				pPool->PushBack(*c);
			}

			// close session handle if valid..
			if (NULL != m_SessionHandle && INVALID_HANDLE_VALUE != m_SessionHandle)
				CloseHandle(m_SessionHandle);
		}
		else
		{
			// stop all active tasks and start deletion process..
			C_SessionCommandList* pList = reinterpret_cast<C_SessionCommandList*> (m_pCommandList);
			C_SessionCommandList* pPool = reinterpret_cast<C_SessionCommandList*> (m_pCommandPool);
			while (!pList->Empty())
			{
				C_SessionCommandList::Iterator i = pList->Begin();
				if (i->m_Active)
					i->m_AsyncTask.Close();

				// give back to pool;
				C_SessionCommandList::Iterator c = i;
				i = pList->Erase(c);
				pPool->PushBack(*c);
			}

			// if arbitration results buffer has been created release it here
			if (m_pArbitrationRegistrationResults)
				delete [] m_pArbitrationRegistrationResults;
			m_pArbitrationRegistrationResults = NULL;

			// queue session for deletion.
			C_SessionDeletion::Instance().Add(*this);
		}

		// reset data to make it possible to create another session, even if the current one is deleting.
		m_SessionCreated			=	false;
		m_SessionState				=	eStateNotActive;
		m_SessionHandle				=	NULL;
		m_SessionParameters			=	0;
		m_PublicSlots				=	0;
		m_PrivateSlots				=	0;
		m_UserIndex					=	0;
		m_Locked					=	false;
		m_QoSEnabled				=	false;
		m_Migrating					=	false;
	}

	bool C_Session::AddPlayer(const SysProfile::UserIdentifier& player, const bool usePublicSlot, SessionCommandNotification* pCompleteNotification)
	{
		if (!m_Initialised)
			return false;

		if (eStateNotActive == m_SessionState || eStateError == m_SessionState || !m_SessionCreated)
			return false;  // session not in a  state where we can add players..

		if (0 == player.m_Uid)
		{
			LOGFATAL(g_Log, "C_Session::AddPlayer() - invalid player Identifier");
			return false; // invalid player
		}

		if (m_Locked)
			return false; // session is locked for adding new players..

		C_SessionCommandList* pList = reinterpret_cast<C_SessionCommandList*> (m_pCommandList);
		C_SessionCommandList* pPool = reinterpret_cast<C_SessionCommandList*> (m_pCommandPool);

		if (pPool->Empty())
		{
			// no command objects left in pool..
			m_SessionState = eStateError;
			return false; 
		}

		// allocate object and configure
		C_SessionCommandList::Iterator i = pPool->Begin();
		pPool->Erase(pPool->Begin());
		i->m_Command								=	C_SessionCommand::eCommandAddPlayer;
		i->m_CommandData.m_Add.m_PrivateSlots[0]	=	!usePublicSlot;
		i->m_CommandData.m_Add.m_Xuids[0]			=	player.m_Uid;
		i->m_CommandData.m_Add.m_NumToAdd			=	1;

		if (NULL == pCompleteNotification)
		{
			i->m_CommandData.m_Add.m_pNotification	=	NULL;
			i->m_CommandData.m_Add.m_NotificationId	=	0;
		}
		else
		{
			i->m_CommandData.m_Add.m_pNotification	=	pCompleteNotification;
			i->m_CommandData.m_Add.m_NotificationId	=	pCompleteNotification->m_Id;

			// reset notification now as we have copied requested data for processing later.
			pCompleteNotification->Reset();
		}

		//LOGINFO(g_Log, "Request to add 0x%x to session", player);

		// add to list for asynchronous processing.
		pList->PushBack(*i);
		return true;
	}

	bool C_Session::AddPlayers(const C_Session::SessionUserList& playerList, SessionCommandNotification* pCompleteNotification)
	{
		if (!m_Initialised)
			return false;

		if (eStateNotActive == m_SessionState || eStateError == m_SessionState || !m_SessionCreated)
			return false;  // session not in a  state where we can add players..
		
		if (playerList.Size() == 0)
			return false;

		if (m_Locked)
			return false; // session is locked for adding new players..

		C_SessionCommandList* pList = reinterpret_cast<C_SessionCommandList*> (m_pCommandList);
		C_SessionCommandList* pPool = reinterpret_cast<C_SessionCommandList*> (m_pCommandPool);

		if (pPool->Empty())
		{
			// no command objects left in pool..
			m_SessionState = eStateError;
			return false; 
		}

		// allocate object and configure
		C_SessionCommandList::Iterator i = pPool->Begin();
		pPool->Erase(pPool->Begin());

		i->m_Command						=	C_SessionCommand::eCommandAddPlayer;
		i->m_CommandData.m_Add.m_NumToAdd	=	playerList.Size();

		for (S32 q=0; q<playerList.Size(); q++)
		{	
			const SessionUser& su = playerList[q];
			i->m_CommandData.m_Add.m_PrivateSlots[q]	=	!su.m_PublicSlot;
			i->m_CommandData.m_Add.m_Xuids[q]			=	su.m_User.m_Uid;
		}

		if (NULL == pCompleteNotification)
		{
			i->m_CommandData.m_Add.m_pNotification	=	NULL;
			i->m_CommandData.m_Add.m_NotificationId	=	0;
		}
		else
		{
			i->m_CommandData.m_Add.m_pNotification	=	pCompleteNotification;
			i->m_CommandData.m_Add.m_NotificationId	=	pCompleteNotification->m_Id;

			// reset notification now as we have copied requested data for processing later.
			pCompleteNotification->Reset();
		}
		
		// add to list for asynchronous processing.
		pList->PushBack(*i);
		return true;
	}

	bool C_Session::RemovePlayer(const SysProfile::UserIdentifier& player, SessionCommandNotification* pCompleteNotification)
	{
		if (!m_Initialised)
			return false;

		if (eStateNotActive == m_SessionState || eStateError == m_SessionState || !m_SessionCreated)
			return false;  // session not in a  state where we can add players..

		if (0 == player.m_Uid)
		{
			LOGFATAL(g_Log, "C_Session::RemovePlayer() - invalid player Identifier");
			return false; // invalid player
		}

		if (m_Locked)
			return false;	// session is locked for removing any curent players.

		C_SessionCommandList* pList = reinterpret_cast<C_SessionCommandList*> (m_pCommandList);
		C_SessionCommandList* pPool = reinterpret_cast<C_SessionCommandList*> (m_pCommandPool);

		if (pPool->Empty())
		{
			// no command objects left in pool..
			m_SessionState = eStateError;
			return false;
		}

		// allocate object and configure
		C_SessionCommandList::Iterator i	=	pPool->Begin(); 
		pPool->Erase(pPool->Begin());
		i->m_Command							=	C_SessionCommand::eCommandRemovePlayer;
		i->m_CommandData.m_Remove.m_Xuids[0]	=	player.m_Uid;
		i->m_CommandData.m_Remove.m_NumToRemove	=	1;

		if (NULL == pCompleteNotification)
		{
			i->m_CommandData.m_Remove.m_pNotification	=	NULL;
			i->m_CommandData.m_Remove.m_NotificationId	=	0;
		}
		else
		{
			i->m_CommandData.m_Remove.m_pNotification	=	pCompleteNotification;
			i->m_CommandData.m_Remove.m_NotificationId	=	pCompleteNotification->m_Id;

			// reset notification now as we have copied requested data for processing later.
			pCompleteNotification->Reset();
		}

		// add to list for asynchronous processing.
		pList->PushBack(*i);
		return true;
	}

	bool C_Session::RemovePlayers(const C_Session::SessionUserList& playerList, SessionCommandNotification* pCompleteNotification)
	{
		if (!m_Initialised)
			return false;

		if (eStateNotActive == m_SessionState || eStateError == m_SessionState || !m_SessionCreated)
			return false;  // session not in a  state where we can add players..
		
		if (playerList.Size() == 0)
			return false;

		if (m_Locked)
			return false;	// session is locked for removing any curent players.

		C_SessionCommandList* pList = reinterpret_cast<C_SessionCommandList*> (m_pCommandList);
		C_SessionCommandList* pPool = reinterpret_cast<C_SessionCommandList*> (m_pCommandPool);

		if (pPool->Empty())
		{
			// no command objects left in pool..
			m_SessionState = eStateError;
			return false;
		}

		// allocate object and configure
		C_SessionCommandList::Iterator i	=	pPool->Begin(); 
		pPool->Erase(pPool->Begin());

		i->m_Command							=	C_SessionCommand::eCommandRemovePlayer;
		i->m_CommandData.m_Remove.m_NumToRemove	=	playerList.Size();

		for (S32 q=0; q<playerList.Size(); q++)
			i->m_CommandData.m_Remove.m_Xuids[q] =	playerList[q].m_User.m_Uid;

		if (NULL == pCompleteNotification)
		{
			i->m_CommandData.m_Remove.m_pNotification	=	NULL;
			i->m_CommandData.m_Remove.m_NotificationId	=	0;
		}
		else
		{
			i->m_CommandData.m_Remove.m_pNotification	=	pCompleteNotification;
			i->m_CommandData.m_Remove.m_NotificationId	=	pCompleteNotification->m_Id;

			// reset notification now as we have copied requested data for processing later.
			pCompleteNotification->Reset();
		}

		// add to list for asynchronous processing.
		pList->PushBack(*i);
		return true;
	}

	// If the original host has left the session then one of the peers can migrate himself to be the ownner (ie: new host) of the session.
	bool C_Session::Migrate(SessionCommandNotification* pCompleteNotification, const SessionParameter* pSessionParameters, const U32 numSessionParameters, const SlotConfig& slotConfig)
	{
#if _XDK_VER < 4929
		LOGINFO(g_Log, "C_Session::Migrate() - Not Available on this version of the XDK.. upgrade....");
		return false;
#endif
		if (!m_Initialised)
			return false;

		if (eStateNotActive == m_SessionState || eStateError == m_SessionState || eStateCreating == m_SessionState || !m_SessionCreated)
		{
			LOGINFO(g_Log, "C_Session::Migrate() - Can't migrate when session is in an error state(created=%d, state=%d).", m_SessionCreated, m_SessionState);
			return false;  // session not in a good state
		}

		// allocate a command object and setup for creating session.
		C_SessionCommandList* pList = reinterpret_cast<C_SessionCommandList*> (m_pCommandList);
		C_SessionCommandList* pPool = reinterpret_cast<C_SessionCommandList*> (m_pCommandPool);
		if (pPool->Empty())
		{
			m_SessionState = eStateError;
			return false;
		}

		C_SessionCommandList::Iterator it = pPool->Begin();
		pPool->Erase(pPool->Begin());
		it->m_Command		=	C_SessionCommand::eCommandMigrateSession;
		it->m_Active		=	false;
		it->m_CommandData.m_Migrate.m_NewHost =	true;
		if (NULL != pCompleteNotification)
		{
			it->m_CommandData.m_Migrate.m_pNotification	= pCompleteNotification;
			it->m_CommandData.m_Migrate.m_NotificationId = pCompleteNotification->m_Id;
		}
		else
		{
			it->m_CommandData.m_Migrate.m_pNotification	= NULL;
			it->m_CommandData.m_Migrate.m_NotificationId = 0;
		}

		// add to list for asynchronous processing.
		pList->PushBack(*it);

		// copy old key so we can still use this to register players until migrate is complete
		m_Migrating = true;

		// clear session before host migrate..
		ZeroMemory(&m_Migrating, sizeof(m_Migrating));

		// copy current session data
		//m_MigrationInfo = m_SessionInfo;
	
		DisableQos();

		// change the session flags so this player now becomes the host
		m_SessionParameters |= SESSION_CREATE_HOST; 

		//Dump(m_SessionInfo);

		if (NULL != pCompleteNotification)
			pCompleteNotification->Reset();

		return true;
	}

	bool C_Session::GetMigrationInfo(MigrationInfo& outInfo)
	{
		if (!m_Initialised)
			return false;
		
		if (eStateNotActive == m_SessionState || !m_SessionCreated)
			return false;  // session not in a good state

		outInfo.m_SessionInfo = m_MigrationInfo;
		return true;
	}

	bool C_Session::MigratePeer(const MigrationInfo& migrationInfo, SessionCommandNotification* pCompleteNotification, const SlotConfig& slotConfig)
	{
#if _XDK_VER < 4929
		LOGINFO(g_Log, "C_Session::Migrate() - Not Available on this version of the XDK.. upgrade....");
		return false;
#endif
		if (!m_Initialised)
			return false;

		if (eStateNotActive == m_SessionState || eStateError == m_SessionState || !m_SessionCreated)
			return false;  // session not in a good state
	
		// allocate a command object and setup for creating session.
		C_SessionCommandList* pList = reinterpret_cast<C_SessionCommandList*> (m_pCommandList);
		C_SessionCommandList* pPool = reinterpret_cast<C_SessionCommandList*> (m_pCommandPool);
		if (pPool->Empty())
		{
			m_SessionState = eStateError;
			return false;
		}

		C_SessionCommandList::Iterator it = pPool->Begin();
		pPool->Erase(pPool->Begin());
		it->m_Command		=	C_SessionCommand::eCommandMigrateSession;
		it->m_Active		=	false;
		it->m_CommandData.m_Migrate.m_NewHost =	false;
		if (NULL != pCompleteNotification)
		{
			it->m_CommandData.m_Migrate.m_pNotification	= pCompleteNotification;
			it->m_CommandData.m_Migrate.m_NotificationId = pCompleteNotification->m_Id;
		}
		else
		{
			it->m_CommandData.m_Migrate.m_pNotification	= NULL;
			it->m_CommandData.m_Migrate.m_NotificationId = 0;
		}

		// add to list for asynchronous processing.
		pList->PushBack(*it);

		// copy old key so we can still use this to register players until migrate is complete
		m_Migrating = true;
					
		// copy data supplied by host.
		m_MigrationInfo	= migrationInfo.m_SessionInfo;
		
		DisableQos();

		//Dump(m_MigrationInfo);

		if (NULL != pCompleteNotification)
			pCompleteNotification->Reset();

		return true;
	}

	bool C_Session::ModifySkill(SessionCommandNotification* pCompleteNotification, const U8 numPlayers, const SysProfile::UserIdentifier* pPlayers)
	{
#if _XDK_VER < 4929
		LOGINFO(g_Log, "C_Session::ModifySkill() - Not Available on this version of the XDK.. upgrade....");
		return false;
#endif

		if (!m_Initialised)
			return false;

		if (eStateNotActive == m_SessionState || eStateError == m_SessionState || !m_SessionCreated)
			return false;  // session not in a  state where we can register for arbitration..
		
		assert(pCompleteNotification);
		assert(pPlayers);
		assert(numPlayers);
		assert(numPlayers <= m_MaxPlayers);

		if (0 == numPlayers || NULL == pCompleteNotification || NULL == pPlayers || numPlayers > m_MaxPlayers)
			return false;
		
		C_SessionCommandList* pList = reinterpret_cast<C_SessionCommandList*> (m_pCommandList);
		C_SessionCommandList* pPool = reinterpret_cast<C_SessionCommandList*> (m_pCommandPool);
		if (pPool->Empty())
		{
			m_SessionState = eStateError;
			return false;
		}

		// copy players who will count towards session skill into local cache..
		for (U32 i=0; i<numPlayers; i++)
			m_pSkillModifiers[i] = pPlayers[i].m_Uid;

		// check current command list, if we already have a skill modify command in the list bail out..
		if (!pList->Empty())
		{
			C_SessionCommandList::Iterator i;
			for (i=pList->Begin(); i!=pList->End(); ++i)
			{
				if (C_SessionCommand::eCommandModifySkill == i->m_Command)
					return false;
			}
		}
		
		// allocate object and configure
		C_SessionCommandList::Iterator i =	pPool->Begin(); 
		pPool->Erase(pPool->Begin());
		i->m_Command										=	C_SessionCommand::eCommandModifySkill;
		i->m_CommandData.m_ModifySkill.m_NotificationId		=	pCompleteNotification->m_Id;
		i->m_CommandData.m_ModifySkill.m_NumPlayers			=	numPlayers;
		i->m_CommandData.m_ModifySkill.m_pNotification		=	pCompleteNotification;

		// add to list for asynchronous processing.
		pList->PushBack(*i);

		// reset notification to game
		pCompleteNotification->Reset();

		return true;
	}

	bool C_Session::RegisterForArbitration(const ArbitrationNonce& arbitrationNonce, SessionCommandNotification* pCompleteNotification)
	{
		if (!m_Initialised)
			return false;

		if (eStateNotActive == m_SessionState || eStateError == m_SessionState || !m_SessionCreated)
			return false;  // session not in a  state where we can register for arbitration..

		if (!(m_SessionParameters&SESSION_CREATE_USES_ARBITRATION))
		{
			// the session we created was not setup for arbitration...
			LOGFATAL(g_Log, "C_Session::RegisterForArbitration() - Cant register for arbitration if the session was not created as arbitrated");
			return false;	
		}

		if (eStateRunning != m_SessionState)
		{
			// can only register for arbitration when the session is running not playing..
			LOGFATAL(g_Log, "C_Session::RegisterForArbitration() - Cant register for arbitration once session has started");
			return false;	
		}

		C_SessionCommandList* pList = reinterpret_cast<C_SessionCommandList*> (m_pCommandList);
		C_SessionCommandList* pPool = reinterpret_cast<C_SessionCommandList*> (m_pCommandPool);

		// iterate current commands. make sure there uis no start session comamnd active
		// yet since we must register for arbitration before starting the session..
		if (!pList->Empty())
		{
			C_SessionCommandList::Iterator i;
			for (i=pList->Begin(); i!=pList->End(); ++i)
			{
				if (C_SessionCommand::eCommandStartSession == i->m_Command)
				{
					LOGFATAL(g_Log, "C_Session::RegisterForArbitration() - Cant register for arbitration once session has started");
					return false;	
				}
			}

			// now iterate through list and make sure we dont register twice..
			for (i=pList->Begin(); i!=pList->End(); ++i)
			{
				if (C_SessionCommand::eCommandArbitrationRegistration == i->m_Command)
				{
					LOGFATAL(g_Log, "C_Session::RegisterForArbitration() - Cant register twice for arbitration");
					return false;	
				}
			}
		}

		if (pPool->Empty())
		{
			// no command objects left in pool..
			m_SessionState = eStateError;
			return false;
		}

		// allocate object and configure
		C_SessionCommandList::Iterator i =	pPool->Begin(); 
		pPool->Erase(pPool->Begin());
		i->m_Command													=	C_SessionCommand::eCommandArbitrationRegistration;
		i->m_CommandData.m_ArbitrationRegistration.m_ArbitrationNonce	=	arbitrationNonce.m_ArbitrationNonce;
		if (NULL != pCompleteNotification)
		{
			i->m_CommandData.m_ArbitrationRegistration.m_pNotification		=	pCompleteNotification;
			i->m_CommandData.m_ArbitrationRegistration.m_NotificationId		=	pCompleteNotification->m_Id;
		}
		else
		{
			i->m_CommandData.m_ArbitrationRegistration.m_pNotification		=	NULL;
			i->m_CommandData.m_ArbitrationRegistration.m_NotificationId		=	0;
		}

		// add to list for asynchronous processing.
		pList->PushBack(*i);

		if (NULL != pCompleteNotification)
			pCompleteNotification->Reset();

		return true;
	}

	bool C_Session::CheckPlayerIsInArbitrationList(const SysProfile::UserIdentifier& playeridentifier)
	{
		if (!m_Initialised)
			return false;

		if (NULL == m_pArbitrationRegistrationResults)
			return false;

		XSESSION_REGISTRATION_RESULTS* r = reinterpret_cast<XSESSION_REGISTRATION_RESULTS*> (m_pArbitrationRegistrationResults);
		if (0 == r->wNumRegistrants)
			return  false;

		// iterate through all enterants for specific player
		for (DWORD i=0; i<r->wNumRegistrants; i++)
		{
			XSESSION_REGISTRANT& registrant = r->rgRegistrants[i];
			for (DWORD q=0; q<registrant.bNumUsers; q++)
			{
				if (XOnlineAreUsersIdentical(playeridentifier.m_Uid, registrant.rgUsers[q]))
					return true;
			}
		}

		return false;
	}

	bool C_Session::ReleaseArbitrationRegistrationBuffer(void)
	{
		if (!m_Initialised)
			return false;

		if (NULL == m_pArbitrationRegistrationResults)
			return true; // No buffer exists..

		// check there is no arbitration registration command in the Queue, if there is then qwe cant release the buffer yet..
		C_SessionCommandList* pList = reinterpret_cast<C_SessionCommandList*> (m_pCommandList);
		if (!pList->Empty())
		{
			C_SessionCommandList::Iterator i;
			for (i=pList->Begin(); i!=pList->End(); ++i)
			{
				if (C_SessionCommand::eCommandArbitrationRegistration == i->m_Command)
				{
					LOGFATAL(g_Log, "C_Session::ReleaseArbitrationRegistrationBuffer() - Cant release buffer until asynchronous arbitration registration command has completed");
					return false;
				}
			}
		}

		delete [] m_pArbitrationRegistrationResults;
		m_pArbitrationRegistrationResults = NULL;
		
		return true;
	}

	bool C_Session::LockCurrentPlayers(const bool lock)
	{
		if (!m_Initialised)
			return false;

		if (eStateNotActive == m_SessionState || eStateError == m_SessionState || !m_SessionCreated)
			return false;

		if (!(m_SessionParameters&SESSION_CREATE_USES_ARBITRATION))
		{
			// the session we created was not setup for arbitration... so no need to lock unlock player
			return false;	
		}

		m_Locked = lock;

		return true;
	}

	bool C_Session::StartPlay(void)
	{
		if (!m_Initialised)
			return false;
				
		if (eStateError == m_SessionState || !m_SessionCreated)
			return false;  // session not in a  state where we can start play

		C_SessionCommandList* pList = reinterpret_cast<C_SessionCommandList*> (m_pCommandList);
		C_SessionCommandList* pPool = reinterpret_cast<C_SessionCommandList*> (m_pCommandPool);

		if (pPool->Empty())
		{
			m_SessionState = eStateError;
			return false; // no commands left in pool
		}

		// allocate object and configure
		C_SessionCommandList::Iterator i	= pPool->Begin();
		pPool->Erase(pPool->Begin());
		i->m_Command						= C_SessionCommand::eCommandStartSession;

		// add to list for asynchronous processing.
		pList->PushBack(*i);

		return true;
	}

	bool C_Session::EndPlay(void)
	{
		if (!m_Initialised)
			return false;

		if (eStateError == m_SessionState || !m_SessionCreated)
			return false;  // session not in a  state where we can end play

		C_SessionCommandList* pList = reinterpret_cast<C_SessionCommandList*> (m_pCommandList);
		C_SessionCommandList* pPool = reinterpret_cast<C_SessionCommandList*> (m_pCommandPool);

		if (pPool->Empty())
		{
			m_SessionState = eStateError;
			return false; // no commands left in pool
		}

		// allocate object and configure
		C_SessionCommandList::Iterator i	= pPool->Begin();
		pPool->Erase(pPool->Begin());
		i->m_Command						= C_SessionCommand::eCommandEndSession;

		// add to list for asynchronous processing.
		pList->PushBack(*i);

		return true;
	}

	bool C_Session::GetSecurityInfo(SessionSecurity& outKey)
	{
		if (eStateNotActive == m_SessionState || eStateCreating == m_SessionState || eStateError == m_SessionState)
			return false;

		outKey.m_ArbitrationNonce	= m_SessionNonce;
		outKey.m_Key				= m_SessionInfo.keyExchangeKey;
		outKey.m_Id					= m_SessionInfo.sessionID;	
		return true;
	}

	// An Idle session means there is no server requests active or pending.
	bool C_Session::IsIdle(void)
	{
		if (!m_Initialised)
			return true;

		if (eStateNotActive == m_SessionState || eStateError == m_SessionState)
			return true;

		C_SessionCommandList* pList = reinterpret_cast<C_SessionCommandList*> (m_pCommandList);
		
		if (pList->Size() == 0)
			return true;
		return false;
	}

	void C_Session::ShowPlayerReviewUI(const U8 userindex, const SysProfile::UserIdentifier& reviewTarget)
	{
		if (!m_Initialised)
			return;
		if (userindex >= XUSER_MAX_COUNT)
			return;

		XShowPlayerReviewUI(userindex, reviewTarget.m_Uid);
	}

	void C_Session::CleanUp(void)
	{
		Delete();

		// release arbitration registration results
		if (m_pArbitrationRegistrationResults)
			delete [] m_pArbitrationRegistrationResults;
		m_pArbitrationRegistrationResults = NULL;

		// remove any items from the command list
		if (m_pCommandList)
		{
			C_SessionCommandList* p = reinterpret_cast<C_SessionCommandList*> (m_pCommandList);
			p->Clear();
			delete p;
			m_pCommandList = NULL;
		}

		// remove any items from the command pool
		if (m_pCommandPool)
		{
			C_SessionCommandList* p = reinterpret_cast<C_SessionCommandList*> (m_pCommandPool);
			p->Clear();
			delete p;
			m_pCommandPool = NULL;
		}

		// release command storage..
		if (m_pCommandStorage)
		{
			delete [] m_pCommandStorage;
			m_pCommandStorage = NULL;
		}

		if (m_pSkillModifiers)
			delete [] m_pSkillModifiers;
		m_pSkillModifiers = NULL;
	}
}
