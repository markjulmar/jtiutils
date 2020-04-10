/****************************************************************************/
//
// AdoConn.h
//
// This implements a wrapper class around the ADODB::Connection object
// which will return disconnected ADODB::Recordset interfaces and provide 
// some exception-safe extractor methods to retrieve data from the 
// ADODB::Field object
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
// 03/15/2004  MCS   Moved out of comutils.h into it's own header.
//
/****************************************************************************/

#ifndef __ADOCONN_H_INC__
#define __ADOCONN_H_INC__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/*----------------------------------------------------------------------------
// INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <vector>
#include <string>
#include <stdlib.h>
#include <JTIUtils.h>
#include <comutil.h>
#include <comdef.h>

/*----------------------------------------------------------------------------
// IMPORTS
-----------------------------------------------------------------------------*/
#ifndef _NO_IMPORT_ADO_
#pragma warning(disable:4146)
#import <c:\program files\common files\system\ado\msado25.tlb> rename("EOF", "adoEOF")
#pragma warning(default:4146)
#endif

namespace JTI_Dac
{
/**************************************************************************************
** adoData
**
** This class is used to retrieve data from an ADO recordset when using the 
** _com_ptr wrapper classes which can throw exceptions.  These accessors wrap the
** fields with try/catch logic and cast the values to the appropriate types.
**
**************************************************************************************/
struct adoData
{
	/// DoesFieldExist
	///
	/// This method returns a true/false result based on whether the given field
	/// exists in the passed recordset.
	///
	static bool DoesFieldExist(ADODB::_RecordsetPtr& prs, const wchar_t* pszField)
	{
		COM_TRY
		{
			_variant_t spVar = prs->Fields->GetItem(pszField)->Value;
			return ((spVar.vt != VT_NULL) && (spVar.vt != VT_EMPTY));
		}
		COM_CATCH_IGNORE
		return false;
	}

	/// GetFieldData
	///
	/// These methods provide exception-safe wrappers for pulling specific types
	/// out of a Field collection from a _RecordsetPtr (_com_ptr).
	///
	static bool GetFieldData(const ADODB::FieldsPtr& prFields, const wchar_t* pszField, long& lValue)
	{
		bool success = false;
		lValue = 0;
		COM_TRY
		{
			_variant_t spVar = prFields->GetItem(pszField)->Value;
			if ((spVar.vt != VT_NULL) && (spVar.vt != VT_EMPTY) && 
				(spVar.vt != VT_DISPATCH) && (spVar.vt != VT_UNKNOWN))
			{
				lValue = static_cast<long>(spVar);
				success = true;
			}
		}
		COM_CATCH_NP(pszField);
		return success;

	}

	static bool GetFieldData(const ADODB::FieldsPtr& prFields, const wchar_t* pszField, DWORD& dwValue)
	{
		bool success = false;
		dwValue = 0;
		COM_TRY
		{
			_variant_t spVar = prFields->GetItem(_bstr_t(pszField))->Value;
			if ((spVar.vt != VT_NULL) && (spVar.vt != VT_EMPTY) && 
				(spVar.vt != VT_DISPATCH) && (spVar.vt != VT_UNKNOWN))
			{
				dwValue = static_cast<DWORD>(static_cast<long>(spVar));
				success = true;
			}
		}
		COM_CATCH_NP(pszField);
		return success;
	}

