// TCodePage.h: the code page of the game's texts.
//
// The texts (TString.tcd, the charts, the chat) are multibyte strings in the code page of the
// client's nation, whatever the system's: CTCodePage::Set picks it, and the conversions to and
// from wide strings, the double byte lead tests and the case tables of the multibyte strings
// follow it. Windows converts with MultiByteToWideChar, the other systems with iconv.
//////////////////////////////////////////////////////////////////////

#pragma once

#include <string.h>
#include <wchar.h>
#include <wctype.h>

#ifdef _WIN32
#include <mbctype.h>
#else
#include <iconv.h>
#include <locale.h>
#include <errno.h>
#include <stdio.h>
#endif

#define TCP_UTF8						65001

class CTCodePage
{
public:
	static UINT Get()
	{
		return Data().m_nCodePage;
	};

	// Sets the code page of the game's texts (1252, 949, ...). Windows also gets it as the CRT's
	// multibyte code page, for its _mbs functions.
	static void Set( UINT nCodePage)
	{
		DATA& vDATA = Data();

		vDATA.m_nCodePage = nCodePage;
		vDATA.m_bDBCS = FALSE;
#ifdef _WIN32
		_setmbcp(int(nCodePage));
#endif

		for( int i=0; i<256; i++)
		{
			vDATA.m_vLEAD[i] = IsLeadByte( nCodePage, BYTE(i));
			vDATA.m_vUPPER[i] = MapCase( nCodePage, BYTE(i), TRUE);
			vDATA.m_vLOWER[i] = MapCase( nCodePage, BYTE(i), FALSE);

			if(vDATA.m_vLEAD[i])
				vDATA.m_bDBCS = TRUE;
		}
	};

	// A double byte code page (932, 936, 949, 950).
	static BOOL IsDBCS()
	{
		return Data().m_bDBCS;
	};

	// The first byte of a double byte character.
	static BOOL IsLeadByte( BYTE ch)
	{
		return Data().m_vLEAD[ch];
	};

	// The case mapping of a single byte character (ASCII and, in the single byte code pages,
	// the accented letters). Lead bytes map to themselves.
	static BYTE ToUpper( BYTE ch)
	{
		return Data().m_vUPPER[ch];
	};

	static BYTE ToLower( BYTE ch)
	{
		return Data().m_vLOWER[ch];
	};

	// A wide character in the other case.
	static wchar_t MapWide( wchar_t ch, BOOL bUpper)
	{
#ifdef _WIN32
		if(bUpper)
			::CharUpperBuffW( &ch, 1);
		else
			::CharLowerBuffW( &ch, 1);
#else
		static locale_t hLOCALE = newlocale( LC_CTYPE_MASK, "C.UTF-8", (locale_t) 0);

		if(hLOCALE)
			ch = wchar_t(bUpper ? towupper_l( wint_t(ch), hLOCALE) : towlower_l( wint_t(ch), hLOCALE));
#endif

		return ch;
	};

	// MultiByteToWideChar: nLength -1 converts up to and with the terminator; NULL pszDest
	// returns the length needed. nCodePage 0 is the game's code page.
	static int ToWide( LPCSTR psz, int nLength, wchar_t *pszDest, int nDest, UINT nCodePage = 0)
	{
		if(!nCodePage)
			nCodePage = Get();

#ifdef _WIN32
		return ::MultiByteToWideChar( nCodePage, 0, psz, nLength, pszDest, nDest);
#else
		return Convert( "WCHAR_T", CodePageName(nCodePage), psz, nLength < 0 ? strlen(psz) + 1 : size_t(nLength), (char *) pszDest, size_t(nDest), sizeof(wchar_t));
#endif
	};

	// WideCharToMultiByte, likewise.
	static int FromWide( const wchar_t *psz, int nLength, LPSTR pszDest, int nDest, UINT nCodePage = 0)
	{
		if(!nCodePage)
			nCodePage = Get();

#ifdef _WIN32
		return ::WideCharToMultiByte( nCodePage, 0, psz, nLength, pszDest, nDest, NULL, NULL);
#else
		return Convert( CodePageName(nCodePage), "WCHAR_T", (const char *) psz, (nLength < 0 ? wcslen(psz) + 1 : size_t(nLength)) * sizeof(wchar_t), pszDest, size_t(nDest), 1);
#endif
	};

protected:
	struct DATA
	{
		UINT m_nCodePage;
		BOOL m_bDBCS;

		BYTE m_vLEAD[256];
		BYTE m_vUPPER[256];
		BYTE m_vLOWER[256];

