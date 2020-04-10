/****************************************************************************/
//
// WorkerThreadPool.h
//
// This file describes a customized thread pool which adjusts the number
// of worker threads based on the timing of existing requests.
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
// 10/31/2002   MCS Added to JTIUtils from TSP++ (modified for template)
// 
/****************************************************************************/

#ifndef __WORKER_THREADPOOL_H_INCL__
#define __WORKER_THREADPOOL_H_INCL__

/*----------------------------------------------------------------------------
// INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <process.h>
#include <ThreadPool.h>
#include <Delegates.h>
#include <Synchronization.h>
#include <SingletonRegistry.h>
#include <Lock.h>

namespace JTI_Util
{
/*****************************************************************************
// WTPTraits_NotifyNop
//
// This class implements a Nop version of the start/end request notifications
//
*****************************************************************************/
struct WTPTraits_NotifyNop
{
	static void WorkerThreadPool_StartThread() {}
	static void WorkerThreadPool_EndThread() {}
};

/*****************************************************************************
// WTPTraits_InitCom
//
// This class implements a traits class which initializes COM
//
*****************************************************************************/
template <DWORD type=COINIT_MULTITHREADED>
struct WTPTraits_InitCom
{
	static void WorkerThreadPool_StartThread() { CoInitializeEx(NULL, type); }
	static void WorkerThreadPool_EndThread() { CoUninitialize(); }
};

/*****************************************************************************
// WTPDefaultTimers
//
// This structure defines the default timeouts used for the WorkThreadPool
//
*****************************************************************************/
template <int ThreadTimeout=(5*60*1000), int MaxThreads=40, int MaxWaitTimeMsec=100, int ThreadIncStep=5>
struct WTPDefaultTimers
{
	enum { 
		THREAD_TIMEOUT		= ThreadTimeout,
		MAX_THREADS			= MaxThreads,
		MAX_WAIT_THRESHOLD	= MaxWaitTimeMsec,
		THREAD_INCREMENT    = ThreadIncStep
	};
};

/*****************************************************************************
// WorkerThreadPool
//
// This class manages two IOCP Threadpools; one for dispatching requests into
// a second thread pool which actually handles the requests.
//
*****************************************************************************/
template <class _Arg = ULONG_PTR, class _TNotify = WTPTraits_NotifyNop, class _TTimers = WTPDefaultTimers<> >
class WorkerThreadPool
{
// Class definitions
private:
	struct WorkItemDelegate {
		bool hasArgs;
		_Arg arg;
		union {
			DelegateInvoke* pfun;
			DelegateInvoke_1<_Arg>* pfun1;
		} ThisDelegate;
		WorkItemDelegate(DelegateInvoke* pfun_) { arg = NULL; hasArgs = false; ThisDelegate.pfun = pfun_; }
		WorkItemDelegate(DelegateInvoke_1<_Arg>* pfun_, _Arg arg_) { hasArgs = true; arg = arg_; ThisDelegate.pfun1 = pfun_; }
		~WorkItemDelegate() { if (hasArgs) delete ThisDelegate.pfun1; else delete ThisDelegate.pfun; }
	};

	struct WorkItemDelegateWaiter {
		bool hasArgs;
		_Arg arg;
		HANDLE hWait;
		HANDLE hStop;
		HANDLE hKill;
		DWORD dwTimeout;
		bool executeOnce;
		union {
			DelegateInvoke_1<bool>* pfun;
			DelegateInvoke_2<_Arg, bool>* pfun1;
		} ThisDelegate;
		WorkItemDelegateWaiter(HANDLE hWait_, HANDLE hStop_, DWORD dwTimeout_, bool ExecuteOnce, DelegateInvoke_1<bool>* pfun_) { 
			arg = NULL; hasArgs = false; ThisDelegate.pfun = pfun_; hStop = hStop_;
			if (hWait_ != NULL)
				DuplicateHandle(GetCurrentProcess(), hWait_, GetCurrentProcess(), &hWait, 0, FALSE, DUPLICATE_SAME_ACCESS); 
			else
				hWait = NULL;
			dwTimeout = dwTimeout_; executeOnce = ExecuteOnce;
			hKill = CreateEvent(NULL, TRUE, FALSE, NULL);
		}
		WorkItemDelegateWaiter(HANDLE hWait_, HANDLE hStop_, DWORD dwTimeout_, bool ExecuteOnce, DelegateInvoke_2<_Arg,bool>* pfun_, _Arg arg_) { 
			hasArgs = true; arg = arg_; ThisDelegate.pfun1 = pfun_;  hStop = hStop_;
			if (hWait_ != NULL)
				DuplicateHandle(GetCurrentProcess(), hWait_, GetCurrentProcess(), &hWait, 0, FALSE, DUPLICATE_SAME_ACCESS); 
			else
				hWait = NULL;
			dwTimeout = dwTimeout_; executeOnce = ExecuteOnce;
			hKill = CreateEvent(NULL, TRUE, FALSE, NULL);
		}
		~WorkItemDelegateWaiter() { 
			if (hasArgs) 
				delete ThisDelegate.pfun1; 
			else 
				delete ThisDelegate.pfun; 
			CloseHandle(hWait); 
			CloseHandle(hKill); 
		}
		HANDLE get_WaitHandle() const { return hKill; }
	};

