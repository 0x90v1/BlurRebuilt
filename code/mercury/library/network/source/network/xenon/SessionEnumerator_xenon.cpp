#include <StdAfx.h>
#include <Network/SessionEnumerator.h>
#include <Network/SessionParameter.h>
#include <Network/Securekey.h>
#include <SystemProfile/SystemProfile.h>
#include <SystemProfile/GameInvite.h>
#include <Core/Utility/Utility.h>
#include <bdNet/bdNet.h>
#include <bdCore/bdSocket/bdCommonAddr.h>
#include "AsyncTask_xenon.h"


namespace NetPlay
{
	// constructior
	C_SessionEnumerator::C_SessionEnumerator() :	m_Initialised(false), m_pAsyncTask(NULL), m_pSearchProperties(NULL), m_pSearchContexts(NULL),
													m_NumSearchProperties(0), m_NumSearchContexts(0), m_MaxSessionToEnumerate(0), m_pEnumeratedSessionData(NULL), m_NumSessionEnumerated(0), m_SearchState(eSearchNotActive),
													m_pQoSSessionData(NULL), m_QoSHasDataStream(false),m_pQoSAddresses(NULL), m_pQoSKeys(NULL), m_pQoSKeyIds(NULL), m_pQoSHandle(NULL)
	{
	}


	// destructor
	C_SessionEnumerator::~C_SessionEnumerator()
	{
		CleanUp();
	}

	/// registers a listener that listens to enumeration events.
	/**	@param listener object listening to enumeration events.
	 *	@return true if listener has been registered, false if already registered.
	 */
	bool C_SessionEnumerator::RegisterListener(C_SessionEnumeratorListener* listener)
	{
		for (	Btl::List<C_SessionEnumeratorListener*>::Iterator it = m_listeners.Begin();
				it != m_listeners.End(); 
				++it)
		{
			if((*it) == listener) 
				return false;
		}
		m_listeners.PushBack(listener);
		return true;
	}

	/// unregisters a listener that was previously registered.
	/**	@param listener we want to unregister.
	 *	@return true if listener has been unregistered succeffully, false if not found.
	 */
	bool C_SessionEnumerator::UnregisterListener(C_SessionEnumeratorListener* listener)
	{
		for (	Btl::List<C_SessionEnumeratorListener*>::Iterator it = m_listeners.Begin();
				it != m_listeners.End(); 
				++it)
		{
			if((*it) == listener) 
			{
				m_listeners.Erase(it);
				return true;
			}
		}
		return false;
	}

	// setup dynamic resources.
	bool C_SessionEnumerator::Initialise(const U32 maxSearchProperties, const U32 maxSearchContexts, const U32 maxSessionsToReturn)
	{
		if (m_Initialised)
			return true;

		m_NumSearchProperties	=	maxSearchProperties;
		m_NumSearchContexts		=	(maxSearchContexts < 2) ? 2 : maxSearchContexts; // need a minimum of 2 for GAME_TYPE and GAME_MODE
		m_MaxSessionToEnumerate	=	maxSessionsToReturn;
			
		if (0 == maxSessionsToReturn)
			return false;

		// allocate contexts 
		if (m_NumSearchContexts)
		{
			m_pSearchContexts = reinterpret_cast<void*> (new XUSER_CONTEXT[m_NumSearchContexts]);
			if (NULL == m_pSearchContexts)
			{
				CleanUp();
				return false;
			}
		}
		
		// allocate properties 
		if (m_NumSearchProperties)
		{
			m_pSearchProperties = reinterpret_cast<void*> (new XUSER_PROPERTY[m_NumSearchProperties]);
			if (NULL == m_pSearchProperties)
			{
				CleanUp();
				return false;
			}
		}
				
		// allocate async task.
		m_pAsyncTask = reinterpret_cast<void*> (new C_AsyncTask);
		if (NULL == m_pAsyncTask)
		{
			CleanUp();
			return false;
		}

		// allocate storage for ping number.
		m_pQoSSessionData = new SessionQoSData[m_MaxSessionToEnumerate];
		if (NULL == m_pQoSSessionData)
		{
			CleanUp();
			return false;
		}

		// allocate QoS Addresses
		m_pQoSAddresses = new XNADDR*[m_MaxSessionToEnumerate];
		if (NULL == m_pQoSAddresses)
		{
			CleanUp();
			return false;
		}

		// allocate QoS keys
		m_pQoSKeys = new XNKEY*[m_MaxSessionToEnumerate];
		if (NULL == m_pQoSKeys)
		{
			CleanUp();
			return false;
		}
			
		// allocate QoS KeyIds
		m_pQoSKeyIds = new XNKID*[m_MaxSessionToEnumerate];
		if (NULL == m_pQoSKeyIds)
		{
			CleanUp();
			return false;
		}

		m_Initialised = true;
		return true;
	}


