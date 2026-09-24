#include "SystemPch.h"
#include "IoImpl.h"
#include <system/serialise/FsPerfLogger.h>
#include <system/serialise/FileSystemGlobals.h>

extern Debug::Logger &g_SystemIoLog;

Debug::Bool gSystemIoSimulateReadErrors("System.Io.SimulateReadErrors", false);
Debug::Int gSystemIoSimulateReadErrorsAfter("System.Io.SimulateReadErrorsAfter", -1);

bool IoDebugShouldReadFail()
{
#ifndef CFG_GOLD
	static int readCount = 0;

	readCount++;

	if (gSystemIoSimulateReadErrors)
		return true;

	if ((gSystemIoSimulateReadErrorsAfter >= 0) && (readCount > gSystemIoSimulateReadErrorsAfter))
		return true;
#endif

	return false;
}

#if defined(PLATFORM_XENON)

#include <xfilecache.h>
#include <System/Debug/BuildStamp.h>
#include <Btl/Utility/Hash.h>

bool gIoFileCachingSuspended = false;

DWORD IoFileCacheCallback(void* context, const char* path, LARGE_INTEGER* pFileOffset, DWORD nNumberOfBytesToRead)
{
	if (gIoFileCachingSuspended)
		return XFILECACHE_READFILEADVISORY_HOLDOFF(500);

	return XFILECACHE_READFILEADVISORY_ALLOW(500);
}

void IoSuspendFileCaching()
{
	gIoFileCachingSuspended = true;
}

void IoResumeFileCaching()
{
	gIoFileCachingSuspended = false;
}

void IoGetCacheStats(IoCacheStats* pStats)
{
#ifndef CFG_GOLD
	XFILECACHESTATS stats;
	if (XFileCacheGetStats(&stats) == ERROR_SUCCESS)
	{
		pStats->cachedTotal = stats.CachedTotal;
		pStats->fileCount = stats.FileCount;
		pStats->cacheHitOpens = stats.CacheHitOpens;
		pStats->cacheMissOpens = stats.CacheMissOpens;
		pStats->cacheHitReads = stats.CacheHitReads;
		pStats->cacheMissReads = stats.CacheMissReads;
		pStats->cacheHitBytesRequested = (S64)stats.CacheHitBytesRequested.QuadPart;
		pStats->cacheMissBytesRequested = (S64)stats.CacheMissBytesRequested.QuadPart;
		pStats->cacheCopyBytes = (S64)stats.CacheCopyBytes.QuadPart;
		pStats->cacheEvictBytes = (S64)stats.CacheEvictBytes.QuadPart;
	}
	else
#endif
	{
		memset(pStats, 0, sizeof(IoCacheStats));
	}
}

#elif defined(PLATFORM_PS3)

#include <sysutil/sysutil_syscache.h>
#include <sys/return_code.h>
#include <sys/timer.h>

Btl::String gIoTitleIdPs3;

class IoServiceFiosAllocator : public cell::fios::allocator
{
public:
	void* Allocate(uint32_t size, uint32_t flags, const char* /*pFile*/, int /*line*/)
	{
		return memalign(FIOS_ALIGNMENT_FROM_MEMFLAGS(flags), size);
	}

	void Deallocate(void* pMemory, uint32_t /*flags*/, const char* /*pFile*/, int /*line*/)
	{
		free(pMemory);
	}

	void* Reallocate(void* /*pMemory*/, uint32_t /*newSize*/, uint32_t /*flags*/, const char* /*pFile*/, int /*line*/)
	{
		return 0;
	}
};

void IoSuspendFileCaching() { }
void IoResumeFileCaching() { }
void IoGetCacheStats(IoCacheStats* pStats) { memset(pStats, 0, sizeof(IoCacheStats)); }

#elif defined(PLATFORM_PC)

void IoSuspendFileCaching() { }
void IoResumeFileCaching() { }
void IoGetCacheStats(IoCacheStats* pStats) { memset(pStats, 0, sizeof(IoCacheStats)); }
#endif

