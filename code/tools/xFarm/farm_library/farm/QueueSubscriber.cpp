#include "StdAfx.h"
#include "system\debuglog.h"
#include <system/debug/log/DebugLog.h>
#include "core\threads.h"
#include "QueueSubscriber.h"

/// constructor
QueueSubscriber::QueueSubscriber() :
	m_iQueueFront(0),
	m_iQueueBack(0),
	m_iQueueItems(0),
	m_iQueueCapacity(32)
{
	m_pQueue = new Debug::LogEvent[m_iQueueCapacity];
	m_pLock = new MT::Mutex();
}

/// destructor
QueueSubscriber::~QueueSubscriber()
{
	delete [] m_pQueue;
	delete m_pLock;
}

/// receive a log event
void QueueSubscriber::Log(Debug::LogEvent *pEvent)
{
	PushBack(*pEvent);
}

/// parse a configuration parameter for this subscriber
void QueueSubscriber::SetXMLParameter(const char *pName, const char *pValue)
{
}

/// this function is called after SetXMLParameter, but before any events are logger
bool QueueSubscriber::Start()
{
	return true;
}

// FIFO queue implementation

/// pop the first event from the queue
bool QueueSubscriber::PopFront(Debug::LogEvent *pOut)
{
	m_pLock->Wait();

	if (m_iQueueBack==m_iQueueFront)
	{
		m_pLock->Release();
		return false;
	}

	if (m_iQueueItems<=0)
	{
		// this shouldn't happen, but just in case
		m_iQueueBack = m_iQueueFront = 0;
		return false;
	}

	*pOut = m_pQueue[m_iQueueFront];
	m_iQueueFront++;
	m_iQueueItems--;

	m_pLock->Release();

	return true;
}

/// add an event to the back of the queue
void QueueSubscriber::PushBack(const Debug::LogEvent &event)
{
	m_pLock->Wait();

	// if the queue is empty, reset the front/back indices to 0
	if (m_iQueueBack == m_iQueueFront)
		m_iQueueBack = m_iQueueFront = 0;

	if (m_iQueueItems >= m_iQueueCapacity)
	{
		m_iQueueCapacity *= 2;
		ReallocateQueue();
	}

	if (m_iQueueBack >= m_iQueueCapacity)
	{
		ReallocateQueue();
	}

	assert(m_iQueueBack < m_iQueueCapacity);

	m_pQueue[m_iQueueBack] = event;
	m_iQueueBack++;
	m_iQueueItems++;

	m_pLock->Release();
}

/// re-allocate the queue and move all the items to the front
void QueueSubscriber::ReallocateQueue()
{
	// allocate new queue
	Debug::LogEvent *pNew = new Debug::LogEvent[m_iQueueCapacity];

	// copy events to the front of the new queue
	for (int i=0; i<m_iQueueBack-m_iQueueFront; i++)
		pNew[i] = m_pQueue[i+m_iQueueFront];

	m_iQueueBack -= m_iQueueFront;
	m_iQueueFront = 0;

	// delete the buffer with the old events
	delete [] m_pQueue;
	m_pQueue = pNew;
}

