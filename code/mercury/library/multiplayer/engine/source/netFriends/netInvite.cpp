#include "stdafx.h"

#include "./netInvite.h"
#include "netDebug/netDebug.h"
#include "netPlatform/netPlatform_Core.h"
#include <bdNet/bdLANDiscovery/bdGameInfo.h>
#include "ui/util/WideString.h"

#ifdef CFG_ALLOW_OPTIMIZE_OFF
#pragma optimize( "", off )
#endif

bdGameInfoRef netInvite::getGameInfo()
{
#if defined(PLATFORM_XENON)
	MEMORY_TRACKER_PUSH("network.multiplayer");
	bdGameInfoRef gameInfo = new bdGameInfo;
	gameInfo->setSecurityID(m_InviteData.hostInfo.sessionID);
	gameInfo->setSecurityKey(m_InviteData.hostInfo.keyExchangeKey);
	gameInfo->setTitleID(netPlatform::Core::instance()->titleId());
	MEMORY_TRACKER_POP("network.multiplayer");
	return gameInfo;
#else
	MEMORY_TRACKER_PUSH("network.multiplayer");
	bdGameInfoRef gameInfo = new bdGameInfo;
	gameInfo->setSecurityID(m_Id);
	gameInfo->setSecurityKey(m_Key);
	gameInfo->setTitleID(netPlatform::Core::instance()->titleId());
	MEMORY_TRACKER_POP("network.multiplayer");
	return gameInfo;
#endif
}

int netInvite::debugstring(char* buff, int len)
{
#if defined(PLATFORM_XENON)
	char str_sessionId[128];
	char str_sessionKey[128];
	char str_sessionAddr[128];

	netDebug::getByteInfo(&m_InviteData.hostInfo.sessionID, sizeof(m_InviteData.hostInfo.sessionID),						str_sessionId,		sizeof(str_sessionId));
	netDebug::getByteInfo(&m_InviteData.hostInfo.keyExchangeKey, sizeof(m_InviteData.hostInfo.keyExchangeKey),			str_sessionKey,		sizeof(str_sessionKey));
	netDebug::getByteInfo(&m_InviteData.hostInfo.hostAddress.inaOnline, sizeof(m_InviteData.hostInfo.hostAddress.inaOnline), str_sessionAddr,	sizeof(str_sessionAddr));

	int count = sprintf_s(	buff, len,
							"active(%s)\nfromInvite(%s)\ntitleId(%u)\ninviter(%llu)\ninvitee(%llu)\n",//\nsessionId(%s)\nsessionKey(%s)\nhostAddr(%s)\n", 
							m_Active?"true":"false", 
							m_InviteData.fFromGameInvite?"true":"false", 
							m_InviteData.dwTitleID,
							m_InviteData.xuidInviter,
							m_InviteData.xuidInvitee,
							str_sessionId[128],
							str_sessionKey[128],
							str_sessionAddr[128]);
#else
	char str_sessionId[128];
	char str_sessionKey[128];

	netDebug::getByteInfo(&m_Id , sizeof(m_Id) ,	str_sessionId,	sizeof(str_sessionId));
	netDebug::getByteInfo(&m_Key, sizeof(m_Key),	str_sessionKey,	sizeof(str_sessionKey));

	int count = sprintf_s(	buff, len,
							"active(%s)\nsessionId(%s)\nsessionKey(%s)\n", 
							m_Active?"true":"false", 
							str_sessionId[128],
							str_sessionKey[128]);
#endif

	return count;
}

netInvite::netInvite(const SysProfile::GameInvite& inviteData)
: SysProfile::GameInvite(inviteData)
{
	m_Active = true;
}
	
#ifdef CFG_ALLOW_OPTIMIZE_OFF
#pragma optimize( "", on )
#endif

