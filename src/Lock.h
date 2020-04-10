/****************************************************************************/
//
// Lock.h
//
// This header describes various classes which may be used in exclusion
// access for a user of the classes.
//
// Copyright (C) 1997-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

#ifndef __JTI_LOCK_H_INCLUDED_
#define __JTI_LOCK_H_INCLUDED_

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#ifndef _WINBASE_
	#define _WIN32_WINNT 0x0500
	#include <winbase.h>
#endif

/*****************************************************************************/
// PC-Lint options
//
//lint -save
//
// Not checking for assignment to this
//lint -esym(1529, PrimitiveLockImpl::operator=, CriticalSectionLockImpl::operator=)
//
// Not assigning variable in constructor
//lint -esym(1539, CriticalSectionLockImpl::_cs)
//lint -esym(1539, LockableObject*::_cs)
//
// Implicit conversions in constructor
//lint -esym(1931, LockingProxy*, CCSLock*)
//
// Ignore the return value
//lint -esym(534, InterlockedExchange)
//
/*****************************************************************************/

namespace JTI_Util
{
/******************************************************************************/
// NopLockImpl
//
// This implements a "non" locking object which can be added to the
// data object to ignore all lock/unlock attempts. This assumes that
// the objects are handling their synchronization in some other fashion, or
// that the access is single-threaded (i.e. STA).
//
/******************************************************************************/
//lint --e{1762} suppress "function could be made const"
class NopLockImpl
{
public:
#if _WIN32_WINNT >= 0x0500
	inline bool TryLock() throw() {return true;}
#endif
	inline void Lock() throw() {}
	inline void Unlock() throw() {}
};

/******************************************************************************/
// PrimitiveLockImpl
//
// This implements a primitive locking object which can be added to the
// data object to provide basic lock/unlock mechanics.  Warning, this lock
// is not capable of being re-entered by the same thread as it doesn't stack
// the unlocks correctly for single-thread re-entry.
//
/******************************************************************************/
class PrimitiveLockImpl
{
// Class data
private:
	volatile long lock_;
	enum { LOCK_FREE=0 };

// Constructor
public:
	PrimitiveLockImpl() : lock_(LOCK_FREE) {}
	PrimitiveLockImpl(const PrimitiveLockImpl&) : lock_(LOCK_FREE) {/* */}
	PrimitiveLockImpl& operator=(const PrimitiveLockImpl&) { lock_ = LOCK_FREE; return *this; }
	~PrimitiveLockImpl() {/* */}

	inline bool TryLock() throw() { 
		long lKey = static_cast<long>(GetCurrentThreadId());
		return (::InterlockedCompareExchange(&lock_, lKey, static_cast<long>(LOCK_FREE)) == lKey);
	}
	inline void Lock() throw() {
		long lKey = static_cast<long>(GetCurrentThreadId());
		while (::InterlockedCompareExchange(&lock_, lKey, static_cast<long>(LOCK_FREE)) != lKey)
			;
	}
	inline void Unlock() throw() { ::InterlockedExchange(&lock_, LOCK_FREE); }
};

/******************************************************************************/
// CriticalSectionLockImpl
//
// This implements a critical section locking object which can be used to
// associate a Win32 critical section with each required lock object.
//
/******************************************************************************/
class CriticalSectionLockImpl
{
private:
	CRITICAL_SECTION _cs;
public:
	CriticalSectionLockImpl() { InitializeCriticalSection(&_cs); }
	~CriticalSectionLockImpl() { DeleteCriticalSection(&_cs); }
	CriticalSectionLockImpl(const CriticalSectionLockImpl&) { InitializeCriticalSection(&_cs); }
	CriticalSectionLockImpl& operator=(const CriticalSectionLockImpl&) { return *this; }

#if _WIN32_WINNT >= 0x0500
	inline bool TryLock() throw() { return (TryEnterCriticalSection(&_cs)!=0); }
#endif
	inline void Lock() throw() { EnterCriticalSection(&_cs); }
	inline void Unlock() throw() { LeaveCriticalSection(&_cs); }
};

/******************************************************************************/
// LockModelPolicy
//
// This template implements the basic locking policy format.
//
/******************************************************************************/
template <class LockType>
class LockModelPolicy
{
public:
	typedef LockType CriticalSection;

