/****************************************************************************/
//
// FileEventLogger.cpp
//
// Logger object which can be used to log information in the form of 
// strings to a specific file.  The creator provides the path, and the file 
// base name.
//
// The logger will write to the file of basename_yyyymmdd.log until the
// date changes.  Once that happens, the file is closed and the logger will
// move to te next filename.
//
// Copyright (C) 1997-2004 JulMar Technology, Inc.   All rights reserved
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
#include "FileEventLogger.h"
#include "TraceLogger.h"
#include <string>
#include <io.h>
#include <process.h>
#include <direct.h>
#undef max
#include <limits>

using namespace JTI_Util;

/*----------------------------------------------------------------------------
	LINT OPTIONS
-----------------------------------------------------------------------------*/
//lint --e{1924}  allow C-style casts
//lint -esym(534, CloseHandle, MoveFileA, SetEndOfFile, WaitForSingleObject)
//lint -esym(1740, FileEventLogger::logFile_, FileEventLogger::threadHandle_, FileEventLogger::RenameFile)

/*****************************************************************************
** Procedure:  FileEventLogger::FileEventLogger
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: Constructor for the file logger object
**
/****************************************************************************/
FileEventLogger::FileEventLogger() : LockableObject<MultiThreadModel>(),
	isStopping_(false,true), hasData_(false,true),  qData_(), dayOfWeek_(99), 
	logFile_(INVALID_HANDLE_VALUE), threadHandle_(INVALID_HANDLE_VALUE),
	dirName_(""), baseName_(""), renName_(""), currName_(""), maxSize_(0), fileIndex_(0),
	truncateExisting_(false)
{
}// FileEventLogger::FileEventLogger 

/*****************************************************************************
** Procedure:  FileEventLogger::~FileEventLogger
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: Destructor for the file logger object
**
/****************************************************************************/
FileEventLogger::~FileEventLogger() 
{ 
	Stop();

}// FileEventLogger::~FileEventLogger

/*****************************************************************************
** Procedure:  FileEventLogger::Log
** 
** Arguments: unsigned int logLevel - Log level
**            const char* pszData - Data to log 
** 
** Returns: TRUE/FALSE result 
** 
** Description: This logs the data to the logfile
**
/****************************************************************************/
bool FileEventLogger::Log(const char* pszData)
{
	std::string s; s.reserve(lstrlenA(pszData)+2);
	s = ((pszData) ? pszData : "");
	s += "\r\n";

	CCSLock<FileEventLogger> lockGuard(this);
	qData_.push_back(s);
	JTI_VERIFY(hasData_.SetEvent());

	return true;

}// FileEventLogger::Log

/*****************************************************************************
** Procedure:  FileEventLogger::Flush
** 
** Arguments:  void
** 
** Returns: void 
** 
** Description: This forces all outstanding data to be written to the file
**              and the file handle is closed.
**
/****************************************************************************/
void FileEventLogger::Flush()
{
	if (CleanQueue())
	{
		// Force the file to synch
		FlushFileBuffers(logFile_);

		// Close the file.
		JTI_VERIFY(CloseHandle(logFile_) != 0);
		logFile_ = INVALID_HANDLE_VALUE;
	}

}// FileEventLogger::Flush

/*****************************************************************************
** Procedure:  FileEventLogger::WorkerThread
** 
** Arguments:  void
** 
** Returns: void 
** 
** Description: This is the main worker thread for the logger 
**
/****************************************************************************/
void FileEventLogger::WorkerThread()
{
	HANDLE arrHandles[2] = {
		isStopping_.get(),
		hasData_.get()
	};

	for (;;)
	{
		// Wait for one of the objects to signal
		DWORD rc = WaitForMultipleObjects(2, arrHandles, FALSE, INFINITE);
		CleanQueue();

		if (qData_.empty())
			hasData_.ResetEvent();	//lint !e534

		// If we are exiting due to the signal, then do so now.
		if (rc == WAIT_OBJECT_0 || rc == WAIT_ABANDONED_0)
			break;
	}

	// Flush the queue
	Flush();

}// FileEventLogger::WorkerThread

