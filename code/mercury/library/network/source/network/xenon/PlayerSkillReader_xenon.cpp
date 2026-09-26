#include <StdAfx.h>
#include <Network/Network.h>
#include <Network/PlayerSkillReader.h>
#include <Network/PlayerSkill.h>
#include <Network/ScoreboardReaderByUsers.h>
#include <Network/ScoreboardStructures.h>

namespace NetPlay
{
	C_PlayerSkillReader::C_PlayerSkillReader() :	m_Initialised(false), m_Reading(false), m_MaxUsersToQuery(0), 
													m_ResultsValid(false), m_GameModeId(0), m_pReader(NULL)
	{
	}

	C_PlayerSkillReader::~C_PlayerSkillReader()
	{
		CleanUp();
	}

	bool C_PlayerSkillReader::Initialise(const U32 maxUsersToQuery)
	{
		if (m_Initialised)
			return true;

		if (0 == maxUsersToQuery)
		{
			LOGERROR(g_Log, "C_PlayerSkillReader::Initialise() - invalid function parameters.");
			return false;
		}

		m_MaxUsersToQuery = maxUsersToQuery;
		m_pReader = new Scoreboard::C_ReaderByUsers;
		if (m_pReader && m_pReader->Initialise(m_MaxUsersToQuery))
			m_Initialised = true;
		else
			CleanUp();

		return m_Initialised;
	}

	bool C_PlayerSkillReader::Start(const SysProfile::UserIdentifier* pUsers, const U32 numUsers, const U32 gameMode)
	{
		if (!m_Initialised)
			return false;

		if (IsReading())
		{
			LOGWARN(g_Log, "C_PlayerSkillReader::Start() - can't start new read until prevoius read request finishes.");
			return false;
		}

		if (NULL == pUsers || 0 == numUsers)
		{
			LOGERROR(g_Log, "C_PlayerSkillReader::Start() - invalid parameters passed");
			return false;
		}

		if (numUsers > m_MaxUsersToQuery)
		{
			LOGWARN(g_Log, "C_PlayerSkillReader::Start() - too many users passed, need to up the value passed to Initialise().");
			return false;
		}

		// cleanup any previous data.
		StopAndCleanup();

		// set users
		if (!m_pReader->SetUsers(pUsers, numUsers))
		{
			StopAndCleanup();
			return false;
		}
		
		// build columns that will get true skill data
		Scoreboard::ColumnIdentifierList columnList;
		Scoreboard::ColumnTypeList columnTypes;

		columnList.PushBack(X_STATS_COLUMN_SKILL_SKILL);
		columnList.PushBack(X_STATS_COLUMN_SKILL_MU);
		columnList.PushBack(X_STATS_COLUMN_SKILL_SIGMA);

		columnTypes.PushBack(Scoreboard::Int64);	// skill	
		columnTypes.PushBack(Scoreboard::Float64);	// mu
		columnTypes.PushBack(Scoreboard::Float64);	// sigma

		m_GameModeId = gameMode; 

		// add scoreboard.
		if (!m_pReader->AddScoreboard(m_GameModeId, columnList, columnTypes, true))
		{
			StopAndCleanup();
			return false;
		}

		// start read
		if(m_pReader->Start(0))
			m_Reading = true;
		else
			m_pReader->StopAndCleanup();

		return m_Reading;
	}

	bool C_PlayerSkillReader::Continue(bool& errorOccurred)
	{
		errorOccurred = true;
		if (!m_Initialised)
			return true;

		if (!m_Reading)
			return true;

		if (!m_pReader->Continue(errorOccurred))
			return false; // still active..

		// valid results ?
		m_ResultsValid = !errorOccurred;
		m_Reading = false;

		return true;
	}

	bool C_PlayerSkillReader::GetSkillFromResults(const SysProfile::UserIdentifier& user, PlayerSkill& skill)
	{
		PlayerSkill defaultSkill;
		skill = defaultSkill;
		if (!m_Initialised)
			return false;

		if (IsReading())
		{
			LOGWARN(g_Log, "C_PlayerSkillReader::GetSkillFromResults() - can't get results while read still active.");
			return false;
		}

		if (!m_ResultsValid)
		{
			LOGWARN(g_Log, "C_PlayerSkillReader::GetSkillFromResults() - can't get results as there is no server data enumerated. assuming default skill.");
			return true;
		}

		Scoreboard::RowData row;
		if (!m_pReader->ExtractRowForUser(user, m_GameModeId, row))
		{
			LOGWARN(g_Log, "C_PlayerSkillReader::GetSkillFromResults() - no server data returned for player, assuming default skill.");
			return true; // no server data so assume default skill (as if the player has never played this ranked mode before).
		}
		
		// validate we have all 3 skill columns we asked for
		if (row.m_Columns.Size() != 3)
		{
			LOGWARN(g_Log, "C_PlayerSkillReader::GetSkillFromResults() - didn't get all 3 skill parameters back from server.. assuming default skill.");
			return true;
		}

		// assign correct skill values
		for (S32 i=0; i<row.m_Columns.Size(); i++)
		{
			switch (row.m_Columns[i].m_FieldId)
			{
			case X_STATS_COLUMN_SKILL_SKILL:
				assert(row.m_Columns[i].m_DataType == Scoreboard::Int64);
				skill.m_Skill = row.m_Columns[i].m_Data.m_S64;
				break;

			case X_STATS_COLUMN_SKILL_MU:
				assert(row.m_Columns[i].m_DataType == Scoreboard::Float64);
				skill.m_Mu = row.m_Columns[i].m_Data.m_F64;
				break;

			case X_STATS_COLUMN_SKILL_SIGMA:
				assert(row.m_Columns[i].m_DataType == Scoreboard::Float64);
				skill.m_Sigma = row.m_Columns[i].m_Data.m_F64;
				break;

			default:
				LOGERROR(g_Log, "C_PlayerSkillReader::GetSkillFromResults() - unrequested column returned in query results.");
				break;
			}
		}
		
		return true;
	}

	void C_PlayerSkillReader::StopAndCleanup(void)
	{
		if (!m_Initialised)
			return;

		m_pReader->StopAndCleanup();
		m_Reading = false;
		m_ResultsValid = false;
		m_GameModeId = 0;
	}

	void C_PlayerSkillReader::CleanUp(void)
	{
		if (m_pReader)
		{
			m_pReader->StopAndCleanup();
			delete m_pReader;
		}

		m_GameModeId = 0;
		m_ResultsValid = false;
		m_Reading = false;
		m_pReader = NULL;
		m_MaxUsersToQuery = 0;
		m_Initialised = false;
	}
}