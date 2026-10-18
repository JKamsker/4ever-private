// TString.h: the engine's CString, CStringA and CStringW, without ATL.
//
// The same API and behavior as ATL's CStringT in an MBCS build: narrow strings are searched,
// compared, trimmed and case-mapped with the CRT's multibyte functions (_mbschr, _mbsstr,
// _mbsicmp, _mbsupr, _mbsinc...), so a DBCS trail byte never matches, and the conversions
// between narrow and wide strings use the thread's ANSI code page.
//
// Like ATL's, the object is a single pointer to the characters, the length and capacity are
// stored just before them, so a CString passed to printf-like functions still reads as a char*.
//////////////////////////////////////////////////////////////////////

#if !defined __TSTRING_H
#define __TSTRING_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <mbstring.h>
#include <new>

#ifdef _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#define TSTRING_EXPLICIT			explicit
#else
#define TSTRING_EXPLICIT
#endif


//////////////////////////////////////////////////////////////////////
// Character traits: what ATL's ChTraitsCRT does for each character type.

template< typename T > struct TStringTraits;

template<> struct TStringTraits<char>
{
	typedef wchar_t OTHER;

	static int Length( const char *psz) { return psz ? int(strlen(psz)) : 0; }
	static const char *CharNext( const char *p) { return (const char *) _mbsinc((const unsigned char *) p); }
	static int IsSpace( char ch) { return _ismbcspace(BYTE(ch)); }
	static int Compare( const char *a, const char *b) { return _mbscmp( (const unsigned char *) a, (const unsigned char *) b); }
	static int CompareNoCase( const char *a, const char *b) { return _mbsicmp( (const unsigned char *) a, (const unsigned char *) b); }
	static int Collate( const char *a, const char *b) { return _mbscoll( (const unsigned char *) a, (const unsigned char *) b); }
	static int CollateNoCase( const char *a, const char *b) { return _mbsicoll( (const unsigned char *) a, (const unsigned char *) b); }
	static const char *FindString( const char *a, const char *b) { return (const char *) _mbsstr( (const unsigned char *) a, (const unsigned char *) b); }
	static const char *FindChar( const char *a, char ch) { return (const char *) _mbschr( (const unsigned char *) a, BYTE(ch)); }
	static const char *FindCharRev( const char *a, char ch) { return (const char *) _mbsrchr( (const unsigned char *) a, BYTE(ch)); }
	static const char *ScanSet( const char *a, const char *b) { return (const char *) _mbspbrk( (const unsigned char *) a, (const unsigned char *) b); }
	static int SpanIncluding( const char *a, const char *b) { return int(_mbsspn( (const unsigned char *) a, (const unsigned char *) b)); }
	static int SpanExcluding( const char *a, const char *b) { return int(_mbscspn( (const unsigned char *) a, (const unsigned char *) b)); }
	static int Uppercase( char *psz, int nSize) { return _mbsupr_s( (unsigned char *) psz, size_t(nSize)); }
	static int Lowercase( char *psz, int nSize) { return _mbslwr_s( (unsigned char *) psz, size_t(nSize)); }
	static void Reverse( char *psz) { _mbsrev((unsigned char *) psz); }
	static int FormattedLength( const char *pszFormat, va_list args) { return _vscprintf( pszFormat, args); }
	static void Format( char *pszBuffer, int nSize, const char *pszFormat, va_list args) { vsprintf_s( pszBuffer, size_t(nSize), pszFormat, args); }

	// The other type's string in this type, through the thread's ANSI code page.
	static int ConvertLength( const wchar_t *psz, int nLength) { return ::WideCharToMultiByte( CP_THREAD_ACP, 0, psz, nLength, NULL, 0, NULL, NULL); }
	static void Convert( char *pszDest, int nDest, const wchar_t *psz, int nLength) { ::WideCharToMultiByte( CP_THREAD_ACP, 0, psz, nLength, pszDest, nDest, NULL, NULL); }
	static int LoadString( HINSTANCE hInstance, UINT nID, char *pszBuffer, int nSize) { return ::LoadStringA( hInstance, nID, pszBuffer, nSize); }
};

