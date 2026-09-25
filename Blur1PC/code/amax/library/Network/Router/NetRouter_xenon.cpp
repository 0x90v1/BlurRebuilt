#include <stdafx.h>
#include <Core/core.h>
#include <Network/Core.h>
#include <SystemProfile/SystemProfileManager.h>
#include <SystemProfile/SystemProfile.h>
#include "../Misc/NetDebug.h"
#include "../Misc/NetDefines.h"
#include "NetRouter_xenon.h"
#include <bdNet/bdNet.h>
#include <bdCore/bdCore.h>
#include <bdLobby/bdLobbyService.h>
#include <bdLobby/bdContentStreaming/bdContentStreaming.h>
#include <bdCore/bdUtilities/bdRandom.h>
#include <GameLogic/GameData/MultiplayerData.h>

#if !defined ( CFG_GOLD )
Debug::Bool g_DisconnectDemonwareLobby("Network.Demonware.Lobby.ForceDisconnect", false);
#endif

// enable this to run a bandwidth scan during the connection process, it takes ages though.
//#define RUN_BANDWIDTH_SCAN_ON_CONNECTION

#if defined ( RACER_NET_OPTIMIZE_OFF )
	#pragma optimize( "", off )
#endif

#define MAX_LSP_SERVERS_TO_ENUMERATE	(20)

//#define SHOW_NETWORK_DEBUG_STRING_OUTPUT

#define USE_DEMONWARE_CONTENT_SERVER

#if defined ( USE_DEMONWARE_CONTENT_SERVER )
	#define DW_TITLE_ID						(0x14D2)
	#define LSP_SERVICE_ID					(0x415607E9)
	#define LSP_PORT						(3017u)
	#define LSP_SERVER_DESCRIPTION			"AMAX - DW ACTI LSP"
	//#define DW_LSG_ADDRESS					"amax.contentserver.mmp3.demonware.net"
	//#define DW_STREAMING_SERVICE_ADDRESS	"amax.binario.mmp3.demonware.net" // not used as we assume the content streaming address is the same as the lsp server.
	#define DW_DEFAULT_STREAMING_LSP_PORT	(1016)
#else
	#define DW_TITLE_ID						(0x1402)
	#define LSP_SERVICE_ID					(0x415607E9)
	#define LSP_PORT 						(1014)
	#define LSP_SERVER_DESCRIPTION			"AMAX - DW ACTI LSP"
#endif


// max time to wait on demonware connection process before failing (in seconds)
#define MAX_DEMONWARE_CONNECTION_TIME_SEC (20)	


namespace RacerNet
{
	// constructor
	C_NetRouterXenon::C_NetRouterXenon() 
		:	m_Initialised(false)
		,	m_XenonAuthState(eXenonAuthStateNotActive)
		,	m_pEnumeratedLsp(NULL)
		,	m_LspEnumerationHandle(INVALID_HANDLE_VALUE)
		,	m_pStringVerifyResults(NULL)
		,	m_StringVerifyActive(false)
		,	m_ConnectionTimer(0)
		,	m_XLSPDisconnected(false)
		,	mKillingLSPThreaded(false)

	{
		ZeroMemory(&m_LspEnumerationTask, sizeof(XOVERLAPPED));
		ZeroMemory(&m_StringVerifyTask, sizeof(XOVERLAPPED));

		m_hSchedulerGroup	= Scheduler::JQ_GROUP_NONE;
		m_ProcessingTask	= false;
	}

	// destructor
	C_NetRouterXenon::~C_NetRouterXenon()
	{		
		Shutdown();
	}

	bool C_NetRouterXenon::Initialise(const U32 sendBufferSize)
	{
		if (m_Initialised)
			return true;

		if (C_NetRouterBase::Initialise(sendBufferSize))
			m_Initialised = true;

		m_hSchedulerGroup = Scheduler::CreateGroup();

		return m_Initialised;
	}

	void C_NetRouterXenon::Shutdown(void)
	{
		PlatformDisconnect();
		C_NetRouterBase::Shutdown();	
		m_Initialised = false;
		Scheduler::DestroyGroup(m_hSchedulerGroup);
	}

