// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE

#include "tools_common/systemdefines.h"

#include <cc++/Thread.h>
#include <cc++/SocketPort.h>
#include <cc++/Socket.h>
#include <cc++/Address.h>
#include <cc++/Missing.h>

#ifdef WIN32
#include <windows.h>
#endif

#include <core/core.h>

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
#include "mysql.h"
#include <sstream>
#include <vector>
#include <fstream>

#include "Database.h"
#include "Query.h"

#ifdef WIN32
#pragma warning(disable:4311)
#pragma warning(disable:4312)
#pragma warning(disable:4244)
#endif

void DBPrint(const char *c, ...);

#ifdef __GNUC__
#define min(A,B) ((A<B)?A:B)
#define max(A,B) ((A>B)?A:B)
#endif

#include <core/utmaths.h>
#include <core/containers/vector.h>
#include <core/containers/list.h>
#include <core/threads.h>


// ken utils
#include <tools_debug/DebugManager.h>
extern Tu::DebugManager g_debugManager;
#include <tools_common\StringManipulation.h>

#include <farm_library/Utility.h>
