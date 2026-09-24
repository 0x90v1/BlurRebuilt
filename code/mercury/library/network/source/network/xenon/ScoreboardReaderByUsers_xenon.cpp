#include <StdAfx.h>
#include <Network/ScoreboardReaderByUsers.h>
#include <Network/ScoreboardStructures.h>
#include <SystemProfile/SystemProfile.h>
#include <core/Maths.h>
#include "AsyncTask_xenon.h"


namespace Scoreboard
{
	// constructor
	C_ReaderByUsers::C_ReaderByUsers() :	m_Initialised(false), m_Reading(false), m_MaxScoreboardsToReadFrom(5), m_MaxUsersToQuery(0), m_NumUsersToQuery(0),
											m_NumScoreboardsToQuery(0), m_pRequiredUsers(NULL), m_pRequriredScoreboards(NULL), m_pAsyncTask(NULL), m_pResultsBuffer(NULL)
	{
	}


	// destructor
	C_ReaderByUsers::~C_ReaderByUsers()
	{
		CleanUp();
	}


	// alocate all dynamic resources.
	bool C_ReaderByUsers::Initialise(const U32 maxUserstoQuery)
	{
		if (m_Initialised)
			return true;

		m_MaxUsersToQuery = (maxUserstoQuery == 0) ? MAX_FRIENDS+1 : maxUserstoQuery;
	
		// storage for users who we are rtrieving scoreboard data for..
		m_pRequiredUsers = reinterpret_cast<void*> (new XUID[m_MaxUsersToQuery]);

		// storage for what scoreboards we will request data from.
		m_pRequriredScoreboards = reinterpret_cast<void*> (new XUSER_STATS_SPEC[m_MaxScoreboardsToReadFrom]);

		//allocate async task handle.
		m_pAsyncTask = reinterpret_cast<void*> (new NetPlay::C_AsyncTask);

		if (NULL != m_pRequiredUsers && NULL != m_pRequriredScoreboards && NULL != m_pAsyncTask)
			m_Initialised = true;
		else
			CleanUp();

		return m_Initialised;
	}


	// set user data before we start reading.
	bool C_ReaderByUsers::SetUsers(const SysProfile::UserIdentifier* pUsers, const U32 numUsers)
	{
		if (!m_Initialised)
			return false;
		
		// make sure we cant modify players while still requesting server data
		assert(m_Reading == false);
		if (m_Reading)
			return false; 

		// check input data
		assert(pUsers);
		assert(numUsers);
		if(NULL == pUsers || 0 == numUsers)
			return false;
			
		// make sure we not passing in to many users.
		if (numUsers > m_MaxUsersToQuery)
		{
			LOGERROR(NetPlay::g_Log, "Trying to read too many users in 1 server call!\n");
			return false;
		}

		// copy data now..
		XUID *x = reinterpret_cast<XUID*> (m_pRequiredUsers);
		m_NumUsersToQuery = numUsers;
		for (U32 i=0; i<m_NumUsersToQuery; i++)
			x[i] = pUsers[i].m_Uid;

		return true;
	}

	
	// add a scoreboard to the list of what needs to be queried.
	bool C_ReaderByUsers::AddScoreboard(const U32 scoreboardId, const ColumnIdentifierList& columns, const ColumnTypeList& columnTypes, const bool flushCache)
	{
		if (!m_Initialised)
			return false;

		assert(m_Reading == false);
		if (m_Reading)
			return false;

		if (flushCache)
			m_NumScoreboardsToQuery = 0;

		// can we actually query anymore scoreboards
		if (m_NumScoreboardsToQuery >= m_MaxScoreboardsToReadFrom)
		{
			LOGERROR(NetPlay::g_Log, "Trying to read from too many scoreboards with 1 server call !\n");
			return false;
		}

		// check for max columns..
		if (columns.Size() > XUSER_STATS_ATTRS_IN_SPEC)
		{
			LOGERROR(NetPlay::g_Log, "Trying to read from too many scoreboard columns !\n");
			return false;
		}

		XUSER_STATS_SPEC* p = reinterpret_cast<XUSER_STATS_SPEC*> (m_pRequriredScoreboards);
		XUSER_STATS_SPEC& sb = p[m_NumScoreboardsToQuery++];

		// copy data
		sb.dwViewId			=	scoreboardId;
		sb.dwNumColumnIds	=	columns.Size();
		for (U32 i=0; i<sb.dwNumColumnIds; i++)
			sb.rgwColumnIds[i] = columns[i];
		return true;
	}


