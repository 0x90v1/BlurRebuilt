#pragma once

#include "Network.h"
#include "WndCore.h"

#include "FarmProcess.h"
#include "FarmServer.h"
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

// screensaver farm network port
const int	NetworkPort		= 8769;
const int	BroadcastPort	= 8769;

extern bool gbJobRunning, gbKillWorker;
extern HANDLE ghKillWorkerEvent;