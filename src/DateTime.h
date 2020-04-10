/****************************************************************************/
//
// DateTime.h
//
// This header describes a generic Date/Time class which does not depend
// on ATL or MFC.
//
// Copyright (C) 1998-2003 JulMar Technology, Inc.   All rights reserved
// This is private property of JulMar Technology, Inc.  It may not be
// distributed or released without express written permission of
// JulMar Technology, Inc.
//
// You must not remove this notice, or any other, from this software.
// 
/****************************************************************************/

#ifndef __JTI_DATETIME_INCLUDED_
#define __JTI_DATETIME_INCLUDED_

/*****************************************************************************/
// PC-Lint options
//
//lint -save
//
// Supress duplicate header include files
//lint -e537
//
// Allow implicit conversions to constructors
//lint -esym(1931, DateTime*)
//
// Allow operator conversions
//lint -esym(1930, DateTime*)
//
// Some of the operator= do not have a RHS assignment
//lint -esym(1529, DateTime::operator=)
//
// Ignore public properties
//lint -esym(1925, DateTime::Year, DateTime::Month, DateTime::Day)
//lint -esym(1925, DateTime::Hour, DateTime::Minute, DateTime::Second)
//lint -esym(1925, DateTime::DayOfWeek, DateTime::DayOfYear)
//lint -esym(1925, DateTimeSpan::TotalDays, DateTimeSpan::TotalHours, DateTimeSpan::TotalMinutes, DateTimeSpan::TotalSeconds)
//lint -esym(1925, DateTimeSpan::Days, DateTimeSpan::Hours, DateTimeSpan::Minutes, DateTimeSpan::Seconds)
//
// Public properties not in constructor initializer list
//lint -esym(1927, DateTime::Year, DateTime::Month, DateTime::Day)
//lint -esym(1927, DateTime::Hour, DateTime::Minute, DateTime::Second)
//lint -esym(1927, DateTime::DayOfWeek, DateTime::DayOfYear)
//lint -esym(1927, DateTimeSpan::TotalDays, DateTimeSpan::TotalHours, DateTimeSpan::TotalMinutes, DateTimeSpan::TotalSeconds)
//lint -esym(1927, DateTimeSpan::Days, DateTimeSpan::Hours, DateTimeSpan::Minutes, DateTimeSpan::Seconds)
//
// Public properties not initialized by constructor
//lint -esym(1401, DateTime::Year, DateTime::Month, DateTime::Day)
//lint -esym(1401, DateTime::Hour, DateTime::Minute, DateTime::Second)
//lint -esym(1401, DateTime::DayOfWeek, DateTime::DayOfYear)
//lint -esym(1401, DateTimeSpan::TotalDays, DateTimeSpan::TotalHours, DateTimeSpan::TotalMinutes, DateTimeSpan::TotalSeconds)
//lint -esym(1401, DateTimeSpan::Days, DateTimeSpan::Hours, DateTimeSpan::Minutes, DateTimeSpan::Seconds)
//
// Public properties not assigned in operators
//lint -esym(1539, DateTime::Year, DateTime::Month, DateTime::Day)
//lint -esym(1539, DateTime::Hour, DateTime::Minute, DateTime::Second)
//lint -esym(1539, DateTime::DayOfWeek, DateTime::DayOfYear)
//lint -esym(1539, DateTimeSpan::TotalDays, DateTimeSpan::TotalHours, DateTimeSpan::TotalMinutes, DateTimeSpan::TotalSeconds)
//lint -esym(1539, DateTimeSpan::Days, DateTimeSpan::Hours, DateTimeSpan::Minutes, DateTimeSpan::Seconds)
//
// Exposing low-level access
//lint -esym(1536, DateTime::dateValue_)
//
/*****************************************************************************/

/*----------------------------------------------------------------------------
    INCLUDE FILES
-----------------------------------------------------------------------------*/
#include <time.h>
#include <string>
#include <limits.h>
#include <math.h>
#include <oleauto.h>
#include <oaidl.h>
#include <winnls.h>
#include <malloc.h>
#include <jtiUtils.h>
#include <TraceLogger.h>

#pragma warning(disable : 4996)

