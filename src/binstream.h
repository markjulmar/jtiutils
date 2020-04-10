/******************************************************************************/
//                                                                        
// binstream.h
//                                             
// This header implements a binary-based stream class comparable to IOStreams
// but storing data in a binary form rather than formatted I/O.
//
// Copyright (C) 1994-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
//                                                                        
/******************************************************************************/

#ifndef _BINSTREAM_INC_
#define _BINSTREAM_INC_

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#pragma warning(disable:4702)
#include <string>
#include <stdexcept>
#include <iterator>
#include <cstdlib>
#include <malloc.h>
#pragma warning (default:4702)

namespace JTI_Util
{
/******************************************************************************/
// schema_exception
//
// Internal exception generated when a iostream fails to read persistent
// data from the stream source.
//
// INTERNAL DATA STRUCTURE
//
/******************************************************************************/
class schema_exception : public std::runtime_error
{
public:
	explicit schema_exception(const std::string& str=std::string("")) : std::runtime_error(str) {/* */}
};

/******************************************************************************/
// binstream
//
// Base class for the binary input stream used to retrieve persistent object
// data from some location. This location could be the registry, a file, an OLE
// object, etc.
//
// INTERNAL DATA STRUCTURE
//
/******************************************************************************/
class binstream
{
// Class data
public:
	enum { 
		eofbit 	= 0x1,
		failbit = 0x2
	};
private:
	unsigned char bitFlags_;

// Destructor
public:
	binstream() : bitFlags_(0) {/* */}
	virtual ~binstream() {/* */}

// Overridable operations required in the derived classes
public:
	virtual bool open() { return true;}
	virtual void close() {/* */}
	virtual bool skip(int) { return false; }
	virtual bool peek(void*, unsigned int) const = 0;
	virtual bool read(void*, unsigned int) = 0;
	virtual bool write(const void*, unsigned int) = 0;

// Other methods
public:
	binstream& ignore(int nCount=1) { skip(nCount); return *this; }
	bool good() const { return (bitFlags_ & failbit) == 0; }
	bool eof() const { return (bitFlags_ & (eofbit|failbit)) != 0; }
	bool fail() const { return !good(); }
	
	unsigned char peek() const { char ch = 0; peek(&ch,1); return ch; }