	class WorkThreadPool : public IOCPThreadPool
	{
	private:
		mutable volatile long running_;
		mutable volatile long inQueue_;
		mutable volatile long inWork_;
		long minThreads_;

		virtual void WorkerThreadStart() {_TNotify::WorkerThreadPool_StartThread();}
		virtual void WorkerThreadEnd() {_TNotify::WorkerThreadPool_EndThread();}
		virtual bool ProcessWork(LPOVERLAPPED pio, DWORD dwBytesTransferred, 
			ULONG_PTR CompletionKey, BOOL rc, DWORD dwLastError)
		{
			UNREFERENCED_PARAMETER(pio); UNREFERENCED_PARAMETER(dwBytesTransferred);

			// Timeout? Allow the thread to exit.
			if (rc == FALSE && dwLastError == WAIT_TIMEOUT)
				return (NumThreads > minThreads_);

			// If we have a wait event, signal it.
			if (pio && pio->hEvent)
				SetEvent(pio->hEvent);

			// Check for our work packet
			if (CompletionKey != 0)
			{
				InterlockedDecrement(&inQueue_);
				IncDecHolder incItem(&inWork_);
				WorkItemDelegate* pDelegate = reinterpret_cast<WorkItemDelegate*>(CompletionKey);
				std::auto_ptr<WorkItemDelegate> acItem(pDelegate);

				// Call the function; we will automatically cleanup if an exception occurs
				// because of the above class holders.
				if (pDelegate->hasArgs)
					(*pDelegate->ThisDelegate.pfun1)(pDelegate->arg);
				else
					(*pDelegate->ThisDelegate.pfun)();
			}
			return false;
		}

		public:
			WorkThreadPool() : IOCPThreadPool(), 
				inQueue_(0), inWork_(0), minThreads_(0), running_(0) { ThreadTimeout = _TTimers::THREAD_TIMEOUT; }
			virtual ~WorkThreadPool() {/* */}

			bool Start(int nMinThreads, int nSimulThreads)
			{
				minThreads_ = nMinThreads;
				return IOCPThreadPool::Start(nSimulThreads, nMinThreads);
			}

			bool PostQueuedCompletionStatus(_Arg pItem, OVERLAPPED* pio) {
				InterlockedIncrement(&inQueue_);
				return IOCPThreadPool::PostQueuedCompletionStatus((TP_COMPLETION_KEY)pItem, 0, pio);
			}

			long get_Queued() const { return InterlockedCompareExchange(&inQueue_, 0, 0); }
			long get_InWork() const { return InterlockedCompareExchange(&inWork_, 0, 0); }
	};

	class DispatchThreadPool : public IOCPThreadPool
	{
	private:
		WorkThreadPool& workPool_;
		EventSynch& evtStop_;
		EventSynch evtWait_;
		OVERLAPPED oio;
		int maxThreads_;
	public:
		DispatchThreadPool(WorkThreadPool& workPool, EventSynch& evtStop) : IOCPThreadPool(), 
			maxThreads_(0), workPool_(workPool), evtWait_(false, true), evtStop_(evtStop) { oio.hEvent = evtWait_.get(); }
		~DispatchThreadPool() {/* */}
		
		bool Start(int nMaxThreads) 
		{ 
			maxThreads_ = nMaxThreads;
			return IOCPThreadPool::Start(1, 1); 
		}

