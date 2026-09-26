#include <StdAfx.h>
#include <Network/Core.h>
#include <Network/Securekey.h>
#include <System/Time/Time.h>
#include <bdCore/bdCore.h>
#include <bdLobby/bdLobbyService.h>


#ifdef CFG_ALLOW_OPTIMIZE_OFF
#pragma optimize( "", off ) // CFG_ALLOW_OPTIMIZE_OFF
#endif

namespace NetPlay
{
	// static instance..
	C_Core C_Core::s_NetworkCore;
  
  
	// constructor
	C_Core::C_Core() : 
	m_StartedUp(false),	m_ValidLocalAddress(false), m_ScanAddressTimer(0), 
	m_NetCableConnected(false), m_ScanForLocalAddress(false),m_BitFields(0)
	{
	}  

	// destructor
	C_Core::~C_Core()
	{
	}  
       

	// Startup network layer
	void C_Core::Initialise(const bool bypassSecurity, const bool networkStackAlreadyStarted)
	{
		if (m_StartedUp)
			return;

		// fire upm demonware, default memory managers can be overriden later by the game code if required.
		bdCore::init(true);

		m_pDemonwareLobby = new bdLobbyService;
		if (NULL == m_pDemonwareLobby)
		{
			Shutdown();
			return;
		}

		// startup Xnet first
		XNetStartupParams xnsp;
		ZeroMemory(&xnsp, sizeof(xnsp));
		xnsp.cfgSizeOfStruct = sizeof(xnsp);
#ifndef CFG_GOLD
		// flags are ignored when linking with secure Xenon libraries..
		xnsp.cfgFlags = bypassSecurity ? XNET_STARTUP_BYPASS_SECURITY : 0; 
#endif

		// increase size of data we can supply as part of QoS queries..
		xnsp.cfgQosDataLimitDiv4 = 512/4;

		// we need a large number of security associations to support parties + large numbers of players.
		xnsp.cfgSecRegMax  = 64;

		if (NO_ERROR == XNetStartup(&xnsp))
		{
			//XNetStartupParams p;
			//DWORD size = sizeof(p);
			//const INT val = XNetGetOpt(XNET_OPTID_STARTUP_PARAMS, (BYTE*)&p, &size);

			WSADATA WsaData;
			if (NO_ERROR == WSAStartup(WINSOCK_VERSION, &WsaData))
			{
				if (S_OK == XOnlineStartup())
				{
					m_StartedUp = true;
					m_ValidLocalAddress = false;
					m_ScanAddressTimer = 0;
					m_NetCableConnected = false;
					m_ScanForLocalAddress = false;
				}
				else
					Cleanup();
			}
			else
				Cleanup();
		}
	}


	// shutdown network layer
	void C_Core::Shutdown(void)
	{
		Cleanup();
	}

	
	// private function to clean up when shutting down..
	void C_Core::Cleanup(void)
	{
		if (NULL != m_pDemonwareLobby)
		{
			m_pDemonwareLobby->disconnect();
			delete m_pDemonwareLobby;
			m_pDemonwareLobby = NULL;
		}

		XOnlineCleanup();	// free live..
		WSACleanup();
		XNetCleanup();

		bdCore::quit();

		m_StartedUp = false;	
		m_ValidLocalAddress = false;
		m_ScanAddressTimer = 0; 	
		m_NetCableConnected = false; 
		m_ScanForLocalAddress = false;
		m_BitFields = 0;
	}


