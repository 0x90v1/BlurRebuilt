#include "StdAfx.h"

void DebugMessage(const char* Prefix, const char* Message, va_list vl)
{
	char	Buffer[512];
	char*	Curr			= Buffer;
	int		BufferLength	= sizeof(Buffer);

	for(; *Prefix && BufferLength > 2; BufferLength--)
		*Curr++ = *Prefix++;

	int Length = _vsnprintf(Curr, BufferLength, Message, vl);

	BufferLength	-= Length;
	Curr			+= Length;

	while(BufferLength < 2)
	{
		BufferLength++;
		Curr--;
	}

	Curr[0] = '\n';
	Curr[1] = '\0';

	OutputDebugString(Buffer);
}

void DebugInfo(const char* Message, ...)
{
	va_list vl;
	va_start(vl, Message);
	DebugMessage("[INFO] ", Message, vl);
	va_end(vl);
}

void DebugWarning(const char* Message, ...)
{
	va_list vl;
	va_start(vl, Message);
	DebugMessage("[WARN] ", Message, vl);
	va_end(vl);
}

void DebugError(const char* Message, ...)
{
	va_list vl;
	va_start(vl, Message);
	DebugMessage("[ERR] ", Message, vl);
	va_end(vl);
}

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