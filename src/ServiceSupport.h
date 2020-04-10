/****************************************************************************/
//
// ServiceSupport.h
//
// This file implements a C++ class wrapper around the Windows service
// interface.
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

#ifndef __SERVICESUPP_H_INCL__
#define __SERVICESUPP_H_INCL__

/*****************************************************************************/
// PC-Lint options
//
//lint -save
//
// Ignore the return values for specific functions
//lint -esym(534, WaitForSingleObject)
//
// Allow C-style casts
//lint --e{1924}
//
// Supress duplicate header include files
//lint -e537
//
/*****************************************************************************/

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#ifndef _WINBASE_
	#define _WIN32_WINNT 0x0500
	#include <winbase.h>
#endif
#include <winsvc.h>
#include <tchar.h>
#include <TraceLogger.h>
#include <Lock.h>

// Define the default lock type based on what's being compiled.
#ifndef _MT
#define JTI_SERVICE_LOCK_TYPE SingleThreadModel
#else
#define JTI_SERVICE_LOCK_TYPE MultiThreadModel
#endif

namespace JTI_Util
{
/****************************************************************************/
// ServiceBase
//
// This class implements the base class which has the non-template
// functions and wrappers implemented.
//
/****************************************************************************/
//lint --e{1704} Private constructor ok
class ServiceBase
{
// Class data
protected:
	tstring ServiceName_;					// Service (short) name
	tstring DisplayName_;					// Display (long) name
	SERVICE_STATUS_HANDLE ServiceHandle_;	// Handle to running service
	SERVICE_STATUS Status_;					// Current status of the service
	EventSynch isStopping_;					// Stop event
	bool isService_;						// Whether this is a true service or console app.
	bool isRunning_;						// True when running

// Constructor
public:
	ServiceBase(const char* pszServiceName, const char* pszDisplayName, DWORD dwStartTime=0);
	ServiceBase(const wchar_t* pszServiceName, const wchar_t* pszDisplayName, DWORD dwStartTime=0);
	virtual ~ServiceBase() { ServiceHandle_ = NULL; }

// Methods for installing/removing/starting/stopping service
public:
	DWORD ServiceStart() const;
	DWORD ServiceStop() const;
	DWORD Install(LPCTSTR pszUserID, LPCTSTR pszPassword, bool fAutoStart=false, LPCTSTR pszDepends=NULL, LPCTSTR pszDescription=NULL) const;
	DWORD Uninstall() const;
	bool IsInstalled() const;
	bool AddApplicationEventLog(LPCTSTR pszMessageFile, DWORD dwType) const;
	void RemoveApplicationEventLog() const;
	DWORD SetServerFailureAction(SERVICE_FAILURE_ACTIONS* psfa);

// Static methods
public:
	static DWORD StartService(LPCTSTR pszServiceName, DWORD dwMaxWaitTime = 10000);
	static DWORD StopService(LPCTSTR pszServiceName, DWORD dwMaxWaitTime = 10000);
	static DWORD EnumServices(DWORD dwType, DWORD dwState, DWORD& dwCount, std::vector<BYTE>& arrDataOut);

// Overrides
protected:
	//---------------------------------------------------------------------
	// Run
	// This function is called when the service is running.  By default
	// it simply waits for the service to be stopped.
	//---------------------------------------------------------------------
	virtual void Run() { 
		if (GetServiceStatus() != SERVICE_RUNNING)
			SetServiceStatus(SERVICE_RUNNING);
		WaitForSingleObject(isStopping_.get(), INFINITE);
		SetServiceStatus(SERVICE_STOP_PENDING);
	}

	//---------------------------------------------------------------------
	// ParseServiceStartupParameters
	// This function is called to parse the command line parameters for
	// the service.
	//---------------------------------------------------------------------
	virtual bool ParseServiceStartupParameters(DWORD /*args*/, LPTSTR* /*argv*/) { return true; }

	//---------------------------------------------------------------------
	// Start/Stop/Pause/Continue/Shutdown/Interrogate
	// These functions are called by the service control dispatcher when
	// the SCM wants to tell us what to do.
	//---------------------------------------------------------------------
	virtual void Start(bool fIsService);
	virtual void Stop();
	virtual void Pause();
	virtual void Continue();
	virtual void Interrogate();
	virtual void Shutdown();

