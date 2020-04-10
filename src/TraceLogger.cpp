/****************************************************************************/
//
// TraceLogger.cpp
//
// This class implements a tracing facility used primarily for debugging
// but capable of being turned on and off.  It uses the IOStreams library
// to implement the tracing for type-safe work.  In addition, it is thread-safe
// and provides an "observer" pattern dispatch mechanism allowing for multiple
// receivers of debug/trace events.
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
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include "JTIUtils.h"
#include "TraceLogger.h"
#include "MemoryMappedFile.h"
#include "DateTime.h"
#include <process.h>
#include <sys/stat.h>

/*----------------------------------------------------------------------------
    GLOBALS
-----------------------------------------------------------------------------*/
using namespace JTI_Util;

/*****************************************************************************
** Procedure:  TraceLogger_Base::TraceLogger_Base
** 
** Arguments:  void
** 
** Returns: void 
** 
** Description: Constructor for the trace logger
**
/****************************************************************************/
TraceLogger_Base::TraceLogger_Base() : 
	trcLevel_(0), listHandlers_(), listElements_(), Stopping_(false,true), HasData_(false,true),
	threadHandle_(INVALID_HANDLE_VALUE), lockElements_(), lockHandlers_(), stopOnAssert_(false), mapPrefix_()
{
}// TraceLogger_Base::TraceLogger_Base

/*****************************************************************************
** Procedure:  TraceLogger_Base::~TraceLogger_Base
** 
** Arguments:  void
** 
** Returns: void 
** 
** Description: Destructor for the trace logger
**
/****************************************************************************/
//lint --e{1740} Stop() clears variables
TraceLogger_Base::~TraceLogger_Base() 
{ 
	// Stop the logger
	Stop(); 

	// Clear any left-over log elements
	std::for_each(listElements_.begin(), listElements_.end(), stdx::delptr<InternalLogElement*>());
	listElements_.clear();

	// Delete all the log handlers
	std::for_each(listHandlers_.begin(), listHandlers_.end(), stdx::delptr<LogHandler*>());
	listHandlers_.clear(); 

}// TraceLogger_Base::~TraceLogger_Base

/*****************************************************************************
** Procedure:  TraceLogger_Base::addTraceHandler
** 
** Arguments:  LogHandler* pl - Log handler object to install
** 
** Returns: void 
** 
** Description: Adds a new log handler object to the trace handler list.
**
/****************************************************************************/
void TraceLogger_Base::addTraceHandler(LogHandler* pl)
{
	CCSWLock lockGuard(&lockHandlers_);

	if (threadHandle_ == INVALID_HANDLE_VALUE)
	{
		// Reset the stopping event
		Stopping_.ResetEvent();	//lint !e534

		// Create our thread
		unsigned int nThreadID;
		threadHandle_ = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, LogEventHandler, reinterpret_cast<void*>(this), 0, &nThreadID));
		if (threadHandle_ == 0)
			throw std::runtime_error("Problem starting trace handler thread");
		else
			::SetThreadPriority(threadHandle_, THREAD_PRIORITY_ABOVE_NORMAL);
	}
	
	trcLevel_ |= pl->LogLevel;
	listHandlers_.push_back(pl);

}// TraceLogger::addTraceHandler

/*****************************************************************************
** Procedure:  TraceLogger_Base::removeTraceHandler
** 
** Arguments:  LogHandler* pl - Log handler object to remove
** 
** Returns: void 
** 
** Description: Removes an existing log handler from the list
**
/****************************************************************************/
void TraceLogger_Base::removeTraceHandler(LogHandler* pl)
{
	CCSWLock lockGuard(&lockHandlers_);
	listHandlers_.erase(
		std::remove(listHandlers_.begin(), listHandlers_.end(), pl), 
		listHandlers_.end());
	lockGuard.Unlock();

	OnLogHandlerTraceLevelChanged();

}// TraceLogger_Base::removeTraceHandler

/*****************************************************************************
** Procedure:  TraceLogger_Base::LogEventHandler
** 
** Arguments:  lpParameter - LogElement instance
** 
** Returns: void
** 
** Description: This is the callback for the LogElement broadcaster
**
/****************************************************************************/
unsigned int __stdcall TraceLogger_Base::LogEventHandler(void* lp)
{
	reinterpret_cast<TraceLogger_Base*>(lp)->Runner();
	return 0;

}// TraceLogger_Base::LogEventHandler

