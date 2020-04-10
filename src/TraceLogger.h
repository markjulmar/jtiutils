/****************************************************************************/
//
// TraceLogger.h
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

#ifndef __TRACE_DEBUG_H_INCL__
#define __TRACE_DEBUG_H_INCL__

#pragma warning (disable:4239)

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#pragma warning (disable:4702)
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <sstream>
#include <iomanip>
#pragma warning (default:4702)
#include <stlx.h>
#include <synchronization.h>
#include <rwlock.h>
#include <lock.h>
#include <singletonregistry.h>

/*****************************************************************************/
// PC-Lint options
//
//lint -save
//
// Allow implicit conversions
//lint -esym(1931, InternalLogElement*)
//
// Ignore public data properties
//lint -esym(1925, LogHandler::LogLevel)
//lint -esym(1925, InternalLogElement::ThreadId, InternalLogElement::Text, InternalLogElement::DateTime)
//lint -esym(1925, LogElement::TraceLevel)
//lint -esym(1925, AssertElement::Filename, AssertElement::Line)
//lint -esym(1925, TraceLogger_Base::TraceLevel, TraceLogger_Base::DebugBreak)
//
// Data did not appear in constructor initializer list
//lint -esym(1927, LogHandler::LogLevel)
//lint -esym(1927, InternalLogElement::ThreadId, InternalLogElement::Text, InternalLogElement::DateTime)
//lint -esym(1927, LogElement::TraceLevel)
//lint -esym(1927, AssertElement::Filename, AssertElement::Line)
//lint -esym(1927, TraceLogger_Base::TraceLevel, TraceLogger_Base::DebugBreak)
//
// Data not initialized by constructor
//lint -esym(1401, LogHandler::LogLevel)
//lint -esym(1401, InternalLogElement::ThreadId, InternalLogElement::Text, InternalLogElement::DateTime)
//lint -esym(1401, LogElement::TraceLevel)
//lint -esym(1401, AssertElement::Filename, AssertElement::Line)
//lint -esym(1401, TraceLogger_Base::TraceLevel, TraceLogger_Base::DebugBreak)
//
// Allow private constructor
//lint -esym(1704, TraceLogType::TraceLogType)
//lint -esym(1704, TraceLogger_Base::TraceLogger_Base)
//
// Allow conversions in macro
//lint -emacro(1058, JTI_TRACE, JTI_TRACEX, JTI_ASSERT, JTI_DUMP, JTI_DUMPX)
//
// Macro could become const variable
//lint -emacro(1923, JTI_TRACE, JTI_TRACEX, JTI_ASSERT, JTI_DUMP, JTI_DUMPX)
//
// do..while(0)
//lint -emacro(717, JTI_ASSERT)
//
/*****************************************************************************/

