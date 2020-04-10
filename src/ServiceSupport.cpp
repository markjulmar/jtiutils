/****************************************************************************/
//
// ServiceSupport.cpp
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

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#include "stdafx.h"
#include <iostream>
#include "JTIUtils.h"
#include "EventLog.h"
#include "ServiceSupport.h"
#include <LMCons.h>

namespace JTI_Util
{
/*----------------------------------------------------------------------------
    GLOBALS
-----------------------------------------------------------------------------*/
typedef BOOL (CALLBACK *CHANGESERVICECONFIG2)(SC_HANDLE hService, DWORD dwInfoLevel, LPVOID lpInfo);

/**************************************************************************************
** Procedure: ServiceBase::ServiceBase
**
** Arguments: 'pszServiceName' - Name of the service
**            'pszDisplayName' - Display name for the service
**
** Returns: void
**
** Description: Constructor for the service
**
**************************************************************************************/
ServiceBase::ServiceBase(const char* pszServiceName, const char* pszDisplayName, DWORD dwStartTime) : 
	ServiceName_(_T("")), DisplayName_(_T("")), ServiceHandle_(NULL), isStopping_(false,true), isService_(false), isRunning_(false)
{
	ServiceName_ = A2T(pszServiceName);
	DisplayName_ = A2T(pszDisplayName);

	Status_.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	Status_.dwCurrentState = SERVICE_STOPPED;
	Status_.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	Status_.dwWin32ExitCode = 0;
	Status_.dwCheckPoint = 0;
	Status_.dwWaitHint = dwStartTime;
	Status_.dwServiceSpecificExitCode = 0;

}// ServiceBase::ServiceBase

/**************************************************************************************
** Procedure: ServiceBase::ServiceBase
**
** Arguments: 'pszServiceName' - Name of the service
**            'pszDisplayName' - Display name for the service
**
** Returns: void
**
** Description: Constructor for the service
**
**************************************************************************************/
ServiceBase::ServiceBase(const wchar_t* pszServiceName, const wchar_t* pszDisplayName, DWORD dwStartTime) : 
	ServiceName_(_T("")), DisplayName_(_T("")), ServiceHandle_(NULL), isStopping_(false,true), isService_(false), isRunning_(false)
{
	ServiceName_ = W2T(pszServiceName);
	DisplayName_ = W2T(pszDisplayName);

	Status_.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	Status_.dwCurrentState = SERVICE_STOPPED;
	Status_.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	Status_.dwWin32ExitCode = 0;
	Status_.dwCheckPoint = 0;
	Status_.dwWaitHint = dwStartTime;
	Status_.dwServiceSpecificExitCode = 0;

}// ServiceBase::ServiceBase

/**************************************************************************************
** Procedure: ServiceBase::ServiceStart
**
** Arguments: void
**
** Returns: void
**
** Description: Static function whih Asks the SCM to start the service process.
**
**************************************************************************************/
DWORD ServiceBase::ServiceStart() const
{
	return ServiceBase::StartService(ServiceName().c_str());

}// ServiceBase::ServiceStart

/**************************************************************************************
** Procedure: ServiceBase::ServiceStop
**
** Arguments: void
**
** Returns: void
**
** Description: Asks the service manager to start the service.
**
**************************************************************************************/
DWORD ServiceBase::ServiceStop() const
{
	return ServiceBase::StopService(ServiceName().c_str());

}// ServiceBase::ServiceStop

/**************************************************************************************
** Procedure: ServiceBase::IsInstalled
**
** Arguments: void
**
** Returns: TRUE/FALSE whether service is installed in the SCM.
**
** Description: This checks with the NT service control manager to determine if the
**              service is currently installed.
**
**************************************************************************************/
bool ServiceBase::IsInstalled() const
{
    bool bResult = false;
	SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM != NULL)
	{
		// Get the shortname for the service.
		SC_HANDLE hService = OpenService(hSCM, ServiceName().c_str(), SERVICE_QUERY_CONFIG);
        if (hService != NULL)
		{
            bResult = true;
            CloseServiceHandle(hService); //lint !e534
        }
        CloseServiceHandle(hSCM); //lint !e534
    }
    return bResult;

}// ServiceBase::IsInstalled

