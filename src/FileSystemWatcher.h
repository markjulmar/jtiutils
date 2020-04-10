/******************************************************************************/
//                                                                        
// FileSystemWatcher.h
//                                             
// This header implements a file system watcher class and event
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
/******************************************************************************/

#ifndef __FILESYSWATCH_H_INCL__
#define __FILESYSWATCH_H_INCL__

// Requires NT4 or better
#if (_WIN32_WINNT < 0x0400)
#error "You must compile for at least Windows NT 4 or better for the FileSystemWatcher classes
#endif

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <vector>
#include <string>
#include <delegates.h>
#include <synchronization.h>

/*****************************************************************************/
// PC-Lint options
//
//lint -save
//
// Ignore public data members
//lint -esym(1925, FileSystemWatcherEvent::ChangeType, FileSystemWatcherEvent::FullPath, FileSystemWatcherEvent::Name)
//lint -esym(1925, FileSystemWatcherEvent::OldFullPath, FileSystemWatcherEvent::OldName)
//lint -esym(1925, FileSystemWatcher::Path, FileSystemWatcher::Filter, FileSystemWatcher::NotifyFilter)
//lint -esym(1925, FileSystemWatcher::EnableRaisingEvents, FileSystemWatcher::IncludeSubdirectories)
//
// Data members are not initialized in constructor list
//lint -esym(1927, FileSystemWatcherEvent::ChangeType, FileSystemWatcherEvent::FullPath, FileSystemWatcherEvent::Name)
//lint -esym(1927, FileSystemWatcherEvent::OldFullPath, FileSystemWatcherEvent::OldName)
//lint -esym(1927, FileSystemWatcher::Path, FileSystemWatcher::Filter, FileSystemWatcher::NotifyFilter)
//lint -esym(1927, FileSystemWatcher::EnableRaisingEvents, FileSystemWatcher::IncludeSubdirectories)
//
// Data members are possibly not initialized by constructor
//lint -esym(1744, FileSystemWatcherEvent::ChangeType, FileSystemWatcherEvent::FullPath, FileSystemWatcherEvent::Name)
//lint -esym(1744, FileSystemWatcherEvent::OldFullPath, FileSystemWatcherEvent::OldName)
//lint -esym(1744, FileSystemWatcher::Path, FileSystemWatcher::Filter, FileSystemWatcher::NotifyFilter)
//lint -esym(1744, FileSystemWatcher::EnableRaisingEvents, FileSystemWatcher::IncludeSubdirectories)
//
//lint -esym(1401, FileSystemWatcherEvent::ChangeType, FileSystemWatcherEvent::FullPath, FileSystemWatcherEvent::Name)
//lint -esym(1401, FileSystemWatcherEvent::OldFullPath, FileSystemWatcherEvent::OldName)
//lint -esym(1401, FileSystemWatcher::Path, FileSystemWatcher::Filter, FileSystemWatcher::NotifyFilter)
//lint -esym(1401, FileSystemWatcher::EnableRaisingEvents, FileSystemWatcher::IncludeSubdirectories)
//
// Data members are not assigned by assignment operator
//lint -esym(1539, FileSystemWatcherEvent::ChangeType, FileSystemWatcherEvent::FullPath, FileSystemWatcherEvent::Name)
//lint -esym(1539, FileSystemWatcherEvent::OldFullPath, FileSystemWatcherEvent::OldName)
//lint -esym(1539, FileSystemWatcher::Path, FileSystemWatcher::Filter, FileSystemWatcher::NotifyFilter)
//lint -esym(1539, FileSystemWatcher::EnableRaisingEvents, FileSystemWatcher::IncludeSubdirectories)
//
// Data members are not free'd by destructor
//lint -esym(1540, FileSystemWatcherEvent::FullPath, FileSystemWatcherEvent::Name)
//lint -esym(1540, FileSystemWatcherEvent::OldFullPath, FileSystemWatcherEvent::OldName)
//
/*****************************************************************************/