	static bool GetFieldData(const ADODB::FieldsPtr& prFields, const wchar_t* pszField, double& dValue)
	{
		bool success = false;
		dValue = 0.0;
		COM_TRY
		{
			_variant_t spVar = prFields->GetItem(_bstr_t(pszField))->Value;
			if ((spVar.vt != VT_NULL) && (spVar.vt != VT_EMPTY) && 
				(spVar.vt != VT_DISPATCH) && (spVar.vt != VT_UNKNOWN))
			{
				dValue = static_cast<double>(spVar);
				success = true;
			}
		}
		COM_CATCH_NP(pszField);
		return success;
	}

#ifdef __AFX_H__
	static bool GetFieldData(const ADODB::FieldsPtr& prFields, const wchar_t* pszField, CString& strValue)
	{
		bool success = false;
		strValue.Empty();
		COM_TRY
		{
			_variant_t spVar = prFields->GetItem(_bstr_t(pszField))->Value;
			if( spVar.vt != VT_NULL )
			{
				if( (spVar.vt != VT_EMPTY) && (spVar.vt != VT_DISPATCH) && (spVar.vt != VT_UNKNOWN))
				{
					strValue = (const TCHAR*) _bstr_t(spVar);
					strValue.TrimRight();
					strValue.ReleaseBuffer();
					success = true;
				}
			}
			else
			{
				strValue = _T("");
				success = true;
			}
		}
		COM_CATCH_NP(pszField);
		return success;
	}
#endif	

#ifdef _XSTRING_
	static bool GetFieldData(const ADODB::FieldsPtr& prFields, const wchar_t* pszField, std::wstring& strValue)
	{
		bool success = false;
		strValue.clear();
		COM_TRY
		{
			_variant_t spVar = prFields->GetItem(_bstr_t(pszField))->Value;
			if ((spVar.vt != VT_NULL) && (spVar.vt != VT_EMPTY) && 
				(spVar.vt != VT_DISPATCH) && (spVar.vt != VT_UNKNOWN))
			{
				strValue = _bstr_t(spVar);
				stdx::trim(&strValue[0]);
				success = true;
			}
		}
		COM_CATCH_NP(pszField);
		return success;
	}

	static bool GetFieldData(const ADODB::FieldsPtr& prFields, const wchar_t* pszField, std::string& strValue)
	{
		bool success = false;
		strValue.clear();
		COM_TRY
		{
			_variant_t spVar = prFields->GetItem(_bstr_t(pszField))->Value;
			if ((spVar.vt != VT_NULL) && (spVar.vt != VT_EMPTY) && 
				(spVar.vt != VT_DISPATCH) && (spVar.vt != VT_UNKNOWN))
			{
				strValue = _bstr_t(spVar);
				stdx::trim(&strValue[0]);
				success = true;
			}
		}
		COM_CATCH_NP(pszField);
		return success;
	}
#endif	

	static bool GetFieldData(const ADODB::FieldsPtr& prFields, const wchar_t* pszField, _bstr_t& strValue)
	{
		bool success = false;
		strValue = L"";
		COM_TRY
		{
			_variant_t spVar = prFields->GetItem(_bstr_t(pszField))->Value;
			if (spVar.vt != VT_NULL)
			{
				if ((spVar.vt != VT_EMPTY) && (spVar.vt != VT_DISPATCH) && (spVar.vt != VT_UNKNOWN))
				{
					strValue = _bstr_t(spVar);
					LPWSTR pe = (LPWSTR)((LPCWSTR)strValue);
					pe += (lstrlenW(pe)-1);
					while (*pe == L' ') --pe; *(pe+1) = 0;
					success = true;
				}
			}
			else
			{
				strValue = _T("");
				success = true;
			}
		}
		COM_CATCH_NP(pszField);
		return success;
	}

