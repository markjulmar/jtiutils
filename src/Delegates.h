/****************************************************************************/
//
// Delegates.h
//
// This set of classes implement delegates similar to C# in the dotNet
// framework.
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

#ifndef _DELEGATES_INC_H_
#define _DELEGATES_INC_H_

/*****************************************************************************/
// PC-Lint options
//
//lint -save
//
// Allow implicit conversions in constructor
//lint -esym(1931, DelegateInvokeFunc*, find_delegate*, exec_delegate*)
//
// Allow C-style conversions
//lint -e1924
//
// Conversion operators are allowed
//lint -e1930
//
// Supress reference arguments
//lint -e1725 
//
// Supress duplicate header include files
//lint -e537
//
/*****************************************************************************/

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#pragma warning(disable:4571)
#include <list>
#include <algorithm>
#pragma warning(default:4571)
#include <JtiUtils.h>
#include <Lock.h>
#include <Stlx.h>

namespace JTI_Util
{
// Our function pointer type
#ifdef _lint
	typedef unsigned long DLPFN;
#else
	typedef ULONG_PTR DLPFN;
#endif

/****************************************************************************/
// DelegateInvoke
//
// This interface declares the simple no-parameter delegate.
//
/****************************************************************************/
class DelegateInvoke
{
public:
	virtual void operator()() = 0;
	virtual operator bool() const = 0;
	virtual bool Equal(DLPFN pfn, const void* pobj) const = 0;
};

/****************************************************************************/
// DelegateInvoke_1
//
// This interface declares the one-parameter delegate
//
/****************************************************************************/
template<class _Arg>
class DelegateInvoke_1
{
public:
	virtual void operator()(const _Arg&) = 0;
	virtual operator bool() const = 0;
	virtual bool Equal(DLPFN pfn, const void* pobj) const = 0;
};

/****************************************************************************/
// DelegateInvoke_2
//
// This interface declares the two-parameter delegate
//
/****************************************************************************/
template<class _Arg1, class _Arg2>
class DelegateInvoke_2
{
public:
	virtual void operator()(const _Arg1&, const _Arg2&) = 0;
	virtual operator bool() const = 0;
	virtual bool Equal(DLPFN pfn, const void* pobj) const = 0;
};

/****************************************************************************/
// DelegateInvoke_3
//
// This interface declares the three-parameter delegate
//
/****************************************************************************/
template<class _Arg1, class _Arg2, class _Arg3>
class DelegateInvoke_3
{
public:
	virtual void operator()(const _Arg1&, const _Arg2&, const _Arg3&) = 0;
	virtual operator bool() const = 0;
	virtual bool Equal(DLPFN pfn, const void* pobj) const = 0;
};

/****************************************************************************/
// DelegateInvokeFunc
//
// This class implements the static function no parameter delegate
//
/****************************************************************************/
class DelegateInvokeFunc : public DelegateInvoke
{
public:
	typedef void (*PFN)();
	DelegateInvokeFunc(PFN pfn) : pfn_(pfn) {/* */}
	virtual void operator()() { if (Valid()) pfn_(); }
	virtual operator bool() const { return Valid(); }
	virtual bool Equal(DLPFN pfn, const void* pobj) const { return (!pobj) && ((DLPFN)(*((DLPFN*)&pfn_)) == pfn); }
private:
	bool Valid() const { 
		union { PFN pfn; FARPROC pfnv; } tPtr; tPtr.pfn = pfn_;
		return !IsBadCodePtr(tPtr.pfnv); 
	}
	PFN pfn_;
};

/****************************************************************************/
// DelegateInvokeFunc_1
//
// This class implements the static single-parameter function delegate
//
/****************************************************************************/
template<class _Arg>
class DelegateInvokeFunc_1 : public DelegateInvoke_1<_Arg>
{
public:
	typedef void (*PFN)(_Arg);
	DelegateInvokeFunc_1(PFN pfn) : pfn_(pfn) {/* */}
	virtual void operator()(const _Arg& arg) { if (Valid()) pfn_(arg); }
	virtual operator bool() const { return Valid(); }
	virtual bool Equal(DLPFN pfn, const void* pobj) const { return (!pobj) && ((DLPFN)(*((DLPFN*)&pfn_)) == pfn); }
private:
	bool Valid() const { 
		union { PFN pfn; FARPROC pfnv; } tPtr; tPtr.pfn = pfn_;
		return !IsBadCodePtr(tPtr.pfnv); 
	}
	PFN pfn_;
};

/****************************************************************************/
// DelegateInvokeFunc_2
//
// This class implements the static dual-parameter function delegate
//
/****************************************************************************/
template<class _Arg1, class _Arg2>
class DelegateInvokeFunc_2 : public DelegateInvoke_2<_Arg1, _Arg2>
{
public:
	typedef void (*PFN)(_Arg1,_Arg2);
	DelegateInvokeFunc_2(PFN pfn) : pfn_(pfn) {/* */}
	virtual void operator()(const _Arg1& arg1, const _Arg2& arg2) { if (Valid()) pfn_(arg1, arg2); }
	virtual operator bool() const { return Valid(); }
	virtual bool Equal(DLPFN pfn, const void* pobj) const { return (!pobj) && ((DLPFN)(*((DLPFN*)&pfn_)) == pfn); }
private:
	bool Valid() const { 
		union { PFN pfn; FARPROC pfnv; } tPtr; tPtr.pfn = pfn_;
		return !IsBadCodePtr(tPtr.pfnv); 
	}
	PFN pfn_;
};

/****************************************************************************/
// DelegateInvokeFunc_3
//
// This class implements the static three-parameter function delegate
//
/****************************************************************************/
template<class _Arg1, class _Arg2, class _Arg3>
class DelegateInvokeFunc_3 : public DelegateInvoke_3<_Arg1,_Arg2,_Arg3>
{
public:
	typedef void (*PFN)(_Arg1,_Arg2,_Arg3);
	DelegateInvokeFunc_3(PFN pfn) : pfn_(pfn) {/* */}
	virtual void operator()(const _Arg1& arg1, const _Arg2& arg2, const _Arg3& arg3) { if (Valid()) pfn_(arg1, arg2, arg3); }
	virtual operator bool() const { return Valid(); }
	virtual bool Equal(DLPFN pfn, const void* pobj) const { return (!pobj) && ((DLPFN)(*((DLPFN*)&pfn_)) == pfn); }
private:
	bool Valid() const { 
		union { PFN pfn; FARPROC pfnv; } tPtr; tPtr.pfn = pfn_;
		return !IsBadCodePtr(tPtr.pfnv); 
	}
	PFN pfn_;
};

/****************************************************************************/
// DelegateInvokeMethod
//
// This class implements the member function no parameter instance method delegate
//
/****************************************************************************/
template<class _Class>
class DelegateInvokeMethod : public DelegateInvoke
{
public:
	typedef void (_Class::*PFN)();
	DelegateInvokeMethod(_Class& object, PFN pfn) :  object_(object), pfn_(pfn) {/* */}
	DelegateInvokeMethod(const DelegateInvokeMethod& rhs) : pfn_(rhs.pfn_), object_(rhs.object_) {/* */}
	DelegateInvokeMethod& operator=(const DelegateInvokeMethod& rhs) { pfn_ = rhs.pfn_; object_ = rhs.object_; return *this; }
	virtual void operator()() { if (Valid()) (object_.*pfn_)(); }
	virtual operator bool() const { return Valid(); }
	virtual bool Equal(DLPFN pfn, const void* pobj) const { return (&object_ == reinterpret_cast<const _Class*>(pobj)) && ((DLPFN)(*((DLPFN*)&pfn_)) == pfn); }
private:
	bool Valid() const { 
		union { PFN pfn; FARPROC pfnv; } tPtr; tPtr.pfn = pfn_;
		return !IsBadCodePtr(tPtr.pfnv); 
	}
	PFN pfn_;
	_Class& object_;
};

/****************************************************************************/
// DelegateInvokeMethod_1
//
// This class implements the member function single-parameter instance 
// method delegate
//
/****************************************************************************/
template<class _Class, class _Arg1>
class DelegateInvokeMethod_1 : public DelegateInvoke_1<_Arg1>
{
public:
	typedef void (_Class::*PFN)(_Arg1);
	DelegateInvokeMethod_1(_Class& object, PFN pfn) :  object_(object), pfn_(pfn) {/* */}
	DelegateInvokeMethod_1(const DelegateInvokeMethod_1& rhs) : pfn_(rhs.pfn_), object_(rhs.object_) {/* */}
	DelegateInvokeMethod_1& operator=(const DelegateInvokeMethod_1& rhs) { pfn_ = rhs.pfn_; object_ = rhs.object_; return *this; }
	virtual void operator()(const _Arg1& arg1) { if (Valid()) (object_.*pfn_)(arg1); }
	virtual operator bool() const { return Valid(); }
	virtual bool Equal(DLPFN pfn, const void* pobj) const { return (&object_ == reinterpret_cast<const _Class*>(pobj)) && ((DLPFN)(*((DLPFN*)&pfn_)) == pfn); }
private:
	bool Valid() const { 
		union { PFN pfn; FARPROC pfnv; } tPtr; tPtr.pfn = pfn_;
		return !IsBadCodePtr(tPtr.pfnv); 
	}
	PFN pfn_;
	_Class& object_;
};

/****************************************************************************/
// DelegateInvokeMethod_2
//
// This class implements the member function two-parameter 
// instance method delegate
//
/****************************************************************************/
template<class _Class, class _Arg1, class _Arg2>
class DelegateInvokeMethod_2 : public DelegateInvoke_2<_Arg1,_Arg2>
{
public:
	typedef void (_Class::*PFN)(_Arg1,_Arg2);
	DelegateInvokeMethod_2(_Class& object, PFN pfn) :  object_(object), pfn_(pfn) {/* */}
	DelegateInvokeMethod_2(const DelegateInvokeMethod_2& rhs) : pfn_(rhs.pfn_), object_(rhs.object_) {/* */}
	DelegateInvokeMethod_2& operator=(const DelegateInvokeMethod_2& rhs) { pfn_ = rhs.pfn_; object_ = rhs.object_; return *this; }
	virtual void operator()(const _Arg1& arg1, const _Arg2& arg2) { if (Valid()) (object_.*pfn_)(arg1,arg2); }
	virtual operator bool() const { return Valid(); }
	virtual bool Equal(DLPFN pfn, const void* pobj) const { return (&object_ == reinterpret_cast<const _Class*>(pobj)) && ((DLPFN)(*((DLPFN*)&pfn_)) == pfn); }
private:
	bool Valid() const { 
		union { PFN pfn; FARPROC pfnv; } tPtr; tPtr.pfn = pfn_;
		return !IsBadCodePtr(tPtr.pfnv); 
	}
	PFN pfn_;
	_Class& object_;
};

/****************************************************************************/
// DelegateInvokeMethod_3
//
// This class implements the member function triple-parameter
// instance method delegate
//
/****************************************************************************/
template<class _Class, class _Arg1, class _Arg2, class _Arg3>
class DelegateInvokeMethod_3 : public DelegateInvoke_3<_Arg1,_Arg2,_Arg3>
{
public:
	typedef void (_Class::*PFN)(_Arg1,_Arg2,_Arg3);
	DelegateInvokeMethod_3(_Class& object, PFN pfn) :  object_(object), pfn_(pfn) {/* */}
	DelegateInvokeMethod_3(const DelegateInvokeMethod_3& rhs) : pfn_(rhs.pfn_), object_(rhs.object_) {/* */}
	DelegateInvokeMethod_3& operator=(const DelegateInvokeMethod_3& rhs) { pfn_ = rhs.pfn_; object_ = rhs.object_; return *this; }
	virtual void operator()(const _Arg1& arg1, const _Arg2& arg2, const _Arg3& arg3) { if (Valid()) (object_.*pfn_)(arg1, arg2, arg3); }
	virtual operator bool() const { return Valid(); }
	virtual bool Equal(DLPFN pfn, const void* pobj) const { return (&object_ == reinterpret_cast<const _Class*>(pobj)) && ((DLPFN)(*((DLPFN*)&pfn_)) == pfn); }
private:
	bool Valid() const { 
		union { PFN pfn; FARPROC pfnv; } tPtr; tPtr.pfn = pfn_;
		return !IsBadCodePtr(tPtr.pfnv); 
	}
	PFN pfn_;
	_Class& object_;
};

//lint -e1932 supress non-abstract base classes for unary_function
//lint -e1505 supress no access specifier

/****************************************************************************/
// find_delegate
//
// This implements a simple algorithm for locating a delegate by type
//
/****************************************************************************/
//lint --e{1932} std::unary_function is not abstract
template <class T>
struct find_delegate : public std::unary_function<T*, bool>
{
	DLPFN pfn_; const void* pobj_;
	find_delegate(DLPFN pfn, const void* pobj=0) : pfn_(pfn), pobj_(pobj) {/* */}
	result_type operator()(T* pfunc) { return pfunc->Equal(pfn_, pobj_); }	//lint !e818
};

//lint +e1932 re-enable non-abstract base classes for unary_function
//lint +e1505 re-enable no access specifier

/****************************************************************************/
// Delegate
//
// This class implements the basic no parameter delegate
//
/****************************************************************************/
template <class _LockType>
class Delegate : public LockableObject<_LockType>
{
// Constructor
public:
	Delegate() {/* */}
	Delegate(const Delegate& rhs) : callList_(rhs.callList_) {/* */}
	virtual ~Delegate() { try	{ clear(); }catch (...) {/* */} }

// Methods
public:
	void operator()() const { 
		CCSLock<Delegate> lockGuard(this);
		std::for_each(callList_.begin(), callList_.end(), exec_delegate()); 
	}

