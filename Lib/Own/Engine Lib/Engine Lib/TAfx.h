// TAfx.h: the part of MFC's core the client uses, without MFC.
//
// File names go through CTPath::Resolve (TFile.h), so the game's paths work where names are
// case sensitive.
//
// CString, CPoint/CRect/CSize and CTime/CTimeSpan are the engine's own (TString.h, TTypes.h,
// TTime.h), with ATL's API and behavior. The rest is written here with MFC's names and behavior:
// CFile throws CFileException* on errors, CArchive reads and writes MFC's binary format
// (CString length prefixes included) and throws CArchiveException* past the end, and
// TRY/CATCH delete the exception at the end of the handler like MFC's macros.
//
// Included only where MFC is not: a project built with MFC gets MFC's own classes.
//////////////////////////////////////////////////////////////////////

#if !defined __TAFX_H
#define __TAFX_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef _AFX

#include <winsock2.h>
#include <windows.h>
#include <tchar.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <imm.h>
#include <commctrl.h>
#include <ole2.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

#include <assert.h>
#include <vector>

#include "TString.h"
#include "TTypes.h"
#include "TTime.h"
#include "TFile.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")


#ifndef AFX_IDS_APP_TITLE
#define AFX_IDS_APP_TITLE			0xE000
#endif

// TRACE writes to the debugger in debug builds, ASSERT checks in debug builds only.
#ifdef _DEBUG
inline void TAfxTrace( LPCTSTR pszFormat, ...)
{
	CString strText;
	va_list args;

	va_start( args, pszFormat);
	strText.FormatV( pszFormat, args);
	va_end(args);

	::OutputDebugString(strText);
}

#define TRACE						TAfxTrace
#define ASSERT(f)					assert(f)
#define VERIFY(f)					assert(f)
#else
#define TRACE						__noop
#define ASSERT(f)					((void) 0)
#define VERIFY(f)					((void) (f))
#endif


//////////////////////////////////////////////////////////////////////
// CObject

class CObject
{
public:
	virtual ~CObject() {}

protected:
	CObject() {}
};


//////////////////////////////////////////////////////////////////////
// Exceptions

class CException : public CObject
{
public:
	CException() { m_bAutoDelete = TRUE; }

	void Delete()
	{
		if(m_bAutoDelete)
			delete this;
	}

	virtual BOOL GetErrorMessage( LPTSTR lpszError, UINT nMaxError, PUINT pnHelpContext = NULL) const
	{
		if( lpszError && nMaxError )
			lpszError[0] = 0;

		return FALSE;
	}

public:
	BOOL m_bAutoDelete;
};

class CMemoryException : public CException
{
};

class CNotSupportedException : public CException
{
};

class CFileException : public CException
{
public:
	enum
	{
		none,
		genericException,
		fileNotFound,
		badPath,
		tooManyOpenFiles,
		accessDenied,
		invalidFile,
		removeCurrentDir,
		directoryFull,
		badSeek,
		hardIO,
		sharingViolation,
		lockViolation,
		diskFull,
		endOfFile
	};

	CFileException( int cause = none, LONG lOsError = -1, LPCTSTR lpszFileName = NULL)
	{
		m_cause = cause;
		m_lOsError = lOsError;
		m_strFileName = lpszFileName;
	}

	static int OsErrorToException( LONG lOsError)
	{
		switch(lOsError)
		{
		case NO_ERROR					: return none;
		case ERROR_FILE_NOT_FOUND		: return fileNotFound;
		case ERROR_PATH_NOT_FOUND		:
		case ERROR_INVALID_NAME			:
		case ERROR_BAD_PATHNAME			:
		case ERROR_INVALID_DRIVE		: return badPath;
		case ERROR_TOO_MANY_OPEN_FILES	: return tooManyOpenFiles;
		case ERROR_ACCESS_DENIED		:
		case ERROR_WRITE_PROTECT		: return accessDenied;
		case ERROR_INVALID_HANDLE		: return invalidFile;
		case ERROR_CURRENT_DIRECTORY	: return removeCurrentDir;
		case ERROR_CANNOT_MAKE			: return directoryFull;
		case ERROR_SEEK					:
		case ERROR_NEGATIVE_SEEK		: return badSeek;
		case ERROR_SHARING_VIOLATION	: return sharingViolation;
		case ERROR_LOCK_VIOLATION		: return lockViolation;
		case ERROR_HANDLE_DISK_FULL		:
		case ERROR_DISK_FULL			: return diskFull;
		case ERROR_HANDLE_EOF			: return endOfFile;
		}

		return genericException;
	}

public:
	CString m_strFileName;
	LONG m_lOsError;
	int m_cause;
};

class CArchiveException : public CException
{
public:
	enum
	{
		none,
		genericException,
		readOnly,
		endOfFile,
		writeOnly,
		badIndex,
		badClass,
		badSchema
	};

	CArchiveException( int cause = none, LPCTSTR lpszArchiveName = NULL)
	{
		m_cause = cause;
		m_strFileName = lpszArchiveName;
	}

public:
	CString m_strFileName;
	int m_cause;
};

