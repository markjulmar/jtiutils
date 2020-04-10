/****************************************************************************/
//
// PsList.h
//
// This file describes objects which can query the process/module list
// under Windows NT or Win2k.
//
// ------------------------------------------------------------------------
// Usage:
//
//	ProcessEnumerator pe;
//	ProcessCollection pc = pe.Snapshot();
//	for (ProcessCollection::const_iterator it = pc.begin(); it != pc.end(); ++it)
//	{
//		_tout << it->ProcessID << _T(": ") << it->Name << endl;
//		ModuleCollection mc =it->Modules;
//		for (ModuleCollection::const_iterator mit = mc.begin(); mit != mc.end(); ++mit)
//		{
//			_tout << _T("   ") << hex << showbase << mit->hModule 
//				  << _T(": ") << mit->Name << _T(" ")
//				  << mit->get_VersionInfo(_T("FileVersion")) << endl;
//		}
//	}
// ------------------------------------------------------------------------
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
//
/****************************************************************************/

#ifndef __PSLIST_H_INCL__
#define __PSLIST_H_INCL__

/*----------------------------------------------------------------------------
	INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <TlHelp32.h>
#include <psapi.h>
#include <vector>
#include <stdexcept>
#include <JTIUtils.h>

// Force a linkage record against VER.LIB
#pragma comment(linker, "/defaultlib:version.lib")

namespace JTI_Util
{
/**************************************************************************
// ModuleVersion
//
// This object is responsible for enumerating the process list
//
**************************************************************************/
class ModuleVersion
{
// Class data
private:
	WORD langType_, codePage_;
	BYTE* verInfo_;

// Constructor
public:
	ModuleVersion(LPCTSTR modName) : verInfo_(0), langType_(0), codePage_(0)
	{
		struct LANGANDCODEPAGE {
		WORD wLanguage;
		WORD wCodePage;
		} *lpTranslate;

		// Get the full path to the module.
		TCHAR moduleFilename[MAX_PATH]; moduleFilename[0] = _T('\0');
		if (modName != NULL)
		{
			HMODULE hModule = GetModuleHandle(modName);
			if (hModule != NULL)
				GetModuleFileName(hModule, moduleFilename, MAX_PATH);
			else
				lstrcpy(moduleFilename, modName);
		}
		else
			GetModuleFileName(NULL, moduleFilename, MAX_PATH);

		if (moduleFilename[0] != _T('\0'))
		{
			// Read the file version info
			DWORD dwHandle, dwSize;
			if ((dwSize = GetFileVersionInfoSize(const_cast<LPTSTR>(moduleFilename), &dwHandle)) > 0)
			{
				verInfo_ = JTI_NEW BYTE[dwSize];
				if (::GetFileVersionInfo(moduleFilename, 0, dwSize, verInfo_))
				{
					UINT len;
					if (VerQueryValue(verInfo_, _T("\\VarFileInfo\\Translation"), reinterpret_cast<void**>(&lpTranslate), &len))
					{
						if (len >= sizeof(LANGANDCODEPAGE))
						{
							langType_ = lpTranslate->wLanguage;
							codePage_ = lpTranslate->wCodePage;
							return;
						}
					}

				}
				delete verInfo_;
				verInfo_ = NULL;
			}
		}
	}

	~ModuleVersion() { delete [] verInfo_; }

// Methods
public:
	tstring get_Value(LPCTSTR key) const
	{
		tstring sValue;
		if (verInfo_)
		{
			TCHAR chBuff[512]; wsprintf(chBuff, _T("\\StringFileInfo\\%04X%04X\\%s"), langType_, codePage_, key);
			void* pVal; UINT len;
			if (VerQueryValue(verInfo_, chBuff, &pVal, &len)) 
				sValue = reinterpret_cast<LPTSTR>(pVal);
		}
		return sValue;
	}
};

