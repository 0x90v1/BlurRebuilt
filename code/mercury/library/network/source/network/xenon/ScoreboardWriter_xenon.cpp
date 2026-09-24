#include <StdAfx.h>
#include <Network/ScoreboardWriter.h>
#include <Network/ScoreboardStructures.h>
#include <Network/Session.h>
#include <SystemProfile/SystemProfile.h>
#include "AsyncTask_xenon.h"


namespace Scoreboard
{
	// constructor
	C_Writer::C_Writer() :	m_Initialised(false), m_Writing(false), m_MaxScoreboards(0), m_MaxColumns(0), m_NumScoreboardsToWrite(0), m_NumColumnsToWrite(0), 
							m_pScoreboardData(NULL), m_pColumnData(NULL), m_pAsyncTask(NULL)
	{
	}

	
	// destructor
	C_Writer::~C_Writer()
	{
		StopAndCleanup();
		CleanUp();
	}


	// initialise system and allocate dynamic resources.
	bool C_Writer::Initialise(const U32 maxScoreboardsToWrite, const U32 maxColumns)
	{
		if (m_Initialised)
			return true;
	
		assert(maxScoreboardsToWrite);
		assert(maxColumns);

		// max scoreboards to write is 6 (1 trueskill + 5 game defined).
		if (maxScoreboardsToWrite > 0 && maxScoreboardsToWrite <= 6)
			m_MaxScoreboards = maxScoreboardsToWrite;
		else
		{
			m_MaxScoreboards = 6;
			LOGWARN(NetPlay::g_Log, "Scoreboard::C_Writer::Initialise() - invalid maximun number of scoreboards supplied, using max (6)");
		}

		m_MaxColumns = (maxColumns > 0) ? maxColumns : 20;

		// allocate column data
		m_pColumnData = reinterpret_cast<void*> (new XUSER_PROPERTY[m_MaxColumns]);

		// allocate scoreboard data
		m_pScoreboardData = reinterpret_cast<void*> (new XSESSION_VIEW_PROPERTIES[m_MaxScoreboards]);

		// allocate async task
		m_pAsyncTask = reinterpret_cast<void*> (new NetPlay::C_AsyncTask);

		if (m_pAsyncTask && m_pScoreboardData && m_pColumnData)
			m_Initialised = true;
		else
			CleanUp();

		return m_Initialised;
	}

	
	// stop any scoreboard write and reset internal data to default.
	bool C_Writer::StopAndCleanup(void)
	{
		if (!m_Initialised)
			return false;

		// stop asynchronous write if active
		NetPlay::C_AsyncTask* pT = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);
		if (!pT->Finished())
		{
			pT->Close();
			LOGWARN(NetPlay::g_Log, "Scoreboard::C_Writer - manually stopping write before it's finished !!");
		}

		// setup data ready for new write 
		m_NumScoreboardsToWrite = 0;
		m_NumColumnsToWrite = 0;
		m_Writing = false;

