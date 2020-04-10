/****************************************************************************/
//
// comutls.h
//
// This header describes basic COM handling functios common throughout
// the whole project.
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// Portions copyright (C) Chris Sells
// 
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
//
// Revision History
// --------------------------------------------------------------
// 04/23/1999  MCS   Initial revision for Graphical Navigator (Mobil)
// 03/12/2000  MCS   Initial revision for SimulRing project
// 04/20/2001  MCS   Updated COM_CATCH macros for new auditor.
// 09/05/2001  MCS   Rewrote catch blocks to remove usage of _alloca for VC7.
// 02/11/2003  MCS	 Updated COM_CATCH macros for method name
//
/****************************************************************************/

#ifndef __COMUTLS_H_INC__
#define __COMUTLS_H_INC__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/*----------------------------------------------------------------------------
//  INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <comdef.h>
#include <crtdbg.h>
#include <string>
#include <stdexcept>
#include <sstream>
#include <atlbase.h>
#include <JTIUtils.h>
#include <stlx.h>

namespace comutils {
/*****************************************************************************
** Procedure:  ComInitialize
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: This structure should be globally instantiated to setup COM
**
/****************************************************************************/
struct ComInitialize
{
	ComInitialize(DWORD type, bool initSecurity=false) 
	{ 
		CoInitializeEx(NULL, type);
		// Initialize security - this is required for WMI usage.
		if (initSecurity)
			CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT,
				RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, 0);
	}
	~ComInitialize() { CoUninitialize(); }
};
}// namespace comutils

/*****************************************************************************
** Procedure:  COINITPARAM
** 
** Arguments:  'px' - POINTER argument to initialize
** 
** Returns: void
** 
** Description: This macro initializes a pointer variable to NULL
**
/****************************************************************************/
#define COINITPARAM(px) if(px==NULL) return E_POINTER;*px=0

/*****************************************************************************
** Procedure:  COINITPARAMV
** 
** Arguments:  'pv' - VARIANT argument to initialize
** 
** Returns: void
** 
** Description: This macro initializes a variant to null.
**
/****************************************************************************/
#define COINITPARAMV(pv) if (pv==NULL) return E_POINTER; ::VariantClear(pv)

/*****************************************************************************
** Procedure:  COINITBOOL
** 
** Arguments:  'pb' - VARIANT_BOOL argument to initialize
** 
** Returns: void
** 
** Description: This macro initializes a VARIANT_BOOL variable to FALSE
**
/****************************************************************************/
#define COINITBOOL(pb) if (pb==NULL) return E_POINTER; *pb = VARIANT_FALSE

/*****************************************************************************
** Procedure:  COM_TRY and COM_CATCH
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: These macros are used to catch _com_ptr based exceptions
**              in an implementation file.
**
/****************************************************************************/
#if !defined(COM_TRY) && !defined(_NO_INC_COMTRY_)
#define COM_TRY try

namespace CT_HELPERS {
#ifdef __autosvcs_h__
	inline void CT_ABORT_TRANSACTION()
	{
		CComPtr<IObjectContext> myctx; 
		if (SUCCEEDED(::CoGetObjectContext(__uuidof(myctx),
			reinterpret_cast<void**>(&myctx))) && myctx != NULL)
			myctx->SetAbort();
	}
#else
	inline void CT_ABORT_TRANSACTION() {}
#endif
};

#define COM_CATCH_IGNORE catch(...) {}

#if defined(_DEBUG) || defined(DEBUG)

#define COM_CATCH(s) \
	catch(const _com_error& err) { \
		CT_HELPERS::CT_ABORT_TRANSACTION(); \
		return ErrorInfo(err, __FILE__, __LINE__); \
	} \
	catch(const std::runtime_error& err) { \
		CT_HELPERS::CT_ABORT_TRANSACTION(); \
		return ErrorInfo(E_INVALIDARG, __FILE__, __LINE__, err.what()); \
	} \
	catch (...) { \
		CT_HELPERS::CT_ABORT_TRANSACTION(); \
		return ErrorInfo(E_UNEXPECTED, __FILE__, __LINE__); \
	}