	static bool GetFieldData(const ADODB::FieldsPtr& prFields, const wchar_t* pszField, bool& bValue)
	{
		bool success = false;
		bValue = false;
		COM_TRY
		{
			_variant_t spVar = prFields->GetItem(_bstr_t(pszField))->Value;
			if ((spVar.vt != VT_NULL) && (spVar.vt != VT_EMPTY) && 
				(spVar.vt != VT_DISPATCH) && (spVar.vt != VT_UNKNOWN))
			{
				bValue = static_cast<bool>(spVar);
				success = true;
			}
		}
		COM_CATCH_NP(pszField);
		return success;
	}

#if defined(__ATLCOMTIME_H__) || defined(__AFXDISP_H__)
	static bool GetFieldData(const ADODB::FieldsPtr& prFields, const wchar_t* pszField, COleDateTime& dtValue)
	{
		bool success = false;

		// Null out the date and mark it as invalid.
		dtValue = (DATE)0;
		dtValue.SetStatus(COleDateTime::invalid);
		COM_TRY
		{
			_variant_t spVar = prFields->GetItem(_bstr_t(pszField))->Value;
			if ((spVar.vt != VT_NULL) && (spVar.vt != VT_EMPTY) && 
				(spVar.vt != VT_DISPATCH) && (spVar.vt != VT_UNKNOWN))
			{
				dtValue = static_cast<DATE>(spVar);
				if (static_cast<DATE>(dtValue) != (DATE)0)
				{
					dtValue.SetStatus(COleDateTime::valid);
					success = true;
				}
			}
		}
		COM_CATCH_NP(pszField);
		return success;
	}
#endif

#ifdef _VECTOR_
	static bool GetFieldData(const ADODB::FieldsPtr& prFields, const wchar_t* pszField, std::vector<BYTE>& arrBytes)
	{
		bool success = false;
		arrBytes.clear(); // Clear existing data

		COM_TRY
		{
			long cbSize = prFields->GetItem(_bstr_t(pszField))->ActualSize;
			_variant_t spVar = prFields->GetItem(_bstr_t(pszField))->Value;
			if ((spVar.vt != VT_NULL) && (spVar.vt != VT_EMPTY) && 
				(cbSize > 0) && (spVar.vt == (VT_ARRAY | VT_UI1)))
			{
				BYTE *pBuf = NULL;
				SafeArrayAccessData(spVar.parray,(void **)&pBuf);
				arrBytes.resize(cbSize+1);
				memcpy(&arrBytes[0], pBuf, cbSize);
				SafeArrayUnaccessData(spVar.parray);
				success = true;
			}
		}
		COM_CATCH_NP(pszField);
		return success;
	}
#endif

#ifdef __AFXCOLL_H__
	static void GetFieldData(const ADODB::FieldsPtr& prFields, const wchar_t* pszField, CByteArray& arrBytes)
	{
		bool success = false;
		arrBytes.RemoveAll();

		COM_TRY
		{
			long cbSize = prFields->GetItem(_bstr_t(pszField))->ActualSize;
			_variant_t spVar = prFields->GetItem(_bstr_t(pszField))->Value;
			if ((spVar.vt != VT_NULL) && (spVar.vt != VT_EMPTY) && 
				(cbSize > 0) && (spVar.vt == (VT_ARRAY | VT_UI1)))
			{
				BYTE *pBuf = NULL;
				SafeArrayAccessData(spVar.parray,(void **)&pBuf);
				arrBytes.SetSize(cbSize+1);
				memcpy(arrBytes.GetData(), pBuf, cbSize);
				SafeArrayUnaccessData(spVar.parray);
				success = true;
			}
		}
		COM_CATCH_NP(pszField);
		return success;
	}
#endif

	static bool GetFieldData(const ADODB::FieldsPtr& prFields, const wchar_t* pszField, ADODB::_RecordsetPtr& rSet)
	{
		bool success = false;
		COM_TRY
		{
			_variant_t spVar = prFields->GetItem(_bstr_t(pszField))->Value;
			if(spVar.vt == VT_DISPATCH)
			{
				rSet = spVar;
				success = true;
			}
		}
		COM_CATCH_NP(pszField);
		return success;
	}

	///
	/// Easy helper functions -- ToXXXX()
	///
	/// Pass the Recordset pointer and name fo the field in Unicode form:
	///
	/// Example:
	///
	///     _RecordsetPtr rSet
	///             ...
	///     long AccountID = ToLong(rSet, L"AccountID");
	///     double Balance = ToDouble(rSet, L"CurrentBalance");
	///
	///
	inline static long ToLong(ADODB::_RecordsetPtr& prs, const wchar_t* pszField)
	{
		long lValue;
		GetFieldData(prs->Fields, pszField, lValue);
		return lValue;
	}

	inline static DWORD ToDWord(ADODB::_RecordsetPtr& prs, const wchar_t* pszField)
	{
		DWORD dwValue;
		GetFieldData(prs->Fields, pszField, dwValue);
		return dwValue;
	}

	inline static double ToFloat(ADODB::_RecordsetPtr& prs, const wchar_t* pszField)
	{
		double dblValue;
		GetFieldData(prs->Fields, pszField, dblValue);
		return dblValue;
	}

#ifdef __AFX_H__
	inline static CString ToString(ADODB::_RecordsetPtr& prs, const wchar_t* pszField)
	{
		CString strValue;
		GetFieldData(prs->Fields, pszField, strValue);
		return strValue;
	}
#else
	inline static JTI_Util::tstring ToString(ADODB::_RecordsetPtr& prs, const wchar_t* pszField)
	{
		JTI_Util::tstring strValue;
		GetFieldData(prs->Fields, pszField, strValue);
		return strValue;
	}
#endif

	inline static bool ToBool(ADODB::_RecordsetPtr& prs, const wchar_t* pszField)
	{
		bool bValue;
		GetFieldData(prs->Fields, pszField, bValue);
		return bValue;
	}

#if defined(__ATLCOMTIME_H__) || defined(__AFXDISP_H__)
	inline static COleDateTime ToDateTime(ADODB::_RecordsetPtr& prs, const wchar_t* pszField)
	{
		COleDateTime dtTime;
		GetFieldData(prs->Fields, pszField, dtTime);
		return dtTime;
	}
#endif

