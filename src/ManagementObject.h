/****************************************************************************/
//
// ManagementObject.h
//
// This class implements a wrapper around the Windows Management Interface
// COM objects.  It provides a semi .NET compatible object interface to WMI.
//
// Copyright (C) 2004 JulMar Technology, Inc.
// All rights reserved
// 
/****************************************************************************/

#ifndef __WINDOWS_MANAGEMENT_INTERFACE_OBJECT_H_INCL__
#define __WINDOWS_MANAGEMENT_INTERFACE_OBJECT_H_INCL__

/*----------------------------------------------------------------------------
//	INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <atlbase.h>
#include <atlcom.h>
#pragma warning (disable:4702)
#include <string>
#include <map>
#pragma warning (default:4702)
#include <JTIUtils.h>
#include <RefCount.h>
#include <Wbemidl.h>

// Force WMI library to be included
#pragma comment(lib,"wbemuuid.lib")

/*****************************************************************************/
// PC-Lint options
//
//lint -save
//
// Data not initialized in constructor
//lint -esym(1927, ManagementOptions::Timeout, ManagementOptions::Context)
//
// Data not initialized in constructor
//lint -esym(1401, ManagementOptions::Timeout, ManagementOptions::Context)
//
// Data not initialized by assignment operator
//lint -esym(1539, ManagementOptions::Timeout, ManagementOptions::Context)
//
// Public data member
//lint -esym(1925, ManagementOptions::Timeout, ManagementOptions::Context)
//
/*****************************************************************************/

namespace JTI_Util
{   
#define _DEFAULT_WMI_SCOPE_ _T("root\\cimv2")

/****************************************************************************/
// ManagementNameValueCollection
//
// This is a name/value collection of strings.
//
/****************************************************************************/
class ManagementNameValueCollection
{
// Typedefs
public:
	typedef std::map<tstring,tstring>::const_iterator const_iterator;
// Class data
private:
	std::map<tstring,tstring> entries_;

// Constructor
public:
	ManagementNameValueCollection() {/* */}
	ManagementNameValueCollection(const ManagementNameValueCollection& rhs) : entries_(rhs.entries_) {/* */}

// Operators
public:
	ManagementNameValueCollection& operator=(const ManagementNameValueCollection& rhs) {
		if (this != &rhs) {
			entries_ = rhs.entries_;
		}
		return *this;
	}
	tstring operator[](const TCHAR* index) const { return at(index); }

// Properties
public:
	__declspec(property(get=get_Count)) int Count;
	__declspec(property(get=get_IsEmpty)) bool IsEmpty;

// Methods
public:
	void Add(const tstring& Name, const tstring& Value) { entries_.insert(std::make_pair(Name, Value)); }
	void Clear() { entries_.clear(); }
	tstring at(const tstring& index) const {
		std::map<tstring,tstring>::const_iterator it = entries_.find(index);
		if (it != entries_.end())
			return it->second;
		return tstring();
	}
	const_iterator begin() const { return entries_.begin(); }
	const_iterator end() const { return entries_.end(); }
	const_iterator find(const TCHAR* index) const { return entries_.find(index); }

// Property helpers
public:
	int get_Count() const { return static_cast<int>(entries_.size()); }
	bool get_IsEmpty() const { return entries_.empty(); }
};

/****************************************************************************/
// ManagementOptions
//
// This class provides the base class for all options objects.
//
/****************************************************************************/
class ManagementOptions
{
// Class data
private:
	DWORD timeout_;
	ManagementNameValueCollection coll_;

// Constructor
public:
	ManagementOptions() : timeout_(INFINITE) {/* */}
	ManagementOptions(DWORD timeout) : timeout_(timeout) {/* */}

// Properties
public:
	__declspec(property(get=get_Timeout, put=set_Timeout)) DWORD Timeout;
	__declspec(property(get=get_Collection)) ManagementNameValueCollection Context;

// Property helpers
public:
	DWORD get_Timeout() const { return timeout_; }
	void set_Timeout(DWORD timeout) { timeout_ = timeout; }
	ManagementNameValueCollection get_Collection() { return coll_; }
};

/****************************************************************************/
// ManagementObjectImpl
//
// This class is the starting point for all WMI access.
//
/****************************************************************************/
class ManagementObjectImpl : public JTI_Util::RefCountedObject<JTI_Util::MultiThreadModel>
{
// Class data
private:
	tstring machine_;
	tstring scope_;
	tstring objectPath_;
	ManagementNameValueCollection coll_;
	ManagementNameValueCollection syscoll_;
	ManagementOptions options_;

// Constructor
public:
	ManagementObjectImpl() : scope_(_DEFAULT_WMI_SCOPE_) {/* */}
	ManagementObjectImpl(const TCHAR* wmiPath) : 
		scope_(_DEFAULT_WMI_SCOPE_), objectPath_((wmiPath)?wmiPath:_T("")) {/* */}
	ManagementObjectImpl(const TCHAR* scope, const TCHAR* wmiPath) :
		scope_((scope)?scope:_DEFAULT_WMI_SCOPE_), objectPath_((wmiPath)?wmiPath:_T("")) {/* */}
	ManagementObjectImpl(const TCHAR* machine, const TCHAR* scope, const TCHAR* wmiPath) :
		machine_((machine)?machine:_T("")), scope_((scope)?scope:_DEFAULT_WMI_SCOPE_), objectPath_((wmiPath)?wmiPath:_T("")) {/* */}

// Operators
private:
	ManagementObjectImpl(const ManagementObjectImpl& rhs);
	ManagementObjectImpl& operator=(const ManagementObjectImpl& rhs);

// Methods
public:
	bool Get() {
		// Clear our collection
		coll_.Clear();
		syscoll_.Clear();

		// Split out the path from the object
		tstring typeName; bool foundMatch=false;
		size_t nPos = objectPath_.find(_T("."));
		if (nPos != tstring::npos)
			typeName = objectPath_.substr(0,nPos);
		else 
			return false;

		// Load up our WMI locater.
		CComPtr<IWbemLocator> piLocator = 0;
		HRESULT hr = piLocator.CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER);
		if (FAILED(hr))
			return false;

