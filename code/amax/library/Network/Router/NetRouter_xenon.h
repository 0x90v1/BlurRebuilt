// ***************************************************************
//  NetRouter_xenon   version:  1.0   ·  date: 05/21/2008
//  -------------------------------------------------------------
//  
//  -------------------------------------------------------------
//  Copyright (C) Bizarre Creations 2008 - All Rights Reserved
// ***************************************************************
// author:	David Worswick
// purpose:	Handles sending / receiving network datagrams. Also
//			responsible for connectong to the platform specific servers.
// ***************************************************************

#ifndef NETWORK_ROUTER_XENON_H
#define NETWORK_ROUTER_XENON_H

#ifdef _MSC_VER
#pragma once
#endif

#include "NetRouterBase.h"
#include <ui/Util/WideString.h>

#include <SchedulerConfig.h>

namespace RacerNet
{
	/// PC specific code used for connecting to the platform holders servers and any game specific servers.
	class C_NetRouterXenon : public C_NetRouterBase
	{
	public:
		typedef enum
		{
			eXenonAuthStateNotActive = 0,
			eXenonAuthStateStartingNetwork,
			eXenonAuthStateEvaluatingBandwidth,
			eXenonAuthStateEnumeratingLsp,
			eXenonAuthStateConnectingToLobby,
			eXenonAuthStateError,
		}eXenonAuthState;

		C_NetRouterXenon();
		virtual ~C_NetRouterXenon();

		/// Initialise dynamic resources.
		/** @param maxSendMessageSize the maximum size of a network message that the game will send.
		 *	@return true if resources were alloacted. 
		 */
		virtual bool Initialise(const U32 maxSendMessageSize);

		/// Shutdown router and release all dynamic resources.
		/** This will also disconnect from all game servers.
		*/
		virtual void Shutdown(void);

		/// Attempt to connect to our service provider for online functionality.
		/** @param onlineMode if true then if using demonware we will go through NAT negotiation otherwise LAN games will only be possible.
		*	@param username name of player who will be authenticated (only relavant on PC).
		*	@param username password for the player who will be authenticated (only relavant on PC).
		*	@return true if the connection logic was started.
		*/
		virtual bool PlatformConnect(const bool onlineMode, char* userName, char* password);

		/// Attempt to disconnect from our platform service provider.
		/** Network multiplayer won't be available if disconnected.
		 */
		virtual void PlatformDisconnect(void);

		/// Call once per frame to pump any active tasks.
		virtual void Poll(const F32 frameTimeInSeconds);

		/// Enable or disable content streaming functionality.
		/** content streaming MUST be enabled prior to calling GetContentStreamingServer() to allocate resources.
		 *	once content streaming is no longer required we can disable it to release resources.
		 *	@param enabled true is we wish to enable content streaming or false if we wish to disable it.
		 *	@return true if succeeded.
		 */
		virtual bool EnableContentStreaming(const bool enabled);

		/// Attempt to verify a character string.
		/** Only used when creating groups so if no connection to the LSP exists this functionality will fail.
		 *	if the function returns true then continue to call PollVerifyString() until completion
		 *	@param pString string to be verified. This needs to stay in memory for the complete duration of the verification process.
		 *	@return true if the async string verification was begun.
		 */
		virtual bool VerifyString(const char* pString);

		/// Continues the asynchronous process of validating a string.
		/** @param validString once the function returns true then this value determins if the string has passed the validation
		 *	@return true once the validation process hhas completed, false if its still in progress..
		 */
		virtual bool PollVerifyString(bool& validString);

		/// Retrieve the type of NAT this player has.
		/** If thre router is not connected then this will return eNatOpen. 
		 */
		virtual eNatType GetNatType(void);

		const bool GetXLSPDisconnected() const;

		void InitContentStreamingDeferred();

		void LspConnectionFailedDeferred();

	private:
		bool NetworkStart(void);
		void ContinueNetworkStart(void);
		void StartLspEnumeration(void);
		void ContinueLspEnumeration(void);
		void ContinueLobbyConnection(void);	
		void LspConnectionFailed(void);
		void KillLspEnumeration(void);

		void KillStringVerification(void);

		void UpdateMatchingRegion(void);

	private:
		bool				m_Initialised;
		eXenonAuthState		m_XenonAuthState;
		U8*					m_pEnumeratedLsp;
		HANDLE				m_LspEnumerationHandle;
		XOVERLAPPED			m_LspEnumerationTask;
		bdAddr				m_ContentStreamingAddress;

		U8*					m_pStringVerifyResults;
		bool				m_StringVerifyActive;
		XOVERLAPPED			m_StringVerifyTask;
		UI::C_WideString128 m_StringToVerify;
		STRING_DATA			m_StringData;		
		U64					m_ConnectionTimer;
		bool				m_XLSPDisconnected;
	
		Scheduler::GroupHandle	m_hSchedulerGroup;
		bool					m_ProcessingTask;

		bool				mKillingLSPThreaded;
	};

void DeferredInitContentStreaming(Scheduler::Task* pTask);

}// end namespace RacerNet

#endif // NETWORK_ROUTER_XENON_H