	/// Start searching for a list of game sessions
	bool C_SessionEnumerator::StartSearch(const SysProfile::FullProfile& localPlayer, const U32 queryId, const U32 maxResults, const SessionParameter* pSearchParams, const U32 numSearchParams, const SesionQueryReturnTypes& returnTypes, const bool qosHasDataStream)
	{
		// kill any old searches and release data
		KillSearch();

		if (!m_Initialised || NULL == pSearchParams || 0 == numSearchParams)
			return false;

		// check player profile is valid.
		if (SysProfile::eProfileStateOnline != localPlayer.m_State)
			return false;

		if (localPlayer.m_UserIndex >= XUSER_MAX_COUNT)
			return false;

		if (0 == maxResults)
		{
			LOGFATAL( g_Log, "C_SessionEnumerator::StartSearch() - MUST pass in at least 1 for maxResults");
			return false;
		}
		else if (maxResults > m_MaxSessionToEnumerate)
		{
			LOGFATAL( g_Log, "C_SessionEnumerator::StartSearch() - maxResults is greater then what was specified for maxSessionsToReturn when Initialise() was called.");
			return false;
		}

	
		// first iterate through pSearchParams to make sure we have enough storage to take a copy of the search data..
		bool foundGameType = false;
		bool foundGameMode = false;
		U32 numContexts = 0;
		U32 numProperties = 0;
		U32 sp = 0;

		for (sp = 0; sp <numSearchParams; sp++)
		{
			if (SessionParameter::eTypeContext == pSearchParams[sp].m_ParameterType)
			{
				numContexts+=1;
				switch (pSearchParams[sp].m_Data.m_ContextData.m_Id)
				{
				case X_CONTEXT_GAME_TYPE:
					foundGameType = true;
					break;
				case X_CONTEXT_GAME_MODE:
					foundGameMode = true;
					break;
				}
			}
			else if (SessionParameter::eTypeProperty == pSearchParams[sp].m_ParameterType)
				numProperties+=1;
			else
				return false;
		}

		if (!foundGameType || !foundGameMode)
		{
			LOGFATAL( g_Log, "C_SessionEnumerator::StartSearch() - MUST pass in X_CONTEXT_GAME_TYPE and X_CONTEXT_GAME_MODE contexts \n");
			return false; // always need 2 to cater for X_CONTEXT_GAME_TYPE and X_CONTEXT_GAME_MODE system contexts
		}

		if (numContexts > m_NumSearchContexts)
		{
			LOGFATAL( g_Log, "C_SessionEnumerator::StartSearch() - Passed in to many contexts, allocate more when calling C_SessionEnumerator::Initialise() \n");
			return false; // not allocated enough at start
		}

		if (numProperties > m_NumSearchProperties)
		{
			LOGFATAL( g_Log, "C_SessionEnumerator::StartSearch() - Passed in to many properties, allocate more when calling C_SessionEnumerator::Initialise() \n");
			return false; // not allocated enough at start
		}

		XUSER_PROPERTY* pProperties = reinterpret_cast<XUSER_PROPERTY*> (m_pSearchProperties);
		XUSER_CONTEXT* pContexts = reinterpret_cast<XUSER_CONTEXT*> (m_pSearchContexts);

		// copy context 
		if (numContexts) 
		{
			U32 numCopied = 0;
			for (sp = 0; sp <numSearchParams; sp++)
			{
				if (SessionParameter::eTypeContext == pSearchParams[sp].m_ParameterType)
				{
					pContexts[numCopied].dwContextId	=	pSearchParams[sp].m_Data.m_ContextData.m_Id;
					pContexts[numCopied].dwValue		=	pSearchParams[sp].m_Data.m_ContextData.m_Value;
					numCopied+=1;
				}
			}
		}

		// copy properties
		if (numProperties)
		{
			U32 numCopied = 0;
			for (sp = 0; sp <numSearchParams; sp++)
			{
				if (SessionParameter::eTypeProperty == pSearchParams[sp].m_ParameterType)
				{
					pProperties[numCopied].dwPropertyId = pSearchParams[sp].m_Data.m_PropertyData.m_Id;

					switch (pSearchParams[sp].m_Data.m_PropertyData.m_ValueType)
					{
					case SessionParameter::Int32:
						pProperties[numCopied].value.type = XUSER_DATA_TYPE_INT32;
						pProperties[numCopied].value.nData = pSearchParams[sp].m_Data.m_PropertyData.m_Value.m_S32;
						break;
					case SessionParameter::Int64:
						pProperties[numCopied].value.type = XUSER_DATA_TYPE_INT64;
						pProperties[numCopied].value.i64Data = pSearchParams[sp].m_Data.m_PropertyData.m_Value.m_S64;
						break;
					case SessionParameter::Float32:
						pProperties[numCopied].value.type = XUSER_DATA_TYPE_FLOAT;
						pProperties[numCopied].value.fData = pSearchParams[sp].m_Data.m_PropertyData.m_Value.m_F32;
						break;
					case SessionParameter::Float64:
						pProperties[numCopied].value.type = XUSER_DATA_TYPE_DOUBLE;
						pProperties[numCopied].value.dblData = pSearchParams[sp].m_Data.m_PropertyData.m_Value.m_F64;
						break;
					default:
						return false; // bad property type
						break;
					}
					numCopied+=1;
				}
			}
		}

		C_AsyncTask* pTask = reinterpret_cast<C_AsyncTask*> (m_pAsyncTask);

		// try and determine how much dynamic memory we need to store the search results data.
		DWORD resultsBufferSize = 0;
		DWORD res = XSessionSearch(queryId, localPlayer.m_UserIndex, maxResults, 0, 0, NULL, NULL, &resultsBufferSize, NULL, &(*pTask));
		
		if ((ERROR_INSUFFICIENT_BUFFER != res) || (0 == resultsBufferSize))
			return false; // failed to work out memory required to store search results

		// allocate buffer for search results
		m_pEnumeratedSessionData = new U8[resultsBufferSize];
		if (NULL == m_pEnumeratedSessionData)
			return false;

		// try and start asynchronous search request.
		res = XSessionSearch(queryId, localPlayer.m_UserIndex, maxResults, numProperties, numContexts, pProperties, pContexts, &resultsBufferSize, (XSESSION_SEARCHRESULT_HEADER*)m_pEnumeratedSessionData, &(*pTask));
		if (ERROR_IO_PENDING == res)
		{
			m_QoSHasDataStream = qosHasDataStream;
			m_SearchState = eSearching; // yay poll async task until completion.
		}
		else
			KillSearch();

		// notify listeners
		if(m_SearchState != eSearchNotActive)
		{
			for (	Btl::List<C_SessionEnumeratorListener*>::Iterator it = m_listeners.Begin();
					it != m_listeners.End(); 
					++it)
			{
				(*it)->OnSessionEnumeratorStarted(this);
			}
		}		

		return m_SearchState != eSearchNotActive;
	}