namespace JTI_Util
{
/****************************************************************************/
// InternalLogElement
//
// Internal structure which represents a single log element
//
/****************************************************************************/
class InternalLogElement
{
public:
// Public data
	DWORD ThreadId;
	std::string Text;
	SYSTEMTIME DateTime;	
protected:
	mutable std::string BuiltString_;

// Constructor
public:
	InternalLogElement(const std::string& sText) : ThreadId(::GetCurrentThreadId()), Text(sText), BuiltString_("") { ::GetLocalTime(&DateTime); }
	InternalLogElement(const InternalLogElement& le) : ThreadId(le.ThreadId), Text(le.Text), DateTime(le.DateTime), BuiltString_(le.BuiltString_) {/* */}
	InternalLogElement& operator=(const InternalLogElement& le) {
		if (this != &le) {
			DateTime = le.DateTime;
			ThreadId = le.ThreadId;
			Text = le.Text;
			BuiltString_ = le.BuiltString_;
		}
		return *this;
	}
	virtual ~InternalLogElement() {/* */}
	virtual const std::string& ToString() const = 0;
};

/****************************************************************************/
// LogElement
//
// Internal structure which represents a single log element
//
/****************************************************************************/
class LogElement : public InternalLogElement
{
public:
// Public data
	unsigned long TraceLevel;

// Constructor
	LogElement(unsigned long nTraceLevel, const std::string& sText) : InternalLogElement(sText), TraceLevel(nTraceLevel) {/* */}
	LogElement(const LogElement& le) : InternalLogElement(le), TraceLevel(le.TraceLevel) {/* */}
	LogElement& operator=(const LogElement& le) {
		if (this != &le) {
			InternalLogElement::operator=(le);
			TraceLevel = le.TraceLevel;
		}
		return *this;
	}

// Accessors
public:
	virtual const std::string& ToString() const 
	{
		if (BuiltString_.empty())
			const_cast<LogElement*>(this)->BuildString();
		return BuiltString_;
	}
private:
	void BuildString();
};

/****************************************************************************/
// AssertElement
//
// Internal structure which represents a single log element
//
/****************************************************************************/
class AssertElement : public InternalLogElement
{
public:
// Public data
	std::string Filename;
	int Line;

// Constructor
	AssertElement(const char* pszFile, int nLine, const std::string& sText) : 
		InternalLogElement(sText), Filename(""), Line(nLine)
	{
		const char* pFile = strrchr(pszFile,'\\');
		Filename = (pFile) ? pFile+1 : pszFile;
	}
	AssertElement(const AssertElement& ae) : InternalLogElement(ae), Filename(ae.Filename), Line(ae.Line) {/* */}
	AssertElement& operator=(const AssertElement& ae) {
		if (this != &ae) {
			InternalLogElement::operator=(ae);
			Filename = ae.Filename;
			Line = ae.Line;
		}
		return *this;
	}

// Accessors
public:
	virtual const std::string& ToString() const 
	{
		if (BuiltString_.empty())
			const_cast<AssertElement*>(this)->BuildString();
		return BuiltString_;
	}
private:
	void BuildString();
};

/*****************************************************************************
// LogHandler
//
// This interface represents the contract which log entry notifications
// are performed through.
//
*****************************************************************************/
class LogHandler
{
// Constructor
public:
	LogHandler() : logLevel_(0) {/* */}
	virtual ~LogHandler() = 0;

// Overrides
public:
	virtual void OnLog(const LogElement&) = 0;
	virtual void OnAssert(const AssertElement&) = 0;

// Properties
public:
	__declspec(property(get=get_LogLevel, put=set_LogLevel)) unsigned int LogLevel;

// Accessors
public:
	unsigned int get_LogLevel() const { return logLevel_; }
	void set_LogLevel(unsigned int logLevel);

// Class data
private:
	unsigned int logLevel_;
};

#ifdef _IOSTREAM_
/*****************************************************************************
// ConsoleLogHandler
//
// This object intercepts any debugging traces and puts them on the console.
// are performed through.
//
*****************************************************************************/
struct ConsoleLogHandler : public LogHandler
{
	ConsoleLogHandler() { LogHandler::LogLevel = 0xffffffff; }
	virtual void OnLog(const LogElement& le)
	{
		std::cout << le.ToString().c_str() << std::endl << std::flush;
	}
	virtual void OnAssert(const AssertElement& ae)
	{
		std::cout << ae.ToString().c_str() << std::endl << std::flush;
	}
};
#endif

/*****************************************************************************
// ODSLogHandler
//
// This object intercepts any debugging traces and puts them on the debug
// Windows console
//
*****************************************************************************/
struct ODSLogHandler : public LogHandler
{
	virtual void OnLog(const LogElement& le)
	{
		::OutputDebugStringA(le.ToString().c_str()); 
		::OutputDebugStringA("\r\n");
	}
	virtual void OnAssert(const AssertElement& ae)
	{
		::OutputDebugStringA(ae.ToString().c_str()); 
		::OutputDebugStringA("\r\n");
	}
};

/*****************************************************************************
// TraceLogType
//
// This class wraps a log event type.  it includes the trace level constant
// and the text description and prefix for the type.
//
*****************************************************************************/
class TraceLogType
{
// Class data
private:
	unsigned long traceLevel_;
	std::string textType_;
	std::string textPrefix_;

// Constructor
private:
	friend class TraceLogger_Base;
	TraceLogType() : traceLevel_(0), textType_(""), textPrefix_("") {/* */}
	TraceLogType(unsigned long nLevel, const std::string& textType, const std::string& textPrefix) :
		traceLevel_(nLevel), textType_(textType), textPrefix_(textPrefix) {/* */}
public:
	TraceLogType(const TraceLogType& rhs) : 
		traceLevel_(rhs.traceLevel_), textType_(rhs.textType_), textPrefix_(rhs.textPrefix_) {/* */}
	TraceLogType& operator=(const TraceLogType& rhs) {
		if (this != &rhs) {
			traceLevel_ = rhs.traceLevel_;
			textType_ = rhs.textType_;
			textPrefix_ = rhs.textPrefix_;
		}
		return *this;
	}

// Properties
public:
	unsigned long get_TraceLevel() const { return traceLevel_; }
	const std::string& get_Type() const { return textType_; }
	const std::string& get_Prefix() const { return textPrefix_; }
};

/*****************************************************************************
// TraceLogger_Base
//
// This base class implements a logging manager which is capable of 
// notifying multiple logging "end-points" when a log entry is generated.
//
*****************************************************************************/
class TraceLogger_Base
{
// Properties
public:
	__declspec(property(get=get_TraceLevel)) unsigned long TraceLevel;
	__declspec(property(get=get_StopOnAsserts, put=set_StopOnAsserts)) bool DebugBreak;

// Functions
public:
	// Add/Remove trace handlers
	void addTraceHandler(LogHandler* pl);
	void removeTraceHandler(LogHandler* pl);