/**************************************************************************
// ModuleEntry
//
// This object represents a single DLL loaded into a process.
//
**************************************************************************/
class ModuleEntry
{
// Class data
private:
	MODULEENTRY32 me_;
	mutable ModuleVersion* pmv_;

// Constructor/Copy
public:
	ModuleEntry(const MODULEENTRY32& me) : pmv_(0) { memcpy(&me_, &me, sizeof(me)); }
	ModuleEntry(const ModuleEntry& rhs) : pmv_(0) { memcpy(&me_, &rhs.me_, sizeof(rhs.me_)); }
	ModuleEntry& operator=(const ModuleEntry& rhs)
	{
		if (this != &rhs)
		{
			delete pmv_;
			pmv_ = NULL;

			memcpy(&me_, &rhs.me_, sizeof(rhs.me_));
		}
		return *this;
	}
	~ModuleEntry()
	{
		delete pmv_;
		pmv_ = NULL;
	}

// Properties
public:
	__declspec(property(get=get_PID)) DWORD ProcessID;
	__declspec(property(get=get_Name)) tstring Name;
	__declspec(property(get=get_FullName)) tstring FullName;
	__declspec(property(get=get_RefCount)) int RefCount;
	__declspec(property(get=get_GlobalRefCount)) int GlobalRefCount;
	__declspec(property(get=get_hModule)) HMODULE hModule;
	__declspec(property(get=get_BaseAddr)) LPCVOID BaseAddress;
	__declspec(property(get=get_Size)) DWORD Size;

// Methods
public:
	tstring get_VersionInfo(LPCTSTR pszKey) const
	{
		if (pmv_ == NULL)
			pmv_ = JTI_NEW ModuleVersion(get_FullName().c_str());
		return pmv_->get_Value(pszKey);
	}

	int get_GlobalRefCount() const { return static_cast<int>(me_.GlblcntUsage); }
	int get_RefCount() const { return static_cast<int>(me_.ProccntUsage); }
	DWORD get_PID() const { return me_.th32ProcessID; }
	tstring get_Name() const { return me_.szModule; }
	tstring get_FullName() const { return me_.szExePath; }
	HMODULE get_hModule() const { return me_.hModule; }
	LPCVOID get_BaseAddr() const { return me_.modBaseAddr; }
	DWORD get_Size() const { return me_.dwSize; }
};

/**************************************************************************
// ModuleCollection
//
// This object holds a list of modules for a process.
//
**************************************************************************/
class ModuleCollection
{
// Class data
public:
	typedef std::vector<ModuleEntry>::const_iterator const_iterator;
private:
	std::vector<ModuleEntry> arrModules_;

// Destructor
public:
	~ModuleCollection() {/* */}

// Methods
public:
	bool empty() const { return arrModules_.empty(); }
	int size() const { return static_cast<int>(arrModules_.size()); }
	const_iterator begin() const { return arrModules_.begin(); }
	const_iterator end() const { return arrModules_.end(); }

// Constructor
private:
	friend class ModuleEnumerator;
	ModuleCollection() {/* */}
	void add(const ModuleEntry& me) { arrModules_.push_back(me); }
};

// Internal functions scoped in namespace
namespace JTI_Internal
{
	enum { MAXINJECTSIZE = 4096 };
	typedef DWORD (WINAPI *PGetCurrentDirectory)(DWORD, LPWSTR);
	typedef LPWSTR (WINAPI *PGetCommandLine)(void);

	// Internal structure used to retrieve data
	struct ExecData
	{
		DWORD rc;							// Last error from remote thread
		PGetCurrentDirectory pGCWD;			// Address of GetCurrentDirectory()
		PGetCommandLine pGCL;				// Address of GetCommandLine()
		wchar_t chWorkingDir[MAX_PATH+1];	// The buffer for GetCurrentDirectory()
		wchar_t chCommandLine[2048];		// The buffer for the command line
	};