/**************************************************************************************
** Procedure: ServiceBase::Install
**
** Arguments: void
**
** Returns: TRUE/FALSE whether service is installed in the SCM.
**
** Description: This installs the service into the SCM.
**
**************************************************************************************/
DWORD ServiceBase::Install(const TCHAR* pszUserID, const TCHAR* pszPassword, bool fAutoStart, const TCHAR* pszDepends, LPCTSTR pszDescription) const
{
	DWORD lastError = 0;

	// Open the Service Control Manager
	bool fInstalled = false;
	SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM != NULL)
	{
		// Get the executable file path
		TCHAR szFilePath[_MAX_PATH];
		GetModuleFileName(NULL, szFilePath, _MAX_PATH); //lint !e534

		// If the service is already installed, then just change it's configuration
		SC_HANDLE hService = NULL;

		if (IsInstalled())
		{
			hService = OpenService(hSCM, ServiceName().c_str(), SERVICE_ALL_ACCESS);
			if (!ChangeServiceConfig(hService, SERVICE_WIN32_OWN_PROCESS,
				(fAutoStart) ? SERVICE_AUTO_START :	SERVICE_DEMAND_START, 
				SERVICE_ERROR_NORMAL, szFilePath,
				NULL, NULL,	pszDepends, pszUserID, pszPassword, DisplayName().c_str()))
			{
				lastError = GetLastError();
				CloseServiceHandle(hService); //lint !e534
				hService = NULL;
			}
		}
		else
		{
			hService = CreateService(hSCM, ServiceName().c_str(), DisplayName().c_str(), 
				SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
				(fAutoStart) ? SERVICE_AUTO_START :	SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,	szFilePath, 
				NULL, NULL,	pszDepends, pszUserID, pszPassword);
		}

		if (hService != NULL)
		{
			// If a description is given, then set that in place.
			if (pszDescription != NULL)
			{
				HMODULE hAdvApi = LoadLibraryA("ADVAPI32.DLL");
				if (hAdvApi != NULL)
				{
#ifdef _UNICODE
					CHANGESERVICECONFIG2 pcsc = (CHANGESERVICECONFIG2)GetProcAddress(hAdvApi, "ChangeServiceConfig2W");
#else
					CHANGESERVICECONFIG2 pcsc = (CHANGESERVICECONFIG2)GetProcAddress(hAdvApi, "ChangeServiceConfig2A");
#endif
					if (pcsc != NULL)
					{
						SERVICE_DESCRIPTION sdBuf;
						sdBuf.lpDescription = const_cast<LPTSTR>(pszDescription);
						if (!pcsc(hService, SERVICE_CONFIG_DESCRIPTION, &sdBuf))
							lastError = GetLastError();
					}
					FreeLibrary(hAdvApi);
				}
			}
			CloseServiceHandle(hService); //lint !e534
			fInstalled = true;
		}
		else if (lastError == 0)
			lastError = GetLastError();
		CloseServiceHandle(hSCM); //lint !e534
	}

	if (fInstalled)
	{
		if (!AddEventLogSupport())
			lastError = GetLastError();
		else
			lastError = 0;
	}

	return lastError;

}// ServiceBase::Install

/**************************************************************************************
** Procedure: ServiceBase::SetServerFailureAction
**
** Arguments: sfa - SERVICE_FAILURE_ACTIONS structure
**
** Returns: TRUE/FALSE whether service had failure action set
**
** Description: This changes the failure action for the service
**
**************************************************************************************/
DWORD ServiceBase::SetServerFailureAction(SERVICE_FAILURE_ACTIONS* psfa)
{
	DWORD lastError = ERROR_BAD_COMMAND;
	HMODULE hAdvApi = LoadLibraryA("ADVAPI32.DLL");
	if (hAdvApi == NULL)
		return lastError;

#ifdef _UNICODE
	CHANGESERVICECONFIG2 pcsc = (CHANGESERVICECONFIG2)GetProcAddress(hAdvApi, "ChangeServiceConfig2W");
#else
	CHANGESERVICECONFIG2 pcsc = (CHANGESERVICECONFIG2)GetProcAddress(hAdvApi, "ChangeServiceConfig2A");
#endif

	if (pcsc != NULL)
	{
		SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (hSCM != NULL)
		{
			// Get the shortname for the service.
			SC_HANDLE hService = OpenService(hSCM, ServiceName().c_str(), SERVICE_ALL_ACCESS);
			if (hService != NULL)
			{
				if (!pcsc(hService, SERVICE_CONFIG_FAILURE_ACTIONS, psfa))
					lastError = GetLastError();
				else
					lastError = 0;
				CloseServiceHandle(hService); //lint !e534
			}
			CloseServiceHandle(hSCM); //lint !e534
		}
	}
	FreeLibrary(hAdvApi);
	return lastError;

}// ServiceBase::SetServerFailureAction