	bool C_NetRouterXenon::PlatformConnect(const bool onlineMode, char* userName, char* password)
	{
		if (!m_Initialised)
			return false;

		// no point disconnecting and re connecting unless online mode changes.
		if (GetPlatformConnectionState() != eConnectionStateNotActive && GetPlatformConnectionState() != eConnectionStateFailed)
		{
			if (onlineMode == IsOnlineMode() &&
				(C_MultiplayerData::Instance().m_pConfig && (C_MultiplayerData::Instance().m_pConfig->m_PeerProxyEnabled == m_UseRelaySocketRouter)))
				return true; // may as well carry on the same connection attempt.
		}
			
		// disconnect if we are currently connected .
		PlatformDisconnect();
	
		// store the connection mode
		m_OnlineMode = onlineMode;

		// startup demonware, it won't try and access the servers since it doesn't need to.
		m_PlatformConnectionState = eConnectionStateInProgress;
		m_LastConnectionErrorCode = eConnectionResultNone;

		LOGINFO(g_Log, "C_NetRouterXenon - Connection started - %s", m_OnlineMode ? "Online mode" : "Offline mode");

		bool res=NetworkStart();
		m_XLSPDisconnected=!res;
		return res;
	}

	void C_NetRouterXenon::PlatformDisconnect(void)
	{
		if (!m_Initialised)
			return;

		KillStringVerification();
		StopBandwidthEvaluation();
		EnableContentStreaming(false);
		
		if (GetPlatformConnectionState() == eConnectionStateNotActive)
			return;

#ifdef SHOW_NETWORK_DEBUG_STRING_OUTPUT
		OutputDebugString(".. Platform Disconnect .. Killing LSP Enumeration\n");
#endif
		KillLspEnumeration();

		// disconnect from the demonware lobby service.
		bdNetImpl *const net = bdNet::getInstance();
		bdLobbyService* lobbyService = NetPlay::C_Core::Instance().DemonwareLobbyService();
		if (lobbyService)
			lobbyService->disconnect();

		// stop demonware netwoking.
		net->stop();
		
		LOGINFO(g_Log, "C_NetRouterXenon - Connection terminated");

		m_PlatformConnectionState = eConnectionStateNotActive;
			
		m_LastConnectionErrorCode = eConnectionResultGeneric; // all connection errors are flagged as generic on xenon.

		m_XenonAuthState = eXenonAuthStateNotActive;

		m_OnlineMode = false;

		m_ConnectionTimer = 0;

		m_XLSPDisconnected=true;
	}

	void C_NetRouterXenon::LspConnectionFailedDeferred()
	{
#ifdef SHOW_NETWORK_DEBUG_STRING_OUTPUT
		OutputDebugString(".. LSP Connection Failed .. Killing LSP Enumeration Deferred\n");
		OutputDebugString(".. Killing LSP Enumeration Deferred\n");
#endif
		// stop lsp enumeration async task
		if (!XHasOverlappedIoCompleted(&m_LspEnumerationTask))
			XCancelOverlapped(&m_LspEnumerationTask);
		ZeroMemory(&m_LspEnumerationTask, sizeof(XOVERLAPPED));

#ifdef SHOW_NETWORK_DEBUG_STRING_OUTPUT
		OutputDebugString(".. Killing LSP Enumeration .. Killed Overlapped Task Deferred\n");
#endif

		// close lsp enumeration handle
		if (m_LspEnumerationHandle != INVALID_HANDLE_VALUE)
			CloseHandle(m_LspEnumerationHandle);
		m_LspEnumerationHandle = INVALID_HANDLE_VALUE;

#ifdef SHOW_NETWORK_DEBUG_STRING_OUTPUT
		OutputDebugString(".. Killing LSP Enumeration .. Killed Handle Deferred\n");
#endif

		// release lsp enumeration buffer.
		if (m_pEnumeratedLsp)
			delete [] m_pEnumeratedLsp;
		m_pEnumeratedLsp = NULL;

#ifdef SHOW_NETWORK_DEBUG_STRING_OUTPUT
		OutputDebugString(".. Killing LSP Enumeration .. Killed Memory Deferred\n");
#endif

		LOGWARN(g_Log, "No connection to xLSP server.");

		// still connected to the platform but not connected to the lsp.
		m_PlatformConnectionState = eConnectionStateComplete;
		m_XenonAuthState = eXenonAuthStateNotActive;

		mKillingLSPThreaded=false;
	}

	void DeferredLspConnectionFailed(Scheduler::Task* pTask)
	{
		C_NetRouterXenon* pSystem = *(C_NetRouterXenon**)pTask->ParamData;
		pSystem->LspConnectionFailedDeferred();
	}

	JQ_DECLARE_PPU_MODULE(LspConnectionFailedThreaded, DeferredLspConnectionFailed);