/*****************************************************************************
** Procedure:  TraceLogger_Base::Stop
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: This stops the trace runner thread.
**
/****************************************************************************/
void TraceLogger_Base::Stop() 
{
	Stopping_.SetEvent();	//lint !e534
	if (threadHandle_ != INVALID_HANDLE_VALUE)	{
		WaitForSingleObject(threadHandle_,INFINITE); 	//lint !e534
		CloseHandle(threadHandle_);	//lint !e534
		threadHandle_ = INVALID_HANDLE_VALUE;
	}

}// TraceLogger_Base::Stop

/*****************************************************************************
** Procedure:  TraceLogger_Base::Runner
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: This is the runner for the logging engine
**
/****************************************************************************/
void TraceLogger_Base::Runner()
{
	HANDLE arrHandles[] = { Stopping_.get(), HasData_.get() };
	CCSLock<CriticalSectionLock> lockGuard(&lockElements_, false);

	for (;;)
	{
		// Wait on either the STOP event, or data arriving.
		DWORD waitCode = WaitForMultipleObjects(2, arrHandles, FALSE, INFINITE);
		if (waitCode == WAIT_OBJECT_0 || waitCode == WAIT_ABANDONED_0)
			break;

		// Lock for exclusive access to the queue.
		lockGuard.Lock();
		if (!listElements_.empty())
		{
			// Copy the elements off.
			LogElementList lstElems;
			lstElems.swap(listElements_);

			// Unlock and dispatch
			lockGuard.Unlock();
			std::for_each(lstElems.begin(), lstElems.end(), 
				std::bind1st(std::mem_fun(&TraceLogger_Base::DispatchSingle),this));

			// Now test the size again; if we have added one while processing, allow the thread
			// to continue working on new data.
			lockGuard.Lock();
			if (listElements_.empty())
				HasData_.ResetEvent();	//lint !e534
		}
		lockGuard.Unlock();
	}

	// Dispatch all the remaining elements.
	lockGuard.Lock();
	std::for_each(listElements_.begin(), listElements_.end(), 
		std::bind1st(std::mem_fun(&TraceLogger_Base::DispatchSingle),this));
	listElements_.clear();

}// TraceLogger_Base::Runner

/*****************************************************************************
** Procedure:  TraceLogger_Base::DispatchSingle
** 
** Arguments:  pile - Log element
** 
** Returns: void 
** 
** Description: This is called to dispatch a single log element
**
/****************************************************************************/
void TraceLogger_Base::DispatchSingle(InternalLogElement* pile)
{
	const LogElement* ple = dynamic_cast<const LogElement*>(pile);
	if (ple != NULL)
		BroadcastLogEvent(ple);
	else
	{
		const AssertElement* pae = dynamic_cast<const AssertElement*>(pile);
		if (pae)
			BroadcastAssert(pae);
	}
	delete pile;

}// TraceLogger_Base::DispatchSingle

/*****************************************************************************
** Procedure:  TraceLogger_Base::BroadcastLogEvent
** 
** Arguments:  le - Log element
** 
** Returns: void 
** 
** Description: This is called to send the log element to all the handlers
**
/****************************************************************************/
void TraceLogger_Base::BroadcastLogEvent(const LogElement* le)
{
	unsigned long nType = le->TraceLevel;

	CCSRLock lockGuard(&lockHandlers_);
	LogHandlerList::iterator itEnd = listHandlers_.end();
	for (LogHandlerList::iterator it = listHandlers_.begin(); it != itEnd; ++it)
	{
		unsigned long nType2 = (*it)->LogLevel;
		if ((nType == 0 && nType2 > 0) || (nType & nType2) > 0)
			(*it)->OnLog(*le);
	}

}// TraceLogger_Base::BroadcastLogEvent

