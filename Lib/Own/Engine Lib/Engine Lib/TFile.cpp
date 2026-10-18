#include "StdAfx.h"
#include <map>
#include <mutex>

#ifdef _WIN32
#include <share.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif


//////////////////////////////////////////////////////////////////////
// Folder listings

namespace
{
	// A folder's names by their lowercase form, and its last write time when it was listed.
	struct TFOLDER
	{
		INT64 m_nTime;
		std::map< CString, CString> m_mapNAME;
	};

	std::mutex g_vLOCK;
	std::map< CString, TFOLDER> g_mapFOLDER;

	// On where names are case sensitive; on Windows when TPATH_RESOLVE=1 (to test it there).
	BOOL InitResolve()
	{
#ifdef _WIN32
		const char *pszValue = getenv("TPATH_RESOLVE");
		return pszValue && pszValue[0] == '1' ? TRUE : FALSE;
#else
		return TRUE;
#endif
	}

	BOOL g_bResolve = InitResolve();

	// ASCII only: a DBCS trail byte is left alone.
	CString LowerName( const CString& strName)
	{
		CString strLower(strName);

		for( int i=0; i<strLower.GetLength(); i++)
			if( strLower[i] >= 'A' && strLower[i] <= 'Z' )
				strLower.SetAt( i, TCHAR(strLower[i] - 'A' + 'a'));

		return strLower;
	}

