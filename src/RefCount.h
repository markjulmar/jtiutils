/****************************************************************************/
//
// RefCount.h
//
// This header describes various classes which may be used in reference counting
// or usage counting for various object types.
//
// Copyright (C) 1997-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

#ifndef __REFCOUNT_H_INCL__
#define __REFCOUNT_H_INCL__

/*****************************************************************************/
// PC-Lint options
//
//lint -save
//
// Supress duplicate header include files
//lint -e537
//
// Allow implicit conversions to constructors
//lint -esym(1931, UsageCountedObject*, DefaultStoragePolicy*, LockingStoragePolicy*, CRefPtr*, CCSRefHolder*)
//
// Private constructors
//lint -esym(1704, RefCountedObject*)
//
/*****************************************************************************/

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#ifndef _WINBASE_
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0500
	#endif
	#include <winbase.h>
#endif
#include <stdexcept>
#include <JTIUtils.h>
#include <Lock.h>
#include <TraceLogger.h>

namespace JTI_Util
{
// Define the default lock type based on what's being compiled.
#ifdef _MT
	#define JTI_REFCOUNT_LOCK_TYPE SingleThreadModel
#else
	#define JTI_REFCOUNT_LOCK_TYPE MultiThreadModel
#endif

/****************************************************************************/
// RefCountedObject
//
// This class defines a reference counted base class which can be derived
// from in order to create a reference counted object.  This is a more
// efficient approach than the below UsageCountedObject class as the count is
// included within the object being counted.  This saves the allocation
// of the reference count off the heap.
//
// This object assumes the same interface as COM.
//
/****************************************************************************/
template <class _LockType = JTI_REFCOUNT_LOCK_TYPE>
class RefCountedObject
{
// Class data
protected:
	mutable long _RefCount;

// Constructor
public:
	RefCountedObject() : _RefCount(1) {/* */}

// Private constructor - can only be deleted by Release() call or
// called by derived types.
protected:
	virtual ~RefCountedObject() {/* */}

// Access methods
public:
	// This method isused to increment the reference counter.
	void AddRef() const 
	{ 
		_LockType::Increment(&_RefCount); 
	}

	// This method is used to decrement the reference count.  
	// If it hits zero, the held object is deleted.
	bool Release() const 
	{
		if (_LockType::Decrement(&_RefCount) == 0) {
			delete this;
			return true;
		}
		return false;
	}
	
	// This swaps the reference count for an object with another object
	void Swap(RefCountedObject& rhs) { std::swap(_RefCount, rhs._RefCount); }

// Unavailable methods; violates ref counting.
private:
	RefCountedObject(const RefCountedObject&);
	RefCountedObject& operator=(const RefCountedObject&);
};

/****************************************************************************/
// UsageCountedObject
//
// This class can be used to implement a copy-on-write usage counted object.
// This seperates the refCount away from the data itself (which is in contrast
// to the above class which combines them).
//
// This class is used to manually implement handle classes when the interface
// is going to be changed, or the full "value-type" experience is desired.
//
/****************************************************************************/
template <class _Ty, class _LockType = JTI_REFCOUNT_LOCK_TYPE>
class UsageCountedObject
{
// Class data
protected:
	typedef _Ty _PtrType;
private:
	_PtrType* _p;
	long* _pnCount;

// Constructor
public:
	UsageCountedObject() : _pnCount(JTI_NEW long(1)), _p(0) {/* */}
	UsageCountedObject(_PtrType* p) : _pnCount(JTI_NEW long(1)), _p(p) {/* */}
	explicit UsageCountedObject(const UsageCountedObject& cnt) : _p(cnt._p), _pnCount(cnt._pnCount) { _LockType::Increment(_pnCount); }
	~UsageCountedObject() { 
		if (_LockType::Decrement(_pnCount) == 0) {
			delete _pnCount;
			delete _p;
		}
	}

// Friend function which allows underlying ptr access when absolutely necessary; note this is dangerous!
public:
	friend _PtrType* get_UnderlyingPointer(UsageCountedObject<_Ty,_LockType>& sp) { return sp._p; }

// Access methods
protected:
	_PtrType* ptr() const { return _p; }

	// Use this method to change the underlying pointer
	void Assign(_PtrType* p)
	{
		if (_LockType::Decrement(_pnCount) == 0)
		{
			*_pnCount = 1;
			delete _p;
		}
		else
			_pnCount = JTI_NEW long(1);
		_p = p;
	}

