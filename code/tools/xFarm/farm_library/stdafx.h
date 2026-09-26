// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _CRT_SECURE_NO_DEPRECATE

#include "tools_common/SystemDefines.h"

const F32 PI = 3.1415926535897932384626433832795f;

#include <cc++/Thread.h>
#include <cc++/SocketPort.h>
#include <cc++/Socket.h>
#include <cc++/Address.h>
#include <cc++/Missing.h>

#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#ifdef WIN32
#include <crtdbg.h>
#endif

#include <string>
#include <map>
#include <vector>
#include <sstream>

#ifdef WIN32
#pragma warning(disable:4311)
#pragma warning(disable:4312)
#pragma warning(disable:4244)
#endif

#include <core/maths.h>
#include <core/utmaths.h>
#include <core/containers/vector.h>
#include <core/containers/list.h>
#include <core/threads.h>


// ken utils
#include <tools_debug/DebugManager.h>
#include <tools_common\StringManipulation.h>
#include <system\DebugLog.h>
namespace xFarm
{
extern Debug::Logger &g_Log;
}

//#include <WindowsX.h>
#ifdef WIN32
#include <CommCtrl.h>
#endif
#include <farm_library/Utility.h>