	// poll once per frame if possible
	// used for asynchronous retrieval of the network address
	void C_Core::Poll(const bool forceNetworkLinkCheck)
	{
		if (!m_StartedUp)  
			return;

		const DWORD currentTick = GetTickCount();

		if (m_ScanAddressTimer == 0 || currentTick >= m_ScanAddressTimer || forceNetworkLinkCheck)
		{
			m_ScanAddressTimer = currentTick + 100;	
			m_ScanAddressTimer = (m_ScanAddressTimer < currentTick) ? 0 : m_ScanAddressTimer; //check for wrap..

			// first check network cable on a periodic basis
			const bool oldCableStatus = m_NetCableConnected;
			const bool newCableStatus = ((XNetGetEthernetLinkStatus() & XNET_ETHERNET_LINK_ACTIVE) != 0);

			if (!newCableStatus)
			{
				// no net cable inserted..
				m_ScanForLocalAddress = false;
				m_ValidLocalAddress = false;
			}
			else if (!oldCableStatus && newCableStatus)
			{
				// rescan for the local XN ADDRESS.. since cable has been removed and re-inserted
				m_ScanForLocalAddress = true;
			}

			// set new network cable status.
			m_NetCableConnected = newCableStatus;
		}

		// are we scanning for a local XN address ??
		if (m_ScanForLocalAddress || forceNetworkLinkCheck)
		{
			XNADDR xn;
			const DWORD xnStatus = XNetGetTitleXnAddr(&xn);
			if (XNET_GET_XNADDR_PENDING != xnStatus) 
			{
				m_ScanForLocalAddress = false;
				if (XNET_GET_XNADDR_NONE == xnStatus || XNET_GET_XNADDR_TROUBLESHOOT == xnStatus)
				{
					LOGINFO( g_Log,  "XNADDR retrieval Failed\n");
					m_ValidLocalAddress = false;
				}
				else
				{
					m_ValidLocalAddress = true;
				}
			}
			else
				m_ValidLocalAddress = false; // XNADDR still being found..
		}
	}

	// force the system to scan for our local network address.. blocks while executing.
	void C_Core::ForceLocalAddressScan(void)
	{
		if (m_StartedUp)
		{
			bool done = false;
			XNADDR x;
			while (!done)
			{
				const DWORD s = XNetGetTitleXnAddr(&x);
				if (XNET_GET_XNADDR_PENDING != s)
				{
					done = true;
					if (XNET_GET_XNADDR_NONE == s)
						m_ValidLocalAddress = false;
					else
						m_ValidLocalAddress = true;
				}	
			}
		}
	}

	// generate a unique 64 bit identifier for this console..
	bool C_Core::GenerateMachineIdentifier(U64& machineId)
	{
		machineId = 0;
		if (!m_StartedUp)
			return false;
		
		if (!m_ValidLocalAddress)
			return false;	// must wait for address to be discovered before using this function.
	
		// read xnaddr
		XNADDR x;
		const DWORD s = XNetGetTitleXnAddr(&x);
		if (XNET_GET_XNADDR_PENDING == s || XNET_GET_XNADDR_NONE == s)
			return false;

		// [GNT - 9.5.2006] - We are not online (LAN game).
		if (x.inaOnline.S_un.S_addr == 0)
		{
			machineId = 0;
			U32 uSize = (sizeof(machineId) < sizeof(x.abEnet))? sizeof(machineId) : sizeof(x.abEnet);

			// [GNT - 9.5.2006] - Use MAC address
			memcpy(&machineId, x.abEnet, uSize);

			if(sizeof(x.abEnet) == 6)
			{
				// [GNT - 26.5.2006] - Add random bits
				U64 temp;
				temp = rand()%(256); // bit of randomness
				machineId |= (temp<<48);

				temp = rand()%(256); // bit of randomness
				machineId |= (temp<<56);
			}
			return true;
		}
		// [GNT - 15.5.2006] - We are online, generate identifier
		else if(XNetXnAddrToMachineId(&(x), &machineId) == 0)
		{
			return true;
		}
		// [GNT - 15.5.2006] - failed.
		else
		{
			// [GNT - 9.5.2006] - We are online, but still failed to generate a number.
			LOGWARN(g_Log, DBSTR "Failed to generate machine identifier.\n");
		}
		return false;
	}
}

#ifdef CFG_ALLOW_OPTIMIZE_OFF
#pragma optimize( "", on ) // CFG_ALLOW_OPTIMIZE_OFF
#endif