namespace JTI_Util
{
/*----------------------------------------------------------------------------
    FORWARD DECLARATIONS
-----------------------------------------------------------------------------*/
class DateTimeSpan;

/******************************************************************************/
// DateTime
//
// This describes the Date/Time holding class
//
/******************************************************************************/
class DateTime
{
// Constructors
public:
	DateTime() : dateValue_(0), Status_(DTS_VALID) {/* */}
	DateTime(const DateTime& rhs) : dateValue_(rhs.dateValue_), Status_(rhs.Status_) {/* */}
	DateTime(const VARIANT& varSrc) : dateValue_(0), Status_(DTS_VALID) { *this = varSrc; }
	DateTime(DATE dtSrc) : dateValue_(dtSrc), Status_(DTS_INVALID)  {/*lint -e534*/  Validate(); }
	DateTime(const char* pszSrc) : dateValue_(0), Status_(DTS_VALID) { /*lint -e534*/ ParseDateTime(pszSrc); }
    DateTime(const wchar_t* pszSrc) : dateValue_(0), Status_(DTS_VALID) {/*lint -e534*/ ParseDateTime(pszSrc); }
#if !defined(_WIN64) && _MSC_VER < 1400
    DateTime(const time_t& timeSrc) : dateValue_(0), Status_(DTS_VALID) { *this = timeSrc; }
#endif
    DateTime(const __time64_t& timeSrc) : dateValue_(0), Status_(DTS_VALID) { *this = timeSrc; }
    DateTime(const SYSTEMTIME& systimeSrc) : dateValue_(0), Status_(DTS_VALID) { *this = systimeSrc; }
    DateTime(const FILETIME& filetimeSrc) : dateValue_(0), Status_(DTS_VALID) { *this = filetimeSrc; }
    DateTime(long nYear, long nMonth, long nDay, long nHour = 0, long nMin = 0, long nSec = 0) : dateValue_(0), Status_(DTS_VALID) { SetDateTime(nYear, nMonth, nDay, nHour, nMin, nSec);  /*lint !e534*/ }
#ifdef _OLEAUTO_H_
	DateTime(WORD wDosDate, WORD wDosTime) : dateValue_(0), Status_(DTS_VALID) { if (!::DosDateTimeToVariantTime(wDosDate, wDosTime, &dateValue_)) Status_ = DTS_INVALID; }
#endif
#ifdef __oledb_h__
    DateTime(const DBTIMESTAMP& dbts) { SetDateTime(dbts.year, dbts.month, dbts.day, dbts.hour, dbts.minute, dbts.second); }
#endif

// Operators
public:
	DateTime& operator=(const DateTime& rhs)
	{ 
		if (this != &rhs) { 
			dateValue_ = rhs.dateValue_; 
			Status_ = rhs.Status_; 
		} 
		return *this; 
	}

	DateTime& operator=(const VARIANT& varSrc)
	{
		//lint --e{641}
		if (varSrc.vt != VT_DATE) {
			VARIANT varDest;
			varDest.vt = VT_EMPTY;
			if(SUCCEEDED(::VariantChangeType(&varDest, const_cast<VARIANT *>(&varSrc), 0, VT_DATE))) {
				dateValue_ = varDest.date;
				Status_ = DTS_VALID;
				::VariantClear(&varDest);
			}
			else
				Status_ = DTS_INVALID;
		}
		else {
			dateValue_ = varSrc.date;
			Status_ = DTS_VALID;
		}
		return *this;
	}

	DateTime& operator=(DATE dt) 
	{ 
		dateValue_ = dt; 
		Status_ = DTS_VALID; 
		return *this;
	}
    DateTime& operator=(const char* pszSrc) 
	{ 
		ParseDateTime(pszSrc);  /*lint !e534*/ 
		return *this; 
	}
    DateTime& operator=(const wchar_t* pszSrc) 
	{ 
		ParseDateTime(pszSrc);  /*lint !e534*/ 
		return *this; 
	}
#if defined(_WIN64) || _MSC_VER < 1400
    DateTime& operator=(const time_t& timeSrc)
	{
		return operator=(static_cast<__time64_t>(timeSrc));
	}
#endif
    DateTime& operator=(const __time64_t& timeSrc)
	{
		struct tm* ptm = _gmtime64(&timeSrc);
		if (ptm) {
			SYSTEMTIME st;
			st.wYear = static_cast<WORD>(ptm->tm_year + 1900);
			st.wMonth = static_cast<WORD>(ptm->tm_mon + 1);
			st.wDayOfWeek = static_cast<WORD>(ptm->tm_wday);
			st.wDay = static_cast<WORD>(ptm->tm_mday);
			st.wHour = static_cast<WORD>(ptm->tm_hour);
			st.wMinute = static_cast<WORD>(ptm->tm_min);
			st.wSecond = static_cast<WORD>(ptm->tm_sec);
			if (::SystemTimeToVariantTime(&st, &dateValue_)) {
				Status_ = DTS_VALID;
				return *this;
			}
		}
		Status_ = DTS_INVALID;
		return *this;
	}
    
