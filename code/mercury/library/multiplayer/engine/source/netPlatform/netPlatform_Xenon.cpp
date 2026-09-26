
#include "stdafx.h"
#include "./netPlatform_Xenon.h"
#include "./netPlatform_Messages.h"
#include "netDebug/netDebug.h"
#include <Network/Core.h>
#include <SystemProfile/SystemProfileManager.h>
#include <bdNet/bdNetStartParams.h>

#ifdef CFG_ALLOW_OPTIMIZE_OFF
#pragma optimize( "", off )
#endif

namespace netPlatform
{
	Xenon::Xenon()
	{
	}

	Xenon::~Xenon()
	{}

	int	Xenon::platformNATtype()
	{
		switch (XOnlineGetNatType())
		{
		case XONLINE_NAT_OPEN:
			return netPlatform::NAT_TYPE_OPEN;
		case XONLINE_NAT_MODERATE:
			return netPlatform::NAT_TYPE_MODERATE;
		case XONLINE_NAT_STRICT:
			return netPlatform::NAT_TYPE_STRICT;
		default:
			LOGWARN(netDebug::g_Log, "<netPlatform::Xenon> NAT type not handled correctly.");
			return netPlatform::NAT_TYPE_NOT_SET;
		}
		return netPlatform::NAT_TYPE_NOT_SET;
	}
		
	// platform functions for state machine
	void Xenon::platformInitialise()
	{
	}

	void Xenon::platformShutdown()
	{
	}

	void Xenon::platformUpdate()
	{
	}

	bool Xenon::platformRequiresSignIn()
	{
		return false;
	}

	bool Xenon::platformRequiresAuthentication()
	{
		return false;
	}

	bool Xenon::platformRequiresTicket()
	{
		return false;
	}

	bool Xenon::platformRequiresLobby()
	{
		return false;
	}

	int Xenon::platformTitleId()
	{
		return 0x4156081F;
	}
	
	const char* Xenon::platformAuthAddress()
	{
		return "NO AUTH ADDRESS";
	}

	int	Xenon::platformAuthPort()
	{
		return 0;
	}

	const char* Xenon::platformLsgAddress()
	{
		return "NO LSG ADDRESS";
	}

	int Xenon::platformLsgPort()
	{
		return 0;
	}

	bool Xenon::platformSignIn()
	{
		return false;
	}

	bool Xenon::platformNetParams(bdNetStartParams& params)
	{
		params.m_onlineGame = (m_onlineMode == MODE_ONLINE)? true : false;

		if (!NetPlay::C_Core::Instance().CableConnected() && !NetPlay::C_Core::Instance().ValidLocalAddress())
		{
			return false;
		}
		return true;
	}

	bool Xenon::platformRequestTicket()
	{
		return false;
	}

	bool Xenon::platformAuthentication()
	{
		return false;
	}

	bool Xenon::platformCleanup()
	{
		return false;
	}

	bool Xenon::platformHasOnlineAccount()
	{
		SysProfile::FullProfile outProfileData;
		SysProfile::C_Manager::Instance().GetProfile(SysProfile::C_Manager::Instance().GetPrimaryPadNo(), outProfileData);
		return (outProfileData.m_State == SysProfile::eProfileStateOnline);
	}

	int	Xenon::platformStartGetHostByName(const bdNChar8 *const domainNameStr, bdGetHostByName** getHostByName, const bdInetAddr resolvedAddr)
	{
		LOGWARN(g_Log, "<netPlatform::Xenon> implement platformStartGetHostByName()");
		return (int) -1;
	}

	int	Xenon::platformPumpGetHostByName(bdGetHostByName **getHostByName, bdInetAddr &resolvedAddr)
	{
		LOGWARN(g_Log, "<netPlatform::Xenon> implement platformPumpGetHostByName()");
		return (int) -1;
	}	

