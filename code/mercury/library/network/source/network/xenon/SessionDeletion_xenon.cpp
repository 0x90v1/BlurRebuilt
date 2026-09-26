#include <StdAfx.h>
#include <Network\SessionDeletion.h>
#include <Network\Securekey.h>
#include <Network\Session.h>
#include "AsyncTask_xenon.h"
#include "SessionCommand_xenon.h"  

namespace NetPlay
{
	// static instance..
	C_SessionDeletion C_SessionDeletion::s_SessionDeletion;
  
	C_SessionDeletion::C_SessionDeletion() : m_Initialised(false), m_pDeleteCommandStorage(NULL), m_pDeleteCommandPool(NULL), m_pDeleteCommandList(NULL)
	{
	}

	C_SessionDeletion::~C_SessionDeletion()
	{
		Cleanup();
	}

	bool C_SessionDeletion::Initialise(const U32 deleteQueueSize)
	{
		if (m_Initialised)
			return true;

		assert(deleteQueueSize);
		if (0 == deleteQueueSize)
			return false;

		m_pDeleteCommandStorage = new C_SessionDeletionCommand[deleteQueueSize];
		m_pDeleteCommandPool = reinterpret_cast<void*> (new C_SessionDeletionCommandList);
		m_pDeleteCommandList = reinterpret_cast<void*> (new C_SessionDeletionCommandList);

		if (m_pDeleteCommandStorage && m_pDeleteCommandPool && m_pDeleteCommandList)
		{
			// put all delete commands into pool
			C_SessionDeletionCommandList* pPool = reinterpret_cast<C_SessionDeletionCommandList*> (m_pDeleteCommandPool);
			for (U32 i=0; i< deleteQueueSize; i++)
				pPool->PushBack(m_pDeleteCommandStorage[i]);

			m_Initialised = true;
		}
		else
			Cleanup();

		return m_Initialised;
	}
	
	void C_SessionDeletion::Cleanup(void)
	{
		// block until alll sessions have been deleted.
		while(ActiveCount())
			Poll();

		// remove any items from the delete command list
		if (m_pDeleteCommandList)
		{
			C_SessionDeletionCommandList* p = reinterpret_cast<C_SessionDeletionCommandList*> (m_pDeleteCommandList);
			if (!p->Empty())
			{
				// block and wait for all  deletion tasks to complete before closing the session handles..
				LOGWARN( g_Log, "C_Session::CleanUp() - Closing down when delete commands active, Blocking until all session deletion tasks complete.");
				C_SessionDeletionCommandList::Iterator i = p->Begin();
				while (!p->Empty())
				{
					for (i=p->Begin(); i!=p->End();)
					{
						if (i->m_AsyncTask.Finished())
						{
							i->m_AsyncTask.Close();	// stop and close asynchronous task
							CloseHandle(i->m_CommandData.m_Delete.m_SessionHandle); // release handle to give resources back to OS
							i = p->Erase(i); // erase item from list and move to next, no point adding it to the free list as that will be blitzed next.
						}
						else
							++i;
					}
				}
			}
			p->Clear();
			delete p;
			m_pDeleteCommandList = NULL;
		}

		// remove any items from the delete command pool
		if (m_pDeleteCommandPool)
		{
			C_SessionDeletionCommandList* p = reinterpret_cast<C_SessionDeletionCommandList*> (m_pDeleteCommandPool);
			p->Clear();
			delete p;
			m_pDeleteCommandPool = NULL;
		}

		// release delete command storage..
		if (m_pDeleteCommandStorage)
		{
			delete [] m_pDeleteCommandStorage;
			m_pDeleteCommandStorage = NULL;
		}

		m_Initialised = false;
	}

	void C_SessionDeletion::Poll(void)
	{
		if (!m_Initialised)
			return;

		C_SessionDeletionCommandList* pDeleteList = reinterpret_cast<C_SessionDeletionCommandList*> (m_pDeleteCommandList);
		C_SessionDeletionCommandList* pDeletePool = reinterpret_cast<C_SessionDeletionCommandList*> (m_pDeleteCommandPool);
		if (!pDeleteList->Empty())
		{
			C_SessionDeletionCommandList::Iterator i = pDeleteList->Begin();
			if (i->m_Active)
			{
				if (i->m_AsyncTask.Finished())
				{
					LOGINFO( g_Log, "C_SessionDeletion : Delete successful");
					CloseHandle(i->m_CommandData.m_Delete.m_SessionHandle);
					i->m_CommandData.m_Delete.m_SessionHandle = NULL;
					i->m_AsyncTask.Close(); 

					// erase item from list and give back to pool..
					C_SessionDeletionCommandList::Iterator cur = i;
					i = pDeleteList->Erase(cur);
					pDeletePool->PushBack(*cur);
				}
				else
				{
					return;
				}
			}
			
			if (pDeleteList->Empty())
				return;
			
			// start deleting first session in list.
			i = pDeleteList->Begin();
			if (XSessionDelete(i->m_CommandData.m_Delete.m_SessionHandle, &i->m_AsyncTask) == ERROR_IO_PENDING)
				i->m_Active = true;
			else
			{
				LOGINFO( g_Log, "C_SessionDeletion : XSessionDelete() failed");
				CloseHandle(i->m_CommandData.m_Delete.m_SessionHandle);

				// erase item from list and give back to pool..
				C_SessionDeletionCommandList::Iterator cur = i;
				i = pDeleteList->Erase(cur);
				pDeletePool->PushBack(*cur);
			}
		}
	}

	U32 C_SessionDeletion::ActiveCount(void)
	{
		U32 active = 0;
		if (m_Initialised)
		{
			C_SessionDeletionCommandList* pDeleteList = reinterpret_cast<C_SessionDeletionCommandList*> (m_pDeleteCommandList);
			active = pDeleteList->Size();
		}
		return active;
	}

	U32 C_SessionDeletion::Remaining(void)
	{
		U32 remaining = 0;
		if (m_Initialised)
		{
			C_SessionDeletionCommandList* pDeletePool = reinterpret_cast<C_SessionDeletionCommandList*> (m_pDeleteCommandPool);
			remaining = pDeletePool->Size();
		}
		return remaining;
	}

	bool C_SessionDeletion::Add(C_Session& toDelete)
	{
		if (m_Initialised)
		{
			if (Remaining() == 0)
			{
				LOGFATAL(g_Log, "C_SesionDeletion::Add()  - Not enough storage allocated to add session to deletion queue.");
				return false;
			}

			C_SessionDeletionCommandList* pList = reinterpret_cast<C_SessionDeletionCommandList*> (m_pDeleteCommandList);
			C_SessionDeletionCommandList* pPool = reinterpret_cast<C_SessionDeletionCommandList*> (m_pDeleteCommandPool);

			C_SessionDeletionCommandList::Iterator i = pPool->Begin();
			pPool->Erase(pPool->Begin());
			i->m_Command								=	C_SessionDeletionCommand::eCommandDeleteSession;
			i->m_CommandData.m_Delete.m_SessionHandle	=	toDelete.m_SessionHandle;

			// add to list for asynchronous processing.
			pList->PushBack(*i);
			return true;
		}

		return false;
	}
}