	void C_NetRouterXenon::Poll(const F32 frameTimeInSeconds)
	{
		PERF_EVENT("NRX.Poll");

		if (!m_Initialised)
			return;

		C_NetRouterBase::Poll(frameTimeInSeconds);

		bool pollLobbyConnection = false;
		bool checkForGoingOffline = false;

		if (eConnectionStateInProgress == m_PlatformConnectionState)
		{
			checkForGoingOffline = m_OnlineMode; // only do this if we are attempting an online connection

			if (m_OnlineMode)
			{
				const U64 passedTime = (Time::GetPerformanceCounter() - m_ConnectionTimer) / 1000; // convert to miliseconds
				if (passedTime >= (MAX_DEMONWARE_CONNECTION_TIME_SEC*1000))
				{
					if(!mKillingLSPThreaded)
					{
						LOGWARN(g_Log, "C_NetRouterXenon::Poll() - timed out connection to demonware service");
	
#ifdef SHOW_NETWORK_DEBUG_STRING_OUTPUT
						OutputDebugString(".. Poll .. LSP Connection Failed\n");
#endif

						mKillingLSPThreaded=true;
						BIZ_ASSERT(m_hSchedulerGroup != Scheduler::JQ_GROUP_NONE); // make sure you set the scheduler group before running the update
						C_NetRouterXenon* pSystem = this;
						Scheduler::AddTask(LspConnectionFailedThreaded, 0, 0, m_hSchedulerGroup, 0, &pSystem, sizeof(pSystem));
					}

					return;
				}
			}

			switch (m_XenonAuthState)
			{
				case eXenonAuthStateStartingNetwork:
					ContinueNetworkStart();
					break;

				case eXenonAuthStateEvaluatingBandwidth:
					if (!BandwidthEvaluationInProgress())
						StartLspEnumeration();
					break;

				case eXenonAuthStateEnumeratingLsp:
					ContinueLspEnumeration();
					break;

				case eXenonAuthStateConnectingToLobby:
					ContinueLobbyConnection();
					break;

				default:
					LOGWARN(g_Log, "C_NetRouterXenon::Poll() - unknown authentication state.");
					break;
			}
		}
		else if (eConnectionStateComplete == m_PlatformConnectionState)
		{
			pollLobbyConnection = m_OnlineMode;	// lobby conenction won't be active if not in online mode.
			checkForGoingOffline = m_OnlineMode; // only do this if we are attempting an online connection
		}

		bool lost = false;
		bdLobbyService* lobbyService = NetPlay::C_Core::Instance().DemonwareLobbyService();
		if (pollLobbyConnection)
		{	
			if (lobbyService)
			{
				lobbyService->pump();

#if defined ( USE_DEMONWARE_CONTENT_SERVER )
				if (m_ContentStreamingEnabled && !m_ProcessingTask)
				{
					bdContentStreaming* contentStreaming = lobbyService->getContentStreaming();
					if (contentStreaming)
						contentStreaming->pump();
				}
			}
#endif
			const bdLobbyConnection::Status connectionStatus = lobbyService->getStatus();
			switch(connectionStatus)
			{			
			case bdLobbyConnection::BD_DISCONNECTED:
				LOGINFO(g_Log, "C_NetRouterXenon::Poll() - Lost connection to xLSP server.");
				lost = true;
				break;
			}
		}

		if (checkForGoingOffline)
		{
			// is any player still connected to xbox live ? if not we need to change to offline mode.
			// TCR states that in systemlink mode loss of connection to xbl should not interrupt the game.
			if (SysProfile::C_Manager::Instance().ServiceConnectionState() != SysProfile::C_Manager::eServiceStateConnected)
				lost = true;

#if !defined ( CFG_GOLD )
			if (g_DisconnectDemonwareLobby)
			{
				g_DisconnectDemonwareLobby = false;
				lost = true;
			}
#endif
		}

		if (lost)
		{
			LOGINFO(g_Log, "C_NetRouterXenon::Poll() - changing offline mode. ");
			KillStringVerification();
			StopBandwidthEvaluation();
			EnableContentStreaming(false);

			m_XLSPDisconnected=true;

			m_OnlineMode = false;

#ifdef SHOW_NETWORK_DEBUG_STRING_OUTPUT
			OutputDebugString(".. Demonware Disconnect .. Killing LSP Enumeration\n");
#endif
			KillLspEnumeration();

			// disconnect from the demonware lobby service.
			if (lobbyService)
				lobbyService->disconnect();

			m_PlatformConnectionState = eConnectionStateComplete;
			m_XenonAuthState = eXenonAuthStateNotActive;
		}
	}

