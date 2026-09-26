#include <StdAfx.h>
#include <Network/Network.h>
#include <Network/PlayerSkillWriter.h>
#include <Network/PlayerSkill.h>
#include <Network/Session.h>
#include "AsyncTask_xenon.h"

namespace NetPlay
{
	C_PlayerSkillWriter::C_PlayerSkillWriter() :	m_Initialised(false), m_Writing(false), m_MaxUsersToWrite(0), m_GameModeId(0),
													m_CurrentIndex(0), m_pAsyncTask(NULL), m_pWriteCache(NULL), m_TotalWriteCount(0),
													m_StartNewWrite(true), m_SessionHandle(NULL)
	{
	}

	C_PlayerSkillWriter::~C_PlayerSkillWriter()
	{
		CleanUp();
	}

	bool C_PlayerSkillWriter::Initialise(const U32 maxUsersToWrite)
	{
		if (m_Initialised)
			return true;
		
		if (0 == maxUsersToWrite)
			return false; // not valid.

		m_MaxUsersToWrite = maxUsersToWrite;
		m_pAsyncTask = new C_AsyncTask;
		m_pWriteCache = new PlayerSkillSubmission[m_MaxUsersToWrite];

		if (m_pAsyncTask && m_pWriteCache)
			m_Initialised = true;
		else
			CleanUp();

		return m_Initialised;
	}

	bool C_PlayerSkillWriter::Start(const C_Session& session, const PlayerSkillSubmission* pSkillData, const U32 numUsers, const U32 gameMode)
	{
		if (!m_Initialised)
			return false;

		if (IsWriting())
		{
			LOGWARN(g_Log, "C_PlayerSkillWriter::Start() - can't start new write until prevoius read request finishes.");
			return false;
		}
		
		// cleanup any previous data. if applicable.
		StopAndCleanup();

		if (NULL == pSkillData || 0 == numUsers)
		{
			LOGERROR(g_Log, "C_PlayerSkillWriter::Start() - invalid parameters passed.");
			return false;
		}

		if (numUsers > m_MaxUsersToWrite)
		{
			LOGERROR(g_Log, "C_PlayerSkillWriter::Start() - trying to write too many players, increase the value passed into Initialise() to allocate more storage.");
			return false;
		}

		if (session.SessionState() != NetPlay::C_Session::eStatePlaying)
		{
			StopAndCleanup();
			LOGERROR(g_Log, "C_PlayerSkillWriter::Start() - can't write skill data if the session has already ended. !!");
			return false;
		}

		if (session.SessionState() == NetPlay::C_Session::eStateError)
		{
			StopAndCleanup();
			LOGERROR(g_Log, "C_PlayerSkillWriter::Start() - Session is in an Error state not writing skill data. !!");
			return false;
		}

		// validate passed in data,m can't write skill for guest players.
		for (U32 i=0; i<numUsers; i++)
		{
			if (pSkillData[i].m_User.m_IsGuest)
			{
				LOGERROR(g_Log, "C_PlayerSkillWriter::Start() - We can't write skill data for guest players!");
				return false;
			}
		}

		// copy session handle as we need it to write later.
		m_SessionHandle = session.m_SessionHandle;

		// cache data to be written one player at a time..
		m_CurrentIndex = 0;
		m_TotalWriteCount = numUsers;
		for (U32 i=0; i<m_TotalWriteCount; i++)
			m_pWriteCache[i] = pSkillData[i];

		m_StartNewWrite = true;
		m_Writing = true;
		return true;
	}

	bool C_PlayerSkillWriter::Continue(bool& errorOccurred)
	{
		errorOccurred = true;
		if (!m_Initialised)
			return true;

		if (!IsWriting())
			return true;

		if (m_StartNewWrite)
		{
			m_StartNewWrite = false;

			// kick off the next write request.
			const PlayerSkillSubmission& currentPlayer = m_pWriteCache[m_CurrentIndex];

			XUSER_PROPERTY prop;
			m_Columns.Clear();

			// player rating
			prop.dwPropertyId	=	X_PROPERTY_RELATIVE_SCORE;
			prop.value.type		=	XUSER_DATA_TYPE_INT32;
			prop.value.nData	=	currentPlayer.m_Rating;
			m_Columns.PushBack(prop);

			// player team
			prop.dwPropertyId	=	X_PROPERTY_SESSION_TEAM;
			prop.value.type		=	XUSER_DATA_TYPE_INT32;
			prop.value.nData	=	currentPlayer.m_Team;
			m_Columns.PushBack(prop);

			// setup view to write to.
			m_View.dwNumProperties	=	m_Columns.Size();
			m_View.dwViewId			=	X_STATS_VIEW_SKILL;
			m_View.pProperties		=	&m_Columns[0];

			// start write..
			const DWORD res = XSessionWriteStats(m_SessionHandle, currentPlayer.m_User.m_Uid, 1, &m_View, &(*m_pAsyncTask));
			if (ERROR_IO_PENDING != res)
			{
				LOGWARN(g_Log, "C_PlayerSkillWriter::Continue() - unable to start writing player skill data.");
				StopAndCleanup();
			}
			else
				return false; // write started ok.
		}
		else
		{
			// wait for current write request to finish.
			if (!m_pAsyncTask->Finished())
				return false; 

			if (m_pAsyncTask->ErrorCode() != ERROR_SUCCESS)
			{
				LOGWARN(g_Log, "C_PlayerSkillWriter::Continue() - unable to fully write player skill data.");
				StopAndCleanup();
			}
			else
			{
				m_pAsyncTask->Close();
				m_CurrentIndex += 1;

				if (m_CurrentIndex >= m_TotalWriteCount)
				{
					// write finished.
					StopAndCleanup();
					errorOccurred = false;
				}
				else
				{
					// write next player in the cache..
					m_StartNewWrite = true;
					return false;
				}
			}
		}

		return true;
	}

	void C_PlayerSkillWriter::StopAndCleanup(void)
	{
		if (!m_Initialised)
			return;

		m_pAsyncTask->Close();

		m_StartNewWrite = true;
		m_TotalWriteCount = 0;
		m_Writing = false;
		m_GameModeId = 0;
		m_CurrentIndex = 0;
		m_SessionHandle = NULL;
	}

	void C_PlayerSkillWriter::CleanUp(void)
	{
		StopAndCleanup();
		m_MaxUsersToWrite = 0;
		m_Initialised = 0;
	}
}