//
// IoService
//

IoService::IoService() :
	_initialised(false),
	_dvd(0),
	_hdd(0)
#ifdef PLATFORM_XENON
	,_cachePriorityList(NULL)
	,_cachePriorityListCount(0)
#endif
#ifdef PLATFORM_PS3
	,_appHomeMedia(0)
	,_dvdMedia(0)
	,_hddMedia(0)
	,_overlay(0)
	,_scheduler(0)
#endif
{
}

IoService::~IoService()
{
}

bool IoService::Initialise()
{
	if (_initialised)
		return true;

	_initialised = true;

#if defined(PLATFORM_XENON)

	U64 cacheSize = (1024LL + 512LL) * 1024LL * 1024LL;

#ifdef BUILD_CONFIG_MASTER
	bool shouldCache = true;
#else
	bool shouldCache = false; // TODO: File caching is knocked out file by default in development

	char fileSystemName[32];

	if (GetVolumeInformation("GAME:\\", 0, 0, 0, 0, 0, fileSystemName, lengthof(fileSystemName)))
	{
		if (!stricmp(fileSystemName, "GDFX"))
		{
			shouldCache = true;
		}
	}

#endif

	//HDD Install detection

	//Method 1:
	//Although the code below seems to work, the "Symbolic link root name "game" is not an expected content package device!"
	//Error scares me. That and the fact that I'm having to do an async call and wait, as the syncronous version of 
	//XContentQueryVolumeDeviceType doesn't actually work in this case, as confirmed as a bug by the chaps at dev-support.
/*
	XCONTENTDEVICETYPE deviceType = XCONTENTDEVICETYPE_ODD;
	HANDLE xContentQueryHandle = CreateEvent( NULL, FALSE, FALSE, NULL );
	if (xContentQueryHandle)
	{
		XOVERLAPPED overlapped;
		ZeroMemory(&overlapped, sizeof(XOVERLAPPED));
		overlapped.hEvent = xContentQueryHandle;
		
		deviceType = XCONTENTDEVICETYPE_ODD;
		DWORD returnValue = XContentQueryVolumeDeviceType("game", &deviceType, &overlapped);
		returnValue = XGetOverlappedResult(&overlapped, NULL, TRUE);

		CloseHandle(xContentQueryHandle);
		if (deviceType == XCONTENTDEVICETYPE_HDD)
			shouldCache = false;
	}
*/
	//Method 2: Attempt to acquire a content license. This was a method advocated by the gameds@microsoft.com chaps.
	DWORD licenseMask;
	DWORD result = XContentGetLicenseMask(
         &licenseMask,
         NULL);
	if (result == ERROR_SUCCESS)
	{
#ifndef CFG_GOLD
		printf("Detected hard drive install!\n");
#endif
		shouldCache = false;
	}
	else
	{
#ifndef CFG_GOLD
		printf("Not running off hard drive install.\n");
#endif
	}

#ifndef CFG_GOLD
	// Check the command line to see if we should disable the caching
	const char* commandLine = GetCommandLine();

	if (strstr(commandLine, "-nocache"))
	{
		shouldCache = false;
	}
	else if (strstr(commandLine, "-cache"))
	{
		shouldCache = true;
	}

	if (shouldCache)
		printf("Transparent caching enabled.\n");
	else
		printf("Transparent caching disabled.\n");
#endif

	if (shouldCache)
	{
		DWORD flags = XFILECACHE_OPPORTUNISTIC_ON | XFILECACHE_PRESERVE;

		for(int i = 0; i < 4; i++)
		{
			XINPUT_STATE is;

			if (XInputGetState(i, &is) == ERROR_SUCCESS)
			{
				if ((is.Gamepad.bLeftTrigger > 127) && (is.Gamepad.bRightTrigger > 127))
					flags |= XFILECACHE_CLEAR_ALL;
			}
		}

		XFileCacheInit(flags, cacheSize, XFILECACHE_DEFAULT_THREAD, 0, Btl::Hash(Debug::GetBuildTime()));

		if (_cachePriorityList)
		{
			XFileCachePreloadFiles(XFILECACHE_STARTUP_FILES, _cachePriorityList, _cachePriorityListCount);
		}
		
		const char* dontCacheList[] = { "GAME:\\Cache\\root_streaming.pak", "GAME:\\Ui\\", "GAME:\\layer1\\Cache\\root_streaming.pak" };
		U32 nDontCacheFiles = lengthof(dontCacheList);
		XFileCachePreload(XFILECACHE_STARTUP_FILES, "GAME:\\Cache\\*");
		XFileCachePreload(XFILECACHE_STARTUP_FILES, "GAME:\\Layer1\\Cache\\*");
		XFileCacheControlFiles(XFILECACHE_DONTCACHE, dontCacheList, nDontCacheFiles);
		XFileCacheSetFileIoCallbacks(IoFileCacheCallback, this);
		XFileCacheControl(XFILECACHE_BACKGROUND_ON);
	}

#elif defined(PLATFORM_PS3)
	cell::fios::allocator* allocator = new IoServiceFiosAllocator;

	cell::fios::fios_parameters fp = FIOS_PARAMETERS_INITIALIZER;

	fp.pAllocator = allocator;

	cell::fios::FIOSInit(&fp);

	_appHomeMedia	= new cell::fios::ps3media(SYS_APP_HOME);
	_dvdMedia		= new cell::fios::ps3media(SYS_DEV_BDVD);
	_hddMedia		= new cell::fios::ps3media(SYS_DEV_HDD0);

	_overlay = new cell::fios::overlay(_dvdMedia);

	cell::fios::overlay::id_t id;

	_overlay->addLayer(SYS_APP_HOME "/", 0, _appHomeMedia, "/", cell::fios::overlay::kOPAQUE, &id);
	_overlay->addLayer(SYS_DEV_BDVD "/", 0, _dvdMedia, "/", cell::fios::overlay::kOPAQUE, &id);
	_overlay->addLayer(SYS_DEV_HDD0 "/", 0, _hddMedia, "/", cell::fios::overlay::kOPAQUE, &id);

	_scheduler = cell::fios::scheduler::createSchedulerForMedia(_overlay);

	cell::fios::scheduler::setDefaultScheduler(_scheduler);
#endif

	_dvd = CreateDevice("dvd");
	_hdd = CreateDevice("hdd");

	if (!_dvd || !_hdd || !InitialiseDevices())
	{
		Shutdown();
		return false;
	}

#if defined(PLATFORM_PS3)
	_dvd->_scheduler = _scheduler;
	_hdd->_scheduler = _scheduler;
#endif

	return true;
}