	/**************************************************************************
	// AdjustPrivilege
	//
	// This changes the privilege level of the caller to add the given
	// attributes (typically SE_DEBUG).
	//
	**************************************************************************/
	inline bool AdjustPrivilege(LPCTSTR pPrivilege, DWORD dwAttributes)
	{
		// give this process permission to monitor and terminate services
		HANDLE hToken = NULL;
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
			return false;
			
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Attributes = dwAttributes;
		LookupPrivilegeValue(NULL, pPrivilege, &tp.Privileges[0].Luid);
		BOOL fRC = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL);
		CloseHandle(hToken);
		return (fRC == TRUE);
	}

	/**************************************************************************
	// RemoteFunction
	//
	// This function is injected into a remote process in order to
	// retrieve the command line and working directory.
	//
	**************************************************************************/
	#pragma runtime_checks( "", off )
	static DWORD __declspec(nothrow) __stdcall RemoteFunction(ExecData* pData)
	{
		// First get the working directory
		pData->pGCWD(sizeofarray(pData->chWorkingDir), pData->chWorkingDir);

		// Now retrieve the command line.
		wchar_t* pSrc = pData->pGCL();
		if (pSrc == NULL)
			pData->rc = 0;
		else
		{
			wchar_t* pDest = pData->chCommandLine;
			while (*pSrc)
				*pDest++ = *pSrc++;
			*pDest = L'\0';
			pData->rc = 1;
		}
		return 0;
	}
	#pragma runtime_checks( "", restore )
}

/**************************************************************************
// ModuleEnumerator
//
// This object is responsible for enumerating the module list for
// a given process.
//
**************************************************************************/
class ModuleEnumerator
{
// Class data
private:
	HINSTANCE helperInst_;
	// Typedefs for ToolHelp32 functions
	typedef HANDLE (WINAPI* PFNCREATETOOLHELP32SNAPSHOT)(DWORD dwFlags, DWORD th32ProcessID);
	typedef BOOL (WINAPI* PFNMODULE32FIRST)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);
	typedef BOOL (WINAPI* PFNMODULE32NEXT)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);
	// Typedefs for PSAPI.DLL functions
	typedef BOOL (WINAPI* PFNENUMPROCESSMODULES)(HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded);
	typedef DWORD (WINAPI* PFNGETMODULEFILENAMEEX)(HANDLE hProcess, HMODULE hModule, LPTSTR lpFilename, DWORD nSize);

	// PSAPI functions
	PFNENUMPROCESSMODULES pfnEnumProcessModules;
	PFNGETMODULEFILENAMEEX pfnGetModuleFileNameEx;
	// Toolhelp32 functions
	PFNCREATETOOLHELP32SNAPSHOT pfnCreateToolhelp32Snapshot;
	PFNMODULE32FIRST pfnModule32First;
	PFNMODULE32NEXT pfnModule32Next;

// Constructor
public:
	ModuleEnumerator() : helperInst_(0)
	{
		// Look for the TOOLHELP32 functions first; they are more reliable and fully supported.
		helperInst_ = ::LoadLibrary(_T("KERNEL32.DLL"));
		pfnCreateToolhelp32Snapshot=(PFNCREATETOOLHELP32SNAPSHOT)GetProcAddress(helperInst_,"CreateToolhelp32Snapshot");
#ifdef UNICODE
		pfnModule32First=(PFNMODULE32FIRST)GetProcAddress(helperInst_,"Module32FirstW");
		pfnModule32Next=(PFNMODULE32NEXT)GetProcAddress(helperInst_,"Module32NextW");
#else
		pfnModule32First=(PFNMODULE32FIRST)GetProcAddress(helperInst_,"Module32First");
		pfnModule32Next=(PFNMODULE32NEXT)GetProcAddress(helperInst_,"Module32Next");
#endif
        
		// If we couldn't find all the functions we are looking for, drop down to the PSAPI.DLL support.
		if (!pfnCreateToolhelp32Snapshot || !pfnModule32First || !pfnModule32Next)
		{
			::FreeLibrary(helperInst_);
			helperInst_ = ::LoadLibrary(_T("PSAPI.DLL"));
			if (helperInst_ != NULL)
			{
				pfnEnumProcessModules=(PFNENUMPROCESSMODULES)GetProcAddress(helperInst_,"EnumProcessModules");
#ifdef _UNICODE
				pfnGetModuleFileNameEx=(PFNGETMODULEFILENAMEEX)GetProcAddress(helperInst_,"GetModuleFileNameExW");
#else
				pfnGetModuleFileNameEx=(PFNGETMODULEFILENAMEEX)GetProcAddress(helperInst_,"GetModuleFileNameExA");
#endif
				if (!pfnEnumProcessModules || !pfnGetModuleFileNameEx)
				{
					::FreeLibrary(helperInst_);
					helperInst_ = NULL;
				}
			}
		}
	}

	~ModuleEnumerator()
	{
		if (helperInst_)
			::FreeLibrary(helperInst_);
	}

