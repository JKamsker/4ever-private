// TTime.h: CTime and CTimeSpan without ATL.
//
// The same classes as ATL's atltime.h: a CTime is a 64-bit time_t read in local time, a
// CTimeSpan a number of seconds; Format takes strftime's formats (and %D %H %M %S for spans).
//////////////////////////////////////////////////////////////////////

#if !defined __TTIME_H
#define __TTIME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <time.h>


class CTimeSpan
{
public:
	CTimeSpan() { m_timeSpan = 0; }
	CTimeSpan( __time64_t time) { m_timeSpan = time; }
	CTimeSpan( LONG lDays, int nHours, int nMins, int nSecs) { SetDateTimeSpan( lDays, nHours, nMins, nSecs); }

	void SetDateTimeSpan( LONG lDays, int nHours, int nMins, int nSecs)
	{
		m_timeSpan = nSecs + 60 * (nMins + 60 * (nHours + __time64_t(24) * lDays));
	}

	LONGLONG GetDays() const { return m_timeSpan / (24 * 3600); }
	LONGLONG GetTotalHours() const { return m_timeSpan / 3600; }
	LONG GetHours() const { return LONG(GetTotalHours() - GetDays() * 24); }
	LONGLONG GetTotalMinutes() const { return m_timeSpan / 60; }
	LONG GetMinutes() const { return LONG(GetTotalMinutes() - GetTotalHours() * 60); }
	LONGLONG GetTotalSeconds() const { return m_timeSpan; }
	LONG GetSeconds() const { return LONG(GetTotalSeconds() - GetTotalMinutes() * 60); }
	__time64_t GetTimeSpan() const { return m_timeSpan; }

	// %D days, %H %M %S the rest, %% a percent sign.
	CString Format( LPCTSTR pszFormat) const
	{
		CString strResult;

		while( pszFormat && *pszFormat )
		{
			TCHAR ch = *pszFormat++;

			if( ch != _T('%') || !*pszFormat )
			{
				strResult.AppendChar(ch);
				continue;
			}

			switch( ch = *pszFormat++ )
			{
			case _T('%')	: strResult.AppendChar(_T('%')); break;
			case _T('D')	: strResult.AppendFormat( _T("%I64d"), GetDays()); break;
			case _T('H')	: strResult.AppendFormat( _T("%02ld"), GetHours()); break;
			case _T('M')	: strResult.AppendFormat( _T("%02ld"), GetMinutes()); break;
			case _T('S')	: strResult.AppendFormat( _T("%02ld"), GetSeconds()); break;
			default			: strResult.AppendChar(_T('%')); strResult.AppendChar(ch); break;
			}
		}

		return strResult;
	}

	CTimeSpan operator + ( CTimeSpan span) const { return CTimeSpan( m_timeSpan + span.m_timeSpan); }
	CTimeSpan operator - ( CTimeSpan span) const { return CTimeSpan( m_timeSpan - span.m_timeSpan); }
	CTimeSpan& operator += ( CTimeSpan span) { m_timeSpan += span.m_timeSpan; return *this; }
	CTimeSpan& operator -= ( CTimeSpan span) { m_timeSpan -= span.m_timeSpan; return *this; }
	bool operator == ( CTimeSpan span) const { return m_timeSpan == span.m_timeSpan; }
	bool operator != ( CTimeSpan span) const { return m_timeSpan != span.m_timeSpan; }
	bool operator < ( CTimeSpan span) const { return m_timeSpan < span.m_timeSpan; }
	bool operator > ( CTimeSpan span) const { return m_timeSpan > span.m_timeSpan; }
	bool operator <= ( CTimeSpan span) const { return m_timeSpan <= span.m_timeSpan; }
	bool operator >= ( CTimeSpan span) const { return m_timeSpan >= span.m_timeSpan; }

private:
	__time64_t m_timeSpan;
};

class CTime
{
public:
	static CTime GetCurrentTime() { return CTime(_time64(NULL)); }

	CTime() { m_time = 0; }
	CTime( __time64_t time) { m_time = time; }

