/****************************************************************************/
//
// Longevity.cpp
//
// This object heiarchy maintains relationships between object types and
// enforces specific destruction ordering for globals and singletons.
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
#include "longevity.h"
#include "Lock.h"

using namespace JTI_Util;

/*----------------------------------------------------------------------------
    GLOBALS
-----------------------------------------------------------------------------*/
namespace {
	// This structure manages the linked list for the lifetime tracking array
	struct LinkedListEntry
	{
		LifetimeTracker_Base* pt;
		LinkedListEntry* pn;
		LinkedListEntry() : pt(0), pn(0) {/* */}
	};
	static LinkedListEntry* _head = 0;
}

/*****************************************************************************
** Procedure:  _getLock
** 
** Arguments:  void
** 
** Returns: Pointer to critical section
** 
** Description: This function returns a critical section, guarenteed to 
**              be initialized.
**
/****************************************************************************/
static CriticalSectionLock* _getLock()
{
	static CriticalSectionLock gcsLock_;
	return &gcsLock_;

}// _getLock

/*****************************************************************************
** Procedure:  _AtExitFn
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: This function is set through an atexit() hook to destroy
**              objects in the Lifetime tracker array.  It is called once
**              for every object added to the lifetime array.
**
/****************************************************************************/
static void _AtExitFn()
{
	CCSLock<CriticalSectionLock> lockGuard(_getLock());
	if (_head != NULL)
	{
		// Get the current head and then assign to the next entry.
		LinkedListEntry* top = _head;
		_head = top->pn;
		
		// Delete the entry and the link itself.  The deletion of the 
		// entry will invoke the proper Deleter policy function to correctly
		// destroy the matching object.
		delete top->pt;
		delete top;
	}

}// _AtExitFn

/*****************************************************************************
** Procedure:  LifetimeTracker_Base::AddTrackedItem
** 
** Arguments:  p - New object to track
** 
** Returns: true/false success
** 
** Description: This function adds a new item to the tracked list.
**
/****************************************************************************/
bool LifetimeTracker_Base::AddTrackedItem(LifetimeTracker_Base* p)
{
	// Create our entry
	LinkedListEntry* ple = new LinkedListEntry;
	ple->pt = p;

	// The chain is kept in lowest longevity order; insert the
	// new element into the proper position.
	CCSLock<CriticalSectionLock> lockGuard(_getLock());
	LinkedListEntry* curr = _head, *prev = NULL;
	while (curr != NULL)
	{
		// Found our insertion position
		if (curr->pt->longevity_ > p->longevity_)
		{
			// At the head of the chain
			if (prev == NULL)
			{
				ple->pn = _head;
				_head = ple;
			}
			else
			{
				ple->pn = curr;
				prev->pn = ple;
			}
			break;
		}
		else
		{
			prev = curr;
			curr = curr->pn;
		}

	}

	// If we did not find a position, then insert at the end of the chain
	if (curr == NULL)
	{
		if (prev != NULL)
			prev->pn = ple;
		else
		{
			ple->pn = _head;
			_head = ple;
		}
	}

	// Add an exit function to delete this element.
	atexit(_AtExitFn); //lint !e534
	return true;

}// LifetimeTracker_Base::AddTrackedItem

