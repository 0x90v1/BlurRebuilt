#include "farmpch.h"

#include "farm/Farm.h"

FarmList::FarmList()
{
}

FarmList::~FarmList()
{
}

bool FarmList::OnStart()
{
	if (!AnnounceUser("MANAGER"))
		return false;

	return Send("LIST");
}

bool FarmList::OnStop()
{
	return Send("QUIT");
}

bool FarmList::OnCommand(const char* Command)
{
	if (stricmp(Command, "LIST START") == 0)
	{
		fprintf(stderr, "JobNo    State    Comment         Command\r\n");
		fprintf(stderr, "------------------------------------------------------------------------------\r\n");

		return true;
	}
	else if (stricmp(Command, "LIST END") == 0)
	{
		fprintf(stderr, "------------------------------------------------------------------------------\r\n");

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

		fprintf(stderr, "%-8i %-8.8s %-15.15s %.44s\r\n", JobNo, (State == 'F') ? "Free" : "Active", Comment, Command);

		return true;
	}

	return false;
}

bool FarmList::OnIdle()
{
	return true;
}