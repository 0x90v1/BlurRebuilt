#include "StdAfx.h"
#include <system/debug/log/DebugLog.h>
#include <farm_library/Utility.h>
#include <system/debug/log/subscribers/logfilesubscriber.h>

bool g_bSilentMode = false;

String SnapAtom(const char*& Text)
{
	if (!*Text)
		return String();

	const char* First = Text;

	while((*Text != '\0') && (*Text != ' '))
		Text++;

	const char* Last = Text;

	if (*Text == ' ')
		Text++;

	return String(First, Last - First);
}

int SnapInt(const char*& Text)
{
	return atoi(SnapAtom(Text));
}

char SnapChar(const char*& Text)
{
	return SnapAtom(Text).Get()[0];
}

String SnapString(const char*& Text)
{
	int Count(SnapInt(Text));

	const char* First(Text);

	for(int Moved(Count); Moved-- > 0 && *Text; )
		Text++;

	if (*Text == ' ')
		Text++;

	return String(First, Count);
}

const long MAX_LENGTH_DEBUG_STR = 255;

Debug::Logger &g_FarmLog = Debug::GetLogger("farmlog");

void InitFarmPrintLog()
{
	Debug::LogFileSubscriber *pS = new Debug::LogFileSubscriber();
	g_FarmLog.AddSubscriber(pS);
	pS->SetXMLParameter("file", "c:\\xfarmnet.log");
	pS->Start();

	g_FarmLog.SetLevel( DebugLevel::Debug );
}

void FarmPrint(const char *c, ...)
{
	if (g_bSilentMode)
		return;

	g_FarmLog.SetPropagateMessages(false);

	char Buffer[4096];

	va_list vl;

	va_start(vl, c);
	vsnprintf(Buffer, sizeof(Buffer), c, vl);
	va_end(vl);

	S_DEBUG( g_FarmLog, Buffer );
}