template<> struct TStringTraits<wchar_t>
{
	typedef char OTHER;

	static int Length( const wchar_t *psz) { return psz ? int(wcslen(psz)) : 0; }
	static const wchar_t *CharNext( const wchar_t *p) { return *p ? p + 1 : p; }
	static int IsSpace( wchar_t ch) { return iswspace(ch); }
	static int Compare( const wchar_t *a, const wchar_t *b) { return wcscmp( a, b); }
	static int CompareNoCase( const wchar_t *a, const wchar_t *b) { return _wcsicmp( a, b); }
	static int Collate( const wchar_t *a, const wchar_t *b) { return wcscoll( a, b); }
	static int CollateNoCase( const wchar_t *a, const wchar_t *b) { return _wcsicoll( a, b); }
	static const wchar_t *FindString( const wchar_t *a, const wchar_t *b) { return wcsstr( a, b); }
	static const wchar_t *FindChar( const wchar_t *a, wchar_t ch) { return wcschr( a, ch); }
	static const wchar_t *FindCharRev( const wchar_t *a, wchar_t ch) { return wcsrchr( a, ch); }
	static const wchar_t *ScanSet( const wchar_t *a, const wchar_t *b) { return wcspbrk( a, b); }
	static int SpanIncluding( const wchar_t *a, const wchar_t *b) { return int(wcsspn( a, b)); }
	static int SpanExcluding( const wchar_t *a, const wchar_t *b) { return int(wcscspn( a, b)); }
	static int Uppercase( wchar_t *psz, int nSize) { return _wcsupr_s( psz, size_t(nSize)); }
	static int Lowercase( wchar_t *psz, int nSize) { return _wcslwr_s( psz, size_t(nSize)); }
	static void Reverse( wchar_t *psz) { _wcsrev(psz); }
	static int FormattedLength( const wchar_t *pszFormat, va_list args) { return _vscwprintf( pszFormat, args); }
	static void Format( wchar_t *pszBuffer, int nSize, const wchar_t *pszFormat, va_list args) { vswprintf_s( pszBuffer, size_t(nSize), pszFormat, args); }

	static int ConvertLength( const char *psz, int nLength) { return ::MultiByteToWideChar( CP_THREAD_ACP, 0, psz, nLength, NULL, 0); }
	static void Convert( wchar_t *pszDest, int nDest, const char *psz, int nLength) { ::MultiByteToWideChar( CP_THREAD_ACP, 0, psz, nLength, pszDest, nDest); }
	static int LoadString( HINSTANCE hInstance, UINT nID, wchar_t *pszBuffer, int nSize) { return ::LoadStringW( hInstance, nID, pszBuffer, nSize); }
};


//////////////////////////////////////////////////////////////////////
// CStringT

