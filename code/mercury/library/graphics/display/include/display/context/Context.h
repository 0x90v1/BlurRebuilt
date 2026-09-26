#ifndef __DISPLAY_CONTEXT_H__
#define __DISPLAY_CONTEXT_H__

#if PLATFORM_PC
#include "display/context/context_pc.h"
#elif PLATFORM_XENON
#include "display/context/context_xenon.h"
//#include "display/context/context_singlethreaded.h"
#elif PLATFORM_PS3 || PLATFORM_SPU
#include "display/context/context_ps3.h"
#else
#include "display/context/context_singlethreaded.h"
#endif

#endif