// Methods
public:
	ModuleCollection SnapshotProcess(DWORD pid)
	{
		if (helperInst_ == NULL)
			throw std::runtime_error("Unable to load ToolHelp or PSAPI.DLL");

		ModuleCollection modColl;

		// Cannot query pid 0 (system); always returns current process info.
		if (pid == 0)
			return modColl;

		MODULEENTRY32 me;
		ZeroMemory(&me, sizeof(me));
		me.dwSize = sizeof(MODULEENTRY32);

		// Enum using Toolhelp32
		if (pfnCreateToolhelp32Snapshot)
		{
			HANDLE htlSnap = (*pfnCreateToolhelp32Snapshot)(TH32CS_SNAPMODULE, pid);
			if (htlSnap == INVALID_HANDLE_VALUE && GetLastError() == ERROR_ACCESS_DENIED)
			{
				// Adjust our privileges if we can.
				if (JTI_Internal::AdjustPrivilege(SE_DEBUG_NAME, SE_PRIVILEGE_ENABLED))
					htlSnap = (*pfnCreateToolhelp32Snapshot)(TH32CS_SNAPMODULE, pid);
			}
			if (htlSnap == INVALID_HANDLE_VALUE ||
				!(*pfnModule32First)(htlSnap, &me))
				return modColl;

			do 
			{
				TranslateFilename(me.szExePath, me.szExePath);
				modColl.add(me);

			} while ((*pfnModule32Next)(htlSnap, &me) == TRUE);
			CloseHandle(htlSnap);
			return modColl;
		}

		// Otherwise use PSAPI.
		me.th32ProcessID = pid;
		HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, me.th32ProcessID);
		if (!hProcess)
		{
			if (JTI_Internal::AdjustPrivilege(SE_DEBUG_NAME, SE_PRIVILEGE_ENABLED))
			{
				hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, me.th32ProcessID);
				if (!hProcess)
					return modColl;
			}
			else
				return modColl;
		}

		DWORD cbNeeded;
		HMODULE arrModules[512];

		// EnumProcessModules returns an array of HMODULES
		if((*pfnEnumProcessModules)(hProcess, arrModules,sizeofarray(arrModules),&cbNeeded))
		{
			int modCount = cbNeeded/sizeof(HMODULE);
			for (int i = 0; i < modCount; ++i)
			{
				me.hModule = arrModules[i];
				me.modBaseAddr = (LPBYTE) me.hModule;
				InternalGetModuleName(hProcess, &me);
				modColl.add(me);
			}
		}
		return modColl;
	}