#define COM_CATCH_NP(s) \
	catch(const _com_error& err) { \
		CT_HELPERS::CT_ABORT_TRANSACTION(); \
		comutils::_dump_com_error(err, __FILE__, __LINE__); \
	} \
	catch(const std::runtime_error& err) { \
		CT_HELPERS::CT_ABORT_TRANSACTION(); \
		comutils::_dump_com_error(E_INVALIDARG, err.what(), NULL, "Runtime error", __FILE__, __LINE__); \
	} \
	catch (...) { \
		CT_HELPERS::CT_ABORT_TRANSACTION(); \
		comutils::_dump_com_error(E_UNEXPECTED, NULL, NULL, "C++ exception", __FILE__, __LINE__); \
	}

#else // _DEBUG

#define COM_CATCH(s) \
	catch(const std::runtime_error& err) { \
		CT_HELPERS::CT_ABORT_TRANSACTION(); \
		return ErrorInfo(E_INVALIDARG, err.what()); \
	} \
	catch(const _com_error& err) { \
		CT_HELPERS::CT_ABORT_TRANSACTION(); \
		return ErrorInfo(err); \
	} \
	catch (...) { \
		CT_HELPERS::CT_ABORT_TRANSACTION(); \
		return ErrorInfo(E_UNEXPECTED); \
	}

#define COM_CATCH_NP(s) \
	catch(const std::runtime_error& err) { \
		CT_HELPERS::CT_ABORT_TRANSACTION(); \
		comutils::_dump_com_error(E_UNEXPECTED, NULL, NULL, err.what()); \
	} \
	catch(const _com_error& err) { \
		CT_HELPERS::CT_ABORT_TRANSACTION(); \
		comutils::_dump_com_error(err); \
	} \
	catch (...) { \
		CT_HELPERS::CT_ABORT_TRANSACTION(); \
		comutils::_dump_com_error(E_UNEXPECTED, NULL, NULL, "C++ exception"); \
	}

#endif // _DEBUG
#endif // COM_TRY

// Work around bug in ATL3's OBJECT_ENTRY_NON_CREATABLE macro, note this 
// is already supplied in VS.NET
#if defined(OBJECT_ENTRY_NON_CREATEABLE_EX) && _ATL_VER < 0x0700
	#define OBJECT_ENTRY_NON_CREATEABLE_EX(clsid, class)\
		{&clsid,class::UpdateRegistry,NULL,NULL,NULL,0,NULL,class::GetCategoryMap,class::ObjectMain },
#endif

/*****************************************************************************
** Procedure:  COM_INTERFACE_ENTRY_THIS
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: This macro can be used in an COM interface map to
**              retrieve the "this" pointer for an ATL object when the
**              object is implemented in the same module/process.
**
/****************************************************************************/
#define COM_INTERFACE_ENTRY_THIS() {&GetObjectCLSID(), 0, comutils::_This},

/*****************************************************************************
** Procedure:  HR
** 
** Arguments:  HRESULT
** 
** Returns: void
** 
** Description: This macro tests an HRESULT for error.  If the given 
**              HRESULT is an error, it throws a _com_error exception.
**              similar to the wrappers from _com_ptr.
**
/****************************************************************************/
#define HR(ex) { comutils::HRESULTEX _hr(ex); }

