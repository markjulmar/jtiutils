/****************************************************************************/
//
// Observer.h
//
// This set of classes implement the observer pattern ala Gamma, et al.
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

#ifndef _OBSERVER_INC_H_
#define _OBSERVER_INC_H_

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#pragma warning (disable:4702)
#include <list>
#include <algorithm>
#pragma warning (default:4702)
#include <Lock.h>
#include <Stlx.h>

namespace JTI_Util
{
// Define the default lock type based on what's being compiled.
#ifdef _MT
	#define JTI_OBSERVERLST_LOCK_TYPE SingleThreadModel
#else
	#define JTI_OBSERVERLST_LOCK_TYPE MultiThreadModel
#endif

/****************************************************************************/
// ObserverList
//
// This class implements the class observer pattern.  It holds a list of 
// objects which can be "invoked" by the client in a thread-safe fashion.
//
// Usage:
//
// class Observer {};
// class Ob_Event2 {};
// static void StaticFunc(Observer* p);

// ObserverList<Observer*> obList;
//
// obList.add(new Observer1);
// obList.add(new Observer2);
//
// obList.invoke(std::mem_fun(&Observer::Event));
// obList.invoke(std::bind2nd(std::mem_fun(&Observer::Event1), 10));
// obList.invoke(Ob_Event2("Test"));
// obList.invoke(std::ptr_fun(&StaticFunc));
//
/****************************************************************************/
template <class _Observer, bool mustDelete = false, class _LockType = JTI_OBSERVERLST_LOCK_TYPE>
class ObserverList : 
	public LockableObject<_LockType>
{
// Constructor
public:
	ObserverList() {/* */}
	virtual ~ObserverList() { clear(); }

// Methods
public:
	void add(const _Observer& ob)
	{
		CCSLock<ObserverList> lockGuard(this);
		lstObservers_.push_front(ob);
	}

	bool remove(const _Observer& ob)
	{
		CCSLock<ObserverList> lockGuard(this);
		if (std::find(lstObservers_.begin(), lstObservers_.end(), ob) != lstObservers_.end()) {
			lstObservers_.remove(ob);
			return true;
		}
		return false; // does not exists
	}

	void clear()
	{
		CCSLock<ObserverList> lockGuard(this);
		DeleteObservers(Int2Type<mustDelete>());
		lstObservers_.clear();
	}

	// Invoke function
	template <class _Func>
	void invoke(_Func func)
	{
		CCSLock<ObserverList> lockGuard(this);
		if (!lstObservers_.empty()) {
			std::list<_Observer> lst;
			std::copy(lstObservers_.begin(), lstObservers_.end(), std::front_inserter(lst));
			lockGuard.Unlock();
			std::for_each(lst.begin(), lst.end(), func);
		}
	}

// Internal functions
private:
	void DeleteObservers(Int2Type<true>)
	{
		std::for_each(lstObservers_.begin(), lstObservers_.end(), 
			stdx::delptr<_Observer>());
	}

	void DeleteObservers(Int2Type<false>)
	{
		/* Do nothing */
	}

// Class data
private:
	std::list<_Observer> lstObservers_;
};

}// namespace JTI_Util

#endif // _OBSERVER_INC_H_
