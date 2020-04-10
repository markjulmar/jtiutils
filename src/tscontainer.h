/****************************************************************************/
//
// TSContainer.h
//
// This file describes a thread-safe container which provides a protected
// container.
//
// Original Copyright (C) 1997-2004 JulMar Technology, Inc.   All rights reserved
// You must not remove this notice, or any other, from this software.
//
/****************************************************************************/

#ifndef __TSCONTAINER_H_INCL__
#define __TSCONTAINER_H_INCL__

/*----------------------------------------------------------------------------
//    INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <vector>
#include "Lock.h"

/*****************************************************************************/
// PC-Lint options
//
//lint -save
//
/*****************************************************************************/

// Define the default lock type based on what's being compiled.
#ifdef _MT
	#define JTI_TSCONTAINER_LOCK_TYPE SingleThreadModel
#else
	#define JTI_TSCONTAINER_LOCK_TYPE MultiThreadModel
#endif

namespace JTI_Util
{
// Partial specialization for std::vector which has no push_front method.
namespace JTI_Internal 
{
	template <class _Container, class _Ty>
	inline void internal_push_front(_Container& container, const _Ty& data) { container.push_front(data); }

	template <class _Ty>
	inline void internal_push_front(std::vector<_Ty>& container, const _Ty& data) { 
		container.push_back(data);
		std::rotate(container.begin(), container.end()-1, container.end());
	}

}// JTI_Internal

/**************************************************************************
// ThreadSafeContainer
//
// This container holds the some value type objects; this class has 
// built-in thread safety support but when the iterators are used directly, 
// the user must perform the lock/unlock.
//
**************************************************************************/
template <class _Ty, template<class,class> class _Container = std::vector, 
		  class _Alloc = std::allocator<_Ty> >
class ThreadSafeContainer : public LockableObject<JTI_TSCONTAINER_LOCK_TYPE>
{
// Public types
public:
	typedef ThreadSafeContainer<_Ty, _Container, _Alloc> ThisType;
	typedef _Container<_Ty, _Alloc> TContainer;
	typedef typename TContainer::value_type value_type;
	typedef typename TContainer::const_iterator const_iterator;
	typedef typename TContainer::iterator iterator;
	typedef typename TContainer::size_type size_type;

// Constructor
public:
	ThreadSafeContainer() : container_() {/* */}
	explicit ThreadSafeContainer(const ThisType& rhs) : container_(rhs.container_) {/* */}
	explicit ThreadSafeContainer(const TContainer& rhs) : container_(rhs) {/* */}

// Accessors
public:
	size_type size() const { CCSLock<ThisType> lockGuard(this); return container_.size(); }
	bool empty() const { CCSLock<ThisType> lockGuard(this); return container_.empty(); }
	const_iterator begin() const { return container_.begin(); }
	const_iterator end() const { return container_.end(); }
	iterator begin() { return container_.begin(); }
	iterator end() { return container_.end(); }
	void clear() { CCSLock<ThisType> lockGuard(this); container_.clear(); }

// Methods
public:
	template <class _OutIt, class _Pred>
	_OutIt locked_copy_if(_OutIt it, _Pred pred) {
		CCSLock<ThisType> lockGuard(this);
		// No copy_if algorithm in STL; so use Not(remove_copy_if)
		return std::remove_copy_if(begin(), end(), it, std::not1(pred));
	}
	template <class _OutIt>
	_OutIt locked_copy(_OutIt it) {
		CCSLock<ThisType> lockGuard(this);
		return std::copy(begin(), end(), it);
	}

	template <class _Pred>
	const_iterator find_if(_Pred predicate) const {
		CCSLock<ThisType> lockGuard(this);
		return std::find_if(container_.begin(), container_.end(), predicate);
	}
	template <class _Pred>
	iterator find_if(_Pred predicate) {
		CCSLock<ThisType> lockGuard(this);
		return std::find_if(container_.begin(), container_.end(), predicate);
	}

	bool exists(const _Ty& data) const {
		CCSLock<ThisType> lockGuard(this); 
		return (std::find(container_.begin(), container_.end(), data) != container_.end());
	}

	void push_back(const _Ty& data) { 
		CCSLock<ThisType> lockGuard(this); 
		container_.push_back(data); 
		lockGuard.Unlock();
	}

	void push_front(const _Ty& data) {
		CCSLock<ThisType> lockGuard(this); 
		JTI_Internal::internal_push_front(container_, data); 
		lockGuard.Unlock();
	}

	_Ty pop()
	{
		CCSLock<ThisType> lockGuard(this); 
		iterator it = container_.begin();
		_Ty t = (*it);
		container_.erase(it);
		return t;
	}

	void remove(const _Ty& data) { 
		CCSLock<ThisType> lockGuard(this); 
		iterator it = std::find(container_.begin(), container_.end(), data);
		if (it != container_.end())
			container_.erase(it);
	}

// Class data
private:
	TContainer container_;
};

} // namespace JTI_Util

#endif // __TSCONTAINER_H_INCL__