/*****************************************************************************
** Procedure:  TraceLogger_Base::BroadcastAssert
** 
** Arguments:  ae - Assert element
** 
** Returns: void 
** 
** Description: This broadcasts failed assertions
**
/****************************************************************************/
void TraceLogger_Base::BroadcastAssert(const AssertElement* ae)
{
	CCSRLock lockGuard(&lockHandlers_);
	LogHandlerList::iterator itEnd = listHandlers_.end();
	for (LogHandlerList::iterator it = listHandlers_.begin(); it != itEnd; ++it)
		(*it)->OnAssert(*ae);

}// TraceLogger_Base::BroadcastAssert

/*****************************************************************************
** Procedure:  TraceLogger_Base::DumpFile
** 
** Arguments:  nLevel - Trace Level
**             fileName - name of the file to dump in the stream.
** 
** Returns: Success indicator
** 
** Description: This adds the contents of a file to the trace buffer.
**
/****************************************************************************/
bool TraceLogger_Base::DumpFile(unsigned long nLevel, const char* fileName, bool isBinary)
{
	// Get the total size of the file.
	struct _stat results;
	if (_stat(fileName, &results) != 0)
		return false;

	try
	{
		// Open the file as a memory mapped file.
		MemoryMappedFile file(fileName, 
			GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN);

		// Now dump it out to the stream.
		if (isBinary)
			InternalHexDump(nLevel, file.Buffer, static_cast<int>(file.Size));
		else
		{
			if (!listHandlers_.empty())
			{
				LogElement* ple = JTI_NEW LogElement(nLevel, 
						std::string(reinterpret_cast<const char*>(file.Buffer), 
						static_cast<int>(file.Size)));
				CCSLock<CriticalSectionLock> lockGuard(&lockElements_);
				if (threadHandle_ != INVALID_HANDLE_VALUE)
				{
					listElements_.push_back(ple);
					HasData_.SetEvent(); //lint !e534
				}
				else
					DispatchSingle(ple);
			}
		}	
	}
	catch(const std::exception&)
	{
		return false;
	}

	return true;

}// TraceLogger_Base::DumpFile

/*****************************************************************************
** Procedure:  TraceLogger_Base::addType
** 
** Arguments:  nLevel - Trace Level
**             textType - Trace description
**             textPrefix - Optional prefix to prepend
** 
** Returns: Success indicator
** 
** Description: This adds a new tracing type to the system.
**
/****************************************************************************/
bool TraceLogger_Base::addType(unsigned long nLevel, const std::string& textType, const std::string& textPrefix) 
{ 
	return (mapPrefix_.insert(std::make_pair(nLevel,
			TraceLogType(nLevel,textType,textPrefix))).second);	

}// TraceLogger_Base::addType

/*****************************************************************************
** Procedure:  TraceLogger_Base::removeType
** 
** Arguments:  nLevel - Trace Level
** 
** Returns: Success indicator
** 
** Description: This removes a tracing type from the system.
**
/****************************************************************************/
void TraceLogger_Base::removeType(unsigned long nLevel) 
{ 
	mapPrefix_.erase(nLevel); /*lint !e534*/ 

}// TraceLogger_Base::removeType

/*****************************************************************************
** Procedure:  TraceLogger_Base::get_TypeInfo
** 
** Arguments:  nLevel - Trace Level
** 
** Returns: TraceLogType
** 
** Description: This retrieves a trace log type.
**
/****************************************************************************/
TraceLogType TraceLogger_Base::get_TypeInfo(unsigned long nLevel) const 
{
	std::map<unsigned long, TraceLogType>::const_iterator it = mapPrefix_.find(nLevel);
	return (it != mapPrefix_.end()) ? it->second : TraceLogType();

}// TraceLogger_Base::get_TypeInfo

/*****************************************************************************
** Procedure:  TraceLogger_Base::get_Prefix
** 
** Arguments:  nLevel - Trace Level
** 
** Returns: String representing prefix
** 
** Description: This returns the prefix for a logtype
**
/****************************************************************************/
std::string TraceLogger_Base::get_Prefix(unsigned long nLevel) const 
{
	std::map<unsigned long, TraceLogType>::const_iterator it = mapPrefix_.find(nLevel);
	return (it != mapPrefix_.end()) ? it->second.get_Prefix() : std::string();

}// TraceLogger_Base::get_Prefix