	operator void*() { return (!fail()) ? (void*)1 : (void*)0; }
	bool operator !() { return fail(); }

// Internal methods
protected:
	void setbit(int bit) { bitFlags_ |= (bit&0xff); }
	void clrbit(int bit=0xff) { bitFlags_ &= ~(bit&0xff); }
};

// insertion operations
inline binstream& operator<<(binstream& stm, long l) {
	if(!stm.write(&l, sizeof(long)))
		throw schema_exception(std::string("operator<<(long)"));
	return stm;
}

inline binstream& operator<<(binstream& stm, float f) {
	if(!stm.write(&f, sizeof(float)))
		throw schema_exception(std::string("operator<<(float)"));
	return stm;
}

inline binstream& operator<<(binstream& stm, double d) {
	if(!stm.write(&d, sizeof(double)))
		throw schema_exception(std::string("operator<<(double)"));
	return stm;
}

inline binstream& operator<<(binstream& stm, __int64 d) {
	if(!stm.write(&d, sizeof(__int64)))
		throw schema_exception(std::string("operator<<(__int64)"));
	return stm;
}

inline binstream& operator<<(binstream& stm, bool f) {
	unsigned char c = (f == true) ? 
		static_cast<unsigned char>(0x1) : 
		static_cast<unsigned char>(0x0);
	if(!stm.write(&c, sizeof(unsigned char)))
		throw schema_exception(std::string("operator<<(bool)"));
	return stm;
}

inline binstream& operator<<(binstream& stm, int i) {
	long l = static_cast<long>(i);
	if(!stm.write(&l, sizeof(long)))
		throw schema_exception(std::string("operator<<(int)"));
	return stm;
}

inline binstream& operator<<(binstream& stm, short w) {
	if(!stm.write(&w, sizeof(short)))
		throw schema_exception(std::string("operator<<(short)"));
	return stm;
}

inline binstream& operator<<(binstream& stm, char ch) {
	if(!stm.write(&ch, sizeof(char)))
		throw schema_exception(std::string("operator<<(char)"));
	return stm;
}

inline binstream& operator<<(binstream& stm, unsigned char by) {
	if(!stm.write(&by, sizeof(unsigned char)))
		throw schema_exception(std::string("operator<<(unsigned char)"));
	return stm;
}

inline binstream& operator<<(binstream& stm, unsigned short w) {
	if(!stm.write(&w, sizeof(unsigned short)))
		throw schema_exception(std::string("operator<<(unsigned short)"));
	return stm;
}

inline binstream& operator<<(binstream& stm, unsigned int w) {
	unsigned long l = static_cast<unsigned long>(w);
	if(!stm.write(&l, sizeof(unsigned long)))
		throw schema_exception(std::string("operator<<(unsigned int)"));
	return stm;
}

inline binstream& operator<<(binstream& stm, unsigned long dw) {
	if(!stm.write(&dw, sizeof(unsigned long)))
		throw schema_exception(std::string("operator<<(unsigned long)"));
	return stm;
}

inline binstream& operator<<(binstream& stm, unsigned __int64 d) {
	if(!stm.write(&d, sizeof(unsigned __int64)))
		throw schema_exception(std::string("operator<<(unsigned __int64)"));
	return stm;
}

#ifdef __AFX_H__
inline binstream& operator<<(binstream& stm, const CString& string) {
	unsigned long len = string.GetLength() * sizeof(wchar_t);
	wchar_t* pwstrBuff = reinterpret_cast<wchar_t*>(_alloca(len));
	MultiByteToWideChar(CP_ACP,0,string,string.GetLength(),pwstrBuff,len);
	if (!stm.write(&len, sizeof(unsigned long)) || !stm.write(pwstrBuff, len))
	{
		throw schema_exception(std::string("operator<<(CString)"));
	}
	return stm;
}
#endif

inline binstream& operator<<(binstream& stm, const std::string& string) {
	unsigned long len = static_cast<unsigned long>(string.length()) * sizeof(wchar_t);
	wchar_t* pwstrBuff = reinterpret_cast<wchar_t*>(_alloca(len));
	MultiByteToWideChar(CP_ACP,0,string.c_str(),static_cast<int>(string.length()),pwstrBuff,len);
	if (!stm.write(&len, sizeof(unsigned long)) || !stm.write(pwstrBuff, len))
		throw schema_exception(std::string("operator<<(std::string)"));
	return stm;
}

inline binstream& operator<<(binstream& stm, const std::wstring& string) {
	unsigned long len = static_cast<unsigned long>(string.length()+1) * sizeof(wchar_t);
	if (!stm.write(&len, sizeof(unsigned long)) ||
		!stm.write(string.c_str(), len))
		throw schema_exception(std::string("operator<<(wstring)"));
	return stm;
}

inline binstream& operator<<(binstream& stm, const GUID& guid) {
	if (!stm.write(&guid, sizeof(GUID)))
		throw schema_exception(std::string("operator<<(GUID)"));
	return stm;
}

// extraction operations
inline binstream& operator>>(binstream& stm, long& l) {
	if(!stm.read(&l, sizeof(long)))
		throw schema_exception(std::string("operator>>(long)"));
	return stm;
}
inline binstream& operator>>(binstream& stm, float& f) {
	if(!stm.read(&f, sizeof(float)))
		throw schema_exception(std::string("operator>>(float)"));
	return stm;
}
inline binstream& operator>>(binstream& stm, double& d) {
	if(!stm.read(&d, sizeof(double)))
		throw schema_exception(std::string("operator>>(double)"));
	return stm;
}
inline binstream& operator>>(binstream& stm, __int64& d) {
	if(!stm.read(&d, sizeof(__int64)))
		throw schema_exception(std::string("operator>>(__int64)"));
	return stm;
}
inline binstream& operator>>(binstream& stm, bool& f) {
	unsigned char i;
	if(!stm.read(&i, sizeof(unsigned char)))
		throw schema_exception(std::string("operator>>(bool)"));
	f = (i == static_cast<unsigned char>(0)) ? false : true;
	return stm;
}
inline binstream& operator>>(binstream& stm, int& i) {
	long l;
	if(!stm.read(&l, sizeof(long)))
		throw schema_exception(std::string("operator>>(int)"));
	i = static_cast<int>(l);
	return stm;
}
inline binstream& operator>>(binstream& stm, short& w) {
	if(!stm.read(&w, sizeof(short)))
		throw schema_exception(std::string("operator>>(short)"));
	return stm;
}
inline binstream& operator>>(binstream& stm, char& ch) {
	if(!stm.read(&ch, sizeof(char)))
		throw schema_exception(std::string("operator>>(char)"));
	return stm;
}
inline binstream& operator>>(binstream& stm, unsigned char& by) {
	if(!stm.read(&by, sizeof(unsigned char)))
		throw schema_exception(std::string("operator>>(unsigned char)"));
	return stm;
}
inline binstream& operator>>(binstream& stm, unsigned short& w) {
	if(!stm.read(&w, sizeof(unsigned short)))
		throw schema_exception(std::string("operator>>(unsigned short)"));
	return stm;
}
inline binstream& operator>>(binstream& stm, unsigned int& w) {
	unsigned long l;
	if(!stm.read(&l, sizeof(unsigned long)))
		throw schema_exception(std::string("operator>>(unsigned int)"));
	w = static_cast<unsigned int>(l);
	return stm;
}
inline binstream& operator>>(binstream& stm, unsigned long& dw) {
	if(!stm.read(&dw, sizeof(unsigned long)))
		throw schema_exception(std::string("operator>>(unsigned long)"));
	return stm;
}
inline binstream& operator>>(binstream& stm, unsigned __int64& d) {
	if(!stm.read(&d, sizeof(unsigned __int64)))
		throw schema_exception(std::string("operator>>(unsigned __int64)"));
	return stm;
}

#ifdef __AFX_H__
inline binstream& operator>>(binstream& stm, CString& string) {
	unsigned long len=0;
	if (!stm.read(&len, sizeof(unsigned long)))
		throw schema_exception(std::string("operator>>(CString)"));
	if (len > 0)
	{
		unsigned char* pBuff = reinterpret_cast<unsigned char*>(_alloca(len));
		if (!stm.read(pBuff, len))
			throw schema_exception(std::string("operator>>(CString)"));
		string = CString(reinterpret_cast<LPWSTR>(pBuff),len/sizeof(wchar_t));
	}
	else 
		string.Empty();
	return stm;
}
#endif

inline binstream& operator>>(binstream& stm, std::string& string) {
	unsigned long len=0;
	if (!stm.read(&len, sizeof(unsigned long)))
		throw schema_exception(std::string("operator>>(string)"));
	if (len > 0)
	{
		unsigned char* pBuff = reinterpret_cast<unsigned char*>(_alloca(len));
		if (!stm.read(pBuff, len))
			throw schema_exception(std::string("operator>>(string)"));

		unsigned long lenA = (len/sizeof(wchar_t));
		char* pstrBuff = reinterpret_cast<char*>(_alloca(lenA));
		WideCharToMultiByte(CP_ACP,0,reinterpret_cast<wchar_t*>(pBuff),len,pstrBuff,lenA,NULL,NULL);
		string = std::string(pstrBuff,lenA);
	}
	else
		string.clear();
	return stm;
}

inline binstream& operator>>(binstream& stm, std::wstring& string) {
	unsigned long len=0;
	if (!stm.read(&len, sizeof(unsigned long)))
		throw schema_exception(std::string("operator>>(string)"));
	if (len > 0)
	{
		unsigned char* pBuff = reinterpret_cast<unsigned char*>(_alloca(len));
		if (!stm.read(pBuff, len))
			throw schema_exception(std::string("operator>>(wstring)"));
		string = std::wstring(reinterpret_cast<LPWSTR>(pBuff),len/sizeof(wchar_t));
	}
	else
		string.clear();
	return stm;
}

inline binstream& operator>>(binstream& stm, GUID& guid) {
	if (!stm.read(&guid, sizeof(GUID)))
		throw schema_exception(std::string("operator>>(GUID)"));
	return stm;
}

/******************************************************************************/
// binstream_input_iterator
//
// Iterator class for the binary stream type.  This allows the stream to be
// used in STL algorithms.
//
/******************************************************************************/
template <typename _Ty = unsigned char>
class binstream_input_iterator : std::iterator<std::input_iterator_tag, _Ty>
{
// Class data
public:
	typedef binstream_input_iterator<_Ty> _Myt;
private:
	binstream* m_stm;

// Constructor
public:
	binstream_input_iterator() : m_stm(0) {/* */}
	binstream_input_iterator(binstream& stm) : m_stm(&stm) { if (m_stm && m_stm->eof()) m_stm = 0; }

