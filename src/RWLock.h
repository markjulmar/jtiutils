/****************************************************************************/
//
// RWLock.h
//
// This header describes a multiple reader single writer locking class
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

#ifndef __JTI_RWLOCK_H_INCLUDED_
#define __JTI_RWLOCK_H_INCLUDED_

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#ifndef JTI_ASSERT
#include <assert.h>
#define JTI_ASSERT assert
#define DEFINED_JTI_ASSERT
#endif
#define _JTI_NO_LINK_RECORD
#include <JTIUtils.h>
#undef _JTI_NO_LINK_RECORD
#include <Lock.h>
#include <Synchronization.h>

/*****************************************************************************/
// PC-Lint options
//
//lint -save
//
// Not checking for assignment to this
//lint -esym(1529, MRSWLock::operator=)
//
/*****************************************************************************/
#pragma warning(disable:4127)

namespace JTI_Util
{
/******************************************************************************/
// MRSWLock
//
// This implements a multiple-reader single writer locking object which can be 
// used to protect a resource where read attempts are much more common than 
// write attempts.
//
/******************************************************************************/
class MRSWLock : public LockableObject<MultiThreadModel>
{
// Internal structures
private:
	// Lock State constants
	// The lockState_ variable is used to maintain the current status of the
	// lock.  It is composed of the following sections:
	//
	// +----------+-----------+---+----+----+-------+-+
	// | WCTR     |  RCTR     | W | WS | RS | CRDR  |R|
	// +----------+-----------+---+----+----+-------+-+
	// 31         22          12  11   10   9       1 0
	//
	// WCTR - Write wait count - # of waiting threads for a write.
	// RCTR - Read wait count - # of waiting threads for a read.
	// W - A write lock is currently in place
	// WS - The write event is signaled
	// RS - The read event is signaled
	// CRDR - Current reader count
	// R - A read lock is in place
	// 
	// The comparable C-structure is:
	// 
	// struct _LockState
	// {
	// 	unsigned long CurrentReaders : 10;
	// 	unsigned long ReadSignaled   : 1;
	// 	unsigned long WriteSignaled  : 1;
	// 	unsigned long WriteLock      : 1;
	// 	unsigned long WaitingReaders : 10;
	// 	unsigned long WaitingWriters : 9;
	// };
	//
	enum { 
		STATE_NONE		  = 0x00000000,	// No reader/writer lock 
		STATE_READLOCK	  = 0x00000001,	// Locked for read
		CURRENT_READERS   = 0x000003ff,	// Reader count (up to 1023 : 10 bits)
		STATE_READSIGNAL  = 0x00000400,	// Reader event is signaled
		STATE_WRITESIGNAL = 0x00000800,	// Writer event is signaled
		STATE_WRITELOCK   = 0x00001000,	// Has a writer lock
		STATE_READWAIT	  = 0x00002000,	// Has at least one waiting reader
		WAITREADERS_MASK  = 0x007FE000,	// Waiting reader mask (up to 1023 : 10 bits)
		WAITREADERS_SHIFT = 13,			// Shift mask for readers
		STATE_WRITEWAIT	  = 0x00800000,	// Has at least one waiting writer
		WAITWRITERS_MASK  = 0xFF800000,	// Mask for writers (up to 511 : 9 bits)
		WAITWRITERS_SHIFT = 23			// Shift mask for writers
	};

	enum { 
		MAX_CACHE_ENTRIES = 100,		// Max cached entries
		MAX_WRITER_WAITTIME = 250,		// Maximum time (msec) a writer will wait before readers are blocked
	};	

