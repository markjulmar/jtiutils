/******************************************************************************/
//                                                                        
// Registry.h
//                                             
// This header implements a class for wrapping the Win32 registry API
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
//                                                                        
/******************************************************************************/

#ifndef __REGISTRY_H_INCL__
#define __REGISTRY_H_INCL__

/*****************************************************************************/
// PC-Lint options
//
//lint -save
//
// Supress duplicate header include files
//lint -e537
//
// Implicit constructor allowed
//lint -esym(1931, RegistryException*, RegistryKeyHolder*)
//
// Ignore public data
//lint -esym(1925, RegistryException::ErrorCode, RegistryException::ErrorText)
//
// Data not initialized in contructor list
//lint -esym(1927, RegistryException::ErrorCode, RegistryException::ErrorText)
//
// Data not initialized in constructor
//lint -esym(1401, RegistryException::ErrorCode, RegistryException::ErrorText)
//
// Default constructor invoked
//lint -esym(1926, RegistryException::ErrorText)
//
// Private constructor
//lint -esym(1704, RegistryKeyHolder*)
//
// Data not explicitly free'd in destructor
//lint -esym(1740, RegistryKeyHolder::regKey_)
//
// Function marked as const returns underlying class data
//lint -esym(1763, RegistryKeyHolder::get_HKEY)
// 
/*****************************************************************************/


/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <WinReg.h>
#include <vector>
#include <string>
#include <tchar.h>
#include <JTIUtils.h>
#include <RefCount.h>

namespace JTI_Util
{
/******************************************************************************/
// RegistryException
//
// This exception is thrown from the registry classes when any error occurs.
//
/******************************************************************************/
class RegistryException : public std::runtime_error
{
// Class data
private:
	DWORD errCode_;

// Constructor
public:
	RegistryException(const std::string& s, DWORD rc=0) : 
		  std::runtime_error(s), errCode_(rc) { if (!rc) rc = GetLastError(); }

// Properties
public:
	__declspec(property(get=get_LastError)) DWORD ErrorCode;
	__declspec(property(get=get_ErrorTxt)) tstring ErrorText;

// Helpers
public:
	DWORD get_LastError() const { return errCode_; }
	tstring get_ErrorTxt() const
	{
		if (errCode_ > 0)
		{
			LPTSTR pMsgBuff;
			if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, errCode_, MAKELANGID(LANG_NEUTRAL, SUBLANG_ENGLISH_US),
				(LPTSTR)&pMsgBuff, 0, NULL))
			{
				tstring sMsg = pMsgBuff;
				LocalFree(pMsgBuff);
				return sMsg;
			}
		}
		return _T("");
	}
};

/******************************************************************************/
// RegistryKeyHolder
//
// This class is used to hold the HKEY element and the name of the element
// so that it can be reference counted.
//
/******************************************************************************/
class RegistryKeyHolder
{
// Class data
private:
	HKEY regKey_;
	tstring name_;

// Constructor
public:
	RegistryKeyHolder(HKEY hkey, const TCHAR* pszName=NULL) : regKey_(hkey), name_(intgetName(hkey,pszName)) {}
	~RegistryKeyHolder() { close_HKEY(); }

// Accessors
public:
	const tstring& get_Name() const { return name_; }
	HKEY get_HKEY() const { return regKey_; }
	LONG close_HKEY() throw()
	{ 
		if (regKey_ != 0) 
		{
			LONG rc = RegCloseKey(regKey_);
			if (rc == ERROR_SUCCESS)
				regKey_ = 0; 
			return rc;
		}
		return ERROR_SUCCESS;
	}

// Internal methods
private:
	static const TCHAR* intgetName(HKEY hkey, const TCHAR* pszName)
	{
		if (pszName == NULL)
		{
			switch((DWORD_PTR)hkey)
			{
				case HKEY_CLASSES_ROOT:
					pszName = _T("HKEY_CLASSES_ROOT");
					break;
				case HKEY_CURRENT_CONFIG:
					pszName = _T("HKEY_CURRENT_CONFIG");
					break;
				case HKEY_CURRENT_USER:
					pszName = _T("HKEY_CURRENT_USER");
					break;
				case HKEY_DYN_DATA:
					pszName = _T("HKEY_DYN_DATA");
					break;
				case HKEY_LOCAL_MACHINE:
					pszName = _T("HKEY_LOCAL_MACHINE");
					break;
				case HKEY_PERFORMANCE_DATA:
					pszName = _T("HKEY_PERFORMANCE_DATA");
					break;
				case HKEY_USERS:
					pszName = _T("HKEY_USERS");
					break;
				default: 
					pszName = _T("UNKNOWN");
					break;
			}
		}
		return pszName;
	}


// Unavailable methods
private:
	RegistryKeyHolder(const RegistryKeyHolder&);
	RegistryKeyHolder& operator=(const RegistryKeyHolder&);
};