inline void AfxThrowMemoryException()
{
	throw new CMemoryException();
}

inline void AfxThrowNotSupportedException()
{
	throw new CNotSupportedException();
}

inline void AfxThrowFileException( int cause, LONG lOsError = -1, LPCTSTR lpszFileName = NULL)
{
	throw new CFileException( cause, lOsError, lpszFileName);
}

inline void AfxThrowArchiveException( int cause, LPCTSTR lpszArchiveName = NULL)
{
	throw new CArchiveException( cause, lpszArchiveName);
}

// Deletes the caught exception when the handler ends, by any path (MFC's macros did too).
struct TAFX_EXCEPTION_LINK
{
	CException *m_pException;
	~TAFX_EXCEPTION_LINK() { if(m_pException) m_pException->Delete(); }
};

#define TRY							try {
#define CATCH(class, e)				} catch( class *e) { TAFX_EXCEPTION_LINK _vLINK_##e = { e };
#define AND_CATCH(class, e)			CATCH( class, e)
#define END_CATCH					}
#define CATCH_ALL(e)				CATCH( CException, e)
#define AND_CATCH_ALL(e)			CATCH( CException, e)
#define END_CATCH_ALL				}
#define THROW(e)					throw e


//////////////////////////////////////////////////////////////////////
// CFile

class CFile : public CObject
{
public:
	enum OpenFlags
	{
		modeRead = 0x00000,
		modeWrite = 0x00001,
		modeReadWrite = 0x00002,
		shareCompat = 0x00000,
		shareExclusive = 0x00010,
		shareDenyWrite = 0x00020,
		shareDenyRead = 0x00030,
		shareDenyNone = 0x00040,
		modeNoInherit = 0x00080,
		modeCreate = 0x01000,
		modeNoTruncate = 0x02000,
		typeText = 0x04000,
		typeBinary = 0x08000,
		osNoBuffer = 0x10000,
		osWriteThrough = 0x20000,
		osRandomAccess = 0x40000,
		osSequentialScan = 0x80000
	};

	enum SeekPosition
	{
		begin = 0x0,
		current = 0x1,
		end = 0x2
	};

	CFile()
	{
		m_hFile = INVALID_HANDLE_VALUE;
		m_bCloseOnDelete = FALSE;
	}

	CFile( HANDLE hFile)
	{
		m_hFile = hFile;
		m_bCloseOnDelete = FALSE;
	}

	CFile( LPCTSTR lpszFileName, UINT nOpenFlags)
	{
		CFileException vERROR;

		m_hFile = INVALID_HANDLE_VALUE;
		m_bCloseOnDelete = FALSE;

		if(!CFile::Open( lpszFileName, nOpenFlags, &vERROR))
			AfxThrowFileException( vERROR.m_cause, vERROR.m_lOsError, vERROR.m_strFileName);
	}

	virtual ~CFile()
	{
		if( m_hFile != INVALID_HANDLE_VALUE && m_bCloseOnDelete )
			CFile::Abort();
	}

	operator HANDLE() const { return m_hFile; }

	virtual BOOL Open( LPCTSTR lpszFileName, UINT nOpenFlags, CFileException *pError = NULL)
	{
		DWORD dwAccess = 0;
		DWORD dwShare = 0;
		DWORD dwCreate = OPEN_EXISTING;
		DWORD dwFlags = FILE_ATTRIBUTE_NORMAL;

		m_hFile = INVALID_HANDLE_VALUE;
		m_strFileName = lpszFileName;

		switch( nOpenFlags & 3 )
		{
		case modeRead		: dwAccess = GENERIC_READ; break;
		case modeWrite		: dwAccess = GENERIC_WRITE; break;
		case modeReadWrite	: dwAccess = GENERIC_READ|GENERIC_WRITE; break;
		}

		// Like MFC, no share flag means exclusive.
		switch( nOpenFlags & 0x70 )
		{
		case shareDenyWrite	: dwShare = FILE_SHARE_READ; break;
		case shareDenyRead	: dwShare = FILE_SHARE_WRITE; break;
		case shareDenyNone	: dwShare = FILE_SHARE_READ|FILE_SHARE_WRITE; break;
		}

		if( nOpenFlags & modeCreate )
			dwCreate = (nOpenFlags & modeNoTruncate) ? OPEN_ALWAYS : CREATE_ALWAYS;

		if( nOpenFlags & osNoBuffer )
			dwFlags |= FILE_FLAG_NO_BUFFERING;

		if( nOpenFlags & osWriteThrough )
			dwFlags |= FILE_FLAG_WRITE_THROUGH;

		if( nOpenFlags & osRandomAccess )
			dwFlags |= FILE_FLAG_RANDOM_ACCESS;

		if( nOpenFlags & osSequentialScan )
			dwFlags |= FILE_FLAG_SEQUENTIAL_SCAN;

		SECURITY_ATTRIBUTES vSA;
		vSA.nLength = sizeof(SECURITY_ATTRIBUTES);
		vSA.lpSecurityDescriptor = NULL;
		vSA.bInheritHandle = (nOpenFlags & modeNoInherit) ? FALSE : TRUE;

		HANDLE hFile = ::CreateFile(
			CTPath::Resolve(lpszFileName),
			dwAccess,
			dwShare,
			&vSA,
			dwCreate,
			dwFlags,
			NULL);

		if( hFile == INVALID_HANDLE_VALUE )
		{
			if(pError)
			{
				pError->m_lOsError = ::GetLastError();
				pError->m_cause = CFileException::OsErrorToException(pError->m_lOsError);
				pError->m_strFileName = lpszFileName;
			}

			return FALSE;
		}

		m_hFile = hFile;
		m_bCloseOnDelete = TRUE;

		return TRUE;
	}

