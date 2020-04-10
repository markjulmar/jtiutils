/****************************************************************************/
//
// SingletonRegistry.h
//
// This object maintains a list of singletons that are used within an application
// and enforces singleton behavior.
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

#ifndef __SINGLETON_REGISTRY_H_INCL__
#define __SINGLETON_REGISTRY_H_INCL__

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <stdexcept>
#include <JTIUtils.h>
#include <longevity.h>
#include <lock.h>

/*****************************************************************************/
// PC-Lint options
//
//lint -save
//
// Private constructor
//lint -esym(1704, Singleton*)
//
/*****************************************************************************/

namespace JTI_Util
{   
/****************************************************************************/
// Singleton
//
// This template class wraps a class and gives it singleton characteristics.
//
/****************************************************************************/
template <class T, 
		  int Longevity = 0,
		  class LockType = SimpleMultiThreadModel>
class Singleton
{
// Class data
private:
	// This implements our global class data; The original release of VS.NET did 
	// not allow for UDT class template parameters outside the template definition 
	// itself.  This construct allows a similar functionality which will
	// compile with VS2002 and beyond.
	static bool* _Destroyed() {	static bool _gDestroyed = false; return &_gDestroyed; }
	static T** _Instance() { static T* _gInstance = 0; return &_gInstance; }
	static typename LockType::CriticalSection& _Lock() { static typename LockType::CriticalSection _gLock; return _gLock; }

// Implementation functions
public:
	// This method returns the singleton instance
	static T& Instance()
	{
		// Use the double-check lock pattern to test the pointer.
		T** pi = _Instance();
		if (*pi == NULL)
		{
			CCSLock<LockType::CriticalSection> guard(&_Lock());
			if (*pi == NULL)
			{
				if (*_Destroyed() == true)
					throw std::logic_error("Singleton Dead Reference Detected");
				*pi = new T();
				LifetimeTracker<T,CallbackDeleter<T> >::SetLongevity(*pi, Longevity, 
						CallbackDeleter<T>(&DestroySingleton));
			}
		}
		return **pi;
	}

// Internal functions
private:
    static void DestroySingleton(T*)
	{
		CCSLock<LockType::CriticalSection> guard(&_Lock());
		T** pi = _Instance();
        
		delete (*pi);
        *pi = NULL;
        *_Destroyed() = true;
	}

// Unavailable functions
private:
	Singleton();
};

} // namespace JTI_Util

#endif // __SINGLETON_REGISTRY_H_INCL__