	// This structure is used to cache off the locking type and count for each
	// thread interested in this MSRWLock.  It is stored in a container and typed
	// by the thread ID.
	struct LockCounter {
		DWORD threadId;		// Owning threadid
		int count;			// reader level lock count
		LockCounter* pNext;	// Next in chain
		LockCounter* pPrev;	// Previous in chain
	};

// Constructor/Destructor
public:
	MRSWLock(int initialCache=0) : 
		writerID_(0), freeCount_(0), currReaders_(NULL), currFree_(NULL), writerCount_(0),
		writerEvent_(false,false,NULL,NULL), readerEvent_(false,true,NULL,NULL), writerTime_(0),
		heapMem_(0)
	{
#ifdef LOCK_STATS
		readerEntryCount_ = 0;
		readerContentionCount_ = 0;
		writerEntryCount_ = 0;
		writerContentionCount_ = 0;
		maxWriterWaitTime_ = 0;
#endif
		lockState_ = STATE_NONE;
		CreateCacheInstances(initialCache);
		if (gmaxSpinCount_ == -1) {
			SYSTEM_INFO sysInfo; GetSystemInfo(&sysInfo);
			gmaxSpinCount_ = (sysInfo.dwNumberOfProcessors > 1) ? 500 : 1;
		}
	}

	// Destructor which frees all our heap memory
	~MRSWLock()
	{
		HeapDestroy(heapMem_);
	}

// Operations
public:
	//////////////////////////////////////////////////////////////////////////
	// IsWriterLockedHeld
	//
	// This returns whether the writer lock is held by this thread.
	//
	inline bool IsWriterLockHeld() const
	{
		return (writerID_ == GetCurrentThreadId());
	}

	//////////////////////////////////////////////////////////////////////////
	// IsReaderLockedHeld
	//
	// This returns whether a reader lock is held by this thread.
	//
	inline bool IsReaderLockHeld() const
	{
		return (const_cast<MRSWLock*>(this)->FindLockCounter(GetCurrentThreadId()) != NULL);
	}

