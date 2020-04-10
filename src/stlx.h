/****************************************************************************/
//
// stlx.h
//
// This header defines usefull STL compatibile templates
//
// Copyright (C) 1999-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
// Revision History
// --------------------------------------------------------------
// 04/23/1999  MCS   Initial revision for Graphical Navigator
//
/****************************************************************************/

#ifndef __STLX_H_INC__
#define __STLX_H_INC__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#ifndef _WINBASE_
#define _WIN32_WINNT 0x0500
#include <windows.h>
#endif
#include <functional>
#include <JTIUtils.h>

//lint --e{1932} supress non-abstract base classes for unary_function
//lint --e{1505} supress no access specifier
namespace stdx {

/******************************************************************************/
// mem_funv and mem_funcv
//
// These are member functor template for void-returning functions.  This is
// necessary for MSVC 6.0; it has been corrected in Visual Studio .NET
//
// Usage:
//
// class someclass {
//    ...
//        void myfunc();
// };
//
// std::vector<someclass*> arr;
// std::for_each(arr.begin(), arr.end(), stdx::mem_funv(&someclass::myfunc));
//
/******************************************************************************/
template<class _Ty>
class mem_fun_tv : public std::unary_function<_Ty *, int> {
public:
	explicit mem_fun_tv(void (_Ty::*_Pm)()) : _Ptr(_Pm) {}
	result_type operator()(_Ty *_P) const { ((_P->*_Ptr)()); return 0; }
private:
	void (_Ty::*_Ptr)();
};

template<class _Ty> 
inline mem_fun_tv<_Ty> mem_funv(void (_Ty::*_Pm)())
{ return (mem_fun_tv<_Ty>(_Pm)); }

template<class _Ty> 
inline mem_fun_tv<_Ty> mem_funcv(void (_Ty::*_Pm)() const)
{ typedef void (_Ty::*funcptr)(); return (mem_fun_tv<_Ty>(reinterpret_cast<funcptr>(_Pm))); }

template<class _Ty, class _A>
class mem_fun1_tv : public std::binary_function<_Ty *, _A, int> {
public:
	explicit mem_fun1_tv(void (_Ty::*_Pm)(_A)) : _Ptr(_Pm) {}
	result_type operator()(_Ty *_P, _A _Arg) const { ((_P->*_Ptr)(_Arg)); return 0; }
private:
	void (_Ty::*_Ptr)(_A);
};

template<class _Ty, class _A> 
inline mem_fun1_tv<_Ty, _A> mem_funv1(void (_Ty::*_Pm)(_A))
{ return (mem_fun1_tv<_Ty, _A>(_Pm)); }

/******************************************************************************/
// mem_funv_ref and mem_funcv_ref
//
// These are member functor template for void-returning functions.
//
// Usage:
//
// class someclass {
//    ...
//        void myfunc();
// };
//
// std::vector<someclass> arr;
// std::for_each(arr.begin(), arr.end(), stdx::mem_funv_ref(&someclass::myfunc));
//
/******************************************************************************/
template<class _Ty>
class mem_fun_ref_tv : public std::unary_function<_Ty, int> {
public:
	explicit mem_fun_ref_tv(void (_Ty::*_Pm)()) : _Ptr(_Pm) {}
	result_type operator()(_Ty& _X) const { ((_X.*_Ptr)()); return 0; }
private:
	void (_Ty::*_Ptr)();
};

template<class _Ty> 
inline mem_fun_ref_tv<_Ty> mem_funv_ref(void (_Ty::*_Pm)())
{ return (mem_fun_ref_tv<_Ty>(_Pm)); }

template<class _Ty> 
inline mem_fun_ref_tv<_Ty> mem_funcv_ref(void (_Ty::*_Pm)() const)
{ typedef void (_Ty::*funcptr)(); return (mem_fun_tv<_Ty>(reinterpret_cast<funcptr>(_Pm))); }

/******************************************************************************/
// inauto_ptr
//
// auto_ptr class for intrinsic data types (i.e. non UDT)
//
// Usage:
//
// stdx::inauto_ptr<char> = new char[256];
//
/******************************************************************************/
template<class _Ty>
class inauto_ptr 
{
public:
	typedef _Ty element_type;
	explicit inauto_ptr(_Ty *_P = 0) throw() : m_fOwner(_P != 0), m_pData(_P) {}
	inauto_ptr(const inauto_ptr<_Ty>& src) throw() : m_fOwner(src.m_fOwner), m_pData(src.release()) {}
	inauto_ptr<_Ty>& operator=(const inauto_ptr<_Ty>& src) throw()
	{
		if (this != &src) {
			if (m_pData != src.get()) {
				if (m_fOwner) 
					delete [] m_pData;
				m_fOwner = src.m_fOwner; 
			}
			else if (src.m_fOwner)
				m_fOwner = true, src.release();	//lint !e534
			m_pData = src.release(); 
		}
		return (*this); 
	}

