#include <stdafx.h>
#include <Core/core.h>
#include "../Misc/NetDebug.h"
#include "NetScoreboards.h"

#if defined ( PLATFORM_XENON )
#include "Xlast/Amax.spa.h"
#endif

#if defined ( PLATFORM_XENON )
	U16 m_sLeaderboardEnums[] = { STATS_VIEW_MP_RACEWINS, STATS_VIEW_MP_POWERUPS, STATS_VIEW_MP_TOTALFANS, STATS_VIEW_MP_LEGENDS, STATS_VIEW_MP_LOBBY, STATS_VIEW_SP_1_A, STATS_VIEW_SP_2_A,	STATS_VIEW_SP_3_A, STATS_VIEW_SP_4_A, STATS_VIEW_SP_5_A, STATS_VIEW_SP_6_A,	STATS_VIEW_SP_7_A, STATS_VIEW_SP_8_A,
	STATS_VIEW_SP_9_A,	STATS_VIEW_SP_10_A,	STATS_VIEW_SP_11_A,	STATS_VIEW_SP_12_A,	STATS_VIEW_SP_13_A,	STATS_VIEW_SP_14_A,	STATS_VIEW_SP_15_A,	STATS_VIEW_SP_16_A,	STATS_VIEW_SP_17_A,	STATS_VIEW_SP_18_A,	STATS_VIEW_SP_19_A,	STATS_VIEW_SP_20_A, STATS_VIEW_SP_21_A,	STATS_VIEW_SP_22_A,	STATS_VIEW_SP_23_A,	STATS_VIEW_SP_24_A,
	STATS_VIEW_SP_25_A,	STATS_VIEW_SP_26_A, STATS_VIEW_SP_27_A,	STATS_VIEW_SP_28_A,	STATS_VIEW_SP_29_A,	STATS_VIEW_SP_30_A,	STATS_VIEW_SP_31_A,	STATS_VIEW_SP_32_A,	STATS_VIEW_SP_33_A,	STATS_VIEW_SP_34_A, STATS_VIEW_SP_35_A,	STATS_VIEW_SP_36_A,	STATS_VIEW_SP_37_A,	STATS_VIEW_SP_38_A,	STATS_VIEW_SP_39_A,	STATS_VIEW_SP_40_A,	
	STATS_VIEW_SP_41_A,	STATS_VIEW_SP_42_A,	STATS_VIEW_SP_43_A,	STATS_VIEW_SP_44_A,	STATS_VIEW_SP_45_A,	STATS_VIEW_SP_46_A,	STATS_VIEW_SP_47_A,	STATS_VIEW_SP_48_A,	STATS_VIEW_SP_49_A,	STATS_VIEW_SP_50_A,	STATS_VIEW_SP_51_A,	STATS_VIEW_SP_52_A,	STATS_VIEW_SP_53_A,	STATS_VIEW_SP_54_A,	STATS_VIEW_SP_55_A,	STATS_VIEW_SP_56_A,	
	STATS_VIEW_SP_57_A,	STATS_VIEW_SP_58_A,	STATS_VIEW_SP_59_A,	STATS_VIEW_SP_60_A,	STATS_VIEW_SP_61_A,	STATS_VIEW_SP_62_A,	STATS_VIEW_SP_63_A,	STATS_VIEW_SP_64_A, STATS_VIEW_SP_65_A,	STATS_VIEW_SP_66_A,	STATS_VIEW_SP_67_A,	STATS_VIEW_SP_68_A,	STATS_VIEW_SP_69_A,	STATS_VIEW_SP_70_A,	STATS_VIEW_SP_71_A,	STATS_VIEW_SP_72_A,
	STATS_VIEW_SP_73_A,	STATS_VIEW_SP_74_A,	STATS_VIEW_SP_75_A,	STATS_VIEW_SP_76_A,	STATS_VIEW_SP_77_A,	STATS_VIEW_SP_78_A,	STATS_VIEW_SP_79_A,	STATS_VIEW_SP_80_A, STATS_VIEW_SP_81_A,	STATS_VIEW_SP_82_A,	STATS_VIEW_SP_83_A,	STATS_VIEW_SP_84_A,	STATS_VIEW_SP_85_A,	STATS_VIEW_SP_86_A,	STATS_VIEW_SP_87_A,	STATS_VIEW_SP_88_A,	
	STATS_VIEW_SP_89_A,	STATS_VIEW_SP_90_A };
#endif
	

#if defined ( RACER_NET_OPTIMIZE_OFF )
	#pragma optimize( "", off )
#endif 

namespace RacerNet
{
	U32 ScoreboardId(const eScoreboards sb)
	{
#if defined ( PLATFORM_XENON )
		if(sb < eMaxScoreboards)
		{
			return m_sLeaderboardEnums[sb];
		}
#else
		return (U32)( sb + 1 );

#endif
		BIZ_ASSERT_FAIL("Requesting invalid scoreboard id");
		return 0;
	}