	// Trace type information
	bool addType(unsigned long nLevel, const std::string& textType, const std::string& textPrefix = "");
	void removeType(unsigned long nLevel);
	TraceLogType get_TypeInfo(unsigned long nLevel) const;
	std::string get_Prefix(unsigned long nLevel) const;

	template <class _Ty>
	void get_Types(_Ty& c) const {
		std::transform(mapPrefix_.begin(), mapPrefix_.end(),
			std::inserter(c, c.begin()), 
			stdx::map_adapter_2<unsigned long, TraceLogType>());
	}

	// Stop tracing
	void Stop();

	// Outputs a trace element
	inline void Trace(unsigned long nLevel, const std::string& stmInfo) {
		if (nLevel == 0 || (nLevel & trcLevel_) != 0)
			InternalTrace(nLevel, stmInfo);
	}

	// Dumps the contents of a file to the trace logger.
	bool DumpFile(unsigned long nLevel, const char* fileName, bool isBinary);

	// Output a hex dump
	inline void HexDump(unsigned long nLevel, const void* pBuffer, int nSize) {
		if ((pBuffer == 0 || nSize == 0) || (nLevel > 0 && (nLevel & trcLevel_) == 0))
			return;
		InternalHexDump(nLevel,pBuffer,nSize);
	}

	// Outputs an assert failure
	void AssertFailed(const char* pszFile, int nLine, const std::string& stmInfo);

// Property access methods
public:
	inline unsigned long get_TraceLevel() const { return trcLevel_; }
	inline bool get_StopOnAsserts() const { return stopOnAssert_; }
	inline void set_StopOnAsserts(bool f) { stopOnAssert_ = f; }

// Internal functions
private:
	friend class LogHandler;
	void OnLogHandlerTraceLevelChanged();

// Constructor/Destructor
private:
	friend class Singleton<TraceLogger_Base, 9999, MultiThreadModel>;
	TraceLogger_Base();
public:
	virtual ~TraceLogger_Base();

// Internal functions
private:
	static unsigned int __stdcall LogEventHandler(void* lpParameter);
	void Runner();
	void DispatchSingle(InternalLogElement* pile);
	void BroadcastLogEvent(const LogElement* le);
	void BroadcastAssert(const AssertElement* ae);
	void InternalHexDump(unsigned long nLevel, const void* pBuffer, int nSize);
	void InternalTrace(unsigned long nLevel, const std::string& stmInfo);

// Unavailable methods
private:
	TraceLogger_Base(const TraceLogger_Base&);
	TraceLogger_Base& operator=(const TraceLogger_Base&);

// Class data
private:
	unsigned long trcLevel_;
	typedef std::vector<LogHandler*> LogHandlerList;
	typedef std::deque<InternalLogElement*> LogElementList;
	LogHandlerList listHandlers_;
	LogElementList listElements_;
	EventSynch Stopping_;
	EventSynch HasData_;
	HANDLE threadHandle_;
	CriticalSectionLock lockElements_;
	MRSWLock lockHandlers_;
	bool stopOnAssert_;
	std::map<unsigned long, TraceLogType> mapPrefix_;
};

/*****************************************************************************
// TraceLogger
//
// This wraps the above base class as a singleton.
//
*****************************************************************************/
typedef Singleton<TraceLogger_Base, 9999, MultiThreadModel> TraceLogger;

} // namespace JTI_Util   

