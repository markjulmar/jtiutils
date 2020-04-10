/****************************************************************************/
//
// Synchronize.h
//
// This header describes various classes which wrap the Win32 synchronization
// functions in auto-create/destroy classes.
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

#ifndef __SYNCHRONIZE_H_INCL__
#define __SYNCHRONIZE_H_INCL__

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#ifndef _WINBASE_
	#define _WIN32_WINNT 0x0500
	#include <windows.h>
#endif

/*****************************************************************************/
// PC-Lint options
//
//lint -save
//
// Implicit conversions in constructor
//lint -esym(1931, EventSynch*, MutexSynch*)
//
// Data not initialized in constructor
//lint -esym(1927, EventSynch::Created, EventSynch::IsValid)
//lint -esym(1927, MutexSynch::Created, MutexSynch::IsValid)
//lint -esym(1927, SemaphoreSynch::Created, SemaphoreSynch::IsValid)
//
// Data not initialized in constructor
//lint -esym(1401, EventSynch::Created, EventSynch::IsValid)
//lint -esym(1401, MutexSynch::Created, MutexSynch::IsValid)
//lint -esym(1401, SemaphoreSynch::Created, SemaphoreSynch::IsValid)
//
// Data not initialized by assignment operator
//lint -esym(1539, EventSynch::Created, EventSynch::IsValid)
//lint -esym(1539, MutexSynch::Created, MutexSynch::IsValid)
//lint -esym(1539, SemaphoreSynch::Created, SemaphoreSynch::IsValid)
//
// Public data member
//lint -esym(1925, EventSynch::Created, EventSynch::IsValid)
//lint -esym(1925, MutexSynch::Created, MutexSynch::IsValid)
//lint -esym(1925, SemaphoreSynch::Created, SemaphoreSynch::IsValid)
//
// const member function indirectly modifies class
//lint -esym(1763, EventSynch::get)
//lint -esym(1763, MutexSynch::get) 
//lint -esym(1763, SemaphoreSynch::get) 

/*****************************************************************************/

namespace JTI_Util
{
/****************************************************************************/
// EventSynch
//
// Win32 Event object class wrapper
//
/****************************************************************************/
class EventSynch
{
// Class data
private:
	HANDLE hEvent_;
	bool wasCreatedByMe_;

// Constructor
public:
	EventSynch(bool bInitiallyOwn = false, bool bManualReset = false, LPCTSTR lpszName = NULL, LPSECURITY_ATTRIBUTES lpsaAttribute = NULL) : 
		hEvent_(NULL), wasCreatedByMe_(false)
	{
		hEvent_ = ::CreateEvent(lpsaAttribute, bManualReset ? TRUE : FALSE, bInitiallyOwn ? TRUE : FALSE, lpszName);
		wasCreatedByMe_ = (GetLastError()!=ERROR_ALREADY_EXISTS);
	}

	EventSynch(const EventSynch& rhs) : hEvent_(0), wasCreatedByMe_(rhs.wasCreatedByMe_) {
		if (rhs.hEvent_)
			::DuplicateHandle(GetCurrentProcess(), rhs.hEvent_, GetCurrentProcess(), &hEvent_, 0, FALSE, DUPLICATE_SAME_ACCESS); //lint !e534
	}
	EventSynch& operator=(const EventSynch& rhs) {
		if (this != &rhs)
		{
			wasCreatedByMe_ = rhs.wasCreatedByMe_;
			if (rhs.hEvent_)
				::DuplicateHandle(GetCurrentProcess(), rhs.hEvent_, GetCurrentProcess(), &hEvent_, 0, FALSE, DUPLICATE_SAME_ACCESS);  //lint !e534
			else
				hEvent_ = NULL;
		}
		return *this;
	}
	