	static long WINAPI Increment(long* plRef) throw() {
		// Simple increment of object count
		return InterlockedIncrement(plRef);
	}
	static long WINAPI Decrement(long* plRef) throw() {
		// Single decrement of object count
		return InterlockedDecrement(plRef);
	}
};

/******************************************************************************/
// SingleThreadModel
//
// This implements a basic non-threaded model for object protection.
// It still provides MT protection for Inc/Dec but no critical section.
//
/******************************************************************************/
typedef LockModelPolicy<NopLockImpl> SingleThreadModel;

/******************************************************************************/
// SimpleMultiThreadModel
//
// This implements a basic multi-threaded model for object protection.
// It does not support re-entry by the same thread, but provides lower 
// overhead than a full critical section.
//
/******************************************************************************/
typedef LockModelPolicy<PrimitiveLockImpl> SimpleMultiThreadModel;

/******************************************************************************/
// MultiThreadModel
//
// This implements a basic threaded model for object protection.
//
/******************************************************************************/
typedef LockModelPolicy<CriticalSectionLockImpl> MultiThreadModel;

/****************************************************************************/
// LockableObject
//
// This class defines a lockable base class for use by each of the
// data holder objects.
//
/****************************************************************************/
template <class _LockType = MultiThreadModel>
class LockableObject
{
// Class data
private:
	typename _LockType::CriticalSection _cs;

// Constructor/Destructor
public:
	//lint -e(1926) Ignore default construction of _cs type
	LockableObject() {}
	virtual ~LockableObject() = 0;

// Functions
public:
#if _WIN32_WINNT >= 0x0500
	inline bool TryLock() throw() { return _cs.TryLock(); }
#endif
	inline void Lock() throw() { _cs.Lock(); }
	inline void Unlock() throw() { _cs.Unlock(); }
};

// Virtual DTORs must be implemented - even if they are abstract
template <class _LockType>
inline LockableObject<_LockType>::~LockableObject() {}

/******************************************************************************/
// LockingProxy
//
// This class wraps a locking object and allows underlying methods to be
// called on the object locking/unlocking them as they are used.  This
// provides function-level locking.  This is used by the CRefPtr class.
//
/******************************************************************************/
template <class T>
class LockingProxy
{
// Class data
private:
	T* p_;
// Constructor
public:
	LockingProxy(T* p) : p_(p) { p_->Lock(); }
	~LockingProxy() { p_->Unlock(); }
	T* operator->() const { return p_; }
private:
	// Disallow copying the proxy
	LockingProxy(const LockingProxy&);
	LockingProxy& operator=(const LockingProxy&);
};

/******************************************************************************/
// CriticalSectionLock
//
// This exposes the critical section as a stand-alone usable object.
// This may be used by the CCSLock wrapper directly if desired.
//
/******************************************************************************/
typedef CriticalSectionLockImpl CriticalSectionLock;

/******************************************************************************/
// CCSLock
//
// Exception-safe locking class which works with any object exposing 
// Lock/Unlock.  This allows an object to be locked for longer than a single
// function call.
//
/******************************************************************************/
template <class T>
class CCSLock
{
private:
	T* p_;
	long lockCount_;
public:
	CCSLock(const T* pob, bool init_lock=true) throw() : 
		p_(const_cast<T*>(pob)), lockCount_(0) { if (init_lock) Lock(); }

	~CCSLock() { 
		while (lockCount_ > 0) 
			Unlock(); 
		p_ = NULL; 
	}

#if _WIN32_WINNT >= 0x0500
	inline bool TryLock() throw() {
		if (p_) {
			if (p_->TryLock()) {
				++lockCount_;
				return true;
			}
		}
		return false;
	}
#endif
	inline void Lock() throw() {
		if (p_) {
			++lockCount_;
			p_->Lock();
		}
	}
	inline void Unlock() throw() {
		if (p_ && lockCount_ > 0) {
			--lockCount_;
			p_->Unlock();
		}
	}

// Unavailable methods
private:
	CCSLock(const CCSLock&);
	CCSLock& operator=(const CCSLock&);
};

/******************************************************************************/
// IncDecHolder
//
// Exception-safe increment/decrement class for keeping a counter based on
// a work item.
//
/******************************************************************************/
class IncDecHolder
{
// Class data
private:
	volatile long * plValue_;

// Constructor/Destructor
public:
	IncDecHolder(volatile long* plValue) : plValue_(plValue) { InterlockedIncrement(plValue_); }
	~IncDecHolder() { InterlockedDecrement(plValue_); }

// Unavailable methods
private:
	IncDecHolder(const IncDecHolder&);
	IncDecHolder& operator=(const IncDecHolder&);
};

}// namespace JTI_Util

#endif // __JTI_LOCK_H_INCLUDED_
