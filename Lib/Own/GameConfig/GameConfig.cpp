// GameConfig.cpp : implementation of the CGameConfig class.
//
// Compiled with precompiled headers DISABLED (see the .vcxproj entries) so it can be shared
// verbatim by TClient and TLauncher, which have different stdafx.h files.
//
// The ini file is read and written here, not with GetPrivateProfileString, so the client is the
// same where there is no such API.
//
//////////////////////////////////////////////////////////////////////

// The client has no MFC (GameConfig.h then takes CString from TAfx.h); the launcher does.
#ifndef TCLIENT_NO_MFC
#include <afx.h>
#endif
#include <tchar.h>
#include <ctype.h>
#include <stdio.h>
#include <vector>
#include "GameConfig.h"

//////////////////////////////////////////////////////////////////////
// The ini file, read and written like GetPrivateProfileString / WritePrivateProfileString do:
// sections and keys are found case-insensitively and trimmed, the first of each name counts,
// values are trimmed and lose a pair of enclosing quotes. The file is small and read again on
// each access, so an edit made while the game runs is seen.
//////////////////////////////////////////////////////////////////////

static BOOL IsIniSpace( TCHAR ch)
{
	return ch == _T(' ') || ch == _T('\t') ? TRUE : FALSE;
}

static CString TrimIni( const CString& strText)
{
	int nStart = 0;
	int nEnd = strText.GetLength();

	while( nStart < nEnd && IsIniSpace(strText[nStart]) )
		nStart++;

	while( nEnd > nStart && IsIniSpace(strText[nEnd - 1]) )
		nEnd--;

	return strText.Mid( nStart, nEnd - nStart);
}

// The file's lines without their ends; FALSE when there is no file.
static BOOL ReadIniLines( LPCTSTR lpszPath, std::vector<CString>& vLINE)
{
	FILE *pFILE = fopen( lpszPath, "rb");

	vLINE.clear();
	if(!pFILE)
		return FALSE;

	CString strLINE;
	int nChar;

	while( (nChar = fgetc(pFILE)) != EOF )
		if( nChar == '\n' )
		{
			if( !strLINE.IsEmpty() && strLINE.Right(1) == _T("\r") )
				strLINE.Truncate(strLINE.GetLength() - 1);

			vLINE.push_back(strLINE);
			strLINE.Empty();
		}
		else
			strLINE += TCHAR(nChar);

	if(!strLINE.IsEmpty())
		vLINE.push_back(strLINE);

	fclose(pFILE);
	return TRUE;
}

// A section line's name; FALSE for other lines.
static BOOL GetIniSection( const CString& strLINE, CString& strName)
{
	CString strText = TrimIni(strLINE);

	if( strText.IsEmpty() || strText[0] != _T('[') )
		return FALSE;

	int nEnd = strText.Find(_T(']'));
	strName = TrimIni(strText.Mid( 1, nEnd < 0 ? strText.GetLength() - 1 : nEnd - 1));

	return TRUE;
}

// A key line's key and value; FALSE for other lines.
static BOOL GetIniKey( const CString& strLINE, CString& strKey, CString& strValue)
{
	int nPos = strLINE.Find(_T('='));

	if( nPos < 0 )
		return FALSE;

	strKey = TrimIni(strLINE.Left(nPos));
	strValue = TrimIni(strLINE.Mid(nPos + 1));

	return TRUE;
}

// Where the section's lines are: [nBegin, nEnd) after its header line; FALSE when it is not there.
static BOOL FindIniSection( const std::vector<CString>& vLINE, LPCTSTR lpszSection, int& nBegin, int& nEnd)
{
	CString strName;

	for( int i=0; i<int(vLINE.size()); i++)
		if( GetIniSection( vLINE[i], strName) && !strName.CompareNoCase(lpszSection) )
		{
			nBegin = i + 1;
			for( nEnd = nBegin; nEnd < int(vLINE.size()) && !GetIniSection( vLINE[nEnd], strName); nEnd++);

			return TRUE;
		}

	return FALSE;
}

// The key's line in the section; -1 when it is not there.
static int FindIniKey( const std::vector<CString>& vLINE, int nBegin, int nEnd, LPCTSTR lpszKey, CString *pValue)
{
	CString strKey;
	CString strValue;

	for( int i=nBegin; i<nEnd; i++)
		if( GetIniKey( vLINE[i], strKey, strValue) && !strKey.CompareNoCase(lpszKey) )
		{
			if(pValue)
				*pValue = strValue;

			return i;
		}

	return -1;
}