	// Start searching for a a specific game session that we have been invited to.
	bool C_SessionEnumerator::StartSearch(const SysProfile::FullProfile& localPlayer, const SysProfile::GameInvite& gameInvite, const SesionQueryReturnTypes& returnTypes, const bool qosHasDataStream)
	{
		// kill any old searches and release data
		KillSearch();

		if (!m_Initialised)
			return false;

		// check player profile is valid.
		if (SysProfile::eProfileStateOnline != localPlayer.m_State)
			return false;

		if (localPlayer.m_UserIndex >= XUSER_MAX_COUNT)
			return false;

		C_AsyncTask* pTask = reinterpret_cast<C_AsyncTask*> (m_pAsyncTask);

		// try and determine how much dynamic memory we need to store the search results data.
		DWORD resultsBufferSize = 0;
		DWORD res = XSessionSearchByID(gameInvite.m_InviteData.hostInfo.sessionID, localPlayer.m_UserIndex, &resultsBufferSize, NULL, &(*pTask));
		
		if ((ERROR_INSUFFICIENT_BUFFER != res) || (0 == resultsBufferSize))
			return false; // failed to work out memory required to store search results

		// allocate buffer for search results
		m_pEnumeratedSessionData = new U8[resultsBufferSize];
		if (NULL == m_pEnumeratedSessionData)
			return false;

		res = XSessionSearchByID(gameInvite.m_InviteData.hostInfo.sessionID, localPlayer.m_UserIndex, &resultsBufferSize, (XSESSION_SEARCHRESULT_HEADER*)m_pEnumeratedSessionData, &(*pTask));
		if (ERROR_IO_PENDING == res)
		{
			m_QoSHasDataStream = qosHasDataStream;
			m_SearchState = eSearching; // yay poll async task until completion.
		}
		else
			KillSearch();

		// notify listeners
		if(m_SearchState != eSearchNotActive)
		{
			for (	Btl::List<C_SessionEnumeratorListener*>::Iterator it = m_listeners.Begin();
					it != m_listeners.End(); 
					++it)
			{
				(*it)->OnSessionEnumeratorStarted(this);
			}
		}		

		return m_SearchState != eSearchNotActive;
	}

