/****************************************************************************/
//
// ThunkWriter.h
//
// This header implements a class which is used to create thunks for
// static method invocations of method functions.
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

#ifndef __THUNK_WRITER_INCL__
#define __THUNK_WRITER_INCL__

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#ifndef _WINBASE_
	#define _WIN32_WINNT 0x0500
	#include <windows.h>
#endif

namespace JTI_Util
{
/******************************************************************************/
// BuildThunk (0 or 1 parameter version)
//
// Function which builds a thunking piece of code for invoking a method call
//
/******************************************************************************/
template <class _ResultType, class _Ty, class _Param>
_ResultType BuildThunk(_Ty* pType, void (_Ty::*classAddr)(_Param))
{
#pragma pack(1)
	struct ThunkTemplate 
	{
		BYTE pushInstr_1;
		DWORD_PTR thisPtr;
		BYTE popECX_1;
		BYTE pushInstr_2;
		void (_Ty::*addrPtr)(_Param);
		BYTE retInstr_1;
	};
#pragma pack()

	ThunkTemplate* pThunk = reinterpret_cast<ThunkTemplate*>(malloc(sizeof(ThunkTemplate)));
	pThunk->pushInstr_1 = 0x68;
	pThunk->thisPtr = (DWORD_PTR) pType;
	pThunk->popECX_1 = 0x59;
	pThunk->pushInstr_2 = 0x68;
	pThunk->addrPtr = classAddr;
	pThunk->retInstr_1 = 0xC3;

	DWORD dwOldProtect;
	VirtualProtect(pThunk, sizeof(ThunkTemplate), PAGE_EXECUTE_WRITECOPY, &dwOldProtect);
	return reinterpret_cast<_ResultType>(pThunk);
}

/******************************************************************************/
// BuildThunk (2 parameter version)
//
// Function which builds a thunking piece of code for invoking a method call
//
/******************************************************************************/
template <class _ResultType, class _Ty, class _Param1, class _Param2>
_ResultType BuildThunk(_Ty* pType, void (_Ty::*classAddr)(_Param1, _Param2))
{
#pragma pack(1)
	struct ThunkTemplate 
	{
		BYTE pushInstr_1;
		DWORD_PTR thisPtr;
		BYTE popECX_1;
		BYTE pushInstr_2;
		void (_Ty::*addrPtr)(_Param1, _Param2);
		BYTE retInstr_1;
	};
#pragma pack()

	ThunkTemplate* pThunk = reinterpret_cast<ThunkTemplate*>(malloc(sizeof(ThunkTemplate)));
	pThunk->pushInstr_1 = 0x68;
	pThunk->thisPtr = (DWORD_PTR) pType;
	pThunk->popECX_1 = 0x59;
	pThunk->pushInstr_2 = 0x68;
	pThunk->addrPtr = classAddr;
	pThunk->retInstr_1 = 0xC3;

	DWORD dwOldProtect;
	VirtualProtect(pThunk, sizeof(ThunkTemplate), PAGE_EXECUTE_WRITECOPY, &dwOldProtect);
	return reinterpret_cast<_ResultType>(pThunk);
}

/******************************************************************************/
// BuildThunk (3 parameter version)
//
// Function which builds a thunking piece of code for invoking a method call
//
/******************************************************************************/
template <class _ResultType, class _Ty, class _Param1, class _Param2, class _Param3>
_ResultType BuildThunk(_Ty* pType, void (_Ty::*classAddr)(_Param1, _Param2, _Param3))
{
#pragma pack(1)
	struct ThunkTemplate 
	{
		BYTE pushInstr_1;
		DWORD_PTR thisPtr;
		BYTE popECX_1;
		BYTE pushInstr_2;
		void (_Ty::*addrPtr)(_Param1, _Param2, _Param3);
		BYTE retInstr_1;
	};
#pragma pack()

	ThunkTemplate* pThunk = reinterpret_cast<ThunkTemplate*>(malloc(sizeof(ThunkTemplate)));
	pThunk->pushInstr_1 = 0x68;
	pThunk->thisPtr = (DWORD_PTR) pType;
	pThunk->popECX_1 = 0x59;
	pThunk->pushInstr_2 = 0x68;
	pThunk->addrPtr = classAddr;
	pThunk->retInstr_1 = 0xC3;

	DWORD dwOldProtect;
	VirtualProtect(pThunk, sizeof(ThunkTemplate), PAGE_EXECUTE_WRITECOPY, &dwOldProtect);
	return reinterpret_cast<_ResultType>(pThunk);
}

/******************************************************************************/
// BuildThunk (4 parameter version)
//
// Function which builds a thunking piece of code for invoking a method call
//
/******************************************************************************/
template <class _ResultType, class _Ty, class _Param1, class _Param2, class _Param3, class _Param4>
_ResultType BuildThunk(_Ty* pType, void (_Ty::*classAddr)(_Param1, _Param2, _Param3, _Param4))
{
#pragma pack(1)
	struct ThunkTemplate 
	{
		BYTE pushInstr_1;
		DWORD_PTR thisPtr;
		BYTE popECX_1;
		BYTE pushInstr_2;
		void (_Ty::*addrPtr)(_Param1, _Param2, _Param3, _Param4);
		BYTE retInstr_1;
	};
#pragma pack()

	ThunkTemplate* pThunk = reinterpret_cast<ThunkTemplate*>(malloc(sizeof(ThunkTemplate)));
	pThunk->pushInstr_1 = 0x68;
	pThunk->thisPtr = (DWORD_PTR) pType;
	pThunk->popECX_1 = 0x59;
	pThunk->pushInstr_2 = 0x68;
	pThunk->addrPtr = classAddr;
	pThunk->retInstr_1 = 0xC3;

	DWORD dwOldProtect;
	VirtualProtect(pThunk, sizeof(ThunkTemplate), PAGE_EXECUTE_WRITECOPY, &dwOldProtect);
	return reinterpret_cast<_ResultType>(pThunk);
}

/******************************************************************************/
// FreeThunk
//
// Frees the memory associated with the thunk
//
/******************************************************************************/
template <class _ResultType>
void FreeThunk(_ResultType pfunc)
{
	LPVOID pvFunc = reinterpret_cast<LPVOID>(pfunc);
	DWORD dwOldProtect;
	VirtualProtect(pvFunc, 20, PAGE_READWRITE, &dwOldProtect);
	free(pvFunc);
}

} // namespace JTI_Util
#endif // __THUNK_WRITER_INCL__
