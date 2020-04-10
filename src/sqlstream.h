/****************************************************************************/
//
// SQLStream.h
//
// This header implements each of the data stream classes which tie our
// field names to value types.
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
// 02/05/2000  MCS   Adopted for SimulRing projects
// 01/08/2002  MCS	 Moved most functions to inline variety.
//
/****************************************************************************/

#ifndef __SQLSTREAM_H_INC__
#define __SQLSTREAM_H_INC__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/*----------------------------------------------------------------------------
	INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <vector>
#include <string>
#include <stdlib.h>
#include <JTIUtils.h>
#include <comutil.h>

namespace JTI_Dac
{
/****************************************************************************/
// CDlParam
//
// Basic parameter class - defines base behavior for all derived parameter
// "breaker" classes.
//
/****************************************************************************/
class CDlParam  
{
// Class data
private:
	bool isDefined_;				// true/false if the parameter is defined
	const std::wstring fieldName_;	// Field name

// Constructor
protected:
	explicit CDlParam(const wchar_t *szName, bool bIsDefined)
		: isDefined_(bIsDefined), fieldName_(szName) {/* */};
// Destructor
public:
	virtual ~CDlParam() {}

// Access methods
public:
	// Returns the validated value combined with the field name
	std::pair<bool,std::pair<std::wstring,std::wstring> > AsValidatedPair() const {
		std::pair<bool,std::wstring> pairValue = ValidatedValue(); //lint !e1933
		std::pair<std::wstring,std::wstring> pairParam(fieldName_,pairValue.second);
		return std::make_pair(pairValue.first, pairParam);
	}