	// filter unresolved qos, and sort sessions.
	void C_SessionEnumerator::FinaliseResults()
	{
		if (!m_Initialised)
			return;

		if(eSearchNotActive == m_SearchState)
		{
			return;
		}
		
		if(eQos != m_SearchState)
		{
			KillSearch();
			return;
		}
	
		// take a copy of the original search data so we can
		XSESSION_SEARCHRESULT_HEADER* pSearch = (XSESSION_SEARCHRESULT_HEADER*)m_pEnumeratedSessionData;
		XSESSION_SEARCHRESULT* pCopy = new XSESSION_SEARCHRESULT[m_NumSessionEnumerated];
		if (!pCopy)
		{
			KillSearch();
			return;
		}

		U32 i=0;
		for (i=0; i<m_NumSessionEnumerated; i++)
			pCopy[i] = pSearch->pResults[i];

		// these will get recalculated
		pSearch->dwSearchResults = 0;
		m_NumSessionEnumerated = 0;
		
		// iterate through QoS data and remove sessions that where not contacted.
		for (i=0; i<m_pQoSHandle->cxnqos; i++)
		{
			if (m_pQoSHandle->axnqosinfo[i].bFlags & XNET_XNQOSINFO_COMPLETE)
			{
				if (m_QoSHasDataStream)
				{
					if (m_pQoSHandle->axnqosinfo[i].bFlags & XNET_XNQOSINFO_TARGET_CONTACTED && m_pQoSHandle->axnqosinfo[i].bFlags & XNET_XNQOSINFO_DATA_RECEIVED)
					{
						m_pQoSSessionData[pSearch->dwSearchResults].m_Ping			= m_pQoSHandle->axnqosinfo[i].wRttMedInMsecs;	// store QoS Ping data for this contacted session.			
						m_pQoSSessionData[pSearch->dwSearchResults].m_pQoSData		= m_pQoSHandle->axnqosinfo[i].pbData;			// store pointer to QoS data
						m_pQoSSessionData[pSearch->dwSearchResults].m_QoSDataSize	= m_pQoSHandle->axnqosinfo[i].cbData;			//	store num bytes in Qos Data	
						pSearch->pResults[pSearch->dwSearchResults++]				= pCopy[i];										// coppy session data
					}
				}
				else
				{
					if (m_pQoSHandle->axnqosinfo[i].bFlags & XNET_XNQOSINFO_TARGET_CONTACTED)
					{
						m_pQoSSessionData[pSearch->dwSearchResults].m_Ping			= m_pQoSHandle->axnqosinfo[i].wRttMedInMsecs; // store QoS Ping data for this contacted session.			
						m_pQoSSessionData[pSearch->dwSearchResults].m_pQoSData		= NULL; // No Data required
						m_pQoSSessionData[pSearch->dwSearchResults].m_QoSDataSize	= 0;	// No Data  required
						pSearch->pResults[pSearch->dwSearchResults++]				= pCopy[i];	// coppy session data
					}
				}
			}
		}

		m_NumSessionEnumerated = pSearch->dwSearchResults;
		// delete temp storage
		delete [] pCopy;
		pCopy = NULL;

		// sort contacted sessions based of lowest ping. (if enough sessions were returned)
		if (m_NumSessionEnumerated > 1)
		{
			for (U32 outer=0; outer<m_NumSessionEnumerated-1; outer++)
			{
				S32 lowerPingIndex	= -1;
				U32 testPing		= m_pQoSSessionData[outer].m_Ping;	 

				for (U32 inner=outer+1; inner<m_NumSessionEnumerated; inner++)
				{
					if (testPing > m_pQoSSessionData[inner].m_Ping)
					{
						lowerPingIndex	=	inner;
						testPing		=	m_pQoSSessionData[inner].m_Ping;
					}
				}

				// swap data if ping is lower.
				if (lowerPingIndex != -1) 
				{
					Btl::Swap(m_pQoSSessionData[outer], m_pQoSSessionData[lowerPingIndex]);
					Btl::Swap(pSearch->pResults[outer], pSearch->pResults[lowerPingIndex]);
				}
			}
		}

		// end search. If sessions have been returned leave QoS Data active so the game can access it.
		// Search memory and QoS memory will be released when game calls KillSearch()
		m_SearchState = eSearchNotActive;

		if (0 == m_NumSessionEnumerated)
			KillSearch();

		// iterate all sessions
		for(U32 i = 0; i < m_NumSessionEnumerated; i ++)
		{							
			// notify listeners
			SessionQoSData qosData;
			U32 openSlots;
			U32 maxSlots;
			SessionSecurity security;
			bdCommonAddrRef address;
			Btl::Vector< NetPlay::SessionParameter > params;

			if(	GetQoSDataFromEnumerated(i, qosData) && 
				GetSlotInfoFromEnumerated(i, openSlots, maxSlots) && 
				GetHostInfoFromEnumerated(i, security, address) &&
				GetParametersFromEnumerated(i, params))
			{
				for (	Btl::List<C_SessionEnumeratorListener*>::Iterator it = m_listeners.Begin();
						it != m_listeners.End(); 
						++it)
				{
						(*it)->OnSessionEnumerated(	this, 
													i, 
													params,
													qosData,
													security,
													address,
													openSlots,
													maxSlots);
				}
			}
		}

		// notify listeners search stopped.
		for (	Btl::List<C_SessionEnumeratorListener*>::Iterator it = m_listeners.Begin();
				it != m_listeners.End(); 
				++it)
		{
			(*it)->OnSessionEnumeratorFinished(this, m_NumSessionEnumerated);
		}
	}
	