	binstream_input_iterator(const binstream_input_iterator& b) : m_stm(b.m_stm) {/* */}
	_Myt& operator=(const _Myt& b) {
		if (this != &b)
			m_stm = b.m_stm;
		return *this;
	}

// Accessors
public:
	// Dereferencing returns the value
	_Ty operator*() const { return const_cast<_Myt*>(this)->GetNextVal(); }

	// Pre/Post increment return the stream
	_Myt& operator++() { return (*this); }
	_Myt operator++(int) { return (*this); }

	// Equality operators
	inline bool operator==(_Myt& _Right) { return m_stm == _Right.m_stm; }
	inline bool operator!=(_Myt& _Right) { return !operator==(_Right); }

// Internal methods
private:
	_Ty GetNextVal() {
		_Ty _Val = 0;
		if (m_stm && !m_stm->eof())
			*m_stm >> _Val;
		else
			m_stm = 0;
		return _Val;
	}
};

/******************************************************************************/
// binstream_output_iterator
//
// Iterator class for the binary stream type.  This allows the stream to be
// used in STL algorithms.
//
/******************************************************************************/
template <typename _Ty = unsigned char, typename _Elem = char>
class binstream_output_iterator : std::iterator<std::output_iterator_tag, _Ty>
{
// Class data
public:
	typedef binstream_output_iterator<_Ty,_Elem> _Myt;
private:
	binstream& m_stm;
	const _Elem* m_Delim;

// Constructor
public:
	binstream_output_iterator(binstream& stm, const _Elem* _Delim = 0) : m_stm(stm), m_Delim(_Delim) {/* */}
	binstream_output_iterator(const binstream_output_iterator& b) : m_stm(b.m_stm), m_Delim(b.m_Delim) {/* */}
	_Myt& operator=(const _Myt& b) {
		if (this != &b)	{
			m_stm = b.m_stm;
			m_Delim = b.m_Delim;
		}
		return *this;
	}

// Accessors
public:
	// Assignment operator - inserts a new element
	_Myt& operator=(const _Ty& _Val) {
		m_stm << _Val;
		if (m_Delim != 0)
			m_stm << *m_Delim;
		return (*this);
	}

