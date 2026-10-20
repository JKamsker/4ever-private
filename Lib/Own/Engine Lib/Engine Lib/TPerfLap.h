#pragma once

// CPU time of the parts of a frame, for the bgfx debug overlay. TPERF_LAP("name") gives
// the time since the previous lap to "name", so the laps of a frame add up to all of it.
// The render layer starts the first lap after each Present. Main thread only.

#define TPERF_MAX_LAPS		48

#define TPERF_MAX_COUNTS	8

struct TPERFLAPS
{
	const char *m_vNAME[TPERF_MAX_LAPS];
	LONGLONG m_vTICKS[TPERF_MAX_LAPS];
	LONGLONG m_nLast;
	int m_nCount;

	// TPERF_COUNT: numbers summed per frame.
	const char *m_vCOUNTNAME[TPERF_MAX_COUNTS];
	LONGLONG m_vCOUNT[TPERF_MAX_COUNTS];
	int m_nCounts;
};

inline int TPerfCountID( const char *pName);

inline TPERFLAPS& TPerfLaps()
{
	static TPERFLAPS s_vLAPS = {};
	return s_vLAPS;
}

inline LONGLONG TPerfNow()
{
	LARGE_INTEGER vNow;

	QueryPerformanceCounter(&vNow);
	return vNow.QuadPart;
}

inline int TPerfLapID( const char *pName)
{
	TPERFLAPS& v = TPerfLaps();

	if( v.m_nCount >= TPERF_MAX_LAPS )
		return -1;
	v.m_vNAME[v.m_nCount] = pName;

	return v.m_nCount++;
}

inline void TPerfLap( int nLap)
{
	TPERFLAPS& v = TPerfLaps();
	LONGLONG nNow = TPerfNow();

	if( nLap >= 0 && v.m_nLast )
		v.m_vTICKS[nLap] += nNow - v.m_nLast;
	v.m_nLast = nNow;
}

#define TPERF_LAP(name)		do { static const int s_nPerfLap = TPerfLapID(name); TPerfLap(s_nPerfLap); } while(0)

// A zone is the time spent in a scope, inside the laps: name it "*..." so the overlay
// tells them apart.
class CTPerfZone
{
public:
	CTPerfZone( int nZone) : m_nZone(nZone), m_nStart(TPerfNow()) {}

	~CTPerfZone()
	{
		if( m_nZone >= 0 )
			TPerfLaps().m_vTICKS[m_nZone] += TPerfNow() - m_nStart;
	}

private:
	int m_nZone;
	LONGLONG m_nStart;
};

inline int TPerfCountID( const char *pName)
{
	TPERFLAPS& v = TPerfLaps();

	if( v.m_nCounts >= TPERF_MAX_COUNTS )
		return -1;
	v.m_vCOUNTNAME[v.m_nCounts] = pName;

	return v.m_nCounts++;
}

#define TPERF_COUNT( name, n)	do { static const int s_nPerfCount = TPerfCountID(name); if( s_nPerfCount >= 0 ) TPerfLaps().m_vCOUNT[s_nPerfCount] += (n); } while(0)

#define TPERF_CAT2( a, b)	a##b
#define TPERF_CAT( a, b)		TPERF_CAT2( a, b)
#define TPERF_ZONE(name)		static const int TPERF_CAT( s_nPerfZone, __LINE__) = TPerfLapID(name); CTPerfZone TPERF_CAT( vPerfZone, __LINE__)(TPERF_CAT( s_nPerfZone, __LINE__))