	// Use this method to assign this use count to a different use count in an assignment.
	void Assign(const UsageCountedObject& cnt)
	{
		// Increment the new use count so it doesn't go away.
		_LockType::Increment(cnt._pnCount);
		
		// Decrement the old use count and delete if necessary.  Then reattach to the new use count.
		if (_LockType::Decrement(_pnCount) == 0) 
		{
			delete _pnCount;
			delete _p;
		}
		_pnCount =  cnt._pnCount;
		_p = cnt._p;
	}

// Unavailable methods
private:
	UsageCountedObject& operator=( const UsageCountedObject& cnt );
};

/****************************************************************************/
// RefCountedPolicy
//
// This class defines a reference counted policy class based on the above
// base class which is used by the smart pointer wrapper class (CRefPtr).
// This is a required parameter to the CRefPtr template.
//
// The interface and concept was (to my knowledge) introduced by 
// Andrei Alexandrescu and presented in a C++ Report article and then later
// expanded in his book "Modern C++ Design".
//
/****************************************************************************/
template <class _Ty>
class RefCountedPolicy
{
// Accessors
public:
	// Copies the object
	static _Ty* Clone(const _Ty* p) {
		if (p) p->AddRef();
		return const_cast<_Ty*>(p);
	}
	// Releases the object; return true if the object should
	// be deleted by the wrapper class.
	static bool Release(const _Ty* p) {
		if (p)p->Release();
		return false;
	}
	// Swaps the refCount with another object
	static void Swap(RefCountedPolicy&) {/* */}
};

/****************************************************************************/
// COMUsagePolicy
//
// This class defines a usage policy for COM objects and is usable by the
// smart pointer wrapper class (CRefPtr).
// This is a required parameter to the CRefPtr template.
//
// The interface and concept was (to my knowledge) introduced by 
// Andrei Alexandrescu and presented in a C++ Report article and then later
// expanded in his book "Modern C++ Design".
//
/****************************************************************************/
template <class _Ty>
class COMUsagePolicy : public RefCountedPolicy<_Ty>
{
	/* Same as RefCountedPolicy */
};

/****************************************************************************/
// UsageCountedPolicy
//
// This class defines a non-intrusive usage counted policy class which
// is used by the smart pointer wrapper class (CRefPtr).
// This is a required parameter to the CRefPtr template.
//
// The interface and concept was (to my knowledge) introduced by 
// Andrei Alexandrescu and presented in a C++ Report article and then later
// expanded in his book "Modern C++ Design".
//
/****************************************************************************/
template <class _Ty>
class UsageCountedPolicy
{
// Class data
private:
	long* _pRefCount;

// Constructor
public:
	UsageCountedPolicy() { _pRefCount = JTI_NEW long(1); }
	UsageCountedPolicy(const UsageCountedPolicy& rhs) : _pRefCount(rhs._pRefCount) {/* */}

// Access methods
public:
	// This method is used to copy the object
	_Ty* Clone(const _Ty* p) {
		::InterlockedIncrement(_pRefCount);
		return const_cast<_Ty*>(p);
	}
	// Releases the object; return true if the object should
	// be deleted by the wrapper class.
	bool Release(const _Ty*) { 
		if (!::InterlockedDecrement(_pRefCount)) {
			delete _pRefCount;
			return true;
		}
		return false;
	}
	// Swaps the refCount with another object
	void Swap(UsageCountedPolicy& rhs) { std::swap(_pRefCount, rhs._pRefCount); }
};

/****************************************************************************/
// DefaultStoragePolicy
//
// The storage policy is used to manage the raw pointer type which is held
// by the CRefPtr class.  This is a required parameter to the CRefPtr template.
//
// The interface and concept was (to my knowledge) introduced by 
// Andrei Alexandrescu and presented in a C++ Report article and then later
// expanded in his book "Modern C++ Design".
//
/****************************************************************************/
template <class T>
class DefaultStoragePolicy
{
// Typedefs
protected:
	typedef T* _PtrType;		// The type returned by operator->
	typedef T& _RefType;		// The type returned by operator*
	typedef T* _StoredType;		// The raw pointer type
// Class data
private:
	_StoredType _p;

// Constructor
public:
	DefaultStoragePolicy() : _p(0) {/* */}
	DefaultStoragePolicy(const _StoredType& p) : _p(p) {/* */}

// Operators
public:
	_PtrType operator->() const { return _p; }
	_RefType operator*() const { return *_p; }

// Access methods
protected:
	_StoredType ptr() const { return _p; }
	_StoredType& ref() const { return _p; }
	bool IsValid() const { return (_p != 0); }
	void Destroy() { delete _p; _p = 0; }
	void Swap(DefaultStoragePolicy& lp) { std::swap(_p, lp._p);	}
};

/****************************************************************************/
// LockingStoragePolicy
//
// The storage policy is used to manage the raw pointer type which is held
// by the CRefPtr class.  This is a required parameter to the CRefPtr template.
//
// The interface and concept was (to my knowledge) introduced by 
// Andrei Alexandrescu and presented in a C++ Report article and then later
// expanded in his book "Modern C++ Design".
//
/****************************************************************************/
template <class T>
class LockingStoragePolicy
{
// Typedefs
protected:
	typedef LockingProxy<T> _PtrType;	// The type returned by operator->
	typedef T& _RefType;		// The type returned by operator*
	typedef T* _StoredType;	// The raw pointer type
// Class data
private:
	_StoredType _p;

// Constructor
public:
	LockingStoragePolicy() : _p(0) {/* */}
	LockingStoragePolicy(const _StoredType& p) : _p(p) {/* */}

// Operators
public:
	_PtrType operator->() const { return LockingProxy<T>(_p); }
	_RefType operator*() const { return *_p; }

// Access methods
protected:
	_StoredType ptr() const { return _p; }
	_StoredType& ref() const { return _p; }
	bool IsValid() const { return (_p != 0); }
	void Destroy() { delete _p; _p = 0; }
	void Swap(LockingStoragePolicy& lp) { std::swap(_p, lp._p);	}
};

/****************************************************************************/
// CRefPtr
//
// This smart pointer class uses the supplied policy classes for determining
// how to reference count the object and how to store the pointer to the object
// and then implements a full wrapper class which allows the object to be
// passed around as a handle.
//
// This class doesn't perform the creation of the underlying type.  It requires
// that the user know the underlying type and how to create it.  It is, therefore
// only usable as a lifetime management class and not a true handle class.
//
// This concept was (to my knowledge) introduced by Andrei Alexandrescu and 
// presented in a C++ Report article and then later expanded in his book 
// "Modern C++ Design".
//
// The names are changed because this class replaced an existing class which
// did essentially the same thing but was not as extensible as the model presented
// by Alexandrescu.
//
/****************************************************************************/
template<class T, 
		 template <class> class _UsagePolicy = RefCountedPolicy,
		 template <class> class _StoragePolicy = DefaultStoragePolicy>

class CRefPtr : 
	public _UsagePolicy<T>,
	public _StoragePolicy<T>
{
// Class data
private:
	typename typedef _UsagePolicy<T> UP;
	typename typedef _StoragePolicy<T> SP;
	typename typedef SP::_PtrType _PtrType;
	typename typedef SP::_RefType _RefType;
	typename typedef SP::_StoredType _StoredType;
	typename typedef CRefPtr<T,_UsagePolicy,_StoragePolicy> _PtrInst;

// Constructors
public:
	CRefPtr(int null=0) { if (null != 0) throw std::runtime_error(std::string("Not a valid pointer")); }
	CRefPtr(_StoredType lp) : SP(lp) {/* */}
	CRefPtr(_StoredType lp, bool bumpRefCount) : SP((bumpRefCount) ? UP::Clone(lp) : lp) {/* */}
	CRefPtr(const _PtrInst& lp) : SP(UP::Clone(lp.ptr())), UP(lp) {/* */}
	~CRefPtr() { 
		if (SP::IsValid() && UP::Release(SP::ptr()))
			SP::Destroy();
	}

// Operators
public:
	// Assignment from a direct pointer
	_PtrInst& operator=(_StoredType rhs) { 
		CRefPtr temp(rhs);
		UP::Swap(temp);
		SP::Swap(temp);
		return *this;
	}

	// Assignment from another CRefPtr instance
	_PtrInst& operator=(const _PtrInst& rhs) {
		CRefPtr temp(rhs);
		UP::Swap(temp);
		SP::Swap(temp);
		return *this;
	}

	// De-referencing
	_RefType operator*() { 
		if (!SP::IsValid()) throw std::runtime_error(std::string("De-referenced null pointer"));
		return SP::operator*(); 
	}
	_RefType operator*() const { 
		if (!SP::IsValid()) throw std::runtime_error(std::string("De-referenced null pointer"));
		return SP::operator*(); 
	}

	// Auto conversion; allows this to be used directly as a type; this is required if the
	// smart-pointer is to be used with STL functor wrappers (i.e. std::mem_fun).  Unfortunately
	// this operator also allows for delete() to be called against the underlying pointer.
	// I cannot find any way to stop this behavior while still allowing for direct implicit
	// conversions to the native type.
	operator _PtrType() const { 
		// Do not throw an exception; this will also be called when "if (p)" syntax is used.
		return SP::operator->();
	}

	// Pointer de-reference
	_PtrType operator->() { 
		if (!SP::IsValid()) throw std::runtime_error(std::string("De-referenced null pointer"));
		return SP::operator->();
	}
	_PtrType operator->() const { 
		if (!SP::IsValid()) throw std::runtime_error(std::string("De-referenced null pointer"));
		return SP::operator->();
	}

	// Allow tests for null "if (!p)"
	bool operator!() const { return (!SP::IsValid()); }

	// Allow for direct tests with pointers
	inline friend bool operator==(const CRefPtr& lhs, const _PtrType rhs) { return lhs.ptr() == rhs; }
	inline friend bool operator==(const _PtrType lhs, const CRefPtr& rhs) { return rhs.ptr() == lhs; }
	inline friend bool operator!=(const CRefPtr& lhs, const _PtrType rhs) { return lhs.ptr() != rhs; }
	inline friend bool operator!=(const _PtrType lhs, const CRefPtr& rhs) { return rhs.ptr() != lhs; }

	// Allow for tests against derived pointer types.
	template <class U>
	inline friend bool operator==(const CRefPtr& lhs, const U* rhs) { return lhs.ptr() == rhs; }
	template <class U>
	inline friend bool operator==(const U* lhs, const CRefPtr& rhs) { return rhs.ptr() == lhs; }
	template <class U>
	inline friend bool operator!=(const CRefPtr& lhs, const U* rhs) { return lhs.ptr() != rhs; }
	template <class U>
	inline friend bool operator!=(const U* lhs, const CRefPtr& rhs) { return rhs.ptr() != lhs; }

	// Finally, an ambiguity resolver when the pointer types are not compatible but are both
	// wrapped by a CRefPtr<> instance.  In this case, both operator== operators could be used.
	template <class U>
	bool operator==(const CRefPtr<U>& rhs) const { return ptr() == rhs.ptr(); }
	template <class U>
	bool operator!=(const CRefPtr<U>& rhs) const { return ptr() != rhs.ptr(); }

	// Allow sorting within the STL containers; sort by pointer value (i.e. behave identically to a raw pointer).
	inline friend bool operator<(const _PtrInst& lhs, const _StoredType rhs) { return lhs.ptr() < rhs; }
	inline friend bool operator<(const _StoredType lhs, const _PtrInst& rhs) { return rhs.ptr() < lhs; }

// Accessor Methods
public:
	_StoredType get() const { return SP::ptr(); }
	bool IsValid() const { return SP::IsValid(); }
};

/****************************************************************************/
// CCSLockRef
//
// Class which adds reference counting to a lock;
//
// Usage:
//
// class A : public RefCountedObject<>, public LockableObject<> {};
// typedef CCSRefHolder<A> APtr;
//     ...
// APtr a1(new A);
// CCSLockRef<A> _guard(a1);
//   ... object locked until _guard goes out of scope.
//
/****************************************************************************/
template <class T,
		 template <class> class _UsagePolicy = RefCountedPolicy,
		 template <class> class _StoragePolicy = DefaultStoragePolicy>
class CCSLockRef : public CCSLock<T>
{
public:
	CRefPtr<T, _UsagePolicy, _StoragePolicy> ptr_; //lint !e1925
	CCSLockRef(const CRefPtr<T,_UsagePolicy,_StoragePolicy>& p, bool lockNow=true) : ptr_(p), CCSLock<T>(p.get(),lockNow) {/* */}
	CCSLockRef(T* p, bool lockNow=true) : ptr_(p,true), CCSLock<T>(p,lockNow) { /* */ }
};

/****************************************************************************/
// CCSRefHolder
//
// Class which holds a ref counted object alive until it is destroyed.
// This class is used with objects that derive from RefCountedObject<>
//
// This class was replaced by the CRefPtr<> implementation which does all
// the heavy lifting using policy classes.  See the above class definition
// for details.
//
// Usage:
//
// class A : public RefCountedObject<> {};
// typedef CCSRefHolder<A> APtr;
//        ...
// APtr a1(new A);
// a1->DoSomething();
//        ... a1 destroyed when out of scope.
//
// If passed to another thread - we cannot pass by value/reference, but
// must get dirty and pass as a pointer:
//
// a1->AddRef();
// _beginthreadex(0,0,&ThreadFunc,a1.get(),0, &tid);
//      ...
// unsigned __stdcall ThreadFunc(void* p) {
//	   APtr a1(reintpret_cast<A*>(p));
//
/****************************************************************************/
template <class T>
struct CCSRefHolder : public CRefPtr<T>
{
	CCSRefHolder() : CRefPtr<T>()
	{
	}

	CCSRefHolder(T* p, bool bumpRef=false) : CRefPtr<T>(p, bumpRef) 
	{
	}
};

}  // namespace JTI_Utils

#endif // __REFCOUNT_H_INCL__
