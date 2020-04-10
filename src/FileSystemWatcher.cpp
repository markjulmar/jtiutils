/****************************************************************************/
//
// FileSystemWatcher.cpp
//
// This file implements a file system watcher class and event
// which can be used to monitor a directory for file system tree for
// change events.
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
#include <JTIUtils.h>
#include "FileSystemWatcher.h"
#include "TraceLogger.h"
#include <process.h>
#include <io.h>

using namespace JTI_Util;

/*----------------------------------------------------------------------------
	LINT OPTIONS
-----------------------------------------------------------------------------*/
//lint -esym(1551, FileSystemWatcher::~FileSystemWatcher) Cannot throw exception
//lint -esym(534, CloseHandle, WaitForSingleObject)
//lint -esym(1740, FileSystemWatcher::Path, FileSystemWatcher::Filter, FileSystemWatcher::hThread_)

/*****************************************************************************
** Procedure:  FileSystemWatcher::~FileSystemWatcher
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: Destructor for the file system watcher
**
/****************************************************************************/
FileSystemWatcher::~FileSystemWatcher() 
{ 
	fireEvents_ = false;	
	Restart();

}// FileSystemWatcher::~FileSystemWatcher

/*****************************************************************************
** Procedure:  FileSystemWatcher::OnFileWriteOccurred
** 
** Arguments:  'arrThen' - Array which captures the previous snap point
**             'arrNow' - New snap point
** 
** Returns: void
** 
** Description: This handles the case where a file is written to
**
/****************************************************************************/
void FileSystemWatcher::OnFileWriteOccurred(std::vector<FILEENTRY>& arrThen, std::vector<FILEENTRY>& arrNow)
{
	// Search for changes in file size.
	for (std::vector<FILEENTRY>::iterator itThen = arrThen.begin();
			itThen != arrThen.end(); ++itThen)
	{
		// Look for the element in the new array.  It should be there.
		std::vector<FILEENTRY>::iterator itNow = findEntry(itThen, arrNow.begin(), arrNow.end());
		if (itNow != arrNow.end())
		{
			if (itNow->fileTime_.dwLowDateTime != itThen->fileTime_.dwLowDateTime ||
				itNow->fileTime_.dwHighDateTime != itThen->fileTime_.dwHighDateTime)
				changedEvent_(FileSystemWatcherEvent(FileSystemWatcherEvent::Changed, itNow->dirName_.c_str(), itNow->fileName_.c_str()));
		}
	}

}// FileSystemWatcher::OnFileWriteOccurred

/*****************************************************************************
** Procedure:  FileSystemWatcher::OnFileChangeOccurred
** 
** Arguments:  'arrThen' - Array which captures the previous snap point
**             'arrNow' - New snap point
** 
** Returns: void
** 
** Description: This handles the case where a file is renamed, created or deleted.
**
/****************************************************************************/
void FileSystemWatcher::OnFileChangeOccurred(std::vector<FILEENTRY>& arrThen, std::vector<FILEENTRY>& arrNow)
{
	// Search for renamed/deleted elements.
	for (std::vector<FILEENTRY>::iterator itThen = arrThen.begin();
			itThen != arrThen.end(); ++itThen)
	{
		// Look for the element in the new array.  If not found, it has either been
		// deleted or renamed.
		if (findEntry(itThen, arrNow.begin(), arrNow.end()) == arrNow.end())
		{
			// If the size of the arrays are different, then this is easy -- it has been deleted.
			if (arrNow.size() != arrThen.size())
				deletedEvent_(FileSystemWatcherEvent(FileSystemWatcherEvent::Deleted, itThen->dirName_.c_str(), itThen->fileName_.c_str()));

			// Otherwise it is a bit more difficult.  Look for a new file in the new array
			else
			{
				std::vector<FILEENTRY>::iterator itNow = arrNow.begin();
				for (; itNow != arrNow.end(); ++itNow)
				{
					if (findEntry(itNow, arrThen.begin(), arrThen.end()) == arrThen.end())
						break;
				}
				renamedEvent_(FileSystemWatcherEvent(itNow->dirName_.c_str(), itNow->fileName_.c_str(), itThen->dirName_.c_str(), itThen->fileName_.c_str()));
			}
		}
	}

	// Search for created elements
	if (arrNow.size() > arrThen.size())
	{
		for (std::vector<FILEENTRY>::iterator itNow = arrNow.begin();
				itNow != arrNow.end(); ++itNow)
		{
			// Look for the element in the new array.  If not found, it has either been
			// deleted or renamed.
			if (findEntry(itNow, arrThen.begin(), arrThen.end()) == arrThen.end())
				createdEvent_(FileSystemWatcherEvent(FileSystemWatcherEvent::Created, itNow->dirName_.c_str(), itNow->fileName_.c_str()));
		}
	}

}// FileSystemWatcher::OnFileChangeOccurred