#if defined(DEBUG) || defined(_DEBUG)

#define JTI_TRACE(x) \
	if (JTI_Util::TraceLogger::Instance().get_TraceLevel()>0) {\
		std::ostringstream ostm(std::ios::out);\
		ostm << ##x << std::ends;\
		JTI_Util::TraceLogger::Instance().Trace(0, ostm.str());\
	}

#define JTI_TRACEX(l, x) \
	if ((JTI_Util::TraceLogger::Instance().get_TraceLevel()&l)>0) {\
		std::ostringstream ostm(std::ios::out);\
		ostm << ##x << std::ends;\
		JTI_Util::TraceLogger::Instance().Trace(##l, ostm.str());\
	}

#define JTI_DUMP(p,s) \
	JTI_Util::TraceLogger::Instance().HexDump(0, p, s);
   
#define JTI_DUMPX(l, p, s) \
	JTI_Util::TraceLogger::Instance().HexDump(l, p, s);

#else

#ifdef _lint
#define RELEASE_TRACE
#endif

#ifdef RELEASE_TRACE

#define JTI_TRACE(x) \
	if (JTI_Util::TraceLogger::Instance().get_TraceLevel()>0) {\
		std::ostringstream ostm(std::ios::out);\
		ostm << ##x << std::ends;\
		JTI_Util::TraceLogger::Instance().Trace(0, ostm.str());\
	}

#define JTI_TRACEX(l, x) \
	if ((JTI_Util::TraceLogger::Instance().get_TraceLevel()&l)>0) {\
		std::ostringstream ostm(std::ios::out);\
		ostm << ##x << std::ends;\
		JTI_Util::TraceLogger::Instance().Trace(##l, ostm.str());\
	}

#define JTI_DUMP(p,s) \
	if (JTI_Util::TraceLogger::Instance().TraceLevel>0) \
		JTI_Util::TraceLogger::Instance().HexDump(0, p, s);
   
#define JTI_DUMPX(l, p, s) \
	if ((JTI_Util::TraceLogger::Instance().TraceLevel&l)>0) \
		JTI_Util::TraceLogger::Instance().HexDump(l, p, s);

#else // _TRACE or DEBUG not defined

#define JTI_TRACE(x)		(__noop)
#define JTI_TRACEX(l, x)	(__noop)
#define JTI_DUMP(p,s)		(__noop)
#define JTI_DUMPX(l,p,s)	(__noop)

#endif // TRACE
#endif // DEBUG

#if defined(DEBUG) || defined(_DEBUG)

#pragma warning (disable : 4127)
#define JTI_ASSERT(expr) \
	do { if (!(expr)) JTI_Util::TraceLogger::Instance().AssertFailed(__FILE__, __LINE__, #expr); } while (0)

#define JTI_VERIFY(expr) \
	do { if (!(expr)) JTI_Util::TraceLogger::Instance().AssertFailed(__FILE__, __LINE__, #expr); } while (0)

#else // DEBUG not defined

#define JTI_ASSERT(expr)
#define JTI_VERIFY(expr) (void)(expr)

#endif

#endif // __TRACE_DEBUG_H_INCL__
