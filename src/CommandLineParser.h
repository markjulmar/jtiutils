/****************************************************************************/
//
// CommandLineParser.h
//
// This file describes a command line parser class for C++
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
//
/****************************************************************************/

#ifndef __COMMANDLINE_PARSER_H_INCL__
#define __COMMANDLINE_PARSER_H_INCL__

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <tchar.h>
#include <malloc.h>
#pragma warning(disable:4571)
#include <list>
#include <algorithm>
#pragma warning(default:4571)

namespace JTI_Util
{
/****************************************************************************/
// ParamValue
//
// This class describes a single parameter on the command line.
//
/****************************************************************************/
class ParamValue
{
// Class data
private:
	const TCHAR* param_;
	const TCHAR* value_;

// Constructor
public:
	ParamValue() : param_(0), value_(0) {/* */}
	ParamValue(const ParamValue &p) : param_(p.param_), value_(p.value_) {/* */}
	ParamValue(const TCHAR* param) : param_(param), value_(0) {/* */}
	ParamValue(const TCHAR* param, const TCHAR* value) : param_(param), value_(value) {/* */}

// Properties
public:
	__declspec(property(get=get_Name)) const TCHAR* Name;
	__declspec(property(get=get_Value)) const TCHAR* Value;
	__declspec(property(get=get_HasValue)) bool HasValue;

// Accessors
public:
	const TCHAR* get_Name() const { return param_; }
	bool get_HasValue() const { return (value_ != NULL); }
	const TCHAR* get_Value() const { return (value_) ? value_ : _T(""); }

// Converters
public:
	float ToFloat()
	{
		TCHAR* err;
		float x = static_cast<float>(_tcstod(value_, &err));
		return x;
	}

	int ToInt32()
	{
		TCHAR* err;
		int x = static_cast<int>(_tcstol(value_, &err, 10));
		return x;
	}

	DWORD GetHashCode() const { return ParamValue::GetHashCode(param_); }

// Hash function for faster lookups (switch)
public:
	static DWORD GetHashCode(const TCHAR* pszText_)
	{
		char* pszText;
#ifdef UNICODE
		int size = WideCharToMultiByte(CP_ACP, 0, pszText_, -1, 0, 0, NULL, NULL);
		if (size == 0)
			return 0;
		pszText = reinterpret_cast<char*>(_alloca(size+1));
		WideCharToMultiByte(CP_ACP,0,pszText_,-1,pszText,size,NULL,NULL);
#else
		pszText = reinterpret_cast<char*>(_alloca(lstrlen(pszText_)+1));
		lstrcpy(pszText,pszText_);
#endif
		int len = lstrlenA(pszText);
		_strlwr_s(pszText, len); // Force lowercase

		const BYTE* b = reinterpret_cast<const BYTE*>(pszText);
		if (len >= 4)
			return b[0]+(b[1] << 12) + (b[2] << 6) + (b[3] << 18) + (b[len-1] << 3) + len;
		else if (len >= 1)
			return b[0]+(b[len-1] << 4)+len;
		return 0;
	}
};

/****************************************************************************/
// CommandLineParser
//
// This class implements a simple command line parser
//
/****************************************************************************/
class CommandLineParser
{
// Class data
public:
	typedef std::list<ParamValue> ParamList;
	typedef ParamList::iterator iterator;
private:
	const TCHAR* m_Program;
	ParamList m_lstParams;

// Constructor
public:
    CommandLineParser(int argc, TCHAR* argv[])
	{
		m_lstParams.clear();
		AddArgs(argc, argv);
	}

// Search function
public:
	iterator find(const TCHAR *param, bool caseSensitive=false)
	{
		ParamList::iterator it;
		for (it = m_lstParams.begin(); it != m_lstParams.end(); ++it)
		{
			if ( (!caseSensitive && !_tcsicmp(it->Name, param)) ||
				 (caseSensitive && !_tcscmp(it->Name, param)) )
				 return it;
		}
		return m_lstParams.end();
	}

// Properties
public:
	__declspec(property(get=get_Program)) const TCHAR* ProgramName;

// Accessors
public:
	const TCHAR* get_Program() const { return m_Program; }
	int size() { return static_cast<int>(m_lstParams.size()); }
	bool empty() { return m_lstParams.empty(); }
	void clear() { m_lstParams.clear();	}
	iterator pop_front() { return m_lstParams.erase(m_lstParams.begin()); }
	iterator pop_back() { return m_lstParams.erase(--m_lstParams.end()); }
	iterator begin() { return m_lstParams.begin(); }
	iterator end() { return m_lstParams.end(); }
	iterator erase(ParamList::iterator it) { return m_lstParams.erase(it); }

// Internal functions
private:
	int AddArgs(int argc, TCHAR* argv[]) 
	{
		if (argc > 0)
		{
			m_Program = argv[0];

			TCHAR* pMatch;
			for (int i = 1; i < argc; ++i)
			{
				TCHAR* pArg = argv[i];
				if (*pArg == _T('-') || *pArg == _T('/'))
					++pArg;

				if ( (pMatch=_tcschr(pArg,_T('=')) ) != NULL )
				{
					if (*(pMatch+1)==_T('\0'))
						m_lstParams.push_back(ParamValue(pArg));
					else
					{
						*pMatch = _T('\0');
						m_lstParams.push_back(ParamValue(pArg, pMatch+1));
					}
				}
				else
					m_lstParams.push_back(ParamValue(pArg));
			}
		}
		return static_cast<int>(m_lstParams.size());
	}
};

} // namespace JTI_Util

#endif // __COMMANDLINE_CommandLineParser_H_INCL__