	DateTime& operator=(const SYSTEMTIME& st)
	{
		if (::SystemTimeToVariantTime(const_cast<SYSTEMTIME*>(&st), &dateValue_)) {
			Status_ = DTS_VALID;
			return *this;
		}
		Status_ = DTS_INVALID;
		return *this;
	}

    DateTime& operator=(const FILETIME& filetimeSrc)
	{
		SYSTEMTIME st;
		if (::FileTimeToSystemTime(&filetimeSrc, &st)) {
			if (::SystemTimeToVariantTime(&st, &dateValue_)) {
				Status_ = DTS_VALID;
				return *this;
			}
		}
		Status_ = DTS_INVALID;
		return *this;
	}

#ifdef __oledb_h__
    DateTime& DateTime::operator=(const DBTIMESTAMP& dbts) 
	{ 
		SetDateTime(dbts.year, dbts.month, dbts.day, dbts.hour, dbts.minute, dbts.second); 
		return *this; 
	}
#endif

	// Comparison operators
	friend inline bool operator==(const DateTime& lhs, const DateTime& rhs) {/*lint -e{777}*/ return (DateTime::DoubleFromDate(static_cast<DATE>(lhs)) == DateTime::DoubleFromDate(static_cast<DATE>(rhs))); }
	friend inline bool operator!=(const DateTime& lhs, const DateTime& rhs) {/*lint -e{777}*/ return (DateTime::DoubleFromDate(static_cast<DATE>(lhs)) != DateTime::DoubleFromDate(static_cast<DATE>(rhs))); }
	friend inline bool operator<(const DateTime& lhs, const DateTime& rhs) {/*lint -e{777}*/ return (DateTime::DoubleFromDate(static_cast<DATE>(lhs)) < DateTime::DoubleFromDate(static_cast<DATE>(rhs))); }
    friend inline bool operator>(const DateTime& lhs, const DateTime& rhs) {/*lint -e{777}*/ return (DateTime::DoubleFromDate(static_cast<DATE>(lhs)) > DateTime::DoubleFromDate(static_cast<DATE>(rhs))); }
    friend inline bool operator<=(const DateTime& lhs, const DateTime& rhs) {/*lint -e{777}*/ return (DateTime::DoubleFromDate(static_cast<DATE>(lhs)) <= DateTime::DoubleFromDate(static_cast<DATE>(rhs))); }
    friend inline bool operator>=(const DateTime& lhs, const DateTime& rhs) {/*lint -e{777}*/ return (DateTime::DoubleFromDate(static_cast<DATE>(lhs)) >= DateTime::DoubleFromDate(static_cast<DATE>(rhs))); }
    
    // Increment/Decrement and math operators
	friend inline DateTime operator+(const DateTime& date, const DateTimeSpan& span);
	friend inline DateTime operator+(const DateTimeSpan& span, const DateTime& date);
	friend inline DateTime operator-(const DateTime& date, const DateTimeSpan& span);
	friend inline DateTimeSpan operator-(const DateTime& date1, const DateTime& date2);

	const DateTime& operator+=(const DateTimeSpan& span) { *this = *this + span; return *this; }
    const DateTime& operator-=(const DateTimeSpan& span) { *this = *this - span; return *this; }
    
	// Implicit operators
	operator DATE() const { return dateValue_; }

