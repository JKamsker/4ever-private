#include "StdAfx.h"
#include "TTextRaster.h"

#include <algorithm>
#include <map>
#include <math.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>


//////////////////////////////////////////////////////////////////////
// GDI+

#ifdef _WIN32
namespace
{
	void MeasureGDIPlus( CFont *pFont, LPCWSTR lpszText, BOOL bTrailingSpaces, int nRenderingHint, FLOAT *pWidth, FLOAT *pHeight)
	{
		HDC hDC = CreateCompatibleDC(NULL);
		HFONT hOLDFONT = (HFONT) SelectObject( hDC, TSAFE_HANDLE(pFont));

		{
			Gdiplus::Font vFONT(hDC);
			Gdiplus::Graphics vGDI(hDC);
			Gdiplus::RectF vRECT;

			vGDI.SetTextRenderingHint((Gdiplus::TextRenderingHint) nRenderingHint);

			if(bTrailingSpaces)
			{
				Gdiplus::StringFormat vFORMAT(Gdiplus::StringFormatFlagsMeasureTrailingSpaces);
				vGDI.MeasureString( lpszText, -1, &vFONT, Gdiplus::PointF( 0.0f, 0.0f), &vFORMAT, &vRECT);
			}
			else
				vGDI.MeasureString( lpszText, -1, &vFONT, Gdiplus::PointF( 0.0f, 0.0f), &vRECT);

			*pWidth = vRECT.Width;
			*pHeight = vRECT.Height;
		}

		SelectObject( hDC, hOLDFONT);
		DeleteDC(hDC);
	}

	void DrawGDIPlus( CFont *pFont, LPCWSTR lpszText, int nRenderingHint, FLOAT fLeft, FLOAT fTop, FLOAT fWidth, FLOAT fHeight, UINT nFormat, int nWidth, int nHeight, LPBYTE pCoverage)
	{
		HDC hDC = CreateCompatibleDC(NULL);
		HFONT hOLDFONT = (HFONT) SelectObject( hDC, TSAFE_HANDLE(pFont));
		BITMAPINFO bmi;
		LPDWORD pBits = NULL;

		ZeroMemory( &bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = nWidth;
		bmi.bmiHeader.biHeight = -nHeight;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biBitCount = 32;

		HBITMAP hBMP = CreateDIBSection( hDC, &bmi, DIB_RGB_COLORS, (LPVOID *) &pBits, NULL, 0);
		HBITMAP hOLDBMP = (HBITMAP) SelectObject( hDC, hBMP);

		if( hBMP && pBits )
		{
			{
				Gdiplus::Font vFONT(hDC);
				Gdiplus::StringFormat vFORMAT;
				Gdiplus::Graphics vGDI(hDC);

				if( nFormat & DT_CENTER )
					vFORMAT.SetAlignment(Gdiplus::StringAlignmentCenter);
				else if( nFormat & DT_RIGHT )
					vFORMAT.SetAlignment(Gdiplus::StringAlignmentFar);
				else
					vFORMAT.SetAlignment(Gdiplus::StringAlignmentNear);

				if( nFormat & DT_VCENTER )
					vFORMAT.SetLineAlignment(Gdiplus::StringAlignmentCenter);
				else
					vFORMAT.SetLineAlignment(Gdiplus::StringAlignmentNear);

				vGDI.SetTextRenderingHint((Gdiplus::TextRenderingHint) nRenderingHint);
				vGDI.DrawString(
					lpszText, -1,
					&vFONT,
					Gdiplus::RectF( fLeft, fTop, fWidth, fHeight),
					&vFORMAT,
					TTEMP(Gdiplus::SolidBrush( Gdiplus::Color( 0xFF, 0xFF, 0xFF))));
			}

			for( int i=0; i<nWidth * nHeight; i++)
				pCoverage[i] = BYTE(pBits[i] & 0xFF);
		}

		SelectObject( hDC, hOLDBMP);
		SelectObject( hDC, hOLDFONT);

		if(hBMP)
			DeleteObject(hBMP);
		DeleteDC(hDC);
	}
}
#endif


//////////////////////////////////////////////////////////////////////
// Font files

namespace
{
	template< class T > T TMin( T a, T b) { return a < b ? a : b; }
	template< class T > T TMax( T a, T b) { return a > b ? a : b; }

	// One font of a file (a collection holds several), loaded on first use.
	struct TFONTFILE
	{
		CString m_strPath;
		int m_nIndex;

		BOOL m_bBold;
		BOOL m_bItalic;

