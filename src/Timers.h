/****************************************************************************/
//
// Timers.h
//
// This header implements a class for managing callback periodic timers
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

#ifndef __JTI_TIMERS_H_INCLUDED_
#define __JTI_TIMERS_H_INCLUDED_

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#ifndef _WINBASE_
	#define _WIN32_WINNT 0x0500
	#include <windows.h>
#endif
#include <vector>
#include <JTIUtils.h>
#include <SingletonRegistry.h>
#include <Delegates.h>
#include <Synchronization.h>
#include <stlx.h>

namespace JTI_Util
{
/******************************************************************************/
// TimerEntry
//
// This describes a single timer entry with delegate.
//
/******************************************************************************/
class TimerEntry
{
// Constructor
public:
	TimerEntry(int timerID, DWORD msecInterval, DelegateInvoke_1<int>* Func) : 
	  timerID_(timerID), lastFired_(GetTickCount()), msecInterval_(msecInterval), invokeFunc_(Func) {/* */}
    ~TimerEntry() { delete invokeFunc_;	}

// Operators
public:
	// Comparison
	bool operator==(const TimerEntry& rhs) const
	{
		return (timerID_ == rhs.timerID_ && invokeFunc_ == rhs.invokeFunc_);
	}

	// Sort by next timer
	bool operator<(const TimerEntry& rhs) const
	{
		return (get_NextFireTime() < rhs.get_NextFireTime());
	}

// Accessors
public:
	int get_ID() const { return timerID_; }
	// Calculate the next tick count when this timer should fire
	DWORD get_NextFireTime() const { return lastFired_ + msecInterval_; }
	// Return the last fire time
	DWORD get_LastFireTime() const { return lastFired_; }
	// Return the timer interval
	DWORD get_Interval() const { return msecInterval_; }
	// Execute the timer
	bool FireTimer();

// Class data
private:
	int timerID_;
	DWORD lastFired_;
	DWORD msecInterval_;
	DelegateInvoke_1<int>* invokeFunc_;

// Unavailable methods
private:
	TimerEntry(const TimerEntry& rhs);
	TimerEntry operator=(const TimerEntry& rhs);
};

/******************************************************************************/
// TimerManager
//
// This describes the timer manager class.  This class controls the invocation
// of timer events.
//
/******************************************************************************/
class TimerManager : public LockableObject<MultiThreadModel>
{
// Constructor
public:
	TimerManager() : thread_(0), evtStop_(false, true), evtNewTimer_(false, true) {/* */}
public:
	~TimerManager();

// Accessors
public:
	// Clear all timers
	void Clear();

	// Add a new timer
	template<class _Object, class _Class>
	void AddTimer(int nTimer, DWORD msecTimeout, const _Object& object, void (_Class::* fnc)(int))
	{
		AddTimer(nTimer, msecTimeout, JTI_NEW DelegateInvokeMethod_1<_Class,int>(*(_Object*)&object, fnc));
	}
	void AddTimer(int nTimer, DWORD msecTimeout, void (*fnc)(int))
	{
		AddTimer(nTimer, msecTimeout, JTI_NEW DelegateInvokeFunc_1<int>(fnc));
	}

	// Remove an existing timer.  This removes the 1st timer in the list matching the id.
	bool KillTimer(int nTimer);

// Internal functions
private:
	void AddTimer(int nTimer, DWORD msecTimeout, DelegateInvoke_1<int>* Func);

	// Timer Worker thread
	void TimerWorker();
	static unsigned __stdcall TimerWorkerThread(void* p)
	{
		reinterpret_cast<TimerManager*>(p)->TimerWorker();
		return 0;
	}

// Unavailable functions
private:
	TimerManager(const TimerManager&);
	TimerManager& operator==(const TimerManager&);

// Class data
private:
	std::vector<TimerEntry*> qTimers_;
	HANDLE thread_;
	EventSynch evtStop_;
	EventSynch evtNewTimer_;
};

}// namespace JTI_Util

#endif // __JTI_TIMERS_H_INCLUDED_