		// Build the connection string; this is the machine name + scope (namespace).
		tstring connString;
		if (!machine_.empty()) {
			if (machine_[0] != _T('\\'))
				connString = _T("\\\\");
			connString += machine_;
			connString += _T("\\");
		}
		connString += scope_;

		// Connect to the appropriate server
		CComPtr<IWbemServices> piNamespace = 0;
		hr = piLocator->ConnectServer(CComBSTR(connString.c_str()), NULL, NULL, 0L, 0L, NULL, NULL, &piNamespace);
		if (FAILED(hr))
			return false;

		// Get an instance enum for this type.
		CComPtr<IEnumWbemClassObject> piEnum = 0;
		hr = piNamespace->CreateInstanceEnum(CComBSTR(typeName.c_str()), 
			WBEM_FLAG_SHALLOW | WBEM_FLAG_FORWARD_ONLY | WBEM_RETURN_IMMEDIATELY, NULL, &piEnum);
		if (FAILED(hr))
			return false;

		// Now walk all objects which match this class type.  There may be more
		// than one - Win32Disk for example has an entry for each logical drive.
		USES_CONVERSION;
		CComPtr<IWbemClassObject> piObject = 0; ULONG celt = 1;
		while (!foundMatch && (hr=piEnum->Next(options_.Timeout, 1, &piObject, &celt)) == S_OK && celt == 1)
		{
			// Search out all the properties to retrieve
			CComVariant val; CIMTYPE valType; CComBSTR bstrName;
			piObject->BeginEnumeration(0);
			while (piObject->Next(0, &bstrName, &val, &valType, NULL) == S_OK)
			{
				if (bstrName.Length() > 0)
				{
					tstring sName = W2T(bstrName), sValue;
					if (val.vt != VT_NULL && val.vt != VT_EMPTY)
					{
						if (val.vt != VT_BSTR && 
							(valType != CIM_OBJECT && valType != CIM_REFERENCE))
							val.ChangeType(VT_BSTR);
						if (val.vt == VT_BSTR)
							sValue = W2T(val.bstrVal);
					}

					if (!foundMatch)
					{
						if (!lstrcmpi(sName.c_str(),_T("__PATH")))
						{
							tstring sPath = sValue;
							nPos = sPath.find(scope_.c_str());
							if (nPos != tstring::npos)
								sPath = sPath.substr(nPos+scope_.length()+1);
							if (lstrcmpi(sPath.c_str(), objectPath_.c_str()) != 0)
							{
								coll_.Clear();
								syscoll_.Clear();
								break;
							}
							else foundMatch = true;
						}
					}

					if (sName.length() > 2 && sName.substr(0,2) == _T("__"))
						syscoll_.Add(sName, sValue);
					else
						coll_.Add(sName, sValue);
				}
				val.Clear();
			}
			piObject->EndEnumeration();
			piObject.Release();
		}
		return true;
	}

// Property helpers
public:
	tstring get_Machine() const { return machine_; }
	void set_Machine(const tstring& machine) { machine_ = machine; }
	tstring get_Path() const { return objectPath_; }
	void set_Path(const tstring& Path) { objectPath_ = Path; }
	tstring get_Scope() const { return scope_; }
	void set_Scope(const tstring& Scope) { scope_ = Scope; }
	ManagementOptions& get_Options() { return options_; }
	const ManagementNameValueCollection& get_Properties() { if (coll_.IsEmpty) Get(); return coll_; }
	const ManagementNameValueCollection& get_SysProperties() { if (syscoll_.IsEmpty) Get(); return syscoll_; }
};