	//---------------------------------------------------------------------
	// AddEventLogSupport
	// This function is called when the service is being installed, it
	// can be overridden if the message file is not bound to the executable
	// process as a resource type.
	//---------------------------------------------------------------------
	virtual bool AddEventLogSupport() const { return AddApplicationEventLog(NULL, EVENTLOG_ERROR_TYPE|EVENTLOG_INFORMATION_TYPE); }

// Function methods
public:
	//---------------------------------------------------------------------
	// ServiceName/DisplayName
	// Retrieves the service and descriptive name
	//---------------------------------------------------------------------
	const tstring& ServiceName() const { return ServiceName_; }
	const tstring& DisplayName() const { return DisplayName_; }
	//---------------------------------------------------------------------
	// SetExitCode
	// This function is used to change the final exit code for the service
	//---------------------------------------------------------------------
	void SetExitCode(DWORD dwExitCode) { Status_.dwWin32ExitCode = dwExitCode; }
	//---------------------------------------------------------------------
	// GetExitCode
	// This returns the currently set return code for the service.
	//---------------------------------------------------------------------
	DWORD GetExitCode() const { return Status_.dwWin32ExitCode; }
	//---------------------------------------------------------------------
	// GetStopEvent
	// This function returns the event which is used as the stop handle.
	//---------------------------------------------------------------------
	EventSynch& GetStopEvent() { return isStopping_; } //lint !e1536
	//---------------------------------------------------------------------
	// IsService
	// This function returns whether we are running as a service.  If it 
	// returns false, it indicates that we are being run as a console app.
	//---------------------------------------------------------------------
	bool IsService() const { return isService_; }
	//---------------------------------------------------------------------
	// RunningUnderSystemAccount
	// This function returns whether we are running under the SYSTEM 
	// privileged account.
	//---------------------------------------------------------------------
	static bool RunningUnderSystemAccount();
	//---------------------------------------------------------------------
	// IsRunning
	// This function returns whether we are running (not stopped).
	//---------------------------------------------------------------------
	bool IsRunning() const { return isRunning_; }
	//---------------------------------------------------------------------
	// GetServiceStatus
	// This function queries the current service status
	//---------------------------------------------------------------------
	DWORD GetServiceStatus() const { return Status_.dwCurrentState; }
	//---------------------------------------------------------------------
	// SetServiceStatus
	// This function is used to change the current state of the running service.
	//---------------------------------------------------------------------
	void SetServiceStatus(DWORD dwState);
	//---------------------------------------------------------------------
	// LogEvent
	// These functions are used to report events in the NT Event Log.
	//---------------------------------------------------------------------
	void LogEvent(WORD wEvent, DWORD dwEventID) const;
	void LogEvent(WORD wEvent, DWORD dwEventID, LPCTSTR pszText1) const;
	void LogEvent(WORD wEvent, DWORD dwEventID, LPCTSTR pszText1, LPCTSTR pszText2) const;
	void LogEvent(WORD wEvent, DWORD dwEventID, LPCTSTR pszText1, LPCTSTR pszText2, LPCTSTR pszText3) const;
	void LogEvent(WORD wEvent, DWORD dwEventID, LPCTSTR pszText1, LPCTSTR pszText2, LPCTSTR pszText3, LPCTSTR pszText4) const;
	void LogEvent(WORD wEvent, DWORD dwEventID, LPCTSTR pszText1, LPCTSTR pszText2, LPCTSTR pszText3, LPCTSTR pszText4, LPCTSTR pszText5) const;

// Utility functions (in both Unicode and Ansi form)
public:
	//---------------------------------------------------------------------
	// GetResourceInstance
	// This function returns the instance handle for the service
	//---------------------------------------------------------------------
	HINSTANCE GetResourceInstance() const { return ::GetModuleHandle(NULL);	}
	
	//---------------------------------------------------------------------
	// LoadString
	// These loads a string resource from the service.
	//---------------------------------------------------------------------
	std::string LoadStringA(UINT nResource) const;
	std::wstring LoadStringW(UINT nResource) const;

