/****************************************************************************/
//
// JTIUtils.h
//
// Base Utility Library - includes all portions of the base support system
//
// Copyright (C) 1997-2004 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

/*----------------------------------------------------------------------------
//	INCLUDE FILES
-----------------------------------------------------------------------------*/
#include "stdafx.h"
#include <JTIUtils.h>

namespace JTI_Util {
namespace JTI_Internal {
/*****************************************************************************
** Procedure: _A2W::_A2W
** 
** Arguments: 'psz' - String to convert
**            'codePage' - Codepage to shift to
** 
** Returns: void
** 
** Description: Constructor for the ANSI to wide conversion
**
/****************************************************************************/
_A2W::_A2W(const char* psz, unsigned int codePage)
{ 
	if ((_pszBuff = (psz==NULL) ? 0 : _buff) != 0)
	{
		int nLength = MultiByteToWideChar(codePage, 0, psz, -1, NULL, 0) + 1;
		if (nLength > sizeofarray(_buff))
		{
			_pszBuff = new wchar_t[nLength];
			if (_pszBuff == NULL)
				throw std::bad_alloc("A2W failed to allocate buffer");
		}
		MultiByteToWideChar(codePage, 0, psz, -1, _pszBuff, nLength);
	}

}// _A2W::_A2W

/*****************************************************************************
** Procedure: _A2W::~_A2W
** 
** Arguments: void 
** 
** Returns: void
** 
** Description: Destructor
**
/****************************************************************************/
_A2W::~_A2W()
{
	// Destroy the buffer if not our internal array.
	if (_pszBuff != _buff)
		delete [] _pszBuff;

}// _A2W::~_A2W

/*****************************************************************************
** Procedure:  _W2A::_W2A
** 
** Arguments: 'psz' - String to convert
**            'codePage' - Codepage to shift to
** 
** Returns: void
** 
** Description: Constructor for the wide to ANSI conversion
**
/****************************************************************************/
_W2A::_W2A(const wchar_t* psz, unsigned int codePage)
{ 
	if ((_pszBuff = (psz==NULL) ? 0 : _buff) != 0)
	{
		int nLength = WideCharToMultiByte(codePage, 0, psz, -1, NULL, 0, NULL, NULL) + 1;
		if (nLength > sizeofarray(_buff))
		{
			_pszBuff = new char[nLength];
			if (_pszBuff == NULL)
				throw std::bad_alloc("W2A failed to allocate buffer");
		}
		WideCharToMultiByte(codePage, 0, psz, -1, _pszBuff, nLength, NULL, NULL);
	}

}// _W2A::_W2A

/*****************************************************************************
** Procedure:  _W2A::~_W2A
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: Destructor
**
/****************************************************************************/
_W2A::~_W2A()
{
	// Destroy the buffer if not our internal array.
	if (_pszBuff != _buff)
		delete [] _pszBuff;

}// _W2A::~_W2A
} // namespace JTI_Internal

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
bool patchk(const wchar_t *szPattern, const wchar_t *szString)
{
	if (*szString == L'\0')
		return (*szPattern == L'\0') ? true : false;
	if (*szPattern == L'*') {
		if (*(szPattern+1) == L'\0')
			return true;
		int nLen = lstrlenW(szString);
		for (int i = 0; i <= nLen; i++) {
			if (patchk(szPattern+1, szString+i))
				return true;
		}
	}
	if ((*szString == *szPattern) || (*szPattern == L'?')) {
		if (patchk(szPattern+1, szString+1))
			return true;
	}
	return false;

}// patchk

/*****************************************************************************
** Procedure:  patchk
** 
** Arguments:  szPattern - Pattern to match
**             szString - Test string
** 
** Returns: TRUE if the string matches the given pattern
** 
** Description: This function pattern matches two ansi strings.
**
/****************************************************************************/
bool patchk(const char *szPattern, const char *szString)
{
	if (*szString == '\0')
		return (*szPattern == '\0') ? true : false;
	if (*szPattern == '*') {
		if (*(szPattern+1) == '\0')
			return true;
		int nLen = lstrlenA(szString);
		for (int i = 0; i <= nLen; i++) {
			if (patchk(szPattern+1, szString+i))
				return true;
		}
	}
	if ((*szString == *szPattern) || (*szPattern == '?')) {
		if (patchk(szPattern+1, szString+1))
			return true;
	}
	return false;

}// patchk

/*****************************************************************************
** Procedure: dprintf
** 
** Arguments: 'pszFormat' - Variable parameter format string
** 
** Returns: Count of characters
** 
** Description: This outputs a debug line to the debug console
**
/****************************************************************************/
int dprintf(const char *pszFormat, ...)
{
	va_list args;
	va_start(args, pszFormat);

	char chBuff[512];
	int nCount = _vsnprintf(chBuff, sizeof(chBuff), pszFormat, args);
	va_end(args); //lint !e1924

	OutputDebugStringA(chBuff);

	return nCount;

}// dprintf

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
wchar_t* A2WHelper(const char* psz, wchar_t* pszBuffer, int nSize)
{
	int nCount = MultiByteToWideChar(CP_ACP, 0, psz, -1, pszBuffer, nSize);
	return (nCount == 0 || nCount > nSize) ? NULL : pszBuffer;

}// A2WHelper

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
char* W2AHelper(const wchar_t* psz, char* pszBuffer, int nSize)
{
	int nCount = WideCharToMultiByte(CP_ACP, 0, psz, -1, pszBuffer, nSize, NULL, NULL);
	return (nCount == 0 || nCount > nSize) ? NULL : pszBuffer;

}// W2AHelper

/*****************************************************************************
** Procedure:  jtiexception::jtiexception
** 
** Arguments:  'serr' - Error string
** 
** Returns: void
** 
** Description: Constructor for jtiexception which gets the last
**              reported Win32 error code
**
/****************************************************************************/
jtiexception::jtiexception(const char* serr) : 
	std::exception(serr), err_(GetLastError()) 
{
}// jtiexception::jtiexception
} // namespace JTI_Util