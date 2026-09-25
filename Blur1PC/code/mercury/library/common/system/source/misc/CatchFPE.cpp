#include "SystemPch.h"

#if !defined(CFG_SPEED) && defined(PLATFORM_XENON)

bool g_bFPEnabled(false);

void EnableFloatExceptions(void)
{
	if (g_bFPEnabled)
	{
		_clearfp();
		_controlfp(_EM_DENORMAL|_EM_INEXACT|_EM_OVERFLOW|_EM_UNDERFLOW, _MCW_EM );

		// Enable FP-precise - will stop on problem instruction using this
		__asm 
		{ 
			mfmsr r11;
			ori r11, r11, 0x900;
			mtmsr r11;
		}
	}
}

void DisableFloatExceptions(void)
{
	if (g_bFPEnabled)
	{
		_controlfp(~0, _MCW_EM );
	}
}

#endif