	Delegate& operator=(const Delegate& rhs) {
		if (this != &rhs) {
			CCSLock<Delegate> lockGuard(&rhs);
			callList_ = rhs.callList_;
		}
		return *this;
	}
  
	template<class _Object, class _Class>
	void add(const _Object& object, void (_Class::* fnc)()) {
		CCSLock<Delegate> lockGuard(this);
		std::list<DelegateInvoke*>::iterator it = std::find_if(callList_.begin( ), callList_.end( ), 
			find_delegate<DelegateInvoke>((DLPFN)(*((DLPFN*)&fnc)),(void*)&object));
		if (it == callList_.end())
			callList_.push_back(JTI_NEW DelegateInvokeMethod<_Class>(*(_Object*)&object,fnc));
	}

	void add(void (*fnc)()) {
		CCSLock<Delegate> lockGuard(this);
		std::list<DelegateInvoke*>::iterator it = std::find_if(callList_.begin( ), callList_.end( ), find_delegate<DelegateInvoke>((DLPFN)fnc));
		if (it == callList_.end())
			callList_.push_back(JTI_NEW DelegateInvokeFunc(fnc));
	}

	template<class _Object, class _Class>
	void remove( const _Object& object, void (_Class::* fnc)( ) )
	{
		CCSLock<Delegate> lockGuard(this);
		std::list<DelegateInvoke*>::iterator it = std::find_if(callList_.begin( ), callList_.end( ), 
			find_delegate<DelegateInvoke>((DLPFN)(*((DLPFN*)&fnc)),(void*)&object));
		if( it != callList_.end( ) )
			callList_.erase( it );
	}      