	bool C_NetRouterXenon::NetworkStart(void)
	{
		bdNetImpl *const net = bdNet::getInstance();
		bdNetStartParams params;
		params.m_onlineGame = m_OnlineMode;

		m_UseRelaySocketRouter = false;

		// if online mode then we want to use peer proxy to bypass NAT issues.
		if ( params.m_onlineGame )
		{
			if (C_MultiplayerData::Instance().m_pConfig && C_MultiplayerData::Instance().m_pConfig->m_PeerProxyEnabled)
			{
				params.m_useRelaySocketRouter = true;
				m_UseRelaySocketRouter = true;

				LOGINFO(g_Log, "C_NetRouterXenon : Peer routing enabled.");
			}
		}

		if (!NetPlay::C_Core::Instance().CableConnected())
		{
			m_LastConnectionErrorCode = eConnectionResultNoNetwork;
			m_PlatformConnectionState = eConnectionStateFailed;
			PlatformDisconnect();
			return false;
		}

		m_PlatformConnectionState	= eConnectionStateInProgress;
		m_ConnectionTimer			= Time::GetPerformanceCounter();

		if (net->start(params))
		{
			m_XenonAuthState = eXenonAuthStateStartingNetwork;
			return true;
		}
		else
		{
			LOGWARN(g_Log, "C_NetRouterXenon::NetworkStart() - unable to start network.");
			m_PlatformConnectionState = eConnectionStateFailed;
			PlatformDisconnect();
		}

		return false;
	}

	void C_NetRouterXenon::ContinueNetworkStart(void)
	{
		bdNetImpl *const net = bdNet::getInstance();

		net->pump();
		if (net->getStatus() == bdNetImpl::BD_NET_PENDING)
			return;

		if (net->getStatus() == bdNetImpl::BD_NET_DONE)
		{
			if (!m_OnlineMode)
			{
				m_XenonAuthState = eXenonAuthStateNotActive;
				m_PlatformConnectionState = eConnectionStateComplete;
			}
			else
			{
				UpdateMatchingRegion(); 

#if defined ( RUN_BANDWIDTH_SCAN_ON_CONNECTION )
				StartBandwidthEvaluation();
#endif
				m_XenonAuthState = eXenonAuthStateEvaluatingBandwidth;
			}
		}
		else
		{
			LOGWARN(g_Log, "C_NetRouterXenon::ContinueNetworkStart() - unable to start network.");
			m_PlatformConnectionState = eConnectionStateFailed;
			PlatformDisconnect();
		}
	}

	void C_NetRouterXenon::StartLspEnumeration(void)
	{
#ifdef SHOW_NETWORK_DEBUG_STRING_OUTPUT
		OutputDebugString(".. Starting LSP Enumeration .. Killing LSP Enumeration\n");
#endif
		KillLspEnumeration();

		// look for all our servers.
		char* serverDesc = LSP_SERVER_DESCRIPTION;

		// Create a title server enumerator
		DWORD requiredSize = 0;
		DWORD res = XTitleServerCreateEnumerator(serverDesc, MAX_LSP_SERVERS_TO_ENUMERATE, &requiredSize, &m_LspEnumerationHandle);
		if(ERROR_SUCCESS == res)
		{
			//allocate results buffer.
			m_pEnumeratedLsp = new U8[requiredSize];
			if (m_pEnumeratedLsp)
			{
				res = XEnumerate(m_LspEnumerationHandle, m_pEnumeratedLsp, requiredSize, NULL, &m_LspEnumerationTask);
				if (ERROR_IO_PENDING == res)
					m_XenonAuthState = eXenonAuthStateEnumeratingLsp;
				else
				{
#ifdef SHOW_NETWORK_DEBUG_STRING_OUTPUT
					OutputDebugString(".. XEnumerate Failed .. LSP Connection Failed\n");
#endif
					LspConnectionFailed();
				}
			}
			else
			{
#ifdef SHOW_NETWORK_DEBUG_STRING_OUTPUT
				OutputDebugString(".. LSP New Failed .. LSP Connection Failed\n");
#endif
				LspConnectionFailed();
			}
		}
		else
		{
#ifdef SHOW_NETWORK_DEBUG_STRING_OUTPUT
			OutputDebugString(".. XTitleServerCreateEnumerator Failed .. LSP Connection Failed\n");
#endif
			LspConnectionFailed();
		}
	}