static BOOL ReadIniString( LPCTSTR lpszPath, LPCTSTR lpszSection, LPCTSTR lpszKey, CString& strValue)
{
	std::vector<CString> vLINE;
	int nBegin = 0;
	int nEnd = 0;

	if( !ReadIniLines( lpszPath, vLINE) ||
		!FindIniSection( vLINE, TrimIni(lpszSection), nBegin, nEnd) ||
		FindIniKey( vLINE, nBegin, nEnd, TrimIni(lpszKey), &strValue) < 0 )
		return FALSE;

	int nLength = strValue.GetLength();
	if( nLength >= 2 && (strValue[0] == _T('"') || strValue[0] == _T('\'')) && strValue[nLength - 1] == strValue[0] )
		strValue = strValue.Mid( 1, nLength - 2);

	return TRUE;
}

// GetPrivateProfileInt's number: decimal or 0x hexadecimal, with a sign, up to the first other
// character; 0 without digits, the default for an empty value.
static int ParseIniInt( const CString& strValue, int nDefault)
{
	LPCTSTR p = strValue;

	if(strValue.IsEmpty())
		return nDefault;
	BOOL bNegative = FALSE;
	UINT nValue = 0;

	if( *p == _T('-') )
	{
		bNegative = TRUE;
		p++;
	}

	if( p[0] == _T('0') && (p[1] == _T('x') || p[1] == _T('X')) )
	{
		for( p += 2; isxdigit(BYTE(*p)); p++)
			nValue = nValue * 16 + UINT(isdigit(BYTE(*p)) ? *p - _T('0') : (*p | 0x20) - _T('a') + 10);
	}
	else
		for( ; isdigit(BYTE(*p)); p++)
			nValue = nValue * 10 + UINT(*p - _T('0'));

	return int(bNegative ? 0 - nValue : nValue);
}

static void WriteIniString( LPCTSTR lpszPath, LPCTSTR lpszSection, LPCTSTR lpszKey, LPCTSTR lpszValue)
{
	std::vector<CString> vLINE;
	CString strLINE;
	int nBegin = 0;
	int nEnd = 0;

	CString strSECTION = TrimIni(lpszSection);
	CString strKEY = TrimIni(lpszKey);

	ReadIniLines( lpszPath, vLINE);
	strLINE.Format( _T("%s=%s"), (LPCTSTR) strKEY, lpszValue);

	if(!FindIniSection( vLINE, strSECTION, nBegin, nEnd))
	{
		CString strSection;

		strSection.Format( _T("[%s]"), (LPCTSTR) strSECTION);
		vLINE.push_back(strSection);
		vLINE.push_back(strLINE);
	}
	else
	{
		int nLine = FindIniKey( vLINE, nBegin, nEnd, strKEY, NULL);

		if( nLine >= 0 )
		{
			// Only the value changes: the key as written, the '=' and the spaces after the
			// value stay.
			CString& strOLD = vLINE[nLine];
			int nPos = strOLD.Find(_T('='));
			int nEndValue = strOLD.GetLength();

			while( nEndValue > nPos + 1 && IsIniSpace(strOLD[nEndValue - 1]) )
				nEndValue--;

			strOLD = strOLD.Left(nPos + 1) + lpszValue + strOLD.Mid(nEndValue);
		}
		else
		{
			// After the section's last line that is not blank.
			while( nEnd > nBegin && TrimIni(vLINE[nEnd - 1]).IsEmpty() )
				nEnd--;

			vLINE.insert( vLINE.begin() + nEnd, strLINE);
		}
	}

	FILE *pFILE = fopen( lpszPath, "wb");

	if(!pFILE)
		return;

	for( size_t i=0; i<vLINE.size(); i++)
	{
		fputs( vLINE[i], pFILE);
		fputs( "\r\n", pFILE);
	}

	fclose(pFILE);
}

//////////////////////////////////////////////////////////////////////
// Central defaults table -- the single source of truth for every fixed setting. Used by the
// 2-argument getters (and therefore by the launcher's settings dialogs and any fresh install).
// Note: .ini section/key names are case-insensitive, so the client's "TextureDetail" and the
// launcher's "TextureDETAIL" resolve to the same entry -- that is intentional (the launcher's
// dialogs are meant to configure the client).
//////////////////////////////////////////////////////////////////////