	void remove( void (*fnc)( ) )
	{
		CCSLock<Delegate> lockGuard(this);
		std::list<DelegateInvoke*>::iterator it = std::find_if(callList_.begin( ), callList_.end( ), find_delegate<DelegateInvoke>((DLPFN)fnc));
		if (it != callList_.end()) {
			delete *it;
			callList_.erase(it);
		}
	}

	void clear()
	{
		CCSLock<Delegate> lockGuard(this);
		if (!callList_.empty())	{
			std::for_each(callList_.begin(),callList_.end(), stdx::delptr<DelegateInvoke*>());
			callList_.clear();
		}
	}

// Embedded classes
private:
	struct exec_delegate { void operator()(DelegateInvoke* func) { (*func)(); } };

// Class data
private:
	std::list<DelegateInvoke*> callList_;
};

/****************************************************************************/
// Delegate_1
//
// This class implements the single-parameter delegate
//
/****************************************************************************/
template<class _LockType, class _Arg>
class Delegate_1  : public LockableObject<_LockType>
{
// Constructor
public:
	Delegate_1() : LockableObject<_LockType>(), callList_() {/* */}
	virtual ~Delegate_1() {	clear(); }

// Methods
public:
	void operator()(const _Arg& arg) const { 
		CCSLock<Delegate_1> lockGuard(this);
		std::for_each(callList_.begin(),callList_.end(), exec_delegate(arg)); 
	}

