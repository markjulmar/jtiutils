/****************************************************************************/
//
// Base64.h
//
// This header implements a class which is used to encode/decode
// Base64 (mime) data.
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

#ifndef __JTI_BASE64_H_INCLUDED_
#define __JTI_BASE64_H_INCLUDED_

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#ifndef _WINBASE_
#define _WIN32_WINNT 0x0500
#include <windows.h>
#endif
#include <tchar.h>
#include <string>
#include <vector>

#define _JTI_NO_LINK_RECORD
#include <JTIUtils.h>
#undef _JTI_NO_LINK_RECORD

namespace JTI_Util
{
/******************************************************************************/
// Base64
//
// This implements a encode/decode class for Base64.
//
/******************************************************************************/
class Base64
{
// Typedefs
public:
	typedef std::vector<unsigned char> ByteArray;

// Implementation
public:
	// EncodeFile - This function takes a filename and returns a Base64 
	// encoded buffer in a string.
	static tstring EncodeFile(const TCHAR* pszFilename)
	{
		// Open the file for read access.
		HANDLE hFile = CreateFile(pszFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			// Get the file size, we assume the file is smaller than 4G
			DWORD dwSize = GetFileSize(hFile, NULL);

			// Create a file mapping object; it increments the ref count on the original file so close
			// it now to avoid resource leaks.
			HANDLE hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, dwSize, NULL);
			CloseHandle(hFile);
			if (hFileMap != NULL)
			{
				// Map a view of the file; again close the handle we used.
				LPVOID lpvFile = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
				CloseHandle(hFileMap);
				if (lpvFile != NULL)
				{
					// Encode and release the view on the file
					tstring s = EncodeBuffer(lpvFile, dwSize);
					UnmapViewOfFile(lpvFile);
					return s;
				}
			}
		}
		return tstring();
	}

	// EncodeBuffer - This function takes a buffer/length combination and returns
	// a Base64 encoded buffer in a string.
	static tstring EncodeBuffer(const void* in, size_t length)
	{
		const unsigned char* data = reinterpret_cast<const unsigned char*>(in);

		// 3 unsigned chars encode to 4 chars.  Output is always an even
		// multiple of 4 characters.
		TCHAR* alphabet =	_T("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=");
		TCHAR out[5] = {0}; 
		tstring s; 
		
		s.reserve(((length + 2) / 3) * 4);
		for (size_t i = 0; i < length; i += 3)
		{
			bool quad = false, trip = false;
			int val = (0xFF & (int) data[i]);
			val <<= 8;
			if ((i + 1) < length) 
			{
				val |= (0xFF & (int) data[i + 1]);
				trip = true;
			}

			val <<= 8;
			if ((i + 2) < length) 
			{
				val |= (0xFF & (int) data[i + 2]);
				quad = true;
			}
			
			out[3] = alphabet[(quad ? (val & 0x3F) : 64)];
			val >>= 6;
			out[2] = alphabet[(trip ? (val & 0x3F) : 64)];
			val >>= 6;
			out[1] = alphabet[val & 0x3F];
			val >>= 6;
			out[0] = alphabet[val & 0x3F];
			
			s.append(out);
		}
		return s;
	}

	static ByteArray DecodeString(const tstring& data)
	{
		return DecodeBuffer(data.c_str(), data.size());
	}

	static ByteArray DecodeBuffer(const void* inData, size_t length)
	{
		const unsigned char* data = static_cast<const unsigned char*>(inData);
		unsigned char base64Decode_[256]; int i;
		for (i = 0; i < 256; i++)
			base64Decode_[i] = 0xff;
		for (i = 'A'; i <= 'Z'; i++)
			base64Decode_[i] = (unsigned char)(i - 'A');
		for (i = 'a'; i <= 'z'; i++)
			base64Decode_[i] = (unsigned char)(26 + i - 'a');
		for (i = '0'; i <= '9'; i++)
			base64Decode_[i] = (unsigned char)(52 + i - '0');
		base64Decode_['+'] = 62;
		base64Decode_['/'] = 63;

		size_t len = ((length + 3) / 4) * 3;
		if (length > 0 && data[length - 1] == '=')
			--len;
		if (length > 1 && data[length - 2] == '=')
			--len;

		ByteArray out; 
		out.reserve(len);

		int shift = 0; // # of excess bits stored in accum
		int accum = 0; // excess bits

		for (size_t ix = 0; ix < length; ++ix)
		{
			TCHAR ch = data[ix];
			int value = base64Decode_[ch];	// ignore high byte of char
			if (value >= 0)					// skip over non-code
			{
				accum <<= 6;				// bits shift up by 6 each time through
				shift += 6;					// loop, with new bits being put in
				accum |= value;				// at the bottom.
				if (shift >= 8)				// whenever there are 8 or more shifted in,
				{ 
					shift -= 8;				// write them out (from the top, leaving any
					unsigned char ch = static_cast<unsigned char>((accum >> shift) & 0xff);
					out.push_back(ch);		// excess at the bottom for next iteration.
				}
			}
		}
		return out;
	}
};

} // namespace JTI_Util

#endif // __JTI_BASE64_H_INCLUDED_