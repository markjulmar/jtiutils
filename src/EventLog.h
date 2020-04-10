/******************************************************************************/
//                                                                        
// EventLog.h
//                                             
// This header implements a class for wrapping the NT event log manager.
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
//                                                                        
/******************************************************************************/

#ifndef __EVENTLOG_H_INCL__
#define __EVENTLOG_H_INCL__

/*****************************************************************************/
// PC-Lint options
//
//lint -save
//
// Supress duplicate header include files
//lint -e537
//
/*****************************************************************************/

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <process.h>
#include <JTIUtils.h>
#include <DateTime.h>
#include <Delegates.h>
#include <Registry.h>

#pragma warning(disable:4706) // assignment within conditional expression

namespace JTI_Util
{
/******************************************************************************/
// EventLogException
//
// This exception is thrown from the EventLog classes when any error occurs.
//
/******************************************************************************/
class EventLogException : public std::runtime_error
{
// Class data
private:
	DWORD errCode_;

// Constructor
public:
	EventLogException(const std::string& s, DWORD rc=-1) : 
		std::runtime_error(s), errCode_(rc) { if (errCode_ == -1) errCode_ = GetLastError(); }

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
// EventLogEntryType
//
// These are the types of event log entries.
//
/******************************************************************************/
enum EventLogEntryType
{
	Error			= EVENTLOG_ERROR_TYPE,
	FailureAudit	= EVENTLOG_AUDIT_FAILURE,
	Information		= EVENTLOG_INFORMATION_TYPE,
	SuccessAudit	= EVENTLOG_AUDIT_SUCCESS,
	Warning			= EVENTLOG_WARNING_TYPE
};

/******************************************************************************/
// EventLogEntry
//
// The EventLogEntry encapsulates a single entry in the event log.  It has
// properties and methods to retrieve various bits of information about the
// entry.
//
/******************************************************************************/
class EventLogEntry
{
// Properties
public:
	__declspec(property(get=get_CategoryText)) tstring Category;
	__declspec(property(get=get_Category)) WORD CategoryNumber;
	__declspec(property(get=get_Data)) const void* Data;
	__declspec(property(get=get_EntryType)) EventLogEntryType EntryType;
	__declspec(property(get=get_EventID)) int EventID;
	__declspec(property(get=get_Index)) int Index;
	__declspec(property(get=get_MachineName)) const tstring& MachineName;
	__declspec(property(get=get_Message)) tstring Message;
	__declspec(property(get=get_ReplacementStrings)) const std::vector<tstring>& ReplacementStrings;
	__declspec(property(get=get_Source)) const tstring& Source;
	__declspec(property(get=get_TimeGenerated)) const DateTime& TimeGenerated;
	__declspec(property(get=get_TimeWritten)) const DateTime& TimeWritten;
	__declspec(property(get=get_UserName)) const tstring& UserName;

// Constructor
public:
	EventLogEntry(const EventLogEntry& rhs) :
		eventID_(rhs.eventID_),index_(rhs.index_),eventCategory_(rhs.eventCategory_),
		entryType_(rhs.entryType_),machineName_(rhs.machineName_), logName_(rhs.logName_),
		messageSource_(rhs.messageSource_),userSID_(rhs.userSID_),dtGenerated_(rhs.dtGenerated_),
		dtWritten_(rhs.dtWritten_),	arrData_(rhs.arrData_),	arrReplacements_(rhs.arrReplacements_) {/* */}
	~EventLogEntry() {/* */}

// Operators
public:
	EventLogEntry& operator=(const EventLogEntry& rhs)
	{
		if (this != &rhs) {
			eventID_ = rhs.eventID_;
			logName_ = rhs.logName_;
			index_ = rhs.eventID_;
			eventCategory_ = rhs.eventCategory_;
			entryType_ = rhs.entryType_;
			machineName_ = rhs.machineName_;
			messageSource_ = rhs.messageSource_;
			userSID_ = rhs.userSID_;
			dtGenerated_ = rhs.dtGenerated_;
			dtWritten_ = rhs.dtWritten_;
			arrData_ = rhs.arrData_;
			arrReplacements_ = rhs.arrReplacements_;
		}
		return *this;
	}