// Internal methods
private:
	bool InternalGetModuleName(HANDLE hProcess, MODULEENTRY32* pme)
	{	
		pme->szExePath[0] = _T('\0');
		pme->szModule[0] = _T('\0');

		// Open the pid and get the first module; this will be the executable.
		if (hProcess)
		{
			if (!(*pfnGetModuleFileNameEx)(hProcess, pme->hModule, pme->szExePath, sizeofarray(pme->szExePath)))
				return false;
		}

		// Copy the basename over
		if (pme->szExePath[0])
		{
			TranslateFilename(pme->szExePath, pme->szExePath);

			TCHAR* p = _tcsrchr(pme->szExePath, _T('\\'));
			if (p)
				lstrcpyn(pme->szModule,p+1,sizeofarray(pme->szModule));
			else
				lstrcpyn(pme->szModule, pme->szExePath, sizeofarray(pme->szModule));
		}

		return true;
	}

	void TranslateFilename(LPCTSTR szFilename, LPTSTR szWin32Name)
	{
		if (szFilename == NULL || szFilename[0] == _T('\0'))
			szFilename = _T("");
		
		// Check for "strange" filenames
		LPCTSTR pszInfo = _tcsstr(szFilename, _T("\\SystemRoot\\"));
		if (pszInfo == szFilename)
		{
			if (GetWindowsDirectory(szWin32Name, MAX_PATH) > 0)
			{
				lstrcat(szWin32Name, _T("\\"));
				lstrcat(szWin32Name, &szFilename[lstrlen(_T("\\SystemRoot\\"))]);
			}
			else
				lstrcpy(szWin32Name, szFilename);
		}
		else
		{
			pszInfo = _tcsstr(szFilename, _T("\\??\\"));
			if (pszInfo == szFilename)
				lstrcpy(szWin32Name, &szFilename[lstrlen(_T("\\??\\"))]);
			else
				lstrcpy(szWin32Name, szFilename);
		}
	}

};



/**************************************************************************
// ProcessEntry
//
// This object represents a single process
//
**************************************************************************/
class ProcessEntry
{
// Class data
private:
	PROCESSENTRY32 pe_;
	bool retrievedData_;
	tstring commandLine_;
	tstring currentWorkingDir_;

// Constructor/Copy
public:
	ProcessEntry(const PROCESSENTRY32& pe) : retrievedData_(false) { memcpy(&pe_, &pe, sizeof(pe)); }
	ProcessEntry(const ProcessEntry& rhs) 
	{ 
		memcpy(&pe_, &rhs.pe_, sizeof(rhs.pe_)); 
		retrievedData_ = rhs.retrievedData_;
		commandLine_ = rhs.commandLine_;
		currentWorkingDir_ = rhs.currentWorkingDir_;
	}
	ProcessEntry& operator=(const ProcessEntry& rhs)
	{
		if (this != &rhs) {
			memcpy(&pe_, &rhs.pe_, sizeof(rhs.pe_));
			retrievedData_ = rhs.retrievedData_;
			commandLine_ = rhs.commandLine_;
			currentWorkingDir_ = rhs.currentWorkingDir_;
		}
		return *this;
	}

// Properties
public:
	__declspec(property(get=get_PID)) DWORD ProcessID;
	__declspec(property(get=get_ParentPID)) DWORD ParentProcessID;
	__declspec(property(get=get_Name)) tstring Name;
	__declspec(property(get=get_ThreadCount)) int ThreadCount;
	__declspec(property(get=get_RefCount)) int RefCount;
	__declspec(property(get=get_BasePriority)) LONG BasePriority;
	__declspec(property(get=get_Modules)) ModuleCollection Modules;
	__declspec(property(get=get_CommandLine)) tstring CommandLine;
	__declspec(property(get=get_CurrentWorkingDir)) tstring WorkingDirectory;

// Methods
public:
	ModuleCollection get_Modules() const 
	{
		ModuleEnumerator me;
		return me.SnapshotProcess(get_PID());
	}