	~EventSynch() 
	{	
		if (hEvent_)
		{
			::CloseHandle(hEvent_);  //lint !e534
			hEvent_ = NULL; 
		}
	}

// Operators
public:
	operator HANDLE () const { return get(); }

// Properties
public:
	__declspec(property(get=get_WasCreated)) bool Created;
	__declspec(property(get=get_IsValid)) bool IsValid;

// Operations
public:
	bool get_IsValid() const throw() { return (hEvent_!=NULL); }
	bool get_WasCreated() const throw() { return wasCreatedByMe_; }

// Operations
public:
	bool SetEvent() throw() { return (::SetEvent(hEvent_) == TRUE); }
	bool PulseEvent() throw() { return (::PulseEvent(hEvent_) == TRUE); }
	bool ResetEvent() throw() { return (::ResetEvent(hEvent_) == TRUE); }
	DWORD Wait(DWORD dwMsecs = INFINITE) const throw() {	return ::WaitForSingleObject(hEvent_,dwMsecs); }

// Operators
public:
	HANDLE get() const throw() { return hEvent_; }
};

/****************************************************************************/
// SemaphoreSynch
//
// Win32 Semaphore object class wrapper
//
/****************************************************************************/
class SemaphoreSynch
{
// Class data
private:
	HANDLE hSemaphore_;
	bool wasCreatedByMe_;

// Constructor
public:
	SemaphoreSynch(long InitialCount, long MaxCount, LPCTSTR lpszName = NULL, LPSECURITY_ATTRIBUTES lpsaAttribute = NULL) : 
		hSemaphore_(0), wasCreatedByMe_(false)
	{
		if (InitialCount < 0 || InitialCount > MaxCount) 
			InitialCount = 0;
		hSemaphore_ = ::CreateSemaphore(lpsaAttribute, InitialCount, MaxCount, lpszName);
		wasCreatedByMe_ = (::GetLastError() != ERROR_ALREADY_EXISTS);
	}
	SemaphoreSynch(const SemaphoreSynch& rhs) : hSemaphore_(0), wasCreatedByMe_(rhs.wasCreatedByMe_) {
		if (rhs.hSemaphore_)
			::DuplicateHandle(GetCurrentProcess(), rhs.hSemaphore_, GetCurrentProcess(), &hSemaphore_, 0, FALSE, DUPLICATE_SAME_ACCESS);  //lint !e534
	}
	SemaphoreSynch& operator=(const SemaphoreSynch& rhs) {
		if (this != &rhs)
		{
			wasCreatedByMe_ = rhs.wasCreatedByMe_;
			if (rhs.hSemaphore_)
				::DuplicateHandle(GetCurrentProcess(), rhs.hSemaphore_, GetCurrentProcess(), &hSemaphore_, 0, FALSE, DUPLICATE_SAME_ACCESS);  //lint !e534
			else
				hSemaphore_ = NULL;
		}
		return *this;
	}
	
	~SemaphoreSynch() 
	{
		if (hSemaphore_)
		{
			::CloseHandle(hSemaphore_);  //lint !e534
			hSemaphore_ = NULL; 
		}
	}

// Operators
public:
	operator HANDLE () const { return get(); }

// Properties
public:
	__declspec(property(get=get_WasCreated)) bool Created;
	__declspec(property(get=get_IsValid)) bool IsValid;

// Operations
public:
	bool get_IsValid() const throw() { return (hSemaphore_ != NULL); }
	bool get_WasCreated() const throw() { return wasCreatedByMe_; }
	DWORD Lock(DWORD dwMsecs = INFINITE) throw() {	return WaitForSingleObject(hSemaphore_,dwMsecs); }
	long Unlock(long Count=1) throw() { 
		if (Count <= 0) Count = 1;
		long OldCount = 0;
		if (::ReleaseSemaphore(hSemaphore_, Count, &OldCount) == FALSE)
			OldCount = 0;
		return OldCount;
	}

// Operators
public:
	HANDLE get() const throw() { return hSemaphore_; }
};

/****************************************************************************/
// MutexSynch
//
// Win32 Semaphore object class wrapper
//
/****************************************************************************/
class MutexSynch
{
// Class data
private:
	HANDLE hMutex_;
	bool wasCreatedByMe_;

// Constructor
public:
	MutexSynch(bool fInitiallyOwn = false, LPCTSTR lpszName = NULL, LPSECURITY_ATTRIBUTES lpsaAttribute = NULL) : 
		hMutex_(0), wasCreatedByMe_(false) 
	{
		hMutex_ = ::CreateMutex(lpsaAttribute, fInitiallyOwn ? TRUE : FALSE, lpszName);
		wasCreatedByMe_ = (::GetLastError() != ERROR_ALREADY_EXISTS);
	}
	MutexSynch(const MutexSynch& rhs) : hMutex_(0), wasCreatedByMe_(rhs.wasCreatedByMe_) {
		if (rhs.hMutex_)
			::DuplicateHandle(GetCurrentProcess(), rhs.hMutex_, GetCurrentProcess(), &hMutex_, 0, FALSE, DUPLICATE_SAME_ACCESS);  //lint !e534
	}
	MutexSynch& operator=(const MutexSynch& rhs) {
		if (this != &rhs)
		{
			wasCreatedByMe_ = rhs.wasCreatedByMe_;
			if (rhs.hMutex_)
				::DuplicateHandle(GetCurrentProcess(), rhs.hMutex_, GetCurrentProcess(), &hMutex_, 0, FALSE, DUPLICATE_SAME_ACCESS);  //lint !e534
			else
				hMutex_ = NULL;
		}
		return *this;
	}
	
	~MutexSynch() 
	{	
		if (hMutex_)
		{
			::CloseHandle(hMutex_);  //lint !e534
			hMutex_ = NULL; 
		}
	}

// Operators
public:
	operator HANDLE () const { return get(); }

// Properties
public:
	__declspec(property(get=get_WasCreated)) bool Created;
	__declspec(property(get=get_IsValid)) bool IsValid;

// Operations
public:
	bool get_IsValid() const throw() { return (hMutex_!=NULL); }
	bool get_WasCreated() const throw() { return wasCreatedByMe_; }
	DWORD Lock(DWORD dwMsecs = INFINITE) throw() {	return WaitForSingleObject(hMutex_,dwMsecs); }
	bool Unlock() throw() {	return (::ReleaseMutex(hMutex_) == TRUE); }

// Operators
public:
	HANDLE get() const throw() { return hMutex_; }
};

}  // namespace JTI_Utils

#endif // __SYNCHRONIZE_H_INCL__
