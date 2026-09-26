#include "stdafx.h"
#include "Application.h"

int main(int argc, char* argv[])
{
#ifdef BUILD_CONFIG_DEBUG
	MessageBox(NULL, "BizBrain: Attach now", "Status", MB_OK | MB_SETFOREGROUND | MB_SYSTEMMODAL);
#endif

	C_Application app(
		"BizBrain",
		"",
		"tools.bizbrain",
		"\\\\bizsymbol\\\\CrashDumps\\",
		false);
		
	return app.Run(argc, argv);
}