	// stop async task and release results bufer is active.
	void C_ReaderByUsers::StopAndCleanup(void)
	{
		if (!m_Initialised)
			return;

		NetPlay::C_AsyncTask* p = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);
		p->Close();

		if (m_pResultsBuffer)
			delete [] m_pResultsBuffer;
		m_pResultsBuffer = NULL;

		m_Reading = false;
	}


	// try and start requesting data from the scoreboard server
	bool C_ReaderByUsers::Start(const U32 titleId)
	{
		if (!m_Initialised)
			return false;

		// stop any previous read and release results buffer..
		StopAndCleanup();
		
		// no input data setup so cant query scoreboards..
		if (0 == m_NumScoreboardsToQuery || 0 == m_NumUsersToQuery)
			return false; 

		bool ok = false;
		XUID* pXuids = reinterpret_cast<XUID*> (m_pRequiredUsers);
		XUSER_STATS_SPEC *pSpecs = reinterpret_cast<XUSER_STATS_SPEC*> (m_pRequriredScoreboards);
		NetPlay::C_AsyncTask* pAsync = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);

		// work out size of results buffers needed..
		DWORD outBufferSize = 0;
		DWORD res = XUserReadStats(titleId, m_NumUsersToQuery, pXuids , m_NumScoreboardsToQuery, pSpecs, &outBufferSize, NULL, NULL);
		if (ERROR_INSUFFICIENT_BUFFER == res && outBufferSize)
		{
			m_pResultsBuffer = new U8[outBufferSize];
			if (m_pResultsBuffer)
			{
				res = XUserReadStats(titleId, m_NumUsersToQuery, pXuids, m_NumScoreboardsToQuery, pSpecs, &outBufferSize, (XUSER_STATS_READ_RESULTS*)m_pResultsBuffer, &(*pAsync));
				if (ERROR_IO_PENDING == res)
				{
					m_Reading = true;
					ok = true;
				}
				else
					StopAndCleanup();
			} 
		}  

		return ok;
	}


	// continue asynchronous read operation.
	bool C_ReaderByUsers::Continue(bool& errorOccurred)
	{
		errorOccurred = true;
		if (!m_Initialised)
			return true;
		
		if (!m_Reading)
			return true;

		NetPlay::C_AsyncTask* pAsync = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);
		if (!pAsync->Finished())
			return false;

		if (ERROR_SUCCESS == pAsync->ErrorCode())
		{
			pAsync->Close();
			m_Reading = false;
			errorOccurred = false;
		}
		else
			StopAndCleanup(); // failed to read so release results buffers now.

		return true;
	}


	// extract row data from scoreboard read results..
	U32 C_ReaderByUsers::ExtractRows(const U32 scoreboardId, RowData* pOutput, const U32 maxRowsToExtract)
	{
		if (!m_Initialised)
			return 0;
		
		assert(pOutput);
		assert(maxRowsToExtract);
		if (NULL == pOutput || 0 == maxRowsToExtract)
			return 0;

		// make sure we are not still reading the data from the server.
		if (m_Reading)
		{
			LOGERROR(NetPlay::g_Log, "can't extract scoreboard rows when data is still being read from the server!\n");
			return 0;
		}

		// check results buffer is still valid.
		if (NULL == m_pResultsBuffer)
		{
			LOGERROR(NetPlay::g_Log, "can't extract scoreboard rows. Results buffer has already been released!\n");
			return 0;
		}

		U32 rowCount = 0;
		U32 i=0;
		const XUSER_STATS_READ_RESULTS* pRes = reinterpret_cast<XUSER_STATS_READ_RESULTS*> (m_pResultsBuffer);
		XUSER_STATS_VIEW* pSb = NULL;
		for (i=0; i<pRes->dwNumViews; i++)
		{
			if (scoreboardId == pRes->pViews[i].dwViewId)
			{
				// found scoreboard we want.
				pSb = &pRes->pViews[i];
				break;
			}
		}

		if (pSb)
		{
			if (pSb->dwNumRows > maxRowsToExtract)
				LOGWARN(NetPlay::g_Log, "Scoreboard results contain more rows than you can extract!\n");
		
			// extract row data now.
			for (U32 z=0; z<pSb->dwNumRows; z++)
			{
				const XUSER_STATS_ROW& row = pSb->pRows[z];
				
				if (0 == row.dwRank)
					continue;	// this player has no data on this scoreboard yet so don't return his entry. maintains cross platform code with demonware.

				RowData out;

				// set scoreboard identifier.
				out.m_ScoreboardId		=	pSb->dwViewId;

				// set name
				_snwprintf_s(out.m_Userdata.m_Name, lengthof(out.m_Userdata.m_Name), _TRUNCATE, L"%S", row.szGamertag);

				// set XUID.
				out.m_Userdata.m_Uid	=	row.xuid;
				out.m_Userdata.m_IsGuest=	false;

				// set entity id. same as XUID on 360
				out.m_EntityId			=	row.xuid;

				// set rank.
				out.m_Rank				=	row.dwRank;

				// set rating.
				out.m_Rating			=	row.i64Rating;
				
				// copy all column data
				for (U32 r=0; r<row.dwNumColumns; r++)
				{	
					ColumnData cd;
					const XUSER_STATS_COLUMN& c = row.pColumns[r];	
					
					switch (c.Value.type)
					{
					case XUSER_DATA_TYPE_INT32:
						cd.SetS32(c.wColumnId, c.Value.nData);
						out.m_Columns.PushBack(cd);
						break;
					case XUSER_DATA_TYPE_INT64:
						cd.SetS64(c.wColumnId, c.Value.i64Data);
						out.m_Columns.PushBack(cd);
						break;
					case XUSER_DATA_TYPE_FLOAT:
						cd.SetF32(c.wColumnId, c.Value.fData);
						out.m_Columns.PushBack(cd);
						break;
					case XUSER_DATA_TYPE_DOUBLE:
						cd.SetF64(c.wColumnId, c.Value.dblData);
						out.m_Columns.PushBack(cd);
						break;
					case XUSER_DATA_TYPE_DATETIME:
						cd.SetFiletime(c.wColumnId, c.Value.ftData);
						out.m_Columns.PushBack(cd);
						break;
					}

					if (out.m_Columns.Size() == out.m_Columns.Capacity())
						break; // reached maximum number of columns we can support.
				}

				// copy to output
				pOutput[rowCount++] = out;

				if (rowCount == maxRowsToExtract)
					break; // maximum number of rows copied to results
			}	
		}
		else
			LOGERROR(NetPlay::g_Log, "can't find specified scoreboard in server results data!\n");
		
		return rowCount;
	}

	bool C_ReaderByUsers::ExtractRowForUser(const SysProfile::UserIdentifier& user,const U32 scoreboardId, RowData& rowOut)
	{
		if (!m_Initialised)
			return false;

		// make sure we are not still reading the data from the server.
		if (m_Reading)
		{
			LOGERROR(NetPlay::g_Log, "can't extract scoreboard row when data is still being read from the server!\n");
			return false;
		}

		// check results buffer is still valid.
		if (NULL == m_pResultsBuffer)
		{
			LOGERROR(NetPlay::g_Log, "can't extract scoreboard row. Results buffer has already been released!\n");
			return false;
		}

		U32 i=0;
		const XUSER_STATS_READ_RESULTS* pRes = reinterpret_cast<XUSER_STATS_READ_RESULTS*> (m_pResultsBuffer);
		XUSER_STATS_VIEW* pSb = NULL;
		for (i=0; i<pRes->dwNumViews; i++)
		{
			if (scoreboardId == pRes->pViews[i].dwViewId)
			{
				// found scoreboard we want.
				pSb = &pRes->pViews[i];
				break;
			}
		}

		if (NULL == pSb)
		{
			LOGERROR(NetPlay::g_Log, "can't extract scoreboard row. scoreboard data for id 0x%x not found in results", scoreboardId);
			return false;
		}

		for (U32 z=0; z<pSb->dwNumRows; z++)
		{
			const XUSER_STATS_ROW& row = pSb->pRows[z];
				
			if (user.m_Uid != row.xuid)
				continue; // not the user we are looking for

			if (0 == row.dwRank)
				return false;	// this player has no data on this scoreboard yet so don't return his entry. maintains cross platform code with demonware.

			// set scoreboard identifier.
			rowOut.m_ScoreboardId =	pSb->dwViewId;

			// set name
			_snwprintf_s(rowOut.m_Userdata.m_Name, lengthof(rowOut.m_Userdata.m_Name), _TRUNCATE, L"%S", row.szGamertag);

			// set XUID.
			rowOut.m_Userdata.m_Uid	=	row.xuid;
			rowOut.m_Userdata.m_IsGuest = false;

			// set entity id. same as XUID on 360
			rowOut.m_EntityId		=	row.xuid;

			// set rank.
			rowOut.m_Rank			=	row.dwRank;

			// set rating.
			rowOut.m_Rating			=	row.i64Rating;

			// copy all column data
			for (U32 r=0; r<row.dwNumColumns; r++)
			{	
				ColumnData cd;
				const XUSER_STATS_COLUMN& c = row.pColumns[r];	

				switch (c.Value.type)
				{
				case XUSER_DATA_TYPE_INT32:
					cd.SetS32(c.wColumnId, c.Value.nData);
					rowOut.m_Columns.PushBack(cd);
					break;
				case XUSER_DATA_TYPE_INT64:
					cd.SetS64(c.wColumnId, c.Value.i64Data);
					rowOut.m_Columns.PushBack(cd);
					break;
				case XUSER_DATA_TYPE_FLOAT:
					cd.SetF32(c.wColumnId, c.Value.fData);
					rowOut.m_Columns.PushBack(cd);
					break;
				case XUSER_DATA_TYPE_DOUBLE:
					cd.SetF64(c.wColumnId, c.Value.dblData);
					rowOut.m_Columns.PushBack(cd);
					break;
				case XUSER_DATA_TYPE_DATETIME:
					cd.SetFiletime(c.wColumnId, c.Value.ftData);
					rowOut.m_Columns.PushBack(cd);
					break;
				}

				if (rowOut.m_Columns.Size() == rowOut.m_Columns.Capacity())
					return true; // reached maximum number of columns we can support.
			}
			return true;
		}

		return false;
	}

	// private function to release resources
	void C_ReaderByUsers::CleanUp(void)
	{
		StopAndCleanup();

		if (m_pAsyncTask)
		{
			// stop async task is running then delete handle
			NetPlay::C_AsyncTask* p = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);
			p->Close();
			delete p;
			m_pAsyncTask = NULL;
		}

		if (m_pRequriredScoreboards)
		{
			// delete scoreboard storage.
			XUSER_STATS_SPEC *p = reinterpret_cast<XUSER_STATS_SPEC*> (m_pRequriredScoreboards);
			delete [] p;
			m_pRequriredScoreboards = NULL;
		}

		if (m_pRequiredUsers)
		{
			// delets user storage..
			XUID* p = reinterpret_cast<XUID*> (m_pRequiredUsers);
			delete [] p;
			m_pRequiredUsers = NULL;
		}

		m_NumScoreboardsToQuery = 0;
		m_NumUsersToQuery = 0;
		m_Initialised = false;
		m_Reading = false;
	}
}// end namespace	