namespace comutils {
inline HRESULT WINAPI _This(void* pv, REFIID /*iid*/, void** ppvObject, DWORD) { *ppvObject = pv; return S_OK; }

/*****************************************************************************
** Procedure:  QueryImplementation
** 
** Arguments:  'punk' - IUnknown interface
**             'clsid' - CLSID to retrieve
**             'ppobj' - Returning pointer to implementation
** 
** Returns: OLE2 result code
** 
** Description: This template function can be used on a class which has
**              the COM_INTERFACE_ENTRY_THIS macro in it's ATL interface
**              map.  This function will then allow the caller to retrieve
**              the underlying implementation pointer (i.e. Cxxx object).
**
/****************************************************************************/
template <typename T>
HRESULT QueryImplementation(IUnknown* punk, REFCLSID clsid, T** ppobj)
{
	COINITPARAM(ppobj);
    if (!punk) return E_INVALIDARG;
    HRESULT hr = punk->QueryInterface(clsid, (void**)ppobj);
    if( FAILED(hr) ) return hr;
#ifdef _ATL_DEBUG_INTERFACES
    // If _ATL_DEBUG_INTERFACES, then ATL has changed the
    // pointer computed by QI to be a _QIThunk for the pointer
    _QIThunk* thunk = reinterpret_cast<_QIThunk*>(*ppobj);
    ATLASSERT(thunk);
#if(_ATL_VER < 0x0700)
    // The pUnk held by the thunk is the pv returned by _This.
    *ppobj = reinterpret_cast<T*>(thunk->pUnk);
    // Delete this thunk as it will never be released.
    ATLASSERT(thunk->m_dwRef == 1);
	if( !thunk->bNonAddRefThunk ) _pModule->DeleteThunk(thunk);
#else
    // The pUnk held by the thunk is the pv returned by _This.
    *ppobj = reinterpret_cast<T*>(thunk->m_pUnk);
    // Delete this thunk as it will never be released.
    ATLASSERT(thunk->m_dwRef == 1);
	if( !thunk->m_bNonAddRefThunk ) _pModule->DeleteThunk(thunk);
#endif // _ATL_VER < 0x700
#endif // _ATL_DEBUG_INTERFACES
    return S_OK;
}

template <typename T>
HRESULT QueryImplementation(IUnknown* punk, T** ppobj) {
    return QueryImplementation(punk, T::GetObjectCLSID(), ppobj);
}

/****************************************************************************/
// IUnknownNoImpl
//
// This structure defines a "nop" IUnknown interface to be used with
// objects that act like COM objects but the lifetime is controlled outside
// COM itself.
//
/****************************************************************************/
struct IUnknownNoImpl : public IUnknown
{
	STDMETHODIMP_(ULONG) AddRef() { return 1; }
	STDMETHODIMP_(ULONG) Release() { return 1; }
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
		if (InlineIsEqualUnknown(riid))	{
			*ppv = this;
			return S_OK;
		}
		else {
			*ppv = 0;
			return E_NOINTERFACE;
		}
	}
};

/****************************************************************************/
// ErrorToString
//
// This function looks up an error string from a system code.
//
/****************************************************************************/
inline std::string ErrorToString(long errCode)
{
    LPVOID lpBuffer;
    if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, 
			0, errCode, 0, (LPSTR)&lpBuffer, 128, NULL) > 0)
	{
		std::string sMessage((LPCSTR)lpBuffer);
		LocalFree(lpBuffer);
		return sMessage;
	}
	return std::string("Unknown");
}

#if defined(_INC_COMDEF)
/****************************************************************************/
// throw_com_error
//
// This function creates a new COM exception to be thrown.
//
/****************************************************************************/
inline void throw_com_error(HRESULT hr, const wchar_t* pszError=NULL) /*throw(_com_error)*/
{
	if (pszError != NULL) {
		ICreateErrorInfo* pICEI;
		if (SUCCEEDED(CreateErrorInfo(&pICEI))) {
			// Used for description only!
			pICEI->SetDescription(const_cast<LPOLESTR>(pszError));
			// Create our error info block.
			IErrorInfo* pErrorInfo;
			pICEI->QueryInterface(IID_IErrorInfo, reinterpret_cast<void**>(&pErrorInfo));
			// Release the error info creator.
			pICEI->Release();
			// Throw our exception
			throw _com_error(hr, pErrorInfo);
		}
	}
	throw _com_error(hr);

}// throw_com_error

/****************************************************************************/
// Dump facility for errors
//
// This function dumps a C++ COM error out to the debug terminal.
//
/****************************************************************************/
inline std::string _format_com_error(HRESULT hr, LPCSTR pszMessage=NULL, LPCSTR pszSource=NULL, LPCSTR pszDesc=NULL, LPCSTR pszFile=NULL, int nLine=0)
{
	std::ostringstream stm;
	stm << "(" << std::hex << std::showbase << hr << ") ";
	if (pszMessage != NULL)
		stm << pszMessage;
	if (pszDesc != NULL)
		stm << " " << pszDesc;
	if (pszSource != NULL)
		stm << " by " << pszSource;
	if (pszFile != NULL)
		stm << " in " << pszFile << " @ " << nLine;
	return stm.str();
}
inline std::string _format_com_error(const _com_error& e, const LPCSTR pszFile=NULL, int nLine=0)
{
	return _format_com_error(e.Error(), _bstr_t(e.ErrorMessage()), e.Source(), static_cast<LPCSTR>(e.Description()), pszFile, nLine);
}
inline void _dump_com_error(HRESULT hr, LPCSTR pszMessage=NULL, LPCSTR pszSource=NULL, LPCSTR pszDesc=NULL, LPCSTR pszFile=NULL, int nLine=0)
{
	OutputDebugStringA(_format_com_error(hr,pszMessage,pszSource,pszDesc, pszFile, nLine).c_str());
}
inline void _dump_com_error(const _com_error &e, const LPCSTR pszFile=NULL, int nLine=0)
{
	_dump_com_error(e.Error(), _bstr_t(e.ErrorMessage()), e.Source(), static_cast<LPCSTR>(e.Description()), pszFile, nLine);
}
#endif // _COM_DEF