/******************************************************************************/
// RegistryHive
//
// This enumeration lists the valid Registry Hives which can be accessed
// by the registry classes.
//
/******************************************************************************/
namespace RegistryHive
{
	enum Types {
		ClassesRoot		= (DWORD_PTR) HKEY_CLASSES_ROOT,
		CurrentConfig	= (DWORD_PTR) HKEY_CURRENT_CONFIG,
		CurrentUser		= (DWORD_PTR) HKEY_CURRENT_USER,
		DynData			= (DWORD_PTR) HKEY_DYN_DATA,
		LocalMachine	= (DWORD_PTR) HKEY_LOCAL_MACHINE,
		PerformanceData = (DWORD_PTR) HKEY_PERFORMANCE_DATA,
		Users			= (DWORD_PTR) HKEY_USERS
	};
};

/******************************************************************************/
// RegistryValueTypes
//
// This enumeration lists the valid value types for the registry.
//
/******************************************************************************/
namespace RegistryValueTypes
{
	enum Types {
		RVType_None			= REG_NONE,
		RVType_String		= REG_SZ,
		RVType_ExpandString	= REG_EXPAND_SZ,
		RVType_Binary		= REG_BINARY,
		RVType_DWord		= REG_DWORD,
		RVType_DWordReverse	= REG_DWORD_BIG_ENDIAN,
		RVType_Link			= REG_LINK,
		RVType_StringArray	= REG_MULTI_SZ,
		RVType_QWord		= REG_QWORD
	};
};

/******************************************************************************/
// RegistryValue
//
// This class wraps a single value from the registry including it's type
// information and data.
//
/******************************************************************************/
class RegistryValue
{
// Class data
private:
	tstring name_;
	RegistryValueTypes::Types type_;
	std::vector<BYTE> data_;

// Constructor
public:
	RegistryValue(const TCHAR* pszName, const unsigned char* pBuffer, DWORD dwSize, 
		RegistryValueTypes::Types rType = RegistryValueTypes::RVType_Binary) :	name_(pszName), type_(rType)
	{
		// Copy the buffer over
		if (dwSize > 0 && pBuffer != NULL) {
			data_.reserve(dwSize);
			std::copy(pBuffer,pBuffer+dwSize, std::back_inserter(data_));
		}
	}

	RegistryValue(const TCHAR* pszName, const TCHAR* pString, 
		RegistryValueTypes::Types rType = RegistryValueTypes::RVType_String) : name_(pszName), type_(rType)
	{
		if (type_ != RegistryValueTypes::RVType_ExpandString && type_ != RegistryValueTypes::RVType_String)
			throw RegistryException("Invalid registry value type encountered.");

		// Copy the string over
		if (pString) {
			int len = (lstrlen(pString)+1) * sizeof(TCHAR);
			data_.reserve(len);
			unsigned char* lp = (unsigned char*)pString;
			std::copy(lp,lp+len, std::back_inserter(data_));
		}
	}

	RegistryValue(const TCHAR* pszName, DWORD dwValue, RegistryValueTypes::Types rType = RegistryValueTypes::RVType_DWord) :
		name_(pszName), type_(rType)
	{
		if (type_ != RegistryValueTypes::RVType_DWord && type_ != RegistryValueTypes::RVType_DWordReverse)
			throw RegistryException("Invalid registry value type encountered.");

		// Copy the DWORD over
		data_.reserve(sizeof(DWORD));
		unsigned char* lp = (unsigned char*)&dwValue;
		std::copy(lp,lp+sizeof(DWORD), std::back_inserter(data_));
	}