/*****************************************************************************
** Procedure:  FileSystemWatcher::OnDirChangeOccurred
** 
** Arguments:  'arrThen' - Array which captures the previous snap point
**             'arrNow' - New snap point
** 
** Returns: void
** 
** Description: This handles the case where a directory is renamed, created or deleted.
**
/****************************************************************************/
void FileSystemWatcher::OnDirChangeOccurred(std::vector<FILEENTRY>& arrThenIn, std::vector<FILEENTRY>& arrNowIn)
{
	// Get a copy of the two directories
	std::vector<FILEENTRY> arrNow, arrThen;
	std::vector<FILEENTRY>::iterator itNow, itThen;
	for (itNow = arrNowIn.begin(); itNow != arrNowIn.end(); ++itNow)
	{
		if (itNow->attribs_ & FILE_ATTRIBUTE_DIRECTORY)
			arrNow.push_back(*itNow);
	}

	for (itThen = arrThenIn.begin(); itThen != arrThenIn.end(); ++itThen)
	{
		if (itThen->attribs_ & FILE_ATTRIBUTE_DIRECTORY)
			arrThen.push_back(*itThen);
	}

	// Search for renamed/deleted elements.
	for (itThen = arrThen.begin(); itThen != arrThen.end(); ++itThen)
	{
		// Look for the element in the new array.  If not found, it has either been
		// deleted or renamed.
		if (findEntry(itThen, arrNow.begin(), arrNow.end()) == arrNow.end())
		{
			// If the size of the arrays are different, then this is easy -- it has been deleted.
			if (arrNow.size() != arrThen.size())
				deletedEvent_(FileSystemWatcherEvent(FileSystemWatcherEvent::Deleted, itThen->dirName_.c_str(), _T("")));

			// Otherwise it is a bit more difficult.  Look for a new file in the new array
			else
			{
				for (itNow = arrNow.begin(); itNow != arrNow.end(); ++itNow)
				{
					if (findEntry(itNow, arrThen.begin(), arrThen.end()) == arrThen.end())
						break;
				}
				renamedEvent_(FileSystemWatcherEvent(itNow->dirName_.c_str(), _T(""), itThen->dirName_.c_str(), _T("")));
			}
		}
	}

	// Search for created elements
	if (arrNow.size() > arrThen.size())
	{
		for (itNow = arrNow.begin(); itNow != arrNow.end(); ++itNow)
		{
			// Look for the element in the new array.  If not found, it has either been
			// deleted or renamed.
			if (findEntry(itNow, arrThen.begin(), arrThen.end()) == arrThen.end())
				createdEvent_(FileSystemWatcherEvent(FileSystemWatcherEvent::Created, itNow->dirName_.c_str(), _T("")));
		}
	}

}// FileSystemWatcher::OnDirChangeOccurred