/****************************************************************************/
// create_variant_blob
//
// This function creates a VARIANT array from a pointer/size
//
/****************************************************************************/
inline VARIANT create_variant_blob(BYTE* pvBlob, long cbSize)
{
	SAFEARRAY FAR* psa;
	SAFEARRAYBOUND rgsabound[1];
	rgsabound[0].lLbound = 0;	
	rgsabound[0].cElements = cbSize;

	// create a single dimensional byte array
	psa = SafeArrayCreate(VT_I1, 1, rgsabound);
			
	// set the blob data into the variant as a set of bytes
	BYTE *pByte;
	if (SafeArrayAccessData(psa,reinterpret_cast<void **>(&pByte)) == NOERROR) {
		memcpy(pByte, pvBlob, cbSize);
		SafeArrayUnaccessData(psa);
	}

	// Assign to a variant array - mark as unsigned byt array
	VARIANT varArray;
	VariantInit(&varArray);
	varArray.vt = (VT_ARRAY | VT_UI1);
	varArray.parray = psa;

	return varArray;

}// create_variant_blob

/****************************************************************************/
// CoGetCallerUserID
//
// This function returns the userid which is performing the given call
//
/****************************************************************************/
inline JTI_Util::tstring CoGetCallerUserID()
{
	JTI_Util::tstring strUser;
#ifdef __autosvcs_h__
	CComPtr<IObjectContext> pContext; 
	if (SUCCEEDED(::CoGetObjectContext(__uuidof(pContext),
		reinterpret_cast<void**>(&pContext))) && pContext != NULL)
	{
		CComPtr<ISecurityProperty> pSecProps;
		if (SUCCEEDED(pContext.QueryInterface(&pSecProps)))
		{
			PSID sidUserID;
			if (SUCCEEDED(pSecProps->GetOriginalCallerSID(&sidUserID)))
			{
				SID_NAME_USE sidUsage;
				TCHAR chUserID[_MAX_PATH], chDomain[_MAX_PATH];
				DWORD dwSize = _MAX_PATH, dwDomainSize = _MAX_PATH;

				// Get the user-name from the SID.
				if (::LookupAccountSid(NULL, sidUserID, chUserID, &dwSize, chDomain, &dwDomainSize, &sidUsage))
					strUser = chUserID;

				// Release the SID.
				pSecProps->ReleaseSID(&sidUserID);
			}
		}
	}
	// Not running under COM+/MTS -- use local NT security to retrieve the
	// user-id name for storing into the database.
	if (strUser.empty())
#endif
	{
		DWORD dwSize = _MAX_PATH; TCHAR chUserID[_MAX_PATH];
		if (::GetUserName(chUserID, &dwSize))
			strUser = chUserID;
	}
	return strUser;

}// GetCallerUserID

/****************************************************************************/
// CoGetCLSIDOfFreeThreadedMarshaler
//
// This function retrieves the CLSID of the Free-Threaded Marshaler.
//
/****************************************************************************/
inline HRESULT CoGetCLSIDOfFreeThreadedMarshaler(CLSID* pclsid)
{
	*pclsid = GUID_NULL;
	IUnknownNoImpl fakeUnk; IUnknown* pUnk = 0;
	HRESULT hr = ::CoCreateFreeThreadedMarshaler(&fakeUnk,&pUnk);
	if (SUCCEEDED(hr))
	{
		CComQIPtr<IMarshal> spMarshal(pUnk);
		hr = spMarshal->GetUnmarshalClass(IID_IUnknown, &fakeUnk,                                              MSHCTX_INPROC,0,MSHLFLAGS_NORMAL, pclsid);
		spMarshal.Release();
		pUnk->Release();
	}
	return hr;
}