	inline bool operator==(const EventLogEntry& rhs) { return (eventID_ == rhs.eventID_ && index_ == rhs.index_); }
	inline bool operator!=(const EventLogEntry& rhs) { return !operator==(rhs); }

// Property accessors
public:
	tstring get_CategoryText() const
	{
		if (eventCategory_ == 0)
			return _T("None");

		TCHAR chRegKey[MAX_PATH];
		TCHAR chDefault[10];
		wsprintf(chDefault, _T("(%d)"), eventCategory_);

		// build path to message dll
		lstrcpy(chRegKey, _T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\"));
		lstrcat(chRegKey,logName_.c_str());
		lstrcat(chRegKey, _T("\\"));
		lstrcat(chRegKey,messageSource_.c_str());
		
		// Open on the local machine
		HKEY hKey;
		if (RegOpenKey(HKEY_LOCAL_MACHINE, chRegKey, &hKey))
			return chDefault;

		// Get the message file associated with this source/log.
		DWORD dwData = MAX_PATH, dwType;
		if (RegQueryValueEx(hKey, _T("CategoryMessageFile"), NULL, &dwType, (LPBYTE)chRegKey, &dwData))
		{
			RegCloseKey(hKey);
			return chDefault;
		}

		// Done with the registry.
		RegCloseKey(hKey);

		// Now load the message DLL
		TCHAR chMsgDll[MAX_PATH];
		LPTSTR pFile = chRegKey, pNextFile = NULL, pMsgBuff;
		HMODULE hLib;

		for (;;)
		{
			if ((pNextFile = _tcschr(pFile,_T(';'))))
				*pNextFile = 0;
			if (!ExpandEnvironmentStrings(pFile, chMsgDll, sizeofarray(chMsgDll)) ||
				!(hLib = LoadLibraryEx(chMsgDll, NULL, LOAD_LIBRARY_AS_DATAFILE)))
				return _T("");

			// Format the message from the message DLL with the insert strings
			if (FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | 
							  FORMAT_MESSAGE_ALLOCATE_BUFFER,
					hLib, eventCategory_, 
					MAKELANGID(LANG_NEUTRAL, SUBLANG_ENGLISH_US),
					(LPTSTR)&pMsgBuff,
					0, NULL))
				break;

			FreeLibrary(hLib);
			if (!pNextFile)
				return chDefault;

			pFile = ++pNextFile;
		}
		FreeLibrary(hLib);

		// Remove any CR/LF from the end.
		LPTSTR pTest = _tcschr(pMsgBuff, _T('\r'));
		if (pTest != NULL)
			*pTest = _T('\0');
		pTest = _tcsrchr(pMsgBuff, _T('\n'));
		if (pTest != NULL)
			*pTest = _T('\0');

		tstring sMessage = pMsgBuff;
		LocalFree((HLOCAL)pMsgBuff);
		return sMessage;
	}

	WORD get_Category() const { return eventCategory_; }
	const void* get_Data() const { return static_cast<const void*>(&arrData_[0]); }
	EventLogEntryType get_EntryType() const { return entryType_;}
	int get_EventID() const { return eventID_; }
	int get_Index() const { return index_; }
	const tstring& get_MachineName() const { return machineName_; }
	const std::vector<tstring>& get_ReplacementStrings() const { return arrReplacements_; }
	const tstring& get_Source() const { return messageSource_; }
	const DateTime& get_TimeGenerated() const { return dtGenerated_; }
	const DateTime& get_TimeWritten() const { return dtWritten_; }
	const tstring& get_UserName() const { return userSID_; }

	tstring get_Message() const
	{
		TCHAR chRegKey[MAX_PATH];

		// build path to message dll
		lstrcpy(chRegKey, _T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\"));
		lstrcat(chRegKey,logName_.c_str());
		lstrcat(chRegKey, _T("\\"));
		lstrcat(chRegKey,messageSource_.c_str());
		
		// Open on the local machine
		HKEY hKey;
		if (RegOpenKey(HKEY_LOCAL_MACHINE, chRegKey, &hKey))
			return _T("");

		// Get the message file associated with this source/log.
		DWORD dwData = MAX_PATH, dwType;
		if (RegQueryValueEx(hKey, _T("EventMessageFile"), NULL, &dwType, (LPBYTE)chRegKey, &dwData))
		{
			RegCloseKey(hKey);
			return _T("");
		}

		// Done with the registry.
		RegCloseKey(hKey);

		// Create an array of NULL-terminated strings
		std::vector<LPCTSTR> arrStrings;
		for (unsigned int i = 0; i < arrReplacements_.size(); ++i)
		{
			const tstring& str = arrReplacements_[i];
			CheckAndConvertReplacementString(const_cast<tstring&>(str));
			arrStrings.push_back(str.c_str());
		}

		// Now load the message DLL
		TCHAR chMsgDll[MAX_PATH];
		LPTSTR pFile = chRegKey, pNextFile = NULL, pMsgBuff;
		HMODULE hLib;

		for (;;)
		{
			if ((pNextFile = _tcschr(pFile,_T(';'))))
				*pNextFile = 0;
			if (!ExpandEnvironmentStrings(pFile, chMsgDll, sizeofarray(chMsgDll)) ||
				!(hLib = LoadLibraryEx(chMsgDll, NULL, LOAD_LIBRARY_AS_DATAFILE)))
				return _T("");

			// Format the message from the message DLL with the insert strings
			if (FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | 
							  FORMAT_MESSAGE_ALLOCATE_BUFFER |
							  FORMAT_MESSAGE_ARGUMENT_ARRAY | 60,
					hLib, eventID_, 
					MAKELANGID(LANG_NEUTRAL, SUBLANG_ENGLISH_US),
					(LPTSTR)&pMsgBuff,
					0, (va_list*)&arrStrings[0]))
				break;

			FreeLibrary(hLib);
			if (!pNextFile)
				return _T("");

			pFile = ++pNextFile;
		}
		FreeLibrary(hLib);

		tstring sMessage = pMsgBuff;
		LocalFree((HLOCAL)pMsgBuff);
		return sMessage;
	}

// Internal constructor
private:
	friend class EventLogEntryCollection;
	friend class EventLog;
	EventLogEntry(const tstring& logName, EVENTLOGRECORD* pRec) : 
		logName_(logName), eventID_(0), index_(0), eventCategory_(0), entryType_(Information)
	{
		// If no record is available, ignore the rest.
		if (pRec == NULL)
			return;

		// Collect the data
		eventID_ = pRec->EventID;
		entryType_ = (EventLogEntryType) pRec->EventType;
		eventCategory_ = pRec->EventCategory;
		index_ = pRec->RecordNumber;
		dtGenerated_ = DateTime((time_t)pRec->TimeGenerated).ToLocalTime();
		dtWritten_ = DateTime((time_t)pRec->TimeWritten).ToLocalTime();

		// Collect the variable-length strings at the end.
		messageSource_ = (LPTSTR)((LPBYTE)pRec + sizeof(EVENTLOGRECORD));
		machineName_ = (LPTSTR)((LPBYTE)pRec + sizeof(EVENTLOGRECORD) + messageSource_.length() + 1);

		// Get the account name if it's there.
		userSID_ = _T("N/A");
		if (pRec->UserSidLength > 0)
		{
			PSID psid = (PSID)((LPBYTE)pRec + pRec->UserSidOffset);
			TCHAR chName[MAX_PATH], chDomain[MAX_PATH];
			DWORD dwName = MAX_PATH, dwDomain = MAX_PATH;
			SID_NAME_USE sidName;
			if (LookupAccountSid((machineName_.empty()) ? NULL : machineName_.c_str(), psid,
					chName, &dwName, chDomain, &dwDomain, &sidName))
			{
				if (chDomain[0] != _T('\0'))
				{
					userSID_ = chDomain;
					userSID_ += _T("\\");
					userSID_ += chName;
				}
				else userSID_ = chName;
			}
		}

		// Get the replacement strings
		if (pRec->NumStrings > 0)
		{
			LPTSTR pStr = (LPTSTR)((LPBYTE)pRec + pRec->StringOffset);
			for (int i = 0; i < pRec->NumStrings; ++i) {
				arrReplacements_.push_back(tstring(pStr));
				pStr += lstrlen(pStr)+1;
			}
		}

		if (pRec->DataLength > 0)
		{
			LPBYTE lpData = (LPBYTE)((LPBYTE)pRec + pRec->DataOffset);
			for (unsigned int i = 0; i < pRec->DataLength; ++i)
				arrData_.push_back(*lpData);
		}
	}