	virtual void Close()
	{
		BOOL bError = FALSE;

		if( m_hFile != INVALID_HANDLE_VALUE )
			bError = !::CloseHandle(m_hFile);

		m_hFile = INVALID_HANDLE_VALUE;
		m_bCloseOnDelete = FALSE;

		if(bError)
			ThrowOsError(::GetLastError());

		m_strFileName.Empty();
	}

	virtual void Abort()
	{
		if( m_hFile != INVALID_HANDLE_VALUE )
			::CloseHandle(m_hFile);

		m_hFile = INVALID_HANDLE_VALUE;
		m_bCloseOnDelete = FALSE;
		m_strFileName.Empty();
	}

	virtual UINT Read( void *lpBuf, UINT nCount)
	{
		DWORD dwRead = 0;

		if(!nCount)
			return 0;

		if(!::ReadFile( m_hFile, lpBuf, nCount, &dwRead, NULL))
			ThrowOsError(::GetLastError());

		return UINT(dwRead);
	}

	virtual void Write( const void *lpBuf, UINT nCount)
	{
		DWORD dwWritten = 0;

		if(!nCount)
			return;

		if(!::WriteFile( m_hFile, lpBuf, nCount, &dwWritten, NULL))
			ThrowOsError(::GetLastError());

		if( dwWritten != nCount )
			AfxThrowFileException( CFileException::diskFull, -1, m_strFileName);
	}

	virtual ULONGLONG Seek( LONGLONG lOff, UINT nFrom)
	{
		LARGE_INTEGER vOFF;
		LARGE_INTEGER vPOS;

		vOFF.QuadPart = lOff;
		vPOS.QuadPart = 0;

		if(!::SetFilePointerEx( m_hFile, vOFF, &vPOS, DWORD(nFrom)))
			ThrowOsError(::GetLastError());

		return ULONGLONG(vPOS.QuadPart);
	}

	void SeekToBegin()
	{
		Seek( 0, begin);
	}

	ULONGLONG SeekToEnd()
	{
		return Seek( 0, end);
	}

	virtual ULONGLONG GetPosition() const
	{
		LARGE_INTEGER vOFF;
		LARGE_INTEGER vPOS;

		vOFF.QuadPart = 0;
		vPOS.QuadPart = 0;

		if(!::SetFilePointerEx( m_hFile, vOFF, &vPOS, FILE_CURRENT))
			ThrowOsError(::GetLastError());

		return ULONGLONG(vPOS.QuadPart);
	}

	virtual ULONGLONG GetLength() const
	{
		LARGE_INTEGER vSIZE;

		if(!::GetFileSizeEx( m_hFile, &vSIZE))
			ThrowOsError(::GetLastError());

		return ULONGLONG(vSIZE.QuadPart);
	}

	virtual void SetLength( ULONGLONG dwNewLen)
	{
		Seek( LONGLONG(dwNewLen), begin);

		if(!::SetEndOfFile(m_hFile))
			ThrowOsError(::GetLastError());
	}

	virtual void Flush()
	{
		if( m_hFile != INVALID_HANDLE_VALUE && !::FlushFileBuffers(m_hFile) )
			ThrowOsError(::GetLastError());
	}

	virtual CString GetFileName() const
	{
		int nPos = m_strFileName.ReverseFind('\\');
		int nSlash = m_strFileName.ReverseFind('/');

		if( nSlash > nPos )
			nPos = nSlash;

		return m_strFileName.Mid(nPos + 1);
	}

	virtual CString GetFileTitle() const
	{
		CString strName = GetFileName();
		int nPos = strName.ReverseFind('.');

		return nPos < 0 ? strName : strName.Left(nPos);
	}

	virtual CString GetFilePath() const
	{
		return m_strFileName;
	}

	virtual void SetFilePath( LPCTSTR lpszNewName)
	{
		m_strFileName = lpszNewName;
	}

	static void Remove( LPCTSTR lpszFileName)
	{
		if(!TFileDelete(lpszFileName))
			AfxThrowFileException( CFileException::OsErrorToException(::GetLastError()), ::GetLastError(), lpszFileName);
	}

	static void Rename( LPCTSTR lpszOldName, LPCTSTR lpszNewName)
	{
		if(!TFileRename( lpszOldName, lpszNewName))
			AfxThrowFileException( CFileException::OsErrorToException(::GetLastError()), ::GetLastError(), lpszOldName);
	}

protected:
	void ThrowOsError( LONG lOsError) const
	{
		AfxThrowFileException( CFileException::OsErrorToException(lOsError), lOsError, m_strFileName);
	}

public:
	HANDLE m_hFile;
	BOOL m_bCloseOnDelete;

protected:
	CString m_strFileName;
};