	int get_RefCount() const { return static_cast<int>(pe_.cntUsage); }
	int get_ThreadCount() const { return static_cast<int>(pe_.cntThreads); }
	DWORD get_PID() const { return pe_.th32ProcessID; }
	DWORD get_ParentPID() const { return pe_.th32ParentProcessID; }
	LONG get_BasePriority() const { return pe_.pcPriClassBase; }
	tstring get_Name() const { return pe_.szExeFile; }
	tstring get_CommandLine() const { const_cast<ProcessEntry*>(this)->RetrieveData(); return commandLine_; }
	tstring get_CurrentWorkingDir() const { const_cast<ProcessEntry*>(this)->RetrieveData(); return currentWorkingDir_; }

// Internal functions
private:
	bool RetrieveData()
	{
		if (retrievedData_)
			return true;
		retrievedData_ = true;

		JTI_Internal::ExecData exData;
		HMODULE hKernel = GetModuleHandleW(L"KERNEL32.DLL");

		exData.rc = 0;
		exData.chCommandLine[0] = L'\0';
		exData.chWorkingDir[0] = L'\0';
		exData.pGCWD = (JTI_Internal::PGetCurrentDirectory) GetProcAddress(hKernel, "GetCurrentDirectoryW");
		exData.pGCL = (JTI_Internal::PGetCommandLine) GetProcAddress(hKernel, "GetCommandLineW");

		if (!exData.pGCWD || !exData.pGCL) {
			return false;
		}

		// Adjust our privilages to get the data
		if (!JTI_Internal::AdjustPrivilege(SE_DEBUG_NAME, SE_PRIVILEGE_ENABLED))
			return false;

		HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
				PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, ProcessID);
		if (hProcess == NULL)
			return false;

		// Verify we didn't end up with an indirect JMP
		LPVOID lpvFunc = &JTI_Internal::RemoteFunction;
		if (*((LPBYTE)lpvFunc) == 0xE9) // JMP?
		{
			CloseHandle(hProcess);
			return false;
		}

		bool okToFree = true;

		// Allocate the memory which will execute our function.
		LPVOID pCode = VirtualAllocEx(hProcess, NULL, JTI_Internal::MAXINJECTSIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		LPVOID pData = VirtualAllocEx(hProcess, NULL, sizeof(JTI_Internal::ExecData), MEM_COMMIT, PAGE_READWRITE);

		if (pCode && pData)
		{
			// Copy our executable function and data there
			DWORD dwSizeCode, dwSizeData;
			if (WriteProcessMemory(hProcess, pCode, lpvFunc, JTI_Internal::MAXINJECTSIZE, &dwSizeCode) &&
				WriteProcessMemory(hProcess, pData, &exData, sizeof(exData), &dwSizeData) &&
				dwSizeCode == JTI_Internal::MAXINJECTSIZE && dwSizeData == sizeof(exData))
			{
				// Now execute the code in the process; we create a thread to do the work.
				DWORD dwThreadId;
				HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, 
						(LPTHREAD_START_ROUTINE)pCode,pData, 0, &dwThreadId);
				if (hThread != NULL)
				{
					// Wait for the thread to terminate in the process.  If the thread
					// never returns then don't free the code/data segment as it might cause
					// the target process to terminate.
					switch(WaitForSingleObject(hThread, 5000))
					{
						case WAIT_OBJECT_0:
							if (ReadProcessMemory(hProcess, pData, &exData, sizeof(exData), 0))
							{
#if defined(UNICODE) || defined(_UNICODE)
								commandLine_ = tstring(exData.chCommandLine);
								currentWorkingDir_ = tstring(exData.chWorkingDir);
#else
								commandLine_ = tstring(JTI_Internal::_W2A(exData.chCommandLine));
								currentWorkingDir_ = tstring(JTI_Internal::_W2A(exData.chWorkingDir));
#endif
							}
							break;
						case WAIT_TIMEOUT:
							okToFree = false;
							break;
						case WAIT_FAILED:
						case WAIT_ABANDONED:
						default:
							break;
					}
					CloseHandle(hThread);
				}
			}
		}
		
		// Cleanup
		if (okToFree)
		{
			if (pCode)
				VirtualFreeEx(hProcess, pCode, 0, MEM_RELEASE);
			if (pData)
				VirtualFreeEx(hProcess, pData, 0, MEM_RELEASE);
		}		
		CloseHandle(hProcess);
		FreeLibrary(hKernel);
		return !commandLine_.empty();
	}
};