	void C_NetRouterXenon::ContinueLspEnumeration(void)
	{
		if (!XHasOverlappedIoCompleted(&m_LspEnumerationTask))
			return;
		
		DWORD serverCount = 0;
		DWORD res = XGetOverlappedResult(&m_LspEnumerationTask, &serverCount, false);
		if (ERROR_SUCCESS == res)
		{
			if (0 == serverCount)
			{
				LOGWARN(g_Log, "No xLSP servers enumerated.");
			}
			else
			{
				// choose a random lsp server to connect to..
				bdRandom r;
				const U32 serverIndex = ((r.nextUInt())%serverCount);

				// create server address..
				const XTITLE_SERVER_INFO* pServers = reinterpret_cast<XTITLE_SERVER_INFO*> (m_pEnumeratedLsp);

				//print out servers
				for ( DWORD dwLoop(0); dwLoop < serverCount; ++dwLoop )
				{
					LOGINFO( g_Log, "[XLSP Server Enumerator] Server: \"%s\" (%d.%d.%d.%d).\n", pServers[ dwLoop ].szServerInfo, pServers[ dwLoop ].inaServer.S_un.S_un_b.s_b1, pServers[ dwLoop ].inaServer.S_un.S_un_b.s_b2, pServers[ dwLoop ].inaServer.S_un.S_un_b.s_b3, pServers[ dwLoop ].inaServer.S_un.S_un_b.s_b4 );
				}

				IN_ADDR tsAddr;
				tsAddr.S_un.S_addr = 0;
				if (XNetServerToInAddr(pServers[serverIndex].inaServer, LSP_SERVICE_ID, &tsAddr) == 0)
				{
					bdAddr addr = bdAddr(bdInetAddr(tsAddr.S_un.S_addr), LSP_PORT);

#if defined ( USE_DEMONWARE_CONTENT_SERVER )
					// content streaming address is same as lsp server but with a different port.
					m_ContentStreamingAddress = bdAddr(bdInetAddr(tsAddr.S_un.S_addr), DW_DEFAULT_STREAMING_LSP_PORT);
#endif

					// release lsp enumeration buffers, we have everything we want now.
#ifdef SHOW_NETWORK_DEBUG_STRING_OUTPUT
					OutputDebugString(".. Found LSP .. Killing LSP Enumeration\n");
#endif
					KillLspEnumeration();

					// prepeare authentication ticket.
					bool authTicketPrepared = false;
					bdAuthTicket authTicket;

					bdAuthInfo authInfo;
					authInfo.m_titleID = DW_TITLE_ID;

					const S32 primaryUserIndex = SysProfile::C_Manager::Instance().GetPrimaryPadNo();
					if (primaryUserIndex >= 0 && primaryUserIndex < XUSER_MAX_COUNT)
					{
						XUSER_SIGNIN_INFO sInfo;
						if (XUserGetSigninInfo(primaryUserIndex, XUSER_GET_SIGNIN_INFO_ONLINE_XUID_ONLY, &sInfo) == ERROR_SUCCESS)
						{
							if (sInfo.xuid != INVALID_XUID && sInfo.dwGuestNumber == 0)
							{
								authTicketPrepared = true;
								authTicket.m_userID = sInfo.xuid;
								authTicket.m_titleID = DW_TITLE_ID;

								BIZ_ASSERT(XUSER_NAME_SIZE <= BD_MAX_USERNAME_LENGTH);

								for (U8 z=0; z<XUSER_NAME_SIZE; z++)
									authTicket.m_username[z] = sInfo.szUserName[z];
							}
						}
					}

					if (authTicketPrepared)
					{
						authTicket.serialize(authInfo.m_data);

						bdLobbyService* lobbyService = NetPlay::C_Core::Instance().DemonwareLobbyService();
						if (lobbyService)
						{
							BIZ_ASSERT(lobbyService->getStatus() == bdLobbyConnection::BD_NOT_CONNECTED);

							if (lobbyService->connect(addr, &authInfo))
							{
								m_XenonAuthState = eXenonAuthStateConnectingToLobby;
								return;
							}
						}
					}
				}
			}
		}
		
#ifdef SHOW_NETWORK_DEBUG_STRING_OUTPUT
		OutputDebugString(".. ContinueLspEnumeration Failed .. LSP Connection Failed\n");
#endif
		LspConnectionFailed();
	}

