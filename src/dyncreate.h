/****************************************************************************/
//
// Dyncreate.h
//
// Support for polymorphic dynamic object creation under RTTI.
// This replaces the MFC mechanism with one based on the native runtime-type
// information in the compiler.  Note that the program should be compiled with
// RTTI support (/GR)
//
// Copyright (C) 1997-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

/*----------------------------------------------------------------------------
//  INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <string.h>

/*----------------------------------------------------------------------------
//  MACROS
-----------------------------------------------------------------------------*/
#define DECLARE_JTI_DYNCREATE(rclass) \
	static JTI_Util::TFactory<rclass> _fac##rclass;

namespace JTI_Util
{    
/****************************************************************************/
// CreateDynamicObject
//
// Create a new object dynamically. Search the list of dynamic
// objects and call each create_object() method until one of them 
// is successful in making the required object. If we get to the end of the
// list, then the object wasn't registered.
//
// Usage:
//
// class A {...};
// DECLARE_JTI_DYNCREATE(A)
//	...
// A* pa = JTI_Util::CreateDynamicObject<A>();
//
/****************************************************************************/
template <class T>
T* CreateDynamicObject()
{
	return reinterpret_cast<T*>(IRttiFactory::CreateDynamicObject(typeid(T).name()));

}// CreateDynamicObject

/****************************************************************************/
// IRttiFactory
//
// This interface describes the abstract RTTI factory which is derived from
// using a type-specific class from TFactory.
//
/****************************************************************************/
class IRttiFactory
{    
// Constructor/Destructor
public:
	IRttiFactory() 
	{ 
		nextPtr_ = headPtr_; 
		headPtr_ = this; 
	}
	
	virtual ~IRttiFactory()	
	{
		IRttiFactory** ppNext = &headPtr_;
		for (; *ppNext != NULL; ppNext = &(*ppNext)->nextPtr_) {
			if (*ppNext == this) {
				*ppNext = (*ppNext)->nextPtr_;
				break;
			}
		}
	}
	
// Methods
public:
	// This method walks all the registered classes and invokes the CreateObject
	// function of the specific type to create.
	static void* CreateDynamicObject(const char* pszClass) 
	{
		void* pObject = NULL;
		const IRttiFactory* currFactory = headPtr_;
		for (; currFactory != NULL && pObject == NULL; currFactory = currFactory->nextPtr_)
			pObject = currFactory->CreateObject(pszClass);
		return pObject;
	}

// Required overrides
private:
	virtual void* CreateObject(const char* pszClass) const = 0;

// Class data
private:
	static IRttiFactory* headPtr_;
	IRttiFactory* nextPtr_;
};

/****************************************************************************/
// TFactory
//
// This template class implements the specific typed factory for a given
// C++ RTTI-capable object.
//
/****************************************************************************/
template <class T> 
class TFactory : public IRttiFactory
{
	virtual void *CreateObject(const char* pszClass) const 
	{
		return !stricmp(typeid(T).name(), pszClass)
				? (void*)( new T ) : (void*)0;
	}
};

}// namespace JTI_Util


