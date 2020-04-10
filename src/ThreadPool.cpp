/****************************************************************************/
//
// ThreadPool.cpp
//
// This file describes the I/O completion port manager class which holds
// and controls an I/O completion port handle and a set of worker threads.
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
// Revision History
// -------------------------------------------------------
// 04/03/2001	MCS	Initial revision for JTTSP console
// 07/10/2002	MCS	Updated with some minor changes from JTISockets
// 10/28/2002   MCS Rewrote as generic class split away from JTISockets.
// 
/****************************************************************************/

/*----------------------------------------------------------------------------
// INCLUDE FILES
-----------------------------------------------------------------------------*/
#include "stdafx.h"
#include <process.h>
#include "ThreadPool.h"
#include "TraceLogger.h"

using namespace JTI_Util;

/*----------------------------------------------------------------------------
	LINT OPTIONS
-----------------------------------------------------------------------------*/
//lint --e{1924}  allow C-style casts
//lint -esym(1926, IOCPThreadPool::threads_)
//lint -esym(534, IOCPThreadPool::Shutdown, CloseHandle, PostQueuedCompletionStatus, std::transform)
//lint -esym(534, IOCPThreadPool::set_NumThreads, WaitForMultipleObjects)
//lint -esym(1740, IOCPThreadPool::iocp_, IOCPThreadPool::IOCP) Not directly freed in destructor

/*****************************************************************************
** search_bytid
** 
** This class is used to search the thread list for the given TID.
**
/****************************************************************************/
struct search_bytid
{
	unsigned int id_;
	explicit search_bytid(unsigned int id) : id_(id) {/* */}
	bool operator()(const std::pair<unsigned int, HANDLE>& item) const
	{
		return id_ == item.first;
	}
};

/*****************************************************************************
** Procedure:  IOCPThreadPool::IOCPManager
** 
** Arguments: void
** 
** Returns: void
** 
** Description: Constructor
**
/****************************************************************************/
IOCPThreadPool::IOCPThreadPool() : LockableObject<MultiThreadModel>(),
	iocp_(0), numThreads_(0), shutdown_(false), timeout_(INFINITE)
{
}// IOCPThreadPool::IOCPThreadPool

/*****************************************************************************
** Procedure:  IOCPThreadPool::~IOCPThreadPool
** 
** Arguments: void
** 
** Returns: void
** 
** Description: Destructor
**
/****************************************************************************/
IOCPThreadPool::~IOCPThreadPool() 
{
	// Perform a shutdown; do not wait in the shutdown.
	if (!Shutdown(2000))
	{
		// Now wait for all the threads to stop; we don't use WaitForSingleObject
		// here because it will deadlock if this is being destroyed due to a DLL unload.
		while (InterlockedCompareExchange(&numThreads_, 0, 0) > 0)
			Sleep(100);
	}

}// IOCPThreadPool::~IOCPThreadPool

/*****************************************************************************
** Procedure:  IOCPThreadPool::Start
** 
** Arguments: 'nThreads' - Number of threads to use
**            'hIOCP' - Handle to the I/O completion port
** 
** Returns: true/false success code
** 
** Description: This starts the IOCP manager
**
/****************************************************************************/
bool IOCPThreadPool::Start(int nConcurrentThreads, int nStartThreads, HANDLE hIOCP) throw()
{
	CCSLock<IOCPThreadPool> lockGuard(this);

	if (iocp_ == NULL) 
	{
		// If we have worker threads in IOCP right now, then don't allow
		// the server to be restarted.
		if (!threads_.empty()) //lint !e1550 Exception not thrown
			return false;

		// Mark we are starting up.
		shutdown_ = false;

		// Set our defaults
		SYSTEM_INFO sysInfo; ::GetSystemInfo(&sysInfo); 
		if (nConcurrentThreads == 0)
			nConcurrentThreads = static_cast<int>(sysInfo.dwNumberOfProcessors);
		if (nStartThreads == 0)
			nStartThreads = nConcurrentThreads + max((nConcurrentThreads/4),1);

		// Create the IO completion port this manager will work with
		iocp_ = CreateIoCompletionPort(hIOCP, NULL, 0, static_cast<DWORD>(nConcurrentThreads));
		if (iocp_ == NULL)
			return false;

		// Set the number of threads -- this will create/destroy threads
		if (!set_NumThreads(nStartThreads))
		{
			CloseHandle(iocp_);
			iocp_ = NULL;
			return false;
		}
	}
	return true;

}// IOCPThreadPool::Start