	// Low-level access (pointer)
	DATE* get() { return &dateValue_; }

// Static functions
public:
    static DateTime Now() {	SYSTEMTIME sysTime; ::GetSystemTime(&sysTime); return DateTime(sysTime);	}
	static DateTime FromTickCount(DWORD dwTickCount);

// Properties
public:
	__declspec(property(get=get_Year)) long Year;
	__declspec(property(get=get_Month)) long Month;
	__declspec(property(get=get_Day)) long Day;
	__declspec(property(get=get_Hour)) long Hour;
	__declspec(property(get=get_Minute)) long Minute;
	__declspec(property(get=get_Second)) long Second;
	__declspec(property(get=get_DayOfWeek)) long DayOfWeek;
	__declspec(property(get=get_DayOfYear)) long DayOfYear;

// Property accessors
public:
	long get_Year() const { SYSTEMTIME st; return (GetAsSystemTime(st)) ? st.wYear : -1; }
	long get_Month() const { SYSTEMTIME st; return (GetAsSystemTime(st)) ? st.wMonth : -1; }
	long get_Day() const { SYSTEMTIME st; return (GetAsSystemTime(st)) ? st.wDay : -1; }
	long get_Hour() const { SYSTEMTIME st; return (GetAsSystemTime(st)) ? st.wHour : -1; }
    long get_Minute() const { SYSTEMTIME st; return (GetAsSystemTime(st)) ? st.wMinute : -1; }
    long get_Second() const { SYSTEMTIME st; return (GetAsSystemTime(st)) ? st.wSecond : -1; }
    long get_DayOfWeek() const { SYSTEMTIME st; return (GetAsSystemTime(st)) ? st.wDayOfWeek+1 : -1; }
	long get_DayOfYear() const 
	{ 
		SYSTEMTIME st; 
		if (GetAsSystemTime(st)) {
			struct tm tmDest;
			tmDest.tm_sec = st.wSecond;
			tmDest.tm_min = st.wMinute;
			tmDest.tm_hour = st.wHour;
			tmDest.tm_mday = st.wDay;
			tmDest.tm_mon = st.wMonth - 1;
			tmDest.tm_year = st.wYear - 1900;
			tmDest.tm_wday = st.wDayOfWeek;
			tmDest.tm_isdst = -1;
			if (mktime(&tmDest) != static_cast<time_t>(-1))
				return tmDest.tm_yday;
		}
		return -1;
	}
	time_t get_time_t() const 
	{
		struct tm tmNow;
		ZeroMemory(&tmNow,sizeof(tmNow));
		tmNow.tm_mon = Month-1;
		tmNow.tm_mday = Day;
		tmNow.tm_year = Year - 1900;
		tmNow.tm_hour = Hour;
		tmNow.tm_min = Minute;
		tmNow.tm_sec = Second;
		return mktime(&tmNow);
	}
    
// Methods
public:
	bool IsLeapYear() const { long year = get_Year(); return ((year != -1) && ((year & 3) == 0) && ((year % 100) != 0 || (year % 400) == 0)); }
	bool IsNoon() const { SYSTEMTIME st; return (GetAsSystemTime(st) && st.wHour == 12 && st.wMinute == 0 && st.wSecond == 0); }
    bool IsMidnight() const { SYSTEMTIME st; return (GetAsSystemTime(st) && st.wHour == 0 && st.wMinute == 0 && st.wSecond == 0); }
    bool IsValid() const { return (Status_ == DTS_VALID && IsValidDate(dateValue_)); }
	bool GetAsSystemTime(SYSTEMTIME& st) const { return (::VariantTimeToSystemTime(dateValue_, &st) > 0); }
    bool SetDate(long nYear, long nMonth, long nDay) { return SetDateTime(nYear, nMonth, nDay, 0, 0, 0); }
    bool SetTime(long nHour, long nMin, long nSec) { return SetDateTime(1899, 12, 30, nHour, nMin, nSec); }
	void MarkInvalid() { Status_ = DTS_INVALID; }

	bool SetDateTime(long nYear, long nMonth, long nDay, long nHour, long nMin, long nSec)
	{
		SYSTEMTIME st = {0};
		st.wYear = static_cast<WORD>(nYear);
		st.wMonth = static_cast<WORD>(nMonth);
		st.wDay = static_cast<WORD>(nDay);
		st.wHour = static_cast<WORD>(nHour);
		st.wMinute = static_cast<WORD>(nMin);
		st.wSecond = static_cast<WORD>(nSec);
		if (::SystemTimeToVariantTime(&st, &dateValue_))
		{
			if (Status_ == DTS_INVALID)
				Status_ = DTS_VALID;
			return true;
		}
		return false;
	}

    inline bool ParseDateTime(const char* lpszDate, DWORD dwFlags = 0, LCID lcid = LANG_USER_DEFAULT);
    inline bool ParseDateTime(const wchar_t* lpszDate, DWORD dwFlags = 0, LCID lcid = LANG_USER_DEFAULT);

	DateTime ToLocalTime() const
	{
		SYSTEMTIME st;
		if (::VariantTimeToSystemTime(dateValue_, &st))
		{
			FILETIME ftLocal, ftUTC;
			if (::SystemTimeToFileTime(&st, &ftUTC) && ::FileTimeToLocalFileTime(&ftUTC, &ftLocal))
				return DateTime(ftLocal);
		}		
		DateTime dt; dt.Status_ = DTS_INVALID; return dt;
	}

	bool Validate() { Status_ = (IsValidDate(dateValue_)) ? DTS_VALID : DTS_INVALID; return IsValid(); }

