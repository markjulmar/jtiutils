/****************************************************************************/
//
// StatTimer.h
//
// This header describes a statistics timer class
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

#ifndef __JTI_STAT_TIMERS_H_INCLUDED_
#define __JTI_STAT_TIMERS_H_INCLUDED_

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#ifndef _WINBASE_
	#define _WIN32_WINNT 0x0500
	#include <windows.h>
#endif
#include <Lock.h>

namespace JTI_Util
{

/****************************************************************************/
// StatTimer
//
// This class can be used to time a function or set of code.
//
// Usage:
//
// void myfunc()
// {
//     StatTimer proc(true);
//     ....
//     DWORD elapsed = proc.ElapsedTime();
//     .... 
//     DWORD elapsed2 = proc.ElapsedTime(); // retrieves from last Start().
// }
//
/****************************************************************************/
class StatTimer
{
// Constructor
public:
	StatTimer(bool fStart=false) { if (fStart) Start(); }
	~StatTimer() {/* */};

// Methods
public:
	void Start() { QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&timeStart_)); }
	double ElapsedTime() const {	
		__int64 timeEnd, timeFreq;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&timeEnd));
		QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&timeFreq)); 
		return (((timeEnd - timeStart_) * 1000.0) / static_cast<double>(timeFreq));
	}

// Class data
private:
	__int64 timeStart_;
};

/****************************************************************************/
// AverageTimer
//
// This class can be used to average a series of times
//
// Usage:
//
// void myfunc()
// {
//     AverageTimer timer;
//     ....
//     timer += newCount;
//     .... 
//     timer += newCount;
//
//     timer.Minimum; timer.Maximum; timer.Average;
// }
//
/****************************************************************************/
class AverageTimer
{
// Constructor
public:
	AverageTimer() {Reset();}
	~AverageTimer() {/* */}

// Operators
public:
	void operator+=(long value) { Add(value); }

// Properties
public:
	__declspec(property(get=get_count)) long Count;
	__declspec(property(get=get_min)) long Minimum;
	__declspec(property(get=get_max)) long Maximum;
	__declspec(property(get=get_avg)) long Average;

// Methods
public:
	void Reset() { min_ = max_ = count_ = total_ = 0; }
	void Add(long value) {
#ifdef _MT
		CCSLock<CriticalSectionLockImpl> guard(&lock_);
#endif
		++count_;
		total_ += value;
		if (min_ == 0 || min_ > value)
			min_ = value;
		if (max_ < value)
			max_ = value;
	}
	long get_count() const { return count_; }
	long get_min() const { return min_; }
	long get_max() const { return max_; }
	long get_avg() const { return (count_) ? (total_/count_) : 0; }

// Class data
private:
	long min_;
	long max_;
	long count_;
	long total_;
#ifdef _MT
	CriticalSectionLockImpl lock_;
#endif
	
};

}// namespace JTI_Util

#endif // __JTI_STAT_TIMERS_H_INCLUDED_