/*****************************************************************************
** Procedure:  IOCPThreadPool::set_NumThreads
** 
** Arguments: 'nThreads' - Number of threads to use
** 
** Returns: true/false success code
** 
** Description: This changes the number of threads involved in the pool
**
/****************************************************************************/
bool IOCPThreadPool::set_NumThreads(int nThreads) throw()
{
	CCSLock<IOCPThreadPool> lockGuard(this);

	if (iocp_ == NULL)
		return false;

	// Save off the current thread count
	int numThreads = get_NumThreads();

	// Already have the given number of threads?
	if (numThreads == nThreads)
		return true;

	// Or if we need to add threads to our list
	if (nThreads > numThreads)
	{
		unsigned int nThreadID;
		for (int i = numThreads; i < nThreads; ++i)
		{
			HANDLE hThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &InternalEventIOEntry, static_cast<void*>(this), 0, &nThreadID));
			if (hThread) {
				InterlockedIncrement(&numThreads_);
				threads_.push_back(std::make_pair(nThreadID, hThread));
			}
		}
	}

	// Or if we need to remove threads from our list
	else // (nThreads < numThreads)
	{
		for (int i = numThreads; i > nThreads; --i)
			::PostQueuedCompletionStatus(iocp_, 0, (TP_COMPLETION_KEY)-1, NULL);
	}

	return true;

}// IOCPThreadPool::set_NumThreads

#pragma warning(disable:4571)
/*****************************************************************************
** Procedure:  IOCPThreadPool::Shutdown
** 
** Arguments: void
** 
** Returns: true/false success code
** 
** Description: This stops the server
**
/****************************************************************************/
bool IOCPThreadPool::Shutdown(DWORD dwWaitTime) throw()
{
	bool allEnded = false;
	CCSLock<IOCPThreadPool> lockGuard(this);

	if (iocp_ != NULL && !shutdown_)
	{
		// Mark that we are shutting down
		shutdown_ = true;

		try
		{
			// Copy the list of handles into a flat array
			std::vector<HANDLE> arrHandles; arrHandles.reserve(threads_.size());
			for (ThreadList::iterator it = threads_.begin(); it != threads_.end(); ++it) 
			{
				// Do not wait on ourselves.
				if (it->first != GetCurrentThreadId())
				{
					HANDLE h;
					if (::DuplicateHandle(GetCurrentProcess(), it->second, GetCurrentProcess(), &h, 0, FALSE, DUPLICATE_SAME_ACCESS))
						arrHandles.push_back(h);
				}
			}

			// Unlock the guard; this allows the threads to exit.
			lockGuard.Unlock();

			// Stop all the worker threads
			set_NumThreads(0);

			// Wait for all the threads to terminate and then close the handles.
			if (!arrHandles.empty())
			{
				allEnded = (WaitForMultipleObjects(static_cast<DWORD>(arrHandles.size()), &arrHandles[0], TRUE, dwWaitTime) != WAIT_TIMEOUT);
				std::for_each(arrHandles.begin(), arrHandles.end(), stdx::sc_ptr_fun(&CloseHandle));
			}
		}
		catch(...)
		{
		}

		// Close the IOCP port
		::CloseHandle(iocp_);
		iocp_ = NULL;
		
		return allEnded;
	}

	return true;

}// IOCPThreadPool::Shutdown
#pragma warning(default:4571)

