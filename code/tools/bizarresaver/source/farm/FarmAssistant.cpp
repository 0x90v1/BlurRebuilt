#include "farmpch.h"
#include "farm/Farm.h"

FarmAssistant::FarmAssistant() :
		m_State(StateOccupied),
		m_JobNo(-1)
{
}

FarmAssistant::~FarmAssistant()
{
}

bool FarmAssistant::OnStart()
{
	return true;
}

bool FarmAssistant::OnStop()
{
	KillMyJobs();

	return Send("QUIT");
}

bool FarmAssistant::OnCommand(const char* Command)
{
	if (stricmp(Command, "FREE") == 0)
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
		FinishJob(m_JobNo);
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
		return false;

	return true;
}

bool FarmAssistant::OnIdle()
{
	if ((m_State == StateFree) && IsEnabled())
	{
		char Buffer[4096];

		if ((m_JobNo = GetFreeJob(Buffer, sizeof(Buffer))) >= 0)
		{
			char Translated[4096];
			char* Rp=NULL, *Wp = NULL;
			for(Rp=Buffer, Wp=Translated; *Rp; )
			{
				if (strnicmp(Rp, "$$JOB$$", 7) == 0)
				{
					Rp += strlen("$$JOB$$");

					int Length(_snprintf(Wp, (Translated + sizeof(Translated) - 1) - Wp, "%i", m_JobNo));

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

			if (!Send("JOB %s", Translated))
				return false;

			m_State = StatePending;
		}
	}

	if ((m_State == StatePending) || (m_State == StateAccepted))
	{
		if (IsJobCancelled(m_JobNo))
			if (!Send("KILL"))
				return false;
	}

	return true;
}

StrangerType FarmAssistant::GetType() const
{
	return ST_Assistant;
}

DWORD FarmAssistant::GetIdleTime() const
{
	return 2500;
}