/*****************************************************************************
** Procedure:  FileSystemWatcher::OnFileSizeChangeOccurred
** 
** Arguments:  'arrThen' - Array which captures the previous snap point
**             'arrNow' - New snap point
** 
** Returns: void
** 
** Description: This handles the case where a file size is changed
**
/****************************************************************************/
void FileSystemWatcher::OnFileSizeChangeOccurred(std::vector<FILEENTRY>& arrThen, std::vector<FILEENTRY>& arrNow)
{
	// Search for changes in file size.
	for (std::vector<FILEENTRY>::iterator itThen = arrThen.begin();
			itThen != arrThen.end(); ++itThen)
	{
		// Look for the element in the new array.  It should be there.
		std::vector<FILEENTRY>::iterator itNow = findEntry(itThen, arrNow.begin(), arrNow.end());
		if (itNow != arrNow.end())
		{
			if (itNow->fileSize_ != itThen->fileSize_)
				changedEvent_(FileSystemWatcherEvent(FileSystemWatcherEvent::Changed, itNow->dirName_.c_str(), itNow->fileName_.c_str()));
		}
	}

}// FileSystemWatcher::OnFileSizeChangeOccurred

/*****************************************************************************
** Procedure:  FileSystemWatcher::SnapshotDirectory
** 
** Arguments:  'arr' - Array to fill
**             'fIncludeSubDirs' - True to include subdirectories in snap point
**             'pszDirName' - Directory name for subdirs
** 
** Returns: void
** 
** Description: This takes a snapshot of a directory at a point in time (snap point).
**
/****************************************************************************/
void FileSystemWatcher::SnapshotDirectory(std::vector<FILEENTRY>& arr, bool fIncludeSubDirs, const TCHAR* pszDirName)
{
	tstring sDir = dirPath_;
	if (pszDirName)
	{
		sDir += _T('\\');
		sDir += pszDirName;
	}

	tstring sMask = sDir; 
	if (!sMask.empty() && *(sMask.rbegin()) != _T('\\'))
		sMask += _T('\\');
	if (!nameFilter_.empty())
		sMask += nameFilter_;
	else
		sMask += _T("*");

	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(sMask.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				ULARGE_INTEGER ul; ul.HighPart = fd.nFileSizeHigh; ul.LowPart = fd.nFileSizeLow;
				arr.push_back(FILEENTRY(sDir.c_str(), fd.cFileName, fd.ftLastWriteTime, ul.QuadPart, fd.dwFileAttributes));
			}
			else if (lstrcmpi(fd.cFileName,_T(".")) && lstrcmpi(fd.cFileName,_T("..")))
			{
				arr.push_back(FILEENTRY(sDir.c_str(), fd.cFileName, fd.ftLastWriteTime, static_cast<__int64>(0), fd.dwFileAttributes));
			}

		} while (FindNextFile(hFind, &fd));
		FindClose(hFind);	//lint !e534
	}

	// Now enumerate through sub-directories if necessary.
	if (fIncludeSubDirs)
	{
		sMask = sDir; sMask += _T("\\*");
		hFind = FindFirstFile(sMask.c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)>0 &&
					lstrcmpi(fd.cFileName,_T(".")) && lstrcmpi(fd.cFileName,_T("..")))
				{
					tstring s;
					if (pszDirName)	{
						s = pszDirName;
						s += _T("\\");
					}
					s += fd.cFileName;
					SnapshotDirectory(arr,fIncludeSubDirs,s.c_str());
				}
			} while (FindNextFile(hFind, &fd));
			FindClose(hFind);	//lint !e534
		}
	}

	// Sort it by name
	std::sort(arr.begin(), arr.end(), FILEENTRY::sortby_name);

}// FileSystemWatcher::SnapshotDirectory

/*****************************************************************************
** Procedure:  FileSystemWatcher::Restart
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: This is called to reset the file system watcher
**
/****************************************************************************/
void FileSystemWatcher::Restart()
{
	if (fireEvents_)
	{
		if (InterlockedCompareExchange(&threadRunning_, 1, 0) == 0)
		{
			JTI_ASSERT(hThread_ == NULL);

			if (dirPath_.empty())
				throw std::runtime_error("Cannot start FileSystemWatcher without the Path property being set.");

			// Start the thread to monitor the directory.
			unsigned nThreadId;
			hThread_ = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &WorkerThread, reinterpret_cast<void*>(this), 0, &nThreadId));
			if (!hThread_)
				throw std::runtime_error("Failed to create FileSystemWatcher monitor thread.");
		}
	}
	else
	{
		HANDLE hThread = hThread_;
		hThread_ = NULL;
		evtStop_.SetEvent();	//lint !e534
		if (hThread != NULL) {
            WaitForSingleObject(hThread, 3000);
			CloseHandle(hThread);
        }
	}

}// FileSystemWatcher::Restart