/*****************************************************************************
** Procedure:  IOCPThreadPool::get_IsRunning
** 
** Arguments: void
** 
** Returns: true/false
** 
** Description: This returns whether the pool is running.
**
/****************************************************************************/
bool IOCPThreadPool::get_IsRunning() const 
{ 
	CCSLock<IOCPThreadPool> lockGuard(this);
	return (iocp_ != NULL && get_NumThreads() > 0); 

}// IOCPThreadPool::get_IsRunning

/*****************************************************************************
** Procedure:  IOCPThreadPool::get_IsShuttingDown
** 
** Arguments: void
** 
** Returns: true/false
** 
** Description: Returns when the pool is shutting down
**
/****************************************************************************/
bool IOCPThreadPool::get_IsShuttingDown() const 
{ 
	CCSLock<IOCPThreadPool> lockGuard(this);
	return shutdown_; 

}// IOCPThreadPool::get_IsShuttingDown

/*****************************************************************************
** Procedure:  IOCPThreadPool::get_IOCP
** 
** Arguments: void
** 
** Returns: HANDLE
** 
** Description: Returns the associated IOCP handle (if any)
**
/****************************************************************************/
HANDLE IOCPThreadPool::get_IOCP() const
{ 
	CCSLock<IOCPThreadPool> lockGuard(this);
	return iocp_;

}// IOCPThreadPool::get_IOCP

/*****************************************************************************
** Procedure:  IOCPThreadPool::get_NumThreads
** 
** Arguments: void
** 
** Returns: Counter
** 
** Description: Returns the current count of threads associated with the IOCP
**
/****************************************************************************/
long IOCPThreadPool::get_NumThreads() const
{ 
	return InterlockedCompareExchange(&numThreads_, 0, 0); 

}// IOCPThreadPool::get_NumThreads

/*****************************************************************************
** Procedure:  IOCPThreadPool::get_Timeout
** 
** Arguments: void
** 
** Returns: Timeout
** 
** Description: Returns the timeout value
**
/****************************************************************************/
DWORD IOCPThreadPool::get_Timeout() const 
{ 
	CCSLock<IOCPThreadPool> lockGuard(this);
	return timeout_; 

}// IOCPThreadPool::get_Timeout

/*****************************************************************************
** Procedure:  IOCPThreadPool::set_Timeout
** 
** Arguments: 't' - new timer
** 
** Returns: void
** 
** Description: This changes the timeout for the thread pool
**
/****************************************************************************/
void IOCPThreadPool::set_Timeout(DWORD t) 
{ 
	CCSLock<IOCPThreadPool> lockGuard(this);
	timeout_ = t; 

}// IOCPThreadPool::set_Timeout

#pragma warning(disable:4571)
/*****************************************************************************
** Procedure:  IOCPThreadPool::Run
** 
** Arguments: void
** 
** Returns: void
** 
** Description: This is the function that processes overlapped I/O events.
**              It can have several threads processing it to handle socket
**              traffic.
**
/****************************************************************************/
void IOCPThreadPool::Run()
{
	LPOVERLAPPED pio; DWORD dwBytesTransferred; TP_COMPLETION_KEY clientKey;

	// Note the worker thread starting ..
	WorkerThreadStart();

	// Wait for new events on any socket
	for (;;)
	{
		// Clear out the input data
		pio = NULL; dwBytesTransferred = 0; clientKey = 0;

		// Get the next completion I/O block.
		BOOL rc = GetQueuedCompletionStatus(iocp_, &dwBytesTransferred, &clientKey, &pio, timeout_);
		if (clientKey == (TP_COMPLETION_KEY)-1)
			break;

		// Check the result code and make sure we save off the correct error.
		DWORD dwLastError = (rc == FALSE) ? GetLastError() : 0;

		// Invalid IOCP handle or timeout? exit the thread.
		if (rc == FALSE && dwLastError == ERROR_INVALID_HANDLE)
			break;

		// Verify the OVERLAPPED structure; if it fails then loop around and get the next one.
		if (!(pio == NULL || (!IsBadReadPtr(pio, sizeof(OVERLAPPED)))))
		{
			JTI_ASSERT(!"Invalid OVERLAPPED I/O packet dequeued from completion port");
			continue;
		}

		// Call virtual worker function; do not allow exceptions outside this function.
		try
		{
			//lint -e{1933} call to unqualified virtual function
			if (ProcessWork(pio, dwBytesTransferred, clientKey, rc, dwLastError))
				break;
		}
		catch(...)
		{
			// Let the derived class cleanup before termination
			WorkerThreadEnd();

			// Cleanup the thread internally
			OnThreadClosing();

			// Rethrow
			throw;
		}
	}

	// Note the worker thread ending
	WorkerThreadEnd();

	// Cleanup the thread
	OnThreadClosing();

}// IOCPThreadPool::Run
#pragma warning(default:4571)