    // Formatting; both Unicode and ANSI forms.
	std::string Format(LPCSTR lpszFormat) const;
	std::wstring Format(LPCWSTR lpszFormat) const;
#if defined(UNICODE) || defined(_UNICODE)
	std::string FormatA(DWORD dwFlags = 0, LCID lcid = LANG_USER_DEFAULT) const;
	std::wstring Format(DWORD dwFlags = 0, LCID lcid = LANG_USER_DEFAULT) const;
	std::string FormatA(UINT nFormatID, HINSTANCE hInstance = GetModuleHandle(0)) const;
	std::wstring Format(UINT nFormatID, HINSTANCE hInstance = GetModuleHandle(0)) const;
#else
	std::string Format(DWORD dwFlags = 0, LCID lcid = LANG_USER_DEFAULT) const;
	std::wstring FormatW(DWORD dwFlags = 0, LCID lcid = LANG_USER_DEFAULT) const;
	std::string Format(UINT nFormatID, HINSTANCE hInstance = GetModuleHandle(0)) const;
	std::wstring FormatW(UINT nFormatID, HINSTANCE hInstance = GetModuleHandle(0)) const;
#endif

// Internal functions
private:
	static double DoubleFromDate(DATE d)
	{
		if (d >= 0) 
			return d;
		double temp = ceil(d);
		return temp - (d - temp);
	}

	static DATE DateFromDouble(double d)
	{
		if (d >= 0) 
			return d;
		double temp = floor(d);
		return temp + (temp - d);
	}

    static bool IsValidDate(DATE dt)
	{ 
        const double MIN_DATE = -657434L;
		const double MAX_DATE = 2958465L;
		return (dt >= MIN_DATE && dt <= MAX_DATE); 
	}

// Class data
private:
	enum DateTimeStatus	{
		DTS_VALID, DTS_INVALID
	};
    DATE dateValue_;
	DateTimeStatus Status_;

};

/******************************************************************************/
// DateTimeSpan
//
// This describes the Date/Time difference (span) holding class
//
/******************************************************************************/
class DateTimeSpan
{
// Constructors
public:
	DateTimeSpan() : timeSpan_(0), Status_(DTS_VALID) {/* */}
	DateTimeSpan(double d) : timeSpan_(d), Status_(DTS_VALID) {/*lint -e{534}*/ Validate();}
    DateTimeSpan(long nDays, long nHours = 0, long nMins = 0, long nSecs = 0) : timeSpan_(0), Status_(DTS_VALID) { SetSpan(nDays, nHours, nMins, nSecs); }
	DateTimeSpan(const DateTimeSpan& rhs) : timeSpan_(rhs.timeSpan_), Status_(rhs.Status_) {/* */}

// Operators
public:
	const DateTimeSpan& operator=(const DateTimeSpan& rhs) 
	{ 
		if (&rhs != this) {
			timeSpan_ = rhs.timeSpan_; 
			Status_ = rhs.Status_;
		}
		return *this; 
	}

	const DateTimeSpan& operator=(double d) 
	{ 
		timeSpan_ = d;
		/*lint -e{534}*/ Validate();
		return *this; 
	}

	// Equality operators
	friend bool operator==(const DateTimeSpan& lhs, const DateTimeSpan& rhs) { /*lint -e{777}*/ return lhs.timeSpan_ == rhs.timeSpan_; }
	friend bool operator!=(const DateTimeSpan& lhs, const DateTimeSpan& rhs) { /*lint -e{777}*/ return lhs.timeSpan_ != rhs.timeSpan_; }
	friend bool operator<(const DateTimeSpan& lhs, const DateTimeSpan& rhs) { /*lint -e{777}*/ return lhs.timeSpan_ < rhs.timeSpan_; }
	friend bool operator>(const DateTimeSpan& lhs, const DateTimeSpan& rhs) { /*lint -e{777}*/ return lhs.timeSpan_ > rhs.timeSpan_; }
	friend bool operator<=(const DateTimeSpan& lhs, const DateTimeSpan& rhs) { /*lint -e{777}*/ return lhs.timeSpan_ <= rhs.timeSpan_; }
	friend bool operator>=(const DateTimeSpan& lhs, const DateTimeSpan& rhs) { /*lint -e{777}*/ return lhs.timeSpan_ >= rhs.timeSpan_; }
    
	// Increment/Decrement and math operators
    const DateTimeSpan& operator+=(const DateTimeSpan& rhs) { return (*this = *this + rhs); }
    const DateTimeSpan& operator-=(const DateTimeSpan& rhs) { return (*this = *this - rhs); }
    DateTimeSpan operator-() const { return DateTimeSpan(-timeSpan_); }
    operator double() const { return timeSpan_; }
    