/****************************************************************************/
// CoIsObjectApartmentNeutral
//
// This function determines if the given object can be freely passed
// between threads of different apartments.
//
/****************************************************************************/
inline HRESULT CoIsObjectApartmentNeutral(IUnknown* pUnk, bool* pb)
{
	*pb = false;
	CComPtr<IMarshal> spMarshal;
	HRESULT hr = pUnk->QueryInterface(IID_IMarshal,(void**)&spMarshal);
	if (FAILED(hr)) 
		return hr;
	
	IUnknownNoImpl fakeUnk; CLSID clsid;
	hr = spMarshal->GetUnmarshalClass(IID_IUnknown, &fakeUnk, MSHCTX_INPROC, 0, MSHLFLAGS_NORMAL, &clsid);
	if (FAILED(hr)) 
		return hr;

	CLSID clsidFTM;
	hr = CoGetCLSIDOfFreeThreadedMarshaler(&clsidFTM);
	if(FAILED(hr))
		return hr;
	
	*pb = (InlineIsEqualGUID(clsidFTM, clsid) == TRUE);
	return hr;
}

/****************************************************************************/
// CoIsMTA
//
// This function determines whether the apartment associated with the current
// thread is MTA.
//
// S_OK - If the threads apartment is MTA
// S_FALSE - Not MTA
//
/****************************************************************************/
inline HRESULT CoIsMTA()
{
	if (SUCCEEDED(CoInitializeEx(0, COINIT_MULTITHREADED))) {
		CoUninitialize();
		return S_OK;
	}
	return S_FALSE;
}

/****************************************************************************/
// CoIsSTA
//
// This function determines whether the apartment associated with the current
// thread is STA.
//
// S_OK - If the threads apartment is STA
// S_FALSE - Not STA
//
/****************************************************************************/
inline HRESULT CoIsSTA()
{
	if (SUCCEEDED(CoInitializeEx(0, COINIT_APARTMENTTHREADED))) {
		CoUninitialize();
		return S_OK;
	}
	return S_FALSE;
}

/****************************************************************************/
// CoIsProxyInterface
//
// This function will determine whether the given interface is a real
// pointer or a pointer to a proxy.
//
// S_OK - proxy
// FAILED - raw pointer
//
/****************************************************************************/
inline HRESULT CoIsProxyInterface(IUnknown* pUnk)
{
	CComPtr<IUnknown> sp; 
	return pUnk->QueryInterface(IID_IProxyManager,(void**)&sp);
}

/****************************************************************************/
// CoIsTypeLibMarshaledInterface
//
// This method determines if the interface is a type-library marshaled
// interface, or whether a custom proxy has been registered.
//
// S_OK - If the IID is typelib marshaled
// S_FALSE - custom marshaler.
//
/****************************************************************************/
inline HRESULT CoIsTypeLibMarshaledInterface(REFIID riid)
{
	// Look for the ProxyStubClsID32 key for the given interface in the registry.
	LPOLESTR lp = 0;
	::StringFromIID(riid, &lp);
	char szKey[512];
	wsprintfA(szKey, "Interface\\%S", lp);
	::CoTaskMemFree(lp);

	HKEY hk;
	HRESULT hr = E_UNEXPECTED;
	if (::RegOpenKeyA(HKEY_CLASSES_ROOT, szKey, &hk) == ERROR_SUCCESS)
	{
		char szValue[512]; LONG cb = sizeof(szValue);
		if (::RegQueryValueA(hk, "ProxyStubClsid32", szValue, &cb) == ERROR_SUCCESS)
		{
			const char* pszClsId = "{00020420-0000-0000-C000-000000000046}";
			hr = (!lstrcmpA(szValue, pszClsId)) ? S_OK : S_FALSE;
		}
		::RegCloseKey(hk) ;
	}        
	return hr;
}

/****************************************************************************/
// HRESULTEX
//
// This class encapsulates an OLE/COM result code and throws an exception
// on failure. This exception can then be caught like any other _com_ptr
// error condition.
//
/****************************************************************************/
struct HRESULTEX
{
	HRESULTEX(void) {/* */}
	HRESULTEX(HRESULT hr) {
		if (FAILED(hr)) 
		{
			IErrorInfo* piErrInfo = 0;
			if (::GetErrorInfo(0L, &piErrInfo) == S_OK)
				throw _com_error(hr,piErrInfo);
			else
				throw _com_error(hr);
		}
	}
	HRESULTEX& operator=(HRESULT hr) {
		if (FAILED(hr)) {
			IErrorInfo* piErrInfo = 0;
			if (::GetErrorInfo(0L, &piErrInfo) == S_OK)
				throw _com_error(hr,piErrInfo);
			else
				throw _com_error(hr);
		}
		return *this;
	}
};
}; // namespace comutils