//////////////////////////////////////////////////////////////////////
// CStdioFile: a CFile read and written through a CRT stream, in text mode by default.

class CStdioFile : public CFile
{
public:
	CStdioFile()
	{
		m_pStream = NULL;
	}

	CStdioFile( LPCTSTR lpszFileName, UINT nOpenFlags)
	{
		CFileException vERROR;

		m_pStream = NULL;

		if(!Open( lpszFileName, nOpenFlags, &vERROR))
			AfxThrowFileException( vERROR.m_cause, vERROR.m_lOsError, vERROR.m_strFileName);
	}

	virtual ~CStdioFile()
	{
		if( m_pStream && m_bCloseOnDelete )
			Abort();
	}

	virtual BOOL Open( LPCTSTR lpszFileName, UINT nOpenFlags, CFileException *pError = NULL)
	{
		TCHAR szMode[4];
		int nMode = 0;

		// The stream mode MFC builds from the flags.
		if( nOpenFlags & modeCreate )
			szMode[nMode++] = (nOpenFlags & modeNoTruncate) ? _T('a') : _T('w');
		else if( nOpenFlags & modeWrite )
			szMode[nMode++] = _T('a');
		else
			szMode[nMode++] = _T('r');

		if( (szMode[0] == _T('r') && (nOpenFlags & modeReadWrite)) ||
			(szMode[0] != _T('r') && !(nOpenFlags & modeWrite)) )
			szMode[nMode++] = _T('+');

		int nFlags = _O_RDONLY|_O_TEXT;
		if( nOpenFlags & (modeWrite|modeReadWrite) )
			nFlags ^= _O_RDONLY;

		if( nOpenFlags & typeBinary )
		{
			szMode[nMode++] = _T('b');
			nFlags ^= _O_TEXT;
		}
		else
			szMode[nMode++] = _T('t');
		szMode[nMode++] = 0;

		m_pStream = NULL;
		if(!CFile::Open( lpszFileName, nOpenFlags & ~(typeText|typeBinary), pError))
			return FALSE;

		int nHandle = _open_osfhandle( intptr_t(m_hFile), nFlags);
		if( nHandle != -1 )
			m_pStream = _tfdopen( nHandle, szMode);

		if(!m_pStream)
		{
			if( nHandle != -1 )
				_close(nHandle);
			else
				::CloseHandle(m_hFile);

			m_hFile = INVALID_HANDLE_VALUE;
			m_bCloseOnDelete = FALSE;

			if(pError)
			{
				pError->m_lOsError = _doserrno;
				pError->m_cause = CFileException::OsErrorToException(_doserrno);
				pError->m_strFileName = lpszFileName;
			}

			return FALSE;
		}

		return TRUE;
	}

	virtual void Close()
	{
		int nResult = 0;

		if(m_pStream)
			nResult = fclose(m_pStream);

		m_pStream = NULL;
		m_hFile = INVALID_HANDLE_VALUE;
		m_bCloseOnDelete = FALSE;

		if(nResult)
			AfxThrowFileException( CFileException::genericException, _doserrno, m_strFileName);

		m_strFileName.Empty();
	}

	virtual void Abort()
	{
		if(m_pStream)
			fclose(m_pStream);

		m_pStream = NULL;
		m_hFile = INVALID_HANDLE_VALUE;
		m_bCloseOnDelete = FALSE;
		m_strFileName.Empty();
	}

	virtual UINT Read( void *lpBuf, UINT nCount)
	{
		if(!nCount)
			return 0;

		UINT nRead = UINT(fread( lpBuf, 1, nCount, m_pStream));
		if( nRead == 0 && !feof(m_pStream) )
			ThrowStreamError();

		return nRead;
	}

	virtual void Write( const void *lpBuf, UINT nCount)
	{
		if( fwrite( lpBuf, 1, nCount, m_pStream) != nCount )
			ThrowStreamError();
	}

	virtual ULONGLONG Seek( LONGLONG lOff, UINT nFrom)
	{
		static const int vORIGIN[] = { SEEK_SET, SEEK_CUR, SEEK_END };

		if(_fseeki64( m_pStream, lOff, vORIGIN[nFrom]))
			AfxThrowFileException( CFileException::badSeek, _doserrno, m_strFileName);

		return ULONGLONG(_ftelli64(m_pStream));
	}

	virtual ULONGLONG GetPosition() const
	{
		LONGLONG llPos = _ftelli64(m_pStream);

		if( llPos < 0 )
			AfxThrowFileException( CFileException::invalidFile, _doserrno, m_strFileName);

		return ULONGLONG(llPos);
	}

	virtual ULONGLONG GetLength() const
	{
		LONGLONG llPos = _ftelli64(m_pStream);

		_fseeki64( m_pStream, 0, SEEK_END);
		LONGLONG llLength = _ftelli64(m_pStream);
		_fseeki64( m_pStream, llPos, SEEK_SET);

		return ULONGLONG(llLength);
	}