	void C_NetRouterXenon::ContinueLobbyConnection(void)
	{
		bdLobbyService* lobbyService = NetPlay::C_Core::Instance().DemonwareLobbyService();
		if (NULL != lobbyService)
		{
			lobbyService->pump();
			const bdLobbyConnection::Status connectionStatus = lobbyService->getStatus();

			switch(connectionStatus)
			{
			case bdLobbyConnection::BD_CONNECTING:
					return;
				break;

			case bdLobbyConnection::BD_CONNECTED:
				{
					// connected to lobby via Lsp now..
					LOGINFO(g_Log, "Connected to xLSP server.");
					m_PlatformConnectionState = eConnectionStateComplete;
					m_XenonAuthState = eXenonAuthStateNotActive;

#if defined ( USE_DEMONWARE_CONTENT_SERVER )
					// now that we are connected we should set the address of the content streaming server.
					// not used in new streaming lib !!! lobbyService->setStreamingServiceAddress(m_ContentStreamingAddress);
#endif
					return;
				}
				break;
			}
		}

#ifdef SHOW_NETWORK_DEBUG_STRING_OUTPUT
		OutputDebugString(".. ContinueLobbyConnection Failed .. LSP Connection Failed\n");
#endif
		LspConnectionFailed();
	}

	void C_NetRouterXenon::LspConnectionFailed(void)
	{
		if(mKillingLSPThreaded)
			return;

#ifdef SHOW_NETWORK_DEBUG_STRING_OUTPUT
		OutputDebugString(".. LSP Connection Failed .. Killing LSP Enumeration\n");
#endif
		KillLspEnumeration();

		LOGWARN(g_Log, "No connection to xLSP server.");

		// still connected to the platform but not connected to the lsp.
		m_PlatformConnectionState = eConnectionStateComplete;
		m_XenonAuthState = eXenonAuthStateNotActive;
	}

	void C_NetRouterXenon::KillLspEnumeration(void)
	{
		if(mKillingLSPThreaded)
			return;

#ifdef SHOW_NETWORK_DEBUG_STRING_OUTPUT
		OutputDebugString(".. Killing LSP Enumeration\n");
#endif

		// stop lsp enumeration async task
		if (!XHasOverlappedIoCompleted(&m_LspEnumerationTask))
			XCancelOverlapped(&m_LspEnumerationTask);
		ZeroMemory(&m_LspEnumerationTask, sizeof(XOVERLAPPED));

#ifdef SHOW_NETWORK_DEBUG_STRING_OUTPUT
		OutputDebugString(".. Killing LSP Enumeration .. Killed Overlapped Task\n");
#endif

		// close lsp enumeration handle
		if (m_LspEnumerationHandle != INVALID_HANDLE_VALUE)
			CloseHandle(m_LspEnumerationHandle);
		m_LspEnumerationHandle = INVALID_HANDLE_VALUE;

#ifdef SHOW_NETWORK_DEBUG_STRING_OUTPUT
		OutputDebugString(".. Killing LSP Enumeration .. Killed Handle\n");
#endif

		// release lsp enumeration buffer.
		if (m_pEnumeratedLsp)
			delete [] m_pEnumeratedLsp;
		m_pEnumeratedLsp = NULL;

#ifdef SHOW_NETWORK_DEBUG_STRING_OUTPUT
		OutputDebugString(".. Killing LSP Enumeration .. Killed Memory\n");
#endif
	}

	void DeferredInitContentStreaming(Scheduler::Task* pTask)
	{
		PERF_EVENT("InitContentStreamingTask");
		C_NetRouterXenon* pSystem = *(C_NetRouterXenon**)pTask->ParamData;
		pSystem->InitContentStreamingDeferred();
	}

	JQ_DECLARE_PPU_MODULE(InitContentStreamingTask, DeferredInitContentStreaming);

	void C_NetRouterXenon::InitContentStreamingDeferred()
	{
		bdLobbyService* lobbyService = NetPlay::C_Core::Instance().DemonwareLobbyService();
		if (!lobbyService || !lobbyService->getContentStreaming() || lobbyService->getStatus() != bdLobbyConnection::BD_CONNECTED)
		{
			m_ProcessingTask = false;
			return;
		}

		// this can block for ~400ms internally calling XEnumerate
		lobbyService->getContentStreaming()->init(LSP_SERVICE_ID);

		m_ContentStreamingEnabled = true;

		m_ProcessingTask = false;
	}