    friend DateTimeSpan operator+(const DateTimeSpan& span1, const DateTimeSpan& span2)
	{
		DateTimeSpan span3;
		if (span1.IsValid() && span2.IsValid()) 
			span3 = span1.timeSpan_ + span2.timeSpan_;
		else
			span3.Status_ = DateTimeSpan::DTS_INVALID;
		return span3;
	}

    friend DateTimeSpan operator-(const DateTimeSpan& span1, const DateTimeSpan& span2)
	{
	    return span1 + (-span2);
	}

// Methods
public:
    void SetSpan(long nDays, long nHours, long nMins, long nSecs) 
	{ 
		timeSpan_ = nDays + ((double)nHours)/24 + ((double)nMins)/(24*60) +	((double)nSecs)/(24*60*60);	
		/*lint -e{534}*/ Validate();
	}

	bool IsValid() const { return Status_ == DTS_VALID; }
	bool Validate() 
	{ 
		const double MAX_SPAN = 3615897L;
		const double MIN_SPAN = -MAX_SPAN;
		Status_ = (timeSpan_ >= MIN_SPAN && timeSpan_ <= MAX_SPAN) ? DTS_VALID : DTS_INVALID;
		return IsValid();
	}

// Properties
public:
	__declspec(property(get=get_TotalDays)) double TotalDays;
	__declspec(property(get=get_TotalHours)) double TotalHours;
	__declspec(property(get=get_TotalMinutes)) double TotalMinutes;
	__declspec(property(get=get_TotalSeconds)) double TotalSeconds;
	__declspec(property(get=get_Days)) long Days;
	__declspec(property(get=get_Hours)) long Hours;
	__declspec(property(get=get_Minutes)) long Minutes;
	__declspec(property(get=get_Seconds)) long Seconds;

// Property helpers
public:
    double get_TotalDays() const { JTI_ASSERT(IsValid()); return timeSpan_; }
    double get_TotalHours() const { JTI_ASSERT(IsValid()); return timeSpan_ * 24; }
    double get_TotalMinutes() const { JTI_ASSERT(IsValid()); return timeSpan_ * 24 * 60; }
    double get_TotalSeconds() const { JTI_ASSERT(IsValid()); return timeSpan_ * 24 * 60 * 60; }
    long get_Days() const { JTI_ASSERT(IsValid()); return (long)timeSpan_; }

	long get_Hours() const
	{
		JTI_ASSERT(IsValid());
		double d,y;
		d = modf(timeSpan_, &y);
		return (long)(d*24);
	}

    long get_Minutes() const
	{
		JTI_ASSERT(IsValid());
		double d,y;
		d = modf(timeSpan_ * 24, &y);
		return (long)(d*60);
	}