	virtual void Flush()
	{
		if( m_pStream && fflush(m_pStream) )
			ThrowStreamError();
	}

	// One line without its '\n'; FALSE at the end of the file.
	virtual BOOL ReadString( CString& rString)
	{
		TCHAR szLINE[256];
		BOOL bRead = FALSE;

		rString.Empty();
		while(_fgetts( szLINE, 256, m_pStream))
		{
			int nLength = int(_tcslen(szLINE));

			bRead = TRUE;
			if( nLength && szLINE[nLength - 1] == _T('\n') )
			{
				szLINE[nLength - 1] = 0;
				rString += szLINE;

				return TRUE;
			}

			rString += szLINE;
		}

		if(ferror(m_pStream))
		{
			clearerr(m_pStream);
			ThrowStreamError();
		}

		return bRead;
	}

	virtual LPTSTR ReadString( LPTSTR lpsz, UINT nMax)
	{
		LPTSTR lpszResult = _fgetts( lpsz, int(nMax), m_pStream);

		if( !lpszResult && !feof(m_pStream) )
		{
			clearerr(m_pStream);
			ThrowStreamError();
		}

		return lpszResult;
	}

	virtual void WriteString( LPCTSTR lpsz)
	{
		if( _fputts( lpsz, m_pStream) == _TEOF )
			ThrowStreamError();
	}

protected:
	void ThrowStreamError() const
	{
		AfxThrowFileException( CFileException::genericException, _doserrno, m_strFileName);
	}

public:
	FILE *m_pStream;
};


//////////////////////////////////////////////////////////////////////
// CFileFind: FindNextFile moves to the next match and returns whether one more follows,
// so the usual loop is: bFound = FindFile(); while(bFound) { bFound = FindNextFile(); ... }

class CFileFind : public CObject
{
public:
	CFileFind()
	{
		m_hContext = NULL;
		m_bFound = FALSE;
		m_bNext = FALSE;
	}

	virtual ~CFileFind()
	{
		Close();
	}

	virtual BOOL FindFile( LPCTSTR pstrName = NULL, DWORD dwUnused = 0)
	{
		Close();

		if(!pstrName)
			pstrName = _T("*.*");

		CString strName = CTPath::Resolve(pstrName);

		m_hContext = ::FindFirstFile( strName, &m_vNEXT);
		if( m_hContext == INVALID_HANDLE_VALUE )
		{
			m_hContext = NULL;
			return FALSE;
		}

		TCHAR szFULL[MAX_PATH];
		LPTSTR lpszName = NULL;

		if(::GetFullPathName( strName, MAX_PATH, szFULL, &lpszName))
		{
			if(lpszName)
				*lpszName = 0;

			m_strRoot = szFULL;
		}
		else
			m_strRoot.Empty();

		m_bNext = TRUE;
		return TRUE;
	}

	virtual BOOL FindNextFile()
	{
		if( !m_hContext || !m_bNext )
			return FALSE;

		m_vFOUND = m_vNEXT;
		m_bFound = TRUE;
		m_bNext = ::FindNextFile( m_hContext, &m_vNEXT);

		return m_bNext;
	}

	void Close()
	{
		if(m_hContext)
			::FindClose(m_hContext);

		m_hContext = NULL;
		m_bFound = FALSE;
		m_bNext = FALSE;
	}

	CString GetFileName() const
	{
		return m_bFound ? CString(m_vFOUND.cFileName) : CString();
	}

	CString GetFileTitle() const
	{
		CString strName = GetFileName();
		int nPos = strName.ReverseFind('.');

		return nPos < 0 ? strName : strName.Left(nPos);
	}

	CString GetFilePath() const
	{
		CString strPath = m_strRoot;

		if( !strPath.IsEmpty() && strPath.Right(1) != _T("\\") && strPath.Right(1) != _T("/") )
			strPath += _T('\\');

		return strPath + GetFileName();
	}

	CString GetRoot() const
	{
		return m_strRoot;
	}

	ULONGLONG GetLength() const
	{
		return m_bFound ? (ULONGLONG(m_vFOUND.nFileSizeHigh) << 32) | m_vFOUND.nFileSizeLow : 0;
	}

	BOOL MatchesMask( DWORD dwMask) const
	{
		return m_bFound && (m_vFOUND.dwFileAttributes & dwMask) ? TRUE : FALSE;
	}

	BOOL IsDirectory() const
	{
		return MatchesMask(FILE_ATTRIBUTE_DIRECTORY);
	}

	BOOL IsDots() const
	{
		return IsDirectory() && m_vFOUND.cFileName[0] == _T('.') && (
			!m_vFOUND.cFileName[1] || (m_vFOUND.cFileName[1] == _T('.') && !m_vFOUND.cFileName[2]));
	}

protected:
	WIN32_FIND_DATA m_vFOUND;
	WIN32_FIND_DATA m_vNEXT;
	HANDLE m_hContext;
	CString m_strRoot;
	BOOL m_bFound;
	BOOL m_bNext;
};


//////////////////////////////////////////////////////////////////////
// CArchive: buffered binary reads and writes in MFC's format (little endian, 4 byte long).