	//lint --e{1539} Supress the warning that we do not assign the base CriticalSection; we don't want to copy that.
	Delegate_1& operator=(const Delegate_1& rhs) {
		if (this != &rhs) {
			CCSLock<Delegate_1> lockGuard(&rhs);
			callList_ = rhs.callList_;
		}
		return *this;
	}

	template<class _Object, class _Class>
	void add(const _Object& object, void (_Class::* fnc)(_Arg))
	{
		CCSLock<Delegate_1> lockGuard(this);
		std::list<DelegateInvoke_1<_Arg>*>::iterator it = std::find_if(callList_.begin(), callList_.end(), 
			find_delegate<DelegateInvoke_1<_Arg> >((DLPFN)(*((DLPFN*)&fnc)), (void*)&object));
		if (it == callList_.end())
			callList_.push_back(JTI_NEW DelegateInvokeMethod_1<_Class, _Arg>(*(_Object*)&object, fnc));
	}

	void add(void (*fnc)(_Arg))
	{
		CCSLock<Delegate_1> lockGuard(this);
		std::list<DelegateInvoke_1<_Arg>*>::iterator it = std::find_if(callList_.begin(), callList_.end(), find_delegate<DelegateInvoke_1<_Arg> >((DLPFN)fnc));
		if (it == callList_.end())
			callList_.push_back(JTI_NEW DelegateInvokeFunc_1<_Arg>(fnc));
	}

