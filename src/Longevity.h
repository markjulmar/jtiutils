/****************************************************************************/
//
// Longevity.h
//
// This object hierarchy maintains relationships between object types and
// enforces specific destruction ordering for globals and singletons.
//
// This class was broken into two classes (a base and implementation) to use
// policies for the deletion mechanism.  This was taken from ideas presented by 
// Andrei Alexandrescu and his LOKI class framework which utilizes policies to 
// implement lifetime tracking.
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
//
/****************************************************************************/

#ifndef __LONGEVITY_H_INCL__
#define __LONGEVITY_H_INCL__

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#pragma warning(disable:4571)
#include <list>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>
#pragma warning(default:4571)
#include <JtiUtils.h>

/*****************************************************************************/
// PC-Lint options
//
//lint -save
//
// Implicit conversion allowed in constructor
//lint -esym(1931, LifetimeTracker_Base*)
//
/*****************************************************************************/

namespace JTI_Util
{   
/****************************************************************************/
// LifetimeTracker_Base
//
// The LifetimeTracker is the basic object used to track an object's lifetime.
// It is a simple numeric identity where the higher the numeric value, the
// longer that object will exist.
//
/****************************************************************************/
class LifetimeTracker_Base
{
public:
	virtual ~LifetimeTracker_Base() {}
	static bool AddTrackedItem(LifetimeTracker_Base* p);
protected:
	LifetimeTracker_Base(unsigned int x) : longevity_(x) {/* */}
private:
	unsigned int longevity_;
};

/****************************************************************************/
// CRTDeleter
//
// This provides a "normal" C++ CRT delete function for the LifetimeTracker
// wrapper to use for deleting it's contained object.
//
/****************************************************************************/
template <typename T>
struct CRTDeleter
{
	void operator()(T* p)
	{
		delete p;
	}
};

/****************************************************************************/
// CallbackDeleter
//
// This provides a callback deleter function to delete a given type.
//
/****************************************************************************/
template <typename T>
struct CallbackDeleter
{
	void operator()(T* p) 
	{ 
		pf_(p); 
	}
	
	typedef void (*VFUNCPTR)(T*);
	VFUNCPTR pf_;
	CallbackDeleter(VFUNCPTR pf) : pf_(pf) {/* */}
};

/****************************************************************************/
// LifetimeTracker
//
// This is the concrete implementation of the lifetime tracker object.
// It provides for overriding the method of destruction (the default is the
// object deleter above).
//
// Usage:
//
// class A {}
// A* pa = new A(), *pb = new A();
//
// LifetimeTracker<A>::SetLongevity(pa, 100);
// LifetimeTracker<A>::SetLongevity(pb, 50);
//
// Destruction order will be "pb" followed by "pa".
//
// Note that you cannot unregister a deletion, so make sure to not delete
// any objects registered with the LifetimeTracker<> template directly.
//
/****************************************************************************/
template <typename T, typename D = CRTDeleter<T> >
class LifetimeTracker : public LifetimeTracker_Base
{
public:
	virtual ~LifetimeTracker() { _destroyer(_p); }

	// This function sets the longevity for a tracked object and provides the
	// mechanism to destroy the object when it is finally reclaimed.
	static void SetLongevity(T* p, unsigned int longevity, D d = CRTDeleter<T>())
	{
		if (!LifetimeTracker_Base::AddTrackedItem(JTI_NEW LifetimeTracker<T,D>(p,longevity,d)))
			throw std::bad_alloc();
	}

// Constructor
private:
	LifetimeTracker(T* p, unsigned int longevity, D d) : 
		LifetimeTracker_Base(longevity), _p(p), _destroyer(d) 
	{
	}

// Class data
private:
	T* _p;
	D _destroyer;
};
}// namespace JTI_Util

#endif // __LONGEVITY_H_INCL__