	// Perforrms any asynchronous tasks, should call at all times
	void C_SessionEnumerator::Poll(void)
	{
		if (!m_Initialised)
			return;

		if (eSearchNotActive == m_SearchState)
			return;

		if (eSearching == m_SearchState)
		{
			// still retrieving sessions from the server
			C_AsyncTask* pTask = reinterpret_cast<C_AsyncTask*> (m_pAsyncTask);
			if (!pTask->Finished())
				return;

			if (pTask->ErrorCode() == ERROR_SUCCESS)
			{
				pTask->Close();

				const XSESSION_SEARCHRESULT_HEADER* pSearch = (XSESSION_SEARCHRESULT_HEADER*)m_pEnumeratedSessionData;
				m_NumSessionEnumerated = pSearch->dwSearchResults;
				if (m_NumSessionEnumerated)
				{
					// copy session security info and addresses for Qos scan..
					for (U32 i=0; i<m_NumSessionEnumerated; i++)
					{
						m_pQoSAddresses[i]	=	&pSearch->pResults[i].info.hostAddress;
						m_pQoSKeys[i]		=	&pSearch->pResults[i].info.keyExchangeKey;
						m_pQoSKeyIds[i]		=	&pSearch->pResults[i].info.sessionID;
					}

					// now try and start QoS scan..
					const S32 res = XNetQosLookup(m_NumSessionEnumerated, (const XNADDR**)m_pQoSAddresses, (const XNKID**)m_pQoSKeyIds, (const XNKEY**)m_pQoSKeys, 0, NULL, NULL, 8, 16384, 0, NULL, &m_pQoSHandle);
					if (0 != res)
						KillSearch();
					else
						m_SearchState = eQos;
				}
				else
					KillSearch();
			}
			else
				KillSearch();
		}
		else if (eQos == m_SearchState)
		{
			// receiving Qos data from each session.
			if (NULL == m_pQoSHandle)
				KillSearch();
			else
			{
				// have all QoS probes have been resolved.
				if (!m_pQoSHandle->cxnqosPending)
				{
					FinaliseResults();

					// game can now access search results.
					m_SearchState = eSearchNotActive;
				}
			}
		}
	}