/****************************************************************************/
// ManagementObject
//
// This class is the starting point for all WMI access.
//
/****************************************************************************/
class ManagementObject
{
// Class data
private:
	ManagementObjectImpl* pimpl_;

// Constructor
public:
	ManagementObject() : pimpl_(new ManagementObjectImpl()) {/* */}
	ManagementObject(const TCHAR* wmiPath) : pimpl_(new ManagementObjectImpl(wmiPath)) {/* */}
	ManagementObject(const TCHAR* scope, const TCHAR* wmiPath) : pimpl_(new ManagementObjectImpl(scope, wmiPath)) {/* */}
	ManagementObject(const TCHAR* machine, const TCHAR* scope, const TCHAR* wmiPath) : pimpl_(new ManagementObjectImpl(machine, scope, wmiPath)) {/* */}

// Copy operators and constructors
public:
	ManagementObject(const ManagementObject& rhs) {
		pimpl_ = rhs.pimpl_;
		pimpl_->AddRef();
	}
	ManagementObject& operator=(const ManagementObject& rhs) {
		if (&rhs != this) {
			pimpl_->Release();
			pimpl_ = rhs.pimpl_;
			pimpl_->AddRef();
		}
		return *this;
	}

// Operators
public:
	tstring operator[](const tstring& index) {
		if (!pimpl_->get_Properties().IsEmpty || pimpl_->Get()==true) {
			return pimpl_->get_Properties().at(index);
		}
		return tstring();
	}

// Properties
public:
	__declspec(property(get=get_Machine, put=set_Machine)) tstring Machine;
	__declspec(property(get=get_Path, put=set_Path)) tstring Path;
	__declspec(property(get=get_Scope, put=set_Scope)) tstring Scope;
	__declspec(property(get=get_Options)) ManagementOptions& Options;
	__declspec(property(get=get_Properties)) const ManagementNameValueCollection& Properties;
	__declspec(property(get=get_SysProperties)) const ManagementNameValueCollection& SystemProperties;

// Methods
public:
	bool Get() { return pimpl_->Get(); }

// Property helpers
public:
	tstring get_Machine() const { return pimpl_->get_Machine(); }
	void set_Machine(const tstring& machine) { pimpl_->set_Machine(machine); }
	tstring get_Path() const { return pimpl_->get_Path(); }
	void set_Path(const tstring& Path) { pimpl_->set_Path(Path); }
	tstring get_Scope() const { return pimpl_->get_Scope(); }
	void set_Scope(const tstring& Scope) { pimpl_->set_Scope(Scope); }
	ManagementOptions& get_Options() { return pimpl_->get_Options(); }
	const ManagementNameValueCollection& get_Properties() const { return pimpl_->get_Properties(); }
	const ManagementNameValueCollection& get_SysProperties() const { return pimpl_->get_SysProperties(); }
};