namespace JTI_Util
{
/******************************************************************************/
// FileSystemWatcherEvent
//
// This class defines the event which is passed to the listening delegates.
// It contains the data which defines the change which has occurred.
//
/******************************************************************************/
//lint --e{1704} Private constructor ok
class FileSystemWatcherEvent
{
// Public data
public:
	enum ChangeTypes {
		Unknown	= 0,
		Changed = 1,
		Created = 2,
		Deleted = 3,
		Renamed = 4
	};

// Properties
public:
	__declspec(property(get=get_ChangeType)) ChangeTypes ChangeType;
	__declspec(property(get=get_FullPath)) const TCHAR* FullPath;
	__declspec(property(get=get_Name)) const TCHAR* Name;
	__declspec(property(get=get_OldFullPath)) const TCHAR* OldFullPath;
	__declspec(property(get=get_OldName)) const TCHAR* OldName;

// Acessors
public:
	ChangeTypes get_ChangeType() const { return cType_; }
	const TCHAR* get_FullPath() const { return pathName_.c_str(); }
	const TCHAR* get_Name() const { return fileName_.c_str(); }
	const TCHAR* get_OldFullPath() const { return oldpathName_.c_str(); }
	const TCHAR* get_OldName() const { return oldfileName_.c_str(); }

// Constructor
private:
	friend class FileSystemWatcher;
	FileSystemWatcherEvent(ChangeTypes cType, const TCHAR* pszPath, const TCHAR* pszFile) : 
		cType_(cType), pathName_(pszPath), fileName_(pszFile), oldpathName_(_T("")), oldfileName_(_T("")) {/* */}
	FileSystemWatcherEvent(const TCHAR* pszPath, const TCHAR* pszFile, const TCHAR* pszOldPath, const TCHAR* pszOldFile) : 
		cType_(Renamed), pathName_(pszPath), fileName_(pszFile), oldpathName_(pszOldPath), oldfileName_(pszOldFile) {/* */}
public:
	FileSystemWatcherEvent(const FileSystemWatcherEvent& rhs) : 
		cType_(rhs.cType_), pathName_(rhs.pathName_), fileName_(rhs.fileName_),
		oldpathName_(rhs.oldpathName_), oldfileName_(rhs.oldfileName_) {/* */}
	FileSystemWatcherEvent& operator=(const FileSystemWatcherEvent& rhs) {
		if (this != &rhs) {
			cType_ = rhs.cType_;
			pathName_ = rhs.pathName_;
			fileName_ = rhs.fileName_;
			oldpathName_ = rhs.oldpathName_;
			oldfileName_ = rhs.oldfileName_;
		}
		return *this;
	}
	~FileSystemWatcherEvent() {/* */}

// Internal data
private:
	ChangeTypes cType_;
	tstring pathName_;
	tstring fileName_;
	tstring oldpathName_;
	tstring oldfileName_;
};

/******************************************************************************/
// FileSystemWatcher
//
// This class defines the file system watcher class which can be used to 
// monitor a single directory tree.
//
/******************************************************************************/
class FileSystemWatcher
{
// Public class data
public:
	enum {
		LastWrite		= FILE_NOTIFY_CHANGE_LAST_WRITE,
		FileName		= FILE_NOTIFY_CHANGE_FILE_NAME,
		DirectoryName	= FILE_NOTIFY_CHANGE_DIR_NAME,
		FileSize		= FILE_NOTIFY_CHANGE_SIZE
	};

// Constructor
public:
	FileSystemWatcher() : 
		dirPath_(_T("")), fireEvents_(false), watchSubDirs_(false), threadRunning_(0), nameFilter_(_T("")),
		notifyFilter_(LastWrite|FileName|DirectoryName), evtStop_(false,true), hThread_(NULL),
		createdEvent_(), changedEvent_(), deletedEvent_(), renamedEvent_()	{/* */}

	~FileSystemWatcher();

// Properties
public:
	__declspec(property(get=get_Path,put=set_Path)) const TCHAR* Path;
	__declspec(property(get=get_IsValid)) bool IsValid;
	__declspec(property(get=get_Filter,put=set_Filter)) const TCHAR* Filter;
	__declspec(property(get=get_NotifyFilter,put=set_NotifyFilter)) unsigned int NotifyFilter;
	__declspec(property(get=get_EnableRaisingEvents,put=set_EnableRaisingEvents)) bool EnableRaisingEvents;
	__declspec(property(get=get_IncludeSubdirectories,put=set_IncludeSubdirectories)) bool IncludeSubdirectories;

// Accessors
public:
	const TCHAR* get_Path() const { return dirPath_.c_str(); }
	void set_Path(const TCHAR* pszPath) { dirPath_ = pszPath; Restart(); }
	unsigned int get_NotifyFilter() const { return notifyFilter_; }
	void set_NotifyFilter(unsigned int filter) { notifyFilter_ = filter; Restart(); }
	const TCHAR* get_Filter() const { return nameFilter_.c_str(); }
	void set_Filter(const TCHAR* pszFilter) { nameFilter_ = pszFilter; Restart(); }
	bool get_EnableRaisingEvents() const { return fireEvents_; }
	void set_EnableRaisingEvents(bool f) { fireEvents_ = f; Restart(); }
	bool get_IncludeSubdirectories() const { return watchSubDirs_; }
	void set_IncludeSubdirectories(bool f) { watchSubDirs_ = f; Restart(); }
	bool get_IsValid() const;

// Delegates
public:
	template<class _Object, class _Class>
	void add_OnChanged(const _Object& object, void (_Class::* fnc)(FileSystemWatcherEvent))
	{
		changedEvent_.add(object,fnc);
	}
	void add_OnChanged(void (*fnc)(FileSystemWatcherEvent))
	{
		changedEvent_.add(fnc);
	}