	// The folder's last write time, -1 when it is no folder.
	INT64 FolderTime( const CString& strFolder)
	{
#ifdef _WIN32
		WIN32_FILE_ATTRIBUTE_DATA vDATA;

		if( !::GetFileAttributesEx( strFolder, GetFileExInfoStandard, &vDATA) || !(vDATA.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
			return -1;

		return (INT64(vDATA.ftLastWriteTime.dwHighDateTime) << 32) | vDATA.ftLastWriteTime.dwLowDateTime;
#else
		struct stat vSTAT;

		if( stat( strFolder, &vSTAT) || !S_ISDIR(vSTAT.st_mode) )
			return -1;

		return INT64(vSTAT.st_mtim.tv_sec) * 1000000000 + vSTAT.st_mtim.tv_nsec;
#endif
	}

	void ListFolder( const CString& strFolder, std::map< CString, CString>& mapNAME)
	{
#ifdef _WIN32
		WIN32_FIND_DATA vFIND;
		HANDLE hFIND = ::FindFirstFile( strFolder + _T("/*"), &vFIND);

		if( hFIND == INVALID_HANDLE_VALUE )
			return;

		do
			mapNAME[LowerName(vFIND.cFileName)] = vFIND.cFileName;
		while(::FindNextFile( hFIND, &vFIND));

		::FindClose(hFIND);
#else
		DIR *pDIR = opendir(strFolder);

		if(!pDIR)
			return;

		while( dirent *pENTRY = readdir(pDIR) )
			mapNAME[LowerName(pENTRY->d_name)] = pENTRY->d_name;

		closedir(pDIR);
#endif
	}

	// The name on disk for a lowercase name in the folder; FALSE when there is none. A miss
	// lists the folder again if it changed since. Called with g_vLOCK held.
	BOOL FindName( const CString& strFolder, const CString& strLower, CString& strName)
	{
		std::map< CString, TFOLDER>::iterator itFOLDER = g_mapFOLDER.find(strFolder);
		INT64 nTime = 0;

		if( itFOLDER != g_mapFOLDER.end() )
		{
			std::map< CString, CString>::iterator itNAME = itFOLDER->second.m_mapNAME.find(strLower);

			if( itNAME != itFOLDER->second.m_mapNAME.end() )
			{
				strName = itNAME->second;
				return TRUE;
			}

			nTime = FolderTime(strFolder);
			if( nTime == itFOLDER->second.m_nTime )
				return FALSE;
		}
		else
			nTime = FolderTime(strFolder);

		TFOLDER& vFOLDER = g_mapFOLDER[strFolder];

		vFOLDER.m_nTime = nTime;
		vFOLDER.m_mapNAME.clear();

		if( nTime != -1 )
			ListFolder( strFolder, vFOLDER.m_mapNAME);

		std::map< CString, CString>::iterator itNAME = vFOLDER.m_mapNAME.find(strLower);
		if( itNAME == vFOLDER.m_mapNAME.end() )
			return FALSE;

		strName = itNAME->second;
		return TRUE;
	}
}


//////////////////////////////////////////////////////////////////////
// CTPath

CString CTPath::Resolve( LPCTSTR lpszPath)
{
	if( !g_bResolve || !lpszPath )
		return CString(lpszPath);

	CString strPath(lpszPath);
	CString strResult;
	int nLength = strPath.GetLength();
	int nPos = 0;

	strPath.Replace( _T('\\'), _T('/'));

	// The root stays as it is: "/" or a drive ("C:/").
	if( nLength && strPath[0] == _T('/') )
		nPos = 1;
	else if( nLength >= 3 && strPath[1] == _T(':') && strPath[2] == _T('/') )
		nPos = 3;
	strResult = strPath.Left(nPos);

	std::lock_guard<std::mutex> lock(g_vLOCK);
	BOOL bFound = TRUE;

	while( nPos <= nLength )
	{
		int nEnd = strPath.Find( _T('/'), nPos);

		if( nEnd < 0 )
			nEnd = nLength;
		CString strName = strPath.Mid( nPos, nEnd - nPos);

		// Past a name that is not on disk, the rest is kept as given.
		if( bFound && !strName.IsEmpty() && strName != _T(".") && strName != _T("..") )
		{
			CString strFolder = strResult;

			if(strFolder.IsEmpty())
				strFolder = _T(".");
			else if( strFolder.GetLength() > 1 && strFolder.Right(1) == _T("/") && strFolder.Right(2) != _T(":/") )
				strFolder.Truncate(strFolder.GetLength() - 1);

			bFound = FindName( strFolder, LowerName(strName), strName);
		}

		strResult += strName;
		if( nEnd < nLength )
			strResult += _T('/');

		nPos = nEnd + 1;
	}

	return strResult;
}

void CTPath::EnableResolve( BOOL bEnable)
{
	g_bResolve = bEnable;
}

BOOL CTPath::IsResolveEnabled()
{
	return g_bResolve;
}

void CTPath::ClearCache()
{
	std::lock_guard<std::mutex> lock(g_vLOCK);
	g_mapFOLDER.clear();
}


//////////////////////////////////////////////////////////////////////
// Files

FILE *TFileOpen( LPCTSTR lpszPath, LPCTSTR lpszMode)
{
#ifdef _WIN32
	return _tfsopen( CTPath::Resolve(lpszPath), lpszMode, _SH_DENYNO);
#else
	// No text mode here: the files are read as they are.
	CString strMode(lpszMode);

	strMode.Remove(_T('t'));
	return fopen( CTPath::Resolve(lpszPath), strMode);
#endif
}

BOOL TFileExists( LPCTSTR lpszPath)
{
#ifdef _WIN32
	return ::GetFileAttributes(CTPath::Resolve(lpszPath)) != INVALID_FILE_ATTRIBUTES ? TRUE : FALSE;
#else
	return access( CTPath::Resolve(lpszPath), F_OK) ? FALSE : TRUE;
#endif
}

BOOL TFileDelete( LPCTSTR lpszPath)
{
#ifdef _WIN32
	return ::DeleteFile(CTPath::Resolve(lpszPath));
#else
	return unlink(CTPath::Resolve(lpszPath)) ? FALSE : TRUE;
#endif
}

BOOL TFileRename( LPCTSTR lpszOldPath, LPCTSTR lpszNewPath)
{
#ifdef _WIN32
	return ::MoveFile( CTPath::Resolve(lpszOldPath), CTPath::Resolve(lpszNewPath));
#else
	return rename( CTPath::Resolve(lpszOldPath), CTPath::Resolve(lpszNewPath)) ? FALSE : TRUE;
#endif
}

BOOL TCreateFolder( LPCTSTR lpszPath)
{
	CString strPath = CTPath::Resolve(lpszPath);

#ifdef _WIN32
	::CreateDirectory( strPath, NULL);
#else
	mkdir( strPath, 0755);
#endif

	return TFileExists(strPath);
}
