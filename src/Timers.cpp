/****************************************************************************/
//
// Timers.cpp
//
// This file implements a class for managing callback periodic timers
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

/*----------------------------------------------------------------------------
	INCLUDE FILES
-----------------------------------------------------------------------------*/
#include "stdafx.h"
#include <process.h>
#include "Timers.h"

using namespace JTI_Util;

/*****************************************************************************
** Procedure:  TimerManager::~TimerManager
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: Destructor for the timer manager
**
/****************************************************************************/
TimerManager::~TimerManager() 
{  
	// Stop the worker
	evtStop_.SetEvent();
	if (thread_ && GetCurrentThread() != thread_)
		WaitForSingleObject(thread_, INFINITE);
	Clear();

}// TimerManager::~TimerManager

/*****************************************************************************
** Procedure:  TimerManager::Clear
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: Removes all timers from the list. 
**
/****************************************************************************/
void TimerManager::Clear()
{
	CCSLock<TimerManager> lockGuard(this);
	std::for_each(qTimers_.begin(), qTimers_.end(), 
		stdx::delptr<TimerEntry*>());
	qTimers_.clear();
	evtNewTimer_.SetEvent();

}// TimerManager::Clear

/*****************************************************************************
** Procedure:  TimerManager::KillTimer
** 
** Arguments:  'nTimer' - Timer ID to remove
** 
** Returns: Success indicator
** 
** Description: This removes a specific timer by id.
**
/****************************************************************************/
bool TimerManager::KillTimer(int nTimer)
{
	CCSLock<TimerManager> lockGuard(this);
	std::vector<TimerEntry*>::iterator it = std::find_if(qTimers_.begin(), qTimers_.end(), 
		stdx::compose_f_gx(std::bind2nd(std::equal_to<int>(),nTimer), 
		std::mem_fun(&TimerEntry::get_ID)));
	bool foundTimer = (it != qTimers_.end());
	if (foundTimer) {
		TimerEntry* pEntry = *it;
		qTimers_.erase(it);
		delete pEntry;
	}
	// We always reset the event so that the timer thread "resets" itself
	// as to the next timeout event.
	evtNewTimer_.SetEvent();
	return foundTimer;

}// TimerManager::KillTimer

/*****************************************************************************
** Procedure:  TimerManager::AddTimer
** 
** Arguments:  'nTimer' - Unique timer id
**             'msecTimeout' - Timeout
**             'func' - Delegate to fire
** 
** Returns: void
** 
** Description: This adds a new timer
**
/****************************************************************************/
void TimerManager::AddTimer(int nTimer, DWORD msecTimeout, DelegateInvoke_1<int>* Func)
{
	CCSLock<TimerManager> lockGuard(this);

	// Verify this entry doesn't exist already
	std::vector<TimerEntry*>::iterator it = std::find_if(qTimers_.begin(), qTimers_.end(), 
		stdx::compose_f_gx(std::bind2nd(std::equal_to<int>(),nTimer), 
		std::mem_fun(&TimerEntry::get_ID)));
	if (it != qTimers_.end()) {
		TimerEntry* pEntry = *it;
		qTimers_.erase(it);
		delete pEntry;
	}

	// Add the element to the list
	qTimers_.push_back(JTI_NEW TimerEntry(nTimer, msecTimeout, Func));

	// If the thread isn't running yet, start it up.
	if (thread_ == 0)
	{
		unsigned nThreadId;
		thread_ = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &TimerWorkerThread, 
						reinterpret_cast<void*>(this), 0, &nThreadId));
		if (thread_ == 0)
			throw std::runtime_error("AddTimer unable to create worker thread.");
	}

	// Wake the thread up.
	evtNewTimer_.SetEvent();

}// TimerManager::AddTimer

/*****************************************************************************
** Procedure:  TimerManager::TimerWorker
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: This is the worker thread which fires timers
**
/****************************************************************************/
void TimerManager::TimerWorker()
{
	CCSLock<TimerManager> lockGuard(this, false);
	HANDLE arrHandles[] = { evtStop_.get(), evtNewTimer_.get() };
	for (;;)
	{
		// Lock for exclusive access to the array
		lockGuard.Lock();

		// No more timers? exit
		if (qTimers_.empty())
			break;

		// Re-sort our vector
		if (qTimers_.size() > 1)
		{
			std::partial_sort(qTimers_.begin(), qTimers_.begin()+1, 
				qTimers_.end(), stdx::ptr_less<TimerEntry>());
		}

		// Get the sleep timeout
		TimerEntry* pEntry = qTimers_.front();
		DWORD dwTickLast = pEntry->get_LastFireTime();
		DWORD dwElapsed = ELAPSED_TIME(dwTickLast);
		DWORD dwTimeout = pEntry->get_Interval();
		
		// Allow other threads to access array again.
		lockGuard.Unlock();

		// Now determine how long to sleep
		if (dwElapsed < dwTimeout)
			dwTimeout = (dwTimeout - dwElapsed);
		else
			dwTimeout = 0;

		// Go to sleep
		DWORD rc = WaitForMultipleObjects(2, arrHandles, FALSE, dwTimeout);
		if (rc == WAIT_OBJECT_0)
			break;
		else if (rc == WAIT_OBJECT_0 + 1)
		{
			// New object - loop around and re-determine sleep time.
			evtNewTimer_.ResetEvent();
			continue;
		}
		else if (rc == WAIT_TIMEOUT)
		{
			// Fire a timer
			lockGuard.Lock();

			// Catch the condition where the timer has been removed _right_ before
			// the timeout occurs and we end up with a timeout before the "change"
			// event.
			if (!qTimers_.empty())
			{
				TimerEntry* pTimer = qTimers_.front();
				if (ELAPSED_TIME(pTimer->get_LastFireTime()) >= pTimer->get_Interval())
					pTimer->FireTimer();
			}

			lockGuard.Unlock();
		}
	}
	CloseHandle(thread_);
	thread_ = 0;

}// TimerManager::TimerWorker

/*****************************************************************************
** Procedure:  TimerEntry::FireTimer
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: This fires the given timer with a validation test in case
**              client owner has deleted the place where the callback
**              would occur or the underlying code has been removed from
**              memory (dynamic DLL), etc.
**
/****************************************************************************/
bool TimerEntry::FireTimer() 
{ 
	if (*invokeFunc_) 
	{ 
		lastFired_ = GetTickCount(); 
		(*invokeFunc_)(timerID_); 
		return true;
	} 
	return false;

}// TimerEntry::FireTimer