	//////////////////////////////////////////////////////////////////////////
	// ReadLock:
	//
	// This method acquires the read lock - a read lock can be acquired more than
	// one time by a single thread - you must release the lock for each time it is 
	// acquired.
	//
    bool ReadLock(DWORD nMillisecondTimeout)
	{
		const DWORD nThreadId = GetCurrentThreadId();

		// If we are already a writer for this lock, then bump our 
		// write counter.
		if (writerID_ == nThreadId)
			return WriteLock(nMillisecondTimeout);

		// Find or create our lock entry
		LockCounter* pLock = FindLockCounter(nThreadId, true);
		assert(pLock != NULL);

		// See what state this lock is currently in.  If it has no readers/writers
		// then transition it to a read lock.
		if (InterlockedCompareExchange(&lockState_, STATE_READLOCK, STATE_NONE) != STATE_NONE)
		{
			// Is currently in a read or write state.  See if we are already a reader for
			// this lock.
			if (pLock->count > 0)
			{
		        JTI_ASSERT(lockState_ & CURRENT_READERS);
				++pLock->count;
				return true;
			}

			// The lock has at least one client already; go through the various cases.
			long startState = lockState_, currState;
			unsigned int spinCount = 0;
			do 
			{
				// Capture the current state
				currState = startState;

				// Collect some numbers
				bool isReadSignaled = ((currState & STATE_READSIGNAL) > 0);
				int currentReaderCount = (currState & CURRENT_READERS);
				int currentReadWaiterCount = ((currState & WAITREADERS_MASK) >> WAITREADERS_SHIFT);
				int currentWriteWaiterCount = ((currState & WAITWRITERS_MASK) >> WAITWRITERS_SHIFT);
				DWORD elapsedTime = ELAPSED_TIME(writerTime_);

				// If we haven't maxed out our reader count and the read event is signaled,
				// then allow this thread to go through the lock. 
				if ((currState < CURRENT_READERS || 
					 (isReadSignaled &&
					  ((currentReaderCount + currentReadWaiterCount) < CURRENT_READERS))) &&
					 (currentWriteWaiterCount == 0 || elapsedTime < MAX_WRITER_WAITTIME))
				{
					// Bump the reader count
					JTI_ASSERT((currState & STATE_WRITELOCK)==0);
					startState = InterlockedCompareExchange(&lockState_, (currState+STATE_READLOCK), currState);
					if (startState == currState)
						break;
				}

				// Check for too many readers or waiting readers or signaling in progress
				else if ((currentReaderCount == CURRENT_READERS) ||
						 ((currState & WAITREADERS_MASK) == WAITREADERS_MASK))
				{
					// Put this thread to sleep
					Sleep(0);
					spinCount = 0;
				}

				// If the read signal is set, and we have writing waiters, then loop until
				// it is unset (i.e. all reader threads are released) and then wait on the read
				// signal.  We want to shift priority to the writer threads in this case.
				else if (isReadSignaled && currentWriteWaiterCount > 0)
				{
					Sleep(0);
					spinCount = 0;
				}

				// Otherwise add to the number of waiting readers. Contention is a bad thing; 
				// we will attempt to use a spin counter rather than wait on our event unless 
				// we spin for too long.  We wait longer on MP boxes as testing shows an 
				// extremely high contention rate on MP boxes; this alleviates some of the need 
				// to enter a kernel object.
				else if (++spinCount > static_cast<unsigned int>(gmaxSpinCount_)) 
				{
					// Try to set the state
					currState = InterlockedCompareExchange(&lockState_, (startState + STATE_READWAIT), startState);
					if (currState == startState)
					{
						// Wait on the reader.
#ifdef LOCK_STATS
						InterlockedIncrement(&readerContentionCount_);
#endif
						long modifyState = 0;
						DWORD dwStatus = readerEvent_.Wait(nMillisecondTimeout);
						if (dwStatus == WAIT_OBJECT_0)
						{
							JTI_ASSERT((lockState_ & STATE_WRITELOCK) == 0);
							JTI_ASSERT(lockState_ & STATE_READSIGNAL);
							JTI_ASSERT((lockState_ & CURRENT_READERS) < CURRENT_READERS);
							JTI_ASSERT(lockState_ & WAITREADERS_MASK);
							modifyState = STATE_READLOCK - STATE_READWAIT;
						}
						else
						{
							// Remove us as a read waiter
							modifyState = (DWORD) -STATE_READWAIT;
							if (dwStatus == WAIT_TIMEOUT)
								dwStatus = ERROR_TIMEOUT;
							else if (dwStatus == WAIT_IO_COMPLETION)
								dwStatus = ERROR_IO_INCOMPLETE;
							else
								dwStatus = GetLastError();
						}

						// One less waiting reader and he may have become a reader
						startState = InterlockedExchangeAdd(&lockState_, modifyState);

						// If this thread was signaled, then potentially reset the
						// lock to stop future threads from re-entering.  This allows writer
						// threads to get through the lock after all existing readers.
						if (dwStatus == WAIT_OBJECT_0)
						{
							JTI_ASSERT((startState & CURRENT_READERS) < CURRENT_READERS);
							JTI_ASSERT(startState & STATE_READSIGNAL);

							// If there is only one reading thread left then lower the
							// read signal flag and reset the reader event.
							if ((startState & WAITREADERS_MASK) == STATE_READWAIT)
							{
								// Reset the event and lower reader signaled flag
								readerEvent_.ResetEvent();
								InterlockedExchangeAdd(&lockState_, -STATE_READSIGNAL);
							}
						}
						// Any other status is a failure.
						else
						{
							if (((startState & WAITREADERS_MASK) == STATE_READWAIT) &&
							    (startState & STATE_READSIGNAL))
							{
								// Ensure the event is signaled before resetting it.
								DWORD tResult = readerEvent_.Wait(INFINITE);
								UNREFERENCED_PARAMETER(tResult);
	                            JTI_ASSERT(tResult == WAIT_OBJECT_0);
								JTI_ASSERT((lockState_ & CURRENT_READERS) < CURRENT_READERS);
	                            
								// Reset the event and lower reader signaled flag
								readerEvent_.ResetEvent();
								InterlockedExchangeAdd(&lockState_, (STATE_READLOCK - STATE_READSIGNAL));

								// Force the lock to be removed.
								pLock->count = 1;
								ReleaseReadLock();
							}
							else
								RemoveLockCounter(pLock);
#ifdef _DEBUG
							// Failed to acquire lock
							DebugBreak();
#endif
							return false;
						}
						JTI_ASSERT(dwStatus == WAIT_OBJECT_0);
						break;                        
					}
				}
				CpuPause();
				startState = lockState_;
			} 
			while(TRUE);
		}
		else
		{
			JTI_ASSERT(pLock->count == 0);
		}

		// Should not have write lock here.
		JTI_ASSERT(writerID_ == 0 && (lockState_ & STATE_WRITELOCK)==0);

		// Success
#ifdef LOCK_STATS
		InterlockedIncrement(&readerEntryCount_);
#endif
		pLock->count = 1;
		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// WriteLock:
	//
	// This method acquires the write lock - a write lock can be acquired more than
	// one time by a single thread - you must release the lock for each time it is 
	// acquired.
	//
	// In addition, a write lock can be acquired by a thread which currently owns
	// a read lock; i.e. we allow conversions from a read to a write.
	//
    bool WriteLock(DWORD nMillisecondTimeout)
	{
		const DWORD nThreadId = GetCurrentThreadId();

		// Check if the current thread already has writer lock.
		if (writerID_ == nThreadId)
		{
			++writerCount_;
			return true;
		}

		// If this thread already has a read lock, then we must release it in order to gain a
		// write lock.  We don't want to lose that information however.
		int readCount = 0;
		LockCounter* pLock = FindLockCounter(nThreadId, false);
		if (pLock != NULL)
		{
			// Remove the lock information from the queue.
			readCount = pLock->count;
			pLock->count = 1;
			ReleaseReadLock();
		}

		// If we have readers or writers then begin our wait loop.
		if (InterlockedCompareExchange(&lockState_, STATE_WRITELOCK, STATE_NONE) != STATE_NONE)
		{
			// Initialize
			long startState = lockState_, currState;
			int spinCount = 0;
			do
			{
				currState = startState;

				// We don't need to wait if there are no readers/writers.  We don't care if
				// there are waiting readers in this case.
				if ((currState == STATE_NONE) || (currState & ~(STATE_WRITESIGNAL | WAITREADERS_MASK)) == 0)
				{
					// Can be a writer
					startState = InterlockedCompareExchange(&lockState_, (currState + STATE_WRITELOCK), currState);
					if (startState == currState)
						break;
				}

				// Check for too many waiting writers
				else if (((currState & WAITWRITERS_MASK) == WAITWRITERS_MASK))
				{
					Sleep(0);
					spinCount = 0;
				}

				// Contention is a bad thing; we will attempt to use a spin counter rather than wait on 
				// our event unless we spin for too long.  We wait longer on MP boxes as testing shows an 
				// extremely high contention rate on MP boxes; this alleviates some of the need to enter a 
				// kernel object.
				else if (++spinCount > gmaxSpinCount_)
				{
					// We need to wait for the write event.  Add to waiting writers
					startState = InterlockedCompareExchange(&lockState_, (currState + STATE_WRITEWAIT), currState);
					if (startState == currState)
					{
						// Save off the time the first thread started waiting; we will begin 
						// blocking reader threads if a threshold is exceeded.
						if (((startState & WAITWRITERS_MASK) >> WAITWRITERS_SHIFT) == 1)
							writerTime_ = GetTickCount();

						// We are now officially waiting - block on the wait event.
#ifdef LOCK_STATS
						InterlockedIncrement(&writerContentionCount_);
#endif
						long modifyState = 0;
						DWORD dwStatus = writerEvent_.Wait(nMillisecondTimeout);
						if (dwStatus == WAIT_OBJECT_0) {
#ifdef LOCK_STATS
							DWORD elapsed = ELAPSED_TIME(writerTime_);
							if (elapsed > maxWriterWaitTime_)
								maxWriterWaitTime_ = elapsed;
#endif
							writerTime_ = GetTickCount();
							modifyState = STATE_WRITELOCK - STATE_WRITEWAIT - STATE_WRITESIGNAL;
						}
						else
						{
							modifyState = -STATE_WRITEWAIT;
							if (dwStatus == WAIT_TIMEOUT)
								dwStatus = ERROR_TIMEOUT;
							else if (dwStatus == WAIT_IO_COMPLETION)
								dwStatus = ERROR_IO_INCOMPLETE;
							else
								dwStatus = GetLastError();
						}

						// Remove this thread from the waiting list.
						startState = InterlockedExchangeAdd(&lockState_, modifyState);

						// If the write event did not get signaled, then perform error cleanup.
						if (dwStatus != WAIT_OBJECT_0)
						{
							// If we have maxed out our waiting threads for the write then
							// loop waiting for all the existing writers to clear out.
							if ((startState & STATE_WRITESIGNAL) && 
								((startState & WAITWRITERS_MASK) == STATE_WRITEWAIT))
							{
								do
								{
									// If the write event is signaled and we have no waiters
									// then wait on the event until we get it.
									startState = lockState_;
									if ((startState & STATE_WRITESIGNAL) &&
										((startState & WAITWRITERS_MASK) == 0))
									{
										DWORD dwTemp = writerEvent_.Wait(100);
										if (dwTemp == WAIT_OBJECT_0)
										{
											// Remove the signaling bit
											startState = InterlockedExchangeAdd(&lockState_, (STATE_WRITELOCK - STATE_WRITESIGNAL));

											// Reset to the orginal status
											JTI_ASSERT(writerID_ == 0);
											JTI_ASSERT(writerCount_ == 0);
											writerID_ = nThreadId;
											writerCount_ = 1;

											// Release the lock
											ReleaseWriteLock();
											break;
										}
									}
									else
										break;
								}
								while(TRUE);
							}
#ifdef _DEBUG
							DebugBreak();
#endif		                        
							return false;
						}
						break;
					}
				}
				CpuPause();
				startState = lockState_;
			} 
			while(TRUE);
		}

		// Save new writer information
		JTI_ASSERT(writerID_ == 0);
		JTI_ASSERT(writerCount_ == 0);
		writerID_ = nThreadId;
		writerCount_ = 1;
#ifdef LOCK_STATS
		InterlockedIncrement(&writerEntryCount_);
#endif
		// If we originally held a read lock, then re-add the counter to the list.
		if (readCount > 0)
		{
			pLock = FindLockCounter(nThreadId, true);
			pLock->count = readCount;
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// ReleaseReadLock:
	//
	// This method releases a lock for the given thread.
	//
    void ReleaseReadLock()
	{
		const DWORD nThreadId = GetCurrentThreadId();

		// Check if the thread has writer lock
		if (writerID_ == nThreadId)
		{
			ReleaseWriteLock();
			return;
		}

		// Find the read count
		LockCounter* pLock = FindLockCounter(nThreadId);
		if (pLock != NULL)
		{
			// If this thread no longer holds a lock then see if we 
			// need to release writers.
			if (--pLock->count == 0)
			{
                JTI_ASSERT((lockState_ & STATE_WRITELOCK) == 0);
                JTI_ASSERT(lockState_ & CURRENT_READERS);

				bool fLastReader = false;
				long startState = lockState_, currState, modifyState;
				do
				{
					currState = startState;
					modifyState = -STATE_READLOCK;
					fLastReader = ((currState & (STATE_READSIGNAL | CURRENT_READERS)) == STATE_READLOCK);
					if (fLastReader)
					{
						if (currState & WAITWRITERS_MASK)
							modifyState += STATE_WRITESIGNAL;
						else if (currState & WAITREADERS_MASK)
							modifyState += STATE_READSIGNAL;
					}

                    JTI_ASSERT((currState & STATE_WRITELOCK) == 0);
                    JTI_ASSERT(currState & CURRENT_READERS);

					startState = InterlockedCompareExchange(&lockState_, (currState + modifyState), currState);
				} 
				while(currState != startState);

				// Check for last reader
				if (fLastReader)
				{
					// Check for waiting writers
					if (currState & WAITWRITERS_MASK)
					{
                        JTI_ASSERT((lockState_ & (STATE_READSIGNAL|STATE_WRITESIGNAL)) == STATE_WRITESIGNAL);
						writerEvent_.SetEvent();
					}

					// Check for waiting readers
					else if (currState & WAITREADERS_MASK)
					{
                        JTI_ASSERT((lockState_ & (STATE_READSIGNAL|STATE_WRITESIGNAL)) == STATE_READSIGNAL);
						readerEvent_.SetEvent();
					}
				}

				// Delete our entry
				RemoveLockCounter(pLock);
			}
		}
		else
		{
#ifdef _DEBUG
			DebugBreak();
#endif
			throw std::logic_error("Failed to locate lockCounter structure");
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// ReleaseWriteLock
	//
	// This method releases the write lock for the owning thread.
	//
    void ReleaseWriteLock()
	{
		const DWORD nThreadId = GetCurrentThreadId();

		// Check validity of caller
		if (writerID_ == nThreadId)
		{
			// Check for nested release
			if (--writerCount_ == 0)
			{
				long currState, startState, modifyState;

				// Not a writer any more
				writerID_ = 0;
				startState = lockState_;
				do
				{
					currState = startState;
					modifyState = -STATE_WRITELOCK;
					if (currState & WAITREADERS_MASK)
						modifyState += STATE_READSIGNAL;
					else if (currState & WAITWRITERS_MASK)
						modifyState += STATE_WRITESIGNAL;
					startState = InterlockedCompareExchange(&lockState_, (currState + modifyState), currState);
				} 
				while (currState != startState);

				// Check for waiting writers/readers
				if (modifyState & STATE_WRITESIGNAL)
				{
                    JTI_ASSERT((lockState_ & (STATE_READSIGNAL|STATE_WRITESIGNAL)) == STATE_WRITESIGNAL);
					writerEvent_.SetEvent();
				}
				else if (modifyState & STATE_READSIGNAL)
				{
                    JTI_ASSERT((lockState_ & (STATE_READSIGNAL|STATE_WRITESIGNAL)) == STATE_READSIGNAL);
					readerEvent_.SetEvent();
				}

				// If this thread originally held a read-lock, then reacquire the read lock.
				LockCounter* pLock = FindLockCounter(nThreadId);
				if (pLock != NULL)
				{
					// Retrieve the original lock counter
					int readCount = pLock->count;

					// Remove it
					pLock->count = 0;
					RemoveLockCounter(pLock);
					
					// Reacquire the read lock - we wait forever.
					if (ReadLock(INFINITE))
					{
						// Reset the lock counter
						pLock = FindLockCounter(nThreadId);
						pLock->count = readCount;
					}
				}
 			}
		}
		else
		{
#ifdef _DEBUG
			DebugBreak();
#endif
		}
	}

#ifdef LOCK_STATS
	inline void ResetStatistics() { readerEntryCount_ = readerContentionCount_ = writerEntryCount_ = writerContentionCount_ = 0; maxWriterWaitTime_ = 0; }
    inline DWORD GetReaderEntryCount() const { return readerEntryCount_; }
    inline DWORD GetReaderContentionCount() const { return readerContentionCount_; }
    inline DWORD GetWriterEntryCount() const { return writerEntryCount_; }
    inline DWORD GetWriterContentionCount() const { return writerContentionCount_; }
	inline DWORD GetMaxWaitTime() const { return maxWriterWaitTime_; }
#endif

// Internal methods
private:
	//////////////////////////////////////////////////////////////////////////
	// AllocLockCounter
	// Allocates a new lock counter from our heap
	inline LockCounter* AllocLockCounter(DWORD nThreadId=0)
	{
		JTI_ASSERT(heapMem_ != 0);
		LockCounter* pLock = reinterpret_cast<LockCounter*>(HeapAlloc(heapMem_, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, sizeof(LockCounter)));
		if (pLock == NULL)
			throw jtiexception(E_OUTOFMEMORY, "Out of memory.");
		pLock->threadId = nThreadId;
		return pLock;
	}

	//////////////////////////////////////////////////////////////////////////
	// FreeLockCounter
	// Returns a lock counter to our heap
	inline void FreeLockCounter(LockCounter* pLock)
	{
		BOOL rc = HeapFree(heapMem_, HEAP_NO_SERIALIZE, pLock);
		JTI_ASSERT(rc == TRUE);
		UNREFERENCED_PARAMETER(rc);
	}

	//////////////////////////////////////////////////////////////////////////
	// CreateCacheInstances
	// Pre-fills our free list with cache instances
	inline void CreateCacheInstances(int initialCache)
	{
		JTI_ASSERT(heapMem_ == 0);
		if (initialCache > MAX_CACHE_ENTRIES)
			initialCache = MAX_CACHE_ENTRIES;
		heapMem_ = HeapCreate(HEAP_NO_SERIALIZE, max(initialCache,2) * sizeof(LockCounter), 0);
		if (heapMem_ == NULL)
			throw jtiexception(E_OUTOFMEMORY, "Out of memory.");
		for (freeCount_ = 0; freeCount_ < initialCache; ++freeCount_)
		{
			LockCounter* p = AllocLockCounter();
			p->pNext = currFree_;
			if (currFree_ != NULL)
				currFree_->pPrev = p;
			currFree_ = p;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// CreateNewLockCounter
	// This method creates or retrieves a cached lock counter
	inline LockCounter* CreateNewLockCounter(DWORD nThreadId)
	{
		if (currFree_ == NULL)
			return AllocLockCounter(nThreadId);

		--freeCount_;
		JTI_ASSERT(freeCount_ >= 0);

		LockCounter* p = currFree_;
		currFree_ = p->pNext;
		if (currFree_)
			currFree_->pPrev = NULL;

		p->count = 0;
		p->pNext = p->pPrev = NULL;
		p->threadId = nThreadId;
		return p;
	}

	//////////////////////////////////////////////////////////////////////////
	// FindLockCounter
	// This method locates an existing LockCounter object for a given threadID 
	// from our maintained list.
	inline LockCounter* FindLockCounter(DWORD nThreadId, bool add = false)
	{
		CCSLock<MRSWLock> _lockGuard(this);
		LockCounter* pCurr = currReaders_;
		while (pCurr) {
			if (pCurr->threadId == nThreadId)
				break;
			pCurr = pCurr->pNext;
		}
		if (add && pCurr == NULL) {
			pCurr = CreateNewLockCounter(nThreadId);
			pCurr->pNext = currReaders_;
			if (currReaders_ != NULL)
				currReaders_->pPrev = pCurr;
			currReaders_ = pCurr;
		}
		return pCurr;
	}

	//////////////////////////////////////////////////////////////////////////
	// RemoveLockCounter
	// This method removes the specified lock counter from our maintained list.
	inline void RemoveLockCounter(LockCounter* p)
	{
		CCSLock<MRSWLock> _lockGuard(this);
		if (p->pNext != NULL)
		{
			JTI_ASSERT(p->pNext->pPrev == p);
			p->pNext->pPrev = p->pPrev;
		}
		if (p->pPrev != NULL)
		{
			JTI_ASSERT(p->pPrev->pNext == p);
			p->pPrev->pNext = p->pNext;
		}
		if (p == currReaders_)
		{
			JTI_ASSERT(p->pPrev == NULL);
			currReaders_ = p->pNext;
		}
		if (freeCount_ >= MAX_CACHE_ENTRIES)
			FreeLockCounter(p);
		else
		{
			++freeCount_;
			p->pPrev = NULL;
			p->pNext = currFree_;
			if (currFree_ != NULL)
				currFree_->pPrev = p;
			currFree_ = p;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// CpuPause
	// This method implements the PAUSE instruction on the Intel platform.
	// This causes the CPU to drop down to the bus speed for this cycle and
	// therefore reduces power consumption without releasing the timeslice
	// from the OS perspective.
	inline void CpuPause()
	{
		// Single processor; give up our timeslice.
		if (gmaxSpinCount_ == 1)
			Sleep(0);
		else
#ifdef _M_IX86
		__asm 
		{
			REP NOP
		}
#endif
	}

// Do not allow copy operations
private:
    MRSWLock(const MRSWLock&);
    MRSWLock& operator=(const MRSWLock&);

// Class data
private:
	LockCounter* currReaders_;
	LockCounter* currFree_;
	long freeCount_;
	volatile long lockState_;
    EventSynch writerEvent_;
	EventSynch readerEvent_;
	DWORD writerID_;
	DWORD writerTime_;
	long writerCount_;
	static int gmaxSpinCount_;
	HANDLE heapMem_;
#ifdef LOCK_STATS
    volatile long readerEntryCount_;
    volatile long readerContentionCount_;
    volatile long writerEntryCount_;
    volatile long writerContentionCount_;
	volatile DWORD maxWriterWaitTime_;
#endif
};

// Globals
int __declspec(selectany) MRSWLock::gmaxSpinCount_ = -1;

/******************************************************************************/
// CCSRLock
//
// Exception-safe locking class which works with the MRSWLock class
//
/******************************************************************************/
class CCSRLock
{
private:
	MRSWLock* p_;
	long lockCount_;
public:
	CCSRLock(const MRSWLock* pob, bool init_lock=true) throw() : 
		p_(const_cast<MRSWLock*>(pob)), lockCount_(0) { if (init_lock) Lock(); }

	~CCSRLock() { 
		while (lockCount_ > 0) 
			Unlock(); 
		p_ = NULL; 
	}

	inline bool Lock(DWORD timeout=INFINITE) throw() {
		if (p_) {
			if (p_->ReadLock(timeout)) {
				++lockCount_;
				return true;
			}
		}
		return false;
	}
	inline void Unlock() throw() {
		if (p_ && lockCount_ > 0) {
			--lockCount_;
			p_->ReleaseReadLock();
		}
	}
};

/******************************************************************************/
// CCSWLock
//
// Exception-safe locking class which works with the MRSWLock class
//
/******************************************************************************/
class CCSWLock
{
private:
	MRSWLock* p_;
	long lockCount_;
public:
	CCSWLock(const MRSWLock* pob, bool init_lock=true) throw() : 
		p_(const_cast<MRSWLock*>(pob)), lockCount_(0) { if (init_lock) Lock(); }

	~CCSWLock() { 
		while (lockCount_ > 0) 
			Unlock(); 
		p_ = NULL; 
	}

	inline bool Lock(DWORD timeout=INFINITE) throw() {
		if (p_) {
			if (p_->WriteLock(timeout)) {
				++lockCount_;
				return true;
			}
		}
		return false;
	}
	inline void Unlock() throw() {
		if (p_ && lockCount_ > 0) {
			--lockCount_;
			p_->ReleaseWriteLock();
		}
	}
};

}// namespace JTI_Util

#pragma warning(default:4127)

#ifdef DEFINED_JTI_ASSERT
#undef JTI_ASSERT
#undef DEFINED_JTI_ASSERT
#endif

#endif // __JTI_RWLOCK_H_INCLUDED_
