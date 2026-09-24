#include "stdafx.h"
#include "Application.h"

int main(int argc, char* argv[])
{
	C_Application app(
		"Texture Export Test",
		"",
		"tools.examplecommandlinetool",
		"\\\\bizbuild03\\\\CrashDumps\\",
		false);
		
	return app.Run(argc, argv);
}

