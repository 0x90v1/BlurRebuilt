
#include "stdafx.h"
#include "netTransport/netTransport_Addr.h"
#include "netTransport/netTransport_Enums.h"
#include "netPlatform/netPlatform_Core.h"

#ifdef CFG_ALLOW_OPTIMIZE_OFF
#pragma optimize( "", off )
#endif

namespace netTransport
{
	Addr::Addr()
	{
		memset(m_buffer, 0, sizeof(m_buffer));
	}

	const char* Addr::toString(char* str, int len) const
	{
		if(netPlatform::Core::instance()->myCommonAddr().isNull())
			return "no local address";

		bdCommonAddr addr;
		bool ok = addr.deserialize(netPlatform::Core::instance()->myCommonAddr(), m_buffer);
		if(!ok) 
		{
			LOGERROR(Debugging::g_logger, "<Addr> failed to deserialise address.");
			sprintf_s(str, len, "ERROR");
			return str;
		}

	#if defined(PLATFORM_XENON)
		XNADDR xnaddr = addr.getXNAddr();
		
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
	#else
		bdAddr publicAddr = addr.getPublicAddr();
		bdAddr localAddr = addr.getLocalAddrByIndex(0);
		//publicAddr.toString(str, len);
		localAddr.toString(str, len);
		return str;
	#endif
	}
		
	bool Addr::toAddr(bdCommonAddrRef addr) const
	{
		addr->deserialize(netPlatform::Core::instance()->myCommonAddr(), m_buffer);

		bool ok = addr->deserialize(netPlatform::Core::instance()->myCommonAddr(), m_buffer);
		if(!ok) LOGERROR(Debugging::g_logger, "<Addr> failed to deserialise address.");
		return ok;
	}

	bool Addr::isLoopback() const
	{
		bdCommonAddr addr;
		bool ok = addr.deserialize(netPlatform::Core::instance()->myCommonAddr(), m_buffer);
		if(!ok) LOGERROR(Debugging::g_logger, "<Addr> failed to deserialise address.");
		return addr.isLoopback();
	}

	bool Addr::fromAddr(bdCommonAddrRef addr)
	{
		if(addr.isNull())
		{
			memset(m_buffer, 0, sizeof(m_buffer));
			return true;
		}

		addr->serialize(m_buffer);
		return true;
	}

	bool Addr::operator == (bdCommonAddrRef addr) const
	{
		bdCommonAddr temp;

		bool ok = temp.deserialize(netPlatform::Core::instance()->myCommonAddr(), m_buffer);
		if(!ok) LOGERROR(Debugging::g_logger, "<Addr> failed to deserialise address.");
		return (ok && (*addr) == temp);
	}
		
	bool Addr::operator == (const Addr& addr) const
	{
		return memcmp(addr.m_buffer, m_buffer, sizeof(m_buffer)) == 0;
	}
}

#ifdef CFG_ALLOW_OPTIMIZE_OFF
#pragma optimize( "", on )
#endif