	//lint --e{1540}
	~inauto_ptr() throw()
	{
		if (m_fOwner) {
			delete [] m_pData; 
			m_pData = 0;
		}
	}

	_Ty *get() const throw() { return m_pData; }	//lint !e1763
	_Ty& operator*() const throw() { return (*get()); }
	_Ty *release() const throw()
	{
		const_cast<inauto_ptr<_Ty>*>(this)->m_fOwner = false;
		return (m_pData); 
	}
	void reset(_Ty* pData = 0)
	{
		if (pData != m_pData)
			delete [] m_pData;
		m_fOwner = (pData != static_cast<_Ty*>(NULL));
		m_pData = pData; 
	}

// Class data
private:
	bool m_fOwner;
	_Ty *m_pData;
};

/******************************************************************************/
// delptr
//
// Auto-deleting unary function
//
// Usage:
//
// std::for_each(arr.begin(), arr.end(), delptr<type>());
//
/******************************************************************************/
template <class T>
struct delptr : std::unary_function<T,void> {
	void operator()(T t) const { delete t; }
};

/******************************************************************************/
// rel_obj
//
// Self releasing unary function (usable by COM objects or objects which
// implement a "Release" method).
//
/******************************************************************************/
template <class T>
struct rel_obj : std::unary_function<T,void> {
	void operator()(T t) const { t->Release(); }
};

/******************************************************************************/
// compose_f_gx
//
// Composition adapter f(g(elem))
//
// This is the general form unary compose function.  It allows nested calls of
// unary predicates such that the result of calling predicate g() for elem is
// used as the input for predicate f().  The whole expression then operates as
// a unary predicate.
// 
/******************************************************************************/
template <class OP1, class OP2>
class compose_f_gx_t : public std::unary_function<typename OP2::argument_type, typename OP1::result_type>
{
private:
	OP1 op1; OP2 op2;
public:
	compose_f_gx_t(const OP1& o1, const OP2& o2) : op1(o1), op2(o2) {}
	typename OP1::result_type 
		operator()(const typename OP2::argument_type& x) const {
			return op1(op2(x));
		}
};

template <class OP1, class OP2>
inline compose_f_gx_t<OP1,OP2>
compose_f_gx(const OP1& o1, const OP2& o2) {
	return compose_f_gx_t<OP1,OP2>(o1,o2);
}

/******************************************************************************/
// compose_f_gx_hx
//
// Composition adapter f(g(elem), h(elem))
//
// This is compose function where the elem is passed to two different unary
// predicates g() and h() and the result of both is then processed by the binary
// predicate f().  In a way, this "injects" a single argument into a composed
// function.
// 
/******************************************************************************/
template <class OP1, class OP2, class OP3>
class compose_f_gx_hx_t : public std::unary_function<typename OP2::argument_type, typename OP1::result_type>
{
private:
	OP1 op1; OP2 op2; OP3 op3;
public:
	compose_f_gx_hx_t (const OP1& o1, const OP2& o2, const OP3& o3) : op1(o1), op2(o2), op3(o3) {}
	typename OP1::result_type
		operator()(const typename OP2::argument_type& x) const {
			return op1(op2(x),op3(x));
		}
};

template <class OP1, class OP2, class OP3>
inline compose_f_gx_hx_t<OP1,OP2,OP3>
compose_f_gx_hx(const OP1& o1, const OP2& o2, const OP3& o3) {
	return compose_f_gx_hx_t<OP1,OP2,OP3>(o1,o2,o3);
}

/******************************************************************************/
// sc_ptr_fun
//
// System call ptr_fun functor.
//
// This is allows __syscall type API functions to be the target of a STL
// predicate function.
// 
/******************************************************************************/
template<class _A, class _R>
class sc_pointer_to_unary_function : public std::unary_function<_A, _R> 
{
protected:
	_R (__stdcall *_Fn)(_A);	// Function to call
public:
	explicit sc_pointer_to_unary_function(_R (__stdcall *_X)(_A)) : _Fn(_X) {}
	_R operator()(_A _X) const {return (_Fn(_X)); }
};

template<class _A1, class _A2, class _R>
class sc_pointer_to_binary_function	: public std::binary_function<_A1, _A2, _R> 
{
protected:
	_R (__stdcall *_Fn)(_A1, _A2);	// Function to call
public:
	explicit sc_pointer_to_binary_function(_R (__stdcall *_X)(_A1, _A2)) : _Fn(_X) {}
	_R operator()(_A1 _X, _A2 _Y) const	{return (_Fn(_X, _Y)); }
};

template<class _A, class _R> 
inline sc_pointer_to_unary_function<_A, _R>
sc_ptr_fun(_R (__stdcall *_X)(_A)) { return (sc_pointer_to_unary_function<_A, _R>(_X)); }

template<class _A1, class _A2, class _R> 
inline sc_pointer_to_binary_function<_A1, _A2, _R>
sc_ptr_fun(_R (__stdcall *_X)(_A1, _A2)) { return (sc_pointer_to_binary_function<_A1, _A2, _R>(_X)); }

/******************************************************************************/
// for_each_if
//
// Modifying version of for_each which allows for a conditional test
//
/******************************************************************************/
template <typename Itfw, typename Op, typename Pred>
Op for_each_if (Itfw first, Itfw last, Op action, Pred condition)
{
    while ( first != last ) {
        if ( condition( *first ) )
            action( *first );
        ++first;
    } return action;
}

/******************************************************************************/
// split
//
// Splits a string into multiple pieces based on a separator
//
/******************************************************************************/
template<class OutIt>
void split(const std::string& s, const std::string& sep, OutIt dest) {
	std::string::size_type left = s.find_first_not_of(sep);
	std::string::size_type right = s.find_first_of(sep, left);
	while (left < right) {
		*dest = s.substr(left, right-left);
		dest++;
		left = s.find_first_not_of(sep, right);
		right = s.find_first_of(sep, left);
	}
}

template<class OutIt>
void split(const std::wstring& s, const std::wstring& sep, OutIt dest) {
	std::wstring::size_type left = s.find_first_not_of(sep);
	std::wstring::size_type right = s.find_first_of(sep, left);
	while (left < right) {
		*dest = s.substr(left, right-left);
		dest++;
		left = s.find_first_not_of(sep, right);
		right = s.find_first_of(sep, left);
	}
}

/******************************************************************************/
// split_preserving_quotes
//
// Splits a string into multiple pieces based on a separator; preserving quotes
// as a single literal
//
/******************************************************************************/
template<class OutIt>
void split_preserving_quotes(const std::string& s, const std::string& sep, OutIt dest) {
	const char quoteChar = '\"';
	if (s.find_first_of(quoteChar) == std::string::npos) {
		split(s,sep,dest);
	}
	else {
		std::string::size_type left = s.find_first_not_of(sep);
		std::string::size_type quote = s.find_first_of(quoteChar);
		std::string::size_type right = s.find_first_of(sep, left);

		while (left < right) {
			if (quote < right) // find the end of the quote or EOS.
				right = s.find_first_of(sep, s.find_first_of(quoteChar, quote+1));
			*dest++ = s.substr(left, right-left);
			left = s.find_first_not_of(sep, right);
			quote = s.find_first_of(quoteChar,left);
			right = s.find_first_of(sep, left);
		}
	}
}

template<class OutIt>
void split_preserving_quotes(const std::wstring& s, const std::wstring& sep, OutIt dest) {
	const wchar_t quoteChar = L'\"';
	if (s.find_first_of(quoteChar) == std::wstring::npos) {
		split(s,sep,dest);
	}
	else {
		std::wstring::size_type left = s.find_first_not_of(sep);
		std::wstring::size_type quote = s.find_first_of(quoteChar);
		std::wstring::size_type right = s.find_first_of(sep, left);

		while (left < right) {
			if (quote < right) // find the end of the quote or EOS.
				right = s.find_first_of(sep, s.find_first_of(quoteChar, quote+1));
			*dest++ = s.substr(left, right-left);
			left = s.find_first_not_of(sep, right);
			quote = s.find_first_of(quoteChar,left);
			right = s.find_first_of(sep, left);
		}
	}
}

/******************************************************************************/
// join:
//
// Quick functions to join an array of strings together
//
/******************************************************************************/
template<class _It>
std::string join(_It itBegin, _It itEnd, std::string sep)
{
	std::string line;
	for(_It it = itBegin; it != itEnd; ++it) {
		if (it != itBegin) line += sep;
		line += *it;
	} 
	return line;

}// join

template<class _It>
std::wstring join(_It itBegin, _It itEnd, std::wstring sep)
{
	std::wstring line;
	for(_It it = itBegin; it != itEnd; ++it) {
		if (it != itBegin) line += sep;
		line += *it;
	} 
	return line;

}// join

/******************************************************************************/
// trim:
//
// Quick functions to strip starting/trailing blanks. Supply both unicode/ansi versions.
//
/******************************************************************************/
#pragma warning(disable : 4996)
inline wchar_t* trim(wchar_t* psz)
{
	if (!psz || !*psz) return psz;
	wchar_t* pc = psz, *pe = (psz+wcslen(psz));
	while (*pc == L' ') ++pc;
	if (pc != psz) memcpy(psz,pc,static_cast<unsigned int>((pe+1-pc)*sizeof(wchar_t)));
	pe = (psz+wcslen(psz)-1);
	while (*pe == L' ') --pe; *(pe+1) = 0;
	return psz;
}
inline char* trim(char* psz)
{
	if (!psz || !*psz) return psz;
	char* pc = psz, *pe = (psz+strlen(psz));
	while (*pc == ' ') ++pc;
	if (pc != psz) memcpy(psz,pc,static_cast<unsigned int>(pe+1-pc));
	pe = (psz+strlen(psz)-1); 
	while (*pe == ' ') --pe; *(pe+1) = 0;
	return psz;
}
#pragma warning(default : 4996)

/******************************************************************************/
// trimupper:
//
// Quick functions to convert a string to upper case and strip 
// starting/trailing blanks. Supply both unicode/ansi versions.
//
/******************************************************************************/
inline wchar_t* trimupper(wchar_t* psz)
{
	psz = stdx::trim(psz);
	int len = lstrlenW(psz);
	_wcsupr_s(psz, len);
	return psz;
}	


inline char* trimupper(char* psz)
{
	psz = stdx::trim(psz);
	int len = lstrlenA(psz);
	_strupr_s(psz, len);
	return psz;
}

/******************************************************************************/
// trimlower:
//
// Quick functions to convert a string to lower case and strip 
// starting/trailing blanks. Supply both unicode/ansi versions.
//
/******************************************************************************/
inline wchar_t* trimlower(wchar_t* psz)
{
	psz = stdx::trim(psz);
	int len = lstrlenW(psz);
	_wcslwr_s(psz, len);
	return psz;
}

inline char* trimlower(char* psz)
{
	psz = stdx::trim(psz);
	int len = lstrlenA(psz);
	_strlwr_s(psz, len);
	return psz;
}

/******************************************************************************/
// string_replace
//
// This function replaces substrings within a std::string/wstring
//
/******************************************************************************/
template <typename CH>
inline void string_replace(std::basic_string<CH>& s, const std::basic_string<CH>& sFind, const std::basic_string<CH>& sReplace)
{
    std::basic_string<CH>::size_type pos = 0, szf = sFind.size(), szr = sReplace.size();;
    while((pos = s.find(sFind, pos)) != std::basic_string<CH>::npos) {
        s.replace(pos, szf, sReplace);
		pos += szr;
	}
}

/******************************************************************************/
// map_adapter_1 and map_adapter_2
//
// These functors allow a map to be used as a source for a transform when 
// only one of the elements (key/data) is actually to be passed to a unary
// function.
//
/******************************************************************************/
template <class _Type1, class _Type2>
struct map_adapter_1 : std::unary_function<std::pair<_Type1, _Type2>, _Type1>
{
	result_type operator()(const argument_type& p) const { return p.first; }
};

template <class _Type1, class _Type2>
struct map_adapter_2 : std::unary_function<std::pair<_Type1, _Type2>, _Type2>
{
	result_type operator()(const argument_type& p) const { return p.second; }
};

/******************************************************************************/
// ptr_less
//
// This functor allows for containers with pointers to be sorted by the
// value of the pointer rather than the address.
//
/******************************************************************************/
template<class T> struct ptr_less : public std::binary_function<T, T, bool>
{
    bool operator()(const T* var1, const T* var2) const {
        return *var1 < *var2;
    }
};

} // namespace stdx

#endif // __STLX_H_INCLUDED__