/*****************************************************************************
** Procedure:  FileEventLogger::CleanQueue
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: This removes all elements from the queue and writes them
**              to our log.
**
/****************************************************************************/
bool FileEventLogger::CleanQueue()
{
	// Reopen our file if necessary
	CheckFile();

	// Write to the file if it's open
	CCSLock<FileEventLogger> lockGuard(this);
	if (logFile_ == INVALID_HANDLE_VALUE) {
		qData_.clear();
		return false;
	}

    // Copy off the current data into a private queue and unlock.
    std::deque<std::string> queue;
	queue.swap(qData_);
	lockGuard.Unlock();

    // Get the file size of the current logfile.  If the hi file size is set then
    // assume max size reached.
    DWORD dwFileSize = 0;
    if (maxSize_ > 0)
    {
        DWORD dwHiSize;
        dwFileSize = GetFileSize(logFile_, &dwHiSize); 
        if (dwHiSize > 0)
            dwFileSize = std::numeric_limits<DWORD>::max();

        // Reopen the file immediately if necessary.
		if (dwFileSize >= maxSize_)
		{
			CheckFile();
			if (logFile_ == INVALID_HANDLE_VALUE)
				return false;
            dwFileSize = 0;
		}
    }

    // Write out our entries.
	while (!queue.empty())
	{
		const std::string& s = queue.front();
		DWORD dwLength = static_cast<DWORD>(s.length()), dwWritten = 0;
		JTI_VERIFY(WriteFile(logFile_, s.c_str(), dwLength, &dwWritten, NULL) != 0);
		JTI_ASSERT(dwWritten == dwLength);

		if (maxSize_ > 0)
        {
            if ((dwFileSize+dwWritten) < dwFileSize || (dwFileSize+dwWritten) >= maxSize_)
		    {
    			CheckFile();
	    		if (logFile_ == INVALID_HANDLE_VALUE)
		    		return false;
                dwFileSize = 0;
		    }
            else
          	    dwFileSize += dwWritten;
        }
		queue.pop_front();
	}
	return true;

}// FileEventLogger::CleanQueue

