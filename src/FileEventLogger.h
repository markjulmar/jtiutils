/****************************************************************************/
//
// FileEventLogger.h
//
// Logger object which can be used to log information in the form of 
// strings to a specific file.  The creator provides the path, and the file 
// base name.
//
// The logger will write to the file of basename_yyyymmdd.log until the
// date changes.  Once that happens, the file is closed and the logger will
// move to te next filename.
//
// Copyright (C) 1997-2004 JulMar Technology, Inc.
// All rights reserved
// 
/****************************************************************************/

#ifndef __EVENT_LOGGER_H_INCL__
#define __EVENT_LOGGER_H_INCL__

#ifndef  _MT
	#error "FileEventLogger class required multi-threaded CRT support"
#endif

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#pragma warning (disable:4702)
#include <string>
#include <deque>
#pragma warning (default:4702)
#include <Lock.h>
#include <Synchronization.h>

/*****************************************************************************/
// PC-Lint options
//
//lint -save
//
// Data not initialized in constructor
//lint -esym(1927, FileEventLogger::MaxSize, FileEventLogger::TruncateExistingData)
//lint -esym(1927, FileEventLogger::RenameFile)
//
// Data not initialized in constructor
//lint -esym(1401, FileEventLogger::MaxSize, FileEventLogger::TruncateExistingData)
//lint -esym(1401, FileEventLogger::RenameFile)
//
// Data not initialized by assignment operator
//lint -esym(1539, FileEventLogger::MaxSize, FileEventLogger::TruncateExistingData)
//lint -esym(1539, FileEventLogger::RenameFile)
//
// Public data member
//lint -esym(1925, FileEventLogger::MaxSize, FileEventLogger::TruncateExistingData)
//lint -esym(1925, FileEventLogger::RenameFile)
//
/*****************************************************************************/

namespace JTI_Util
{   
/****************************************************************************/
// FileEventLogger
//
// This class manages the event file logging.
//
/****************************************************************************/
class FileEventLogger : public LockableObject<MultiThreadModel>
{
// Class data
private:
	EventSynch isStopping_;
	EventSynch hasData_;
	std::deque<std::string> qData_;
	WORD dayOfWeek_;
	HANDLE logFile_;
	HANDLE threadHandle_;
	std::string dirName_;
	std::string baseName_;
	std::string renName_;
	std::string currName_;
	DWORD maxSize_;
	int fileIndex_;
	bool truncateExisting_;

// Destructor
public:
	FileEventLogger();
	virtual ~FileEventLogger();

// Properties
public:
	__declspec(property(get=get_MaxSize, put=set_MaxSize)) DWORD MaxSize;
	__declspec(property(get=get_Truncate, put=set_Truncate)) bool TruncateExistingData;
	__declspec(property(get=get_RenameFilespec, put=set_RenameFilespec)) const char* RenameFile;
	__declspec(property(get=get_LogDirectory, put=set_LogDirectory)) const char* LogDirectory;
	__declspec(property(get=get_CurrentFileName)) const char* CurrentFilename;

// Access functions
public:
	bool Start(const char* pszDirectory, const char* pszBaseName);
	void Stop() throw();
	void Flush();
	bool Log(const char* pszBuff);

	DWORD get_MaxSize() const { return maxSize_; }
	void set_MaxSize(DWORD maxSize) { maxSize_ = maxSize; }

	bool get_Truncate() const { return truncateExisting_; }
	void set_Truncate(bool f) { truncateExisting_ = f; }

	const char* get_CurrentFileName() const { return currName_.c_str(); }
	const char* get_RenameFilespec() const { return renName_.c_str(); }
	void set_RenameFilespec(const char* pszName) { renName_ = (pszName) ? pszName : ""; }

	const char* get_LogDirectory() const { return dirName_.c_str(); }
	void set_LogDirectory(const char* pszName) { dirName_ = (pszName) ? pszName : ""; }

// Constructor
private:
	std::string BuildFilename(const std::string& strName, const SYSTEMTIME& stNow) const;
	std::string GetNextFileName(const std::string& strDir, const std::string& fileSpec, const SYSTEMTIME& stNow);
	void CheckFile();
	bool CleanQueue();
	bool HitMaxSize() const;
	void WorkerThread();
	static UINT __stdcall StartThreadFunc(void* pParam) {
		reinterpret_cast<FileEventLogger*>(pParam)->WorkerThread();
		return 0;
	}
};

#ifdef __TRACE_DEBUG_H_INCL__
/****************************************************************************/
// FELHandler
//
// This class hooks into the TraceDebug facility and routes it through
// the given file log handler.
//
/****************************************************************************/
class FELHandler : public LogHandler
{
// Class data
private:
	FileEventLogger& fileLogger_;

// Constructor
public:
	FELHandler(FileEventLogger& fel) : fileLogger_(fel) {/* */}
	FELHandler(const FELHandler& rhs) : fileLogger_(rhs.fileLogger_) {/* */}
	FELHandler& operator=(const FELHandler& rhs) {
		if (this != &rhs) fileLogger_ = rhs.fileLogger_;
		return *this;
	}

// Overrides
public:
	virtual void OnLog(const LogElement& le)
	{
		fileLogger_.Log(le.ToString().c_str());
	}

	virtual void OnAssert(const AssertElement& ae)
	{
		fileLogger_.Log(ae.ToString().c_str());
	}
};
#endif

} // namespace JTI_Util

#endif // __EVENT_LOGGER_H_INCL__