		BOOL m_bLoaded;
		BOOL m_bFailed;

		std::vector<BYTE> m_vDATA;
		stbtt_fontinfo m_vINFO;

		int m_nUPEM;
		int m_nWinAscent;
		int m_nWinDescent;
		int m_nLineSpacing;

		// The glyph of each code point up to 0xFFFF, -1 until looked up.
		std::vector<int> m_vGLYPH;

		TFONTFILE() : m_nIndex(0), m_bBold(FALSE), m_bItalic(FALSE), m_bLoaded(FALSE), m_bFailed(FALSE),
			m_nUPEM(2048), m_nWinAscent(0), m_nWinDescent(0), m_nLineSpacing(0)
		{
		}
	};

	// A LOGFONT's fonts: the face asked for (or GDI's substitute) first, then the fonts that
	// have the glyphs it lacks, and the size of the em in pixels.
	struct TFONTCHAIN
	{
		std::vector<TFONTFILE *> m_vFILE;
		FLOAT m_fEM;
	};

	// Glyph advances are 3 % wider in GDI+'s layout, with 1/6 em of padding on each side.
	const FLOAT TGDIPLUS_SPACING = 1.03f;
	const FLOAT TGDIPLUS_PADDING = 1.0f / 6.0f;

	std::mutex g_vLOCK;
	BOOL g_bFontFiles = FALSE;
	BOOL g_bScanned = FALSE;

	std::vector< std::unique_ptr<TFONTFILE> > g_vFONTFILE;
	std::map< CString, std::vector<TFONTFILE *> > g_mapFAMILY;
	std::map< std::string, TFONTCHAIN> g_mapCHAIN;

	CString LowerName( CString strName)
	{
		strName.MakeLower();
		strName.Trim();

		return strName;
	}

	DWORD ReadBE( const BYTE *p, int nBytes)
	{
		DWORD dwValue = 0;

		for( int i=0; i<nBytes; i++)
			dwValue = (dwValue << 8) | p[i];

		return dwValue;
	}

	BOOL ReadAt( FILE *pFILE, long nOffset, void *pDATA, size_t nSize)
	{
		return !fseek( pFILE, nOffset, SEEK_SET) && fread( pDATA, 1, nSize, pFILE) == nSize ? TRUE : FALSE;
	}

	// The family names (every language of the Windows names) and the style of the font at
	// nOffset of the file, read from its name and head tables.
	BOOL ReadFontNames( FILE *pFILE, long nOffset, std::vector<CString>& vNAME, BOOL& bBold, BOOL& bItalic)
	{
		BYTE vHEAD[12];

		if(!ReadAt( pFILE, nOffset, vHEAD, 12))
			return FALSE;

		int nTables = int(ReadBE( vHEAD + 4, 2));
		std::vector<BYTE> vDIR( nTables * 16);

		if( !nTables || !ReadAt( pFILE, nOffset + 12, &vDIR[0], vDIR.size()) )
			return FALSE;

		long nName = 0;
		long nHead = 0;
		DWORD dwNameSize = 0;

		for( int i=0; i<nTables; i++)
		{
			const BYTE *p = &vDIR[i * 16];

			if(!memcmp( p, "name", 4))
			{
				nName = long(ReadBE( p + 8, 4));
				dwNameSize = ReadBE( p + 12, 4);
			}
			else if(!memcmp( p, "head", 4))
				nHead = long(ReadBE( p + 8, 4));
		}

		if( !nName || !nHead || !dwNameSize || dwNameSize > 4 * 1024 * 1024 )
			return FALSE;

		BYTE vMACSTYLE[2];
		if(!ReadAt( pFILE, nHead + 44, vMACSTYLE, 2))
			return FALSE;

		bBold = (vMACSTYLE[1] & 0x01) ? TRUE : FALSE;
		bItalic = (vMACSTYLE[1] & 0x02) ? TRUE : FALSE;

		std::vector<BYTE> vTABLE(dwNameSize);
		if(!ReadAt( pFILE, nName, &vTABLE[0], dwNameSize))
			return FALSE;

		DWORD dwCount = ReadBE( &vTABLE[2], 2);
		DWORD dwStrings = ReadBE( &vTABLE[4], 2);

		for( DWORD i=0; i<dwCount && 6 + i * 12 + 12 <= dwNameSize; i++)
		{
			const BYTE *p = &vTABLE[6 + i * 12];
			DWORD dwPlatform = ReadBE( p, 2);
			DWORD dwNameID = ReadBE( p + 6, 2);
			DWORD dwLength = ReadBE( p + 8, 2);
			DWORD dwOffset = dwStrings + ReadBE( p + 10, 2);

			if( dwNameID != 1 || dwOffset + dwLength > dwNameSize )
				continue;

			// Windows names are UTF-16 big endian, Macintosh ones single bytes.
			CStringW strWide;

			if( dwPlatform == 3 )
				for( DWORD j=0; j + 1 < dwLength; j += 2)
					strWide.AppendChar(wchar_t(ReadBE( &vTABLE[dwOffset + j], 2)));
			else if( dwPlatform == 1 )
				for( DWORD j=0; j<dwLength; j++)
					strWide.AppendChar(wchar_t(vTABLE[dwOffset + j]));
			else
				continue;

			vNAME.push_back(LowerName(CString(strWide)));
		}

		return !vNAME.empty();
	}