struct TConfigDefaultInt  { LPCTSTR lpszSection; LPCTSTR lpszKey; int     nDefault; };
struct TConfigDefaultStr  { LPCTSTR lpszSection; LPCTSTR lpszKey; LPCTSTR lpszDefault; };

static const TConfigDefaultInt s_DefaultInts[] =
{
	// [Settings] -- display
	{ _T("Settings"), _T("ScreenX"),               1024 },
	{ _T("Settings"), _T("ScreenY"),               768  },
	{ _T("Settings"), _T("SwapEffect"),            0    },
	{ _T("Settings"), _T("PresentationInterval"),  0    },
	{ _T("Settings"), _T("Behavior"),              0    },
	{ _T("Settings"), _T("TextureDetail"),         1    },
	// [Settings] -- graphics quality
	{ _T("Settings"), _T("MapDETAIL"),             1    },
	{ _T("Settings"), _T("ObjDETAIL"),             1    },
	{ _T("Settings"), _T("MapSHADOW"),             1    },
	{ _T("Settings"), _T("ObjSHADOW"),             1    },
	{ _T("Settings"), _T("MapSFX"),                1    },
	{ _T("Settings"), _T("SfxDETAIL"),             1    },
	{ _T("Settings"), _T("DLIGHTMAP"),             1    },
	{ _T("Settings"), _T("FLIGHTMAP"),             1    },
	{ _T("Settings"), _T("FarIMAGE"),              1    },
	// [Settings] -- audio
	{ _T("Settings"), _T("MASTER"),                1    },
	{ _T("Settings"), _T("MainVolume"),            100  },
	{ _T("Settings"), _T("BGM"),                   1    },
	{ _T("Settings"), _T("BGMVolume"),             100  },
	{ _T("Settings"), _T("SOUND"),                 1    },
	{ _T("Settings"), _T("SFXVolume"),             100  },
	// [Settings] -- gameplay / UI toggles
	{ _T("Settings"), _T("NpcNAME"),               1    },
	{ _T("Settings"), _T("MonNAME"),               1    },
	{ _T("Settings"), _T("PcNAME"),                1    },
	{ _T("Settings"), _T("AUTOHELP"),              1    },
	{ _T("Settings"), _T("TALKBOX"),               1    },
	{ _T("Settings"), _T("HUD"),                   1    },
	{ _T("Settings"), _T("DENYWHI"),               0    },
	{ _T("Settings"), _T("DENYCOM"),               0    },
	{ _T("Settings"), _T("CONCHAT"),               0    },
	{ _T("Settings"), _T("MOUSECLICKMOVE"),        0    },
	{ _T("Settings"), _T("AUTOTARGETING"),         1    },
	{ _T("Settings"), _T("FontSize"),              100  },
	{ _T("Settings"), _T("GroupMonNames"),         1    },
	{ _T("Settings"), _T("FIRST"),                 1    },

	// [Launcher] -- patch config + flags
	{ _T("Launcher"), _T("version"),               0    },
	{ _T("Launcher"), _T("port"),                  0    },
	{ _T("Launcher"), _T("PrePatchFirst"),         0    },
	{ _T("Launcher"), _T("PrePatchAutoStart"),     0    },
};

static const TConfigDefaultStr s_DefaultStrs[] =
{
	// [Settings]
	{ _T("Settings"), _T("WindowedMode"),  _T("FALSE") },
	{ _T("Settings"), _T("UseShader"),     _T("TRUE")  },
	{ _T("Settings"), _T("Renderer"),      _T("BGFX")  },  // BGFX or D3D9
	{ _T("Settings"), _T("BGFXRenderer"),  _T("Auto")  },  // Auto, D3D11, D3D12, Vulkan or OpenGL
	{ _T("Settings"), _T("OBJRange"),      _T("1.0")   },
	{ _T("Settings"), _T("GroupMonNamesNear"),  _T("20.0") },
	{ _T("Settings"), _T("GroupMonNamesRatio"), _T("0.5")  },
	{ _T("Settings"), _T("Font"),          _T("")      },
	{ _T("Settings"), _T("FontQuality"),   _T("ANTIALIASED_QUALITY") },
	{ _T("Settings"), _T("TextRenderer"),  _T("GDI+")  },  // GDI+ or FontFiles (the Linux rasterizer)

	// [Launcher]
	{ _T("Launcher"), _T("directory"),     _T("")            },  // resolved to exe folder by the launcher
	{ _T("Launcher"), _T("exe"),           _T("TClient.exe") },
	{ _T("Launcher"), _T("address"),       _T("127.0.0.1")   },
	{ _T("Launcher"), _T("disclaimer"),    _T("FALSE")       },
};