	void MpBuildScoreboardColumnsToWrite(const eScoreboards sb, const MpScoreboardDataForEvent& data, ScoreboardColumnWriteList& out)
	{
		out.Clear(); 
		Scoreboard::ColumnData temp;

#if defined ( PLATFORM_XENON )

		temp.SetS64(PROPERTY_LB_GAMERANK, data.m_GameRank);
		out.PushBack(temp);

		temp.SetS64(PROPERTY_LB_TOTALFANS, data.m_TotalFans);
		out.PushBack(temp);

		temp.SetS64(PROPERTY_LB_TOTALTIMEPLAYED, data.m_TotalTimePlayed);
		out.PushBack(temp);

		temp.SetS64(PROPERTY_LB_RACEPOINTS, data.m_RacePoints);
		out.PushBack(temp);

		temp.SetS64(PROPERTY_LB_TOPTHREE, data.m_TopThree);
		out.PushBack(temp);

		temp.SetS64(PROPERTY_LB_TOTALRACES, data.m_TotalRaces);
		out.PushBack(temp);

		temp.SetS64(PROPERTY_LB_RACEWINS, data.m_RaceWins);
		out.PushBack(temp);

		temp.SetS64(PROPERTY_LB_POWERUPHITS, data.m_PowerUpHits);
		out.PushBack(temp);

		temp.SetS64(PROPERTY_LB_POWERUPSFIRED, data.m_PowerUpsFired);
		out.PushBack(temp);

		temp.SetS64(PROPERTY_LB_CARSWRECKED, data.m_CarsWrecked);
		out.PushBack(temp);

		temp.SetS64(PROPERTY_LB_LEGENDLEVEL, data.m_LegendLevel);
		out.PushBack(temp);
		
		temp.SetS64(PROPERTY_LB_TIMETOACHIEVELEGEND, data.m_TimeToAchieveLegend);
		out.PushBack(temp);

		switch (sb)
		{
		case eMpScoreboardTotalWins:
				temp.SetS64(PROPERTY_LB_RATING, data.m_RacePointsRating);
			out.PushBack(temp);
			break;
		case eMpScoreboardPerkRatio:
			temp.SetS64(PROPERTY_LB_RATING, data.m_PowerUpHitsRating);
			out.PushBack(temp);
			break;
		case eMpScoreboardTotalFans:
			temp.SetS64(PROPERTY_LB_RATING, data.m_TotalFansRating);
			out.PushBack(temp);
			break;
		case eMpScoreboardLegends:
			temp.SetS64(PROPERTY_LB_RATING, data.m_LegendLevelRating);
			out.PushBack(temp);
			break;
		case eMpScoreboardLobby:
			temp.SetS64(PROPERTY_LB_RATING, data.m_RaceWinsRating);
			out.PushBack(temp);
			break;
		}
#else
		temp.SetS64(0, data.m_GameRank);
		out.PushBack(temp);

		temp.SetS64(1, data.m_TotalFans);
		out.PushBack(temp);

		temp.SetS64(2, data.m_TotalTimePlayed);
		out.PushBack(temp);

		temp.SetS64(3, data.m_RacePoints);
		out.PushBack(temp);

		temp.SetS64(4, data.m_TopThree);
		out.PushBack(temp);

		temp.SetS64(5, data.m_TotalRaces);
		out.PushBack(temp);

		temp.SetS64(6, data.m_RaceWins);
		out.PushBack(temp);

		temp.SetS64(7, data.m_PowerUpHits);
		out.PushBack(temp);

		temp.SetS64(8, data.m_PowerUpsFired);
		out.PushBack(temp);

		temp.SetS64(9, data.m_CarsWrecked);
		out.PushBack(temp);

		temp.SetS64(10, data.m_LegendLevel);
		out.PushBack(temp);
		
		temp.SetS64(11, data.m_TimeToAchieveLegend);
		out.PushBack(temp);
#endif
	}

	void SpBuildScoreboardColumnsToWrite(const eSPScoreboardTypes sb, const SpScoreboardDataForEvent& data, ScoreboardColumnWriteList& out)
	{
		out.Clear(); 
		Scoreboard::ColumnData temp;

#if defined ( PLATFORM_XENON )

		temp.SetS64(PROPERTY_LB_TOTALFANS, data.m_TotalFans);
		out.PushBack(temp);

		temp.SetS64(PROPERTY_LB_LAPTIME, data.m_LapTime);
		out.PushBack(temp);

		temp.SetS64(PROPERTY_LB_COMPTIME, data.m_CompTime);
		out.PushBack(temp);

		temp.SetS64(PROPERTY_LB_CARSWRECKED, data.m_Wrecks);
		out.PushBack(temp);

		temp.SetS64(PROPERTY_LB_VEHICLE_ID, data.m_VehicleId);
		out.PushBack(temp);

		temp.SetS64(PROPERTY_LB_DIFFICULTY_LEVEL, data.m_Difficulty);
		out.PushBack(temp);

		switch (sb)
		{
		case eSPRacesScoreboard:
			temp.SetS64(PROPERTY_LB_RATING, data.m_RacesRating);
			out.PushBack(temp);
			break;
		case eSPFanDemandsScoreboard:
			temp.SetS64(PROPERTY_LB_RATING, data.m_FanDemandsRating);
			out.PushBack(temp);
			break;
		case eSPCheckpointsScoreboard:
			temp.SetS64(PROPERTY_LB_RATING, data.m_CheckpointsRating);
			out.PushBack(temp);
			break;
		case eSPDestructionScoreboard:
			temp.SetS64(PROPERTY_LB_RATING, data.m_DestructionRating);
			out.PushBack(temp);
			break;
		case eSP1on1Scoreboard:
			temp.SetS64(PROPERTY_LB_RATING, data.m_1on1Rating);
			out.PushBack(temp);
			break;
		}
#else

		temp.SetS64(0, data.m_TotalFans);
		out.PushBack(temp);

		temp.SetS64(1, data.m_LapTime);
		out.PushBack(temp);

		temp.SetS64(2, data.m_CompTime);
		out.PushBack(temp);

		temp.SetS64(3, data.m_Wrecks);
		out.PushBack(temp);

		temp.SetS64(4, data.m_VehicleId);
		out.PushBack(temp);

		temp.SetS64(5, data.m_Difficulty);
		out.PushBack(temp);
#endif
	}

} // end namespace RacerNet


#if defined ( RACER_NET_OPTIMIZE_OFF )
	#pragma optimize( "", on )
#endif