	void CheckAndConvertReplacementString(tstring& s) const
	{
		if (s.length() < 3 || s[0] != _T('%') || s[1] != _T('%') ||	_ttoi(&s[2]) == 0)
			return;

		TCHAR chRegKey[MAX_PATH];

		// build path to message dll
		lstrcpy(chRegKey, _T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\"));
		lstrcat(chRegKey,logName_.c_str());
		lstrcat(chRegKey, _T("\\"));
		lstrcat(chRegKey,messageSource_.c_str());
		
		// Open on the local machine
		HKEY hKey;
		if (RegOpenKey(HKEY_LOCAL_MACHINE, chRegKey, &hKey))
			return;

		// Get the message file associated with this source/log.
		DWORD dwData = MAX_PATH, dwType;
		if (RegQueryValueEx(hKey, _T("ParameterMessageFile"), NULL, &dwType, (LPBYTE)chRegKey, &dwData))
		{
			RegCloseKey(hKey);
			return;
		}

		// Done with the registry.
		RegCloseKey(hKey);

		// Now load the message DLL
		TCHAR chMsgDll[MAX_PATH];
		LPTSTR pFile = chRegKey, pNextFile = NULL, pMsgBuff;
		HMODULE hLib;

		for (;;)
		{
			if ((pNextFile = _tcschr(pFile,_T(';'))))
				*pNextFile = 0;
			if (!ExpandEnvironmentStrings(pFile, chMsgDll, sizeofarray(chMsgDll)) ||
				!(hLib = LoadLibraryEx(chMsgDll, NULL, LOAD_LIBRARY_AS_DATAFILE)))
				return;

			// Format the message from the message DLL with the insert strings
			if (FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | 
							  FORMAT_MESSAGE_ALLOCATE_BUFFER,
					hLib, _ttoi(&s[2]), 
					MAKELANGID(LANG_NEUTRAL, SUBLANG_ENGLISH_US),
					(LPTSTR)&pMsgBuff,
					0, NULL))
				break;

			FreeLibrary(hLib);
			if (!pNextFile)
				return;

			pFile = ++pNextFile;
		}
		FreeLibrary(hLib);
		s = pMsgBuff;
		LocalFree((HLOCAL)pMsgBuff);
	}

// Internal data
private:
	int eventID_;
	int index_;
	WORD eventCategory_;
	EventLogEntryType entryType_;
	tstring machineName_;
	tstring messageSource_;
	tstring userSID_;
	tstring logName_;
	DateTime dtGenerated_;
	DateTime dtWritten_;
	std::vector<BYTE> arrData_;
	std::vector<tstring> arrReplacements_;
};

/******************************************************************************/
// EventLogEntryCollection
//
// The EventLogEntryCollection encapsulates a collection of event log entries.
//
/******************************************************************************/
class EventLogEntryCollection
{
// Class data
public:
	typedef std::vector<EventLogEntry>::const_iterator const_iterator;
private:
	std::vector<EventLogEntry> arrEntries_;

// Constructor
public:
	EventLogEntryCollection() {/* */}
	EventLogEntryCollection(const EventLogEntryCollection& rhs) : arrEntries_(rhs.arrEntries_) {/* */}
	~EventLogEntryCollection() {/* */}

// Properties
public:
	__declspec(property(get=get_Count)) int Count;

// Property accessors
public:
	const_iterator begin() const { return arrEntries_.begin(); }
	const_iterator end() const { return arrEntries_.end(); }
	int get_Count() const { return static_cast<int>(arrEntries_.size()); }
	const EventLogEntry& Item(int index) const { return arrEntries_[index]; }

// Internal constructor
private:
	friend class EventLogWrittenEventArgs;
	friend class EventLogHolder;
	bool Populate(const tstring& logName, HANDLE eventLog, DWORD dwPos=0)
	{
		// First try to read from our indicated position.
		if (!ReadEventLogEntries(logName, eventLog, EVENTLOG_FORWARDS_READ | EVENTLOG_SEEK_READ, dwPos))
		{
			DWORD rc = GetLastError();
			if (rc == ERROR_INVALID_PARAMETER)
			{
				// Per KB
				// The ReadEventLog() Win32 API function might fail and GetLastError() returns 87 
				// (ERROR_INVALID_PARAMETERS) while having all valid parameters passed to ReadEventLog(). 
				// Cause: The Event Logging Service fails to process the read operation when an application 
				// uses the ReadEventLog() function with the EVENTLOG_SEEK_READ flag to read large event log file. 
				if (!ReadEventLogEntries(logName, eventLog, EVENTLOG_FORWARDS_READ | EVENTLOG_SEQUENTIAL_READ, dwPos))
					throw EventLogException("ReadEventLog failed", GetLastError());
			}
			else if (rc != ERROR_HANDLE_EOF)
				throw EventLogException("ReadEventLog failed", rc);
		}
		return !arrEntries_.empty();
	}