		virtual bool ProcessWork(LPOVERLAPPED pio, DWORD dwBytesTransferred, 
			ULONG_PTR CompletionKey, BOOL rc, DWORD dwLastError)
		{
			UNREFERENCED_PARAMETER(pio); UNREFERENCED_PARAMETER(dwBytesTransferred);
			UNREFERENCED_PARAMETER(rc); UNREFERENCED_PARAMETER(dwLastError);

			// Reset our wait event
			evtWait_.ResetEvent();

			// Move the request to the real thread pool; it may
			// get queued up on the worker pool, but that's ok.
			if (CompletionKey && workPool_.PostQueuedCompletionStatus((_Arg)CompletionKey, &oio))
			{
				// Check to see how busy the pool is.  We may need to create
				// some additional worker threads if we cannot start this request
				// right away.
				if (evtWait_.Wait(_TTimers::MAX_WAIT_THRESHOLD) == WAIT_TIMEOUT)
				{
					long numQueued = workPool_.get_Queued();
					long numThreads = workPool_.NumThreads;
					long numActive = workPool_.get_InWork();
					long numNeeded = min(numActive + numQueued, maxThreads_);
					if (numThreads < numNeeded) {
						numThreads = min(numNeeded+_TTimers::THREAD_INCREMENT, maxThreads_);
						workPool_.set_NumThreads(numThreads);
					}
				}
			}

			// Never ask the thread to terminate
			return false;
		}
	};


// Constructor
public:
	WorkerThreadPool() :
	  evtStop_(false, true), workerPool_(), dispatchPool_(workerPool_, evtStop_), 
		isShuttingDown_(false) {/* */}
	virtual ~WorkerThreadPool() { InternalStop();}

// Properties
public:
	__declspec(property(get=get_TotalWorkers)) int TotalWorkers;
	__declspec(property(get=get_InQueue)) int InQueue;
	__declspec(property(get=get_InProgress)) int InProgress;
	__declspec(property(get=get_isRunning)) bool IsRunning;

// Methods
public:
	bool Start(int nMinThreads=0, int nMaxThreads=0, int nSimulThreads=0)
	{
		isShuttingDown_ = false;
		if (nMaxThreads == 0)
			nMaxThreads = _TTimers::MAX_THREADS;
		if (nMinThreads == 0)
			nMinThreads = max(nMaxThreads/4,2);
		if (nSimulThreads == 0)
		{
			SYSTEM_INFO sysInfo; ::GetSystemInfo(&sysInfo);
			nSimulThreads = static_cast<int>(sysInfo.dwNumberOfProcessors);
			if (nSimulThreads > nMaxThreads)
				nSimulThreads = nMaxThreads;
		}
		return (workerPool_.Start(nMinThreads, nSimulThreads) && dispatchPool_.Start(nMaxThreads));
	}

	void Shutdown() { InternalStop(); }

// Methods
public:
	template<class _Object, class _Class>
	bool QueueUserWorkItem(const _Object& object, void (_Class::*fnc)())
	{
		return QueueWorkItem(JTI_NEW WorkItemDelegate(JTI_NEW DelegateInvokeMethod<_Class>(*(_Object*)&object, fnc)));
	}

	template<class _Object, class _Class>
	bool QueueUserWorkItem(const _Object& object, void (_Class::*fnc)(_Arg), _Arg arg)
	{
		return QueueWorkItem(JTI_NEW WorkItemDelegate(JTI_NEW DelegateInvokeMethod_1<_Class, _Arg>(*(_Object*)&object, fnc), arg));
	}

	bool QueueUserWorkItem(void (*fnc)(_Arg), _Arg arg)
	{
		return QueueWorkItem(JTI_NEW WorkItemDelegate(JTI_NEW DelegateInvokeFunc_1<_Arg>(fnc), arg));
	}

	void UnregisterWaitForSingleObject(HANDLE hWait) const
	{
		SetEvent(hWait);
	}

