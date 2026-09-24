#include "stdafx.h"

#include "RichPresenceBuilder.h"
#include <Ui\UiHelpers.h>
#include <GameLogic\Layers\RaceBookLayer.h>
#include <GameLogic\Layers\RaceLogicLayer.h>
#include <GameLogic\GameModes\GameModeBase.h>
#include <GameLogic\GameModes\GameModeDefines.h>
#include <GameLogic\RaceModes\RaceModeDefines.h>

#include <GameLogic\Profile\GameProfileManager.h>
#include <GameLogic\SinglePlayer\SinglePlayer.h>
#include <GameLogic\FriendsChallenges\FriendChallenges.h>

#include <Network\Core\NetServices.h>
#include <Network\Core\NetRace.h>
#include <Network\Core\NetParty.h>

#include "Xlast/Amax.spa.h"

namespace Game
{
	//	static instance
	C_RichPresenceBuilder C_RichPresenceBuilder::s_Instance;

//	Returns true if we need to update the presence based on the enum we've passed in
//	On PS3/PC this enum may be changed here to reference a non-Xenon rich presence index
	bool C_RichPresenceBuilder::BuildRichPresence( const U32 userIndex, RacerNet::eRichPresence& richPresence )
	{
	switch(richPresence)
	{
#ifdef PLATFORM_XENON
		case RacerNet::eRichPresence_SignedIn:
			{
				//	SET_PRESENCE( "Just getting started" )
				XUserSetContext(userIndex, CONTEXT_RP_TOP, CONTEXT_RP_TOP_SIGNEDIN);
				XUserSetContext(userIndex, X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_MAINPRESENCE_ONELINE);
				break;
			}
		case RacerNet::eRichPresence_Idle:
			{
				//	SET_PRESENCE( "Idle" )
				XUserSetContext(userIndex, CONTEXT_RP_TOP, CONTEXT_RP_TOP_IDLE);
				XUserSetContext(userIndex, X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_MAINPRESENCE_ONELINE);
				break;
			}	
		case RacerNet::eRichPresence_Stickerbook:
			{
				//	SET_PRESENCE( "Checking out Stickers" )
				XUserSetContext(userIndex, CONTEXT_RP_TOP, CONTEXT_RP_TOP_STICKERBOOK);
				XUserSetContext(userIndex, X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_MAINPRESENCE_ONELINE);
				break;
			}
		case RacerNet::eRichPresence_Leaderboards:
			{
				//	If(single player or NOT in a lobby)
				//	SET_PRESENCE( "In the Leaderboards" )
				if(UI::GetRaceBookLayer()->GetGameModePtr()->IsOnline())
				{
					// TODO_RP
					SetOnlinePresenceTop(userIndex);
					XUserSetContext(userIndex, CONTEXT_RP_BOTTOM, CONTEXT_RP_BOTTOM_LEADERBOARDS);
				}
				else
				{
					XUserSetContext(userIndex, CONTEXT_RP_TOP, CONTEXT_RP_TOP_LEADERBOARDS);
					XUserSetContext(userIndex, X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_MAINPRESENCE_ONELINE);
				}
				break;
			}
		case RacerNet::eRichPresence_Help:
			{
				//	SET_PRESENCE( "In the Help menu" )
				XUserSetContext(userIndex, CONTEXT_RP_TOP, CONTEXT_RP_TOP_HELP);
				XUserSetContext(userIndex, X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_MAINPRESENCE_ONELINE);
				break;
			}
		case RacerNet::eRichPresence_Showroom:
			{
				//	If(single player or NOT in a lobby) - should only get this enum in those cases
				//	SET_PRESENCE( "In the Showroom" )
				if(UI::GetRaceBookLayer()->GetGameModePtr()->IsOnline())
				{
					// TODO_RP
					SetOnlinePresenceTop(userIndex);
					XUserSetContext(userIndex, CONTEXT_RP_BOTTOM, CONTEXT_RP_BOTTOM_SHOWROOM);
				}
				else
				{
					XUserSetContext(userIndex, CONTEXT_RP_TOP, CONTEXT_RP_TOP_SHOWROOM);
					XUserSetContext(userIndex, X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_MAINPRESENCE_ONELINE);
				}
				break;
			}
#endif // PLATFORM_XENON
		case RacerNet::eRichPresence_InRace:
			{
				E_RaceMode currentRaceMode = UI::GetRaceBookLayer()->GetRaceMode();
				//	We're only interested in setting the presence here if we're either playing single player or online.
				if(!UI::GetRaceBookLayer()->GetGameModePtr()->IsMultiplayer())
				{
					//	Single player OR Friend demands
					switch(currentRaceMode)
					{
					case E_RaceMode_StreetRace:	// Fall-through
					case E_RaceMode_Destruction:// Fall-through
					case E_RaceMode_Checkpoint:	// Fall-through
					case E_RaceMode_BossBattle:	// Fall-through
						{
#ifdef PLATFORM_XENON
							//	Find SP tier
							S32 currentSpEventId = C_GameProfileManager::Instance().GetPrimaryProfile()->mCurrentSPEventId;
							SP_SaveGame* pSave = SP_GetCurrentSaveGame();
							if(pSave)
							{
								SP_Event* currentSpEvent = SP_GetEventById(pSave, currentSpEventId);
								if(currentSpEvent)
								{
									S8 tierIndex = currentSpEvent->tier->index;

									XUserSetContext(userIndex, CONTEXT_RP_TOP, CONTEXT_RP_TOP_TIER0 + tierIndex);	//	Gives us the presence string for the current Tier

									if(currentRaceMode == E_RaceMode_StreetRace)
									{
										XUserSetContext(userIndex, CONTEXT_RP_BOTTOM, CONTEXT_RP_BOTTOM_SPRACING);
									}
									else if(currentRaceMode == E_RaceMode_Destruction)
									{
										XUserSetContext(userIndex, CONTEXT_RP_BOTTOM, CONTEXT_RP_BOTTOM_SPDESTRUCTION);
									}
									else if(currentRaceMode == E_RaceMode_Checkpoint)
									{
										XUserSetContext(userIndex, CONTEXT_RP_BOTTOM, CONTEXT_RP_BOTTOM_SPCHECKPOINT);
									}
									else if(currentRaceMode == E_RaceMode_BossBattle)
									{
										//	Tier index tells us which boss the player is fighting
										XUserSetContext(userIndex, CONTEXT_RP_BOTTOM, CONTEXT_RP_BOTTOM_SPBOSS0 + tierIndex);
									}

									XUserSetContext(userIndex, X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_MAINPRESENCE);
								}
							}
#else
							if(currentRaceMode == E_RaceMode_StreetRace)
								richPresence = RacerNet::eRichPresencePS3_SpRace;
							else if(currentRaceMode == E_RaceMode_Destruction)
								richPresence = RacerNet::eRichPresencePS3_SpDestruction;
							else if(currentRaceMode == E_RaceMode_Checkpoint)
								richPresence = RacerNet::eRichPresencePS3_SpCheckpoint;
							else if(currentRaceMode == E_RaceMode_BossBattle)
								richPresence = RacerNet::eRichPresencePS3_SpBoss;
#endif
							break;
						}
					case E_RaceMode_FriendDemand_StreetRace:	// Fall-through
					case E_RaceMode_FriendDemand_Destruction:	// Fall-through
					case E_RaceMode_FriendDemand_Checkpoint:	// Fall-through
					case E_RaceMode_FriendDemand_FanDemand:	// Fall-through
					case E_RaceMode_FriendDemand_BossBattle:	// Fall-through
						{
							//	Find out if it's a new challenge or re-challenge
							bool reChallenge = C_FriendChallenges::Instance().GetAttemptingFriendDemand()->mReChallenge;
#ifdef PLATFORM_XENON
							XUserSetContext(userIndex, CONTEXT_RP_TOP, CONTEXT_RP_TOP_FRIENDCHALLENGES);
							if(reChallenge)
							{
								XUserSetContext(userIndex, CONTEXT_RP_BOTTOM, CONTEXT_RP_BOTTOM_RECHALLENGE);
							}
							else
							{
								XUserSetContext(userIndex, CONTEXT_RP_BOTTOM, CONTEXT_RP_BOTTOM_FRIENDCHALLENGE);
							}
							XUserSetContext(userIndex, X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_MAINPRESENCE);

#else
							richPresence = (reChallenge) ? RacerNet::eRichPresencePS3_RC_ReChallenge : RacerNet::eRichPresencePS3_FC_Challenge;
#endif
							break;
						}
					}
				}
				else if(UI::GetRaceBookLayer()->GetGameModePtr()->IsOnline())
				{
					if(UI::GetRaceBookLayer()->GetGameModePtr()->IsRanked())
					{
						//	Ranked Online game
#ifdef PLATFORM_XENON
						SetOnlinePresenceTop(userIndex);

						if(currentRaceMode == E_RaceMode_Multiplayer_Racing)
							XUserSetContext(userIndex, CONTEXT_RP_BOTTOM, CONTEXT_RP_BOTTOM_ONLINERACING);
						else if(currentRaceMode == E_RaceMode_Multiplayer_Destruction)
							XUserSetContext(userIndex, CONTEXT_RP_BOTTOM, CONTEXT_RP_BOTTOM_ONLINEMOTORMASH);
#else
						if(currentRaceMode == E_RaceMode_Multiplayer_Racing)
							richPresence = RacerNet::eRichPresencePS3_OnlineRacing;
						else if(currentRaceMode == E_RaceMode_Multiplayer_Destruction)
							richPresence = RacerNet::eRichPresencePS3_OnlineMM;
#endif
					}
					else
					{
#ifdef PLATFORM_XENON
						SetOnlinePresenceTop(userIndex);
						XUserSetContext(userIndex, CONTEXT_RP_BOTTOM, CONTEXT_RP_BOTTOM_PRIVATEGAME);
#endif
						return false;	//	Don't need to set the Private Game presence here, we'll have set that in the lobby
					}
				}
				else
					return false;	//	We'll set Split Screen and LAN presence when entering those modes, not on the HUD
				break;
			}
#ifdef PLATFORM_XENON
		case RacerNet::eRichPresence_SpMain:
			{
				//	SET_PRESENCE( "Single Player")
				XUserSetContext(userIndex, CONTEXT_RP_TOP, CONTEXT_RP_TOP_SINGLEPLAYER);
				XUserSetContext(userIndex, X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_MAINPRESENCE_ONELINE);
				break;
			}
		case RacerNet::eRichPresence_Photos:
			{
				//	SET_PRESENCE( "Checking out Photos" )
				XUserSetContext(userIndex, CONTEXT_RP_TOP, CONTEXT_RP_TOP_PHOTOS);
				XUserSetContext(userIndex, X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_MAINPRESENCE_ONELINE);
				break;
			}
		case RacerNet::eRichPresence_FriendChallenges:
			{
				//	SET_PRESENCE( "Friend Challenges" )
				XUserSetContext(userIndex, CONTEXT_RP_TOP, CONTEXT_RP_TOP_FRIENDCHALLENGES);
				XUserSetContext(userIndex, X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_MAINPRESENCE_ONELINE);
				break;
			}
		case RacerNet::eRichPresence_SpCareer:
			{
				XUserSetContext(userIndex, CONTEXT_RP_TOP, CONTEXT_RP_TOP_CAREER);
				XUserSetContext(userIndex, X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_MAINPRESENCE_ONELINE);
				break;
			}
		case RacerNet::eRichPresence_Sp_Choosing:
			{
				U32 spTier = UI::GetRaceBookLayer()->GetCurrentSpTier();

				XUserSetContext(userIndex, CONTEXT_RP_TOP, CONTEXT_RP_TOP_TIER0 + spTier);	//	Gives us the presence string for the current Tier
				XUserSetContext(userIndex, CONTEXT_RP_BOTTOM, CONTEXT_RP_BOTTOM_SPCHOOSINGEVENT);
				XUserSetContext(userIndex, X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_MAINPRESENCE);

				break;
			}
		case RacerNet::eRichPresence_MpMain:
			{
				XUserSetContext(userIndex, CONTEXT_RP_TOP, CONTEXT_RP_TOP_MULTIPLAYER);
				XUserSetContext(userIndex, X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_MAINPRESENCE_ONELINE);
				break;
			}
		case RacerNet::eRichPresence_SplitScreen:
			{
				XUserSetContext(userIndex, CONTEXT_RP_TOP, CONTEXT_RP_TOP_SPLITSCREEN);
				XUserSetContext(userIndex, X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_MAINPRESENCE_ONELINE);
				break;
			}
		case RacerNet::eRichPresence_RaceHistory:
			{
				XUserSetContext(userIndex, CONTEXT_RP_TOP, CONTEXT_RP_TOP_RACEHISTORY);
				XUserSetContext(userIndex, X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_MAINPRESENCE_ONELINE);
				break;
			}
		case RacerNet::eRichPresence_LAN:
			{
				XUserSetContext(userIndex, CONTEXT_RP_TOP, CONTEXT_RP_TOP_SYSTEMLINK);
				XUserSetContext(userIndex, X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_MAINPRESENCE_ONELINE);
				break;
			}
		case RacerNet::eRichPresence_OnlineMain:
			{
				SetOnlinePresenceTop(userIndex);
				XUserSetContext(userIndex, CONTEXT_RP_BOTTOM, CONTEXT_RP_BOTTOM_ONLINEMAIN);
				break;
			}
		case RacerNet::eRichPresence_ModShop:
			{
				SetOnlinePresenceTop(userIndex);
				XUserSetContext(userIndex, CONTEXT_RP_BOTTOM, CONTEXT_RP_BOTTOM_MODSHOP);
				break;
			}
		case RacerNet::eRichPresence_Challenges:
			{
				SetOnlinePresenceTop(userIndex);
				XUserSetContext(userIndex, CONTEXT_RP_BOTTOM, CONTEXT_RP_BOTTOM_CHALLENGES);
				break;
			}
#endif
		case RacerNet::eRichPresence_MpLobby:
			{
				if(UI::GetRaceBookLayer()->GetGameModePtr()->IsOnline())
				{
					bool ranked = UI::GetRaceBookLayer()->GetGameModePtr()->IsRanked();
					//	[TODO_RP] based on the ranked flag, we know if this is a public or private game
#ifdef PLATFORM_XENON
					SetOnlinePresenceTop(userIndex);

					if(ranked)
					{
						XUserSetContext(userIndex, CONTEXT_RP_BOTTOM, CONTEXT_RP_BOTTOM_INLOBBY);
					}
					else
					{
						XUserSetContext(userIndex, CONTEXT_RP_BOTTOM, CONTEXT_RP_BOTTOM_PRIVATEGAME);
					}

#else
					if(!ranked)
						richPresence = RacerNet::eRichPresencePS3_OnlinePrivate;
#endif
				}
				else
				{
					//	System Link/LAN - which we don't need to do anything with as we set this presence when you
					//	choose it from the MP carousel. Bail out if it's not XENON so we don't keep our existing LAN presence.
#ifndef PLATFORM_XENON
					return false;
#endif
				}
				break;
			}
	}

	return true;
}

void C_RichPresenceBuilder::GetRankAndLegend( S32& rank, S32& legendLevel )
{
	C_GameProfile* pProf = C_GameProfileManager::Instance().GetPrimaryProfile();
	if(pProf)
	{
		rank = pProf->mMultiplayerProfile.GetCurrentLevel() + 1;	//	Gives us 1-50
		legendLevel = pProf->mMultiplayerProfile.GetLegendLevel() - 1; // Gives us 0-9 (and -1 which means non-Legend)
	}
}

#ifdef PLATFORM_XENON
//	Sets the top line of RP to say "Xbox LIVE:" with "Rank <1-50>" or "Legend <I-X>"
void C_RichPresenceBuilder::SetOnlinePresenceTop( const U32 userIndex )
{
	S32 rank, legendLevel;
	GetRankAndLegend(rank, legendLevel);
	if(legendLevel > -1)
	{
		XUserSetContext(userIndex, X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_ONLINELEGEND);
		XUserSetContext(userIndex, CONTEXT_RP_TOP, CONTEXT_RP_TOP_ONLINEWITHLEGEND);
		XUserSetContext(userIndex, CONTEXT_RP_LEGEND_ROMAN, legendLevel); // Legend level is stored in profile as 1-10
	}
	else
	{
		XUserSetContext(userIndex, X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_ONLINERANK);
		XUserSetContext(userIndex, CONTEXT_RP_TOP, CONTEXT_RP_TOP_ONLINEWITHRANK);
		XUserSetProperty(userIndex, PROPERTY_RP_RANK, sizeof(rank), &rank );	//	Rank is stored in profile as 0-49
	}
}

#endif

#ifndef PLATFORM_XENON
//	PS3 + PC join/invite data needs to be built into the presence structure here dependant on the presence type
void C_RichPresenceBuilder::SetJoinSessionInfo( NetPlay::RichPresence& out, const RacerNet::eRichPresence state, bool allowJoinInProgress )
{
	RacerNet::C_NetServices& ns = RacerNet::C_NetServices::Instance();
	NetPlay::RichPresence pres;
	switch(state)
	{
	case RacerNet::eRichPresence_SignedIn:
	case RacerNet::eRichPresence_Idle:
	case RacerNet::eRichPresence_Stickerbook:		
	case RacerNet::eRichPresence_Leaderboards:		
	case RacerNet::eRichPresence_Help:				
	case RacerNet::eRichPresence_Showroom:		
	case RacerNet::eRichPresence_SpMain:			
	case RacerNet::eRichPresence_Photos:			
	case RacerNet::eRichPresence_FriendChallenges:
	case RacerNet::eRichPresence_SpCareer:			
	case RacerNet::eRichPresence_Sp_Choosing:
	case RacerNet::eRichPresencePS3_SpBoss:
	case RacerNet::eRichPresencePS3_SpRace:
	case RacerNet::eRichPresencePS3_SpDestruction:
	case RacerNet::eRichPresencePS3_SpCheckpoint:
	case RacerNet::eRichPresencePS3_FC_Challenge:
	case RacerNet::eRichPresencePS3_RC_ReChallenge:
	case RacerNet::eRichPresence_SplitScreen:
	case RacerNet::eRichPresence_LAN:
		{
			pres.m_Index = state;
			out = pres;
			break;
		}

	case RacerNet::eRichPresence_MpMain:					
	case RacerNet::eRichPresence_RaceHistory:		
	case RacerNet::eRichPresence_OnlineMain:		
	case RacerNet::eRichPresence_ModShop:				
	case RacerNet::eRichPresence_Challenges:		
	case RacerNet::eRichPresence_MpLobby:
	case RacerNet::eRichPresencePS3_OnlineRacing:
	case RacerNet::eRichPresencePS3_OnlineMM:
	case RacerNet::eRichPresencePS3_OnlinePrivate:
		{
			pres.m_Index = state;

			// check for join in progress on any of our sessions.
			if (allowJoinInProgress)
			{
				if (ns.IsInitialised())
				{
					bdSecurityID sessionId;
					if (ns.Race().JoinInProgressAvailable(sessionId))
					{
						pres.m_JoinInProgressPossible = true;
						pres.m_OnlineSessionId = sessionId;
					}
					else if (ns.Party().JoinInProgressAvailable(sessionId))
					{
						pres.m_JoinInProgressPossible = true;
						pres.m_OnlineSessionId = sessionId;
					}
				}
			}
			out = pres;
			break;
		}
	}
}
#endif

} // namespace Game