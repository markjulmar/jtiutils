/****************************************************************************/
//
// SEHException.h
//
// This header implements a structured exception handler class which allows
// SEH exceptions to be caught as C++ seh_exception classes using the normal
// C++ try/catch mechanism.  When using Microsoft Visual C++, the project
// must have the /EHa flag set and not the /EHsc flag.
//
// Copyright (C) 2002-2004 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

#ifndef __JTI_SEH_EXCEPT_H__
#define __JTI_SEH_EXCEPT_H__

/*----------------------------------------------------------------------------
//  INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <JTIUtils.h>
#include <eh.h>

namespace JTI_Util
{
/*****************************************************************************
** seh_exception
** 
** This exception class can catch Structured Exceptions (SEH) on the 
** Windows platform.
**
/****************************************************************************/
class seh_exception : public std::runtime_error
{
	// Constructors
public:
	seh_exception() : _type(0), std::runtime_error("") {}
	seh_exception(unsigned int type) : _type(type), std::runtime_error("") {}
	seh_exception(const seh_exception& rhs) : _type(rhs._type), std::runtime_error(rhs) {}
public:
	int GetExceptionType() const { return _type; }
	static void Install() { _set_se_translator(&_SEHHandler); }
private:
	static void _SEHHandler(unsigned int uException, _EXCEPTION_POINTERS*) {
		throw seh_exception(uException);
	}
	// Class data
private:
	unsigned int _type;
};
}// namespace JTI_Util

#endif // __JTI_SEH_EXCEPT_H__