/*****************************************************************************
** Procedure:  FileEventLogger::CheckFile
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: This checks the file and reopens it if necessary.
**
/****************************************************************************/
void FileEventLogger::CheckFile()
{
	// Lock access to the file handle
	CCSLock<FileEventLogger> lockGuard(this);

	bool fReopen = (logFile_ == INVALID_HANDLE_VALUE);
		
	// See if our date has changed; if so, open a new log.
	SYSTEMTIME stNow; ::GetLocalTime(&stNow);
	if (dayOfWeek_ != stNow.wDay)
	{
		fReopen = true;
		fileIndex_ = 0;
	}
	else if (HitMaxSize())
	{
		fReopen = true;
	}

	// Need to re-open the file?
	if (fReopen)
	{
		// Reset
		fReopen = false;

		// If we already have a log file, close it here.
		if (logFile_ != INVALID_HANDLE_VALUE) {
			JTI_VERIFY(::CloseHandle(logFile_) != 0);
			logFile_ = INVALID_HANDLE_VALUE;
		}

		// Get the directory to work with
		std::string strDir = dirName_;
		if (!strDir.empty())
		{
			if (_access(strDir.c_str(),0) == -1) {
				if (_mkdir(strDir.c_str())== -1)
					strDir = "";
			}
		}

		// Check to see if we are going to rename the file to a new name.
		if (!currName_.empty() && !renName_.empty())
		{
			MoveFileExA(currName_.c_str(), GetNextFileName(strDir, renName_, stNow).c_str(), 
				MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);
		}

		// Set the new day of week
		dayOfWeek_ = stNow.wDay;

		// Get the next filename
		currName_ = GetNextFileName(strDir, baseName_, stNow);

		// Open the file.
		logFile_ = ::CreateFileA(currName_.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (logFile_ != INVALID_HANDLE_VALUE)
		{
			if (truncateExisting_)
			{
				SetFilePointer(logFile_, 0L, NULL, FILE_BEGIN);
				SetEndOfFile(logFile_);
			}
			else
				SetFilePointer(logFile_, 0L, NULL, FILE_END);
		}
	}
}//

/*****************************************************************************
** Procedure:  FileEventLogger::GetNextFileName
** 
** Arguments:  void
** 
** Returns: Pointer to buffer
** 
** Description: This creates the next filename in sequence
**
/****************************************************************************/
std::string FileEventLogger::GetNextFileName(const std::string& strDir, const std::string& fileSpec, const SYSTEMTIME& stNow)
{
	// Build the filename.
	std::ostringstream stm;
	for(; fileIndex_ >= 0; ++fileIndex_) {
		stm.str("");
		stm << strDir;
		if (!strDir.empty() && strDir[strDir.length()-1] != '\\')
			stm << '\\';
		stm << BuildFilename(fileSpec, stNow).c_str();
		if (truncateExisting_ || _access(stm.str().c_str(),0) == -1 || fileSpec.find("%c") == std::string::npos)
			break;
	}

	// Reset and throw an exception to notify the caller.
	if (fileIndex_ < 0)	{
		fileIndex_ = 0;
		throw jtiexception(-1, "Ran out of file indexes for file logger.");
	}

	return stm.str();

}// FileEventLogger::GetNextFileName

/*****************************************************************************
** Procedure:  FileEventLogger::HitMaxSize
** 
** Arguments:  void
** 
** Returns: true/false
** 
** Description: This checks the file size to see if we have hit our max size.
**
/****************************************************************************/
bool FileEventLogger::HitMaxSize() const
{
	if (maxSize_ > 0 && logFile_ != INVALID_HANDLE_VALUE)
		return (GetFileSize(logFile_, NULL) >= maxSize_);
	return false;

}// FileEventLogger::HitMaxSize

/*****************************************************************************
** Procedure:  FileEventLogger::BuildFilename
** 
** Arguments:  stNow - Current Time
** 
** Returns: string of new filename
** 
** Description: This builds the string for the filename using strftime rules
**
/****************************************************************************/
//lint -e{661}	//lint --e(661) Possible access outside bounds of pointer array
std::string FileEventLogger::BuildFilename(const std::string& strName, const SYSTEMTIME& stNow) const
{
	std::ostringstream ostm;

	if (strName.empty())
	{
		ostm << std::dec << std::setfill('0') << std::setw(4) << std::setiosflags(std::ios::right) << stNow.wYear
			 << std::dec << std::setfill('0') << std::setw(2) << std::setiosflags(std::ios::right) << stNow.wMonth
			 << std::dec << std::setfill('0') << std::setw(2) << std::setiosflags(std::ios::right) << stNow.wDay
			 << ".log";
	}
	else
	{
		std::string::const_iterator itEnd = strName.end();
		for (std::string::const_iterator it = strName.begin(); it != itEnd; ++it)
		{
			if ((*it) == '%')
			{
				if (++it == itEnd)
					break;
				switch ((*it))
				{
					case 'd': // Day of month as decimal number (01 – 31) 
						ostm << std::dec << std::setfill('0') << std::setw(2) << std::setiosflags(std::ios::right) << stNow.wDay;
						break;
					case 'm': // Month as a decimal number
						ostm << std::dec << std::setfill('0') << std::setw(2) << std::setiosflags(std::ios::right) << stNow.wMonth;
						break;
					case 'y': // Year without century, as decimal number (00 – 99)
						ostm << std::dec << std::setfill('0') << std::setw(2) << std::setiosflags(std::ios::right) << stNow.wYear - 2000;
						break;
					case 'Y': // Year with century
						ostm << std::dec << std::setfill('0') << std::setw(4) << std::setiosflags(std::ios::right) << stNow.wYear;
						break;
					case 'c': // Counter
						if (fileIndex_ < 999)
							ostm << std::dec << std::setfill('0') << std::setw(3) << std::setiosflags(std::ios::right) << fileIndex_;
						else
							ostm << std::dec << fileIndex_;
						break;
					case '%': // Percent sign
						ostm << '%';
						break;
					default:
						ostm << (*it);
						break;
				}
			}
			else
				ostm << (*it);
		}
	}
	return ostm.str();

}// FileEventLogger::BuildFilename

/*****************************************************************************
** Procedure:  FileEventLogger::Start
** 
** Arguments:  void
** 
** Returns: void 
** 
** Description: This starts the file logger
**
/****************************************************************************/
bool FileEventLogger::Start(const char* pszDirectory, const char* pszBaseName)
{
	if (threadHandle_ == INVALID_HANDLE_VALUE)
	{
		dirName_ = pszDirectory;
		baseName_ = pszBaseName;

		// Start the thread
		UINT wThreadID = 0;
		threadHandle_ = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, FileEventLogger::StartThreadFunc, reinterpret_cast<void*>(this), 0, &wThreadID));
		if (threadHandle_ != 0)
			SetThreadPriority(threadHandle_, THREAD_PRIORITY_BELOW_NORMAL);
		else
			threadHandle_ = INVALID_HANDLE_VALUE;
	}
	return (threadHandle_ == INVALID_HANDLE_VALUE);

}// FileEventLogger::Start

/*****************************************************************************
** Procedure:  FileEventLogger::Stop
** 
** Arguments:  void
** 
** Returns: void 
** 
** Description: This stops the event logger
**
/****************************************************************************/
void FileEventLogger::Stop() throw()
{
	isStopping_.SetEvent();	//lint !e534
	if (threadHandle_ != INVALID_HANDLE_VALUE)
	{
		if (threadHandle_ != GetCurrentThread())
			WaitForSingleObject(threadHandle_, INFINITE);
		JTI_VERIFY(CloseHandle(threadHandle_) != 0);
		threadHandle_ = INVALID_HANDLE_VALUE;
	}

}// FileEventLogger::Stop