	inline static ADODB::_RecordsetPtr ToRecordset(ADODB::_RecordsetPtr& prs, const wchar_t* pszField)
	{
		ADODB::_RecordsetPtr rs;
		GetFieldData(prs->Fields, pszField, rs);
		return rs;
	}
};

/**************************************************************************
// CAdoConnection
//
// This object wraps the ADO Connection object and allows direct SQL
// statements to be executed against a cached connection object.
//
**************************************************************************/
class CAdoConnection
{
// Constructor
public:
	CAdoConnection(const char* pszConnStr = "") : connstr_((pszConnStr)?pszConnStr:""), timeout_(0), closeOnErr_(true) {/* */}
	CAdoConnection(const CAdoConnection& rhs) : connstr_(rhs.connstr_), timeout_(rhs.timeout_), closeOnErr_(rhs.CloseOnError) {/* */}
	~CAdoConnection() {/* */}

// Properties
public:
	__declspec(property(get=get_ConnectionString)) const std::string ConnectionString;
	__declspec(property(get=get_LastError)) HRESULT LastError;
	__declspec(property(get=get_LastErrorText)) const std::string LastErrorText;
	__declspec(property(get=get_CloseOnError, put=set_CloseOnError)) bool CloseOnError;
	__declspec(property(get=get_Timeout, put=set_Timeout)) long CommandTimeoutSeconds;

// Methods
public:
	HRESULT OpenConnection(bool& bConnOpened, const char* pszConnStr = NULL)
	{
		USES_CONVERSION;

		lastHresult_ = S_OK;
		lastErr_.clear();
		bConnOpened = false;

		try
		{
			// Reset the connection data if necessary
			if (pszConnStr != NULL && _stricmp(connstr_.c_str(), pszConnStr) != 0) {
				CloseConnection();
				connstr_ = pszConnStr;
			}

			// If our connection has been reset, then create a new one
			if (connPtr_ == 0)
			{
				HR(connPtr_.CreateInstance(__uuidof(ADODB::Connection)));
			}

			// Connect to the database.
			if (connPtr_->GetState() == ADODB::adStateClosed)
			{
				HR(connPtr_->Open(A2W(connstr_.c_str()), _bstr_t(), _bstr_t(), ADODB::adConnectUnspecified));
				bConnOpened = true;
			}
		}
		catch(const _com_error& e)
		{
			lastHresult_ = e.Error();
			lastErr_ = comutils::_format_com_error(e);
			CloseConnection();
		}
		catch(...)
		{
			CloseConnection();
			lastHresult_ = E_UNEXPECTED;
			lastErr_ = comutils::_format_com_error(lastHresult_, NULL, NULL, "An unexpected exception has occurred.");
		}
		return lastHresult_;
	}

	void CloseConnection()
	{
		try
		{
			if (connPtr_ != 0)
				connPtr_->raw_Close();
			connPtr_ = 0;
		}
		catch(...)
		{
		}
	}

	HRESULT ExecQuery(const char* pszQueryFormat, ...)
	{
		stdx::inauto_ptr<char> chBuff;

		// Look for a format signature.
		if (strchr(pszQueryFormat, '%') != NULL)
		{
			va_list args;
			va_start(args, pszQueryFormat);

			// Allocate a buffer for our query
			chBuff.reset(new char[MAX_QUERY_STRING]);
			LPSTR lpBuff = chBuff.get();

			// Format the string
			int nCount = _vsnprintf_s(lpBuff, MAX_QUERY_STRING, _TRUNCATE, pszQueryFormat, args);
			if (nCount == -1)
				*(lpBuff + MAX_QUERY_STRING-1) = '\0';
			va_end(args);
			pszQueryFormat = lpBuff;
		}

		// Run the query
		try
		{
			// Attempt to open the connection if necessary.
			bool bConnOpened = false;
			HR(OpenConnection(bConnOpened));

			// Execute the passed SQL query; assume it is a text command
			connPtr_->CommandTimeout = timeout_;
			connPtr_->Execute(_bstr_t(pszQueryFormat), NULL, ADODB::adCmdText | ADODB::adExecuteNoRecords);
		}
		catch(const _com_error &e)
		{
			lastHresult_ = e.Error();
			lastErr_ = comutils::_format_com_error(e);
			CloseConnection();
		}
		catch(...)
		{
			CloseConnection();
			lastHresult_ = E_UNEXPECTED;
			lastErr_ = comutils::_format_com_error(lastHresult_, NULL, NULL, "An unexpected exception has occurred.");
		}
		return lastHresult_;
	}