/****************************************************************************/
// ManagementObjectArray
//
// This class holds a collection of ManagementObjects
//
/****************************************************************************/
class ManagementObjectArray
{
// Class data
private:
	std::vector<ManagementObject> obs_;

// Constructor
public:
	ManagementObjectArray() {/* */}
	ManagementObjectArray(const ManagementObjectArray& rhs) : obs_(rhs.obs_) {/* */}

// Properties
public:
	__declspec(property(get=get_Count)) int Count;
	__declspec(property(get=get_Empty)) bool IsEmpty;

// Operators
public:
	ManagementObject operator[](int index) { return obs_[index]; }
	const ManagementObject operator[](int index) const { return obs_[index]; }
	ManagementObjectArray& operator=(const ManagementObjectArray& rhs) {
		if (this != &rhs) {
			obs_ = rhs.obs_;
		}
		return *this;
	}

// Methods
public:
	void Add(ManagementObject ob) { obs_.push_back(ob); }
	ManagementObject at(int index) { return obs_[index]; }
	void Clear() { obs_.clear(); }

// Property helpers
public:
	int get_Count() const { return static_cast<int>(obs_.size()); }
	bool get_Empty() const { return obs_.empty(); }
};

/****************************************************************************/
// ManagementObjectSearcher
//
// This class holds a collection of ManagementObjects
//
/****************************************************************************/
class ManagementObjectSearcher
{
// Class data
private:
	tstring machine_;
	tstring scope_;
	tstring query_;
	ManagementObjectArray coll_;

// Constructor
public:
	ManagementObjectSearcher(const TCHAR* query) : 
	  scope_(_DEFAULT_WMI_SCOPE_), query_((query)?query:_T("")) {/* */}
	  ManagementObjectSearcher(const TCHAR* scope, const TCHAR* query) :
	  scope_((scope)?scope:_DEFAULT_WMI_SCOPE_), query_((query)?query:_T("")) {/* */}
	  ManagementObjectSearcher(const TCHAR* machine, const TCHAR* scope, const TCHAR* query) :
	  machine_((machine)?machine:_T("")), scope_((scope)?scope:_DEFAULT_WMI_SCOPE_), query_((query)?query:_T("")) {/* */}

// Properties
public:
	__declspec(property(get=get_Array)) ManagementObjectArray Items;
	__declspec(property(get=get_Machine, put=set_Machine)) tstring Machine;
	__declspec(property(get=get_Query, put=set_Query)) tstring Query;
	__declspec(property(get=get_Scope, put=set_Scope)) tstring Scope;

// Methods
public:
	bool Get()
	{
		// Clear our collection
		coll_.Clear();

		// Determine if this is a query or a single object type.
		bool isQuery = false;
		if (query_.empty())
			return false;

		tstring query = query_; _tcsupr(&query[0]);
		if (query.find(_T("SELECT")) != tstring::npos)
			isQuery = true;

		// Load up our WMI locator.
		CComPtr<IWbemLocator> piLocator = 0;
		HRESULT hr = piLocator.CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER);
		if (FAILED(hr))
			return false;