	// how many sessions did we enumerate..
	U32	C_SessionEnumerator::NumSessionsEnumerated(void) const
	{
		if (!m_Initialised)
			return 0;
		if (m_SearchState != eSearchNotActive)
			return 0;
		return m_NumSessionEnumerated;
	}


	// get the hosting part of the enumerated session
	bool C_SessionEnumerator::GetHostInfoFromEnumerated(const U32 sessionIndex, SessionSecurity& sessionSecurity, bdCommonAddrRef& sessionAddress) const 
	{
		if (!m_Initialised)
			return false;

		if (eSearchNotActive != m_SearchState)
			return false; // no search active

		if (0 == m_NumSessionEnumerated)
			return false; // some sessions enumerated

		if (sessionIndex >= m_NumSessionEnumerated)
			return false; 

		const XSESSION_SEARCHRESULT_HEADER* pSearch = (XSESSION_SEARCHRESULT_HEADER*)m_pEnumeratedSessionData;
		
		bdNetImpl *const net = bdNet::getInstance();
		if (net->getStatus() != bdNetImpl::BD_NET_DONE)
			return false;

		// session key data..
		sessionSecurity.m_ArbitrationNonce	=	0;
		sessionSecurity.m_Key				=	pSearch->pResults[sessionIndex].info.keyExchangeKey;
		sessionSecurity.m_Id				=	pSearch->pResults[sessionIndex].info.sessionID;

		// remote address..
		sessionAddress = new bdCommonAddr(net->getLocalCommonAddr(), pSearch->pResults[sessionIndex].info.hostAddress, 1000); // assume port 1000 since all xbox360 traffic should be using this in game ! 
		
		return true;
	}


	// get open and max player slots for the specified session
	bool C_SessionEnumerator::GetSlotInfoFromEnumerated(const U32 sessionIndex, U32& openSlots, U32& maxSlots) const
	{
		if (!m_Initialised)
			return false;

		if (eSearchNotActive != m_SearchState)
			return false; // no search active

		if (0 == m_NumSessionEnumerated)
			return false; // some sessions enumerated

		if (sessionIndex >= m_NumSessionEnumerated)
			return false; 

		const XSESSION_SEARCHRESULT_HEADER* pSearch = (XSESSION_SEARCHRESULT_HEADER*)m_pEnumeratedSessionData;
		const XSESSION_SEARCHRESULT& s = pSearch->pResults[sessionIndex];

		openSlots	=	s.dwOpenPrivateSlots + s.dwOpenPublicSlots;
		maxSlots	=	openSlots + (s.dwFilledPrivateSlots + s.dwFilledPublicSlots);

		return true;
	}