	HRESULT GetRecordset(ADODB::_Recordset** ppRecordset, const char* pszQueryFormat, ...)
	{
		// Validate incoming parameters
		COINITPARAM(ppRecordset);
		stdx::inauto_ptr<char> chBuff;

		// If this is a var-arg command, then format it.
		if (strchr(pszQueryFormat, '%') != NULL)
		{
			va_list args;
			va_start(args, pszQueryFormat);

			// Allocate a buffer for our query
			chBuff.reset(new char[MAX_QUERY_STRING]);
			LPSTR lpBuff = chBuff.get();

			// Format the string
			int nCount = _vsnprintf_s(lpBuff, MAX_QUERY_STRING, _TRUNCATE, pszQueryFormat, args);
			if (nCount == -1)
				*(lpBuff + MAX_QUERY_STRING-1) = '\0';
			va_end(args);
			pszQueryFormat = lpBuff;
		}

		try
		{
			// Open the connection if necessary
			bool bConnOpened = false;
			HR(OpenConnection(bConnOpened));

			// Create an ADO recordset object
			ADODB::_RecordsetPtr spRecordset(__uuidof(ADODB::Recordset), NULL, CLSCTX_INPROC);
			if (spRecordset == NULL)
			{
				HR(E_NOINTERFACE);
			}
			else
			{
				// Set the command timeout
				connPtr_->CommandTimeout = timeout_;

				// Initialize the client-side cursor.
				spRecordset->CursorLocation = ADODB::adUseClient;

				// Connect the recordset to the selected connection.
				spRecordset->PutRefActiveConnection(connPtr_);

				// Populate our recordset by executing the provided query. Note, we ask for a returned
				// static recordset since we are about to go disconnected, we cannot use a dynamic recordset.
				spRecordset->Open(_bstr_t(pszQueryFormat), _variant_t(DISP_E_PARAMNOTFOUND, VT_ERROR), ADODB::adOpenStatic, ADODB::adLockBatchOptimistic, ADODB::adCmdText);

				// If the recordset is closed, then it returned no records.
				// Otherwise, if there is returned data, then cause the provider to 
				// fetch it by moving to the first row.
				if (spRecordset->State != ADODB::adStateClosed && spRecordset->RecordCount > 0)
					spRecordset->MoveFirst();

				// Now disconnect the recordset - this leaves all the data fetched in place, but we no longer
				// have a database connection attached to it (i.e. future updates/deletes, etc. don't work).
				// This allows the connection to be re-pooled when it is no longer in use.
				spRecordset->PutRefActiveConnection(NULL);

				// Now detach the actual ADO object from our wrapper and assign it to the passed recordset object.
				spRecordset->QueryInterface(__uuidof(*ppRecordset), reinterpret_cast<void**>(ppRecordset));
			}
		}
		catch(const _com_error &e)
		{
			lastHresult_ = e.Error();
			lastErr_ = comutils::_format_com_error(e);
			CloseConnection();
		}
		catch(...)
		{
			CloseConnection();
			lastHresult_ = E_UNEXPECTED;
			lastErr_ = comutils::_format_com_error(lastHresult_, NULL, NULL, "An unexpected exception has occurred.");
		}
		return lastHresult_;
	}

// Property helpers
public:
	const std::string& get_ConnectionString() const { return connstr_; }
	HRESULT get_LastError() const { return lastHresult_; }
	const std::string& get_LastErrorText() const { return lastErr_; }
	bool get_CloseOnError() const { return closeOnErr_; }
	void set_CloseOnError(bool f) { closeOnErr_ = f; }
	long get_Timeout() const { return timeout_; }
	void set_Timeout(long lTimeout) { timeout_ = lTimeout; }

// Class data
private:
	std::string connstr_;
	bool closeOnErr_;
	long timeout_;
	ADODB::_ConnectionPtr connPtr_;
	HRESULT lastHresult_;
	std::string lastErr_;
	enum { MAX_QUERY_STRING = 4096 };

// Unavailable methods
private:
	CAdoConnection& operator=(const CAdoConnection&);
};

}// JTI_Dac

#endif // __ADOCONN_H_INC__