void IoService::CachePrioritise(const char** fileList, int numFiles)
{
#ifdef PLATFORM_XENON
	_cachePriorityList = fileList;
	_cachePriorityListCount = numFiles;
#endif
}

#if defined(PLATFORM_PS3)
void ShutdownOps(void *context, cell::fios::op *pOp)
{
	cell::fios::scheduler* scheduler = (cell::fios::scheduler*)context;
	pOp->cancel();
	scheduler->deleteOp(pOp);
}
#endif

void IoService::Shutdown()
{
	if (!_initialised)
		return;

	for(XtUt::List<IoDevice>::Iterator it(&_devices); it; ++it)
		it->Shutdown();

	XtUt::Free(&_devices);

#if defined(PLATFORM_PS3)

	cell::fios::scheduler::setDefaultScheduler(_scheduler);


	if (_scheduler)
	{
		_scheduler->suspend();

//		_scheduler->cancelAllOps();
	
//		_scheduler->iterateOps( ShutdownOps, (void *)_scheduler );
		
//		_scheduler->closeAllFiles();

//		_scheduler->shutdownAndCancelOps();
		
		
	}
	


	//cell::fios::scheduler::destroyScheduler(_scheduler);

	//delete _overlay;
	//delete _hddMedia;
	//delete _dvdMedia;
	//delete _appHomeMedia;

	//_scheduler = 0;
	//_overlay = 0;
	//_hddMedia = 0;
	//_dvdMedia = 0;
	//_appHomeMedia = 0;

	//cell::fios::FIOSTerminate();
#endif

	_initialised = false;
}