#endif // #if _INC_COMDEF

#ifdef __ATLCOM_H__
/****************************************************************************/
// CComContainerCopy
//
// The default implementation of CComEnumOnSTL shares the STL collection with
// the COM collection object, in many cases this is a problem if the collection
// can change when enumerators are outstanding. This class allows a seperate
// collection to be created which is owned by the enumerator.
//
// The initial version of this code was taken from ATL INTERNALS (C) Chris Sells
//
// Example usage:
//
// STDMETHODIMP CMyClass::GetEnum(IUnknown** ppunkEnum) 
// {
//   *ppunkEnum = 0;
//	 typedef CComEnumOnSTL<IEumVARIANT, &IID_IEnumVARIANT, VARIANT, 
//                         _Copy<VARIANT>, std::vector<VARIANT> >
//                         CComEnumVariantOnVector;
//
//   CComObject<CComEnumVariantOnVector>* pe = 0;
//   HRESULT hr = pe->CreateInstance(&pe);
//   if (SUCCEEDED(hr))
//   {
//      pe->AddRef();
//      CComObject<CComContainerCopy<std::vector<VARIANT> > >* pCopy = 0;
//      hr = pCopy->CreateInstance(&pCopy);
//      if (SUCCEEDED(hr))
//      {
//         pCopy->AddRef();
//         hr = pCopy->Copy(m_arr);
//         if (SUCCEEDED(hr))
//         {
//             hr = pe->Init(pCopy->GetIUnknown(), pCopy->GetUnderlyingContainer());
//             if (SUCCEEDED(hr))
//                 hr = pe->QueryInterface(ppunkEnum);
//         }
//         pCopy->Release();
//     }
//     pe->Release();
//   }
//   return hr;
// }
//
/****************************************************************************/
template <typename _CollType, typename _ThreadingModel=CComObjectThreadModel>
class CComContainerCopy :
	public CComObjectRootEx<_ThreadingModel>,
	public IUnknown	// CComEnumOnSTL requires an IUnknown*
{
private:
	_CollType coll_;
public:
	BEGIN_COM_MAP(CComContainerCopy)
		COM_INTERFACE_ENTRY(IUnknown)
	END_COM_MAP()
	_CollType& GetUnderlyingContainer() { return coll_; }
	HRESULT Copy(const _CollType& col)
	{
		try {
			coll_ = col;
			return S_OK;
		}
		catch (std::exception)
		{
			return E_OUTOFMEMORY;
		}
	}
};

/****************************************************************************/
// ISupportErrorInfoImplEx
//
// This template implements an extra error handler function which pulls
// the error text information for HRESULTs coming into the function.
//
/****************************************************************************/
template <class T, const IID* piid>
class ISupportErrorInfoImplEx : public ISupportErrorInfoImpl<piid>
{
private:
	inline HRESULT _ISReportError(HRESULT hr, LPCSTR lpszDesc=NULL)
	{
		CComPtr<ICreateErrorInfo> pICEI;
		if (SUCCEEDED(CreateErrorInfo(&pICEI))) {
			pICEI->SetGUID(this->GetErrorIID());
			LPOLESTR lpsz = NULL;
			ProgIDFromCLSID(this->GetErrorCLSID(), &lpsz);
			if (lpsz != NULL) {
				pICEI->SetSource(lpsz);
				CoTaskMemFree(lpsz);
			}
			if (lpszDesc != NULL)
				pICEI->SetDescription(_bstr_t(lpszDesc));
			CComPtr<IErrorInfo> pErrorInfo;
			if (SUCCEEDED(pICEI->QueryInterface(IID_IErrorInfo, (void**)&pErrorInfo)))
				SetErrorInfo(0, pErrorInfo);
		}
		return (hr == 0) ? DISP_E_EXCEPTION : hr;
	}
	
protected:
	virtual const CLSID& GetErrorCLSID() const { return T::GetObjectCLSID(); }
	virtual const IID& GetErrorIID() const { return *piid; }