	RegistryValue(const TCHAR* pszName, __int64 nValue) :
		name_(pszName), type_(RegistryValueTypes::RVType_QWord)
	{
		// Copy the DWORD over
		data_.reserve(sizeof(__int64));
		unsigned char* lp = (unsigned char*)&nValue;
		std::copy(lp,lp+sizeof(__int64), std::back_inserter(data_));
	}

	RegistryValue(const RegistryValue& rhs) :
		name_(rhs.name_), type_(rhs.type_), data_(rhs.data_) {/* */}

	~RegistryValue() {/* */}

	RegistryValue &operator=(const RegistryValue &rhs)
	{
		if (this != &rhs) {
			name_ = rhs.name_;
			type_ = rhs.type_;
			data_ = rhs.data_;
		}
		return *this;
	}

// Properties
public:
	__declspec(property(get=get_Name)) const tstring& Name;
	__declspec(property(get=get_Type)) RegistryValueTypes::Types Type;
	__declspec(property(get=get_Data)) const std::vector<BYTE>& Value;

// Property helpers
public:
	const tstring get_Name() const { return name_; }
	const std::vector<BYTE>& get_Data() const { return data_; }
	RegistryValueTypes::Types get_Type() const { return type_; }

// Methods
public:
	tstring ToString() const {
		if (type_ == RegistryValueTypes::RVType_String || type_ == RegistryValueTypes::RVType_ExpandString)
			return tstring((const TCHAR*)&data_[0]);
		else if (type_ == RegistryValueTypes::RVType_DWord)
		{
			TCHAR chBuff[30]; _ltot(ToDWord(), chBuff, 10);
			return tstring(chBuff);
		}
		else if (type_ == RegistryValueTypes::RVType_QWord)
		{
			TCHAR chBuff[30]; _i64tot(ToQWord(), chBuff, 10);
			return tstring(chBuff);
		}
		else return (_T("Blob"));
	}

	DWORD ToDWord() const
	{
		if (type_ != RegistryValueTypes::RVType_DWord && type_ != RegistryValueTypes::RVType_DWordReverse)
			return 0;
		return *((LPDWORD)&data_[0]);
	}

	__int64 ToQWord() const
	{
		if (type_ != RegistryValueTypes::RVType_QWord)
			return 0;
		return *((__int64*)&data_[0]);
	}
};