class CArchive
{
public:
	enum Mode
	{
		store = 0,
		load = 1,
		bNoFlushOnDelete = 2,
		bNoByteSwap = 4
	};

	CArchive( CFile *pFile, UINT nMode, int nBufSize = 4096, void *lpBuf = NULL)
	{
		m_pFile = pFile;
		m_nMode = nMode;
		m_nBufSize = UINT(nBufSize < 128 ? 128 : nBufSize);
		m_bUserBuf = lpBuf != NULL;
		m_lpBufStart = lpBuf ? (BYTE *) lpBuf : new BYTE[m_nBufSize];
		m_lpBufCur = m_lpBufStart;
		m_lpBufMax = IsLoading() ? m_lpBufStart : m_lpBufStart + m_nBufSize;
	}

	~CArchive()
	{
		if( m_pFile && !(m_nMode & bNoFlushOnDelete) )
		{
			try
			{
				Close();
			}
			catch( CException *e )
			{
				e->Delete();
			}
		}

		Abort();
	}

	BOOL IsLoading() const { return (m_nMode & load) != 0; }
	BOOL IsStoring() const { return (m_nMode & load) == 0; }
	CFile *GetFile() const { return m_pFile; }

	// Up to nMax bytes; fewer at the end of the file.
	UINT Read( void *lpBuf, UINT nMax)
	{
		LPBYTE pDEST = (LPBYTE) lpBuf;
		UINT nTotal = 0;

		while( nMax > 0 )
		{
			UINT nCopy = UINT(m_lpBufMax - m_lpBufCur);

			if( nCopy > nMax )
				nCopy = nMax;

			if(nCopy)
			{
				memcpy( pDEST, m_lpBufCur, nCopy);
				m_lpBufCur += nCopy;
				pDEST += nCopy;
				nMax -= nCopy;
				nTotal += nCopy;

				continue;
			}

			// Large blocks go straight to the file.
			if( nMax >= m_nBufSize )
			{
				UINT nRead = m_pFile->Read( pDEST, nMax);

				if(!nRead)
					break;

				pDEST += nRead;
				nMax -= nRead;
				nTotal += nRead;

				continue;
			}

			m_lpBufCur = m_lpBufStart;
			m_lpBufMax = m_lpBufStart + m_pFile->Read( m_lpBufStart, m_nBufSize);

			if( m_lpBufMax == m_lpBufStart )
				break;
		}

		return nTotal;
	}

	void EnsureRead( void *lpBuf, UINT nCount)
	{
		if( Read( lpBuf, nCount) != nCount )
			AfxThrowArchiveException( CArchiveException::endOfFile, GetFileName());
	}

	void Write( const void *lpBuf, UINT nMax)
	{
		const BYTE *pSRC = (const BYTE *) lpBuf;

		while( nMax > 0 )
		{
			UINT nCopy = UINT(m_lpBufMax - m_lpBufCur);

			if( nCopy > nMax )
				nCopy = nMax;

			memcpy( m_lpBufCur, pSRC, nCopy);
			m_lpBufCur += nCopy;
			pSRC += nCopy;
			nMax -= nCopy;

			if( m_lpBufCur == m_lpBufMax )
				Flush();
		}
	}

	// Stores the buffer; when loading, gives the unread bytes back to the file.
	void Flush()
	{
		if(!m_pFile)
			return;

		if(IsLoading())
		{
			if( m_lpBufMax != m_lpBufCur )
				m_pFile->Seek( -LONGLONG(m_lpBufMax - m_lpBufCur), CFile::current);

			m_lpBufCur = m_lpBufMax;
		}
		else
		{
			if( m_lpBufCur != m_lpBufStart )
				m_pFile->Write( m_lpBufStart, UINT(m_lpBufCur - m_lpBufStart));

			m_lpBufCur = m_lpBufStart;
		}
	}

	void Close()
	{
		Flush();
		m_pFile = NULL;
	}

	void Abort()
	{
		if( !m_bUserBuf && m_lpBufStart )
			delete[] m_lpBufStart;

		m_lpBufStart = NULL;
		m_lpBufCur = NULL;
		m_lpBufMax = NULL;
		m_pFile = NULL;
	}

	BOOL ReadString( CString& rString)
	{
		BOOL bRead = FALSE;
		char cChar;

		rString.Empty();
		while( Read( &cChar, 1) == 1 )
		{
			bRead = TRUE;

			if( cChar == '\n' )
				break;

			if( cChar != '\r' )
				rString += cChar;
		}

		return bRead;
	}

	void WriteString( LPCTSTR lpsz)
	{
		Write( lpsz, UINT(_tcslen(lpsz) * sizeof(TCHAR)));
	}