	// Required implementation by derived classes
	virtual std::pair<bool,std::wstring> ValidatedValue() const = 0;

// Methods
protected:
	bool get_IsDefined() const { return isDefined_; }
	void set_IsDefined(bool bIsDefined) { isDefined_= bIsDefined;}

// Non-implemented methods
private:
	CDlParam(const CDlParam&);
	const CDlParam& operator=(const CDlParam&);
};

/****************************************************************************/
// CDlBstr
//
// This encapsulates the insertion of a string value into a execution
// string.
//
/****************************************************************************/
class CDlBstr : public CDlParam  
{
// Class data
private:
	std::wstring value_;
	const bool isNullValid_;

// Constructor/Destructor
public:
	CDlBstr(const wchar_t *szName, const wchar_t *rgWchar, bool bNullValid= false) : 
		CDlParam(szName, true), value_(rgWchar), isNullValid_(bNullValid) {/* */}
	CDlBstr(const wchar_t *rgWchar, bool bNullValid= false) : 
		CDlParam(L"", true), value_(rgWchar), isNullValid_(bNullValid) {/* */}

// Overrides from the CDlParam class
public:
	virtual std::pair<bool,std::wstring> ValidatedValue() const;

// Non-implemented methods
private:
	CDlBstr(const CDlBstr&);
	const CDlBstr& operator=(const CDlBstr&);
};

/****************************************************************************/
// CDlBool
//
// This encapsulates the insertion of a boolean value into a execution
// string.
//
/****************************************************************************/
class CDlBool : public CDlParam  
{
// Class data
private:
	bool value_;

// Constructor/Destructor
public:
	CDlBool(const wchar_t *szName, VARIANT_BOOL bVal) : 
		CDlParam(szName, true), value_(bVal ? true: false) {/* */}
	CDlBool(VARIANT_BOOL bVal) : 
		CDlParam(L"", true), value_(bVal ? true: false) {/* */}
	CDlBool(bool bVal) : 
		CDlParam(L"", true), value_(bVal ? true: false) {/* */}

// Overrides from CDlParam
public:
	virtual std::pair<bool,std::wstring> ValidatedValue() const 
	{ 
		return std::make_pair(get_IsDefined(), value_ ? std::wstring(L"1") : std::wstring(L"0")); 
	}

// Non-implemented methods
private:
	CDlBool(const CDlBool&);
	const CDlBool& operator=(const CDlBool&);
};

/****************************************************************************/
// CDlLong
//
// This encapsulates the insertion of a numeric long value into the data
// string.
//
/****************************************************************************/
class CDlLong : public CDlParam  
{
// Class data
private:
	long value_;

// Constructor/Destructor
public:
	CDlLong(const wchar_t *szName, long lVal) : 
		CDlParam(szName, true), value_(lVal) {/* */}
	CDlLong(long lVal) : 
		CDlParam(L"", true), value_(lVal) {/* */}

// Overrides from CDlParam
public:
	virtual std::pair<bool,std::wstring> ValidatedValue() const {
		wchar_t chBuff[50];
		return (get_IsDefined())
				? std::make_pair(true, std::wstring(_ltow(value_, chBuff, 10)))
				: std::make_pair(false, std::wstring(_ltow(value_, chBuff, 10)));
	}

// Non-implemented methods
private:
	CDlLong(const CDlLong&);
	const CDlLong& operator=(const CDlLong&);
};

/****************************************************************************/
// CDlCount
//
// A "Count", if defined, is always valid at face value and may be
// negative. There is no special value which should be treated as
// an empty parameter.
//
/****************************************************************************/
typedef CDlLong CDlCount;

/****************************************************************************/
// CDlDouble
//
// Implements a double value insertor into the data stream
//
/****************************************************************************/
class CDlDouble : public CDlParam  
{
// Class data
private:
	double value_;

// Constructor/Destructor
public:
	CDlDouble(const wchar_t *szName, double dblVal) : 
		CDlParam(szName, true), value_(dblVal) {/* */}
	CDlDouble(double dblVal) : 
		CDlParam(L"", true), value_(dblVal) {/* */}

// Overrides from CDlParam
public:
	virtual std::pair<bool,std::wstring> ValidatedValue() const {
		wchar_t chwBuff[20]; char chBuff[20]; _gcvt(value_,15,chBuff);
		return (get_IsDefined())
				? std::make_pair(true, std::wstring(JTI_Util::A2WHelper(chBuff,chwBuff,20)))
				: std::make_pair(false, std::wstring(JTI_Util::A2WHelper(chBuff,chwBuff,20)));
	}

// Non-implemented methods
private:
	CDlDouble(const CDlDouble&);
	const CDlDouble& operator=(const CDlDouble&);
};

/****************************************************************************/
// CDlDate
//
// Implements a date/time stamp insertor into the data stream
//
/****************************************************************************/
class CDlDate : public CDlParam
{
// Class data
private:
	double value_;

// Constructor/Destructor
public:
	CDlDate(const wchar_t *szName, double dblVal) :
		CDlParam(szName, true), value_(dblVal) {/* */}
	CDlDate(double dblVal) :
		CDlParam(L"", true), value_(dblVal) {/* */}

// Overrides from CDlParam
public:
	virtual std::pair<bool,std::wstring> ValidatedValue() const;

// Non-implemented methods
private:
	CDlDate(const CDlDate&);
	const CDlDate& operator=(const CDlDate&);
};

/****************************************************************************/
// CDlKey
//
// Implements a numeric key value (long)
//
/****************************************************************************/
class CDlKey : public CDlParam
{
// Class data
private:
	long value_;

// Constructor/Destructor
public:
	CDlKey(const wchar_t *szName, long lVal) : 
		CDlParam(szName, true), value_(lVal) {/* */}
	CDlKey(long lVal) : 
		CDlParam(L"", true), value_(lVal) {/* */}

// Overrides from CDlParam
public:
	virtual std::pair<bool,std::wstring> ValidatedValue() const {
		wchar_t chBuff[50];
		return (get_IsDefined())
				? std::make_pair(Validate(), std::wstring(_ltow(value_, chBuff, 10)))
				: std::make_pair(false, std::wstring(_ltow(value_, chBuff, 10)));
	}
// Internal methods
private:
	bool Validate() const { return value_ != 0;}

// Non-implemented methods
private:
	CDlKey(const CDlKey&);
	const CDlKey& operator=(const CDlKey&);
};

/****************************************************************************/
// CDlCmd
//
// Basic command broker for executing commands against an ADO recordset.
//
/****************************************************************************/
class CDlCmd  
{
// Constructor/Destructor
public:
	virtual ~CDlCmd() = 0;
protected:
	CDlCmd() {/* */}

// Required overrides from derived classes
public:
	virtual std::wstring BuildCommand() const = 0;
	virtual operator _bstr_t() const = 0;	//lint !e1930
};

// Required for virtual destructor; even abstract methods.
inline CDlCmd::~CDlCmd() {}

/****************************************************************************/
// CDlCmdSQL
//
// Command broker which executes an SQL statement with optional parameters
//
/****************************************************************************/
class CDlCmdSQL : public CDlCmd  
{
// Class data
protected:
	enum { NOMINAL_ARG_COUNT = 5 };
	std::vector<std::wstring> argsArray_;
	const wchar_t *sqlText_;
	long argLength_;

// Constructor/Destructor
public:
	explicit CDlCmdSQL(const wchar_t* szName) : 
		CDlCmd(), sqlText_(szName), argLength_(0) { argsArray_.reserve(NOMINAL_ARG_COUNT); }

// Insertion operators
public:
	CDlCmdSQL& operator <<(const CDlParam &param)
	{
		std::pair<bool, std::pair<std::wstring,std::wstring> > pairValidated = param.AsValidatedPair();
		if (pairValidated.first) {
			const std::wstring& strValue = pairValidated.second.second;
			argsArray_.push_back(strValue);
			argLength_ += static_cast<long>(strValue.length());
		}
		return *this;
	}

// Methods
public:
	virtual std::wstring BuildCommand() const;
	void DropParams() 
	{
		argsArray_.clear();
		argLength_ = 0;
		argsArray_.reserve(NOMINAL_ARG_COUNT);
	}
   	virtual operator _bstr_t() const	//lint !e1930 Implicit conversion to another type
	{
		return _bstr_t(BuildCommand().c_str());
	}

// Non-implemented methods
private:
	CDlCmdSQL(const CDlCmdSQL&);
	const CDlCmdSQL& operator=(const CDlCmdSQL&);
};

/****************************************************************************/
// CDlCmdExec
//
// Command broker which performs an "EXEC xxx" for executing a stored
// procedure on the database.
//
/****************************************************************************/
class CDlCmdExec : public CDlCmdSQL	//lint !e1932
{
// Class data
protected:
	std::vector<std::pair<std::wstring,std::wstring> > argsPairArray_;

// Constructor/Destructor
public:
	explicit CDlCmdExec(const wchar_t* pszCommand) : 
		CDlCmdSQL(pszCommand) { argsPairArray_.reserve(NOMINAL_ARG_COUNT); }

// Insertion operator
public:
	CDlCmdExec& operator <<(const CDlParam &param) 
	{
		const long PARAM_OVERHEAD_LEN = 2; // Room for '@' + '=' 
		std::pair<bool, std::pair<std::wstring,std::wstring> > pairValidated= param.AsValidatedPair();
		if (pairValidated.first) {
			argsPairArray_.push_back(pairValidated.second);
			argLength_ += (PARAM_OVERHEAD_LEN + static_cast<long>(pairValidated.second.first.length() + pairValidated.second.second.length()));
		}
		return *this;
	}                                                                                                                   

// Methods
public:
	virtual std::wstring BuildCommand() const;
   	virtual operator _bstr_t() const	//lint !e1930 Implicit conversion to another type
	{
		return _bstr_t(BuildCommand().c_str());
	}

// Non-implemented methods
private:
	CDlCmdExec(const CDlCmdExec&);
	const CDlCmdExec& operator=(const CDlCmdExec&);
};

/****************************************************************************/
// CDlCmdShape
//
// This is used solely to generate an ADO Shape command expression--
// compose the overall Shape  command expression by combining the 
// subexpression values of the member commands with some additional 
// syntactic sugar.
//
/****************************************************************************/
class CDlCmdShape : public CDlCmd  
{
// Class data
private:
	const std::wstring textHeader_;
	std::vector<std::pair<std::wstring,std::wstring> > shapeDetails_;

// Constructor/Destructor
public:
	CDlCmdShape(const CDlCmd *pHeader) : CDlCmd(), textHeader_(pHeader->BuildCommand()), shapeDetails_() {/* */}

// Methods
public:
	virtual std::wstring BuildCommand() const;
	void AddDetailRecord(const CDlCmd* pcmdParam, const wchar_t* pszJoinType) { 	
		shapeDetails_.push_back(std::make_pair(pcmdParam->BuildCommand(), std::wstring(pszJoinType)));
	}
	virtual operator _bstr_t() const	//lint !e1930
	{
		return _bstr_t(BuildCommand().c_str());
	}

// Non-implemented methods
private:
	CDlCmdShape(const CDlCmdShape&);
	const CDlCmdShape& operator=(const CDlCmdShape&);
};

}// namespace JTI_Dac
#endif // __SQLSTREAM_H_INC__
