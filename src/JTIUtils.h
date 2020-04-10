/****************************************************************************/
//
// JTIUtils.h
//
// Base Utility Library - includes all portions of the base support system
//
// Copyright (C) 1997-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
// Revision History
// -------------------------------------------------------------
// 02/1997		Initial revision (TSP++ support library)
// 11/1999		Graphical Diagramming Navigator classes merged
// 01/2000		Added classes for SimulRing
// 06/2001		Added binary stream and reference counting classes
// 09/2001		Added .NET "compatible" classes (registry, file, delegates)
// 09/2002		Added XML support classes from SimulRing
// 04/2003		Updated for full C++ standards (VS.NET 7.1)
// 12/2003		Updates for VS.NET "Whidbey" support.
// 11/2004      Updates for VS.NET 2005 Beta 1 refresh
//
/****************************************************************************/

#ifndef __JTI_BASE_UTILS_H_INCL__
#define __JTI_BASE_UTILS_H_INCL__

#define JTILIB_VERSION 0x00030300	// V 3.3.0 (11/2004)

/*----------------------------------------------------------------------------
//  INCLUDE FILES
-----------------------------------------------------------------------------*/
#ifdef JTI_NEW
#undef JTI_NEW
#undef new
#endif
#include <tchar.h>
#include <string>
#include <stdexcept>
#include <stdarg.h>

// Redefine the new operator when using a debug library
#include <new>
#if defined(DEBUG) || defined(_DEBUG)
#include <crtdbg.h>
#ifndef JTI_NEW
	#define JTI_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__) 
#endif
#else
	#define JTI_NEW new
#endif

// Add a linker record in case the user does not.
#if !defined(_LIB) && !defined(_JTI_NO_LINK_RECORD)
#if defined(_DLL)
#if defined(DEBUG) || defined(_DEBUG)
	#if defined(UNICODE) || defined(_UNICODE)
		#pragma comment(lib, "jtiUtlsDUx.lib")
	#else
		#pragma comment(lib, "jtiUtlsDx.lib")
	#endif
#else
	#if defined(UNICODE) || defined(_UNICODE)
		#pragma comment(lib, "jtiUtlsUx.lib")
	#else
	#pragma comment(lib, "jtiUtlsx.lib")
	#endif
#endif

#else

#if defined(DEBUG) || defined(_DEBUG)
	#if defined(UNICODE) || defined(_UNICODE)
		#pragma comment(lib, "jtiUtlsDU.lib")
	#else
		#pragma comment(lib, "jtiUtlsD.lib")
	#endif
#else
	#if defined(UNICODE) || defined(_UNICODE)
		#pragma comment(lib, "jtiUtlsU.lib")
	#else
	#pragma comment(lib, "jtiUtls.lib")
	#endif
#endif

#endif // _DLL

#endif // !_LIB && !_JTI_NO_LINK_RECORD

/*****************************************************************************
** _tout/_tin/_terr
** 
** Macros which masks Unicode vs. Ansi for the common in/out/err streams.
** 
/****************************************************************************/
#ifdef _UNICODE
#define _tout wcout
#define _tin wcin
#define _terr wcerr
#else
#define _tout cout
#define _tin cin
#define _terr cerr
#endif

/*****************************************************************************
** WIDEN
** 
** This macro converts an ANSI string into a wide string during the compile
** phase.  
**
** Example:
**
**   const wchar_t* pszFilename = WIDEN(__FILE__);
** 
/****************************************************************************/
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)

/*****************************************************************************
** CH2STR
** 
** This macro converts a literal to a string.
**
** Example:
**
**   const char* pszLine = CH2STR(__LINE__);
** 
/****************************************************************************/
#define CH2STR2(x)  #x
#define CH2STR(x)   CH2STR2(x)

/*****************************************************************************
** PRINT_COMPILER_MSG/PRINT_COMPILER_TODO
** 
** Macros which print out messages during compile phase.
** 
/****************************************************************************/
#ifdef _MSC_VER
	#define PRINT_COMPILER_MSG(desc)  message(__FILE__ "(" CH2STR(__LINE__) "):" #desc)
	#define PRINT_COMPILER_TODO(desc) message(__FILE__ "(" CH2STR(__LINE__) "): TODO: " #desc)
#endif

/*****************************************************************************
** sizeofarray
** 
** Returns the number of elements in an array
** 
/****************************************************************************/
#ifndef sizeofarray
	#define sizeofarray(rg) (sizeof(rg)/sizeof(*rg))
#endif