	void AddFontFile( const CString& strPath)
	{
		FILE *pFILE = TFileOpen( strPath, _T("rb"));
		BYTE vTAG[12];

		if(!pFILE)
			return;

		std::vector<long> vOFFSET;

		if(ReadAt( pFILE, 0, vTAG, 12))
		{
			if(!memcmp( vTAG, "ttcf", 4))
			{
				DWORD dwCount = ReadBE( vTAG + 8, 4);

				for( DWORD i=0; i<dwCount && i<64; i++)
				{
					BYTE vOFF[4];

					if(ReadAt( pFILE, long(12 + i * 4), vOFF, 4))
						vOFFSET.push_back(long(ReadBE( vOFF, 4)));
				}
			}
			else
				vOFFSET.push_back(0);
		}

		for( size_t i=0; i<vOFFSET.size(); i++)
		{
			std::vector<CString> vNAME;
			BOOL bBold = FALSE;
			BOOL bItalic = FALSE;

			if(!ReadFontNames( pFILE, vOFFSET[i], vNAME, bBold, bItalic))
				continue;

			std::unique_ptr<TFONTFILE> pFONT(new TFONTFILE());

			pFONT->m_strPath = strPath;
			pFONT->m_nIndex = int(i);
			pFONT->m_bBold = bBold;
			pFONT->m_bItalic = bItalic;

			for( size_t j=0; j<vNAME.size(); j++)
			{
				std::vector<TFONTFILE *>& vFAMILY = g_mapFAMILY[vNAME[j]];

				if( std::find( vFAMILY.begin(), vFAMILY.end(), pFONT.get()) == vFAMILY.end() )
					vFAMILY.push_back(pFONT.get());
			}

			g_vFONTFILE.push_back(std::move(pFONT));
		}

		fclose(pFILE);
	}

	BOOL IsFontFile( const CString& strName)
	{
		CString strExt = strName.Right(4);

		strExt.MakeLower();
		return strExt == _T(".ttf") || strExt == _T(".ttc") || strExt == _T(".otf") ? TRUE : FALSE;
	}