	bool ReadEventLogEntries(const tstring& logName, HANDLE eventLog, DWORD dwType, DWORD dwPos)
	{
		BYTE bBuffer[1024]; 
		EVENTLOGRECORD *pevlr = (EVENTLOGRECORD*) bBuffer; 
		DWORD dwRead, dwNeeded; 

		// Read the 1st record
		if (ReadEventLog(eventLog, 
			dwType, dwPos, pevlr, sizeof(bBuffer), &dwRead, &dwNeeded))
		{
			do 
			{
				while (dwRead > 0) 
				{
					arrEntries_.push_back(EventLogEntry(logName, pevlr));
					dwRead -= pevlr->Length; 
					pevlr = (EVENTLOGRECORD*)((LPBYTE)pevlr + pevlr->Length); 
				} 
				pevlr = (EVENTLOGRECORD*) bBuffer; 
			} while (ReadEventLog(eventLog,
				EVENTLOG_FORWARDS_READ | EVENTLOG_SEQUENTIAL_READ,
				0,            // ignored for sequential reads 
				pevlr,        // pointer to buffer 
				sizeof(bBuffer),  // size of buffer 
				&dwRead,      // number of bytes read 
				&dwNeeded));   // bytes in next record 
		}
		return !arrEntries_.empty();
	}
};

/******************************************************************************/
// EventLogWrittenEventArgs
//
// The EventLogWrittenEventArgs object is passed to the notify delegate
// when the event log is being monitored for changes.
//
/******************************************************************************/
class EventLogWrittenEventArgs
{
// Class data
private:
	EventLogEntryCollection elCollection_;

// Constructor
private:
	friend class EventLogHolder;
	EventLogWrittenEventArgs(const tstring& logName, HANDLE eventLog, DWORD dwPos=0)
	{
		elCollection_.Populate(logName, eventLog, dwPos);
	}

// Properties
public:
	__declspec(property(get=get_Entry)) const EventLogEntryCollection& Entry;

// Property helpers
public:
	const EventLogEntryCollection& get_Entry() const { return elCollection_; }
};

/******************************************************************************/
// EventLogHolder
//
// The EventLogHolder class is the handle/body wrapper for the event log
// information.
//
/******************************************************************************/
class EventLogHolder
{
// Class data
private:
	tstring logName_;
	tstring machineName_;
	tstring sourceName_;
	HANDLE eventLog_;
	HANDLE eventSource_;
	HANDLE thread_;
	HANDLE evtStop_;
	bool raisingEvents_;
	Delegate_1<MultiThreadModel, EventLogWrittenEventArgs> writeEvent_;

// Constructor
private:
	friend class EventLog;
	friend class UsageCountedObject<EventLogHolder, MultiThreadModel>;
	EventLogHolder(LPCTSTR pszLog=NULL, LPCTSTR pszServer=NULL) : 
		logName_((pszLog) ? pszLog : _T("")), machineName_((pszServer) ? pszServer : _T("")),
		sourceName_(_T("")), eventLog_(0), eventSource_(0), thread_(0), evtStop_(0),
		raisingEvents_(false) {/* */}
	~EventLogHolder() { Close(); }

// Methods
private:
	inline void EnableRaisingEvents(bool f) 
	{
		if (!eventLog_)
		{
			if (logName_.empty())
				throw EventLogException("You must supply a log name to read the event log entries");
			Open();
		}

		if (f != raisingEvents_)
		{
#ifdef _MT
			if (!f)
			{
				SetEvent(evtStop_);
				if (thread_ && GetCurrentThread() != thread_)
					WaitForSingleObject(thread_, INFINITE);
				CloseHandle(thread_);
				thread_ = NULL;
				CloseHandle(evtStop_);
				evtStop_ = NULL;
			}
			else
			{
				evtStop_ = CreateEvent(NULL, TRUE, FALSE, NULL);
				unsigned threadID;
				thread_ = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, WatcherThreadProc, reinterpret_cast<void*>(this), 0, &threadID));
			}
			raisingEvents_ = f;
#endif
		}
	}