/*****************************************************************************
** Procedure:  ELAPSED_TIME
** 
** Arguments:  DWORD dw - Original tickcount to calculate elapsed time from 
** 
** Returns: Returning elapsed time in msecs. 
** 
** Description: This macro returns the elapsed time from the given time marker 
**
/****************************************************************************/
#ifndef ELAPSED_TIME
	#define ELAPSED_TIME(dw) ((dw==0) ? 0 : (GetTickCount()<dw) ? ((0xffffff-dw)+GetTickCount()) : (GetTickCount()-dw))
#endif

/*****************************************************************************/
// PC-Lint options
//
//lint -save
//
// Private constructor
//lint -esym(1704, _A2W*, _W2A*)
//
// Constructor can be used for implicit conversion
//lint -esym(1931, _A2W*, _W2A*)
//
// Conversion operator found
//lint -esym(1930, _A2W*, _W2A*)
//
// No default constructor
//lint -esym(1712, _A2W*, _W2A*)
//
/*****************************************************************************/

namespace JTI_Util
{
/******************************************************************************/
// tstring/tstringstream
//
// This defines a string as unicode/ansi based on the current 
// compiler settings.  
//
/******************************************************************************/
#if defined(_UNICODE) || defined(UNICODE)
	typedef std::wstring tstring;
	typedef std::wstringstream tstringstream;
#else
	typedef std::string tstring;
	typedef std::stringstream tstringstream;
#endif

namespace JTI_Internal
{
/******************************************************************************/
// _A2W
//
// This converts an ANSI string to Unicode
//
/******************************************************************************/
class _A2W
{
// Constructor
public:
	_A2W(const char* psz, unsigned int codePage=0);
	~_A2W();

// Operators
public:
	operator wchar_t*() const 
	{ 
		return _pszBuff; 
	}

// Class data
private:
	enum { BUFF_SIZE = 255 };
	wchar_t _buff[BUFF_SIZE];
	wchar_t* _pszBuff;

// Unavailable methods
private:
	_A2W(const _A2W&);
	_A2W& operator=(const _A2W&);
};

/******************************************************************************/
// _W2A
//
// This converts an Unicode string to Ansi
//
/******************************************************************************/
class _W2A
{
// Constructor
public:
	_W2A(const wchar_t* psz, unsigned int codePage=0);
	~_W2A();

// Operators
public:
	operator char*() const 
	{ 
		return _pszBuff; 
	}

// Class data
private:
	enum { BUFF_SIZE = 255 };
	char _buff[BUFF_SIZE];
	char* _pszBuff;

// Unavailable methods
private:
	_W2A(const _W2A&) throw();
	_W2A& operator=(const _W2A&) throw();
};
} // namespace JTI_Internal

/*****************************************************************************
** Procedure:  A2WHelper
** 
** Arguments:  psz - String to convert
**             pszBuffer - Pointer to buffer
**             nSize - size of buffer
** 
** Returns: Pointer to converted string
** 
** Description: This function converts an ANSI string to wide.
**
/****************************************************************************/
extern wchar_t* A2WHelper(const char* psz, wchar_t* pszBuffer, int nSize);

/*****************************************************************************
** Procedure:  W2AHelper
** 
** Arguments:  psz - String to convert
**             pszBuffer - Buffer to place string into
**             nSize - size of buffer
** 
** Returns: Pointer to converted string
** 
** Description: This function converts a wide-string to ANSI
**
/****************************************************************************/
extern char* W2AHelper(const wchar_t* psz, char* pszBuffer, int nSize);

/*****************************************************************************
** Procedure:  patchk
** 
** Arguments:  szPattern - Pattern to match
**             szString - Test string
** 
** Returns: TRUE if the string matches the given pattern
** 
** Description: This function pattern matches two unicode strings.
**
/****************************************************************************/
extern bool patchk(const char *szPattern, const char *szString);
extern bool patchk(const wchar_t *szPattern, const wchar_t *szString);

/******************************************************************************/
// _SupportsConversion
//
// This internal template structure determines whether a class has 
// virtual functions defined for it.
//
// Use the below SupportsConversion instead
//
/******************************************************************************/
#pragma warning(disable:4244) // remove cast warnings
namespace JTI_Internal
{
	template <class _Type1, class _Type2>
	class _SupportsConversion
	{
		typedef char No; struct Yes { No no[2]; };
		static Yes test(_Type2);
		static No test(...);
		static _Type1 make();
	public:
		enum { check = sizeof(test(make())) == sizeof(Yes) };
	};
}
#pragma warning(default:4244)

/******************************************************************************/
// SupportsConversion
//
// This template structure determines whether a class has virtual functions
// defined for it.
//
// class A
// {
// };
//
// SupportsConversion<int,long>::check		== true
// SupportsConversion<int,double>::check	== true
// SupportsConversion<int,A>::check			== false
//
/******************************************************************************/
template <class _Type1, class _Type2>
struct SupportsConversion
{
	enum { check = JTI_Internal::_SupportsConversion<_Type1,_Type2>::check };
	enum { check_both = check && JTI_Internal::_SupportsConversion<_Type2, _Type1>::check };
};

/******************************************************************************/
// IsDerivedFrom
//
// This template class determines whether a class (_Derived) derives from a
// another class (_Base).
//
// class A 
// {
// };
//
// class B : public A
// {
// };
//
// Then you can test in the code:
//
//	IsDerivedFrom<A,B>::check == false
//	IsDerivedFrom<B,A>::check == true
//
// To enforce a base interface at compile time:
//
// class C
// {
// };
//
// template <class T>
// class D : public IsDerivedFrom<T,A>
// {
// };
//
// D<A> d1;  <-- this is ok
// D<C> d2;  <-- this will fail @ compile time with an error.
//
/******************************************************************************/
template <typename _Derived, typename _Base>
class IsDerivedFrom
{
	static void constraints(_Derived* p) {_Base* pb = p; pb = p; }
public:
	enum { check = SupportsConversion<_Derived,_Base>::check && !SupportsConversion<_Base,_Derived>::check };
	IsDerivedFrom() { void(*p)(_Derived*) = constraints; p; }
};

/******************************************************************************/
// HasVTable
//
// This template structure determines whether a class has virtual functions
// defined for it.
//
// class A
// {
// };
// 
// class E
// {
// public:
// 	E() {}
// 	virtual ~E() {}
// };
//
//
//	HasVTable<A>::check	== false
//	HasVTable<E>::check	== true
//
/******************************************************************************/
template<class _Ty> 
struct HasVTable
{
   class X : public _Ty {
      X(); //lint !e1704
      virtual void test();
   };
   enum { check = sizeof(X) == sizeof(_Ty) };
};

/******************************************************************************/
// Int2Type/Bool2Type
//
// This template structure allows an integer to become a valid type which can
// then be used to change a function signature:
//
//	void DoTest(Int2Type<true>)
//	{
//		DoTestImpl();
//	}
//	void DoTest(Int2Type<false>)
//	{
//		cout << "DoTest not supported." << endl;
//	}
//
// This can then be called as:
//
// DoTest(true) or DoTest(false) which will invoke either function.
//
/******************************************************************************/
template<int val>
struct Int2Type
{
   enum { value = val };
};

/******************************************************************************/
// Type2Type
//
// This template structure allows a type to be treated as a distinct type
// for the purposes of template expansion.  This allows template specialization
// to be treated differently by return type (which currently isn't enough
// to differentiate a function in C++).
//
// template <class T, class U>
// T* Create(const U& arg, Type2Type<T>)
// {
//	   return new T(arg);
// }
// template <class U>
// Widget* Create(const U& arg, Type2Type<Widget>)
// {
//     return new Widget(arg, -1);
// }
//
// String* pStr = Create("Hello", Type2Type<String>());
// Widget* pW = Create(100, Type2Type<Widget>());
//
/******************************************************************************/
template<typename _Ty>
struct Type2Type
{
    typedef _Ty OriginalType;
};

/*****************************************************************************
** jtiexception
** 
** Library exception; used to return OS error codes with strings
**
/****************************************************************************/
class jtiexception : public std::exception
{
public:
	explicit jtiexception(const char* serr = "");
	explicit jtiexception(long err, const char* serr = "") : std::exception(serr), err_(err) {/* */}
	long code() const { return err_; }
private:
	long err_;
};

/*****************************************************************************
** dprintf
** 
** Formatted "debug" printf
**
/****************************************************************************/
extern int dprintf(const char *pszFormat, ...);

}// namespace JTI_Utils

/*****************************************************************************
** A2W/W2A
** 
** Ansi/Unicode conversion helper macros
**
/****************************************************************************/
#undef A2W
#undef W2A
#undef A2T
#undef T2A
#undef W2T
#undef T2W
#undef USES_CONVERSION

#define USES_CONVERSION (__noop)

#define A2W(s) static_cast<wchar_t*>(JTI_Util::JTI_Internal::_A2W(s))
#define W2A(s) static_cast<char*>(JTI_Util::JTI_Internal::_W2A(s))

#if defined(UNICODE) || defined(_UNICODE)
#define A2T(s) A2W(s)
#define T2W(s) s
#define T2A(s) W2A(s)
#define W2T(s) s
#else
#define A2T(s) s
#define T2W(s) A2W(s)
#define T2A(s) s
#define W2T(s) W2A(s)
#endif

#endif // __JTI_BASE_UTILS_H_INCL__