static int LookupDefaultInt(LPCTSTR lpszSection, LPCTSTR lpszKey)
{
	for (int i = 0; i < sizeof(s_DefaultInts) / sizeof(s_DefaultInts[0]); ++i)
	{
		if (_tcsicmp(s_DefaultInts[i].lpszSection, lpszSection) == 0 &&
			_tcsicmp(s_DefaultInts[i].lpszKey,     lpszKey)     == 0)
			return s_DefaultInts[i].nDefault;
	}
	return 0;
}

static LPCTSTR LookupDefaultStr(LPCTSTR lpszSection, LPCTSTR lpszKey)
{
	for (int i = 0; i < sizeof(s_DefaultStrs) / sizeof(s_DefaultStrs[0]); ++i)
	{
		if (_tcsicmp(s_DefaultStrs[i].lpszSection, lpszSection) == 0 &&
			_tcsicmp(s_DefaultStrs[i].lpszKey,     lpszKey)     == 0)
			return s_DefaultStrs[i].lpszDefault;
	}
	return _T("");
}

//////////////////////////////////////////////////////////////////////
// CGameConfig
//////////////////////////////////////////////////////////////////////

CGameConfig& CGameConfig::Instance()
{
	static CGameConfig s_instance;
	return s_instance;
}

CGameConfig::CGameConfig()
{
}

void CGameConfig::Init()
{
	TCHAR szPath[MAX_PATH] = { 0 };
	::GetModuleFileName(NULL, szPath, MAX_PATH);

	// The exe's folder: '\\' on Windows, '/' elsewhere.
	LPTSTR pSlash = _tcsrchr(szPath, _T('\\'));
	LPTSTR pAltSlash = _tcsrchr(szPath, _T('/'));
	if (pAltSlash > pSlash)
		pSlash = pAltSlash;
	if (pSlash)
		*(pSlash + 1) = _T('\0');

	m_strPath = szPath;
	m_strPath += _T("config.ini");
}

void CGameConfig::EnsureInit() const
{
	if (m_strPath.IsEmpty())
		const_cast<CGameConfig*>(this)->Init();
}

int CGameConfig::GetInt(LPCTSTR lpszSection, LPCTSTR lpszKey) const
{
	return GetInt(lpszSection, lpszKey, LookupDefaultInt(lpszSection, lpszKey));
}

int CGameConfig::GetInt(LPCTSTR lpszSection, LPCTSTR lpszKey, int nDefault) const
{
	EnsureInit();
	CString strValue;
	return ReadIniString(m_strPath, lpszSection, lpszKey, strValue) ? ParseIniInt(strValue, nDefault) : nDefault;
}

CString CGameConfig::GetString(LPCTSTR lpszSection, LPCTSTR lpszKey) const
{
	return GetString(lpszSection, lpszKey, LookupDefaultStr(lpszSection, lpszKey));
}

CString CGameConfig::GetString(LPCTSTR lpszSection, LPCTSTR lpszKey, LPCTSTR lpszDefault) const
{
	EnsureInit();
	CString strValue;
	return ReadIniString(m_strPath, lpszSection, lpszKey, strValue) ? strValue : CString(lpszDefault ? lpszDefault : _T(""));
}

void CGameConfig::SetInt(LPCTSTR lpszSection, LPCTSTR lpszKey, int nValue)
{
	EnsureInit();
	CString strValue;
	strValue.Format(_T("%d"), nValue);
	WriteIniString(m_strPath, lpszSection, lpszKey, strValue);
}

void CGameConfig::SetString(LPCTSTR lpszSection, LPCTSTR lpszKey, LPCTSTR lpszValue)
{
	EnsureInit();
	WriteIniString(m_strPath, lpszSection, lpszKey, lpszValue);
}