/*****************************************************************************
** Procedure:  TraceLogger_Base::OnLogHandlerTraceLevelChanged
** 
** Arguments:  void
** 
** Returns: void 
** 
** Description: This is called by owned LogHandlers when their tracing
**              level is changed.
**
/****************************************************************************/
void TraceLogger_Base::OnLogHandlerTraceLevelChanged()
{
	unsigned long TraceLevel = 0;

	CCSRLock lockGuard(&lockHandlers_);
	LogHandlerList::iterator itEnd = listHandlers_.end();
	for (LogHandlerList::iterator it = listHandlers_.begin(); it != itEnd; ++it)
		TraceLevel |= (*it)->LogLevel;

	trcLevel_ = TraceLevel;

}// TraceLogger_Base::OnLogHandlerTraceLevelChanged

/*****************************************************************************
** Procedure:  TraceLogger_Base::InternalHexDump
** 
** Arguments:  nLevel - Trace level
**             pBuffer - Buffer to dump
**             nSize - Size of buffer
** 
** Returns: void 
** 
** Description: This dumps the given buffer in hex.
**
/****************************************************************************/
void TraceLogger_Base::InternalHexDump(unsigned long nLevel, const void* pBuffer, int nSize)
{
	const char* lpByte = reinterpret_cast<const char*>(pBuffer); char b[17];
	int nCount = 0, nLine = 0;
	std::ostringstream ostm;

	ostm << "Hex Dump of block @ " << pBuffer << " for " << nSize << " bytes.\r\n";

	while (nCount < nSize)
	{
		ostm << std::setw(8) << std::setfill('0') << nLine << "  ";

		int i;
		for (i = 0; i < 16; ++i) {
			if (nSize-nCount > 0) {
				b[i] = (*lpByte++);
				nCount++;
			}
			else
				b[i] = 0;
			ostm << std::hex << std::setw(2) << std::setfill('0') << (b[i]&0xff) << " ";
		}
	
		nLine = nCount;
                
		for (i = 0; i < 16; ++i)
			if (b[i] < 0x20 || b[i] > 0x7d)
				b[i] = '.';
		b[16] = '\0';
		ostm << b << "\r\n";
	}

	if (!listHandlers_.empty())
	{
		LogElement* ple = JTI_NEW LogElement(nLevel, ostm.str());
		CCSLock<CriticalSectionLock> lockGuard(&lockElements_);
		if (threadHandle_ != INVALID_HANDLE_VALUE)
		{
			listElements_.push_back(ple);
			HasData_.SetEvent(); //lint !e534
		}
		else
			DispatchSingle(ple);
	}

}// TraceLogger_Base::InternalHexDump

/*****************************************************************************
** Procedure:  TraceLogger_Base::InternalTrace
** 
** Arguments:  nLevel - Trace level
**             stmInfo - Trace info string
** 
** Returns: void 
** 
** Description: This adds the given buffer to the trace log
**
/****************************************************************************/
void TraceLogger_Base::InternalTrace(unsigned long nLevel, const std::string& stmInfo)
{
	if (!listHandlers_.empty())
	{
		LogElement* ple = JTI_NEW LogElement(nLevel, stmInfo);
		CCSLock<CriticalSectionLock> lockGuard(&lockElements_);
		if (threadHandle_ != INVALID_HANDLE_VALUE)
		{
			listElements_.push_back(ple);
			HasData_.SetEvent();	//lint !e534
		}
		else
			DispatchSingle(ple);
	}
}// TraceLogger_Base::InternalTrace

/*****************************************************************************
** Procedure:  TraceLogger_Base::AssertFailed
** 
** Arguments:  pszFile - File which assert happened in
**             nLine - Line number
**             stmInfo - Trace info string
** 
** Returns: void 
** 
** Description: This adds the given buffer to the assert trace log
**
/****************************************************************************/
void TraceLogger_Base::AssertFailed(const char* pszFile, int nLine, const std::string& stmInfo)
{
	// If we are to stop; do it now.
	if (stopOnAssert_) 
		::DebugBreak();

	if (!listHandlers_.empty())
	{
		AssertElement* pae = JTI_NEW AssertElement(pszFile, nLine, stmInfo);
		CCSLock<CriticalSectionLock> lockGuard(&lockElements_);
		if (threadHandle_ != INVALID_HANDLE_VALUE)
		{
			listElements_.push_back(pae);
			HasData_.SetEvent();	//lint !e534
		}
		else
			DispatchSingle(pae);
	}

}// TraceLogger_Base::AssertFailed