/*****************************************************************************
** Procedure:  IOCPThreadPool::OnThreadClosing
** 
** Arguments: void
** 
** Returns: void
** 
** Description: This function closes out a thread.
**
/****************************************************************************/
void IOCPThreadPool::OnThreadClosing() throw()
{
	// Remove the thread count
	InterlockedDecrement(&numThreads_);

	// Remove the handle from the array and close it.
	CCSLock<IOCPThreadPool> lockGuard(this);
	ThreadList::iterator it = std::find_if(threads_.begin(), threads_.end(), 
		search_bytid(GetCurrentThreadId()));
	if (it != threads_.end()) {
		CloseHandle(it->second);
		threads_.erase(it);
	}

}// IOCPThreadPool::OnThreadClosing

/*****************************************************************************
** Procedure:  IOCPThreadPool::AssociateHandle
** 
** Arguments: 'hHandle' - Handle of device/file to associate with IOCP
**            'completionKey' - Completion key
** 
** Returns: True/False success code
** 
** Description: This function associates the given handle with the IOCP and
**              thread pool.
**
/****************************************************************************/
bool IOCPThreadPool::AssociateHandle(HANDLE hHandle, TP_COMPLETION_KEY completionKey) throw()
{
	return (iocp_ == ::CreateIoCompletionPort(hHandle, iocp_, completionKey, 0));

}// IOCPThreadPool::AssociateHandle

/*****************************************************************************
** Procedure:  IOCPThreadPool::PostQueuedCompletionStatus
** 
** Arguments: 'dwNumBytesTransferred' - # of bytes transferred in I/O operation
**            'completionKey' - Completion key
**            'lpOverlapped' - OVERLAPPEDIO structure
** 
** Returns: True/False success code
** 
** Description: This function associates the given handle with the IOCP and
**              thread pool.
**
/****************************************************************************/
bool IOCPThreadPool::PostQueuedCompletionStatus(TP_COMPLETION_KEY completionKey, DWORD dwNumBytesTransferred, LPOVERLAPPED lpOverlapped) throw()
{
	return (::PostQueuedCompletionStatus(iocp_, dwNumBytesTransferred, completionKey, lpOverlapped) != FALSE);

}// IOCPThreadPool::PostQueuedCompletionStatus

/*****************************************************************************
** Procedure:  IOCPThreadPool::IsCurrentThreadInPool
** 
** Arguments: void
** 
** Returns: True/False success code
** 
** Description: This function determines whether the current thread is in the
**              thread pool.
**
/****************************************************************************/
bool IOCPThreadPool::IsCurrentThreadInPool() const throw()
{
	CCSLock<IOCPThreadPool> lockGuard(this); 
	//lint -e(1550) Exception is not thrown
	return (std::find_if(threads_.begin(), threads_.end(), search_bytid(GetCurrentThreadId())) != threads_.end());

}// IOCPThreadPool::IsCurrentThreadInPool