	template<class _Object, class _Class>
	HANDLE RegisterWaitForSingleObject(HANDLE hWait, const _Object& object, void (_Class::*fnc)(bool), DWORD dwTimeout, bool executeOnlyOnce)
	{
		// Verify the parameters
		if (dwTimeout == INFINITE && hWait == NULL)
			return false;

		WorkItemDelegateWaiter* pWaiter = JTI_NEW WorkItemDelegateWaiter(hWait, evtStop_.get(), dwTimeout, executeOnlyOnce,
			JTI_NEW DelegateInvokeMethod_1<_Class, bool>(*(_Object*)&object, fnc));
		unsigned nThreadId;
		HANDLE handle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, 
				&RegisterWaitCallback, reinterpret_cast<void*>(pWaiter), 0, &nThreadId));
		if (handle)
		{
			CloseHandle(handle);
			return pWaiter->get_WaitHandle();
		}
		delete pWaiter;
		return INVALID_HANDLE_VALUE;
	}

	template<class _Object, class _Class>
	HANDLE RegisterWaitForSingleObject(HANDLE hWait, const _Object& object, void (_Class::*fnc)(_Arg, bool), _Arg arg, DWORD dwTimeout, bool executeOnlyOnce)
	{
		// Verify the parameters
		if (dwTimeout == INFINITE && hWait == NULL)
			return false;

		WorkItemDelegateWaiter* pWaiter = JTI_NEW WorkItemDelegateWaiter(hWait, evtStop_.get(), dwTimeout, executeOnlyOnce,
			JTI_NEW DelegateInvokeMethod_2<_Class, _Arg, bool>(*(_Object*)&object, fnc), arg);
		unsigned nThreadId;
		HANDLE handle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, 
				&RegisterWaitCallback, reinterpret_cast<void*>(pWaiter), 0, &nThreadId));
		if (handle)
		{
			CloseHandle(handle);
			return pWaiter->get_WaitHandle();
		}
		delete pWaiter;
		return INVALID_HANDLE_VALUE;
	}

	HANDLE RegisterWaitForSingleObject(HANDLE hWait, void (*fnc)(_Arg, bool), _Arg arg, DWORD dwTimeout, bool executeOnlyOnce)
	{
		// Verify the parameters
		if (dwTimeout == INFINITE && hWait == NULL)
			return false;

		WorkItemDelegateWaiter* pWaiter = JTI_NEW WorkItemDelegateWaiter(hWait, evtStop_.get(), dwTimeout, executeOnlyOnce,
			JTI_NEW DelegateInvokeFunc_2<_Arg, bool>(fnc), arg));
		unsigned nThreadId;
		HANDLE handle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, 
				&RegisterWaitCallback, reinterpret_cast<void*>(pWaiter), 0, &nThreadId));
		if (handle)
		{
			CloseHandle(handle);
			return pWaiter->get_WaitHandle();
		}
		delete pWaiter;
		return INVALID_HANDLE_VALUE;
	}

	HANDLE RegisterWaitForSingleObject(HANDLE hWait, void (*fnc)(bool), DWORD dwTimeout, bool executeOnlyOnce)
	{
		// Verify the parameters
		if (dwTimeout == INFINITE && hWait == NULL)
			return false;

		WorkItemDelegateWaiter* pWaiter = JTI_NEW WorkItemDelegateWaiter(hWait, evtStop_.get(), dwTimeout, executeOnlyOnce,
			JTI_NEW DelegateInvokeFunc_1<bool>(fnc));
		unsigned nThreadId;
		HANDLE handle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, 
				&RegisterWaitCallback, reinterpret_cast<void*>(pWaiter), 0, &nThreadId));
		if (handle)
		{
			CloseHandle(handle);
			return pWaiter->get_WaitHandle();
		}
		delete pWaiter;
		return INVALID_HANDLE_VALUE;
	}

// Property helpers
public:
	bool get_isRunning() const { return workerPool_.IsRunning; }
	int get_TotalWorkers() const { return workerPool_.NumThreads; }
	int get_InQueue() const { return workerPool_.get_Queued(); }
	int get_InProgress() const { return workerPool_.get_InWork(); }

// Internal methods
private:
	static unsigned __stdcall RegisterWaitCallback(void* p)
	{
		WorkItemDelegateWaiter* pItem = reinterpret_cast<WorkItemDelegateWaiter*>(p);
		HANDLE arrHandles[3] = { pItem->hStop, pItem->hKill, pItem->hWait };
		int nCount = (pItem->hWait == NULL) ? 2 : 3;

		_TNotify::WorkerThreadPool_StartThread();

		for (;;)
		{
			DWORD rc = WaitForMultipleObjects(nCount, arrHandles, FALSE, pItem->dwTimeout);
			if (rc == WAIT_FAILED ||
				rc == WAIT_OBJECT_0 || rc == WAIT_ABANDONED_0 ||
				rc == WAIT_OBJECT_0 + 1 || rc == WAIT_ABANDONED_0 + 1)
				break;

			if (pItem->hasArgs)
				(*pItem->ThisDelegate.pfun1)(pItem->arg, (rc == WAIT_TIMEOUT));
			else
				(*pItem->ThisDelegate.pfun)((rc==WAIT_TIMEOUT));

			if (pItem->executeOnce)
				break;
		}
		delete pItem;

		_TNotify::WorkerThreadPool_EndThread();

		return 0;
	}

	void InternalStop()
	{
		isShuttingDown_ = true;

		// Shutdown the dispatcher
		evtStop_.SetEvent();

		// Shutdown the dispatcher
		dispatchPool_.Shutdown();

		// Wait for all requests to be services.
		while (workerPool_.get_Queued() > 0)
			Sleep(100);
		workerPool_.Shutdown();
	}

	bool QueueWorkItem(WorkItemDelegate* pItem)
	{
		if (isShuttingDown_ || !dispatchPool_.PostQueuedCompletionStatus((TP_COMPLETION_KEY)pItem)) 
		{
			delete pItem;
			return false;
		}
		return true;
	}

// Class data
private:
	WorkThreadPool workerPool_;			// Worker thread pool
	DispatchThreadPool dispatchPool_;	// Dispatch pool
	EventSynch evtStop_;				// Stop event
	bool isShuttingDown_;

// Unavailable methods
private:
	WorkerThreadPool(const WorkerThreadPool&);
	WorkerThreadPool& operator=(const WorkerThreadPool&);
};

}// namespace JTI_Util



#endif // __WORKER_THREADPOOL_H_INCL__