	template<class _Object, class _Class>
	void add_OnCreated(const _Object& object, void (_Class::* fnc)(FileSystemWatcherEvent))
	{
		createdEvent_.add(object,fnc);
	}
	void add_OnCreated(void (*fnc)(FileSystemWatcherEvent))
	{
		createdEvent_.add(fnc);
	}

	template<class _Object, class _Class>
	void add_OnDeleted(const _Object& object, void (_Class::* fnc)(FileSystemWatcherEvent))
	{
		deletedEvent_.add(object,fnc);
	}
	void add_OnDeleted(void (*fnc)(FileSystemWatcherEvent))
	{
		deletedEvent_.add(fnc);
	}

	template<class _Object, class _Class>
	void add_OnRenamed(const _Object& object, void (_Class::* fnc)(FileSystemWatcherEvent))
	{
		renamedEvent_.add(object,fnc);
	}
	void add_OnRenamed(void (*fnc)(FileSystemWatcherEvent))
	{
		renamedEvent_.add(fnc);
	}

// Internal structures
private:
	struct FILEENTRY
	{
		FILETIME fileTime_;
		tstring dirName_;
		tstring fileName_;
		__int64 fileSize_;
		DWORD attribs_;
		FILEENTRY(const TCHAR* dirName, const TCHAR* filename, const FILETIME& ftm, __int64 size, DWORD attribs) : 
			fileTime_(ftm), dirName_(dirName), fileName_(filename), fileSize_(size), attribs_(attribs)
			{
				if (!dirName_.empty() && *(dirName_.rbegin()) != _T('\\'))
					dirName_ += _T('\\');
				dirName_ += fileName_;
			}

		bool operator==(const FILEENTRY& rhs) const
		{
			return (dirName_ == rhs.dirName_ &&
					 ((attribs_ & FILE_ATTRIBUTE_DIRECTORY) == (rhs.attribs_ & FILE_ATTRIBUTE_DIRECTORY)));
		}

		bool operator!=(const FILEENTRY& rhs) const
		{
			return !operator==(rhs);
		}

		static bool sortby_name(const FILEENTRY& fe1, const FILEENTRY& fe2)
		{
			//tstring s1 = fe1.dirName_; s1 += _T("\\"); s1 += fe1.fileName_;
			//tstring s2 = fe2.dirName_; s2 += _T("\\"); s2 += fe2.fileName_;
			return fe1.dirName_ < fe2.dirName_;
		}
	};

// Internal functions
private:
	void OnFileWriteOccurred(std::vector<FILEENTRY>& arrThen, std::vector<FILEENTRY>& arrNow);
	void OnFileChangeOccurred(std::vector<FILEENTRY>& arrThen, std::vector<FILEENTRY>& arrNow);
	void OnDirChangeOccurred(std::vector<FILEENTRY>& arrThenIn, std::vector<FILEENTRY>& arrNowIn);
	void OnFileSizeChangeOccurred(std::vector<FILEENTRY>& arrThen, std::vector<FILEENTRY>& arrNow);
	void SnapshotDirectory(std::vector<FILEENTRY>& arr, bool fIncludeSubDirs, const TCHAR* pszDirName=NULL);
	void Restart();
	void RunWorker();

	template<class _It>
	_It findEntry(_It it, _It startIt, _It endIt)
	{
		for (_It itHere = startIt; itHere != endIt; ++itHere) {
			if (*it == *itHere)
				return itHere;
		}
		return endIt;
	}
	static unsigned __stdcall WorkerThread(void* p)
	{
		reinterpret_cast<FileSystemWatcher*>(p)->RunWorker();
		return 0;
	}

// Internal class data
private:
	tstring dirPath_;				// Directory we are watching
	bool fireEvents_;				// True to fire events
	bool watchSubDirs_;				// True to watch sub-directories
	long threadRunning_;			// Thread running
	tstring nameFilter_;			// Filter for filenames
	unsigned int notifyFilter_;		// Notification filter
	EventSynch evtStop_;			// Stop event
	HANDLE hThread_;				// Thread handle
	Delegate_1<MultiThreadModel, FileSystemWatcherEvent> createdEvent_;
	Delegate_1<MultiThreadModel, FileSystemWatcherEvent> changedEvent_;
	Delegate_1<MultiThreadModel, FileSystemWatcherEvent> deletedEvent_;
	Delegate_1<MultiThreadModel, FileSystemWatcherEvent> renamedEvent_;
};

} // namespace JTI_Util

#endif // __FILESYSWATCH_H_INCL__