/**************************************************************************************
** Procedure: ServiceBase::Uninstall
**
** Arguments: void
**
** Returns: TRUE/FALSE whether service is uninstalled from the SCM.
**
** Description: This removes the service from the SCM list.
**
**************************************************************************************/
DWORD ServiceBase::Uninstall() const
{
	DWORD lastError = 0;

	// If it is not currently installed, then we are done.
	if (!IsInstalled())
		return lastError;

	// Open the service and ask it to shutdown if it is running.
	bool fDeleted = false;
	SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM != NULL)
	{
		SC_HANDLE hService = OpenService(hSCM, ServiceName().c_str(), SERVICE_STOP | DELETE);
		if (hService != NULL)
		{
			// Ask the service to stop running.
			SERVICE_STATUS status;
			if (!ControlService(hService, SERVICE_CONTROL_STOP, &status))
				lastError = GetLastError();

			// Now delete the service.
			fDeleted = (DeleteService(hService) == TRUE);
			if (fDeleted)
				RemoveApplicationEventLog();
			else
				lastError = GetLastError();

			CloseServiceHandle(hService); //lint !e534
		}
		CloseServiceHandle(hSCM); //lint !e534
	}
	
	if (fDeleted)
		lastError = 0;

	return lastError;

}// ServiceBase::Uninstall

/**************************************************************************************
** Procedure: ServiceBase::AddApplicationEventLog
**
** Arguments: 'pszMessageFile' - The message file to add
**            'dwType' - Then event type to register
**
** Returns: void
**
** Description: Writes the key into the registry to allow this application
**              to log entries into the system event log.
**
**************************************************************************************/
bool ServiceBase::AddApplicationEventLog(const TCHAR* pszMessageFile, DWORD dwType) const
{	
	try
	{
		EventLog log;
		log.Source = ServiceName_.c_str();
		log.CreateEventSource(pszMessageFile, dwType);
	}
	catch(const EventLogException&)
	{
		return false;
	}

	return true;

}// ServiceBase::AddApplicationEventLog

/**************************************************************************************
** Procedure: ServiceBase::RemoveApplicationEventLog
**
** Arguments: void
**
** Returns: void
**
** Description: Removes all the keys from the registry related to our event log.
**
**************************************************************************************/
void ServiceBase::RemoveApplicationEventLog() const
{
	try
	{
		EventLog::DeleteEventSource(ServiceName_.c_str());
	}
	catch(const EventLogException&)
	{
	}

}// ServiceBase::RemoveApplicationEventLog

/**************************************************************************************
** Procedure: ServiceBase::InternalRun
**
** Arguments: void
**
** Returns: void
**
** Description: Internal run loop for the service.
**
**************************************************************************************/
void ServiceBase::InternalRun()
{
	// Mark us as running
	isRunning_ = true;

	// Pass control to the derived class to do the running.
	Run();	//lint !e1933

	// Tell the SCM we are stopping.
	DWORD currState = GetServiceStatus();
	if (currState != SERVICE_STOP_PENDING &&
		currState != SERVICE_STOPPED)
		SetServiceStatus(SERVICE_STOP_PENDING);

	// The Run() function should only return when stopped.
	isRunning_ = false;

}// ServiceBase::InternalRun

