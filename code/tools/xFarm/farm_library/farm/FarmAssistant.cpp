#include "StdAfx.h"
#include "Farm.h"
#include "system\debuglog.h"
#include <system/debug/log/DebugLog.h>
#include "core\threads.h"
#include "system\network.h"
#include "system\debug\log\networkrepeater.h"
#include "system\serialise.h"
#include "system\serialise\C_MemoryStream.h"
#include "core\utility\endian.h"
#include "system\time\time.h"

using namespace xFarm;

FarmAssistant::FarmAssistant() :
		m_State(StateOccupied),
		m_JobNo(-1),
		m_bScreensaverMode(false)
{
	m_LastCommandTime = ost::getTicks();
	m_bReceiveLogEvent = false;
}

FarmAssistant::~FarmAssistant()
{
}

bool FarmAssistant::OnStart()
{
	return true;
}

void FarmAssistant::SetScreensaverMode(bool bMode)
{
	m_bScreensaverMode = bMode;
}

bool FarmAssistant::OnStop()
{
	LOGDEBUG( g_Log, "%s: FarmAssistant::OnStop() -> killing jobs\n", m_Process->GetMyUserName());
	if (!m_bScreensaverMode)
		KillMyJobs();

	return Send("QUIT");
}

void FarmAssistant::ReceiveLogEvent(const char *pData)
{
	// work out whether we need to endian-swap. the log event should be < 0x80000
	int iTest = *(int*)pData;
	bool bNeedEndianSwap = !(iTest < 0x80000);

	// deserialise the event
	Debug::LogEvent event;
	Sys::C_MemoryStream buf((U8*)pData, (U32)0x7fffffff);
	if (bNeedEndianSwap)
	{
		Sys::BinaryDeserialiser<Ut::DoEndianSwap> ar(buf);

		ar >> event;
	}
	else
	{
		Sys::BinaryDeserialiser<Ut::NoEndianSwap> ar(buf);

		ar >> event;
	}

	// format the event and append it to the log file
	U64 secs = event.time;
	int c = event.msg.Length();

	// convert time from UTC to local
#if defined( PLATFORM_PC )
	secs += SYS_DELTA_EPOCH_IN_USEC;
	secs *= 10;

	SYSTEMTIME st, st2;
	FileTimeToSystemTime((FILETIME*)&secs, &st);
	SystemTimeToTzSpecificLocalTime(NULL, &st, &st2);
	SystemTimeToFileTime(&st2, (FILETIME*)&secs);

	secs /= 10;
	secs -= SYS_DELTA_EPOCH_IN_USEC;
#endif

	secs = secs / SYS_USEC_PER_SEC;

	m_Process->AppendLog(m_JobNo, event.level, (int)secs, event.logger.Chars(), event.msg.Chars());
}

bool FarmAssistant::OnCommand(const char* Command)
{
	m_LastCommandTime = ost::getTicks();
	if (m_bReceiveLogEvent)
	{
		ReceiveLogEvent(Command);	
		m_bReceiveLogEvent = false;
		return true;
	}

	if (stricmp(Command, "LOG") == 0)
	{
		m_bReceiveLogEvent = true;
	}
	else if (stricmp(Command, "FREE") == 0)
	{
		if (m_State == StateOccupied)
			m_State = StateFree;
	}
	else if (stricmp(Command, "BUSY") == 0)
	{
		if (m_State != StateAccepted)
			m_State = StateOccupied;
	}
	else if (stricmp(Command, "ACCEPTED") == 0)
	{
		AcceptJob(m_JobNo);
		m_State = StateAccepted;
	}
	else if (stricmp(Command, "IGNORED") == 0)
	{
		FreeJob(m_JobNo);
		m_State = StateFree;
		m_JobNo = -1;
	}
	else if (stricmp(Command, "FINISHED") == 0)
	{
		FinishJob(m_JobNo, true);
		m_State = StateFree;
		m_JobNo = -1;

		if (!OnIdle())
			return false;
	}
	else if (strnicmp(Command, "FAILED", 6) == 0)
	{
		FinishJob(m_JobNo, false);
		m_State = StateFree;
		m_JobNo = -1;

		if (!OnIdle())
			return false;
	}
	else if (stricmp(Command, "CANCELLED") == 0)
	{
		CancelJob(m_JobNo);
		m_State = StateFree;
		m_JobNo = -1;

		if (!OnIdle())
			return false;
	}
	else
	{
		LOGDEBUG( xFarm::g_Log, "%s: Unrecognized command\n", m_Process->GetMyUserName());
		return false;
	}

	return true;
}

bool FarmAssistant::OnIdle()
{
	// check how long ago we last received a message
	unsigned long diff = ost::getTicks() - m_LastCommandTime;
	if (diff > 60*10*1000 && diff < 0x7fffffff)
	{
		// it's been more than 10 minutes; quit out
		LOGDEBUG( xFarm::g_Log, "%s ping timeout %d\n", m_Process->GetMyUserName(), (int)diff);
		return false;
	}

	// if the worker we are connected to has nothing to do...
	if ((m_State == StateFree) && IsActive())
	{
		char Buffer[4096];

		// get a free job
		if ((m_JobNo = GetFreeJob(Buffer, sizeof(Buffer), m_Process->getFlags())) >= 0)
		{
			char Translated[4096];
			char *Rp, *Wp;

			// prepare commandline for sending
			for(Rp = Buffer, Wp = Translated; *Rp; )
			{
				if (strnicmp(Rp, "$$JOB$$", 7) == 0)
				{
					Rp += strlen("$$JOB$$");

					int Length(snprintf(Wp, (Translated + sizeof(Translated) - 1) - Wp, "%i", m_JobNo));

					if (Length > 0)
						Wp  += Length;
					else
						break;
				}
				else
				{
					if (Wp >= (Translated + sizeof(Translated) - 1))
						break;

					*Wp++ = *Rp++;
				}
			}

			*Wp = '\0';

			// send job for worker to process
			if (!Send("JOB %s", Translated))
			{
				LOGDEBUG( xFarm::g_Log, "%s: send job failed\n", m_Process->GetMyUserName());
				return false;
			}

			m_State = StatePending;
		}
	}

	if ((m_State == StatePending) || (m_State == StateAccepted))
	{
		if (IsJobCancelled(m_JobNo))
			if (!Send("KILL"))
			{
				LOGDEBUG( xFarm::g_Log, "%s: send kill failed\n", m_Process->GetMyUserName());
				return false;
			}
	}

	return true;
}

StrangerType FarmAssistant::GetType() const
{
	return ST_Assistant;
}

int FarmAssistant::GetIdleTime() const
{
	return 2500;
}

int FarmAssistant::GetState() const
{
	return m_State;
}

int FarmAssistant::GetJobNo() const
{
	return m_JobNo;
}