/******************************************************************************/
// RegistryKeyCollection
//
// This class implements a collection of key names.
//
/******************************************************************************/
class RegistryKeyCollection
{
// Exposed typedefs
public:
	typedef std::vector<tstring>::const_iterator const_iterator;
// Class data
private:
	std::vector<tstring> keyNames_;

// Constructor
private:
	friend class RegistryKey;
	RegistryKeyCollection(HKEY hkey)
	{
		DWORD dwNumValues = 0;
		LONG rc = ::RegQueryInfoKey(hkey, NULL, NULL, NULL,
			&dwNumValues, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		if (rc == ERROR_SUCCESS)
		{
			if (dwNumValues > 0)
			{
				TCHAR chName[MAX_PATH];	
				DWORD dwName; 
				FILETIME ft;

				keyNames_.reserve(dwNumValues);
				for (DWORD dwIndex = 0; rc != ERROR_NO_MORE_ITEMS; ++dwIndex)
				{
					dwName = sizeof(chName);
					rc = RegEnumKeyEx(hkey, dwIndex, chName, &dwName, NULL, NULL, NULL, &ft);
					if (rc != ERROR_SUCCESS && rc != ERROR_MORE_DATA)
						break;

					// Add the item to the list.
					keyNames_.push_back(chName);
				}
			}
		}
	}
public:
	RegistryKeyCollection(const RegistryKeyCollection& rhs) : keyNames_(rhs.keyNames_) {/* */}

// Operators
public:
	RegistryKeyCollection& operator=(const RegistryKeyCollection& rhs)
	{
		if (this != &rhs) {
			keyNames_ = rhs.keyNames_;
		}
		return *this;
	}

	const tstring& operator[](int nIndex) const
	{
		if (nIndex < 0 || nIndex >= static_cast<int>(keyNames_.size()))
			throw RegistryException("Collection accessed beyond range", ERROR_INVALID_PARAMETER);
		return keyNames_.at(nIndex);
	}

// Properties
public:
	__declspec(property(get=get_Count)) int Count;

// Property helpers
public:
	int get_Count() const { return static_cast<int>(keyNames_.size()); }
	const tstring& GetItem(int nIndex) const
	{
		if (nIndex < 0 || nIndex >= static_cast<int>(keyNames_.size()))
			throw RegistryException("Collection accessed beyond range", ERROR_INVALID_PARAMETER);
		return keyNames_.at(nIndex);
	}

// Iterator access
public:
	const_iterator begin() const { return keyNames_.begin(); }
	const_iterator end() const { return keyNames_.end(); }
};

/******************************************************************************/
// RegistryValueCollection
//
// This class implements a collection of value names.
//
/******************************************************************************/
class RegistryValueCollection
{
// Exposed typedefs
public:
	typedef std::vector<tstring>::const_iterator const_iterator;
// Class data
private:
	std::vector<tstring> valueNames_;

// Constructor
private:
	friend class RegistryKey;
	RegistryValueCollection(HKEY hkey)
	{
		DWORD dwNumValues = 0;
		LONG rc = RegQueryInfoKey(hkey, NULL, NULL, NULL,
			NULL, NULL, NULL, &dwNumValues, NULL, NULL, NULL, NULL);
		if (rc == ERROR_SUCCESS)
		{
			if (dwNumValues > 0)
			{
				TCHAR chName[MAX_PATH];	
				DWORD dwName;

				valueNames_.reserve(dwNumValues);
				for (DWORD dwIndex = 0; rc != ERROR_NO_MORE_ITEMS; ++dwIndex)
				{
					dwName = sizeof(chName);
					rc = RegEnumValue(hkey, dwIndex, chName, &dwName, NULL, NULL, NULL, NULL);
					if (rc != ERROR_SUCCESS && rc != ERROR_MORE_DATA)
						break;

					// Add the item to the list.
					valueNames_.push_back(chName);
				}
			}
		}
	}
public:
	RegistryValueCollection(const RegistryValueCollection& rhs) : valueNames_(rhs.valueNames_) {/* */}

// Operators
public:
	RegistryValueCollection& operator=(const RegistryValueCollection& rhs)
	{
		if (this != &rhs) {
			valueNames_ = rhs.valueNames_;
		}
		return *this;
	}

	const tstring& operator[](int nIndex) const
	{
		if (nIndex < 0 || nIndex >= static_cast<int>(valueNames_.size()))
			throw RegistryException("Collection accessed beyond range", ERROR_INVALID_PARAMETER);
		return valueNames_.at(nIndex);
	}

// Properties
public:
	__declspec(property(get=get_Count)) int Count;

// Property helpers
public:
	int get_Count() const { return static_cast<int>(valueNames_.size()); }
	const tstring& GetItem(int nIndex) const
	{
		if (nIndex < 0 || nIndex >= static_cast<int>(valueNames_.size()))
			throw RegistryException("Collection accessed beyond range", ERROR_INVALID_PARAMETER);
		return valueNames_.at(nIndex);
	}

// Iterator access
public:
	const_iterator begin() const { return valueNames_.begin(); }
	const_iterator end() const { return valueNames_.end(); }
};

/******************************************************************************/
// RegistryKey
//
// This class is the primary accessing class for a single key and it's
// sub-elements.
//
/******************************************************************************/
class RegistryKey : private UsageCountedObject<RegistryKeyHolder, MultiThreadModel>
{
// Typedefs
private:
	typedef UsageCountedObject<RegistryKeyHolder, MultiThreadModel> _BaseClass;

// Constructor/Destructor
private:
	friend class Registry;
	friend class RegistryKeyCollection;
	friend class RegistryValueCollection;