/**************************************************************************
// ProcessCollection
//
// This object holds the list of enumerated processes.
//
**************************************************************************/
class ProcessCollection
{
// Class data
public:
	typedef std::vector<ProcessEntry>::const_iterator const_iterator;
private:
	std::vector<ProcessEntry> arrProcess_;

// Destructor
public:
	~ProcessCollection() {/* */}

// Accessors
public:
	bool empty() const { return arrProcess_.empty(); }
	int size() const { return static_cast<int>(arrProcess_.size()); }
	const_iterator begin() const { return arrProcess_.begin(); }
	const_iterator end() const { return arrProcess_.end(); }

// Constructor/Builders
private:
	friend class ProcessEnumerator;
	ProcessCollection() {/* */}
	void add(const ProcessEntry& pe) {arrProcess_.push_back(pe);}
};

/**************************************************************************
// ProcessEnumerator
//
// This object is responsible for enumerating the process list
//
**************************************************************************/
class ProcessEnumerator
{
// Class data
private:
	HINSTANCE helperInst_;
	// Typedefs for ToolHelp32 functions
	typedef HANDLE (WINAPI* PFNCREATETOOLHELP32SNAPSHOT)(DWORD dwFlags, DWORD th32ProcessID);
	typedef BOOL (WINAPI* PFNPROCESS32FIRST)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
	typedef BOOL (WINAPI* PFNPROCESS32NEXT)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
	// Typedefs for PSAPI.DLL functions
	typedef BOOL (WINAPI* PFNENUMPROCESSES)(DWORD* lpidProcess, DWORD cb, DWORD* cbNeeded);
	typedef BOOL (WINAPI* PFNENUMPROCESSMODULES)(HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded);
	typedef DWORD (WINAPI* PFNGETMODULEFILENAMEEX)(HANDLE hProcess, HMODULE hModule, LPTSTR lpFilename, DWORD nSize);
	// PSAPI functions
	PFNENUMPROCESSES pfnEnumProcesses;
	PFNENUMPROCESSMODULES pfnEnumProcessModules;
	PFNGETMODULEFILENAMEEX pfnGetModuleFileNameEx;
	// Toolhelp32 functions
	PFNCREATETOOLHELP32SNAPSHOT pfnCreateToolhelp32Snapshot;
	PFNPROCESS32FIRST pfnProcess32First;
	PFNPROCESS32NEXT pfnProcess32Next;

// Constructor
public:
	ProcessEnumerator() : helperInst_(0)
	{
		// Look for the TOOLHELP32 functions first; they are more reliable and fully supported.
		helperInst_ = LoadLibrary(_T("KERNEL32.DLL"));
		pfnCreateToolhelp32Snapshot=(PFNCREATETOOLHELP32SNAPSHOT)GetProcAddress(helperInst_,"CreateToolhelp32Snapshot");
#ifdef UNICODE
		pfnProcess32First=(PFNPROCESS32FIRST)GetProcAddress(helperInst_,"Process32FirstW");
		pfnProcess32Next=(PFNPROCESS32NEXT)GetProcAddress(helperInst_,"Process32NextW");
#else
		pfnProcess32First=(PFNPROCESS32FIRST)GetProcAddress(helperInst_,"Process32First");
		pfnProcess32Next=(PFNPROCESS32NEXT)GetProcAddress(helperInst_,"Process32Next");
#endif
        
		// If we couldn't find all the functions we are looking for, drop down to the PSAPI.DLL support.
		if (!pfnCreateToolhelp32Snapshot || !pfnProcess32First || !pfnProcess32Next)
		{
			::FreeLibrary(helperInst_);
			helperInst_ = ::LoadLibrary(_T("PSAPI.DLL"));
			if (helperInst_ != NULL)
			{
				pfnEnumProcesses=(PFNENUMPROCESSES)GetProcAddress(helperInst_,"EnumProcesses");
				pfnEnumProcessModules=(PFNENUMPROCESSMODULES)GetProcAddress(helperInst_,"EnumProcessModules");
#ifdef UNICODE
				pfnGetModuleFileNameEx=(PFNGETMODULEFILENAMEEX)GetProcAddress(helperInst_,"GetModuleFileNameExW");
#else
				pfnGetModuleFileNameEx=(PFNGETMODULEFILENAMEEX)GetProcAddress(helperInst_,"GetModuleFileNameExA");
#endif
				if (!pfnEnumProcesses || !pfnEnumProcessModules || !pfnGetModuleFileNameEx)
				{
					::FreeLibrary(helperInst_);
					helperInst_ = NULL;
				}
			}
			else
			{
				pfnEnumProcesses = NULL;
				pfnEnumProcessModules = NULL;
				pfnGetModuleFileNameEx = NULL;
			}
		}
	}