/**************************************************************************************
** Procedure: ServiceBase::InternalLogEvent
**
** Arguments: 'pFormat' - Variable format list
**
** Returns: void
**
** Description: Writes a line to the event log for this service.
**
**************************************************************************************/
void ServiceBase::InternalLogEvent(WORD wType, DWORD dwEventID, WORD nCount, LPCTSTR* ppszBuffers) const
{
	if (IsService())
	{
	    // Get a handle to use with ReportEvent.
		HANDLE hEventSource = RegisterEventSource(NULL, ServiceName_.c_str());
		if (hEventSource != NULL)
	    {
	        // Write to event log.
			ReportEvent(hEventSource,
                wType,				  // event type
                0,                    // event category
                dwEventID,			  // event ID
                NULL,                 // current user's SID
                nCount,               // strings in lpszStrings
                0,                    // no bytes of raw data
                ppszBuffers,		  // array of error strings
                NULL);                //lint !e534
			DeregisterEventSource(hEventSource); //lint !e534
	    }
	}

}// ServiceBase::InternalLogEvent

/**************************************************************************************
** Procedure: ServiceBase::LoadStringA
**
** Arguments: 'nResource' - Resource ID to load
**
** Returns: string
**
** Description: Loads a specific resource string
**
**************************************************************************************/
std::string ServiceBase::LoadStringA(UINT nResource) const 
{
	return std::string(W2A(LoadStringW(nResource).c_str()));
}

/**************************************************************************************
** Procedure: ServiceBase::LoadStringW
**
** Arguments: 'nResource' - Resource ID to load
**
** Returns: string
**
** Description: Loads a specific resource string
**
**************************************************************************************/
std::wstring ServiceBase::LoadStringW(UINT nResource) const 
{
	wchar_t chBuff[MAX_PATH];
	::LoadStringW(GetResourceInstance(), nResource, chBuff, sizeofarray(chBuff)); //lint !e534
	return std::wstring(chBuff);

}// ServiceBase::LoadStringW

/**************************************************************************************
** Procedure: ServiceBase::GetLastErrorMessageA
**
** Arguments: 'dwError' - Optional error
**
** Returns: string
**
** Description: Returns a test message for the given HR
**
**************************************************************************************/
std::string ServiceBase::GetLastErrorMessageA(DWORD dwError)
{
	return std::string(W2A(GetLastErrorMessageW(dwError).c_str()));

}// ServiceBase::GetLastErrorMessageA

/**************************************************************************************
** Procedure: ServiceBase::GetLastErrorMessageW
**
** Arguments: 'dwError' - Optional error
**
** Returns: string
**
** Description: Returns a test message for the given HR
**
**************************************************************************************/
std::wstring ServiceBase::GetLastErrorMessageW(DWORD err)
{
	// Get the current error code
	if (err == 0) err = GetLastError();

	std::wstring sError;
	LPWSTR pMsgBuff = NULL;
	if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_ENGLISH_US),
		(LPWSTR)&pMsgBuff, 0, NULL))
	{
		sError = pMsgBuff;
		LocalFree(pMsgBuff);
	}
	return sError;

}// ServiceBase::GetLastErrorMessageW

/**************************************************************************************
** Procedure: ServiceBase::LogEvent
**
** Arguments: 'wEvent' - Event type
**            'dwEventID' - Event ID
**
** Returns: void
**
** Description: This logs an event to the system event log
**
**************************************************************************************/
void ServiceBase::LogEvent(WORD wEvent, DWORD dwEventID) const 
{ 
	InternalLogEvent(wEvent,dwEventID,0,NULL); 

}// ServiceBase::LogEvent

/**************************************************************************************
** Procedure: ServiceBase::LogEvent
**
** Arguments: 'wEvent' - Event type
**            'dwEventID' - Event ID
**            'pszText..' - Text for the event message
**
** Returns: void
**
** Description: This logs an event to the system event log
**
**************************************************************************************/
void ServiceBase::LogEvent(WORD wEvent, DWORD dwEventID, LPCTSTR pszText1) const 
{ 
	InternalLogEvent(wEvent,dwEventID,1,&pszText1); 

}// ServiceBase::LogEvent

/**************************************************************************************
** Procedure: ServiceBase::LogEvent
**
** Arguments: 'wEvent' - Event type
**            'dwEventID' - Event ID
**            'pszText..' - Text for the event message
**
** Returns: void
**
** Description: This logs an event to the system event log
**
**************************************************************************************/
void ServiceBase::LogEvent(WORD wEvent, DWORD dwEventID, LPCTSTR pszText1, LPCTSTR pszText2) const 
{
	LPCTSTR parrBuff[] = { pszText1, pszText2 }; 
	InternalLogEvent(wEvent,dwEventID,2,parrBuff); 

}// ServiceBase::LogEvent