	/// extract a specific parameter from the unumerated session data
	bool C_SessionEnumerator::GetSessionParameterFromEnumerated(const U32 sessionIndex, const U32 paramId, SessionParameter& paramData) const
	{	
		// clear outgoing parameter incase its not blank on entry.
		SessionParameter tempParam;
		paramData = tempParam;

		if (!m_Initialised)
			return false;

		if (eSearchNotActive != m_SearchState)
			return false; // no search active

		if (0 == m_NumSessionEnumerated)
			return false; // some sessions enumerated

		if (sessionIndex >= m_NumSessionEnumerated)
			return false; 

		const XSESSION_SEARCHRESULT_HEADER* pSearch = (XSESSION_SEARCHRESULT_HEADER*)m_pEnumeratedSessionData;
		const XSESSION_SEARCHRESULT& s = pSearch->pResults[sessionIndex];
		
		U32 i=0;
		// check enumerated contexts..
		for (i=0; i<s.cContexts; i++)
		{
			XUSER_CONTEXT& c = s.pContexts[i];
			if (paramId == c.dwContextId)
			{
				// found
				paramData.CreateContext(paramId, c.dwValue);
				return true;
			}
		}

		// check enumerated properties
		for (i=0; i<s.cProperties; i++)
		{
			XUSER_PROPERTY& p = s.pProperties[i];
			if (paramId == p.dwPropertyId)
			{
				// found..
				switch (p.value.type)
				{
				case XUSER_DATA_TYPE_INT32:
					paramData.CreatePropertyS32(paramId, p.value.nData);
					return true;
					break;
				case XUSER_DATA_TYPE_INT64:
					paramData.CreatePropertyS64(paramId, p.value.i64Data);
					return true;
					break;
				case XUSER_DATA_TYPE_FLOAT:
					paramData.CreatePropertyF32(paramId, p.value.fData);
					return true;
					break;
				case XUSER_DATA_TYPE_DOUBLE:
					paramData.CreatePropertyF64(paramId, p.value.dblData);
					return true;
					break;
				default:
					// un supported property type.
					return false;
					break;
				}
			}
		}

		return false;
	}

/// extract a specific parameter from the unumerated session data
	bool C_SessionEnumerator::GetParametersFromEnumerated(const U32 sessionIndex, Btl::Vector< SessionParameter>& paramData) const
	{
		if (!m_Initialised)
			return false;

		// used for listeners.
		if (eSearchNotActive != m_SearchState)
			return false; // no search active

		if (0 == m_NumSessionEnumerated)
			return false; // some sessions enumerated

		if (sessionIndex >= m_NumSessionEnumerated)
			return false; 

		const XSESSION_SEARCHRESULT_HEADER* pSearch = (XSESSION_SEARCHRESULT_HEADER*)m_pEnumeratedSessionData;
		const XSESSION_SEARCHRESULT& s = pSearch->pResults[sessionIndex];
		
		// placeholder
		SessionParameter tempParam;
				
		U32 i=0;
		// check enumerated contexts..
		for (i=0; i<s.cContexts; i++)
		{
			XUSER_CONTEXT& c = s.pContexts[i];
			tempParam.CreateContext(c.dwContextId, c.dwValue);
			paramData.PushBack(tempParam);
		}

		// check enumerated properties
		for (i=0; i<s.cProperties; i++)
		{
			XUSER_PROPERTY& p = s.pProperties[i];
			// found..
			switch (p.value.type)
			{
			case XUSER_DATA_TYPE_INT32:
				tempParam.CreatePropertyS32(p.dwPropertyId, p.value.nData);
				paramData.PushBack(tempParam);
				break;
			case XUSER_DATA_TYPE_INT64:
				tempParam.CreatePropertyS64(p.dwPropertyId, p.value.i64Data);
				paramData.PushBack(tempParam);
				break;
			case XUSER_DATA_TYPE_FLOAT:
				tempParam.CreatePropertyF32(p.dwPropertyId, p.value.fData);
				paramData.PushBack(tempParam);
				break;
			case XUSER_DATA_TYPE_DOUBLE:
				tempParam.CreatePropertyF64(p.dwPropertyId, p.value.dblData);
				paramData.PushBack(tempParam);
				break;
			default:
				// un supported property type.
				break;
			}
		}
		return true;
	}
	