	inline EventLogEntryCollection get_Entries() const 
	{
		if (!eventLog_)
		{
			if (logName_.empty())
				throw EventLogException("You must supply a log name to read the event log entries");
			const_cast<EventLogHolder*>(this)->Open();
		}

		// Read the events into the collection
		EventLogEntryCollection coll;
		coll.Populate(logName_, eventLog_, 0);
		return coll;
	}

	void Clear()
	{
		if (!eventLog_)
		{
			if (logName_.empty())
				throw EventLogException("You must supply a log name to read the event log entries");
			Open();
		}
		if (eventLog_) {
			ClearEventLog(eventLog_, NULL);
			Open();
		}
	}

	void Close()
	{
		if (eventLog_)	{
			EnableRaisingEvents(false);
			CloseEventLog(eventLog_);
			eventLog_ = NULL;
		}
	}

	void RegisterSource()
	{
		if (sourceName_.empty())
			throw EventLogException("Source must be supplied", ERROR_INVALID_PARAMETER);

		if (eventSource_) {
			DeregisterEventSource(eventSource_);
			eventSource_ = NULL;
		}

		// See if the source exists.  If not, create it.
		if (!SourceExists(sourceName_.c_str(), machineName_.c_str()))
		{
			// Create the event source
			if (logName_.empty())
				logName_ = _T("Application");
			CreateEventSource(NULL, 0, sourceName_.c_str(), logName_.c_str(), machineName_.c_str());
		}

		// Register the event source; it must already exist
		LPCTSTR pszMachine = (machineName_ == _T(".")) ? _T("") : machineName_.c_str();
		eventSource_ = RegisterEventSource(pszMachine, sourceName_.c_str());
		if (eventSource_ == NULL)
			throw EventLogException("RegisterEventSource failed.");
	}

	void Open()
	{
		// Close any existing log.
		if (eventLog_)
			Close();

		// Now open the new event log.
		eventLog_ = OpenEventLog(machineName_.c_str(), logName_.c_str());
		if (eventLog_ == NULL)
			throw EventLogException("OpenEventLog failed.");
	}

	static void CreateEventSource(LPCTSTR pszMessageFile, DWORD dwTypesSupported, LPCTSTR pszSource, LPCTSTR pszLogName, LPCTSTR pszMachine=NULL)
	{
		// Validate the parameters
		if (pszSource == NULL || *pszSource == _T('\0'))
			throw EventLogException("LogName, Source and MessageFile must be supplied", ERROR_INVALID_PARAMETER);

		// Assume application log if not supplied.
		if (pszLogName == NULL || *pszLogName == _T('\0'))
			pszLogName = _T("Application");

		// If no message file is supplied, use the executable name.
		TCHAR chBuff[_MAX_PATH];
		if (pszMessageFile == NULL || *pszMessageFile == _T('\0'))
		{
			GetModuleFileName(NULL, chBuff, sizeofarray(chBuff));
			pszMessageFile = chBuff;
		}

		try
		{
			RegistryKey regKey = RegistryKey::OpenRemoteBaseKey(RegistryHive::LocalMachine, pszMachine);
			regKey = regKey.OpenSubKey(_T("SYSTEM\\CurrentControlSet\\Services\\EventLog"));
			if (!regKey.SubKeyExists(pszLogName))
				regKey = regKey.CreateSubKey(pszLogName);
			else
				regKey = regKey.OpenSubKey(pszLogName);
			regKey = regKey.CreateSubKey(pszSource);
			if (pszMessageFile != NULL)
				regKey.SetValue(_T("EventMessageFile"), RegistryValue(_T(""), pszMessageFile));
			if (dwTypesSupported > 0)
				regKey.SetValue(_T("TypesSupported"), RegistryValue(_T(""), dwTypesSupported));
		}
		catch(const RegistryException& e)
		{
			throw EventLogException(e.what(), e.ErrorCode);
		}
	}