    long get_Seconds() const
	{
		JTI_ASSERT(IsValid());
		double d,y;
		d = modf(timeSpan_ * 24 * 60, &y);
		return (long)(d*60);
	}

// Class data
private:
	enum DateTimeStatus	{
		DTS_VALID, DTS_INVALID
	};
    double  timeSpan_;
	DateTimeStatus Status_;
};

/*****************************************************************************
** Procedure:  DateTime::FromTickCount
** 
** Arguments:  dwTickCount - Tickcount in question
** 
** Returns: New DateTime object
** 
** Description: This function converts a tickcount into a DateTime class
**
/****************************************************************************/
inline DateTime DateTime::FromTickCount(DWORD dwTickCount) 
{ 
	DWORD dwDelta = ELAPSED_TIME(dwTickCount); 
	return (dwDelta > 0) ? (DateTime::Now() - DateTimeSpan(0,0,0,dwDelta/1000)) : DateTime::Now(); 

}// DateTime::FromTickCount

/*****************************************************************************
** Procedure:  operator+
** 
** Arguments:  'date' - Date
**             'span' - DateTimespan
** 
** Returns: New DateTime object
** 
** Description: This function adds a span to a date
**
/****************************************************************************/
inline DateTime operator+(const DateTime& date, const DateTimeSpan& span)
{
	JTI_ASSERT(date.IsValid());
	JTI_ASSERT(span.IsValid());
	return DateTime::DateFromDouble(DateTime::DoubleFromDate(static_cast<DATE>(date)) + static_cast<double>(span));

}// operator+

/*****************************************************************************
** Procedure:  operator+
** 
** Arguments:  'date' - Date
**             'span' - DateTimespan
** 
** Returns: New DateTime object
** 
** Description: This function adds a span to a date
**
/****************************************************************************/
inline DateTime operator+(const DateTimeSpan& span, const DateTime& date)
{
	JTI_ASSERT(date.IsValid());
	JTI_ASSERT(span.IsValid());
	return DateTime::DateFromDouble(DateTime::DoubleFromDate(static_cast<DATE>(date)) + static_cast<double>(span));

}// operator+ 

/*****************************************************************************
** Procedure:  operator-
** 
** Arguments:  'date1' - Date
**             'date2' - Date
** 
** Returns: New DateTimeSpan object
** 
** Description: This function subtracts a date from another date
**
/****************************************************************************/
inline DateTimeSpan operator-(const DateTime& date1, const DateTime& date2)
{
	JTI_ASSERT(date1.IsValid());
	JTI_ASSERT(date2.IsValid());
	return DateTimeSpan(DateTime::DoubleFromDate(static_cast<DATE>(date1)) - DateTime::DoubleFromDate(static_cast<DATE>(date2)));

}// operator-

/*****************************************************************************
** Procedure:  operator-
** 
** Arguments:  'date' - Date
**             'span' - DateTimespan
** 
** Returns: New DateTime object
** 
** Description: This function subtracts a span from a date
**
/****************************************************************************/
inline DateTime operator-(const DateTime& date, const DateTimeSpan& span)
{
	JTI_ASSERT(date.IsValid());
	JTI_ASSERT(span.IsValid());
	return DateTime::DateFromDouble(DateTime::DoubleFromDate(static_cast<DATE>(date)) - static_cast<double>(span));

}// operator-

/*****************************************************************************
** Procedure:  DateTime::ParseDateTime
** 
** Arguments:  lpszDate - Date to parse
**             dwFlags - processing flags
**             lcid - Locality to use
** 
** Returns: true/false
** 
** Description: This parses a string to get the date/time from it.
**
/****************************************************************************/
inline bool DateTime::ParseDateTime(const char* lpszDate, DWORD dwFlags, LCID lcid)
{
	int nSize = MultiByteToWideChar(CP_ACP, 0, lpszDate, -1, NULL, 0);
	if (nSize > 0)
	{
		wchar_t* pszwBuff = (wchar_t*) _alloca(nSize * sizeof(wchar_t));
		MultiByteToWideChar(CP_ACP, 0, lpszDate, -1, pszwBuff, nSize);
		return ParseDateTime(pszwBuff, dwFlags, lcid);
	}
    return false;

}// DateTime::ParseDateTime

/*****************************************************************************
** Procedure:  DateTime::ParseDateTime
** 
** Arguments:  lpszDate - Date to parse
**             dwFlags - processing flags
**             lcid - Locality to use
** 
** Returns: true/false
** 
** Description: This parses a string to get the date/time from it.
**
/****************************************************************************/
inline bool DateTime::ParseDateTime(const wchar_t* lpszDate, DWORD dwFlags, LCID lcid)
{
	if (FAILED(::VarDateFromStr(const_cast<wchar_t*>(lpszDate), lcid, dwFlags, &dateValue_)))
        Status_ = DTS_INVALID;
    return IsValid();

}// DateTime::ParseDateTime

/*****************************************************************************
** Procedure:  DateTime::Format
** 
** Arguments:  dwFlags - Formatting flags
**             lcid - Locality to use
** 
** Returns: string
** 
** Description: This converts the current date/time to the given format
**
/****************************************************************************/
#if defined(UNICODE) || defined(_UNICODE)
inline std::string DateTime::FormatA(DWORD dwFlags, LCID lcid) const
#else
inline std::string DateTime::Format(DWORD dwFlags, LCID lcid) const
#endif
{
	// If invalid, return a null string
	BSTR bstrVal;
	if (Status_ == DTS_INVALID ||
		(FAILED(::VarBstrFromDate(dateValue_, lcid, dwFlags, &bstrVal))))
		return "";

	int nSize = WideCharToMultiByte(CP_ACP, 0, bstrVal, -1, NULL, 0, NULL, NULL);
	if (nSize > 0)
	{
		char* pszBuff = (char*) _alloca(static_cast<unsigned int>(nSize));
		WideCharToMultiByte(CP_ACP, 0, bstrVal, -1, pszBuff, nSize, NULL, NULL);
		::SysFreeString(bstrVal);
		return std::string(pszBuff);
	}
	return "";

}//DateTime::FormatA

/*****************************************************************************
** Procedure:  DateTime::Format
** 
** Arguments:  dwFlags - Formatting flags
**             lcid - Locality to use
** 
** Returns: string
** 
** Description: This converts the current date/time to the given format
**
/****************************************************************************/
#if defined(UNICODE) || defined(_UNICODE)
inline std::wstring DateTime::Format(DWORD dwFlags, LCID lcid) const
#else
inline std::wstring DateTime::FormatW(DWORD dwFlags, LCID lcid) const
#endif
{
	// If invalid, return a null string
	BSTR bstrVal;
	if (Status_ == DTS_INVALID ||
		(FAILED(::VarBstrFromDate(dateValue_, lcid, dwFlags, &bstrVal))))
		return L"";

	std::wstring str = bstrVal;
	::SysFreeString(bstrVal);
	return str;

}//DateTime::FormatW

/*****************************************************************************
** Procedure:  DateTime::FormatA
** 
** Arguments:  pszFormat - Format string (ala strtime)
** 
** Returns: string
** 
** Description: This formats the date/time object to a string
**
/****************************************************************************/
inline std::string DateTime::Format(LPCSTR pFormat) const
{
	// If invalid, return a null string
	UDATE ud;
	if (Status_ == DTS_INVALID ||
		(::VarUdateFromDate(dateValue_, 0, &ud) != S_OK))
		return "";

	struct tm tmTemp;
	tmTemp.tm_sec = ud.st.wSecond;
	tmTemp.tm_min = ud.st.wMinute;
	tmTemp.tm_hour = ud.st.wHour;
	tmTemp.tm_mday = ud.st.wDay;
	tmTemp.tm_mon = ud.st.wMonth - 1;
	tmTemp.tm_year = ud.st.wYear - 1900;
	tmTemp.tm_wday = ud.st.wDayOfWeek;
	tmTemp.tm_yday = ud.wDayOfYear - 1;
	tmTemp.tm_isdst	= 0;

	char chBuffer[256];
	strftime(chBuffer, sizeofarray(chBuffer), pFormat, &tmTemp);
	return std::string(chBuffer);
}// DateTime::FormatA

/*****************************************************************************
** Procedure:  DateTime::FormatW
** 
** Arguments:  pszFormat - Format string (ala strtime)
** 
** Returns: string
** 
** Description: This formats the date/time object to a string
**
/****************************************************************************/
inline std::wstring DateTime::Format(LPCWSTR pFormat) const
{
	// If invalid, return a null string
	UDATE ud;
	if (Status_ == DTS_INVALID ||
		(::VarUdateFromDate(dateValue_, 0, &ud) != S_OK))
		return L"";

	struct tm tmTemp;
	tmTemp.tm_sec = ud.st.wSecond;
	tmTemp.tm_min = ud.st.wMinute;
	tmTemp.tm_hour = ud.st.wHour;
	tmTemp.tm_mday = ud.st.wDay;
	tmTemp.tm_mon = ud.st.wMonth - 1;
	tmTemp.tm_year = ud.st.wYear - 1900;
	tmTemp.tm_wday = ud.st.wDayOfWeek;
	tmTemp.tm_yday = ud.wDayOfYear - 1;
	tmTemp.tm_isdst	= 0;

	wchar_t chBuffer[256];
	wcsftime(chBuffer, sizeofarray(chBuffer), pFormat, &tmTemp);
	return std::wstring(chBuffer);
}// DateTime::FormatW

/*****************************************************************************
** Procedure:  DateTime::FormatA
** 
** Arguments:  nFormatID - String id
**             hInstance - Module to retrieve string from
** 
** Returns: string
** 
** Description: This formats the date/time from a resource string
**
/****************************************************************************/
#if defined(UNICODE) || defined(_UNICODE)
inline std::string DateTime::FormatA(UINT nFormatID, HINSTANCE hInstance) const
#else
inline std::string DateTime::Format(UINT nFormatID, HINSTANCE hInstance) const
#endif
{
	char chBuffer[256];
	if (LoadStringA(hInstance, nFormatID, chBuffer, sizeofarray(chBuffer)))
		return Format(chBuffer);
	return "";

}// DateTime::FormatA

/*****************************************************************************
** Procedure:  DateTime::FormatW
** 
** Arguments:  nFormatID - String id
**             hInstance - Module to retrieve string from
** 
** Returns: string
** 
** Description: This formats the date/time from a resource string
**
/****************************************************************************/
#if defined(UNICODE) || defined(_UNICODE)
inline std::wstring DateTime::Format(UINT nFormatID, HINSTANCE hInstance) const
#else
inline std::wstring DateTime::FormatW(UINT nFormatID, HINSTANCE hInstance) const
#endif
{
	wchar_t chBuffer[256];
	if (LoadStringW(hInstance, nFormatID, chBuffer, sizeofarray(chBuffer)))
		return Format(chBuffer);
	return L"";

}// DateTime::FormatW

}// namespace JTI_Util

#endif  // __COMDATE_H__