	~ProcessEnumerator()
	{
		if (helperInst_)
			::FreeLibrary(helperInst_);
	}

// Methods
public:
	bool CanEnumerateProcesses() const { return helperInst_ != NULL; }

	ProcessCollection Snapshot() const
	{
		if (helperInst_ == NULL)
			throw std::runtime_error("Unable to load ToolHelp or PSAPI.DLL");

		ProcessCollection pColl;
		PROCESSENTRY32 pe;
		ZeroMemory(&pe, sizeof(pe));
		pe.dwSize = sizeof(PROCESSENTRY32);

		// Enum using Toolhelp32
		if (pfnCreateToolhelp32Snapshot)
		{
			HANDLE htlSnap = (*pfnCreateToolhelp32Snapshot)(TH32CS_SNAPPROCESS, 0);
			if (htlSnap == INVALID_HANDLE_VALUE)
				return pColl;
			
			// Walk the process list
			if ((*pfnProcess32First)(htlSnap, &pe) == TRUE)
			{
				do 
				{
					pColl.add(pe);
				} while((*pfnProcess32Next)(htlSnap, &pe) == TRUE);

			}
			CloseHandle(htlSnap);
			return pColl;
		}

		// Enum using PSAPI.DLL
		DWORD cbNeeded;
		DWORD arrPids[512];

		// EnumProcesses returns an array of process IDs
		if(!(*pfnEnumProcesses)(arrPids,sizeofarray(arrPids),&cbNeeded))
			return pColl;

		int procCount = cbNeeded/sizeof(DWORD);
		for (int i = 0; i < procCount; i++)
		{
			pe.th32ProcessID = arrPids[i];
			InternalGetModuleName(pe.th32ProcessID, pe.szExeFile, sizeofarray(pe.szExeFile));
			pColl.add(pe);
		}
		return pColl;
	}

// Internal methods
private:
	bool InternalGetModuleName(DWORD pid, LPTSTR pszBuff, int nSize) const
	{	
		if (pid == 0)
			lstrcpyn(pszBuff,_T("[System Process]"),nSize);
		else
		{
			*pszBuff = _T('\0');

			// Open the pid and get the first module; this will be the executable.
			HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
			if (!hProcess)
			{
				// Adjust our privileges if we can.
				if (JTI_Internal::AdjustPrivilege(SE_DEBUG_NAME, SE_PRIVILEGE_ENABLED))
					hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
			}

			if (hProcess)
			{
				HMODULE hModule;
				DWORD nModuleCount;
				bool fRC = false;

				(*pfnEnumProcessModules)(hProcess, &hModule, sizeof(hModule), &nModuleCount);
				if (nModuleCount > 0) 
					fRC = ((*pfnGetModuleFileNameEx)(hProcess, hModule, pszBuff, nSize) != 0);
				CloseHandle(hProcess);
				if (!fRC)
				{
					if (pid < 10)
						lstrcpyn(pszBuff, _T("System"), nSize);
					else
						return false;
				}
			}
			else
				lstrcpyn(pszBuff, _T("unknown"), nSize);

			if (*pszBuff)
			{
				TCHAR* psz = _tcsrchr(pszBuff, _T('\\'));
				if (psz)
					lstrcpyn(pszBuff, psz+1, nSize);
			}
		}

		return true;
	}
};

}// JTI_Util

#endif // __PSLIST_H_INCL__
