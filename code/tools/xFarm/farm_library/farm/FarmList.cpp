#include "StdAfx.h"
#include "Farm.h"

FarmingList::FarmingList()
{
}

FarmingList::~FarmingList()
{
}

bool FarmingList::OnStart()
{
	if (!AnnounceUser("MANAGER"))
		return false;

	return Send("LIST");
}

bool FarmingList::OnStop()
{
	return Send("QUIT");
}

bool FarmingList::OnCommand(const char* Command)
{
	if (stricmp(Command, "LIST START") == 0)
	{
		FarmPrint("JobNo    State    Comment         Command\r\n");
		FarmPrint("------------------------------------------------------------------------------\r\n");

		return true;
	}
	else if (stricmp(Command, "LIST END") == 0)
	{
		FarmPrint("------------------------------------------------------------------------------\r\n");

		return false;
	}
	else if (strnicmp(Command, "JOB ", 4) == 0)
	{
		const char* Data(Command + 4);

		int Idx(SnapInt(Data));
		int JobNo(SnapInt(Data));
		char State(SnapChar(Data));
		int ProcessNo(SnapInt(Data));
		int TimeStarted(SnapInt(Data));
		String User(SnapAtom(Data));
		String Command(SnapString(Data));
		String Comment(SnapString(Data));

		FarmPrint("%-8i %-8.8s %-15.15s %.44s\r\n", JobNo, (State == 'F') ? "Free" : "Active", Comment.Get(), Command.Get());

		return true;
	}

	return false;
}

bool FarmingList::OnIdle()
{
	return true;
}