	explicit RegistryKey(HKEY hKey, const TCHAR* pszName=NULL) : 
		_BaseClass(JTI_NEW RegistryKeyHolder(hKey,pszName)) {/* */}

	RegistryKey(LPTSTR pRemoteMachine, HKEY hKey)
	{
		HKEY theKey = hKey;
		LONG rc = ::RegConnectRegistry(pRemoteMachine, hKey, &theKey);
		if (rc != ERROR_SUCCESS)
			throw RegistryException("RegConnectRegistry failed");
		Assign(JTI_NEW RegistryKeyHolder(theKey,NULL));
	}
	RegistryKey(const RegistryKeyHolder& rkh, const TCHAR* pSubKey, REGSAM samDesired = KEY_ALL_ACCESS, LPTSTR pRemoteMachine = 0)
	{
		HKEY theKey = rkh.get_HKEY();
		if (pRemoteMachine)
		{
			LONG rc = ::RegConnectRegistry(pRemoteMachine, rkh.get_HKEY(), &theKey);
			if (rc != ERROR_SUCCESS)
				throw RegistryException("RegConnectRegistry failed.", rc);
		}
		HKEY newKey;
		LONG rc = ::RegOpenKeyEx(theKey, pSubKey, 0, samDesired, &newKey);
		if (rc != ERROR_SUCCESS)
			throw RegistryException("RegOpenKeyEx failed.", rc);

		// Build the full path name
		tstring strName = rkh.get_Name();
		strName += _T("\\");
		strName += pSubKey;

		Assign(JTI_NEW RegistryKeyHolder(newKey, strName.c_str()));
	}
public:
	RegistryKey(const RegistryKey &rhs) : _BaseClass(rhs) {/* */}
	~RegistryKey() {/* */}

// Operators
public:
	RegistryKey& operator=(const RegistryKey& rhs) {
		if (this != &rhs)
			_BaseClass::Assign(rhs);
		return *this;
	}
	RegistryKey& operator=(HKEY hKey) {
		if (hKey != ptr()->get_HKEY())
			_BaseClass::Assign(JTI_NEW RegistryKeyHolder(hKey));
		return *this;
	}

// Properties
public:
	__declspec(property(get=get_Name)) tstring Name;
	__declspec(property(get=get_SubKeyCount)) int SubKeyCount;
	__declspec(property(get=get_ValueCount)) int ValueCount;
	__declspec(property(get=get_LastWriteTime)) FILETIME LastWriteTime;	

// Methods
public:
	// Connect to a remote registry
	static RegistryKey OpenRemoteBaseKey(RegistryHive::Types hive, const TCHAR* pMachineName)
	{
		HKEY hKey;
		LONG rc = ::RegConnectRegistry(pMachineName, (HKEY)hive, &hKey);
		if (rc != ERROR_SUCCESS)
			throw RegistryException("RegConnectRegistry failed.", rc);
		return RegistryKey(hKey);
	}

	// Open a specific sub-key of this key.
	RegistryKey OpenSubKey(const TCHAR* pSubKey, REGSAM samDesired = KEY_ALL_ACCESS) const
	{
		return RegistryKey(*ptr(), pSubKey, samDesired);
	}

	RegistryKey CreateSubKey(const TCHAR* pSubKey, LPDWORD pdwDisposition=NULL, LPTSTR pClass = _T(""),
		DWORD dwOptions = REG_OPTION_NON_VOLATILE, REGSAM samDesired = KEY_ALL_ACCESS,
		LPSECURITY_ATTRIBUTES pSecurityAttributes = NULL) const
	{
		HKEY hKey;
		DWORD dwDisposition;
		if (!pdwDisposition)
			pdwDisposition = &dwDisposition;
		LONG rc = RegCreateKeyEx(get_HKEY(), pSubKey, 0, pClass, dwOptions, samDesired, pSecurityAttributes, &hKey,	pdwDisposition);
		if (rc != ERROR_SUCCESS)
			throw RegistryException("RegCreateKeyEx failed.", rc);
		return RegistryKey(hKey);
	}

