#include "farmpch.h"
#include "Farm.h"

FarmUser::FarmUser(FarmWindow& Wnd) :
		m_Wnd(Wnd),
		m_Command(0),
		m_JobVersion(0),
		m_MachineVersion(0)
{
}

FarmUser::~FarmUser()
{
}

bool FarmUser::OnStart()
{
	if (!AnnounceUser("MANAGER"))
		return false;

	m_Wnd.AddCommand(new UiCommand(UiCT_Version));
	m_Wnd.AddCommand(new UiCommand(UiCT_ListJobs));
	m_Wnd.AddCommand(new UiCommand(UiCT_ListMachines));

	return true;
}

bool FarmUser::OnStop()
{
	PostMessage(m_Wnd, g_UiMessage, UiM_ClearJobs, 0);
	PostMessage(m_Wnd, g_UiMessage, UiM_ClearMachines, 0);
	PostMessage(m_Wnd, g_UiMessage, UiM_SetList, 0);

	return Send("QUIT");
}

bool FarmUser::OnCommand(const char* Command)
{
	if (m_Command)
	{
		switch(m_Command->Type)
		{
			case UiCT_Version:
				if (strnicmp(Command, "VERSION ", 8) == 0)
				{
					const char* Data(Command + 8);
					int JobVersion(SnapInt(Data));
					int MachineVersion(SnapInt(Data));

					if (JobVersion != m_JobVersion)
					{
						m_Wnd.AddCommand(new UiCommand(UiCT_ListJobs));
						m_JobVersion = JobVersion;
					}

					if (MachineVersion != m_MachineVersion)
					{
						m_Wnd.AddCommand(new UiCommand(UiCT_ListMachines));
						m_MachineVersion = MachineVersion;
					}

					return NextCommand();
				}
			break;

			case UiCT_ListJobs:
				if (stricmp(Command, "LIST START") == 0)
				{
					PostMessage(m_Wnd, g_UiMessage, UiM_ClearJobs, 0);

					return true;
				}
				else if (stricmp(Command, "LIST END") == 0)
				{
					PostMessage(m_Wnd, g_UiMessage, UiM_SetList, 0);

					return NextCommand();
				}
				else if (strnicmp(Command, "JOB ", 4) == 0)
				{
					if (UiJob* Jb = new UiJob)
					{
						const char* Data(Command + 4);

						Jb->Idx			= SnapInt(Data);
						Jb->JobNo		= SnapInt(Data);
						Jb->State		= SnapChar(Data);
						Jb->ProcessNo	= SnapInt(Data);
						Jb->TimeStarted	= SnapInt(Data);
						Jb->User		= SnapAtom(Data);
						Jb->Command		= SnapString(Data);
						Jb->Comment		= SnapString(Data);

						PostMessage(m_Wnd, g_UiMessage, UiM_NewJob, (LPARAM)Jb);
					}

					return true;
				}
			break;

			case UiCT_ListMachines:
				if (stricmp(Command, "MACHINE START") == 0)
				{
					PostMessage(m_Wnd, g_UiMessage, UiM_ClearMachines, 0);

					return true;
				}
				else if (stricmp(Command, "MACHINE END") == 0)
				{
					PostMessage(m_Wnd, g_UiMessage, UiM_SetList, 0);

					return NextCommand();
				}
				else if (strnicmp(Command, "MACHINE ", 8) == 0)
				{
					if (UiMachine* Prc = new UiMachine)
					{
						const char* Data(Command + 8);

						Prc->ProcessNo	= SnapInt(Data);
						Prc->Machine	= SnapAtom(Data);
						Prc->IsEnabled	= SnapInt(Data) != 0;

						PostMessage(m_Wnd, g_UiMessage, UiM_NewMachine, (LPARAM)Prc);
					}

					return true;
				}
			break;

			case UiCT_Job:
				if (!strnicmp(Command, "ACCEPTED ", 9))
					return NextCommand();
				else if (!stricmp(Command, "FAILED"))
					return NextCommand();
			break;

			case UiCT_Kill:
				if (!strnicmp(Command, "CANCELLED ", 10))
					return NextCommand();
				else if (!strnicmp(Command, "BADJOB ", 7))
					return NextCommand();
			break;

			case UiCT_Priority:
				if (!strnicmp(Command, "PRIORITY ", 9))
					return NextCommand();
				else if (!strnicmp(Command, "BADJOB ", 7))
					return NextCommand();
			break;

			case UiCT_Requeue:
				if (!strnicmp(Command, "REQUEUED ", 9))
					return NextCommand();
				else if (!strnicmp(Command, "BADJOB ", 7))
					return NextCommand();
			break;

			case UiCT_UpdateCommand:
				if (!strnicmp(Command, "UPDATED ", 8))
					return NextCommand();
				else if (!strnicmp(Command, "BADJOB ", 7))
					return NextCommand();
			break;

			case UiCT_UpdateComment:
				if (!strnicmp(Command, "UPDATED ", 8))
					return NextCommand();
				else if (!strnicmp(Command, "BADJOB ", 7))
					return NextCommand();
			break;

			case UiCT_Enable:
				if (!strnicmp(Command, "ENABLED ", 8))
					return NextCommand();
				else if (!strnicmp(Command, "BADMACHINE ", 11))
					return NextCommand();
			break;

			case UiCT_Disable:
				if (!strnicmp(Command, "DISABLED ", 9))
					return NextCommand();
				else if (!strnicmp(Command, "BADMACHINE ", 11))
					return NextCommand();
			break;
		}
	}

	return false;
}

bool FarmUser::OnIdle()
{
	if (!m_Command)
		if (!NextCommand())
			return false;

	return true;
}

DWORD FarmUser::GetIdleTime() const
{
	return 500;
}

bool FarmUser::NextCommand()
{
	delete m_Command;

	if ((m_Command = m_Wnd.GetCommand()) != 0)
	{
		switch(m_Command->Type)
		{
			case UiCT_Version:
				if (!Send("VERSION"))
					return false;
			break;

			case UiCT_ListJobs:
				if (!Send("LIST"))
					return false;
			break;

			case UiCT_ListMachines:
				if (!Send("MACHINES"))
					return false;
			break;

			case UiCT_Job:
				if (!Send("JOB %i %s %i %s", strlen(m_Command->Command), m_Command->Command, strlen(m_Command->Comment), m_Command->Comment))
					return false;
			break;

			case UiCT_Kill:
				if (!Send("KILL %i", m_Command->JobNo))
					return false;
			break;

			case UiCT_Priority:
				if (!Send("PRIORITY %i", m_Command->JobNo))
					return false;
			break;

			case UiCT_Requeue:
				if (!Send("REQUEUE %i", m_Command->JobNo))
					return false;
			break;

			case UiCT_UpdateCommand:
				if (!Send("COMMAND %i %s", m_Command->JobNo, m_Command->Command))
					return false;
			break;

			case UiCT_UpdateComment:
				if (!Send("COMMENT %i %s", m_Command->JobNo, m_Command->Comment))
					return false;
			break;

			case UiCT_Enable:
				if (!Send("ENABLE %i", m_Command->JobNo))
					return false;
			break;

			case UiCT_Disable:
				if (!Send("DISABLE %i", m_Command->JobNo))
					return false;
			break;

			case UiCT_Quit:
			return false;
		}
	}

	return true;
}