	static bool SourceExists(LPCTSTR pszSource, LPCTSTR pszMachine=NULL)
	{
		try
		{
			RegistryKey regKey = RegistryKey::OpenRemoteBaseKey(RegistryHive::LocalMachine, pszMachine);
			regKey = regKey.OpenSubKey(_T("SYSTEM\\CurrentControlSet\\Services\\EventLog"), KEY_READ);
			RegistryKeyCollection rLogs = regKey.GetSubKeyNames();
			for (RegistryKeyCollection::const_iterator it = rLogs.begin(); it != rLogs.end(); ++it)
			{
				RegistryKey regSubKey = regKey.OpenSubKey(it->c_str(), KEY_READ);
				if (regSubKey.SubKeyExists(pszSource, KEY_READ))
					return true;
			}
		}
		catch(const RegistryException&)
		{
		}
		return false;
	}

	static void Delete(LPCTSTR pszLogName, LPCTSTR pszMachine=NULL)
	{
		if (pszLogName == NULL)
			throw EventLogException("LogName must be supplied", ERROR_INVALID_PARAMETER);
		try
		{
			RegistryKey regKey = RegistryKey::OpenRemoteBaseKey(RegistryHive::LocalMachine, pszMachine);
			regKey = regKey.OpenSubKey(_T("SYSTEM\\CurrentControlSet\\Services\\EventLog"), KEY_READ);
			regKey.DeleteSubKey(pszLogName);
		}
		catch(const RegistryException& e)
		{
			throw EventLogException(e.what(), e.ErrorCode);
		}
	}

	static void DeleteEventSource(LPCTSTR pszSource, LPCTSTR pszLogName, LPCTSTR pszMachine=NULL)
	{
		// Validate the parameters
		if (pszSource == NULL)
			throw EventLogException("Source must be supplied", ERROR_INVALID_PARAMETER);

		tstring strLog;
		if (pszLogName == NULL)
		{
			strLog = LogNameFromSourceName(pszSource, pszMachine);
			pszLogName = strLog.c_str();
		}

		try
		{
			RegistryKey regKey = RegistryKey::OpenRemoteBaseKey(RegistryHive::LocalMachine, pszMachine);
			regKey = regKey.OpenSubKey(_T("SYSTEM\\CurrentControlSet\\Services\\EventLog"), KEY_READ);
			RegistryKeyCollection rLogs = regKey.GetSubKeyNames();
			for (RegistryKeyCollection::const_iterator it = rLogs.begin(); it != rLogs.end(); ++it)
			{
				RegistryKey regSubKey = regKey.OpenSubKey(it->c_str());
				if (regSubKey.SubKeyExists(pszSource))
				{
					regSubKey.DeleteSubKeyTreee(pszSource);
					break;
				}
			}
		}
		catch(const RegistryException& e)
		{
			throw EventLogException(e.what(), e.ErrorCode);
		}
	}

	static bool Exists(LPCTSTR pszLogName, LPCTSTR pszMachine=NULL)
	{
		try
		{
			RegistryKey regKey = RegistryKey::OpenRemoteBaseKey(RegistryHive::LocalMachine, pszMachine);
			regKey = regKey.OpenSubKey(_T("SYSTEM\\CurrentControlSet\\Services\\EventLog"), KEY_READ);
			return regKey.SubKeyExists(pszLogName);
		}
		catch(const RegistryException&)
		{
		}
		return false;
	}

	static tstring LogNameFromSourceName(LPCTSTR pszSource, LPCTSTR pszMachine=NULL)
	{
		try
		{
			RegistryKey regKey = RegistryKey::OpenRemoteBaseKey(RegistryHive::LocalMachine, pszMachine);
			regKey = regKey.OpenSubKey(_T("SYSTEM\\CurrentControlSet\\Services\\EventLog"), KEY_READ);
			RegistryKeyCollection rLogs = regKey.GetSubKeyNames();
			for (RegistryKeyCollection::const_iterator it = rLogs.begin(); it != rLogs.end(); ++it)
			{
				RegistryKey regSubKey = regKey.OpenSubKey(it->c_str(), KEY_READ);
				if (regSubKey.SubKeyExists(pszSource, KEY_READ))
					return *it;
			}
		}
		catch(const RegistryException& e)
		{
			throw EventLogException(e.what(), e.ErrorCode);
		}
		return _T("");
	}

	void WriteEntry(EventLogEntryType evtType, WORD evtCategory, DWORD evtID,
					int nCount, LPCTSTR* ppszArray, int nDataSize, LPVOID pData, PSID psid = NULL)
	{
		if (eventSource_ == 0)
			RegisterSource();

		BYTE sidBuffer[4096];
		DWORD dwSize = sizeof(sidBuffer);
		
		// Lookup the SID if it isn't supplied.  Use the currently
		// logged on (interactive) user.
		if (psid == NULL)
		{
			TCHAR szUserName[256], szDomain[256];
			DWORD dwName = sizeofarray(szUserName);
			DWORD dwDomain = sizeofarray(szDomain);
			SID_NAME_USE sidType;

			::ZeroMemory(szUserName, sizeof(szUserName));
			::ZeroMemory(szDomain, sizeof(szDomain));
			::ZeroMemory(&sidType, sizeof(sidType));

			::GetUserName(szUserName, &dwName);
			if (::LookupAccountName(NULL, szUserName, &sidBuffer, &dwSize, 
									szDomain, &dwDomain, &sidType))
				psid = sidBuffer;
		}

		// Report the event
		if (!::ReportEvent(eventSource_, static_cast<WORD>(evtType), evtCategory, evtID, psid, 
					  static_cast<WORD>(nCount), nDataSize, ppszArray, pData))
		{
			throw EventLogException("ReportEvent failed.");			
		}
	}