IoRequest IoService::Open(IoFile& file, const char* filename, IoCallback* callback)
{
	file = IoFile();

	IoRequestImpl* reason = &gIoRequest_InvalidParameters;

	if (filename && *filename)
	{
		if (IoDevice* device = GetDeviceByPath(filename))
		{
			if (IoFileImpl* impl = new IoFileImpl)
			{
				impl->filename	= filename;
				impl->device	= device;

				if (IoRequestImpl* request = new IoRequestImpl(kIoAction_Open, impl->self, callback))
				{
					IoRequest keepAlive = request->self;

					file = impl->self;

#if defined(PLATFORM_XENON) || defined(PLATFORM_PC)
					device->Queue(request);
#elif defined(PLATFORM_PS3)
					cell::fios::opattr_t attr = FIOS_OPATTR_INITIALIZER;

					attr.pCallback			= OpCallbackOpen;
					attr.pCallbackContext	= request;
					request->op = device->_scheduler->openFile(&attr, impl->filename.Chars(), cell::fios::kO_RDONLY, &impl->handle);

#endif
					return request->self;
				}

				impl->self = IoFile();
			}

			reason = &gIoRequest_OutOfMemory;
		}
		else
			reason = &gIoRequest_NoDeviceMapping;
	}

	if (callback)
	{
		callback->Event(kIoEvent_Failed, reason->self);
		callback->Release();
	}

	return reason->self;
}

IoRequest IoService::Read(const IoFile& file, void* buffer, U64 offset, U32 size, IoPriority pri, IoCallback* callback)
{
	IoFileImpl*		fi		= file._impl;
	IoRequestImpl*	reason	= &gIoRequest_InvalidParameters;

	if (fi && buffer && size)
	{
		if (fi->state == IoFileImpl::kState_Open)
		{
			if (IoRequestImpl* request = new IoRequestImpl(kIoAction_Read, file, callback))
			{
				IoRequest keepAlive = request->self;

				request->buffer			= buffer;
				request->offset			= offset;
				request->requestSize	= size;
				request->priority		= pri;

#if defined(PLATFORM_XENON) || defined(PLATFORM_PC)
				fi->device->Queue(request);
#elif defined(PLATFORM_PS3)
				cell::fios::opattr_t attr = FIOS_OPATTR_INITIALIZER;

				attr.pCallback			= OpCallbackRead;
				attr.pCallbackContext	= request;
				attr.priority			= cell::fios::kPRIO_DEFAULT;
				attr.deadline			= FIOSGetCurrentTime();

				switch(pri)
				{
					case kIoPriority_High:	attr.priority += 1;	attr.deadline += FIOSMillisecondsToAbstime(500);	break;
					case kIoPriority_Low:	attr.priority -= 1;	attr.deadline += FIOSMillisecondsToAbstime(8000);	break;

					default:
					case kIoPriority_Medium:
						attr.deadline += FIOSMillisecondsToAbstime(2000);
					break;
				}

	
				request->op = fi->device->_scheduler->readFile(&attr, fi->handle, buffer, size, offset);
#endif

				return request->self;
			}

			reason = &gIoRequest_OutOfMemory;
		}
		else
		{
			reason = &gIoRequest_FileNotYetOpen;
		}
	}

	if (callback)
	{
		callback->Event(kIoEvent_Failed, reason->self);
		callback->Release();
	}

	return reason->self;
}

