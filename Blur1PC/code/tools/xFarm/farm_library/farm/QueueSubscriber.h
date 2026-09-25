#ifndef __QUEUESUBSCRIBER_H__
#define __QUEUESUBSCRIBER_H__

namespace MT
{
	class Mutex;
}

// log subscriber which puts log events in a thread-safe FIFO queue
class QueueSubscriber : public Debug::I_LogSubscriber
{
public:
							/// constructor
							QueueSubscriber();
							/// destructor
							~QueueSubscriber();

	// implementation of the subscriber interface

							/// send a log event to this subscriber
	void					Log(Debug::LogEvent *pEvent);

							/// parse a configuration parameter for this subscriber
	void					SetXMLParameter(const char *pName, const char *pValue);

							/// this function is called after SetXMLParameter, but before any events are logger
	bool					 Start();

	// FIFO queue implementation
							/// pop the first event from the queue, return false if the queue is empty
	bool					PopFront(Debug::LogEvent *pOut);

							/// add an event to the back of the queue
	void					PushBack(const Debug::LogEvent &event);

private:
							/// re-allocate the queue and move all the items to the front
	void					ReallocateQueue();

	Debug::LogEvent			*m_pQueue;
	int						m_iQueueFront;
	int						m_iQueueBack;
	int						m_iQueueItems;
	int						m_iQueueCapacity;

	MT::Mutex				*m_pLock;
};

#endif