	// extract QOS data for specific session.
	bool C_SessionEnumerator::GetQoSDataFromEnumerated(const U32 sessionIndex, SessionQoSData& qosData) const
	{
		// clear first..
		SessionQoSData blank;
		qosData = blank;

		if (!m_Initialised)
			return false;

		if (eSearchNotActive != m_SearchState)
			return false; // no search active

		if (0 == m_NumSessionEnumerated)
			return false; // some sessions enumerated

		if (sessionIndex >= m_NumSessionEnumerated)
			return false; 

		qosData = m_pQoSSessionData[sessionIndex];
		return true;
	}

	/// stop asynchronous search but keep session storage.
	void C_SessionEnumerator::StopSearch()
	{
		if (!m_Initialised)
			return;

		// no search to stop.
		if(eSearchNotActive == m_SearchState)
			return;
				
		// close task.
		C_AsyncTask* pT = reinterpret_cast<C_AsyncTask*> (m_pAsyncTask);
		if (pT) pT->Close();

		// sort what session we have so far.
		FinaliseResults();
	}

	/// stop asynchronous search and release session storage
	void C_SessionEnumerator::KillSearch(void)
	{
		if (!m_Initialised)
			return;

		if(m_SearchState != eSearchNotActive)
		{
			// notify listeners
			for (	Btl::List<C_SessionEnumeratorListener*>::Iterator it = m_listeners.Begin();
					it != m_listeners.End(); 
					++it)
			{
				(*it)->OnSessionEnumeratorFinished(this, 0);
			}
		}

		KillQoS();

		C_AsyncTask* pT = reinterpret_cast<C_AsyncTask*> (m_pAsyncTask);
		if (pT)
			pT->Close();

		if (m_pEnumeratedSessionData)
			delete [] m_pEnumeratedSessionData;
		m_pEnumeratedSessionData = NULL;
		m_NumSessionEnumerated = 0;
		m_QoSHasDataStream = false;
		m_SearchState = eSearchNotActive;
	}


	// termionate QoS scan and release QoS resources.
	void C_SessionEnumerator::KillQoS(void)
	{
		if (!m_Initialised)
			return;

		if (NULL != m_pQoSHandle)
			XNetQosRelease(m_pQoSHandle);
		m_pQoSHandle = NULL;
	}


	// private function to cleanup any dynamic resources.
	void C_SessionEnumerator::CleanUp(void)
	{
		KillSearch();

		//release QoS keyIds
		if (NULL != m_pQoSKeyIds)
			delete [] m_pQoSKeyIds;
		m_pQoSKeyIds = NULL;

		//release QoS keys
		if (NULL != m_pQoSKeys)
			delete [] m_pQoSKeys;
		m_pQoSKeys = NULL;

		// release QoS addresses..
		if (NULL != m_pQoSAddresses)
			delete [] m_pQoSAddresses;
		m_pQoSAddresses = NULL;

		// release ping storage..
		if (NULL != m_pQoSSessionData)
			delete [] m_pQoSSessionData;
		m_pQoSSessionData = NULL;

		// destroy async task is created and active.
		C_AsyncTask* pT = reinterpret_cast<C_AsyncTask*> (m_pAsyncTask);
		if (pT)
			delete pT;
		m_pAsyncTask = NULL; 

		// destroy dynamic..
		if (m_pSearchProperties)
			delete [] m_pSearchProperties;
		m_pSearchProperties = NULL;

		if (m_pSearchContexts)
			delete [] m_pSearchContexts;
		m_pSearchContexts = NULL;

		m_NumSearchProperties = 0;
		m_NumSearchContexts= 0;
		m_MaxSessionToEnumerate = 0;

		m_NumSessionEnumerated = 0;

		m_Initialised = false;
	}
}