		return true;
	}


	// Queue some scoreboard data for writing.
	bool C_Writer::Queue(const U32 scoreboardId, const S64 rating, const eWriteType writeType, const ColumnData* pColumnData, const U32 numColumns)
	{
		if (!m_Initialised)
			return false;

		if (m_Writing)
		{
			LOGERROR(NetPlay::g_Log, "Can't Queue Scoreboard data when still writing old data to server !!\n");
			return false; // cant Queue Data when writing current data
		}

		assert(pColumnData);
		assert(numColumns);
		if (NULL == pColumnData || 0 == numColumns)
			return false;

		if (m_NumScoreboardsToWrite == m_MaxScoreboards)
		{
			LOGERROR(NetPlay::g_Log, "Trying to write to more scoreboards than was originally allocated !!\n");
			return false;
		}

		if (m_NumColumnsToWrite+numColumns > m_MaxColumns)
		{
			LOGERROR(NetPlay::g_Log, "Trying to write to more columns than was originally allocated !!\n");
			return false;
		}

		// yay we can now copy data into internal structures..
		XUSER_PROPERTY *pColumns =  reinterpret_cast<XUSER_PROPERTY*> (m_pColumnData);
		XSESSION_VIEW_PROPERTIES* pViews = reinterpret_cast<XSESSION_VIEW_PROPERTIES*> (m_pScoreboardData);

		// setup which scoreboard this data is for..
		XSESSION_VIEW_PROPERTIES& view = pViews[m_NumScoreboardsToWrite++];
		view.dwViewId			=	scoreboardId;
		view.dwNumProperties	=	numColumns;
		view.pProperties		=	&pColumns[m_NumColumnsToWrite];

		// configure all column data
		for (U32 i=0; i<numColumns; i++)
		{
			XUSER_PROPERTY& col = pColumns[m_NumColumnsToWrite++];
			const ColumnData& source = pColumnData[i];

			col.dwPropertyId	=	source.m_FieldId;
			switch (source.m_DataType)
			{
			case Scoreboard::Int32:
				col.value.type	= XUSER_DATA_TYPE_INT32;
				col.value.nData	= source.m_Data.m_S32; 
				break;
			case Scoreboard::Int64:
				col.value.type		= XUSER_DATA_TYPE_INT64;
				col.value.i64Data	= source.m_Data.m_S64;				
				break;
			case Scoreboard::Float64:
				col.value.type		= XUSER_DATA_TYPE_DOUBLE;
				col.value.dblData	= source.m_Data.m_F64;	
				break;
			case Scoreboard::Float32:
				col.value.type		= XUSER_DATA_TYPE_FLOAT;
				col.value.fData		= source.m_Data.m_F32;	
				break;
			case Scoreboard::DateTime:
				col.value.type		= XUSER_DATA_TYPE_DATETIME;
				col.value.ftData	= source.m_Data.m_DateTime;	
				break;

			default:
				LOGERROR(NetPlay::g_Log, "Trying to write invalid column data to scoreboard !!\n");
				StopAndCleanup();
				return false;
				break;
			}
		}

		return true;
	}


	// start the asynchronous write procedure
	bool C_Writer::Start(const SysProfile::UserIdentifier& user, const NetPlay::C_Session& session)
	{
		if (!m_Initialised)
			return false;

		if (m_Writing)
		{
			LOGERROR(NetPlay::g_Log, "Trying to write to scoreboard when previous write not finished. !!\n");
			return false;
		}

		if (0 == m_NumScoreboardsToWrite)
		{
			LOGWARN(NetPlay::g_Log, "Trying to write to scoreboard no data has been Queued !!\n");
			return false;
		}

		if (user.m_IsGuest)
		{
			LOGWARN(NetPlay::g_Log, "Trying to write to scoreboard for a guest player.. !!\n");
			StopAndCleanup();
			return false;
		}

		if (session.SessionState() != NetPlay::C_Session::eStatePlaying)
		{
			StopAndCleanup();
			LOGERROR(NetPlay::g_Log, "MUST finish writing to scoreboards before ending session. !!\n");
			return false;
		}

		if (session.SessionState() == NetPlay::C_Session::eStateError)
		{
			StopAndCleanup();
			LOGERROR(NetPlay::g_Log, "Session is in an Error state not writing to scoreboard. !!\n");
			return false;
		}

		NetPlay::C_AsyncTask* pT = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);
		const XSESSION_VIEW_PROPERTIES* pViews = reinterpret_cast<XSESSION_VIEW_PROPERTIES*> (m_pScoreboardData);

		const DWORD res = XSessionWriteStats(session.m_SessionHandle, user.m_Uid, m_NumScoreboardsToWrite, pViews, &(*pT));
		if (ERROR_IO_PENDING != res)
		{
			StopAndCleanup();
			return false;
		}
		
		m_Writing = true;
		return true;
	}


	// update the asynchronous write and monitor for completion.
	bool C_Writer::Continue(bool& errorOccurred)
	{
		errorOccurred = true;

		if (!m_Initialised)
			return true;

		if (!m_Writing)
			return true;

		NetPlay::C_AsyncTask* pT = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);
		if (!pT->Finished())
			return false; // still writing..

		if (pT->ErrorCode() != ERROR_SUCCESS)
			LOGWARN(NetPlay::g_Log, "Failed to Write Scoreboard data 0x%x\n", pT->ErrorCode());
		else
			errorOccurred = false;

		StopAndCleanup();
		return true;
	}


	// private function used for cleaning up resources..
	void C_Writer::CleanUp(void)
	{
		// stop and delete asyncronous task
		NetPlay::C_AsyncTask* pT = reinterpret_cast<NetPlay::C_AsyncTask*> (m_pAsyncTask);
		if (pT)
		{
			if (!pT->Finished())
				pT->Close();
			delete pT;
			m_pAsyncTask = NULL;
		}
	
		if (m_pScoreboardData)
			delete [] m_pScoreboardData;
		m_pScoreboardData = NULL;

		if (m_pColumnData)
			delete [] m_pColumnData;
		m_pColumnData = NULL;

		m_Initialised = false;
		m_MaxScoreboards = 0;
		m_MaxColumns = 0;
		m_NumScoreboardsToWrite = 0;
		m_NumColumnsToWrite = 0;
		m_Writing = false;
	}

}// end namespace