	CArchive& operator << ( char ch) { return WriteT(ch); }
	CArchive& operator << ( unsigned char by) { return WriteT(by); }
	CArchive& operator << ( short w) { return WriteT(w); }
	CArchive& operator << ( unsigned short w) { return WriteT(w); }
	CArchive& operator << ( int i) { return WriteT(INT32(i)); }
	CArchive& operator << ( unsigned int u) { return WriteT(UINT32(u)); }
	CArchive& operator << ( long l) { return WriteT(INT32(l)); }
	CArchive& operator << ( unsigned long dw) { return WriteT(UINT32(dw)); }
	CArchive& operator << ( LONGLONG dwdw) { return WriteT(dwdw); }
	CArchive& operator << ( ULONGLONG dwdw) { return WriteT(dwdw); }
	CArchive& operator << ( float f) { return WriteT(f); }
	CArchive& operator << ( double d) { return WriteT(d); }
	CArchive& operator << ( wchar_t ch) { return WriteT(ch); }

	CArchive& operator >> ( char& ch) { return ReadT(ch); }
	CArchive& operator >> ( unsigned char& by) { return ReadT(by); }
	CArchive& operator >> ( short& w) { return ReadT(w); }
	CArchive& operator >> ( unsigned short& w) { return ReadT(w); }
	CArchive& operator >> ( int& i) { INT32 v; ReadT(v); i = int(v); return *this; }
	CArchive& operator >> ( unsigned int& u) { UINT32 v; ReadT(v); u = (unsigned int) v; return *this; }
	CArchive& operator >> ( long& l) { INT32 v; ReadT(v); l = long(v); return *this; }
	CArchive& operator >> ( unsigned long& dw) { UINT32 v; ReadT(v); dw = (unsigned long) v; return *this; }
	CArchive& operator >> ( LONGLONG& dwdw) { return ReadT(dwdw); }
	CArchive& operator >> ( ULONGLONG& dwdw) { return ReadT(dwdw); }
	CArchive& operator >> ( float& f) { return ReadT(f); }
	CArchive& operator >> ( double& d) { return ReadT(d); }
	CArchive& operator >> ( wchar_t& ch) { return ReadT(ch); }

protected:
	template< class T > CArchive& ReadT( T& vValue)
	{
		if( UINT(m_lpBufMax - m_lpBufCur) >= sizeof(T) )
		{
			memcpy( &vValue, m_lpBufCur, sizeof(T));
			m_lpBufCur += sizeof(T);
		}
		else
			EnsureRead( &vValue, sizeof(T));

		return *this;
	}

	template< class T > CArchive& WriteT( const T& vValue)
	{
		if( UINT(m_lpBufMax - m_lpBufCur) >= sizeof(T) )
		{
			memcpy( m_lpBufCur, &vValue, sizeof(T));
			m_lpBufCur += sizeof(T);
		}
		else
			Write( &vValue, sizeof(T));

		return *this;
	}

	CString GetFileName() const
	{
		return m_pFile ? m_pFile->GetFilePath() : CString();
	}

protected:
	CFile *m_pFile;
	UINT m_nMode;
	UINT m_nBufSize;
	BOOL m_bUserBuf;

	BYTE *m_lpBufStart;
	BYTE *m_lpBufCur;
	BYTE *m_lpBufMax;
};

// MFC's CString length prefix: a BYTE, then 0xFF + WORD, then 0xFFFF + DWORD, then
// 0xFFFFFFFF + ULONGLONG. 0xFF 0xFFFE first marks a wide string.
inline ULONGLONG AfxReadStringLength( CArchive& ar, int& nCharSize)
{
	BYTE bLength;
	WORD wLength;
	DWORD dwLength;
	ULONGLONG qwLength;

	nCharSize = sizeof(char);
	ar >> bLength;
	if( bLength < 0xFF )
		return bLength;

	ar >> wLength;
	if( wLength == 0xFFFE )
	{
		// MFC's wide strings are UTF-16, 2 bytes a character (wchar_t is 4 on Linux).
		nCharSize = 2;

		ar >> bLength;
		if( bLength < 0xFF )
			return bLength;

		ar >> wLength;
	}

	if( wLength < 0xFFFF )
		return wLength;

	ar >> dwLength;
	if( dwLength < 0xFFFFFFFF )
		return dwLength;

	ar >> qwLength;
	return qwLength;
}

inline void AfxWriteStringLength( CArchive& ar, ULONGLONG nLength, BOOL bUnicode)
{
	if(bUnicode)
	{
		ar << BYTE(0xFF);
		ar << WORD(0xFFFE);
	}

	if( nLength < 0xFF )
	{
		ar << BYTE(nLength);
		return;
	}
	ar << BYTE(0xFF);

	if( nLength < 0xFFFE )
	{
		ar << WORD(nLength);
		return;
	}
	ar << WORD(0xFFFF);

	if( nLength < 0xFFFFFFFF )
	{
		ar << DWORD(nLength);
		return;
	}

	ar << DWORD(0xFFFFFFFF);
	ar << ULONGLONG(nLength);
}