	void SetSecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd)
	{
		LONG rc = RegSetKeySecurity(get_HKEY(), si, psd);
		if (rc != ERROR_SUCCESS)
			throw RegistryException("RegSetKeySecurity failed.", rc);
	}

	void DeleteSubKey(const TCHAR* pKeyName, bool fThrowOnMissingSubKey = false) const
	{
		LONG rc = ::RegDeleteKey(get_HKEY(), pKeyName);
		if (rc != ERROR_SUCCESS && fThrowOnMissingSubKey)
			throw RegistryException("RegDeleteKey failed.", rc);
	}

	void DeleteSubKeyTreee(const TCHAR* pKeyName) const
	{
		IntDeleteSubKeyTree(get_HKEY(), pKeyName);
	}

	bool SubKeyExists(const TCHAR* pSubKey, REGSAM samDesired = KEY_ALL_ACCESS) const
	{
		bool hasKey = false;
		HKEY hKey;
		LONG rc = ::RegOpenKeyEx(get_HKEY(), pSubKey, 0, samDesired, &hKey);
		if (rc == ERROR_SUCCESS)
		{
			hasKey = true;
			::RegCloseKey(hKey);
		}
		return hasKey;
	}

	void Flush() const
	{
		LONG rc = ::RegFlushKey(get_HKEY());
		if (rc != ERROR_SUCCESS)
			throw RegistryException("RegFlushKey failed.", rc);
	}

	void DeleteValue(const TCHAR* pValueName) const
	{
		LONG rc = ::RegDeleteValue(get_HKEY(), pValueName);
		if (rc != ERROR_SUCCESS)
			throw RegistryException("RegDeleteValue failed.", rc);
	}

	void Close() 
	{
		LONG rc = ptr()->close_HKEY();
		if (rc != ERROR_SUCCESS)
			throw RegistryException("RegCloseKey failed.", rc);
	}

	RegistryValueCollection GetValueNames() const
	{
		return RegistryValueCollection(get_HKEY());
	}

	RegistryKeyCollection GetSubKeyNames() const
	{
		return RegistryKeyCollection(get_HKEY());
	}

	// Returns the value for a key; throws an exception if not found.
	RegistryValue GetValue(const TCHAR* pValueName = 0) const
	{
		DWORD dwType, dwSize = 0;
		LONG rc = RegQueryValueEx(get_HKEY(), pValueName, 0, &dwType, NULL, &dwSize);
		if (rc != ERROR_SUCCESS)
			throw RegistryException("RegQueryValueEx failed.", rc);
		std::vector<BYTE> arrBuffer(dwSize);
		// Re-query with the new buffer.
		rc = RegQueryValueEx(get_HKEY(), pValueName, 0, &dwType, (unsigned char*)&arrBuffer[0], &dwSize);
		if (rc != ERROR_SUCCESS)
			throw RegistryException("RegQueryValueEx failed.", rc);
		return RegistryValue(pValueName, (unsigned char*)&arrBuffer[0], dwSize, (RegistryValueTypes::Types)dwType);
	}

	// Returns the value for a key or a default if the key is not found.
	RegistryValue GetValue(const TCHAR* pValueName, RegistryValue valDefault) const
	{
		DWORD dwType, dwSize = 0;
		LONG rc = RegQueryValueEx(get_HKEY(), pValueName, 0, &dwType, NULL, &dwSize);
		if (rc != ERROR_SUCCESS)
			return valDefault;

		std::vector<BYTE> arrBuffer(dwSize);
		// Re-query with the new buffer.
		rc = RegQueryValueEx(get_HKEY(), pValueName, 0, &dwType, (unsigned char*)&arrBuffer[0], &dwSize);
		if (rc != ERROR_SUCCESS)
			return valDefault;

		return RegistryValue(pValueName, (unsigned char*)&arrBuffer[0], dwSize, (RegistryValueTypes::Types)dwType);
	}