		// Build the connection string; this is the machine name + scope (namespace).
		tstring connString;
		if (!machine_.empty()) {
			if (machine_[0] != _T('\\'))
				connString = _T("\\\\");
			connString += machine_;
			connString += _T("\\");
		}
		connString += scope_;

		// Connect to the appropriate server
		CComPtr<IWbemServices> piNamespace = 0;
		hr = piLocator->ConnectServer(CComBSTR(connString.c_str()), NULL, NULL, 0L, 0L, NULL, NULL, &piNamespace);
		if (FAILED(hr))
			return false;

		// Get an instance enum for this type.
		CComPtr<IEnumWbemClassObject> piEnum = 0;

		// If this is a query, then execute the query using WQL.
		if (isQuery)
		{
			hr = piNamespace->ExecQuery(CComBSTR(L"WQL"), CComBSTR(query_.c_str()), 
				WBEM_FLAG_FORWARD_ONLY | WBEM_RETURN_IMMEDIATELY, NULL, &piEnum);
		}
		else
		{
			hr = piNamespace->CreateInstanceEnum(CComBSTR(query_.c_str()), 
				WBEM_FLAG_SHALLOW | WBEM_FLAG_FORWARD_ONLY | WBEM_RETURN_IMMEDIATELY, NULL, &piEnum);
		}

		if (FAILED(hr))
			return false;

		// Now walk all objects which match this class type.  There may be more
		// than one - Win32Disk for example has an entry for each logical drive.
		USES_CONVERSION;
		CComPtr<IWbemClassObject> piObject = 0; ULONG celt = 1;
		while ((hr=piEnum->Next(INFINITE, 1, &piObject, &celt)) == S_OK && celt == 1)
		{
			// Search out all the properties to retrieve
			CComVariant val; CIMTYPE valType; CComBSTR bstrName;
			piObject->BeginEnumeration(WBEM_FLAG_SYSTEM_ONLY);
			while (piObject->Next(0, &bstrName, &val, &valType, NULL) == S_OK)
			{
				if (bstrName.Length() > 0)
				{
					tstring sName = W2T(bstrName), sValue;
					if (val.vt != VT_NULL && val.vt != VT_EMPTY)
					{
						if (val.vt != VT_BSTR && 
							(valType != CIM_OBJECT && valType != CIM_REFERENCE))
							val.ChangeType(VT_BSTR);
						if (val.vt == VT_BSTR)
							sValue = W2T(val.bstrVal);
					}

					// Found a object; create an instance
					if (!lstrcmpi(sName.c_str(),_T("__PATH")) && !sValue.empty())
					{
						tstring sPath = sValue;
						size_t nPos = sPath.find(scope_.c_str());
						if (nPos != tstring::npos)
							sPath = sPath.substr(nPos+scope_.length()+1);
						coll_.Add(ManagementObject(machine_.c_str(), scope_.c_str(), sPath.c_str()));
						break;
					}
				}
				val.Clear();
			}
			piObject->EndEnumeration();
			piObject.Release();
		}
		return true;
	}

// Property helpers
public:
	tstring get_Machine() const { return machine_; }
	void set_Machine(const tstring& machine) { machine_ = machine; }
	tstring get_Query() const { return query_; }
	void set_Query(const tstring& Path) { query_ = Path; }
	tstring get_Scope() const { return scope_; }
	void set_Scope(const tstring& Scope) { scope_ = Scope; }
	ManagementObjectArray get_Array() { 
		if (coll_.IsEmpty)
			Get();
		return coll_; 
	}
};
#undef _DEFAULT_WMI_SCOPE_

}// namespace JTI_Util

#endif // __WINDOWS_MANAGEMENT_INTERFACE_OBJECT_H_INCL__
