/****************************************************************************/
//
// ThreadPool.h
//
// This file describes the I/O completion port manager class which holds
// and controls an I/O completion port handle and a set of worker threads.
//
// Copyright (C) 2001-2003 JulMar Technology, Inc.   All rights reserved
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

#ifndef __IOCP_THREADPOOL_H_INCL__
#define __IOCP_THREADPOOL_H_INCL__

/*----------------------------------------------------------------------------
// INCLUDE FILES
-----------------------------------------------------------------------------*/
#pragma warning(disable:4571)
#include <list>
#pragma warning(default:4571)
#include <Lock.h>

/*****************************************************************************/
// PC-Lint options
//
//lint -save
//
// Ignore repeated include files
//lint -e{537}
//
// Ignore C-style casts
//lint --e{1924}
//
// Ignore public data properties
//lint -esym(1925, IOCPThreadPool::IsRunning, IOCPThreadPool::IOCP, IOCPThreadPool::NumThreads)
//lint -esym(1925, IOCPThreadPool::IsShuttingDown)
//
// Data did not appear in constructor initializer list (properties)
//lint -esym(1927, IOCPThreadPool::IsRunning, IOCPThreadPool::IOCP, IOCPThreadPool::NumThreads)
//lint -esym(1927, IOCPThreadPool::IsShuttingDown)
//
// Data not initialized by constructor (properties)
//lint -esym(1401, IOCPThreadPool::IsRunning, IOCPThreadPool::IOCP, IOCPThreadPool::NumThreads)
//lint -esym(1401, IOCPThreadPool::IsShuttingDown)
//
// Data not initialized by assignment operator (properties)
//lint -esym(1539, IOCPThreadPool::IsRunning, IOCPThreadPool::IOCP, IOCPThreadPool::NumThreads)
//lint -esym(1539, IOCPThreadPool::IsShuttingDown)
//
// Const method indirectly modifies class
//lint -esym(1763, IOCPThreadPool::get_IOCP)
//
// Private constructors defined
//lint -esym(1704, IOCPThreadPool*)
//
/*****************************************************************************/
// Our function pointer type
#ifdef _lint
	typedef unsigned long TP_COMPLETION_KEY;
#else
	typedef ULONG_PTR TP_COMPLETION_KEY;
#endif

namespace JTI_Util
{
/*****************************************************************************
// IOCPThreadPool
//
// This class manages a given IOCP handle and a pool of worker threads to 
// dispatch work events to worker threads tied to the I/O port.
//
*****************************************************************************/
class IOCPThreadPool : public LockableObject<MultiThreadModel>
{
// Class data
private:
	typedef std::list<std::pair<unsigned int, HANDLE> > ThreadList;
	HANDLE iocp_;					// IOCP tied to thread pool
	volatile mutable long numThreads_;	// Number of workers currently tied to IOCP
	volatile bool shutdown_;		// True when shutting down
	ThreadList threads_;			// IOCP threads
	DWORD timeout_;					// Thread timeout value

	// Constructor
public:
	IOCPThreadPool();
	virtual ~IOCPThreadPool();

	// Properties
public:
	__declspec(property(get=get_IsRunning)) bool IsRunning;
	__declspec(property(get=get_IOCP)) HANDLE IOCP;
	__declspec(property(get=get_NumThreads, put=set_NumThreads)) long NumThreads;
	__declspec(property(get=get_IsShuttingDown)) bool IsShuttingDown;
	__declspec(property(get=get_Timeout, put=set_Timeout)) DWORD ThreadTimeout;

// Methods
public:
	bool Start(int nConcurrentThreads = 0, int nStartThreads = 0, HANDLE hIOCP = INVALID_HANDLE_VALUE) throw();
	bool Shutdown(DWORD dwWaitTime=60000) throw();
	bool AssociateHandle(HANDLE hHandle, TP_COMPLETION_KEY completionKey) throw();
	bool PostQueuedCompletionStatus(TP_COMPLETION_KEY completionKey, DWORD dwNumBytesTransferred=0, LPOVERLAPPED lpOverlapped=NULL) throw();
	bool IsCurrentThreadInPool() const throw();

// Property helpers
public:
	bool get_IsRunning() const throw();
	bool get_IsShuttingDown() const throw();
	HANDLE get_IOCP() const throw();
	long get_NumThreads() const throw();
	bool set_NumThreads(int nThreads) throw();
	DWORD get_Timeout() const;
	void set_Timeout(DWORD t);

// Internal methods
protected:
	virtual void WorkerThreadStart() {/* */}
	virtual bool ProcessWork(LPOVERLAPPED pio, DWORD dwBytesTransferred, TP_COMPLETION_KEY CompletionKey, BOOL rc, DWORD dwLastError) = 0;
	virtual void WorkerThreadEnd() {/* */}

private:
	void Run();
	void OnThreadClosing() throw();
	static unsigned int __stdcall InternalEventIOEntry(void* pv) {
		reinterpret_cast<IOCPThreadPool*>(pv)->Run();
		return 0;
	}
	// Unavailable
	IOCPThreadPool(const IOCPThreadPool& rhs);
	IOCPThreadPool& operator=(const IOCPThreadPool& rhs);
};

} // JTI_Util

#endif // __IOCP_THREADPOOL_H_INCL__