	template<class _Object, class _Class>
	void remove(const _Object& object, void (_Class::* fnc)(_Arg))
	{
		CCSLock<Delegate_1> lockGuard(this);
		std::list<DelegateInvoke_1<_Arg>*>::iterator it = std::find_if(callList_.begin(), callList_.end(), 
			find_delegate<DelegateInvoke_1<_Arg> >((DLPFN)(*((DLPFN*)&fnc)), (void*)&object));
		if (it != callList_.end()) {
			delete (*it);
			callList_.erase(it);
		}
	}      

	void remove( void (*fnc)( _Arg ) )
	{
		CCSLock<Delegate_1> lockGuard(this);
		std::list<DelegateInvoke_1<_Arg>*>::iterator it = std::find_if(callList_.begin(), callList_.end(), find_delegate<DelegateInvoke_1<_Arg> >((DLPFN)fnc));
		if (it != callList_.end()) {
			delete (*it);
			callList_.erase(it);	//lint !e534
		}
	}

	void clear()
	{
		CCSLock<Delegate_1> lockGuard(this);
		if (!callList_.empty())	{
			std::for_each(callList_.begin(),callList_.end(), stdx::delptr<DelegateInvoke_1<_Arg>*>());
			callList_.clear();
		}
	}

// Embedded classes
private:
	//lint -e{1725} ignore reference class member
	struct exec_delegate { 
		const _Arg& arg1_;
		exec_delegate(const _Arg& arg1) : arg1_(arg1) {/* */}
		exec_delegate(const exec_delegate& rhs) : arg1_(rhs.arg1_) {/* */}
		exec_delegate& operator=(const exec_delegate& rhs) { arg1_ = rhs.arg1_; return *this; }
		void operator()(DelegateInvoke_1<_Arg>* func) const { (*func)(arg1_); } 
	};

// Class data
private:
	std::list<DelegateInvoke_1<_Arg>*> callList_;
};

/****************************************************************************/
// Delegate_2
//
// This class implements the two-parameter delegate
//
/****************************************************************************/
template<class _LockType, class _Arg1, class _Arg2>
class Delegate_2 : public LockableObject<_LockType>
{
// Constructor
public:
	Delegate_2() {/* */}
	virtual ~Delegate_2() { try	{ clear(); }catch (...) {/* */} }

// Methods
public:
	void operator()(const _Arg1& arg1, const _Arg2& arg2) const { 
		CCSLock<Delegate_2> lockGuard(this);
		std::for_each(callList_.begin(),callList_.end(), exec_delegate(arg1,arg2)); 
	}