	bool C_NetRouterXenon::EnableContentStreaming(const bool enabled)
	{
		if (m_Initialised)
		{
			bool ok = true;
			// need to pass service id to content streamer.
			if (enabled)
			{
				if (!m_ContentStreamingEnabled)
				{
					bdLobbyService* lobbyService = NetPlay::C_Core::Instance().DemonwareLobbyService();
					if (lobbyService && lobbyService->getStatus() == bdLobbyConnection::BD_CONNECTED)
					{
						bdContentStreaming* cs = lobbyService->getContentStreaming();
						if (cs)
						{
							if (!m_ProcessingTask)
							{
								m_ProcessingTask = true;

								BIZ_ASSERT(m_hSchedulerGroup != Scheduler::JQ_GROUP_NONE); // make sure you set the scheduler group before running the update
								C_NetRouterXenon* pSystem = this;
								Scheduler::AddTask(InitContentStreamingTask, 0, 0, m_hSchedulerGroup, 0, &pSystem, sizeof(pSystem));
							}
						}
						else
							ok = false;
					}
					else
						ok = false;
				}
			}
			else
			{
				// [Rog] Bug 38083 - Make sure no tasks are still running before we try and shut down else bad things happen.
				if (m_ProcessingTask)
				{
					Scheduler::Flush(m_hSchedulerGroup);
				}

				if (m_ContentStreamingEnabled)
				{
					bdLobbyService* lobbyService = NetPlay::C_Core::Instance().DemonwareLobbyService();
					if (lobbyService)
					{
						bdContentStreaming* cs = lobbyService->getContentStreaming();
						if (cs)
							cs->abortHTTPOperation();

						lobbyService->deleteContentStreaming();
					}
				}
				m_ContentStreamingEnabled = false;
			}

			return ok;
		}
		return false;
	}

	bool C_NetRouterXenon::VerifyString(const char* pString)
	{
		if (!m_Initialised)
			return false;

		KillStringVerification();

		if (NULL == pString)
			return false;

		const U32 len = strlen(pString) + 1; // account for termination.
		if (1 == len || len > m_StringToVerify.Size())
			return false; // no string supplied or greater than max size.

		// since string verification is used for lsp features we should be connected !!
		if (m_PlatformConnectionState != eConnectionStateComplete || !m_OnlineMode)
			return false;
		
		bdLobbyService* lobbyService = NetPlay::C_Core::Instance().DemonwareLobbyService();
		if (NULL == lobbyService)
			return false;

		const bdLobbyConnection::Status connectionStatus = lobbyService->getStatus();
		if (bdLobbyConnection::BD_CONNECTED != connectionStatus)
			return false;

		m_StringToVerify.Format(L"%S", pString);

		const U32 numStrings = 1;

		// allocate results buffer.
		const DWORD resultsSize = sizeof(STRING_VERIFY_RESPONSE) + (sizeof(HRESULT) * numStrings);

		m_pStringVerifyResults = new U8[resultsSize];
		if (NULL == m_pStringVerifyResults)
			return false;

		ZeroMemory(m_pStringVerifyResults, resultsSize);

		m_StringData.pszString		= m_StringToVerify.NonConstChars();
		m_StringData.wStringSize	= m_StringToVerify.Length() + 1; // add terminating character.
		
		const DWORD res = XStringVerify(0,
										"en-us",
										numStrings,
										&m_StringData,
										resultsSize,
										(STRING_VERIFY_RESPONSE*)m_pStringVerifyResults,
										&m_StringVerifyTask);

		if (ERROR_IO_PENDING == res)
			m_StringVerifyActive = true;
		else
			KillStringVerification();

		return m_StringVerifyActive;
	}

	bool C_NetRouterXenon::PollVerifyString(bool& validString)
	{
		validString = false;
		if (m_StringVerifyActive)
		{
			if (!XHasOverlappedIoCompleted(&m_StringVerifyTask))
				return false;

			const DWORD res = XGetOverlappedResult(&m_StringVerifyTask, NULL, false);
			if (ERROR_SUCCESS == res)
			{
				STRING_VERIFY_RESPONSE* p = reinterpret_cast<STRING_VERIFY_RESPONSE*> (m_pStringVerifyResults);
				if (p->wNumStrings > 0)
				{
					if (S_OK == p->pStringResult[0])
						validString = true;
				}
			}

			KillStringVerification();
		}
		return true;
	}