	// Every font file of the folder and its subfolders.
	void ScanFolder( const CString& strFolder, int nDepth)
	{
		if( nDepth > 4 )
			return;

#ifdef _WIN32
		WIN32_FIND_DATA vFIND;
		HANDLE hFIND = ::FindFirstFile( strFolder + _T("\\*"), &vFIND);

		if( hFIND == INVALID_HANDLE_VALUE )
			return;

		do
		{
			CString strName(vFIND.cFileName);

			if( strName == _T(".") || strName == _T("..") )
				continue;

			if( vFIND.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
				ScanFolder( strFolder + _T("\\") + strName, nDepth + 1);
			else if(IsFontFile(strName))
				AddFontFile(strFolder + _T("\\") + strName);
		}
		while(::FindNextFile( hFIND, &vFIND));

		::FindClose(hFIND);
#else
		DIR *pDIR = opendir(strFolder);

		if(!pDIR)
			return;

		while( dirent *pENTRY = readdir(pDIR) )
		{
			CString strName(pENTRY->d_name);
			CString strPath = strFolder + _T("/") + strName;
			struct stat vSTAT;

			if( strName == _T(".") || strName == _T("..") || stat( strPath, &vSTAT) )
				continue;

			if(S_ISDIR(vSTAT.st_mode))
				ScanFolder( strPath, nDepth + 1);
			else if(IsFontFile(strName))
				AddFontFile(strPath);
		}

		closedir(pDIR);
#endif
	}

	// The game's own fonts (next to the executable), then the system's.
	void ScanFonts()
	{
		if(g_bScanned)
			return;
		g_bScanned = TRUE;

		CString strExe;

#ifdef _WIN32
		TCHAR szPATH[MAX_PATH] = { 0 };

		::GetModuleFileName( NULL, szPATH, MAX_PATH);
		strExe = szPATH;
		strExe = strExe.Left(strExe.ReverseFind(_T('\\')) + 1);
		ScanFolder( strExe + _T("Fonts"), 0);

		TCHAR szWINDOWS[MAX_PATH] = { 0 };
		::GetWindowsDirectory( szWINDOWS, MAX_PATH);
		ScanFolder( CString(szWINDOWS) + _T("\\Fonts"), 0);

		const char *pszLOCAL = getenv("LOCALAPPDATA");
		if(pszLOCAL)
			ScanFolder( CString(pszLOCAL) + _T("\\Microsoft\\Windows\\Fonts"), 0);
#else
		char szPATH[4096] = { 0 };
		ssize_t nLength = readlink( "/proc/self/exe", szPATH, sizeof(szPATH) - 1);

		if( nLength > 0 )
		{
			strExe = CString(szPATH, int(nLength));
			strExe = strExe.Left(strExe.ReverseFind(_T('/')) + 1);
			ScanFolder( strExe + _T("Fonts"), 0);
		}

		ScanFolder( _T("/usr/share/fonts"), 0);
		ScanFolder( _T("/usr/local/share/fonts"), 0);

		const char *pszHOME = getenv("HOME");
		if(pszHOME)
		{
			ScanFolder( CString(pszHOME) + _T("/.local/share/fonts"), 0);
			ScanFolder( CString(pszHOME) + _T("/.fonts"), 0);
		}
#endif
	}

	BOOL LoadFont( TFONTFILE *pFONT)
	{
		if(pFONT->m_bLoaded)
			return TRUE;

		if(pFONT->m_bFailed)
			return FALSE;
		pFONT->m_bFailed = TRUE;

		FILE *pFILE = TFileOpen( pFONT->m_strPath, _T("rb"));
		if(!pFILE)
			return FALSE;

		fseek( pFILE, 0, SEEK_END);
		long nSize = ftell(pFILE);
		fseek( pFILE, 0, SEEK_SET);

		if( nSize > 0 )
		{
			pFONT->m_vDATA.resize(nSize);

			if( fread( &pFONT->m_vDATA[0], 1, nSize, pFILE) != size_t(nSize) )
				pFONT->m_vDATA.clear();
		}
		fclose(pFILE);

		if(pFONT->m_vDATA.empty())
			return FALSE;

		const BYTE *pDATA = &pFONT->m_vDATA[0];
		int nOffset = stbtt_GetFontOffsetForIndex( pDATA, pFONT->m_nIndex);

		if( nOffset < 0 || !stbtt_InitFont( &pFONT->m_vINFO, pDATA, nOffset) )
		{
			pFONT->m_vDATA.clear();
			return FALSE;
		}

		DWORD dwHead = stbtt__find_table( (BYTE *) pDATA, DWORD(nOffset), "head");
		DWORD dwOS2 = stbtt__find_table( (BYTE *) pDATA, DWORD(nOffset), "OS/2");
		int nAscent = 0;
		int nDescent = 0;
		int nLineGap = 0;

		stbtt_GetFontVMetrics( &pFONT->m_vINFO, &nAscent, &nDescent, &nLineGap);

		pFONT->m_nUPEM = dwHead ? int(ReadBE( pDATA + dwHead + 18, 2)) : 2048;
		pFONT->m_nLineSpacing = nAscent - nDescent + nLineGap;

		// GDI's cell is the Windows ascent and descent of the OS/2 table.
		if( dwOS2 && size_t(dwOS2 + 78) <= pFONT->m_vDATA.size() )
		{
			pFONT->m_nWinAscent = int(ReadBE( pDATA + dwOS2 + 74, 2));
			pFONT->m_nWinDescent = int(ReadBE( pDATA + dwOS2 + 76, 2));
		}
		else
		{
			pFONT->m_nWinAscent = nAscent;
			pFONT->m_nWinDescent = -nDescent;
		}

		if( pFONT->m_nUPEM <= 0 )
			pFONT->m_nUPEM = 2048;

		pFONT->m_vGLYPH.assign( 0x10000, -1);
		pFONT->m_bLoaded = TRUE;
		pFONT->m_bFailed = FALSE;

		return TRUE;
	}

	int GetGlyph( TFONTFILE *pFONT, wchar_t wChar)
	{
		DWORD dwCode = DWORD(wChar) & 0xFFFF;
		int& nGlyph = pFONT->m_vGLYPH[dwCode];

		if( nGlyph < 0 )
			nGlyph = stbtt_FindGlyphIndex( &pFONT->m_vINFO, int(dwCode));

		return nGlyph;
	}

	// The loaded font of the family closest to the style, NULL when the family is not installed.
	TFONTFILE *FindFont( const CString& strFamily, BOOL bBold, BOOL bItalic)
	{
		std::map< CString, std::vector<TFONTFILE *> >::iterator itFAMILY = g_mapFAMILY.find(LowerName(strFamily));

		if( itFAMILY == g_mapFAMILY.end() )
			return NULL;

		std::vector<TFONTFILE *>& vFONT = itFAMILY->second;
		TFONTFILE *pBEST = NULL;
		int nBEST = -1;

		for( size_t i=0; i<vFONT.size(); i++)
		{
			int nScore = (vFONT[i]->m_bBold == bBold ? 2 : 0) + (vFONT[i]->m_bItalic == bItalic ? 1 : 0);

			if( nScore > nBEST && LoadFont(vFONT[i]) )
			{
				pBEST = vFONT[i];
				nBEST = nScore;
			}
		}

		return pBEST;
	}

	// The fonts of a LOGFONT: the face or what GDI puts in its place, then fonts for the glyphs
	// they lack.
	TFONTCHAIN& GetChain( const LOGFONT& vLOGFONT)
	{
		std::string strKEY( (const char *) &vLOGFONT, sizeof(LOGFONT));
		std::map< std::string, TFONTCHAIN>::iterator itCHAIN = g_mapCHAIN.find(strKEY);

		if( itCHAIN != g_mapCHAIN.end() )
			return itCHAIN->second;

		ScanFonts();

		static LPCTSTR vSUBSTITUTE[] = { _T("Arial"), _T("Liberation Sans"), _T("Arimo"), _T("DejaVu Sans"), NULL };
		static LPCTSTR vVERTICAL[] = { _T("Malgun Gothic"), _T("Noto Sans KR"), _T("Noto Sans CJK KR"), _T("NanumGothic"), _T("Gulim"), NULL };
		static LPCTSTR vFALLBACK[] = {
			_T("Malgun Gothic"), _T("Noto Sans KR"), _T("Noto Sans CJK KR"), _T("NanumGothic"), _T("Gulim"),
			_T("Microsoft JhengHei"), _T("Microsoft YaHei"), _T("MS Gothic"), _T("Noto Sans CJK JP"), _T("Noto Sans CJK SC"),
			_T("Segoe UI"), _T("Noto Sans"), _T("DejaVu Sans"), NULL };

		CString strFace(vLOGFONT.lfFaceName);
		BOOL bVertical = strFace.Left(1) == _T("@") ? TRUE : FALSE;
		BOOL bBold = vLOGFONT.lfWeight >= 600 ? TRUE : FALSE;
		BOOL bItalic = vLOGFONT.lfItalic ? TRUE : FALSE;
		std::vector<CString> vFAMILY;

		if(bVertical)
			strFace = strFace.Mid(1);

		if(!strFace.IsEmpty())
			vFAMILY.push_back(strFace);

		if(bVertical)
			for( int i=0; vVERTICAL[i]; i++)
				vFAMILY.push_back(vVERTICAL[i]);

		for( int i=0; vSUBSTITUTE[i]; i++)
			vFAMILY.push_back(vSUBSTITUTE[i]);

		for( int i=0; vFALLBACK[i]; i++)
			vFAMILY.push_back(vFALLBACK[i]);

		TFONTCHAIN& vCHAIN = g_mapCHAIN[strKEY];

		for( size_t i=0; i<vFAMILY.size(); i++)
		{
			TFONTFILE *pFONT = FindFont( vFAMILY[i], bBold, bItalic);

			if( pFONT && std::find( vCHAIN.m_vFILE.begin(), vCHAIN.m_vFILE.end(), pFONT) == vCHAIN.m_vFILE.end() )
				vCHAIN.m_vFILE.push_back(pFONT);
		}

		// The em: a negative height is the em, a positive one the cell (Windows ascent and descent).
		vCHAIN.m_fEM = 12.0f;

		if( vLOGFONT.lfHeight < 0 )
			vCHAIN.m_fEM = FLOAT(-vLOGFONT.lfHeight);
		else if( vLOGFONT.lfHeight > 0 && !vCHAIN.m_vFILE.empty() )
		{
			TFONTFILE *pFONT = vCHAIN.m_vFILE[0];
			vCHAIN.m_fEM = FLOAT(vLOGFONT.lfHeight) * pFONT->m_nUPEM / TMax( 1, pFONT->m_nWinAscent + pFONT->m_nWinDescent);
		}

		return vCHAIN;
	}

	struct TGLYPHPOS
	{
		TFONTFILE *m_pFONT;
		int m_nGlyph;
		FLOAT m_fAdvance;
		wchar_t m_wChar;
	};

	struct TTEXTLINE
	{
		size_t m_nBegin;
		size_t m_nEnd;
		FLOAT m_fWidth;
	};

	BOOL IsSpace( wchar_t wChar)
	{
		return wChar == L' ' || wChar == L'\t' || wChar == 0x3000 ? TRUE : FALSE;
	}

	// The width of the line's glyphs, without its trailing spaces unless asked for.
	FLOAT LineWidth( const std::vector<TGLYPHPOS>& vGLYPH, size_t nBegin, size_t nEnd, BOOL bTrailingSpaces)
	{
		if(!bTrailingSpaces)
			while( nEnd > nBegin && IsSpace(vGLYPH[nEnd - 1].m_wChar) )
				nEnd--;

		FLOAT fWidth = 0.0f;
		for( size_t i=nBegin; i<nEnd; i++)
			fWidth += vGLYPH[i].m_fAdvance;

		return fWidth;
	}

	// The glyphs of the text and its lines: at the line feeds, and at fMaxWidth (0 for no limit)
	// after the last space that fits, or before the first glyph that does not.
	void Layout( TFONTCHAIN& vCHAIN, LPCWSTR lpszText, FLOAT fMaxWidth, BOOL bTrailingSpaces, std::vector<TGLYPHPOS>& vGLYPH, std::vector<TTEXTLINE>& vLINE)
	{
		vGLYPH.clear();
		vLINE.clear();

		if(vCHAIN.m_vFILE.empty())
			return;

		for( LPCWSTR p = lpszText; *p; p++)
		{
			TGLYPHPOS vPOS = { vCHAIN.m_vFILE[0], 0, 0.0f, *p };

			if( *p != L'\n' && *p != L'\r' )
				for( size_t i=0; i<vCHAIN.m_vFILE.size(); i++)
				{
					int nGlyph = GetGlyph( vCHAIN.m_vFILE[i], *p);

					if( nGlyph > 0 )
					{
						vPOS.m_pFONT = vCHAIN.m_vFILE[i];
						vPOS.m_nGlyph = nGlyph;
						break;
					}
				}

			if( *p != L'\n' && *p != L'\r' )
			{
				int nAdvance = 0;
				int nBearing = 0;

				stbtt_GetGlyphHMetrics( &vPOS.m_pFONT->m_vINFO, vPOS.m_nGlyph, &nAdvance, &nBearing);
				vPOS.m_fAdvance = nAdvance * vCHAIN.m_fEM / vPOS.m_pFONT->m_nUPEM * TGDIPLUS_SPACING;
			}

			vGLYPH.push_back(vPOS);
		}

		FLOAT fAvailable = fMaxWidth > 0.0f ? fMaxWidth - 2.0f * TGDIPLUS_PADDING * vCHAIN.m_fEM : 0.0f;
		size_t nBegin = 0;
		size_t nBreak = 0;
		FLOAT fWidth = 0.0f;

		for( size_t i=0; i<vGLYPH.size(); i++)
		{
			wchar_t wChar = vGLYPH[i].m_wChar;

			if( wChar == L'\n' )
			{
				TTEXTLINE vTEXTLINE = { nBegin, i, LineWidth( vGLYPH, nBegin, i, bTrailingSpaces) };

				vLINE.push_back(vTEXTLINE);
				nBegin = i + 1;
				nBreak = 0;
				fWidth = 0.0f;

				continue;
			}

			if( wChar == L'\r' )
				continue;

			if( fAvailable > 0.0f && !IsSpace(wChar) && fWidth + vGLYPH[i].m_fAdvance > fAvailable + 0.01f && i > nBegin )
			{
				size_t nEnd = nBreak > nBegin ? nBreak : i;
				TTEXTLINE vTEXTLINE = { nBegin, nEnd, LineWidth( vGLYPH, nBegin, nEnd, bTrailingSpaces) };

				vLINE.push_back(vTEXTLINE);
				nBegin = nEnd;
				nBreak = 0;
				fWidth = 0.0f;

				for( size_t j=nBegin; j<i; j++)
					fWidth += vGLYPH[j].m_fAdvance;
			}

			fWidth += vGLYPH[i].m_fAdvance;

			// A line may break after the spaces.
			if( IsSpace(wChar) && (i + 1 >= vGLYPH.size() || !IsSpace(vGLYPH[i + 1].m_wChar)) )
				nBreak = i + 1;
		}

		TTEXTLINE vTEXTLINE = { nBegin, vGLYPH.size(), LineWidth( vGLYPH, nBegin, vGLYPH.size(), bTrailingSpaces) };
		vLINE.push_back(vTEXTLINE);
	}

	// GDI+'s height of n lines: the Windows ascent and descent plus 1/8 em, then a line spacing
	// per line.
	FLOAT TextHeight( TFONTCHAIN& vCHAIN, size_t nLines)
	{
		TFONTFILE *pFONT = vCHAIN.m_vFILE[0];
		FLOAT fScale = vCHAIN.m_fEM / pFONT->m_nUPEM;

		return (pFONT->m_nWinAscent + pFONT->m_nWinDescent) * fScale + vCHAIN.m_fEM / 8.0f +
			FLOAT(nLines - 1) * pFONT->m_nLineSpacing * fScale;
	}

	void MeasureFontFiles( CFont *pFont, LPCWSTR lpszText, BOOL bTrailingSpaces, FLOAT *pWidth, FLOAT *pHeight)
	{
		std::lock_guard<std::mutex> lock(g_vLOCK);
		TFONTCHAIN& vCHAIN = GetChain(pFont->GetLogFont());
		std::vector<TGLYPHPOS> vGLYPH;
		std::vector<TTEXTLINE> vLINE;

		*pWidth = 0.0f;
		*pHeight = 0.0f;

		if( !lpszText || !*lpszText || vCHAIN.m_vFILE.empty() )
			return;

		Layout( vCHAIN, lpszText, 0.0f, bTrailingSpaces, vGLYPH, vLINE);

		for( size_t i=0; i<vLINE.size(); i++)
			*pWidth = TMax( *pWidth, vLINE[i].m_fWidth);

		*pWidth += 2.0f * TGDIPLUS_PADDING * vCHAIN.m_fEM;
		*pHeight = TextHeight( vCHAIN, vLINE.size());
	}

	void DrawFontFiles( CFont *pFont, LPCWSTR lpszText, FLOAT fLeft, FLOAT fTop, FLOAT fWidth, FLOAT fHeight, UINT nFormat, int nWidth, int nHeight, LPBYTE pCoverage)
	{
		std::lock_guard<std::mutex> lock(g_vLOCK);
		TFONTCHAIN& vCHAIN = GetChain(pFont->GetLogFont());
		std::vector<TGLYPHPOS> vGLYPH;
		std::vector<TTEXTLINE> vLINE;

		memset( pCoverage, 0, size_t(nWidth) * nHeight);
		if( !lpszText || !*lpszText || vCHAIN.m_vFILE.empty() )
			return;

		Layout( vCHAIN, lpszText, fWidth, FALSE, vGLYPH, vLINE);

		TFONTFILE *pMAIN = vCHAIN.m_vFILE[0];
		FLOAT fEM = vCHAIN.m_fEM;
		FLOAT fLineSpacing = pMAIN->m_nLineSpacing * fEM / pMAIN->m_nUPEM;
		FLOAT fBlockTop = fTop;

		if( nFormat & DT_VCENTER )
			fBlockTop = fTop + (fHeight - TextHeight( vCHAIN, vLINE.size())) / 2.0f;

		// The layout rectangle clips, as in GDI+.
		int nClipLeft = TMax( 0, int(floorf(fLeft)));
		int nClipTop = TMax( 0, int(floorf(fTop)));
		int nClipRight = TMin( nWidth, int(ceilf(fLeft + fWidth)));
		int nClipBottom = TMin( nHeight, int(ceilf(fTop + fHeight)));
		std::vector<BYTE> vBITMAP;

		for( size_t nLine=0; nLine<vLINE.size(); nLine++)
		{
			TTEXTLINE& vTEXTLINE = vLINE[nLine];
			FLOAT fBaseline = fBlockTop + nLine * fLineSpacing + pMAIN->m_nWinAscent * fEM / pMAIN->m_nUPEM;
			FLOAT fX = fLeft + TGDIPLUS_PADDING * fEM;

			if( nFormat & DT_CENTER )
				fX = fLeft + (fWidth - vTEXTLINE.m_fWidth) / 2.0f;
			else if( nFormat & DT_RIGHT )
				fX = fLeft + fWidth - TGDIPLUS_PADDING * fEM - vTEXTLINE.m_fWidth;

			for( size_t i=vTEXTLINE.m_nBegin; i<vTEXTLINE.m_nEnd; i++)
			{
				TGLYPHPOS& vPOS = vGLYPH[i];

				if( vPOS.m_wChar == L'\r' )
					continue;

				FLOAT fScale = fEM / vPOS.m_pFONT->m_nUPEM;
				FLOAT fPixelX = floorf(fX);
				FLOAT fPixelY = floorf(fBaseline);
				FLOAT fShiftX = fX - fPixelX;
				FLOAT fShiftY = fBaseline - fPixelY;
				int nX0 = 0;
				int nY0 = 0;
				int nX1 = 0;
				int nY1 = 0;

				stbtt_GetGlyphBitmapBoxSubpixel( &vPOS.m_pFONT->m_vINFO, vPOS.m_nGlyph, fScale, fScale, fShiftX, fShiftY, &nX0, &nY0, &nX1, &nY1);

				int nGlyphW = nX1 - nX0;
				int nGlyphH = nY1 - nY0;

				if( nGlyphW > 0 && nGlyphH > 0 )
				{
					vBITMAP.assign( size_t(nGlyphW) * nGlyphH, 0);
					stbtt_MakeGlyphBitmapSubpixel( &vPOS.m_pFONT->m_vINFO, &vBITMAP[0], nGlyphW, nGlyphH, nGlyphW, fScale, fScale, fShiftX, fShiftY, vPOS.m_nGlyph);

					for( int y=0; y<nGlyphH; y++)
					{
						int nPosY = int(fPixelY) + nY0 + y;

						if( nPosY < nClipTop || nPosY >= nClipBottom )
							continue;

						for( int x=0; x<nGlyphW; x++)
						{
							int nPosX = int(fPixelX) + nX0 + x;

							if( nPosX < nClipLeft || nPosX >= nClipRight )
								continue;

							BYTE& bPIXEL = pCoverage[nPosY * nWidth + nPosX];
							bPIXEL = BYTE(TMin( 255, int(bPIXEL) + vBITMAP[y * nGlyphW + x]));
						}
					}
				}

				fX += vPOS.m_fAdvance;
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////
// CTTextRaster

void CTTextRaster::SetFontFiles( BOOL bFontFiles)
{
	g_bFontFiles = bFontFiles;
}

BOOL CTTextRaster::UsesFontFiles()
{
#ifdef _WIN32
	return g_bFontFiles;
#else
	return TRUE;
#endif
}

void CTTextRaster::Measure( CFont *pFont, LPCWSTR lpszText, BOOL bTrailingSpaces, int nRenderingHint, FLOAT *pWidth, FLOAT *pHeight)
{
	*pWidth = 0.0f;
	*pHeight = 0.0f;

	if(!pFont)
		return;

#ifdef _WIN32
	if(!UsesFontFiles())
	{
		MeasureGDIPlus( pFont, lpszText, bTrailingSpaces, nRenderingHint, pWidth, pHeight);
		return;
	}
#endif

	MeasureFontFiles( pFont, lpszText, bTrailingSpaces, pWidth, pHeight);
}

void CTTextRaster::Draw( CFont *pFont, LPCWSTR lpszText, int nRenderingHint, FLOAT fLeft, FLOAT fTop, FLOAT fWidth, FLOAT fHeight, UINT nFormat, int nWidth, int nHeight, LPBYTE pCoverage)
{
	if( !pCoverage || nWidth <= 0 || nHeight <= 0 )
		return;

	if(!pFont)
	{
		memset( pCoverage, 0, size_t(nWidth) * nHeight);
		return;
	}

#ifdef _WIN32
	if(!UsesFontFiles())
	{
		memset( pCoverage, 0, size_t(nWidth) * nHeight);
		DrawGDIPlus( pFont, lpszText, nRenderingHint, fLeft, fTop, fWidth, fHeight, nFormat, nWidth, nHeight, pCoverage);
		return;
	}
#endif

	DrawFontFiles( pFont, lpszText, fLeft, fTop, fWidth, fHeight, nFormat, nWidth, nHeight, pCoverage);
}