	//---------------------------------------------------------------------
	// GetLastErrorMessage
	// This function can be used to convert the last error code into a
	// textual message.
	//---------------------------------------------------------------------
	static std::string GetLastErrorMessageA(DWORD dwError = 0);
	static std::wstring GetLastErrorMessageW(DWORD dwError = 0);

// Internal functions
protected:
	void InternalRun();
	void InternalLogEvent(WORD wType, DWORD dwEventID, WORD nCount, LPCTSTR* ppszBuffers) const;
	void ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);

// Required overrides
protected:
	virtual SERVICE_TABLE_ENTRY* GetServiceTable() const = 0;
	virtual void SetupConsoleHandler() = 0;
	virtual LPHANDLER_FUNCTION GetFunctionHandler() const = 0;

// Locked up methods
private:
	ServiceBase(const ServiceBase&);
	ServiceBase& operator=(const ServiceBase&);
};

/****************************************************************************/
// ServiceInstance
//
// This class models the typed service instance which is derived from
// in order to create a concrete class.
//
/****************************************************************************/
//lint --e{1704} Private constructor ok
template <class _Ty, class LockType = JTI_SERVICE_LOCK_TYPE>
class ServiceInstance : 
	public ServiceBase,
	public LockableObject<LockType>
{
// Constructor
public:
	ServiceInstance(const char* pszServiceName, const char* pszDisplayName, DWORD dwStartTime=0) :
		  ServiceBase(pszServiceName, pszDisplayName, dwStartTime) {JTI_ASSERT(g_pThis == NULL); g_pThis = (_Ty*)this;}
	ServiceInstance(const wchar_t* pszServiceName, const wchar_t* pszDisplayName, DWORD dwStartTime=0) : 
		  ServiceBase(pszServiceName, pszDisplayName, dwStartTime) {JTI_ASSERT(g_pThis == NULL); g_pThis = (_Ty*)this;}
	virtual ~ServiceInstance() {g_pThis = NULL;}

// Singleton accessor
public:
	static _Ty& Instance() { 
		JTI_ASSERT(g_pThis != NULL);
		return *g_pThis; 
	}

// Static methods for callback
protected:
	static void WINAPI _Handler(DWORD dwOpcode) 
	{
		_Ty& This = ServiceInstance<_Ty,LockType>::Instance();
		switch (dwOpcode)
		{
			case SERVICE_CONTROL_STOP:	This.Stop(); break;
			case SERVICE_CONTROL_PAUSE:	This.Pause(); break;
			case SERVICE_CONTROL_CONTINUE: This.Continue(); break;
			case SERVICE_CONTROL_INTERROGATE: This.Interrogate(); break;
			case SERVICE_CONTROL_SHUTDOWN: This.Shutdown(); break;
			default: break;
		}
	}
	static BOOL WINAPI _ConsoleHandler(DWORD dwCtrlType) 
	{
		// If the user shuts down, logs off, closes the window or hits CTRL+C then exit.
		if (dwCtrlType == CTRL_SHUTDOWN_EVENT || dwCtrlType == CTRL_LOGOFF_EVENT || dwCtrlType == CTRL_CLOSE_EVENT ||
			dwCtrlType == CTRL_BREAK_EVENT || dwCtrlType == CTRL_C_EVENT) {
			ServiceInstance<_Ty,LockType>::Instance().Stop();
			return TRUE;
		}
		return FALSE;
	}
	static void WINAPI _ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv) 
	{ 
		ServiceInstance<_Ty,LockType>::Instance().ServiceMain(dwArgc, lpszArgv); 
	}

// Required overrides
protected:
	virtual SERVICE_TABLE_ENTRY* GetServiceTable() const
	{
        static SERVICE_TABLE_ENTRY st[] = { 
			{ const_cast<LPTSTR>(ServiceName_.c_str()), _ServiceMain }, 
			{ NULL, NULL } 
		};
		return st;
	}

	virtual LPHANDLER_FUNCTION GetFunctionHandler() const
	{
		return _Handler; 
	}
		
	virtual void SetupConsoleHandler()
	{
		SetConsoleCtrlHandler(_ConsoleHandler, TRUE);
	}

// Locked up methods
private:
	ServiceInstance(const ServiceInstance&);
	ServiceInstance& operator=(const ServiceInstance&);
// Class data
private:
	static _Ty* g_pThis;
};

/****************************************************************************/
// ServiceInstance globals
/****************************************************************************/
template <class _Ty, class LockType>
_Ty* ServiceInstance<_Ty, LockType>::g_pThis;

} // namespace JTI_Util

#endif // __SERVICESUPP_H_INCL__