	void C_NetRouterXenon::KillStringVerification(void)
	{
		if (m_StringVerifyActive)
		{
			if (!XHasOverlappedIoCompleted(&m_StringVerifyTask))
				XCancelOverlapped(&m_StringVerifyTask);	
		}

		ZeroMemory(&m_StringVerifyTask, sizeof(XOVERLAPPED));

		if (m_pStringVerifyResults)
			delete [] m_pStringVerifyResults;
		m_pStringVerifyResults = NULL;

		m_StringVerifyActive = false;
	}

	eNatType C_NetRouterXenon::GetNatType(void)
	{
		if (!m_Initialised)
			return eNatOpen;

		eNatType t = eNatOpen;
		switch (XOnlineGetNatType())
		{
		case XONLINE_NAT_OPEN:
			t = eNatOpen;
			break;

		case XONLINE_NAT_MODERATE:
			t = eNatModerate;
			break;

		case XONLINE_NAT_STRICT:
			t = eNatStrict;
			break;
		}

		return t;
	}

	void C_NetRouterXenon::UpdateMatchingRegion(void)
	{
		eMatchingRegion region = eMatchingRegionUnknown;

		switch (XGetLocale())
		{
		case XC_LOCALE_AUSTRALIA:
		case XC_LOCALE_NEW_ZEALAND:
			region = eMatchingRegionAunz;
			break;
		
		case XC_LOCALE_CANADA:
		case XC_LOCALE_UNITED_STATES:
			region = eMatchingRegionNorthAmerica;
			break;

		case XC_LOCALE_MEXICO:
		case XC_LOCALE_CHILE:
		case XC_LOCALE_COLOMBIA:
		case XC_LOCALE_BRAZIL:
			region = eMatchingRegionSouthAmerica;
			break;

		case XC_LOCALE_CHINA:
		case XC_LOCALE_HONG_KONG:
		case XC_LOCALE_INDIA:
		case XC_LOCALE_KOREA:
		case XC_LOCALE_SINGAPORE:
		case XC_LOCALE_TAIWAN:
		case XC_LOCALE_JAPAN:
		case XC_LOCALE_SOUTH_AFRICA:
			region = eMatchingRegionAsia;
			break;

		case XC_LOCALE_AUSTRIA:
		case XC_LOCALE_BELGIUM:
		case XC_LOCALE_CZECH_REPUBLIC:
		case XC_LOCALE_DENMARK:
		case XC_LOCALE_FINLAND:
		case XC_LOCALE_FRANCE:
		case XC_LOCALE_GERMANY:
		case XC_LOCALE_GREECE:
		case XC_LOCALE_HUNGARY:
		case XC_LOCALE_IRELAND:
		case XC_LOCALE_ITALY:
		case XC_LOCALE_NETHERLANDS:
		case XC_LOCALE_NORWAY:
		case XC_LOCALE_POLAND:
		case XC_LOCALE_PORTUGAL:
		case XC_LOCALE_SLOVAK_REPUBLIC:
		case XC_LOCALE_SPAIN:
		case XC_LOCALE_SWEDEN:
		case XC_LOCALE_SWITZERLAND:
		case XC_LOCALE_GREAT_BRITAIN:
			region = eMatchingRegionEurope;
			break;

		default:
			switch(XGetGameRegion())
			{
			case XC_GAME_REGION_NA_ALL:
			case XC_GAME_REGION_RESTOFWORLD_ALL:
				region = eMatchingRegionNorthAmerica;
				break;

			case XC_GAME_REGION_ASIA_ALL:
			case XC_GAME_REGION_ASIA_JAPAN:
			case XC_GAME_REGION_ASIA_CHINA:
			case XC_GAME_REGION_ASIA_REST:
				region = eMatchingRegionAsia;
				break;

			case XC_GAME_REGION_EUROPE_ALL:
			case XC_GAME_REGION_EUROPE_REST:
				region = eMatchingRegionEurope;
				break;

			case XC_GAME_REGION_EUROPE_AUNZ:
				region = eMatchingRegionAunz;
				break;

			default:
				region = eMatchingRegionNorthAmerica;
				break;
			}
			break;
		}

		SetMatchingRegion(region);
	}

	const bool C_NetRouterXenon::GetXLSPDisconnected() const
	{
		return m_XLSPDisconnected;
	}

}// end namespace RacerNet

#if defined ( RACER_NET_OPTIMIZE_OFF )
	#pragma optimize( "", on )
#endif
