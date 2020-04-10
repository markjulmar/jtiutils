/****************************************************************************/
//
// SQLStream.cpp
//
// This file implements the larger data stream functions for the DAC
// building class.
//
// Copyright (C) 1999-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#include "stdafx.h"
#include "sqlstream.h"
#include "TraceLogger.h"

namespace JTI_Dac
{
/**************************************************************************************
** Procedure: CDlDate::ValidatedValue
**
** Arguments: void
**
** Returns:	Bool/BSTR pair for the given value.
**
** Description: This is used during the column creation process to validate the
**              internal boolean value.
**
**************************************************************************************/
std::pair<bool,std::wstring> CDlDate::ValidatedValue() const
{
	SYSTEMTIME st;
	wchar_t szAnsiDateTime[255] = {0};
	std::pair<bool,std::wstring> pairValue= std::make_pair(false, std::wstring());
	if (get_IsDefined() && VariantTimeToSystemTime(value_, &st))	
	{
		if (wsprintfW(szAnsiDateTime, L"'%04d-%02d-%02d %02d:%02d:%02d'", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond))
			pairValue= std::make_pair(true, std::wstring(szAnsiDateTime));
	}
	return pairValue;

}// CDlDate::ValidatedValue

/**************************************************************************************
** Procedure: CDlBstr::ValidatedValue
**
** Arguments: void
**
** Returns:	Bool/BSTR pair for the given value.
**
** Description: This is used during the column creation process to validate the
**              internal boolean value.
**
**************************************************************************************/
std::pair<bool,std::wstring> CDlBstr::ValidatedValue() const 
{
	// Bracket entire string in single quotes and escape each embedded quote with 
	// by an additional single quote. Zero length string generates destination 
	// string of two consecutive single quotes
	const wchar_t *pSrc = value_.c_str();
	const wchar_t QUOTE= L'\'';

	// Begin building the destination string; it starts with a quote
	std::wstring strValue; strValue.reserve(value_.length() * 2);
	strValue = QUOTE;
	    
	// If the string is not null --
	if (!value_.empty())
	{
    	while (*pSrc)
    	{	
    		// double-copy sequence of quotes, if any
    		while (*pSrc && (*pSrc == QUOTE))
    		{	
    			strValue += *pSrc;	
    			strValue += *pSrc++;	
    		}

    		// pSrc is pointing at non-quote or reached NULL
    		size_t lFoundAtIdx= wcscspn(pSrc, L"'");	
    		if (!lFoundAtIdx) 
				continue; //reached NULL	

			// Copy the new section over and increment our buffer.
			strValue += std::wstring(pSrc,lFoundAtIdx);
    		pSrc += lFoundAtIdx; // points to quote or terminating NULL	
    	}
	}	
	    
	// .. and ends with a quote
	strValue += QUOTE;

	return (value_.empty()) ? std::make_pair(isNullValid_, strValue) : std::make_pair(true, strValue);

}// CDlBstr::ValidatedValue

/**************************************************************************************
** Procedure: CDlCmdSQL::BuildCommand
**
** Arguments: void
**
** Returns:	String with full command to execute
**
** Description: This takes all the parameters and builds a string to send to the 
**              data driver.
**
**************************************************************************************/
std::wstring CDlCmdSQL::BuildCommand() const
{
	std::wstring strExpression(sqlText_); 
	strExpression.reserve(lstrlenW(sqlText_)+argLength_);

	// Walk the expressions, changing out the question marks which 
	// indicate parameters for the real string in our array.
	long nCurr = 0;
	std::wstring::size_type idx = strExpression.find_first_of(L'?', 0);
	while (idx != std::wstring::npos)
	{
		JTI_ASSERT(static_cast<long>(argsArray_.size()) > nCurr);
		if (static_cast<long>(argsArray_.size()) > nCurr)
			strExpression.replace(idx,1,argsArray_[nCurr].c_str());
		++nCurr; idx = strExpression.find_first_of(L'?', idx+1);
	}

	return strExpression;

}// CDlCmdSQL::BuildCommand

/**************************************************************************************
** Procedure: CDlCmdExec::BuildCommand
**
** Arguments: void
**
** Returns:	String with full command to execute
**
** Description: This takes all the parameters and builds a string to send to the 
**              data driver.
**
**************************************************************************************/
std::wstring CDlCmdExec::BuildCommand() const
{
	const wchar_t* gpszExec = L"Exec ";
	std::wstring strExpression;	 
	strExpression.reserve(lstrlenW(gpszExec)+lstrlenW(sqlText_)+1+argLength_+static_cast<int>(argsPairArray_.size()));
	strExpression = gpszExec;

	strExpression += sqlText_;
	strExpression += L' ';

	// Append all paramater expressions with a trailing comma.
	for(std::vector<std::pair<std::wstring,std::wstring> >::const_iterator it = argsPairArray_.begin(); it != argsPairArray_.end(); ++it)
	{
		strExpression += L'@';
		strExpression += it->first;
		strExpression += L'=';
		strExpression += it->second;
		strExpression += L',';
	}

	// Remove the trailing comma or blank.
	strExpression.erase(strExpression.length()-1);
	return strExpression;

}// CDlCmdExec::BuildCommand

/**************************************************************************************
** Procedure: CDlCmdShape::BuildCommand
**
** Arguments: void
**
** Returns:	String representing command to execute
**
** Description: This builds the shape command to send to the driver.
**
** The format for shape commands are:
**
** SHAPE  { primary SQL statement }
** APPEND ({ secondary SQL statement}
** RELATE field to field) AS primary_field ,
**        ({ ... })
**
**************************************************************************************/
std::wstring CDlCmdShape::BuildCommand() const
{
	static const wchar_t * const gSyntax_[] = {
		L"SHAPE { ",		// 0
		L" } APPEND ",		// 1
		L" ({ ",			// 2
		L" } ",				// 3
		L", "				// 4
	};

	// Build the shape command
	std::wstring sResult = gSyntax_[0];
	sResult += textHeader_;
	sResult += gSyntax_[1];

	for (std::vector<std::pair<std::wstring,std::wstring> >::const_iterator it = shapeDetails_.begin(); it != shapeDetails_.end(); ++it)
	{
		if (it != shapeDetails_.begin())
			sResult += gSyntax_[4];
		sResult += gSyntax_[2];
		sResult += it->first;
		sResult += gSyntax_[3];
		sResult += it->second;
	}
	return sResult;

}// CDlCmdShape::BuildCommand

}// namespace JTI_Dac