	Delegate_2& operator=(const Delegate_2& rhs) {
		if (this != &rhs) {
			CCSLock<Delegate_2> lockGuard(&rhs);
			callList_ = rhs.callList_;
		}
		return *this;
	}

	template<class _Object, class _Class>
	void add(const _Object& object, void (_Class::* fnc)(_Arg1,_Arg2))
	{
		CCSLock<Delegate_2> lockGuard(this);
		std::list<DelegateInvoke_2<_Arg1,_Arg2>*>::iterator it = std::find_if(callList_.begin(), callList_.end(), 
			find_delegate<DelegateInvoke_2<_Arg1,_Arg2> >((DLPFN)(*((DLPFN*)&fnc)), (void*)&object));
		if (it == callList_.end())
			callList_.push_back(JTI_NEW DelegateInvokeMethod_2<_Class, _Arg1, _Arg2>(*(_Object*)&object, fnc));
	}

	void add(void (*fnc)(_Arg1,_Arg2))
	{
		CCSLock<Delegate_2> lockGuard(this);
		std::list<DelegateInvoke_2<_Arg1,_Arg2>*>::iterator it = std::find_if(callList_.begin(), callList_.end(), find_delegate<DelegateInvoke_2<_Arg1,_Arg2> >((DLPFN)fnc));
		if (it == callList_.end())
			callList_.push_back(JTI_NEW DelegateInvokeFunc_2<_Arg1,_Arg2>(fnc));
	}

	template<class _Object, class _Class>
	void remove(const _Object& object, void (_Class::* fnc)(_Arg1,_Arg2))
	{
		CCSLock<Delegate_2> lockGuard(this);
		std::list<DelegateInvoke_2<_Arg1,_Arg2>*>::iterator it = std::find_if(callList_.begin(), callList_.end(), 
			find_delegate<DelegateInvoke_2<_Arg1,_Arg2> >((DLPFN)(*((DLPFN*)&fnc)), (void*)&object));
		if (it != callList_.end()) {
			delete (*it);
			callList_.erase(it);
		}
	}      

	void remove(void (*fnc)(_Arg1,_Arg2))
	{
		CCSLock<Delegate_2> lockGuard(this);
		std::list<DelegateInvoke_2<_Arg1,_Arg2>*>::iterator it = std::find_if(callList_.begin(), callList_.end(), find_delegate<DelegateInvoke_2<_Arg1,_Arg2> >((DLPFN)fnc));
		if (it != callList_.end()) {
			delete (*it);
			callList_.erase(it);
		}
	}

	void clear()
	{
		CCSLock<Delegate_2> lockGuard(this);
		if (!callList_.empty())	{
			std::for_each(callList_.begin(),callList_.end(), stdx::delptr<DelegateInvoke_2<_Arg1,_Arg2>*>());
			callList_.clear();
		}
	}

// Embedded classes
private:
	struct exec_delegate { 
		const _Arg1& arg1_; const _Arg2& arg2_;
		exec_delegate(const _Arg1& arg1, const _Arg2& arg2) : arg1_(arg1), arg2_(arg2) {/* */}
		exec_delegate(const exec_delegate& rhs) : arg1_(rhs.arg1_),arg2_(rhs.arg2_) {/* */}
		exec_delegate& operator=(const exec_delegate& rhs) { arg1_ = rhs.arg1_; arg2_ = rhs.arg2_; return *this; }
		void operator()(DelegateInvoke_2<_Arg1,_Arg2>* func) const { (*func)(arg1_,arg2_); } 
	};

// Class data
private:
	std::list<DelegateInvoke_2<_Arg1,_Arg2>*> callList_;
};

/****************************************************************************/
// Delegate_3
//
// This class implements the three-parameter delegate
//
/****************************************************************************/
template<class _LockType, class _Arg1, class _Arg2, class _Arg3>
class Delegate_3 : public LockableObject<_LockType>
{
// Constructor
public:
	Delegate_3() {/* */}
	virtual ~Delegate_3() { try	{ clear(); }catch (...) {/* */} }

// Methods
public:
	void operator()(const _Arg1 arg1, const _Arg2 arg2, const _Arg3 arg3) const { 
		CCSLock<Delegate_3> lockGuard(this);
		std::for_each(callList_.begin(),callList_.end(), exec_delegate(arg1,arg2,arg3)); 
	}