	inline HRESULT ErrorInfo(HRESULT hResult, const LPCSTR szFile=NULL, int nLine=0) {
		std::string sMessage = comutils::ErrorToString(hResult);
		comutils::_dump_com_error(hResult,sMessage.c_str(),NULL,NULL,szFile,nLine);
		return _ISReportError(hResult,sMessage.c_str());
	}
	inline HRESULT ErrorInfo(HRESULT hResult, const LPCSTR szFile, int nLine, const char* pszMessage) {
		comutils::_dump_com_error(hResult,pszMessage,NULL,NULL,szFile,nLine);
		return _ISReportError(hResult,pszMessage);
	}
	inline HRESULT ErrorInfo(HRESULT hResult, LPCSTR pszFunc, const LPCSTR szFile=NULL, int nLine=0) {
		std::string sMessage = comutils::ErrorToString(hResult);
		comutils::_dump_com_error(hResult,sMessage.c_str(),NULL,pszFunc,szFile,nLine);
		return _ISReportError(hResult,sMessage.c_str());
	}
	inline HRESULT ErrorInfo(const _com_error& e, const LPCSTR szFile=NULL, int nLine=0) {
		comutils::_dump_com_error(e,szFile,nLine);
		return _ISReportError(e.Error(),e.Description());
	}
	inline HRESULT ProcessError(HRESULT hResult, LPCSTR pszFunc=NULL) {
		std::string sMessage = comutils::ErrorToString(hResult);
		if (pszFunc == NULL)
			pszFunc = sMessage.c_str();
		return _ISReportError(hResult, pszFunc);
	}
	inline HRESULT ProcessError(HRESULT hResult, LPCWSTR pszFunc=NULL) {
		USES_CONVERSION;
		std::string sMessage = comutils::ErrorToString(hResult);
		if (pszFunc == NULL)
			pszFunc = sMessage.c_str();
		return _ISReportError(hResult, W2A(pszFunc));
	}
};

#ifdef __ObjectControl_FWD_DEFINED__
/****************************************************************************/
// IObjectControlImpl
//
// This template implements the MTS/COM+ IObjectControl interface for
// a component object.
//
/****************************************************************************/
template <class T, bool fSupportsPooling>
class IObjectControlImpl : public IObjectControl
{
// Constructor
public:
	IObjectControlImpl() {}

// Access Methods
public:
	bool HaveObjectContext() const { return m_spObjectContext != NULL; }
	HRESULT EnableCommit() { return (m_spObjectContext) ? m_spObjectContext->EnableCommit() : E_POINTER; }
	HRESULT DisableCommit() { return (m_spObjectContext) ? m_spObjectContext->DisableCommit() : E_POINTER; }
	HRESULT SetComplete() {	return (m_spObjectContext) ? m_spObjectContext->SetComplete() : E_POINTER; }
	HRESULT SetAbort() { return (m_spObjectContext) ? m_spObjectContext->SetAbort() : E_POINTER; }
	bool IsCallerInRole(LPCTSTR pszRole) { BOOL bInRole = FALSE; if (m_spObjectContext) m_spObjectContext->IsCallerInRole(_bstr_t(pszRole), &bInRole); return (bInRole==TRUE); }
	bool IsInTransaction() { return (m_spObjectContext) ? (m_spObjectContext->IsInTransaction()==TRUE) : FALSE; }
	bool IsSecurityEnabled() { return (m_spObjectContext) ? (m_spObjectContext->IsSecurityEnabled()==TRUE) : FALSE; }

// IObjectControl
public:
	// Whenever a client calls an MTS object that isn't already activated, 
	// the MTS run-time environment automatically activates the object. 
	// This is called just-in-time activation. MTS invokes this method before 
	// passing the client's method call on to the object. This allows objects 
	// to do context-specific initialization
	STDMETHOD(Activate)() {
		HRESULT hr = ::CoGetObjectContext(__uuidof(m_spObjectContext), reinterpret_cast<void**>(&m_spObjectContext));
		if (SUCCEEDED(hr))
			return S_OK;
		return hr;
	}

	// This indicates to the MTS run-time environment that the object can be 
	// added to an object pool after deactivation rather than being destroyed. 
	// Whenever an instance is required, one is drawn from the pool rather 
	// than created.
	STDMETHOD_(BOOL, CanBePooled)() {
		return fSupportsPooling;
	}
	
	// This is called when the object is being de-activatd and has indicated
	// that it can be pooled. It needs to release any resources as if it
	// were being destroyed.
	STDMETHOD_(void, Deactivate)() {
			m_spObjectContext.Release();
	}

// Class data
public:
	CComPtr<IObjectContext> m_spObjectContext;
};
#endif // __ObjectControl_FWD_DEFINED__

#endif // __COM_UTLS_INC__