/**************************************************************************************
** Procedure: ServiceBase::LogEvent
**
** Arguments: 'wEvent' - Event type
**            'dwEventID' - Event ID
**            'pszText..' - Text for the event message
**
** Returns: void
**
** Description: This logs an event to the system event log
**
**************************************************************************************/
void ServiceBase::LogEvent(WORD wEvent, DWORD dwEventID, LPCTSTR pszText1, LPCTSTR pszText2, LPCTSTR pszText3) const 
{
	LPCTSTR parrBuff[] = { pszText1, pszText2, pszText3 }; 
	InternalLogEvent(wEvent,dwEventID,3,parrBuff); 

}// ServiceBase::LogEvent

/**************************************************************************************
** Procedure: ServiceBase::LogEvent
**
** Arguments: 'wEvent' - Event type
**            'dwEventID' - Event ID
**            'pszText..' - Text for the event message
**
** Returns: void
**
** Description: This logs an event to the system event log
**
**************************************************************************************/
void ServiceBase::LogEvent(WORD wEvent, DWORD dwEventID, LPCTSTR pszText1, LPCTSTR pszText2, LPCTSTR pszText3, LPCTSTR pszText4) const 
{ 
	LPCTSTR parrBuff[] = { pszText1, pszText2, pszText3, pszText4 }; 
	InternalLogEvent(wEvent,dwEventID,4,parrBuff);	

}// ServiceBase::LogEvent

/**************************************************************************************
** Procedure: ServiceBase::LogEvent
**
** Arguments: 'wEvent' - Event type
**            'dwEventID' - Event ID
**            'pszText..' - Text for the event message
**
** Returns: void
**
** Description: This logs an event to the system event log
**
**************************************************************************************/
void ServiceBase::LogEvent(WORD wEvent, DWORD dwEventID, LPCTSTR pszText1, LPCTSTR pszText2, LPCTSTR pszText3, LPCTSTR pszText4, LPCTSTR pszText5) const 
{ 
	LPCTSTR parrBuff[] = { pszText1, pszText2, pszText3, pszText4, pszText5 }; 
	InternalLogEvent(wEvent,dwEventID,5,parrBuff); 

}// ServiceBase::LogEvent

/**************************************************************************************
** Procedure: ServiceBase::SetServiceStatus
**
** Arguments: 'dwState' - New service state
**
** Returns: void
**
** Description: This changes the state of the service according to the SCM.
**
**************************************************************************************/
void ServiceBase::SetServiceStatus(DWORD dwState) 
{
	// Update the state/checkpoint
	if (Status_.dwCurrentState == dwState)
		++Status_.dwCheckPoint;
	else
	{
		Status_.dwCurrentState = dwState;
		Status_.dwCheckPoint = 0;
	}

	if (IsService())
		::SetServiceStatus(ServiceHandle_, &Status_); //lint !e534

}// ServiceBase::SetServiceStatus

/**************************************************************************************
** Procedure: ServiceBase::Stop
**
** Arguments: void
**
** Returns: void
**
** Description: This begins the orderly shutdown of the service ordered by the SCM
**
**************************************************************************************/
void ServiceBase::Stop() 
{ 
	SetServiceStatus(SERVICE_STOP_PENDING); 
	isStopping_.SetEvent(); 

}// ServiceBase::Stop

/**************************************************************************************
** Procedure: ServiceBase::Pause
**
** Arguments: void
**
** Returns: void
**
** Description: This is called when the SCM wants to pause the service
**
**************************************************************************************/
void ServiceBase::Pause()
{
}// ServiceBase::Pause

/**************************************************************************************
** Procedure: ServiceBase::Continue
**
** Arguments: void
**
** Returns: void
**
** Description: This is called to un-pause the service
**
**************************************************************************************/
void ServiceBase::Continue()
{
}// ServiceBase::Continue

/**************************************************************************************
** Procedure: ServiceBase::Interrogate
**
** Arguments: void
**
** Returns: void
**
** Description: This is called to interrogate the service
**
**************************************************************************************/
void ServiceBase::Interrogate()
{
}// ServiceBase::Interrogate