		DATA()
		{
			m_nCodePage = 0;
			m_bDBCS = FALSE;
		};
	};

	static DATA& Data()
	{
		static DATA vDATA = InitData();
		return vDATA;
	};

	// Western until Set: no lead bytes, ASCII case.
	static DATA InitData()
	{
		DATA vDATA;

		vDATA.m_nCodePage = 1252;
		for( int i=0; i<256; i++)
		{
			vDATA.m_vLEAD[i] = FALSE;
			vDATA.m_vUPPER[i] = BYTE(i >= 'a' && i <= 'z' ? i - 'a' + 'A' : i);
			vDATA.m_vLOWER[i] = BYTE(i >= 'A' && i <= 'Z' ? i - 'A' + 'a' : i);
		}

		return vDATA;
	};

	static BYTE IsLeadByte( UINT nCodePage, BYTE ch)
	{
#ifdef _WIN32
		return ::IsDBCSLeadByteEx( nCodePage, ch) ? TRUE : FALSE;
#else
		switch(nCodePage)
		{
		case 932	: return (ch >= 0x81 && ch <= 0x9F) || (ch >= 0xE0 && ch <= 0xFC) ? TRUE : FALSE;
		case 936	:
		case 949	:
		case 950	: return ch >= 0x81 && ch <= 0xFE ? TRUE : FALSE;
		}

		return FALSE;
#endif
	};

	// A single byte character's case through its wide character, when it maps back to one byte.
	static BYTE MapCase( UINT nCodePage, BYTE ch, BOOL bUpper)
	{
		if( ch < 0x80 )
		{
			if(bUpper)
				return BYTE(ch >= 'a' && ch <= 'z' ? ch - 'a' + 'A' : ch);

			return BYTE(ch >= 'A' && ch <= 'Z' ? ch - 'A' + 'a' : ch);
		}

		if(IsLeadByte( nCodePage, ch))
			return ch;

		char vCHAR[2] = { char(ch), 0 };
		wchar_t vWIDE[2] = { 0, 0 };

		if( ToWide( vCHAR, 1, vWIDE, 1, nCodePage) != 1 )
			return ch;

		vWIDE[0] = MapWide( vWIDE[0], bUpper);

		char vBACK[4] = { 0, 0, 0, 0 };
		wchar_t vCHECK[2] = { 0, 0 };

		// Only when one byte stands for it and comes back to the same character.
		if( FromWide( vWIDE, 1, vBACK, 4, nCodePage) != 1 ||
			ToWide( vBACK, 1, vCHECK, 1, nCodePage) != 1 || vCHECK[0] != vWIDE[0] )
			return ch;

		return BYTE(vBACK[0]);
	};

#ifndef _WIN32
	static const char *CodePageName( UINT nCodePage)
	{
		static thread_local char vNAME[16];

		if( nCodePage == TCP_UTF8 )
			return "UTF-8";

		snprintf( vNAME, sizeof(vNAME), "CP%u", nCodePage);
		return vNAME;
	};

	// iconv from pszFrom to pszTo; the result's length in units of nUnit bytes, 0 when the
	// destination is too small. nDEST 0 only measures. Bytes that are no character are skipped.
	static int Convert( const char *pszTo, const char *pszFrom, const char *pSRC, size_t nSRC, char *pDEST, size_t nDEST, size_t nUnit)
	{
		iconv_t hICONV = iconv_open( pszTo, pszFrom);

		if( hICONV == (iconv_t) -1 )
			return 0;

		BOOL bMeasure = !pDEST || !nDEST;
		char vBUF[256];
		char *pIN = (char *) pSRC;
		size_t nIN = nSRC;
		size_t nTOTAL = 0;
		BOOL bResult = TRUE;

		while( nIN && bResult )
		{
			char *pOUT = bMeasure ? vBUF : pDEST + nTOTAL;
			size_t nROOM = bMeasure ? sizeof(vBUF) : nDEST * nUnit - nTOTAL;
			size_t nBEFORE = nROOM;

			if( iconv( hICONV, &pIN, &nIN, &pOUT, &nROOM) == (size_t) -1 )
			{
				if( errno == EILSEQ || errno == EINVAL )
				{
					pIN++;
					nIN--;
				}
				else if( errno != E2BIG || !bMeasure )
					bResult = FALSE;
			}

			nTOTAL += nBEFORE - nROOM;
		}

		iconv_close(hICONV);
		return bResult ? int(nTOTAL / nUnit) : 0;
	};
#endif
};