	bool Xenon::profileIdentifiable(const SysProfile::FullProfile& profile)
	{
		if(	profile.m_State == SysProfile::eProfileStateNotActive ||
			profile.m_State == SysProfile::eProfileStateBlagged ||
			profile.m_IsGuest)
		{
			return false;
		}
		return true;
	}
	bdOnlineUserID Xenon::getUserId(const SysProfile::UserIdentifier &user)
	{
		bdOnlineUserID id;

		if(user.m_Uid == 0)
		{
		#if defined( CFG_DEBUG )
			LOGFATAL(netDebug::g_Log, "<netPlatform::Xenon::getUserId()> 'null' user identifier.");
		#endif
			id = 0;
		}
		else
		{
			id = user.m_Uid;
		}
		return id;
	}

	bool Xenon::sameUser(const SysProfile::UserIdentifier &a, const SysProfile::UserIdentifier &b)
	{
		return (a == b);
	}

	int	Xenon::getHandleStatus(const bdAddrHandleRef& addrHandle)
	{
		if(addrHandle.isNull())
			return ADDRESS_STATUS_IDLE;

		int xnStatus = XNetGetConnectStatus(addrHandle->m_addr);

		switch(xnStatus)
		{
		default:
		case XNET_CONNECT_STATUS_IDLE:			return ADDRESS_STATUS_IDLE;
		case XNET_CONNECT_STATUS_PENDING:		return ADDRESS_STATUS_PENDING;
		case XNET_CONNECT_STATUS_CONNECTED:		return ADDRESS_STATUS_CONNECTED;
		case XNET_CONNECT_STATUS_LOST:			return ADDRESS_STATUS_LOST;
		}
	}
		
	const char* Xenon::getAddrInfo(const bdCommonAddrRef& addr, char* str, int len)
	{
		if(addr.isNull())
		{
			sprintf_s(str, len, "NULL");
			return str;
		}

		XNADDR xnaddr = addr->getXNAddr();
		
		if(0)
		{
			sprintf_s(str, len, "%d.%d.%d.%d.%d.%d", 
									xnaddr.abEnet[0], xnaddr.abEnet[1], xnaddr.abEnet[2], 
									xnaddr.abEnet[3], xnaddr.abEnet[4], xnaddr.abEnet[5]);
			return str;
		}
			
		if(xnaddr.ina.s_addr != 0)
		{
			sprintf_s(str, len, "%d.%d.%d.%d:%d", 
				xnaddr.ina.s_net, xnaddr.ina.s_host, xnaddr.ina.s_lh, xnaddr.ina.s_impno, xnaddr.wPortOnline);
		}
		else
		{
			sprintf_s(str, len, "%d.%d.%d.%d:%d", 
				xnaddr.inaOnline.s_net, xnaddr.inaOnline.s_host, xnaddr.inaOnline.s_lh, xnaddr.inaOnline.s_impno, xnaddr.wPortOnline);
		}
		return str;
	}

	const char* Xenon::getHandleInfo(const bdAddrHandleRef &handle, char* str, int len)
	{
		if(handle.isNull())
		{
			sprintf_s(str, len, "NULL");
			return str;
		}

		bdInAddr addr(handle->m_addr.s_addr);
		int port = handle->m_port;
		int addrLen = addr.toString(str, len);
		
		// append port
		sprintf_s((str + addrLen), (len - addrLen), ":%d", port);
		return str;
	}

	#include "Xbdm.h"
	const char* Xenon::getConsoleName(char* name, int len)
	{
		unsigned long count = len;
		HRESULT result = DmGetXboxName(name, &count);
		
		if(result == XBDM_NOERR)
		{
			return name;
		}
		else
		{
			if(getAddrInfo(myCommonAddr(), name, len))
				return name;
			
			sprintf_s(name, len, "XBOX360");
			return name;
		}
	}
};

#ifdef CFG_ALLOW_OPTIMIZE_OFF
#pragma optimize( "", on )
#endif

