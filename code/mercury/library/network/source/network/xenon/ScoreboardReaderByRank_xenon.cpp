#include <StdAfx.h>
#include <Network/ScoreboardReaderByRank.h>
#include <Network/ScoreboardStructures.h>
#include <SystemProfile/SystemProfile.h>
#include <core/Maths.h>
#include "AsyncTask_xenon.h"


namespace Scoreboard
{
	// constructor
	C_ReaderByRank::C_ReaderByRank() :	m_Initialised(false), m_Reading(false), m_MaxScoreboardsToReadFrom(10), m_NumScoreboardsToQuery(0), m_pRequriredScoreboards(NULL),
										m_pAsyncTask(NULL), m_pResultsBuffer(NULL), m_Handle(NULL)
	{
	}


	// destructor
	C_ReaderByRank::~C_ReaderByRank()
	{
		CleanUp();
	}


	// create dynamic resources.
	bool C_ReaderByRank::Initialise(void)
	{
		if (m_Initialised)
			return true;

		// storage for what scoreboards we will request data from.
		m_pRequriredScoreboards = reinterpret_cast<void*> (new XUSER_STATS_SPEC[m_MaxScoreboardsToReadFrom]);

		//allocate async task handle.
		m_pAsyncTask = reinterpret_cast<void*> (new NetPlay::C_AsyncTask);

		if (NULL != m_pRequriredScoreboards && NULL != m_pAsyncTask)
			m_Initialised = true;
		else
			CleanUp();

		return m_Initialised;
	}


	// add a scoreboard to the list of what needs to be queried.
	bool C_ReaderByRank::AddScoreboard(const U32 scoreboardId, const ColumnIdentifierList& columns, const ColumnTypeList& columnTypes, const bool flushCache)
	{
		if (!m_Initialised)
			return false;

		assert(!m_Reading);
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


	// private function to de allocate all dynamic cesources
	void C_ReaderByRank::CleanUp(void)
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
		
		m_NumScoreboardsToQuery = 0;
		m_Initialised = false;
		m_Reading = false;
	}


	// request scoreboard data starting at a specific rank.
	bool C_ReaderByRank::StartReadByRank(const U32 titleId, const U32 startRank, const U32 maxRowsToRetrieve)
	{
		if (!m_Initialised)
			return false;
		
		// stop any old server request
		StopAndCleanup();

		assert(maxRowsToRetrieve);
		if (0 == m_NumScoreboardsToQuery || 0 == maxRowsToRetrieve)
			return false;

		bool ok = false;
		XUSER_STATS_SPEC *pSpecs = reinterpret_cast<XUSER_STATS_SPEC*> (m_pRequriredScoreboards);
		NetPlay::C_AsyncTask* pAsync = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);

		const U32 r = (startRank == 0) ? 1 : startRank; // ranks start at 1 not 0.

		DWORD resultsBufferSize = 0;
		DWORD res = XUserCreateStatsEnumeratorByRank(titleId, r, maxRowsToRetrieve, m_NumScoreboardsToQuery, pSpecs, &resultsBufferSize, &m_Handle);
		if (ERROR_SUCCESS == res && resultsBufferSize)
		{
			m_pResultsBuffer = new U8[resultsBufferSize];
			if (m_pResultsBuffer)
			{
				res = XEnumerate(m_Handle, m_pResultsBuffer, resultsBufferSize, NULL, &(*pAsync));
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


	// start a read request based on a users pivot point in then scoreboard.
	bool C_ReaderByRank::StartReadByPivot(const U32 titleId, const SysProfile::UserIdentifier& userPivot, const U32 maxRowsToRetrieve)
	{
		if (!m_Initialised)
			return false;
	
		// stop any old server request
		StopAndCleanup();

		assert(maxRowsToRetrieve);
		if (0 == m_NumScoreboardsToQuery || 0 == maxRowsToRetrieve)
			return false;

		bool ok = false;
		XUSER_STATS_SPEC *pSpecs = reinterpret_cast<XUSER_STATS_SPEC*> (m_pRequriredScoreboards);
		NetPlay::C_AsyncTask* pAsync = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);

		DWORD resultsBufferSize = 0;
		DWORD res = XUserCreateStatsEnumeratorByXuid(titleId, userPivot.m_Uid, maxRowsToRetrieve, m_NumScoreboardsToQuery, pSpecs, &resultsBufferSize, &m_Handle);
		if (ERROR_SUCCESS == res && resultsBufferSize)
		{
			m_pResultsBuffer = new U8[resultsBufferSize];
			if (m_pResultsBuffer)
			{
				res = XEnumerate(m_Handle, m_pResultsBuffer, resultsBufferSize, NULL, &(*pAsync));
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
	bool C_ReaderByRank::Continue(bool& errorOccurred)
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
	U32 C_ReaderByRank::ExtractRows(const U32 scoreboardId, RowData* pOutput, const U32 maxRowsToExtract, U32& scoreboardSize)
	{
		scoreboardSize = 0;

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
			// set total rows on this scoreboard.
			scoreboardSize = pSb->dwTotalViewRows;

			if (pSb->dwNumRows > maxRowsToExtract)
				LOGWARN(NetPlay::g_Log, "Scoreboard results contain more rows than you can extract!\n");
		
			// extract row data now.
			for (i=0; i<pSb->dwNumRows; i++)
			{
				const XUSER_STATS_ROW& row = pSb->pRows[i];
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
				pOutput[i] = out;

				rowCount++;

				if (rowCount == maxRowsToExtract)
					break; // maximum number of rows copied to results
			}	
		}
		else
			LOGERROR(NetPlay::g_Log, "can't find specified scoreboard in server results data!\n");
		
		return rowCount;
	}


	// stop asynchronous task and release release restlts buffer.
	void C_ReaderByRank::StopAndCleanup(void)
	{
		if (!m_Initialised)
			return;

		NetPlay::C_AsyncTask* p = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);
		p->Close();

		// close enumerator handle..
		if (NULL != m_Handle && INVALID_HANDLE_VALUE != m_Handle)
			CloseHandle(m_Handle);
		m_Handle = NULL;

		if (m_pResultsBuffer)
			delete [] m_pResultsBuffer;
		m_pResultsBuffer = NULL;

		m_Reading = false;
	}
}// end namespace	