	Delegate_3& operator=(const Delegate_3& rhs) {
		if (this != &rhs) {
			CCSLock<Delegate_3> lockGuard(&rhs);
			callList_ = rhs.callList_;
		}
		return *this;
	}

	template<class _Object, class _Class>
	void add(const _Object& object, void (_Class::* fnc)(_Arg1,_Arg2,_Arg3))
	{
		CCSLock<Delegate_3> lockGuard(this);
		std::list<DelegateInvoke_3<_Arg1,_Arg2,_Arg3>*>::iterator it = std::find_if(callList_.begin(), callList_.end(), 
			find_delegate<DelegateInvoke_3<_Arg1,_Arg2,_Arg3> >((DLPFN)(*((DLPFN*)&fnc)), (void*)&object));
		if (it == callList_.end())
			callList_.push_back(JTI_NEW DelegateInvokeMethod_3<_Class, _Arg1, _Arg2, _Arg3>(*(_Object*)&object, fnc));
	}

	void add(void (*fnc)(_Arg1,_Arg2,_Arg3))
	{
		CCSLock<Delegate_3> lockGuard(this);
		std::list<DelegateInvoke_3<_Arg1,_Arg2,_Arg3>*>::iterator it = std::find_if(callList_.begin(), callList_.end(), find_delegate<DelegateInvoke_3<_Arg1,_Arg2,_Arg3> >((DLPFN)fnc));
		if (it == callList_.end())
			callList_.push_back(JTI_NEW DelegateInvokeFunc_3<_Arg1,_Arg2,_Arg3>(fnc));
	}

	template<class _Object, class _Class>
	void remove(const _Object& object, void (_Class::* fnc)(_Arg1,_Arg2,_Arg3))
	{
		CCSLock<Delegate_3> lockGuard(this);
		std::list<DelegateInvoke_3<_Arg1,_Arg2,_Arg3>*>::iterator it = std::find_if(callList_.begin(), callList_.end(), 
			find_delegate<DelegateInvoke_3<_Arg1,_Arg2,_Arg3> >((DLPFN)(*((DLPFN*)&fnc)), (void*)&object));
		if (it != callList_.end()) {
			delete (*it);
			callList_.erase(it);
		}
	}      

	void remove(void (*fnc)(_Arg1,_Arg2,_Arg3))
	{
		CCSLock<Delegate_3> lockGuard(this);
		std::list<DelegateInvoke_3<_Arg1,_Arg2,_Arg3>*>::iterator it = std::find_if(callList_.begin(), callList_.end(), find_delegate<DelegateInvoke_3<_Arg1,_Arg2,_Arg3> >((DLPFN)fnc));
		if (it != callList_.end()) {
			delete (*it);
			callList_.erase(it);
		}
	}

	void clear()
	{
		CCSLock<Delegate_3> lockGuard(this);
		if (!callList_.empty())	{
			std::for_each(callList_.begin(),callList_.end(), stdx::delptr<DelegateInvoke_3<_Arg1,_Arg2,_Arg3>*>());
			callList_.clear();
		}
	}

// Embedded classes
private:
	struct exec_delegate { 
		const _Arg1& arg1_; const _Arg2& arg2_; const _Arg3& arg3_;
		exec_delegate(const _Arg1& arg1, const _Arg2& arg2, const _Arg3& arg3) : arg1_(arg1), arg2_(arg2), arg3_(arg3) {/* */}
		exec_delegate(const exec_delegate& rhs) : arg1_(rhs.arg1_),arg2_(rhs.arg2_), arg3_(rhs.arg3_) {/* */}
		exec_delegate& operator=(const exec_delegate& rhs) { arg1_ = rhs.arg1_; arg2_ = rhs.arg2_; arg3_ = rhs.arg3_; return *this; }
		void operator()(DelegateInvoke_3<_Arg1,_Arg2,_Arg3>* func) const { (*func)(arg1_,arg2_,arg3_); } 
	};

// Class data
private:
	std::list<DelegateInvoke_3<_Arg1,_Arg2,_Arg3>*> callList_;
};

} // namespace JTI_Util

//lint +e1725 re-enable reference arguments

#pragma warning (default:4512)

#endif // _DELEGATES_INC_H_