	static unsigned __stdcall WatcherThreadProc(void* p)
	{
		reinterpret_cast<EventLogHolder*>(p)->EventLogWatcher();
		return 0;
	}

	void EventLogWatcher()
	{
		// Open a new handle to the event log
		HANDLE hEventLog = ::OpenEventLog(machineName_.c_str(), logName_.c_str());

		HANDLE arrHandles[2];
		arrHandles[0] = evtStop_;
		arrHandles[1] = CreateEvent(NULL, TRUE, FALSE, NULL);
		NotifyChangeEventLog(hEventLog, arrHandles[1]);
		
		while (raisingEvents_)
		{
			// First record how many records are in the event log currently.
			DWORD dwNumRecs = 0, dwOldestRecord = 0;
			GetOldestEventLogRecord(hEventLog, &dwOldestRecord);
			GetNumberOfEventLogRecords(hEventLog, &dwNumRecs);

			// Wait for a change
			DWORD rc = WaitForMultipleObjects(2, arrHandles, FALSE, INFINITE);
			if (rc == WAIT_OBJECT_0)
				break;

			// Build an EventLogWrittenEvent event and fire the event
			EventLogWrittenEventArgs evArgs(logName_, eventLog_, (dwNumRecs + dwOldestRecord));
			writeEvent_(evArgs);
		}

		// Close our handle to the event log.
		CloseEventLog(hEventLog);
	}

// Unavailable methods
private:
	EventLogHolder(const EventLogHolder&);
	EventLogHolder& operator=(const EventLogHolder&);
};

/******************************************************************************/
// EventLog
//
// The EventLog class is the primary manipulating class for managing the
// event log.
//
/******************************************************************************/
class EventLog : private UsageCountedObject<EventLogHolder, MultiThreadModel>
{
// Typedefs
private:
	typedef UsageCountedObject<EventLogHolder, MultiThreadModel> _BaseClass;

// Constructor
public:
	EventLog() : _BaseClass(JTI_NEW EventLogHolder) { /* */ }
	EventLog(LPCTSTR pszLog, LPCTSTR pszServer=NULL) : _BaseClass(JTI_NEW EventLogHolder(pszLog, pszServer)) { ptr()->Open(); }
	EventLog(const EventLog &rhs) : _BaseClass(rhs) {/* */}
	~EventLog() {/* */}

// Operators
public:
	EventLog& operator=(const EventLog& rhs) {
		if (this != &rhs)
			_BaseClass::Assign(rhs);
		return *this;
	}

// Properties
public:
	__declspec(property(get=get_Source,put=put_Source)) const tstring& Source;
	__declspec(property(get=get_Log,put=put_Log)) const tstring& Log;
	__declspec(property(get=get_MachineName,put=put_MachineName)) const tstring& MachineName;
	__declspec(property(get=get_LogDisplayName)) const tstring& LogDisplayName;
	__declspec(property(get=get_Entries)) EventLogEntryCollection Entries;
	__declspec(property(get=get_EnableRaisingEvents,put=put_EnableRaisingEvents)) bool EnableRaisingEvents;

// Property accessors
public:
	inline const tstring& get_Source() const { return ptr()->sourceName_; }
	inline void put_Source(LPCTSTR pszSource) { ptr()->sourceName_ = pszSource; }
	inline const tstring& get_Log() const { return ptr()->logName_; }
	inline void put_Log(LPCTSTR pszLog) { ptr()->logName_ = pszLog; }
	inline const tstring& get_MachineName() const { return ptr()->machineName_; }
	inline void put_MachineName(LPCTSTR pszMachine) { ptr()->machineName_ = pszMachine; }
	inline const tstring& get_LogDisplayName() const { return ptr()->logName_; }
	inline bool get_EnableRaisingEvents() const { return ptr()->raisingEvents_; }
	inline void put_EnableRaisingEvents(bool f) { ptr()->EnableRaisingEvents(f); }
	inline EventLogEntryCollection get_Entries() const { return ptr()->get_Entries(); }

// Methods
public:
	void WriteEntry(int eventID, short categoryID = 0, EventLogEntryType evtType = Information) const
	{
		ptr()->WriteEntry(evtType, categoryID, eventID, 0, NULL, 0, NULL, NULL);
	}

	void WriteEntry(LPCTSTR pszText, int eventID = 0, short categoryID = 0, EventLogEntryType evtType = Information) const
	{
		ptr()->WriteEntry(evtType, categoryID, eventID, 1, &pszText, 0, NULL, NULL);
	}

	void WriteEntry(LPCTSTR pszText1, LPCTSTR pszText2, int eventID = 0, short categoryID = 0, EventLogEntryType evtType = Information)  const
	{
		LPCTSTR parrBuff[] = { pszText1, pszText2 };
		ptr()->WriteEntry(evtType, categoryID, eventID, 2, parrBuff, 0, NULL, NULL);
	}