	void SetValue(const TCHAR* pValueName, RegistryValue val) const
	{
		const std::vector<BYTE>& vec = val.get_Data();
		LONG rc = ::RegSetValueEx(get_HKEY(), pValueName, 0, 
			val.Type, &vec[0], static_cast<DWORD>(vec.size()));
		if (rc != ERROR_SUCCESS)
			throw RegistryException("RegSetValueEx failed.", rc);
	}

// Property helpers
public:
	tstring get_Name() const
	{
		return ptr()->get_Name();
	}

	FILETIME get_LastWriteTime() const
	{
		FILETIME ft;
		LONG rc = ::RegQueryInfoKey(get_HKEY(), NULL, NULL, NULL, NULL, NULL, NULL,
					NULL, NULL, NULL, NULL, &ft);
		if (rc != ERROR_SUCCESS)
			throw RegistryException("RegQueryInfoKey failed.", rc);
		return ft;
	}

	int get_SubKeyCount() const
	{
		DWORD dwKeys;
		LONG rc = ::RegQueryInfoKey(get_HKEY(), NULL, NULL, NULL, &dwKeys, NULL, NULL,
					NULL, NULL, NULL, NULL, NULL);
		if (rc != ERROR_SUCCESS)
			throw RegistryException("RegQueryInfoKey failed.", rc);
		return dwKeys;
	}

	int get_ValueCount() const
	{
		DWORD dwValues;
		LONG rc = ::RegQueryInfoKey(get_HKEY(), NULL, NULL, NULL, NULL, NULL, NULL,
					&dwValues, NULL, NULL, NULL, NULL);
		if (rc != ERROR_SUCCESS)
			throw RegistryException("RegQueryInfoKey failed.", rc);
		return dwValues;
	}

// Internal functions
private:
	HKEY get_HKEY() const { return ptr()->get_HKEY(); }

	// Deletes a tree
	void IntDeleteSubKeyTree(HKEY hKeyMain, const TCHAR* pKeyName) const
	{
		// Attempt to delete the key directly. Under Win95, this will also delete any branches under it.
		if (RegDeleteKey(hKeyMain, pKeyName) != ERROR_SUCCESS)
		{
			// Open the top-level key.
			HKEY hKey;
			LONG rc = RegOpenKeyEx(hKeyMain, pKeyName, 0, KEY_ENUMERATE_SUB_KEYS | DELETE, &hKey);
			if (rc == ERROR_SUCCESS)
			{
				TCHAR chBuff[_MAX_PATH]; DWORD dwReqSize;
				while (rc == ERROR_SUCCESS)
				{
					dwReqSize = _MAX_PATH;
					rc = RegEnumKeyEx(hKey, 0, chBuff, &dwReqSize, NULL, NULL, NULL, NULL);
					if (rc == ERROR_NO_MORE_ITEMS)
					{
						rc = RegDeleteKey(hKey, pKeyName);
						if (rc != ERROR_SUCCESS)
							throw RegistryException("RegDeleteKey failed.", rc);
						break;
					}
					else if (rc == ERROR_SUCCESS)
						IntDeleteSubKeyTree(hKey, chBuff);
					else
						throw RegistryException("RegEnumKeyEx failed.", rc);
				}
				RegCloseKey(hKey);
			}
			else
				throw RegistryException("RegOpenKeyEx failed.", rc);
		}
	}
};

/******************************************************************************/
// Registry
//
// This class holds a series of public static properties to access the
// root hives of each registry tree.
//
/******************************************************************************/
class Registry
{
// Class members
public:
	static RegistryKey ClassesRoot() { return RegistryKey(HKEY_CLASSES_ROOT); }
	static RegistryKey CurrentConfig() { return RegistryKey(HKEY_CURRENT_CONFIG); }
	static RegistryKey CurrentUser() { return RegistryKey(HKEY_CURRENT_USER); }
	static RegistryKey DynData() { return RegistryKey(HKEY_DYN_DATA); }
	static RegistryKey LocalMachine() {	return RegistryKey(HKEY_LOCAL_MACHINE); }
	static RegistryKey PerformanceData() { return RegistryKey(HKEY_PERFORMANCE_DATA); }
	static RegistryKey Users() { return RegistryKey(HKEY_USERS); }
};

}// namespace JTI_Util

#endif // __REGISTRY_H_INCL__