template< typename T > class CStringT
{
public:
	typedef TStringTraits<T> TRAITS;
	typedef typename TRAITS::OTHER Y;
	typedef T XCHAR;
	typedef T *PXSTR;
	typedef const T *PCXSTR;

protected:
	// The header before the characters. The empty string shares one that is never written.
	struct DATA
	{
		int m_nLength;
		int m_nAlloc;

		T *Chars() { return (T *) (this + 1); }
	};

	static DATA *EmptyData()
	{
		static DATA vEMPTY[2] = { { 0, 0 }, { 0, 0 } };
		return &vEMPTY[0];
	}

	DATA *Data() const { return ((DATA *) m_pszData) - 1; }

	void Attach( DATA *pData) { m_pszData = pData->Chars(); }

	static DATA *Allocate( int nLength)
	{
		// Room for nLength characters and the terminator, rounded up like a growing buffer.
		int nAlloc = nLength < 8 ? 8 : nLength + (nLength >> 2);
		DATA *pData = (DATA *) ::operator new( sizeof(DATA) + (nAlloc + 1) * sizeof(T));

		pData->m_nLength = 0;
		pData->m_nAlloc = nAlloc;
		pData->Chars()[0] = 0;

		return pData;
	}

	void Free()
	{
		DATA *pData = Data();

		if( pData != EmptyData() )
			::operator delete(pData);

		Attach(EmptyData());
	}

	// A buffer of at least nLength characters, the current ones kept.
	T *Reserve( int nLength)
	{
		DATA *pData = Data();

		if( pData != EmptyData() && pData->m_nAlloc >= nLength )
			return m_pszData;

		DATA *pNew = Allocate(nLength);
		memcpy( pNew->Chars(), m_pszData, (pData->m_nLength + 1) * sizeof(T));
		pNew->m_nLength = pData->m_nLength;

		if( pData != EmptyData() )
			::operator delete(pData);
		Attach(pNew);

		return m_pszData;
	}

	void SetLength( int nLength)
	{
		if( Data() == EmptyData() )
			return;

		Data()->m_nLength = nLength;
		m_pszData[nLength] = 0;
	}

	void Assign( const T *psz, int nLength)
	{
		if( nLength <= 0 )
		{
			Empty();
			return;
		}

		// The source may be inside this string.
		if( psz >= m_pszData && psz <= m_pszData + GetLength() )
		{
			CStringT strCopy( psz, nLength);
			Swap(strCopy);

			return;
		}

		Reserve(nLength);
		memcpy( m_pszData, psz, nLength * sizeof(T));
		SetLength(nLength);
	}

	void AssignOther( const Y *psz, int nLength)
	{
		if( !psz || nLength <= 0 )
		{
			Empty();
			return;
		}

		int nDest = TRAITS::ConvertLength( psz, nLength);
		Empty();

		if( nDest > 0 )
		{
			Reserve(nDest);
			TRAITS::Convert( m_pszData, nDest, psz, nLength);
			SetLength(nDest);
		}
	}

	void Swap( CStringT& strOther)
	{
		T *psz = m_pszData;

		m_pszData = strOther.m_pszData;
		strOther.m_pszData = psz;
	}

public:
	CStringT() { Attach(EmptyData()); }
	CStringT( const CStringT& strSrc) { Attach(EmptyData()); Assign( strSrc.m_pszData, strSrc.GetLength()); }
	CStringT( CStringT&& strSrc) { Attach(EmptyData()); Swap(strSrc); }
	CStringT( const T *psz) { Attach(EmptyData()); Assign( psz, TRAITS::Length(psz)); }
	CStringT( const T *psz, int nLength) { Attach(EmptyData()); Assign( psz, nLength); }
	TSTRING_EXPLICIT CStringT( T ch, int nRepeat = 1)
	{
		Attach(EmptyData());

		if( nRepeat > 0 )
		{
			Reserve(nRepeat);
			for( int i=0; i<nRepeat; i++)
				m_pszData[i] = ch;
			SetLength(nRepeat);
		}
	}

	TSTRING_EXPLICIT CStringT( const Y *psz) { Attach(EmptyData()); AssignOther( psz, int(TStringTraits<Y>::Length(psz))); }
	TSTRING_EXPLICIT CStringT( Y *psz) { Attach(EmptyData()); AssignOther( psz, int(TStringTraits<Y>::Length(psz))); }
	TSTRING_EXPLICIT CStringT( const Y *psz, int nLength) { Attach(EmptyData()); AssignOther( psz, nLength); }
	TSTRING_EXPLICIT CStringT( const CStringT<Y>& strSrc) { Attach(EmptyData()); AssignOther( strSrc.GetString(), strSrc.GetLength()); }
	TSTRING_EXPLICIT CStringT( const unsigned char *psz) { Attach(EmptyData()); *this = (const char *) psz; }

	~CStringT() { Free(); }

	CStringT& operator = ( const CStringT& strSrc) { if( this != &strSrc ) Assign( strSrc.m_pszData, strSrc.GetLength()); return *this; }
	CStringT& operator = ( CStringT&& strSrc) { if( this != &strSrc ) { Empty(); Swap(strSrc); } return *this; }
	CStringT& operator = ( const T *psz) { Assign( psz, TRAITS::Length(psz)); return *this; }
	CStringT& operator = ( T ch) { T vCHAR[2] = { ch, 0 }; Assign( vCHAR, 1); return *this; }
	CStringT& operator = ( const Y *psz) { AssignOther( psz, int(TStringTraits<Y>::Length(psz))); return *this; }
	CStringT& operator = ( const CStringT<Y>& strSrc) { AssignOther( strSrc.GetString(), strSrc.GetLength()); return *this; }
	CStringT& operator = ( const unsigned char *psz) { return *this = (const char *) psz; }

	CStringT& operator += ( const CStringT& str) { Append( str.m_pszData, str.GetLength()); return *this; }
	CStringT& operator += ( const T *psz) { Append(psz); return *this; }
	CStringT& operator += ( T ch) { AppendChar(ch); return *this; }
	CStringT& operator += ( const Y *psz) { return *this += CStringT(psz); }
	CStringT& operator += ( const CStringT<Y>& str) { return *this += CStringT(str); }

	operator const T* () const { return m_pszData; }
	const T *GetString() const { return m_pszData; }

	int GetLength() const { return Data()->m_nLength; }
	int GetAllocLength() const { return Data()->m_nAlloc; }
	bool IsEmpty() const { return GetLength() == 0; }
	void Empty() { Free(); }

	T GetAt( int iChar) const { return m_pszData[iChar]; }
	T operator [] ( int iChar) const { return m_pszData[iChar]; }
	void SetAt( int iChar, T ch) { if( iChar >= 0 && iChar < GetLength() ) m_pszData[iChar] = ch; }

	// The buffer for direct writes: ReleaseBuffer gives the length back.
	T *GetBuffer() { return Reserve(GetLength()); }
	T *GetBuffer( int nMinLength) { return Reserve( nMinLength > GetLength() ? nMinLength : GetLength()); }
	T *GetBufferSetLength( int nLength)
	{
		if( nLength <= 0 )
		{
			Empty();
			Reserve(0);

			return m_pszData;
		}

		Reserve(nLength);
		SetLength(nLength);

		return m_pszData;
	}

	void ReleaseBuffer( int nNewLength = -1)
	{
		if( nNewLength < 0 )
			nNewLength = TRAITS::Length(m_pszData);

		ReleaseBufferSetLength(nNewLength);
	}

	void ReleaseBufferSetLength( int nNewLength)
	{
		if( Data() == EmptyData() )
			return;

		if( nNewLength > Data()->m_nAlloc )
			nNewLength = Data()->m_nAlloc;

		if( nNewLength <= 0 )
			Empty();
		else
			SetLength(nNewLength);
	}

	void Preallocate( int nLength) { Reserve(nLength); }
	void FreeExtra() {}
	void Truncate( int nNewLength) { if( nNewLength >= 0 && nNewLength < GetLength() ) { if(nNewLength) SetLength(nNewLength); else Empty(); } }

	void SetString( const T *psz) { *this = psz; }
	void SetString( const T *psz, int nLength) { Assign( psz, nLength); }

	void Append( const T *psz) { Append( psz, TRAITS::Length(psz)); }
	void Append( const CStringT& str) { Append( str.m_pszData, str.GetLength()); }
	void Append( const T *psz, int nLength)
	{
		if( !psz || nLength <= 0 )
			return;

		// The source may be inside this string.
		if( psz >= m_pszData && psz <= m_pszData + GetLength() )
		{
			CStringT strCopy( psz, nLength);
			Append( strCopy.m_pszData, nLength);

			return;
		}

		int nOld = GetLength();
		Reserve(nOld + nLength);
		memcpy( m_pszData + nOld, psz, nLength * sizeof(T));
		SetLength(nOld + nLength);
	}

	void AppendChar( T ch)
	{
		int nOld = GetLength();

		Reserve(nOld + 1);
		m_pszData[nOld] = ch;
		SetLength(nOld + 1);
	}

	// Formatting
	void Format( const T *pszFormat, ...)
	{
		va_list args;

		va_start( args, pszFormat);
		FormatV( pszFormat, args);
		va_end(args);
	}

	void AppendFormat( const T *pszFormat, ...)
	{
		va_list args;

		va_start( args, pszFormat);
		AppendFormatV( pszFormat, args);
		va_end(args);
	}

	void FormatV( const T *pszFormat, va_list args)
	{
		// The format may be this string.
		CStringT strFormat(pszFormat);

		Empty();
		AppendFormatV( strFormat, args);
	}

	void AppendFormatV( const T *pszFormat, va_list args)
	{
		va_list vCOPY;

		va_copy( vCOPY, args);
		int nLength = TRAITS::FormattedLength( pszFormat, vCOPY);
		va_end(vCOPY);

		if( nLength <= 0 )
			return;

		CStringT strFormat(pszFormat);
		int nOld = GetLength();

		Reserve(nOld + nLength);
		TRAITS::Format( m_pszData + nOld, nLength + 1, strFormat, args);
		SetLength(nOld + nLength);
	}

	BOOL LoadString( UINT nID)
	{
		T vBUF[1024];
		int nLength = TRAITS::LoadString( ::GetModuleHandle(NULL), nID, vBUF, 1024);

		if( nLength <= 0 )
			return FALSE;

		Assign( vBUF, nLength);
		return TRUE;
	}

	// Substrings (out-of-range requests give what fits, like ATL)
	CStringT Mid( int iFirst) const { return Mid( iFirst, GetLength() - iFirst); }
	CStringT Mid( int iFirst, int nCount) const
	{
		if( iFirst < 0 )
			iFirst = 0;

		if( nCount < 0 )
			nCount = 0;

		if( iFirst + nCount > GetLength() )
			nCount = GetLength() - iFirst;

		if( iFirst > GetLength() )
			nCount = 0;

		if( iFirst == 0 && iFirst + nCount == GetLength() )
			return *this;

		return CStringT( m_pszData + iFirst, nCount);
	}

	CStringT Left( int nCount) const
	{
		if( nCount < 0 )
			nCount = 0;

		return nCount >= GetLength() ? *this : CStringT( m_pszData, nCount);
	}

	CStringT Right( int nCount) const
	{
		if( nCount < 0 )
			nCount = 0;

		return nCount >= GetLength() ? *this : CStringT( m_pszData + GetLength() - nCount, nCount);
	}

	CStringT SpanIncluding( const T *pszCharSet) const { return Left(TRAITS::SpanIncluding( m_pszData, pszCharSet)); }
	CStringT SpanExcluding( const T *pszCharSet) const { return Left(TRAITS::SpanExcluding( m_pszData, pszCharSet)); }

	CStringT Tokenize( const T *pszTokens, int& iStart) const
	{
		if( iStart < 0 )
			iStart = 0;

		if( !pszTokens || !*pszTokens )
		{
			if( iStart < GetLength() )
				return CStringT(m_pszData + iStart);
		}
		else
		{
			const T *pszPlace = m_pszData + iStart;
			const T *pszEnd = m_pszData + GetLength();

			if( pszPlace < pszEnd )
			{
				int nIncluding = TRAITS::SpanIncluding( pszPlace, pszTokens);

				if( pszPlace + nIncluding < pszEnd )
				{
					pszPlace += nIncluding;
					int nExcluding = TRAITS::SpanExcluding( pszPlace, pszTokens);
					int iFrom = iStart + nIncluding;

					iStart = iFrom + nExcluding + 1;
					return Mid( iFrom, nExcluding);
				}
			}
		}

		// No more tokens.
		iStart = -1;
		return CStringT();
	}

	// Searches
	int Find( T ch, int iStart = 0) const
	{
		if( iStart < 0 || iStart >= GetLength() )
			return -1;

		const T *psz = TRAITS::FindChar( m_pszData + iStart, ch);
		return psz ? int(psz - m_pszData) : -1;
	}

	int Find( const T *pszSub, int iStart = 0) const
	{
		if( !pszSub || iStart < 0 || iStart > GetLength() )
			return -1;

		const T *psz = TRAITS::FindString( m_pszData + iStart, pszSub);
		return psz ? int(psz - m_pszData) : -1;
	}

	int ReverseFind( T ch) const
	{
		const T *psz = TRAITS::FindCharRev( m_pszData, ch);
		return psz ? int(psz - m_pszData) : -1;
	}

	int FindOneOf( const T *pszCharSet) const
	{
		const T *psz = TRAITS::ScanSet( m_pszData, pszCharSet);
		return psz ? int(psz - m_pszData) : -1;
	}

	// Changes
	int Replace( T chOld, T chNew)
	{
		int nCount = 0;

		if( chOld == chNew )
			return 0;

		for( int iChar = 0; iChar < GetLength(); iChar = int(TRAITS::CharNext(m_pszData + iChar) - m_pszData))
			if( m_pszData[iChar] == chOld )
			{
				m_pszData[iChar] = chNew;
				nCount++;
			}

		return nCount;
	}

	int Replace( const T *pszOld, const T *pszNew)
	{
		int nOldLength = TRAITS::Length(pszOld);
		int nNewLength = TRAITS::Length(pszNew);
		int nCount = 0;

		if( !nOldLength )
			return 0;

		// Each match is found by the multibyte search, so it starts on a character.
		CStringT strResult;
		const T *pszStart = m_pszData;
		const T *pszEnd = m_pszData + GetLength();

		while( pszStart < pszEnd )
		{
			const T *pszTarget = TRAITS::FindString( pszStart, pszOld);

			if(!pszTarget)
				break;

			strResult.Append( pszStart, int(pszTarget - pszStart));
			strResult.Append( pszNew, nNewLength);
			pszStart = pszTarget + nOldLength;
			nCount++;
		}

		if(nCount)
		{
			strResult.Append( pszStart, int(pszEnd - pszStart));
			Swap(strResult);
		}

		return nCount;
	}

	int Remove( T chRemove)
	{
		int nLength = GetLength();
		T *pszDest = m_pszData;
		const T *pszSource = m_pszData;
		const T *pszEnd = m_pszData + nLength;

		while( pszSource < pszEnd )
		{
			const T *pszNext = TRAITS::CharNext(pszSource);

			// Every byte of a multibyte character is kept or removed together.
			if( *pszSource != chRemove )
				while( pszSource < pszNext )
					*pszDest++ = *pszSource++;

			pszSource = pszNext;
		}

		int nCount = int(pszSource - pszDest);
		if(nCount)
			ReleaseBufferSetLength(nLength - nCount);

		return nCount;
	}

	int Insert( int iIndex, T ch)
	{
		T vCHAR[2] = { ch, 0 };
		return Insert( iIndex, vCHAR);
	}

	int Insert( int iIndex, const T *psz)
	{
		int nInsert = TRAITS::Length(psz);
		int nLength = GetLength();

		if( iIndex < 0 )
			iIndex = 0;

		if( iIndex > nLength )
			iIndex = nLength;

		if( nInsert > 0 )
		{
			CStringT strInsert( psz, nInsert);

			Reserve(nLength + nInsert);
			memmove( m_pszData + iIndex + nInsert, m_pszData + iIndex, (nLength - iIndex + 1) * sizeof(T));
			memcpy( m_pszData + iIndex, strInsert.m_pszData, nInsert * sizeof(T));
			SetLength(nLength + nInsert);
		}

		return GetLength();
	}

	int Delete( int iIndex, int nCount = 1)
	{
		int nLength = GetLength();

		if( iIndex < 0 )
			iIndex = 0;

		if( nCount < 0 )
			nCount = 0;

		if( nCount + iIndex > nLength )
			nCount = nLength - iIndex;

		if( nCount > 0 )
		{
			memmove( m_pszData + iIndex, m_pszData + iIndex + nCount, (nLength - iIndex - nCount + 1) * sizeof(T));
			ReleaseBufferSetLength(nLength - nCount);
		}

		return GetLength();
	}

	// ATL throws when the CRT refuses an invalid multibyte string; the string is left as it was.
	CStringT& MakeUpper() { return MapCase(TRAITS::Uppercase); }
	CStringT& MakeLower() { return MapCase(TRAITS::Lowercase); }
	CStringT& MakeReverse() { if(!IsEmpty()) TRAITS::Reverse(m_pszData); return *this; }

	// Trimming walks the characters, so a trail byte is never taken for a space.
	CStringT& TrimRight() { return TrimRightIf( [] ( T ch) { return TRAITS::IsSpace(ch) != 0; }); }
	CStringT& TrimRight( T chTarget) { return TrimRightIf( [chTarget] ( T ch) { return ch == chTarget; }); }
	CStringT& TrimRight( const T *pszTargets)
	{
		if( !pszTargets || !*pszTargets )
			return *this;

		return TrimRightIf( [pszTargets] ( T ch) { return TRAITS::FindChar( pszTargets, ch) != NULL; });
	}

	CStringT& TrimLeft() { return TrimLeftIf( [] ( T ch) { return TRAITS::IsSpace(ch) != 0; }); }
	CStringT& TrimLeft( T chTarget) { return TrimLeftIf( [chTarget] ( T ch) { return ch == chTarget; }); }
	CStringT& TrimLeft( const T *pszTargets)
	{
		if( !pszTargets || !*pszTargets )
			return *this;

		return TrimLeftIf( [pszTargets] ( T ch) { return TRAITS::FindChar( pszTargets, ch) != NULL; });
	}

	CStringT& Trim() { return TrimRight().TrimLeft(); }
	CStringT& Trim( T chTarget) { return TrimRight(chTarget).TrimLeft(chTarget); }
	CStringT& Trim( const T *pszTargets) { return TrimRight(pszTargets).TrimLeft(pszTargets); }

	// Comparisons
	int Compare( const T *psz) const { return TRAITS::Compare( m_pszData, psz); }
	int CompareNoCase( const T *psz) const { return TRAITS::CompareNoCase( m_pszData, psz); }
	int Collate( const T *psz) const { return TRAITS::Collate( m_pszData, psz); }
	int CollateNoCase( const T *psz) const { return TRAITS::CollateNoCase( m_pszData, psz); }

	static int StringLength( const T *psz) { return TRAITS::Length(psz); }

	friend CStringT operator + ( const CStringT& a, const CStringT& b) { CStringT s(a); s.Append(b); return s; }
	friend CStringT operator + ( const CStringT& a, const T *b) { CStringT s(a); s.Append(b); return s; }
	friend CStringT operator + ( const T *a, const CStringT& b) { CStringT s(a); s.Append(b); return s; }
	friend CStringT operator + ( const CStringT& a, T b) { CStringT s(a); s.AppendChar(b); return s; }
	friend CStringT operator + ( T a, const CStringT& b) { CStringT s( a, 1); s.Append(b); return s; }

	friend bool operator == ( const CStringT& a, const CStringT& b) { return a.GetLength() == b.GetLength() && a.Compare(b) == 0; }
	friend bool operator == ( const CStringT& a, const T *b) { return a.Compare(b ? b : EmptyData()->Chars()) == 0; }
	friend bool operator == ( const T *a, const CStringT& b) { return b.Compare(a ? a : EmptyData()->Chars()) == 0; }
	friend bool operator == ( const CStringT& a, T b) { return a.GetLength() == 1 && a.m_pszData[0] == b; }
	friend bool operator == ( T a, const CStringT& b) { return b.GetLength() == 1 && b.m_pszData[0] == a; }
	friend bool operator != ( const CStringT& a, const CStringT& b) { return !(a == b); }
	friend bool operator != ( const CStringT& a, const T *b) { return !(a == b); }
	friend bool operator != ( const T *a, const CStringT& b) { return !(a == b); }
	friend bool operator != ( const CStringT& a, T b) { return !(a == b); }
	friend bool operator != ( T a, const CStringT& b) { return !(a == b); }
	friend bool operator < ( const CStringT& a, const CStringT& b) { return a.Compare(b) < 0; }
	friend bool operator < ( const CStringT& a, const T *b) { return a.Compare(b) < 0; }
	friend bool operator < ( const T *a, const CStringT& b) { return b.Compare(a) > 0; }
	friend bool operator > ( const CStringT& a, const CStringT& b) { return a.Compare(b) > 0; }
	friend bool operator > ( const CStringT& a, const T *b) { return a.Compare(b) > 0; }
	friend bool operator > ( const T *a, const CStringT& b) { return b.Compare(a) < 0; }
	friend bool operator <= ( const CStringT& a, const CStringT& b) { return a.Compare(b) <= 0; }
	friend bool operator <= ( const CStringT& a, const T *b) { return a.Compare(b) <= 0; }
	friend bool operator <= ( const T *a, const CStringT& b) { return b.Compare(a) >= 0; }
	friend bool operator >= ( const CStringT& a, const CStringT& b) { return a.Compare(b) >= 0; }
	friend bool operator >= ( const CStringT& a, const T *b) { return a.Compare(b) >= 0; }
	friend bool operator >= ( const T *a, const CStringT& b) { return b.Compare(a) <= 0; }

protected:
	CStringT& MapCase( int (*pfnMap)( T *, int))
	{
		if(IsEmpty())
			return *this;

		CStringT strOld(*this);
		if(pfnMap( m_pszData, GetLength() + 1))
			Swap(strOld);

		return *this;
	}

	template< typename F > CStringT& TrimRightIf( F fnTrim)
	{
		const T *psz = m_pszData;
		const T *pszLast = NULL;

		while(*psz)
		{
			if(fnTrim(*psz))
			{
				if(!pszLast)
					pszLast = psz;
			}
			else
				pszLast = NULL;

			psz = TRAITS::CharNext(psz);
		}

		if(pszLast)
			Truncate(int(pszLast - m_pszData));

		return *this;
	}

	template< typename F > CStringT& TrimLeftIf( F fnTrim)
	{
		const T *psz = m_pszData;

		while( *psz && fnTrim(*psz) )
			psz = TRAITS::CharNext(psz);

		if( psz != m_pszData )
			Delete( 0, int(psz - m_pszData));

		return *this;
	}

protected:
	T *m_pszData;
};

typedef CStringT<char>				CStringA;
typedef CStringT<wchar_t>			CStringW;
typedef CStringT<TCHAR>				CString;

#endif // !defined __TSTRING_H