/**************************************************************************************
** Procedure: ServiceBase::Shutdown
**
** Arguments: void
**
** Returns: void
**
** Description: This can be called by the service to initiate a shutdown
**
**************************************************************************************/
void ServiceBase::Shutdown()
{
	Stop();

}// ServiceBase::Shutdown

/**************************************************************************************
** Procedure: ServiceBase::Start
**
** Arguments: 'fIsService' - true to initiate connection to SCM
**
** Returns: void
**
** Description: This starts the service
**
**************************************************************************************/
void ServiceBase::Start(bool fIsService) 
{
	// Save off our "service" status
	isService_ = fIsService;

	// Add our service to the running SCM list; this will cause a callback to ServiceMain above.
	if (isService_)
	{
		if (StartServiceCtrlDispatcher(GetServiceTable()) == FALSE)
		{
			SetExitCode(GetLastError());
			JTI_TRACE("Service failed to start." << std::endl << std::hex << std::showbase << GetLastError() << " " << GetLastErrorMessageA());
			return;
		}
		// SCM will call back through ServiceMain..
	}
	
	// If we are simply an EXE, then run.
	else if (!isService_)
	{
		SetupConsoleHandler();
		InternalRun();
	}

}// ServiceBase::Start

/**************************************************************************************
** Procedure: ServiceBase::ServiceMain
**
** Arguments: 'dwArgc' - Argc
**            'lpszArgv' - Argv
**
** Returns: void
**
** Description: This is the main entry point for the service
**
**************************************************************************************/
void ServiceBase::ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
	// Parse the parameters first
	if (!ParseServiceStartupParameters(dwArgc,lpszArgv))
		return;

	// Register the control request handler
	Status_.dwCurrentState = SERVICE_START_PENDING;
	ServiceHandle_ = RegisterServiceCtrlHandler(ServiceName_.c_str(), GetFunctionHandler());
	if (ServiceHandle_ == NULL)
	{
		JTI_TRACE("Service Control Handler not installed, rc=" << std::showbase << std::hex << GetLastError() << "; cannot start service.");
		return;
	}

	// Mark us as "pending" start.
	SetServiceStatus(SERVICE_START_PENDING);
	Status_.dwWin32ExitCode = S_OK;
	Status_.dwCheckPoint = 0;
	Status_.dwWaitHint = 0;

	// When the Run function returns, the service has stopped.
	InternalRun();
	if (GetServiceStatus() != SERVICE_STOPPED)
		SetServiceStatus(SERVICE_STOPPED);

}// ServiceBase::ServiceMain

/**************************************************************************************
** Procedure: ServiceBase::StartService
**
** Arguments: pszServiceName - name of the service to start
**
** Returns: Win32 error code
**
** Description: This method starts the named service
**
**************************************************************************************/
DWORD ServiceBase::StartService(LPCTSTR pszServiceName, DWORD dwMaxWaitTime)
{
	bool fStarted = false;
	DWORD lastErr = 0;

	SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM != NULL)
	{
		SC_HANDLE hService = OpenService(hSCM, pszServiceName, SERVICE_ALL_ACCESS);
		if (hService == NULL)
			lastErr = GetLastError();
		else
		{
			if (::StartService(hService, 0, NULL))
			{
				SERVICE_STATUS sStatus;
				if (QueryServiceStatus(hService, &sStatus))
				{
					DWORD dwStartTickCount = GetTickCount();
					DWORD dwOldCheckPoint = sStatus.dwCheckPoint;

					while (sStatus.dwCurrentState == SERVICE_START_PENDING)
					{
						DWORD dwWaitTime = sStatus.dwWaitHint / 10;
						if (dwWaitTime < 1000)
							dwWaitTime = 1000;
						else if (dwWaitTime > dwMaxWaitTime)
							dwWaitTime = dwMaxWaitTime;

						Sleep(dwWaitTime);

						// Check the status again. 
						if (!QueryServiceStatus(hService, &sStatus))
							break;

						if (sStatus.dwCheckPoint > dwOldCheckPoint)
						{
							dwStartTickCount = GetTickCount();
							dwOldCheckPoint = sStatus.dwCheckPoint;
						}
						else if (ELAPSED_TIME(dwStartTickCount) > sStatus.dwWaitHint)
							break;
					}
					fStarted = (sStatus.dwCurrentState == SERVICE_RUNNING);
				}
				else
					lastErr = GetLastError();
			}
			else
				lastErr = GetLastError();

			CloseServiceHandle(hService);	//lint !e534
		}
		CloseServiceHandle(hSCM); //lint !e534
	}
	else
		lastErr = GetLastError();

	if (fStarted == true)
		lastErr = 0;
	else if (lastErr == 0)
		lastErr = ERROR_SERVICE_REQUEST_TIMEOUT;

	return lastErr;

}//  ServiceBase::StartService