	void WriteEntry(LPCTSTR pszText1, LPCTSTR pszText2, LPCTSTR pszText3, int eventID = 0, short categoryID = 0, EventLogEntryType evtType = Information)  const
	{
		LPCTSTR parrBuff[] = { pszText1, pszText2, pszText3 };
		ptr()->WriteEntry(evtType, categoryID, eventID, 2, parrBuff, 0, NULL, NULL);
	}

	void WriteEntry(LPCTSTR pszText1, LPCTSTR pszText2, LPCTSTR pszText3, LPCTSTR pszText4, int eventID = 0, short categoryID = 0, EventLogEntryType evtType = Information)  const
	{
		LPCTSTR parrBuff[] = { pszText1, pszText2, pszText3, pszText4 };
		ptr()->WriteEntry(evtType, categoryID, eventID, 2, parrBuff, 0, NULL, NULL);
	}

	void WriteEntry(EventLogEntryType evtType, int evtID, short evtCategory,
			std::vector<tstring>& arrStrings, int nDataSize=0, LPVOID pData=NULL, PSID psid = NULL)
	{
		LPCTSTR* parrBuff = NULL;
		if (arrStrings.size() > 0)
		{
			parrBuff = JTI_NEW LPCTSTR[arrStrings.size()];
			for (size_t i = 0; i < arrStrings.size(); ++i)
				parrBuff[i] = arrStrings[i].c_str();
		}
		ptr()->WriteEntry(evtType, evtCategory, evtID, static_cast<int>(arrStrings.size()), parrBuff, nDataSize, pData, psid);
	}

// Delegates
public:
	template<class _Object, class _Class>
	void add_OnEntryWritten(const _Object& object, void (_Class::* fnc)(EventLogWrittenEventArgs))
	{
		ptr()->writeEvent_.add(object,fnc);
	}
	inline void add_OnEntryWritten(void (*fnc)(EventLogWrittenEventArgs))
	{
		ptr()->writeEvent_.add(fnc);
	}

	inline void Clear() { ptr()->Clear(); }
	inline void Close() { ptr()->Close(); }

	void CreateEventSource(LPCTSTR pszMessageFile, DWORD dwTypesSupported)
	{
		CreateEventSource(pszMessageFile, dwTypesSupported, 
				Source.c_str(), Log.c_str(), ptr()->machineName_.c_str());
	}

// Static methods
public:
	static void CreateEventSource(LPCTSTR pszSource, LPCTSTR pszLogName, LPCTSTR pszMachine)
	{
		CreateEventSource(NULL, 0, pszSource, pszLogName, pszMachine);
	}

	static void CreateEventSource(LPCTSTR pszMessageFile, LPCTSTR pszSource, LPCTSTR pszLogName, LPCTSTR pszMachine)
	{
		CreateEventSource(pszMessageFile, 0, pszSource, pszLogName, pszMachine);
	}

	static void CreateEventSource(LPCTSTR pszMessageFile, DWORD dwTypesSupported, LPCTSTR pszSource, LPCTSTR pszLogName, LPCTSTR pszMachine=NULL)
	{
		EventLogHolder::CreateEventSource(pszMessageFile, dwTypesSupported, pszSource, pszLogName, pszMachine);
	}

	static void Delete(LPCTSTR pszLogName, LPCTSTR pszMachine=NULL)
	{
		EventLogHolder::Delete(pszLogName, pszMachine);
	}

	static void DeleteEventSource(LPCTSTR pszSource, LPCTSTR pszLogName=NULL, LPCTSTR pszMachine=NULL)
	{
		EventLogHolder::DeleteEventSource(pszSource, pszLogName, pszMachine);
	}

	static bool Exists(LPCTSTR pszLogName, LPCTSTR pszMachine=NULL)
	{
		return EventLogHolder::Exists(pszLogName, pszMachine);
	}

	static std::vector<EventLog> GetEventLogs(LPCTSTR pszMachine=NULL)
	{
		std::vector<EventLog> arrEvents;
		try
		{
			RegistryKey regKey = RegistryKey::OpenRemoteBaseKey(RegistryHive::LocalMachine, pszMachine);
			regKey = regKey.OpenSubKey(_T("SYSTEM\\CurrentControlSet\\Services\\EventLog"), KEY_READ);
			RegistryKeyCollection rLogs = regKey.GetSubKeyNames();
			for (RegistryKeyCollection::const_iterator it = rLogs.begin(); it != rLogs.end(); ++it)
				arrEvents.push_back(EventLog(it->c_str(), pszMachine));
		}
		catch(const RegistryException& e)
		{
			arrEvents.clear();
			throw EventLogException(e.what(), e.ErrorCode);
		}
		return arrEvents;		
	}

	static tstring LogNameFromSourceName(LPCTSTR pszSource, LPCTSTR pszMachine=NULL)
	{
		return EventLogHolder::LogNameFromSourceName(pszSource, pszMachine);
	}

	static bool SourceExists(LPCTSTR pszSource, LPCTSTR pszMachine=NULL)
	{
		return EventLogHolder::SourceExists(pszSource, pszMachine);
	}
};

} // namespace JTI_Util

#pragma warning(default:4706) // assignment within conditional expression

#endif // __EVENTLOG_H_INCL__