	// Dereferencing returns the stream itself
	_Myt& operator*() const { return const_cast<_Myt&>(*this); }

	// Pre/Post increment return the stream
	_Myt& operator++() { return (*this); }
	_Myt operator++(int) { return (*this); }
};

/******************************************************************************/
// VersionInfo
//
// Stores and manages schema versions within a object stream source
//
// INTERNAL DATA STRUCTURE
//
/******************************************************************************/
class VersionInfo
{
// Class data
private:
	static const DWORD VER_MARK = 0x52455600;
	int m_nVerRead;

// Constructor
public:
	VersionInfo(binstream& ar) : m_nVerRead(1) {
		// If the stream doesn't support backups then exit.
		if (ar.skip(0))
		{
			// Read the version marker and backup if it doesn't exist.
			DWORD dwID; ar >> dwID;
			if ((dwID & 0xffffff00) == VER_MARK)
				m_nVerRead = (int) (dwID & 0x000000ff);
			else
				ar.skip(-1 * static_cast<int>(sizeof(DWORD)));
		}
	}

	VersionInfo(binstream& ar, int nVer) : m_nVerRead(1) {
		// If the stream doesn't support backups then exit.
		if (ar.skip(0))
		{
			// Otherwise, store the version marker
			DWORD dwID = (VER_MARK | (DWORD)nVer);	
			ar << dwID;
		}
	}

// Properties
public:
	__declspec(property(get=get_Version)) int Version;

// Accessors
public:
	int get_Version() const { return m_nVerRead; }
};

} // namespace JTI_Util

#endif // _BINSTREAM_INC_