/*****************************************************************************
** Procedure:  LogHandler::~LogHandler
** 
** Arguments:  void
** 
** Returns: void 
** 
** Description: Destructor for the log handler
**
/****************************************************************************/
LogHandler::~LogHandler() 
{
}// LogHandler::~LogHandler

/*****************************************************************************
** Procedure:  LogHandler::set_LogLevel
** 
** Arguments:  logLevel - New log level
** 
** Returns: void 
** 
** Description: This changes the logging level for this handler
**
/****************************************************************************/
void LogHandler::set_LogLevel(unsigned int logLevel) 
{ 
	logLevel_ = logLevel; 
	TraceLogger::Instance().OnLogHandlerTraceLevelChanged();

}// LogHandler::set_LogLevel

/*****************************************************************************
** Procedure:  LogElement::BuildString
** 
** Arguments:  void
** 
** Returns: void 
** 
** Description: This builds the logged string
**
/****************************************************************************/
void LogElement::BuildString()
{
	std::ostringstream ostm;
	ostm << std::dec << std::setfill('0') << std::setw(4) << std::setiosflags(std::ios::right) << DateTime.wYear << '-'
		 << std::dec << std::setfill('0') << std::setw(2) << std::setiosflags(std::ios::right) << DateTime.wMonth << '-'
		 << std::dec << std::setfill('0') << std::setw(2) << std::setiosflags(std::ios::right) << DateTime.wDay << ' '
		 << std::dec << std::setfill('0') << std::setw(2) << std::setiosflags(std::ios::right) << DateTime.wHour << ':'
		 << std::dec << std::setfill('0') << std::setw(2) << std::setiosflags(std::ios::right) << DateTime.wMinute << ':'
		 << std::dec << std::setfill('0') << std::setw(2) << std::setiosflags(std::ios::right) << DateTime.wSecond << '.'
		 << std::dec << std::setfill('0') << std::setw(4) << std::setiosflags(std::ios::right) << DateTime.wMilliseconds
		 << " [" << std::dec << std::setfill('0') << std::setw(4) << std::setiosflags(std::ios::right) << ThreadId << "] ";

	BuiltString_.reserve(Text.length() + 75);
	BuiltString_ = ostm.str();
	BuiltString_ += TraceLogger::Instance().get_Prefix(TraceLevel);
	BuiltString_ += Text;

}// LogElement::BuildString

/*****************************************************************************
** Procedure:  AssertElement::BuildString
** 
** Arguments:  void
** 
** Returns: void 
** 
** Description: This builds the logged string
**
/****************************************************************************/
void AssertElement::BuildString()
{
	std::ostringstream ostm;
	ostm << std::dec << std::setfill('0') << std::setw(4) << std::setiosflags(std::ios::right) << DateTime.wYear << '-'
		<< std::dec << std::setfill('0') << std::setw(2) << std::setiosflags(std::ios::right) << DateTime.wMonth << '-'
		<< std::dec << std::setfill('0') << std::setw(2) << std::setiosflags(std::ios::right) << DateTime.wDay << ' '
		<< std::dec << std::setfill('0') << std::setw(2) << std::setiosflags(std::ios::right) << DateTime.wHour << ':'
		<< std::dec << std::setfill('0') << std::setw(2) << std::setiosflags(std::ios::right) << DateTime.wMinute << ':'
		<< std::dec << std::setfill('0') << std::setw(2) << std::setiosflags(std::ios::right) << DateTime.wSecond << '.'
		<< std::dec << std::setfill('0') << std::setw(4) << std::setiosflags(std::ios::right) << DateTime.wMilliseconds
		<< " [" << std::dec << std::setfill('0') << std::setw(4) << std::setiosflags(std::ios::right) << ThreadId << "] "
		<< "Assert failed @ " << Filename << " (" << Line << ") ";

	BuiltString_.reserve(255 + Text.length());
	BuiltString_ = ostm.str();
	BuiltString_ += Text;

}// AssertElement::BuildString

