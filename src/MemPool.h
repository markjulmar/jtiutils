/****************************************************************************/
//
// MemPool.h
//
// This class defines a small object memory allocator which provides
// better performance than the general purpose allocator from MS.
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

#ifndef __MEMPOOL_H_INCLUDED__
#define __MEMPOOL_H_INCLUDED__

/*----------------------------------------------------------------------------
    INCLUDE FILE
-----------------------------------------------------------------------------*/
#include <stdexcept>
#include <Lock.h>

namespace JTI_Util
{
// Define the default lock type based on what's being compiled.
#ifdef _MT
	#define JTI_MEMPOOL_LOCK_TYPE SingleThreadModel
#else
	#define JTI_MEMPOOL_LOCK_TYPE SimpleMultiThreadModel
#endif

/****************************************************************************/
// MemPool
//
// Class which encapsulates a small-alloc memory pool
//
/****************************************************************************/
template <class T, int EXPANSION_SIZE = 4096, class LockType = JTI_MEMPOOL_LOCK_TYPE>
class MemPool : public LockableObject<LockType>
{
// Class data
private:
	typedef MemPool<T, EXPANSION_SIZE, LockType> _MemPtr;
	_MemPtr* pNext_;

// Constructor
public:
	MemPool(size_t size = EXPANSION_SIZE) { ExpandFreeList(size); }
	~MemPool() {
		_MemPtr* pNext = pNext_;
		for (; pNext_ != NULL; pNext_ = pNext) {
			pNext = pNext->pNext_; free(pNext_);
		}
	}

	void* Alloc(size_t size) {
		if (size != sizeof(T)) return ::JTI_NEW char[size];
		CCSLock<MemPool> lock(this);
		if (!pNext_) {
			if (!ExpandFreeList())
				return NULL;
		}
		_MemPtr* pHead = pNext_;
		pNext_ = pHead->pNext_;
		return pHead;
	}

	void Free(void* pElem, size_t size) {
		if (size != sizeof(T)) {
			::delete [] ((char*) pElem);
			return;
		}
		CCSLock<MemPool> lock(this);
		_MemPtr* pHead = static_cast<_MemPtr*>(pElem);
		pHead->pNext_ = pNext_;
		pNext_ = pHead;
	}

// Internal methods
private:
	bool ExpandFreeList(size_t count = EXPANSION_SIZE) {
        size_t size = std::max(sizeof(T),sizeof(_MemPtr*));
		_MemPtr* pList = reinterpret_cast<_MemPtr*>(malloc(size));
		if (!pList) throw std::bad_alloc();
		pNext_ = pList;
		for (size_t i = 0; i < count; ++i) {
			pList->pNext_ = reinterpret_cast<_MemPtr*>(malloc(size));
			pList = pList->pNext_;
		}
		pList->pNext_ = 0;
		return true;
	}
};

} // namespace JTI_Util

#endif // __MEMPOOL_H_INCLUDED__