/*****************************************************************************
** Procedure:  FileSystemWatcher::RunWorker
** 
** Arguments:  void
** 
** Returns: void
** 
** Description: This is the main monitor thread.  It sets up the file notification
**              and then calls each delegate when an event occurs.
**
/****************************************************************************/
void FileSystemWatcher::RunWorker()
{
	std::vector<FILEENTRY> fileEntries; // File entries
	HANDLE hevtFileChange = 0, hevtDirChange = 0;
	HANDLE arrHandles[3] = {0};
	DWORD cntHandles = 0;

	while (fireEvents_)
	{
		// Reset the event
		evtStop_.ResetEvent(); //lint !e534

		// Close all the file change handles
		if (hevtFileChange)
			FindCloseChangeNotification(hevtFileChange); //lint !e534
		if (hevtDirChange)
			FindCloseChangeNotification(hevtDirChange); //lint !e534
		hevtFileChange = hevtDirChange = 0;
		
		// Now setup the handles
		arrHandles[0] = evtStop_.get(); cntHandles++;
		if (notifyFilter_ & ~DirectoryName)
		{
			hevtFileChange = FindFirstChangeNotification(dirPath_.c_str(), watchSubDirs_, (notifyFilter_&~DirectoryName));
			if (hevtFileChange != INVALID_HANDLE_VALUE)
				arrHandles[cntHandles++] = hevtFileChange;
			else hevtFileChange = NULL;
		}
				
		if (notifyFilter_ & DirectoryName)
		{
			hevtDirChange = FindFirstChangeNotification(dirPath_.c_str(), watchSubDirs_, FILE_NOTIFY_CHANGE_DIR_NAME);
			if (hevtDirChange != INVALID_HANDLE_VALUE)
				arrHandles[cntHandles++] = hevtDirChange;
			else hevtDirChange = NULL;
		}

		// If we have no directory to monitor, then exit immediately.
		if (cntHandles == 1)
		{
			fireEvents_ = false;
			break;
		}
			
		// Take a snapshot of the directory as it is now.
		SnapshotDirectory(fileEntries, watchSubDirs_);

		// Now wait for one of our handles to signal
		for (;;)
		{
			// Wait on the event
			DWORD rc = WaitForMultipleObjects(cntHandles, arrHandles, FALSE, INFINITE);
			if (rc == WAIT_OBJECT_0 || rc == WAIT_FAILED)
				break;

			// Take a new snapshot
			std::vector<FILEENTRY> arrNew;
			SnapshotDirectory(arrNew, watchSubDirs_);

			// Some file event occurred; determine which handle caused the trigger.
			HANDLE hEvent = arrHandles[rc-WAIT_OBJECT_0];
			if (hEvent == hevtFileChange)
			{
				OnFileChangeOccurred(fileEntries, arrNew);
				OnFileWriteOccurred(fileEntries, arrNew);
				OnFileSizeChangeOccurred(fileEntries, arrNew);
			}
			else if (hEvent == hevtDirChange)
				OnDirChangeOccurred(fileEntries, arrNew);

			// Copy over the new fileentry data
			fileEntries = arrNew;

			// Reset the fcn and loop around.
			FindNextChangeNotification(hEvent); //lint !e534
		}
	}
	InterlockedCompareExchange(&threadRunning_, 0, 1); //lint !e534

}// FileSystemWatcher::RunWorker

/*****************************************************************************
** Procedure:  FileSystemWatcher::get_IsValid
** 
** Arguments:  void
** 
** Returns: true/false
** 
** Description: This returns whether the given path exists.
**
/****************************************************************************/
bool FileSystemWatcher::get_IsValid() const
{
	return (!dirPath_.empty() && _taccess(dirPath_.c_str(), 0) != -1);

}// FileSystemWatcher::get_IsValid
