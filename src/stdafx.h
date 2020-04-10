// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define STRICT
#define _WIN32_WINNT 0x0500		//lint !e1923
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#pragma warning(disable:4702)
#include <windows.h>
#include <atlbase.h>
#include <atlcom.h>
#include "JTIUtils.h"
#include "comutls.h"
#include "adoconn.h"
#include "Base64.h"
#include "binstream.h"
#include "CommandLineParser.h"
#include "DateTime.h"
#include "Delegates.h"
#include "DynCreate.h"
#include "EventLog.h"
#include "FileEventLogger.h"
#include "FileSystemWatcher.h"
#include "Lock.h"
#include "Longevity.h"
#include "ManagementObject.h"
#include "MemoryMappedFile.h"
#include "MemPool.h"
#include "memstream.h"
#include "MsxmlHelper.h"
#include "Observer.h"
#include "PsList.h"
#include "RefCount.h"
#include "Registry.h"
#include "RWLock.h"
//#include "SEHException.h"
#include "ServiceSupport.h"
#include "SingletonRegistry.h"
#include "sqlstream.h"
#include "StatTimer.h"
#include "stlx.h"
#include "Synchronization.h"
#include "ThreadPool.h"
#include "ThunkWriter.h"
#include "Timers.h"
#include "TraceLogger.h"
#include "tscontainer.h"
#include "WorkerThreadPool.h"
#include "XmlConfig.h"
#include "XmlParser.h"
