#pragma once

#include "Network.h"
#include "WndCore.h"

#ifndef INFINITE
#define INFINITE 0xFFFFFFFF  // Infinite timeout
#endif

#include <core\containers\string.h>

#include "FarmJobFlags.h"
#include "FarmProcess.h"
#include "FarmFeedback.h"
#include "FarmServer.h"
#include "DummyFarmServer.h"
#include "FarmStranger.h"
#include "FarmAssistant.h"
#include "FarmManager.h"
#include "FarmWorker.h"
#include "FarmJob.h"
#include "FarmKill.h"
#include "FarmList.h"
#include "FarmPriority.h"
#include "FarmRequeue.h"
#include "FarmCommand.h"
#include "FarmComment.h"
#include "FarmUser.h"
#include "FarmUi.h"
#include "SpawnProcess.h"
#include "Common.h"

//#ifdef _DEBUG
//const int	NetworkPort		= 8765 - 2;
//const int	BroadcastPort	= 8766 - 2;
//#else
//const int	NetworkPort		= 8765;
//const int	BroadcastPort	= 8766;
//#endif
class SQLAccessInterface;

bool ExecuteStranger(U16 broadcastPort, U16 networkPort, FarmStranger* Stranger, unsigned int timeout, bool bDeleteStranger);
bool ExecuteServer(U16 broadcastPort, U16 networkPort, bool bScreensaverMode, bool bDebugMode, bool bBroadcast, SQLAccessInterface *pSQLAccess);
bool ExecuteWorker(U16 broadcastPort, U16 networkPort);

void ExecuteUi(bool IsManager, U16 broadcastPort, U16 networkPort);