IoRequest IoService::Close(const IoFile& file, IoCallback* callback)
{
	IoFileImpl*		fi		= file._impl;
	IoRequestImpl*	reason	= &gIoRequest_InvalidParameters;

	if (fi)
	{
		// TODO: Take device mutex to prevent race with state?

		if (fi->state == IoFileImpl::kState_Open)
		{
			if (IoRequestImpl* request = new IoRequestImpl(kIoAction_Close, file, callback))
			{
				IoRequest keepAlive = request->self;

				fi->state = IoFileImpl::kState_Closing;

#if defined(PLATFORM_XENON) || defined(PLATFORM_PC)
				fi->device->Queue(request);
#elif defined(PLATFORM_PS3)
				cell::fios::opattr_t attr = FIOS_OPATTR_INITIALIZER;

				attr.pCallback			= OpCallbackClose;
				attr.pCallbackContext	= request;

				request->op = fi->device->_scheduler->closeFile(&attr, fi->handle);
#endif

				return request->self;
			}

			reason = &gIoRequest_OutOfMemory;
		}
		else
		{
			reason = &gIoRequest_FileNotYetOpen;
		}
	}

	if (callback)
	{
		callback->Event(kIoEvent_Failed, reason->self);
		callback->Release();
	}

	return reason->self;
}

IoDevice* IoService::GetDeviceByPath(const char* path)
{
	// TODO: Need to check to see if cached here and use hdd device if so...  Also, memory card device?

#if defined(PLATFORM_XENON)
	if (strnicmp(path, "GAME:", 5) == 0)
		return _dvd;
#elif defined(PLATFORM_PS3)
	if (strnicmp(path, "/dev_bdvd/", 10) == 0)
		return _dvd;
#endif

	return _hdd;
}

IoDevice* IoService::CreateDevice(const char* name)
{
	return XtUt::AppendEx(&_devices, new IoDevice(name));
}

bool IoService::InitialiseDevices()
{
	for(XtUt::List<IoDevice>::Iterator it(&_devices); it; ++it)
	{
		if (!it->Initialise())
			return false;
	}

	return true;
}

#ifdef PLATFORM_PS3
cell::fios::err_t IoService::OpCallbackOpen(void* context, cell::fios::op* op, cell::fios::opevent_t event, void* param)
{
	IoRequestImpl* impl = (IoRequestImpl*)context;

	switch(event)
	{
		case cell::fios::kOPEVENT_COMPLETED:
		{
			cell::fios::err_t err = op->wait();

			switch(err)
			{
				case cell::fios::CELL_FIOS_NOERROR:			impl->error = kIoError_Okay;				break;
				case cell::fios::CELL_FIOS_ERROR_BADPATH:	impl->error = kIoError_FileNotFound;		break;
				case EMFILE:								impl->error = kIoError_TooManyFilesOpen;	break;
				default:									impl->error = kIoError_Unknown;				break;
			}

			IoFileImpl*	fi		= impl->file._impl;
			IoDevice*	device	= fi->device;

			fi->state		= IoFileImpl::kState_Open;
			fi->fileSize	= fi->handle ? fi->handle->getFileSize() : 0;
			if (impl->error == kIoError_Okay)
			{
				FSPERF_OPENED("IoDevice", fi->filename.Chars(), impl->file.Handle(), Sys::E_FileAccess_Read);
			}
			else
			{
				if (impl->error == kIoError_FileNotFound)
				{
					FSERRORSPAM(g_SystemIoLog, "IoService::OpCallbackOpen, %s, fiosErr %i", impl->file._impl->filename.Chars(), err);
				}
				else
				{
					FSERROR(g_SystemIoLog, "IoService::OpCallbackOpen, %s, fiosErr %i", impl->file._impl->filename.Chars(), err);
				}
			}
			impl->DoUserCallback((impl->error == kIoError_Okay) ? kIoEvent_Complete : kIoEvent_Failed);

			device->_scheduler->deleteOp(op);
		}
		break;

		case cell::fios::kOPEVENT_DELETE:
			impl->DoUserRelease();

			impl->op	= 0;
			impl->self	= IoRequest();
		break;
	}

	return cell::fios::CELL_FIOS_NOERROR;
}