inline CArchive& operator >> ( CArchive& ar, CString& str)
{
	int nCharSize;
	ULONGLONG nLength = AfxReadStringLength( ar, nCharSize);

	if( nLength > 0x7FFFFFFF / sizeof(wchar_t) )
		AfxThrowArchiveException( CArchiveException::genericException);

	if( nCharSize == 2 )
	{
		std::vector<WORD> vUTF16( size_t(nLength) + 1, 0);
		CStringW strW;

		if(nLength)
			ar.EnsureRead( &vUTF16[0], UINT(nLength * 2));

		// UTF-16 units to wide characters (the same where wchar_t is 2 bytes).
		for( ULONGLONG i=0; i<nLength; i++)
			strW.AppendChar(wchar_t(vUTF16[size_t(i)]));

		str = strW;
	}
	else
	{
		CStringA strA;

		ar.EnsureRead( strA.GetBufferSetLength(int(nLength)), UINT(nLength));
		strA.ReleaseBuffer(int(nLength));
		str = strA;
	}

	return ar;
}

inline CArchive& operator << ( CArchive& ar, const CString& str)
{
	AfxWriteStringLength( ar, ULONGLONG(str.GetLength()), sizeof(TCHAR) == sizeof(wchar_t));
	ar.Write( LPCTSTR(str), UINT(str.GetLength() * sizeof(TCHAR)));

	return ar;
}

inline CArchive& operator << ( CArchive& ar, SIZE size) { ar.Write( &size, sizeof(SIZE)); return ar; }
inline CArchive& operator << ( CArchive& ar, POINT point) { ar.Write( &point, sizeof(POINT)); return ar; }
inline CArchive& operator << ( CArchive& ar, const RECT& rect) { ar.Write( &rect, sizeof(RECT)); return ar; }
inline CArchive& operator >> ( CArchive& ar, SIZE& size) { ar.EnsureRead( &size, sizeof(SIZE)); return ar; }
inline CArchive& operator >> ( CArchive& ar, POINT& point) { ar.EnsureRead( &point, sizeof(POINT)); return ar; }
inline CArchive& operator >> ( CArchive& ar, RECT& rect) { ar.EnsureRead( &rect, sizeof(RECT)); return ar; }


//////////////////////////////////////////////////////////////////////
// CGdiObject / CFont: owners of a GDI handle.

class CGdiObject : public CObject
{
public:
	CGdiObject() { m_hObject = NULL; }
	virtual ~CGdiObject() { DeleteObject(); }

	operator HGDIOBJ() const { return m_hObject; }

	BOOL Attach( HGDIOBJ hObject)
	{
		if(!hObject)
			return FALSE;

		m_hObject = hObject;
		return TRUE;
	}

	HGDIOBJ Detach()
	{
		HGDIOBJ hObject = m_hObject;

		m_hObject = NULL;
		return hObject;
	}

	BOOL DeleteObject()
	{
		if(!m_hObject)
			return FALSE;

		return ::DeleteObject(Detach());
	}

	int GetObject( int nCount, LPVOID lpObject) const
	{
		return ::GetObject( m_hObject, nCount, lpObject);
	}

public:
	HGDIOBJ m_hObject;
};

// The font keeps its LOGFONT: text drawn from font files (TTextRaster.h) works from it, not from
// the GDI font.
class CFont : public CGdiObject
{
public:
	CFont() { memset( &m_vLOGFONT, 0, sizeof(LOGFONT)); }

	operator HFONT() const { return (HFONT) m_hObject; }

	BOOL CreateFontIndirect( const LOGFONT *lpLogFont)
	{
		m_vLOGFONT = *lpLogFont;
		return Attach(::CreateFontIndirect(lpLogFont));
	}

	BOOL CreateFont(
		int nHeight,
		int nWidth,
		int nEscapement,
		int nOrientation,
		int nWeight,
		BYTE bItalic,
		BYTE bUnderline,
		BYTE cStrikeOut,
		BYTE nCharSet,
		BYTE nOutPrecision,
		BYTE nClipPrecision,
		BYTE nQuality,
		BYTE nPitchAndFamily,
		LPCTSTR lpszFacename)
	{
		LOGFONT vLOGFONT;

		memset( &vLOGFONT, 0, sizeof(LOGFONT));
		vLOGFONT.lfHeight = nHeight;
		vLOGFONT.lfWidth = nWidth;
		vLOGFONT.lfEscapement = nEscapement;
		vLOGFONT.lfOrientation = nOrientation;
		vLOGFONT.lfWeight = nWeight;
		vLOGFONT.lfItalic = bItalic;
		vLOGFONT.lfUnderline = bUnderline;
		vLOGFONT.lfStrikeOut = cStrikeOut;
		vLOGFONT.lfCharSet = nCharSet;
		vLOGFONT.lfOutPrecision = nOutPrecision;
		vLOGFONT.lfClipPrecision = nClipPrecision;
		vLOGFONT.lfQuality = nQuality;
		vLOGFONT.lfPitchAndFamily = nPitchAndFamily;

		if(lpszFacename)
			_tcsncpy_s( vLOGFONT.lfFaceName, lpszFacename, _TRUNCATE);

		return CreateFontIndirect(&vLOGFONT);
	}

	int GetLogFont( LOGFONT *pLogFont) const
	{
		*pLogFont = m_vLOGFONT;
		return sizeof(LOGFONT);
	}

	const LOGFONT& GetLogFont() const
	{
		return m_vLOGFONT;
	}

protected:
	LOGFONT m_vLOGFONT;
};

#endif // !_AFX

#endif // !defined __TAFX_H