/**************************************************************************************
** Procedure: ServiceBase::StopService
**
** Arguments: pszServiceName - name of the service to stop
**
** Returns: Win32 error code
**
** Description: This method stops the named service
**
**************************************************************************************/
DWORD ServiceBase::StopService(LPCTSTR pszServiceName, DWORD dwMaxWaitTime)
{
	bool fStopped = false;
	DWORD lastErr = 0;

	SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM != NULL)
	{
		SC_HANDLE hService = OpenService(hSCM, pszServiceName, SERVICE_ALL_ACCESS);
		if (hService == NULL)
			lastErr = GetLastError();
		else
		{
			SERVICE_STATUS sStatus;
			fStopped = (ControlService(hService, SERVICE_CONTROL_STOP, &sStatus) == TRUE);
			if (fStopped)
			{
				for (int i = 0; i < static_cast<int>(dwMaxWaitTime / 1000); ++i)
				{
					if (!QueryServiceStatus(hService, &sStatus)) {
						lastErr = GetLastError();
						break;
					}
					if (sStatus.dwCurrentState != SERVICE_STOP_PENDING)
						break;
					::Sleep(1000);
				}
				fStopped = (sStatus.dwCurrentState == SERVICE_STOPPED);
			}
			else
				lastErr = GetLastError();

			CloseServiceHandle(hService); //lint !e534
		}
		CloseServiceHandle(hSCM); //lint !e534
	}
	else
		lastErr = GetLastError();

	if (fStopped == true)
		lastErr = 0;
	else if (lastErr == 0)
		lastErr = ERROR_SERVICE_REQUEST_TIMEOUT;

	return lastErr;

}// ServiceBase::StopService

/**************************************************************************************
** Procedure: ServiceBase::EnumServices
**
** Arguments: dwCount - returning count
**            arrDataOut - returning array of data
**
** Returns: void
**
** Description: This method enumerates the installed services
**
**************************************************************************************/
DWORD ServiceBase::EnumServices(DWORD dwType, DWORD dwState, DWORD& dwCount, std::vector<BYTE>& arrDataOut)
{
	DWORD dwLastError = S_OK;
	SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM != NULL)
	{
		// First determine how much memory we need to get the whole list.
		DWORD dwSize = 0;
		EnumServicesStatus(hSCM, dwType, dwState, NULL, 0, &dwSize, &dwCount, NULL);

		dwLastError = GetLastError();
		if (dwLastError == ERROR_MORE_DATA && dwSize > 0)
		{
			std::vector<BYTE> arrData; arrData.reserve(dwSize+1);
			ENUM_SERVICE_STATUS* pStatus = reinterpret_cast<ENUM_SERVICE_STATUS*>(&arrData[0]);

			// Get the data
			if (EnumServicesStatus(hSCM, dwType, dwState, pStatus, dwSize, &dwSize, &dwCount, NULL))
			{
				arrDataOut.swap(arrData);
				dwLastError = 0;
			}
			else
			{
				dwCount = 0;
				dwLastError = GetLastError();
			}
		}
		CloseServiceHandle(hSCM);
	}
	return dwLastError;

}// ServiceBase::EnumServices

/**************************************************************************************
** Procedure: ServiceBase::RunningUnderSystemAccount
**
** Arguments: void
**
** Returns: true/false
**
** Description: This method determines whether we are running under the SYSTEM account.
**
**************************************************************************************/
bool ServiceBase::RunningUnderSystemAccount()
{	
	TCHAR chName[UNLEN + 1]; DWORD dwLength = sizeofarray(chName);
	return (GetUserName(chName, &dwLength) == TRUE && lstrcmpi(chName, _T("SYSTEM")) == 0);

}// ServiceBase::RunningUnderSystemAccount

} // namespace JTI_Util