cell::fios::err_t IoService::OpCallbackRead(void* context, cell::fios::op* op, cell::fios::opevent_t event, void* param)
{
	IoRequestImpl* impl = (IoRequestImpl*)context;

	switch(event)
	{
		case cell::fios::kOPEVENT_COMPLETED:
		{
			cell::fios::err_t err = op->wait();

			switch(err)
			{
				case cell::fios::CELL_FIOS_NOERROR:		impl->error = kIoError_Okay;		break;
				case cell::fios::CELL_FIOS_ERROR_EOF:	impl->error = kIoError_EndOfFile;	break;
				default:								impl->error = kIoError_Unknown;		break;
			}

			impl->actualSize = op->getFulfilledSize();

			if ((impl->error == kIoError_EndOfFile) && (impl->actualSize > 0))
				impl->error = kIoError_Okay;

			if (impl->error != kIoError_Okay)
			{
				FSERROR(g_SystemIoLog, "IoService::OpCallbackRead, %s, fiosErr %i", impl->file._impl->filename.Chars(), err);
			}

			if (IoDebugShouldReadFail())
			{
				impl->error = kIoError_Unknown;
				memset(impl->buffer, 0xCD, impl->actualSize);
			}

			impl->DoUserCallback((impl->error == kIoError_Okay) ? kIoEvent_Complete : kIoEvent_Failed);

			impl->file._impl->device->_scheduler->deleteOp(op);
		}
		break;

		case cell::fios::kOPEVENT_DELETE:
			impl->DoUserRelease();

			impl->op	= 0;
			impl->self	= IoRequest();
		break;
	}

	return cell::fios::CELL_FIOS_NOERROR;
}

cell::fios::err_t IoService::OpCallbackClose(void* context, cell::fios::op* op, cell::fios::opevent_t event, void* param)
{
	IoRequestImpl* impl = (IoRequestImpl*)context;

	switch(event)
	{
		case cell::fios::kOPEVENT_COMPLETED:
		{
			cell::fios::err_t err = op->wait();

			switch(err)
			{
				case cell::fios::CELL_FIOS_NOERROR:	impl->error = kIoError_Okay;	break;
				default:							impl->error = kIoError_Unknown;	break;
			}

			IoFileImpl*	fi		= impl->file._impl;
			if (fi->state != IoFileImpl::kState_Closed)
			{	
				FSPERF_CLOSED("IoDevice", impl->file.Handle());
			}
			if (impl->error != kIoError_Okay)
			{
				FSERRORSPAM(g_SystemIoLog, "IoService::OpCallbackClose, %s, fiosErr %i", impl->file._impl->filename.Chars(), err);
			}
			impl->DoUserCallback((impl->error == kIoError_Okay) ? kIoEvent_Complete : kIoEvent_Failed);

			IoDevice*	device	= fi->device;

			fi->state	= IoFileImpl::kState_Closed;
			fi->self	= IoFile();

			device->_scheduler->deleteOp(op);
		}
		break;

		case cell::fios::kOPEVENT_DELETE:
			impl->DoUserRelease();

			impl->op	= 0;
			impl->self	= IoRequest();
		break;
	}

	return cell::fios::CELL_FIOS_NOERROR;
}
#endif

IoService gIoService;

//
// Interface
//

bool IoInitialise()
{
	return gIoService.Initialise();
}

void IoShutdown()
{
	gIoService.Shutdown();
}

void IoCachePrioritise(const char** fileList, int numFiles)
{
	gIoService.CachePrioritise(fileList, numFiles);
}

IoRequest IoFileOpen(IoFile& file, const char* filename, IoCallback* callback)
{
	return gIoService.Open(file, filename, callback);
}

IoRequest IoFileRead(const IoFile& file, void* buffer, U64 offset, U32 size, IoPriority pri, IoCallback* callback)
{
	return gIoService.Read(file, buffer, offset, size, pri, callback);
}

IoRequest IoFileClose(const IoFile& file, IoCallback* callback)
{
	return gIoService.Close(file, callback);
}

#ifdef PLATFORM_PS3
void IoSetTitleIdPs3(const char* titleId)
{
	gIoTitleIdPs3 = titleId;
}
#endif
