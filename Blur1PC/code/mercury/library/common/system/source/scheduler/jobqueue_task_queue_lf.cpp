#include "SystemPch.h"
#include <core/Core.h>

#include "system/scheduler/jobqueue.h"

#if JQ_USER_LF_QUEUE

namespace Scheduler
{

#if PLATFORM_PPU_SPU
#define REORDER_BARRIER() __asm__ volatile ( "":::"memory" )
#elif PLATFORM_XENON
    // With VC++ 2003 you need to declare _ReadWriteBarrier
    extern "C" void _ReadWriteBarrier();
	// Tell the compiler that this is an intrinsic, not a function.
	#pragma intrinsic(_ReadWriteBarrier)
	#define REORDER_BARRIER() _ReadWriteBarrier()
#else
	#define REORDER_BARRIER()
#endif


//! Initializes the queue, either allocating a new free list or sharing with an existing queue object.  
/** 
	This function should only ever be called once, after which the object cannot be moved from its original 
	memory location, except to copy it to the SPU on Playstation3 (from where it will reference its original
	address via the ThisPtr member).
*/
void jqAtomicQueue::Init(jqAtomicQueue* SharedFreeList)
{
	ThisPtr = this;

	// Initialize the free list.
	_FreeList.Val64 = 0;
	if (SharedFreeList)
		FreeListPtr = SharedFreeList->FreeListPtr;
	else
		FreeListPtr = &_FreeList;

	// Initialize the queue to empty.
	NodeType* Node = AllocateNode();
	Node->Next.Ptr = NULL;
	Head.Ptr = Tail.Ptr = Node;
}

//! Pushes a new node onto the front of the queue.  Allocates additional nodes if needed.
void jqAtomicQueue::Push(const Task& Data)
{
	// Allocate and fill the node structure.
	NodeType* Node = AllocateNode();
	REORDER_BARRIER();
	MT::MemoryFence();

	// Using two atomics to fill in the node structure, to avoid trashing the Count field in its Next pointer.
	Spu::Store(&Node->Data, &Data, sizeof(Task));
	Spu::Set((U32*)&Node->Next.Ptr, NULL);

	for (;;)
	{
		REORDER_BARRIER();

		// Get the current pointer to the tail node.
		NodePointerType CurTail(Spu::Get((U64*)&ThisPtr->Tail));

		// Get the tail node's next pointer.  If NULL, the tail node pointer correctly points to the end of the queue.
		// Otherwise, we have to scan the tail forward in order to find the true end.
		NodePointerType Next(Spu::Get((U64*)&CurTail.Ptr->Next));

		REORDER_BARRIER();
		MT::MemoryFence();
		if (CurTail.Val64 == Spu::Get((U64*)&ThisPtr->Tail))
		{
			if (Next.Ptr == NULL)
			{
				// Next was NULL, meaning CurTailPtr was the actual tail and we can attempt to append after CurTailPtr.  
				// If the compare and swap succeeds we have appended, otherwise we'll try again.
				REORDER_BARRIER();
				if (MT::AtomicCompareAndSwap(&CurTail.Ptr->Next.Val64, NodePointerType(Node, Next.Count+1).Val64, Next.Val64))
				{
					// Now that we have added a node successfully, make a single attempt to advance the tail node pointer
					// to point at the new node.
					REORDER_BARRIER();
					MT::AtomicCompareAndSwap(&ThisPtr->Tail.Val64, NodePointerType(Node, CurTail.Count+1).Val64, CurTail.Val64);
					return;
				}
			}
			else
			{
				// Next was not NULL, meaning CurTail has fallen behind the true tail.  Attempt to advance it.
				REORDER_BARRIER();
				MT::AtomicCompareAndSwap(&ThisPtr->Tail.Val64, NodePointerType(Next.Ptr, CurTail.Count+1).Val64, CurTail.Val64);
			}
		}
	}
}

//! Pops one node from the back of the queue.  Returns false if the queue is empty at the time of the pop.
bool jqAtomicQueue::Pop(Task *p)
{
	for (;;)
	{
		REORDER_BARRIER();

		// Get the Head, Tail and Head->Next pointers.
		NodePointerType CurHead(Spu::Get((U64*)&ThisPtr->Head));
		NodePointerType CurTail(Spu::Get((U64*)&ThisPtr->Tail));
		NodePointerType Next(Spu::Get((U64*)&CurHead.Ptr->Next));

		REORDER_BARRIER();
		MT::MemoryFence();
		if (CurHead.Val64 == Spu::Get((U64*)&ThisPtr->Head))
		{
			// If Head is equal to Tail, the queue might be empty or else Tail might have fallen behind the real tail.
			if (CurHead.Ptr == CurTail.Ptr)
			{
				// If the queue really is empty, Head.Next should equal NULL.
				if (Next.Ptr == NULL)
					return false;

				// The Head node had a Next node, which means the Tail pointer must be falling behind.
				// Attempt to advance it by one node.
				REORDER_BARRIER();
				MT::AtomicCompareAndSwap(&ThisPtr->Tail.Val64, NodePointerType(Next.Ptr, CurTail.Count+1).Val64, CurTail.Val64);
			}
			else if (Next.Ptr != NULL)
			{
				// Success- Transfer the node's data into the output pointer and move the old Head node to the free list.
				// Must be read BEFORE the compare and swap, otherwise another Pop might free the node.
				Spu::Fetch(p, &Next.Ptr->Data, sizeof(Task));
				// Attempt to pop the Head node by advancing the Head pointer once.  The fence is here to make sure the
				// data copy happens before the CAS.
				MT::MemoryFence();
				REORDER_BARRIER();
				if (MT::AtomicCompareAndSwap(&ThisPtr->Head.Val64, NodePointerType(Next.Ptr, CurHead.Count+1).Val64, CurHead.Val64))
				{
					REORDER_BARRIER();
					MT::MemoryFence();
					FreeNode(CurHead.Ptr);
					return true;
				}
			}
		}
	}
}

//! Allocates a new queue node from the free list, possibly invoking the system memory allocator to allocate more nodes.
/**
	Internal method.  Note that once allocated, queue nodes will never be freed.  This is to prevent the possibility
	of the lock free counting mechanisms getting out of sync.
*/
jqAtomicQueue::NodeType* jqAtomicQueue::AllocateNode()
{
	for (;;)
	{
		REORDER_BARRIER();
		NodePointerType OldFreeList;
		OldFreeList.Val64 = Spu::Get((U64*)FreeListPtr);

		// If the free list pointer is NULL, attempt to allocate a new block of nodes and try again.
		if (OldFreeList.Ptr == NULL)
		{
			AllocateNodeBlock(QUEUE_REFILL_SIZE);
		}
		else
		{
			// Free list is not empty.  
			// Attempt to advance the free list pointer by one and return the old free list head as the new block.
			NodePointerType OldFreeListNext(Spu::Get((U64*)&OldFreeList.Ptr->Next));
			MT::MemoryFence();
			REORDER_BARRIER();	// ensure the item's next pointer is older than the free list ptr
								// this means that if the free list ptr hasn't changed then the next ptr won't have changed either

			if (MT::AtomicCompareAndSwap(&FreeListPtr->Val64, NodePointerType(OldFreeListNext.Ptr, OldFreeList.Count+1).Val64, OldFreeList.Val64))
				return OldFreeList.Ptr;
		}
	}
}

//! Releases a node to the free list.
/**
	Internal method.  Note that once allocated, queue nodes will never be freed.  This is to prevent the possibility
	of the lock free counting mechanisms getting out of sync.
*/
void jqAtomicQueue::FreeNode(NodeType* Node)
{
	for (;;)
	{
		REORDER_BARRIER();

		// Link the node to the current free list head.
		NodePointerType OldFreeList(Spu::Get((U64*)FreeListPtr));
		Spu::Set((U32*)&Node->Next.Ptr, (U32)OldFreeList.Ptr);
		// Attempt to replace the free list head with this node.
		MT::MemoryFence();
		REORDER_BARRIER();
		if (MT::AtomicCompareAndSwap(&FreeListPtr->Val64, NodePointerType(Node, OldFreeList.Count+1).Val64, OldFreeList.Val64))
			return;
	} 
}

//! Allocates a new block of node objects via the system memory allocator, linking them onto the head of the free list.
/** 
	\warning It is critical that the callback defined for tlMemAlloc be thread safe!  It is preferable that
	it also be lock free.
*/
void jqAtomicQueue::AllocateNodeBlock(int Count)
{
#if TL_PLAT_PS3_SPU
	// SPU does not support allocating from the heap.
	assert(false);
#else
	// Allocate a new block of nodes.
	NodeType* NodeBlock = (NodeType*)BizAlignedMalloc((sizeof(NodeType) * Count), tl_alignof(NodeType));

	// Link the new nodes together.
	for(int i = 0; i < Count - 1; i++)
	{
		NodeBlock[i].Next.Ptr = &NodeBlock[i+1];
	}
	NodeBlock[Count-1].Next.Ptr = NULL;

	// Attempt to install the new block at the head of the free list.  If somebody else got here first and
	//  changed the free list, we have to release our block and start over again (although there will most
	//  likely now be nodes available, at least).
	NodePointerType OldFreeList(Spu::Get((U64*)FreeListPtr));
	OldFreeList.Ptr = NULL; // make sure the list is still empty, to avoid a memory leaks

	MT::MemoryFence();
	REORDER_BARRIER();
	if (!MT::AtomicCompareAndSwap(&FreeListPtr->Val64, NodePointerType(NodeBlock, OldFreeList.Count+1).Val64, OldFreeList.Val64))
	{
		REORDER_BARRIER();
		BizAlignedFree(NodeBlock);
	}
#endif
	REORDER_BARRIER();
}


} // namespace Scheduler

#endif