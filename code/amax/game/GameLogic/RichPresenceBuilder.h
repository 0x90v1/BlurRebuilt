#ifndef RICH_PRESENCE_BUILDER_H
#define RICH_PRESENCE_BUILDER_H

#ifdef _MSC_VER
#pragma once
#endif

#include <network\Presence.h>

namespace Game // seems like the best place for it, as it needs to read all the game-side stuff most of the time
{

class C_RichPresenceBuilder
{
public:
	static C_RichPresenceBuilder&				Instance() { return s_Instance; }

	bool										BuildRichPresence(const U32 userIndex, RacerNet::eRichPresence& richPresence);

#ifndef PLATFORM_XENON
	void										SetJoinSessionInfo(NetPlay::RichPresence& out, const RacerNet::eRichPresence state, bool allowJoinInProgress);
#endif

private:
	C_RichPresenceBuilder() {} ;
	~C_RichPresenceBuilder() {};

	static C_RichPresenceBuilder				s_Instance;

	void										GetRankAndLegend(S32& rank, S32& legendLevel);
#ifdef PLATFORM_XENON
	//	Sets the top line of RP to say "Xbox LIVE:" with "Rank <1-50>" or "Legend <I-X>"
	void										SetOnlinePresenceTop(const U32 userIndex);
#endif // PLATFORM_XENON

};

}	// namespace Game


#endif // RICH_PRESENCE_BUILDER_H