	CTime( int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nDST = -1)
	{
		struct tm vTM;

		memset( &vTM, 0, sizeof(vTM));
		vTM.tm_sec = nSec;
		vTM.tm_min = nMin;
		vTM.tm_hour = nHour;
		vTM.tm_mday = nDay;
		vTM.tm_mon = nMonth - 1;
		vTM.tm_year = nYear - 1900;
		vTM.tm_isdst = nDST;

		m_time = _mktime64(&vTM);
	}

	CTime( const SYSTEMTIME& st, int nDST = -1)
	{
		*this = CTime( st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, nDST);
	}

	__time64_t GetTime() const { return m_time; }

	struct tm *GetLocalTm( struct tm *pTM) const
	{
		if( !pTM || _localtime64_s( pTM, &m_time) )
			return NULL;

		return pTM;
	}

	struct tm *GetGmtTm( struct tm *pTM) const
	{
		if( !pTM || _gmtime64_s( pTM, &m_time) )
			return NULL;

		return pTM;
	}

	int GetYear() const { struct tm vTM; return GetLocalTm(&vTM) ? vTM.tm_year + 1900 : 0; }
	int GetMonth() const { struct tm vTM; return GetLocalTm(&vTM) ? vTM.tm_mon + 1 : 0; }
	int GetDay() const { struct tm vTM; return GetLocalTm(&vTM) ? vTM.tm_mday : 0; }
	int GetHour() const { struct tm vTM; return GetLocalTm(&vTM) ? vTM.tm_hour : -1; }
	int GetMinute() const { struct tm vTM; return GetLocalTm(&vTM) ? vTM.tm_min : -1; }
	int GetSecond() const { struct tm vTM; return GetLocalTm(&vTM) ? vTM.tm_sec : -1; }
	int GetDayOfWeek() const { struct tm vTM; return GetLocalTm(&vTM) ? vTM.tm_wday + 1 : 0; }

	bool GetAsSystemTime( SYSTEMTIME& st) const
	{
		struct tm vTM;

		if(!GetLocalTm(&vTM))
			return false;

		st.wYear = WORD(vTM.tm_year + 1900);
		st.wMonth = WORD(vTM.tm_mon + 1);
		st.wDay = WORD(vTM.tm_mday);
		st.wHour = WORD(vTM.tm_hour);
		st.wMinute = WORD(vTM.tm_min);
		st.wSecond = WORD(vTM.tm_sec);
		st.wDayOfWeek = WORD(vTM.tm_wday);
		st.wMilliseconds = 0;

		return true;
	}

	CString Format( LPCTSTR pszFormat) const
	{
		struct tm vTM;
		TCHAR szBuffer[128];

		if( !GetLocalTm(&vTM) || !_tcsftime( szBuffer, 128, pszFormat, &vTM) )
			szBuffer[0] = 0;

		return CString(szBuffer);
	}

	CString FormatGmt( LPCTSTR pszFormat) const
	{
		struct tm vTM;
		TCHAR szBuffer[128];

		if( !GetGmtTm(&vTM) || !_tcsftime( szBuffer, 128, pszFormat, &vTM) )
			szBuffer[0] = 0;

		return CString(szBuffer);
	}

	CTime& operator += ( CTimeSpan span) { m_time += span.GetTimeSpan(); return *this; }
	CTime& operator -= ( CTimeSpan span) { m_time -= span.GetTimeSpan(); return *this; }
	CTimeSpan operator - ( CTime time) const { return CTimeSpan( m_time - time.m_time); }
	CTime operator - ( CTimeSpan span) const { return CTime( m_time - span.GetTimeSpan()); }
	CTime operator + ( CTimeSpan span) const { return CTime( m_time + span.GetTimeSpan()); }
	bool operator == ( CTime time) const { return m_time == time.m_time; }
	bool operator != ( CTime time) const { return m_time != time.m_time; }
	bool operator < ( CTime time) const { return m_time < time.m_time; }
	bool operator > ( CTime time) const { return m_time > time.m_time; }
	bool operator <= ( CTime time) const { return m_time <= time.m_time; }
	bool operator >= ( CTime time) const { return m_time >= time.m_time; }

private:
	__time64_t m_time;
};

#endif // !defined __TTIME_H
