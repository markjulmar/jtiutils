/****************************************************************************/
//
// MemoryMappedFile.h
//
// This class implements a memory map file wrapper class which allows
// the caller to manipulate a file as if it were a block of memory.
//
// Copyright (C) 2004 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

#ifndef __MEMORYMAP_INCL__
#define __MEMORYMAP_INCL__

/*----------------------------------------------------------------------------
// INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <windows.h>
#include <jtiutils.h>

/*****************************************************************************/
// PC-Lint options
//
//lint -save
//
/*****************************************************************************/

namespace JTI_Util
{
/****************************************************************************/
// MemoryMappedFile
//
// This class wraps a memory mapped file.
//
/****************************************************************************/
class MemoryMappedFile
{
// Class data
private:
	void* lpv_;
	__int64 size_;

// Constructor
public:
	MemoryMappedFile(const char* pszFile, 
			DWORD dwDesiredAccess = GENERIC_READ, 
			DWORD dwShareMode = FILE_SHARE_READ, 
			LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL,
			DWORD dwCreationDisposition = OPEN_EXISTING,
			DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL) : lpv_(0)
	{
		USES_CONVERSION;
		HANDLE hFile = CreateFile(A2T(const_cast<char*>(pszFile)), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			// Get the total file size to map
			DWORD dwSizeHigh = 0, dwSizeLow = GetFileSize(hFile, &dwSizeHigh);

			// Store off the file size
			size_ = ((static_cast<__int64>(dwSizeHigh)) << 32) + dwSizeLow;

			// Create a file mapping object; it increments the ref count on the original file so close
			// it now to avoid resource leaks.
			DWORD protectionType = ((dwDesiredAccess & (FILE_WRITE_ATTRIBUTES|FILE_WRITE_DATA|FILE_WRITE_EA)) > 0) ? PAGE_READWRITE : PAGE_READONLY;
			HANDLE hFileMap = CreateFileMapping(hFile, lpSecurityAttributes, protectionType, dwSizeHigh, dwSizeLow, NULL);
			CloseHandle(hFile);

			// Map a view of the file; again close the handle we used.
			if (hFileMap != NULL) {
				protectionType = ((dwDesiredAccess & (FILE_WRITE_ATTRIBUTES|FILE_WRITE_DATA|FILE_WRITE_EA)) > 0) ? FILE_MAP_WRITE : FILE_MAP_READ;
				lpv_ = MapViewOfFile(hFileMap, protectionType, 0, 0, 0);
				CloseHandle(hFileMap);
			}
		}
	}

	~MemoryMappedFile()
	{
		if (lpv_ != NULL)
			UnmapViewOfFile(lpv_);
	}

// Properties
public:
	__declspec(property(get=get_Pointer)) void* Buffer;
	__declspec(property(get=get_IsValid)) bool IsValid;
	__declspec(property(get=get_Size)) __int64 Size;

// Property helpers
public:
	bool get_IsValid() { return (lpv_ != NULL); }
	void* get_Pointer() { return lpv_; }
	__int64 get_Size() { return size_; }
};
}// namespace JTI_Util
#endif // __MEMORYMAP_INCL__

