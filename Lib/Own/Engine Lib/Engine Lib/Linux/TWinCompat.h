// TWinCompat.h: the part of the Windows API the client still uses, for Linux. The stand-in
// headers of this folder (windows.h, tchar.h...) include it.
//
// The types keep their Windows sizes (DWORD and LONG are 32 bits, long is 64 bits on Linux), the
// constants their Windows values (TWinConst.h), and the functions do what the client asks of
// them: the CRT's secure and MBCS variants, time, interlocked operations, rectangles, and the file
// functions over POSIX ones, so the engine's CFile and CFileFind work unchanged.
//////////////////////////////////////////////////////////////////////

#pragma once

#ifdef _WIN32
#error TWinCompat.h replaces the Windows headers on the other systems only.
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <wchar.h>
#include <wctype.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#include <alloca.h>
#include <sched.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef __cplusplus
#include <type_traits>
#endif


//////////////////////////////////////////////////////////////////////
// Types

typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef int BOOL;
typedef unsigned int UINT;
typedef int INT;
typedef float FLOAT;
typedef double DOUBLE;
typedef char CHAR;
typedef unsigned char UCHAR;
typedef short SHORT;
typedef unsigned short USHORT;
// long long, not int64_t (long on Linux): long keeps its own overloads (32 bits in the game's
// files, as on Windows).
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
typedef long long INT64;
typedef unsigned long long UINT64;
typedef unsigned long long DWORD64;
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int16_t INT16;
typedef uint16_t UINT16;
typedef int8_t INT8;
typedef uint8_t UINT8;
typedef BYTE BOOLEAN;
typedef wchar_t WCHAR;
typedef char TCHAR;
typedef char _TCHAR;
typedef DWORD COLORREF;
typedef WORD ATOM;
typedef LONG HRESULT;
typedef intptr_t INT_PTR;
typedef uintptr_t UINT_PTR;
typedef intptr_t LONG_PTR;
typedef uintptr_t ULONG_PTR;
typedef uintptr_t DWORD_PTR;
typedef size_t SIZE_T;
typedef intptr_t SSIZE_T;
typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;
// long long like Windows' __int64 (int64_t is long here): packets read and write it as __int64,
// 8 bytes, where long is their 32-bit LONG.
typedef long long __time64_t;

#define VOID void
#define CONST const

typedef void *LPVOID;
typedef void *PVOID;
typedef const void *LPCVOID;
typedef CHAR *LPSTR;
typedef CHAR *PSTR;
typedef const CHAR *LPCSTR;
typedef const CHAR *PCSTR;
typedef TCHAR *LPTSTR;
typedef const TCHAR *LPCTSTR;
typedef WCHAR *LPWSTR;
typedef WCHAR *PWSTR;
typedef const WCHAR *LPCWSTR;
typedef const WCHAR *PCWSTR;
typedef BYTE *LPBYTE;
typedef BYTE *PBYTE;
typedef WORD *LPWORD;
typedef WORD *PWORD;
typedef DWORD *LPDWORD;
typedef DWORD *PDWORD;
typedef LONG *LPLONG;
typedef LONG *PLONG;
typedef BOOL *LPBOOL;
typedef BOOL *PBOOL;
typedef INT *LPINT;
typedef INT *PINT;
typedef UINT *LPUINT;
typedef UINT *PUINT;
typedef FLOAT *LPFLOAT;
typedef FLOAT *PFLOAT;
typedef INT64 *PINT64;
typedef UINT64 *PUINT64;
typedef ULONG_PTR *PULONG_PTR;

// Handles: distinct pointer types, as with STRICT.
#define DECLARE_HANDLE(name)		struct name##__ { int unused; }; typedef struct name##__ *name

typedef void *HANDLE;
typedef HANDLE *LPHANDLE;
typedef HANDLE HGDIOBJ;
typedef HANDLE HGLOBAL;
typedef HANDLE HLOCAL;
DECLARE_HANDLE(HWND);
DECLARE_HANDLE(HINSTANCE);
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HFONT);
DECLARE_HANDLE(HBITMAP);
DECLARE_HANDLE(HBRUSH);
DECLARE_HANDLE(HPEN);
DECLARE_HANDLE(HRGN);
DECLARE_HANDLE(HICON);
DECLARE_HANDLE(HMENU);
DECLARE_HANDLE(HKEY);
DECLARE_HANDLE(HKL);
DECLARE_HANDLE(HACCEL);
DECLARE_HANDLE(HRSRC);
DECLARE_HANDLE(HIMC);
DECLARE_HANDLE(HMONITOR);
DECLARE_HANDLE(HHOOK);
typedef HICON HCURSOR;
typedef HINSTANCE HMODULE;

#define WINAPI
#define WINAPIV
#define APIENTRY
#define CALLBACK
#define STDMETHODCALLTYPE
#define STDAPI						extern "C" HRESULT
#define __stdcall
#define __cdecl
#define __fastcall
#define __forceinline				inline __attribute__((always_inline))
#define FAR
#define NEAR
#define far
#define near
#define IN
#define OUT
#define OPTIONAL
#define UNALIGNED
#define __noop(...)					((void) 0)
#define __debugbreak()				__builtin_trap()
#define __assume(x)					((void) 0)
#define _countof(a)					(sizeof(a) / sizeof((a)[0]))
#define ARRAYSIZE(a)				_countof(a)
#define UNREFERENCED_PARAMETER(p)	((void) (p))

#define STDMETHOD(method)			virtual HRESULT method
#define STDMETHOD_(type, method)	virtual type method
#define PURE						= 0
#define interface					struct

#ifndef TRUE
#define TRUE						1
#endif

#ifndef FALSE
#define FALSE						0
#endif


//////////////////////////////////////////////////////////////////////
// Structures

typedef struct tagPOINT { LONG x; LONG y; } POINT, *PPOINT, *LPPOINT;
typedef struct tagSIZE { LONG cx; LONG cy; } SIZE, *PSIZE, *LPSIZE;
typedef struct tagRECT { LONG left; LONG top; LONG right; LONG bottom; } RECT, *PRECT, *LPRECT;
typedef const RECT *LPCRECT;
typedef struct tagPOINTS { SHORT x; SHORT y; } POINTS;

typedef struct tagLOGFONTA
{
	LONG lfHeight;
	LONG lfWidth;
	LONG lfEscapement;
	LONG lfOrientation;
	LONG lfWeight;
	BYTE lfItalic;
	BYTE lfUnderline;
	BYTE lfStrikeOut;
	BYTE lfCharSet;
	BYTE lfOutPrecision;
	BYTE lfClipPrecision;
	BYTE lfQuality;
	BYTE lfPitchAndFamily;
	CHAR lfFaceName[32];
} LOGFONTA, LOGFONT, *PLOGFONT, *LPLOGFONT;

typedef struct tagPALETTEENTRY { BYTE peRed; BYTE peGreen; BYTE peBlue; BYTE peFlags; } PALETTEENTRY, *LPPALETTEENTRY;
typedef struct _RGNDATAHEADER { DWORD dwSize; DWORD iType; DWORD nCount; DWORD nRgnSize; RECT rcBound; } RGNDATAHEADER;
typedef struct _RGNDATA { RGNDATAHEADER rdh; char Buffer[1]; } RGNDATA, *LPRGNDATA;

typedef struct _SYSTEMTIME
{
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME, *LPSYSTEMTIME;

typedef struct _FILETIME { DWORD dwLowDateTime; DWORD dwHighDateTime; } FILETIME, *LPFILETIME;

typedef union _LARGE_INTEGER
{
	struct { DWORD LowPart; LONG HighPart; };
	struct { DWORD LowPart; LONG HighPart; } u;
	LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef union _ULARGE_INTEGER
{
	struct { DWORD LowPart; DWORD HighPart; };
	struct { DWORD LowPart; DWORD HighPart; } u;
	ULONGLONG QuadPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;

typedef struct _GUID { DWORD Data1; WORD Data2; WORD Data3; BYTE Data4[8]; } GUID, IID, CLSID;
typedef const GUID& REFGUID;
typedef const IID& REFIID;

typedef struct _SECURITY_ATTRIBUTES { DWORD nLength; LPVOID lpSecurityDescriptor; BOOL bInheritHandle; } SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

typedef struct _WIN32_FIND_DATAA
{
	DWORD dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
	DWORD dwReserved0;
	DWORD dwReserved1;
	CHAR cFileName[260];
	CHAR cAlternateFileName[14];
} WIN32_FIND_DATAA, WIN32_FIND_DATA, *LPWIN32_FIND_DATA;

typedef struct _WIN32_FILE_ATTRIBUTE_DATA
{
	DWORD dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
} WIN32_FILE_ATTRIBUTE_DATA, *LPWIN32_FILE_ATTRIBUTE_DATA;

typedef enum _GET_FILEEX_INFO_LEVELS { GetFileExInfoStandard } GET_FILEEX_INFO_LEVELS;

typedef struct _MEMORYSTATUSEX
{
	DWORD dwLength;
	DWORD dwMemoryLoad;
	ULONGLONG ullTotalPhys;
	ULONGLONG ullAvailPhys;
	ULONGLONG ullTotalPageFile;
	ULONGLONG ullAvailPageFile;
	ULONGLONG ullTotalVirtual;
	ULONGLONG ullAvailVirtual;
	ULONGLONG ullAvailExtendedVirtual;
} MEMORYSTATUSEX, *LPMEMORYSTATUSEX;

typedef struct _SYSTEM_INFO
{
	WORD wProcessorArchitecture;
	WORD wReserved;
	DWORD dwPageSize;
	LPVOID lpMinimumApplicationAddress;
	LPVOID lpMaximumApplicationAddress;
	DWORD_PTR dwActiveProcessorMask;
	DWORD dwNumberOfProcessors;
	DWORD dwProcessorType;
	DWORD dwAllocationGranularity;
	WORD wProcessorLevel;
	WORD wProcessorRevision;
} SYSTEM_INFO, *LPSYSTEM_INFO;

typedef struct _OVERLAPPED
{
	ULONG_PTR Internal;
	ULONG_PTR InternalHigh;
	DWORD Offset;
	DWORD OffsetHigh;
	HANDLE hEvent;
} OVERLAPPED, *LPOVERLAPPED;

#pragma pack(push, 2)
typedef struct tagBITMAPFILEHEADER
{
	WORD bfType;
	DWORD bfSize;
	WORD bfReserved1;
	WORD bfReserved2;
	DWORD bfOffBits;
} BITMAPFILEHEADER;
#pragma pack(pop)

typedef struct tagBITMAPINFOHEADER
{
	DWORD biSize;
	LONG biWidth;
	LONG biHeight;
	WORD biPlanes;
	WORD biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG biXPelsPerMeter;
	LONG biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER;

typedef struct tagRGBQUAD { BYTE rgbBlue; BYTE rgbGreen; BYTE rgbRed; BYTE rgbReserved; } RGBQUAD;

typedef struct tagMSG
{
	HWND hwnd;
	UINT message;
	WPARAM wParam;
	LPARAM lParam;
	DWORD time;
	POINT pt;
} MSG, *LPMSG;


//////////////////////////////////////////////////////////////////////
// Constants with Windows values

#include "TWinConst.h"

#define MAKEWORD(a, b)				((WORD) (((BYTE) ((DWORD_PTR) (a) & 0xff)) | ((WORD) ((BYTE) ((DWORD_PTR) (b) & 0xff))) << 8))
#define MAKELONG(a, b)				((LONG) (((WORD) ((DWORD_PTR) (a) & 0xffff)) | ((DWORD) ((WORD) ((DWORD_PTR) (b) & 0xffff))) << 16))
#define LOWORD(l)					((WORD) ((DWORD_PTR) (l) & 0xffff))
#define HIWORD(l)					((WORD) ((DWORD_PTR) (l) >> 16))
#define LOBYTE(w)					((BYTE) ((DWORD_PTR) (w) & 0xff))
#define HIBYTE(w)					((BYTE) ((DWORD_PTR) (w) >> 8))
#define MAKELPARAM(l, h)			((LPARAM) (DWORD) MAKELONG( l, h))
#define MAKEWPARAM(l, h)			((WPARAM) (DWORD) MAKELONG( l, h))
#define MAKELRESULT(l, h)			((LRESULT) (DWORD) MAKELONG( l, h))
#define GET_X_LPARAM(lp)			((int) (short) LOWORD(lp))
#define GET_Y_LPARAM(lp)			((int) (short) HIWORD(lp))
#define RGB(r, g, b)				((COLORREF) (((BYTE) (r) | ((WORD) ((BYTE) (g)) << 8)) | (((DWORD) (BYTE) (b)) << 16)))
#define GetRValue(rgb)				(LOBYTE(rgb))
#define GetGValue(rgb)				(LOBYTE(((WORD) (rgb)) >> 8))
#define GetBValue(rgb)				(LOBYTE((rgb) >> 16))
#define MAKEFOURCC(ch0, ch1, ch2, ch3)	((DWORD) (BYTE) (ch0) | ((DWORD) (BYTE) (ch1) << 8) | ((DWORD) (BYTE) (ch2) << 16) | ((DWORD) (BYTE) (ch3) << 24))

#define S_OK						((HRESULT) 0)
#define S_FALSE						((HRESULT) 1)
#define E_FAIL						((HRESULT) 0x80004005L)
#define E_POINTER					((HRESULT) 0x80004003L)
#define E_NOINTERFACE				((HRESULT) 0x80004002L)
#define E_NOTIMPL					((HRESULT) 0x80004001L)
#define E_OUTOFMEMORY				((HRESULT) 0x8007000EL)
#define E_INVALIDARG				((HRESULT) 0x80070057L)
#define E_ABORT						((HRESULT) 0x80004004L)
#define E_UNEXPECTED				((HRESULT) 0x8000FFFFL)
#define SUCCEEDED(hr)				(((HRESULT) (hr)) >= 0)
#define FAILED(hr)					(((HRESULT) (hr)) < 0)
#define MAKE_HRESULT(sev, fac, code)	((HRESULT) (((unsigned long) (sev) << 31) | ((unsigned long) (fac) << 16) | ((unsigned long) (code))))
#define HRESULT_FROM_WIN32(x)		((HRESULT) (x) <= 0 ? ((HRESULT) (x)) : ((HRESULT) (((x) & 0x0000FFFF) | (7 << 16) | 0x80000000)))

#define INVALID_HANDLE_VALUE		((HANDLE) (LONG_PTR) -1)

// WM_NCHITTEST's results.
#define HTERROR						(-2)
#define HTTRANSPARENT				(-1)
#define HTNOWHERE					0
#define HTCLIENT					1
#define HTCAPTION					2
#define HTSYSMENU					3
#define HTMINBUTTON					8
#define HTMAXBUTTON					9
#define HTCLOSE						20

#define _TRUNCATE					((size_t) -1)
#define _MAX_PATH					260
#define _MAX_DRIVE					3
#define _MAX_DIR					256
#define _MAX_FNAME					256
#define _MAX_EXT					256

#define _O_RDONLY					O_RDONLY
#define _O_WRONLY					O_WRONLY
#define _O_RDWR						O_RDWR
#define _O_CREAT					O_CREAT
#define _O_TRUNC					O_TRUNC
#define _O_APPEND					O_APPEND
#define _O_BINARY					0
#define _O_TEXT						0
#define _SH_DENYNO					0x40
#define _SH_DENYRW					0x10
#define _SH_DENYWR					0x20
#define _SH_DENYRD					0x30


//////////////////////////////////////////////////////////////////////
// min and max: the macros of windows.h, as functions of the common type of their arguments.

#ifdef __cplusplus
template< class A, class B > inline typename std::common_type<A, B>::type min( A a, B b) { return b < a ? b : a; }
template< class A, class B > inline typename std::common_type<A, B>::type max( A a, B b) { return a < b ? b : a; }
#endif


//////////////////////////////////////////////////////////////////////
// Memory

#define ZeroMemory(p, n)			memset( (p), 0, (n))

#ifdef __cplusplus
inline int memcpy_s( void *pDest, size_t nDest, const void *pSource, size_t nCount)
{
	if( nCount > nDest )
		return ERANGE;

	memcpy( pDest, pSource, nCount);
	return 0;
}
#endif
#define FillMemory(p, n, v)			memset( (p), (v), (n))
#define CopyMemory(d, s, n)			memcpy( (d), (s), (n))
#define MoveMemory(d, s, n)			memmove( (d), (s), (n))
#define SecureZeroMemory(p, n)		memset( (p), 0, (n))
#define _alloca						alloca


//////////////////////////////////////////////////////////////////////
// Strings: the MSVC CRT's names and its secure variants.

#define _T(x)						x
#define _TEXT(x)					x
#define TEXT(x)						x
#define __T(x)						x
#define _TEOF						EOF

#define _stricmp					strcasecmp
#define _strnicmp					strncasecmp
#define stricmp						strcasecmp
#define strnicmp					strncasecmp
#define _strcmpi					strcasecmp
#define _strdup						strdup
#define _snprintf					snprintf
#define _vsnprintf					vsnprintf
#define _atoi64						atoll
#define _strtoi64					strtoll
#define _strtoui64					strtoull
#define _wcsicmp					wcscasecmp
#define _wcsnicmp					wcsncasecmp
#define _wcsicoll					wcscasecmp
#define _wtoi(s)					((int) wcstol( (s), NULL, 10))
#define lstrcmp						strcmp
#define lstrcmpi					strcasecmp
#define lstrlen						strlen
#define lstrcpy						strcpy
#define lstrcat						strcat

#define _tcslen						strlen
#define _tcscpy						strcpy
#define _tcsncpy					strncpy
#define _tcscat						strcat
#define _tcscmp						strcmp
#define _tcsncmp					strncmp
#define _tcsicmp					strcasecmp
#define _tcsnicmp					strncasecmp
#define _tcschr						strchr
#define _tcsrchr					strrchr
#define _tcsstr						strstr
#define _tcstok						strtok
#define _tcstol						strtol
#define _tcstoul					strtoul
#define _tcstod						strtod
#define _tcsdup						strdup
#define _tcsftime					strftime
#define _ttoi						atoi
#define _ttol						atol
#define _tstoi						atoi
#define _ttof						atof
#define _tstof						atof
#define _ttoi64						atoll
#define _stprintf					sprintf
#define _sntprintf					snprintf
#define _vstprintf					vsprintf
#define _vsntprintf					vsnprintf
#define _tprintf					printf
#define _ftprintf					fprintf
#define _stscanf					sscanf
#define _fgetts						fgets
#define _fputts						fputs
#define _tfopen						fopen
#define _tremove					remove
#define _trename					rename
#define _istdigit					isdigit
#define _istxdigit					isxdigit
#define _istspace					isspace
#define _istalpha					isalpha
#define _istalnum					isalnum
#define _istupper					isupper
#define _istlower					islower
#define _totupper					toupper
#define _totlower					tolower

#ifdef __cplusplus
inline int _vscprintf( const char *pszFormat, va_list args)
{
	va_list vCOPY;
	va_copy( vCOPY, args);

	int nLength = vsnprintf( NULL, 0, pszFormat, vCOPY);
	va_end(vCOPY);

	return nLength;
}

inline int _vscwprintf( const wchar_t *pszFormat, va_list args)
{
	// vswprintf has no length mode: grow until it fits.
	for( size_t nSize = 256; nSize < (1 << 24); nSize *= 2)
	{
		wchar_t *pBUF = (wchar_t *) malloc( nSize * sizeof(wchar_t));
		va_list vCOPY;

		va_copy( vCOPY, args);
		int nLength = vswprintf( pBUF, nSize, pszFormat, vCOPY);
		va_end(vCOPY);
		free(pBUF);

		if( nLength >= 0 )
			return nLength;
	}

	return -1;
}

inline int vsprintf_s( char *pszBuffer, size_t nSize, const char *pszFormat, va_list args)
{
	return vsnprintf( pszBuffer, nSize, pszFormat, args);
}

inline int vswprintf_s( wchar_t *pszBuffer, size_t nSize, const wchar_t *pszFormat, va_list args)
{
	return vswprintf( pszBuffer, nSize, pszFormat, args);
}

inline int sprintf_s( char *pszBuffer, size_t nSize, const char *pszFormat, ...)
{
	va_list args;
	va_start( args, pszFormat);

	int nLength = vsnprintf( pszBuffer, nSize, pszFormat, args);
	va_end(args);

	return nLength;
}

template< size_t N > inline int sprintf_s( char (&pszBuffer)[N], const char *pszFormat, ...)
{
	va_list args;
	va_start( args, pszFormat);

	int nLength = vsnprintf( pszBuffer, N, pszFormat, args);
	va_end(args);

	return nLength;
}

template< size_t N > inline int _snprintf_s( char (&pszBuffer)[N], size_t nCount, const char *pszFormat, ...)
{
	va_list args;
	va_start( args, pszFormat);

	int nLength = vsnprintf( pszBuffer, nCount == _TRUNCATE || nCount >= N ? N : nCount + 1, pszFormat, args);
	va_end(args);

	return nLength;
}

inline int strcpy_s( char *pszDest, size_t nSize, const char *pszSource)
{
	if( !pszDest || !nSize )
		return EINVAL;

	size_t nLength = strlen(pszSource);
	if( nLength >= nSize )
	{
		pszDest[0] = 0;
		return ERANGE;
	}

	memcpy( pszDest, pszSource, nLength + 1);
	return 0;
}

template< size_t N > inline int strcpy_s( char (&pszDest)[N], const char *pszSource)
{
	return strcpy_s( pszDest, N, pszSource);
}

inline int strncpy_s( char *pszDest, size_t nSize, const char *pszSource, size_t nCount)
{
	if( !pszDest || !nSize )
		return EINVAL;

	size_t nLength = strnlen( pszSource, nCount == _TRUNCATE ? nSize - 1 : nCount);
	if( nLength >= nSize )
		nLength = nSize - 1;

	memcpy( pszDest, pszSource, nLength);
	pszDest[nLength] = 0;

	return 0;
}

template< size_t N > inline int strncpy_s( char (&pszDest)[N], const char *pszSource, size_t nCount)
{
	return strncpy_s( pszDest, N, pszSource, nCount);
}

#define _tcsncpy_s					strncpy_s
#define _tcscpy_s					strcpy_s

inline int strcat_s( char *pszDest, size_t nSize, const char *pszSource)
{
	size_t nLength = strlen(pszDest);
	return nLength < nSize ? strcpy_s( pszDest + nLength, nSize - nLength, pszSource) : ERANGE;
}

template< size_t N > inline int strcat_s( char (&pszDest)[N], const char *pszSource)
{
	return strcat_s( pszDest, N, pszSource);
}

#define _tcscat_s					strcat_s

inline char *_strrev( char *psz)
{
	for( char *b = psz, *e = psz + strlen(psz) - 1; b < e; b++, e--)
	{
		char c = *b;
		*b = *e;
		*e = c;
	}

	return psz;
}

inline wchar_t *_wcsrev( wchar_t *psz)
{
	for( wchar_t *b = psz, *e = psz + wcslen(psz) - 1; b < e; b++, e--)
	{
		wchar_t c = *b;
		*b = *e;
		*e = c;
	}

	return psz;
}

inline char *_strupr( char *psz)
{
	for( char *p = psz; *p; p++)
		*p = char(toupper((unsigned char) *p));

	return psz;
}

inline char *_strlwr( char *psz)
{
	for( char *p = psz; *p; p++)
		*p = char(tolower((unsigned char) *p));

	return psz;
}

inline int _strupr_s( char *psz, size_t)
{
	_strupr(psz);
	return 0;
}

inline int _strlwr_s( char *psz, size_t)
{
	_strlwr(psz);
	return 0;
}

template< size_t N > inline int _strupr_s( char (&psz)[N]) { _strupr(psz); return 0; }
template< size_t N > inline int _strlwr_s( char (&psz)[N]) { _strlwr(psz); return 0; }

inline int vsnprintf_s( char *pszBuffer, size_t nSize, size_t nCount, const char *pszFormat, va_list args)
{
	size_t nMax = nCount == _TRUNCATE || nCount >= nSize ? nSize : nCount + 1;
	return vsnprintf( pszBuffer, nMax, pszFormat, args);
}

// The numeric conversions only: the _s variants differ for strings.
#define fscanf_s					fscanf
#define sscanf_s					sscanf

inline int _wcsupr_s( wchar_t *psz, size_t)
{
	for( wchar_t *p = psz; *p; p++)
		*p = wchar_t(towupper(wint_t(*p)));

	return 0;
}

inline int _wcslwr_s( wchar_t *psz, size_t)
{
	for( wchar_t *p = psz; *p; p++)
		*p = wchar_t(towlower(wint_t(*p)));

	return 0;
}

// _itoa and friends: the number in the radix, lowercase digits.
template< class T > inline char *TWinCompatToA( T nValue, char *pszBuffer, int nRadix, BOOL bSigned)
{
	char vDIGIT[72];
	int nCount = 0;
	BOOL bNegative = bSigned && nValue < 0 && nRadix == 10;
	unsigned long long nAbs = bNegative ? 0ull - (unsigned long long) nValue : (unsigned long long) nValue;

	if( !bSigned || nRadix != 10 )
		nAbs = (unsigned long long) (typename std::make_unsigned<T>::type) nValue;

	do
	{
		vDIGIT[nCount++] = "0123456789abcdefghijklmnopqrstuvwxyz"[nAbs % nRadix];
		nAbs /= nRadix;
	}
	while(nAbs);

	char *p = pszBuffer;
	if(bNegative)
		*p++ = '-';

	while(nCount)
		*p++ = vDIGIT[--nCount];
	*p = 0;

	return pszBuffer;
}

inline char *_itoa( int nValue, char *pszBuffer, int nRadix) { return TWinCompatToA( nValue, pszBuffer, nRadix, TRUE); }
inline char *itoa( int nValue, char *pszBuffer, int nRadix) { return TWinCompatToA( nValue, pszBuffer, nRadix, TRUE); }
inline char *_ltoa( long nValue, char *pszBuffer, int nRadix) { return TWinCompatToA( nValue, pszBuffer, nRadix, TRUE); }
inline char *_ultoa( unsigned long nValue, char *pszBuffer, int nRadix) { return TWinCompatToA( nValue, pszBuffer, nRadix, FALSE); }
inline char *_i64toa( long long nValue, char *pszBuffer, int nRadix) { return TWinCompatToA( nValue, pszBuffer, nRadix, TRUE); }
inline char *_ui64toa( unsigned long long nValue, char *pszBuffer, int nRadix) { return TWinCompatToA( nValue, pszBuffer, nRadix, FALSE); }
inline int _itoa_s( int nValue, char *pszBuffer, size_t, int nRadix) { _itoa( nValue, pszBuffer, nRadix); return 0; }
template< size_t N > inline int _itoa_s( int nValue, char (&pszBuffer)[N], int nRadix) { _itoa( nValue, pszBuffer, nRadix); return 0; }
#define _itot						_itoa
#define _ltot						_ltoa
#endif

#define OutputDebugString(s)		fputs( (s), stderr)
#define _CrtSetDbgFlag(f)			0
#define OutputDebugStringA(s)		fputs( (s), stderr)


//////////////////////////////////////////////////////////////////////
// Files: fopen_s and the CRT's 64-bit names.

#define _fseeki64					fseeko
#define _ftelli64					ftello
#define _fileno						fileno
#define _access						access
#define _unlink						unlink
#define _getcwd						getcwd
#define _chdir						chdir
#define _rmdir						rmdir
#define _stat64						stat
#define _fsopen(path, mode, share)	fopen( (path), (mode))
#define _tfsopen					_fsopen

inline int fopen_s( FILE **ppFILE, const char *pszPath, const char *pszMode)
{
	*ppFILE = fopen( pszPath, pszMode);
	return *ppFILE ? 0 : errno;
}

inline int _mkdir( const char *pszPath)
{
	return mkdir( pszPath, 0755);
}

// The CRT's file descriptors are the POSIX ones; a HANDLE is the descriptor + 1 (see below).
#define _doserrno					errno
#define _close						close
#define _tfdopen					fdopen
#define _fdopen						fdopen
#define _open_osfhandle(h, flags)	(int(LONG_PTR(h)) - 1)


//////////////////////////////////////////////////////////////////////
// Time

inline DWORD GetTickCount()
{
	struct timespec vTIME;
	clock_gettime( CLOCK_MONOTONIC, &vTIME);

	return DWORD(vTIME.tv_sec * 1000ull + vTIME.tv_nsec / 1000000);
}

inline ULONGLONG GetTickCount64()
{
	struct timespec vTIME;
	clock_gettime( CLOCK_MONOTONIC, &vTIME);

	return ULONGLONG(vTIME.tv_sec) * 1000ull + vTIME.tv_nsec / 1000000;
}

#define timeGetTime					GetTickCount

inline BOOL QueryPerformanceFrequency( LARGE_INTEGER *pFrequency)
{
	pFrequency->QuadPart = 1000000000;
	return TRUE;
}

inline BOOL QueryPerformanceCounter( LARGE_INTEGER *pCounter)
{
	struct timespec vTIME;
	clock_gettime( CLOCK_MONOTONIC, &vTIME);

	pCounter->QuadPart = LONGLONG(vTIME.tv_sec) * 1000000000 + vTIME.tv_nsec;
	return TRUE;
}

inline void Sleep( DWORD dwMilliseconds)
{
	if(!dwMilliseconds)
	{
		sched_yield();
		return;
	}

	struct timespec vTIME = { time_t(dwMilliseconds / 1000), long(dwMilliseconds % 1000) * 1000000 };
	nanosleep( &vTIME, NULL);
}

inline void TWinCompatSystemTime( const struct tm& vTM, long nMilliseconds, SYSTEMTIME *pTime)
{
	pTime->wYear = WORD(vTM.tm_year + 1900);
	pTime->wMonth = WORD(vTM.tm_mon + 1);
	pTime->wDayOfWeek = WORD(vTM.tm_wday);
	pTime->wDay = WORD(vTM.tm_mday);
	pTime->wHour = WORD(vTM.tm_hour);
	pTime->wMinute = WORD(vTM.tm_min);
	pTime->wSecond = WORD(vTM.tm_sec);
	pTime->wMilliseconds = WORD(nMilliseconds);
}

inline void GetLocalTime( SYSTEMTIME *pTime)
{
	struct timeval vNOW;
	struct tm vTM;

	gettimeofday( &vNOW, NULL);
	localtime_r( &vNOW.tv_sec, &vTM);
	TWinCompatSystemTime( vTM, long(vNOW.tv_usec / 1000), pTime);
}

inline void GetSystemTime( SYSTEMTIME *pTime)
{
	struct timeval vNOW;
	struct tm vTM;

	gettimeofday( &vNOW, NULL);
	gmtime_r( &vNOW.tv_sec, &vTM);
	TWinCompatSystemTime( vTM, long(vNOW.tv_usec / 1000), pTime);
}

inline __time64_t _time64( __time64_t *pTime)
{
	__time64_t nTime = __time64_t(time(NULL));

	if(pTime)
		*pTime = nTime;

	return nTime;
}

inline __time64_t _mktime64( struct tm *pTM)
{
	return __time64_t(mktime(pTM));
}

inline __time64_t _mkgmtime64( struct tm *pTM)
{
	return __time64_t(timegm(pTM));
}

inline int _localtime64_s( struct tm *pTM, const __time64_t *pTime)
{
	time_t nTime = time_t(*pTime);
	return localtime_r( &nTime, pTM) ? 0 : EINVAL;
}

inline int _gmtime64_s( struct tm *pTM, const __time64_t *pTime)
{
	time_t nTime = time_t(*pTime);
	return gmtime_r( &nTime, pTM) ? 0 : EINVAL;
}

inline int localtime_s( struct tm *pTM, const time_t *pTime)
{
	return localtime_r( pTime, pTM) ? 0 : EINVAL;
}

inline int gmtime_s( struct tm *pTM, const time_t *pTime)
{
	return gmtime_r( pTime, pTM) ? 0 : EINVAL;
}

inline struct tm *_localtime64( const __time64_t *pTime)
{
	time_t nTime = time_t(*pTime);
	return localtime(&nTime);
}


//////////////////////////////////////////////////////////////////////
// Interlocked operations and threads

#ifdef __cplusplus
template< class T > inline T InterlockedIncrement( volatile T *p) { return __atomic_add_fetch( p, 1, __ATOMIC_SEQ_CST); }
template< class T > inline T InterlockedDecrement( volatile T *p) { return __atomic_sub_fetch( p, 1, __ATOMIC_SEQ_CST); }
template< class T, class V > inline T InterlockedExchange( volatile T *p, V v) { return __atomic_exchange_n( p, T(v), __ATOMIC_SEQ_CST); }
template< class T, class V > inline T InterlockedExchangeAdd( volatile T *p, V v) { return __atomic_fetch_add( p, T(v), __ATOMIC_SEQ_CST); }

template< class T, class V, class C > inline T InterlockedCompareExchange( volatile T *p, V v, C c)
{
	T nExpected = T(c);

	__atomic_compare_exchange_n( p, &nExpected, T(v), false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	return nExpected;
}
#endif

inline DWORD GetCurrentThreadId()
{
	return DWORD(pthread_self());
}

inline DWORD GetLastError()
{
	return DWORD(errno);
}

inline void SetLastError( DWORD dwError)
{
	errno = int(dwError);
}


//////////////////////////////////////////////////////////////////////
// Rectangles (user32)

inline BOOL SetRect( LPRECT p, int l, int t, int r, int b) { p->left = l; p->top = t; p->right = r; p->bottom = b; return TRUE; }
inline BOOL SetRectEmpty( LPRECT p) { return SetRect( p, 0, 0, 0, 0); }
inline BOOL CopyRect( LPRECT d, const RECT *s) { *d = *s; return TRUE; }
inline BOOL IsRectEmpty( const RECT *p) { return p->right <= p->left || p->bottom <= p->top; }
inline BOOL PtInRect( const RECT *p, POINT pt) { return pt.x >= p->left && pt.x < p->right && pt.y >= p->top && pt.y < p->bottom; }
inline BOOL EqualRect( const RECT *a, const RECT *b) { return a->left == b->left && a->top == b->top && a->right == b->right && a->bottom == b->bottom; }
inline BOOL OffsetRect( LPRECT p, int dx, int dy) { p->left += dx; p->right += dx; p->top += dy; p->bottom += dy; return TRUE; }
inline BOOL InflateRect( LPRECT p, int dx, int dy) { p->left -= dx; p->right += dx; p->top -= dy; p->bottom += dy; return TRUE; }

inline BOOL IntersectRect( LPRECT d, const RECT *a, const RECT *b)
{
	RECT r;

	r.left = a->left > b->left ? a->left : b->left;
	r.top = a->top > b->top ? a->top : b->top;
	r.right = a->right < b->right ? a->right : b->right;
	r.bottom = a->bottom < b->bottom ? a->bottom : b->bottom;

	if( IsRectEmpty(a) || IsRectEmpty(b) || r.left >= r.right || r.top >= r.bottom )
	{
		SetRectEmpty(d);
		return FALSE;
	}

	*d = r;
	return TRUE;
}

inline BOOL UnionRect( LPRECT d, const RECT *a, const RECT *b)
{
	if(IsRectEmpty(a))
	{
		if(IsRectEmpty(b))
		{
			SetRectEmpty(d);
			return FALSE;
		}

		*d = *b;
		return TRUE;
	}

	if(IsRectEmpty(b))
	{
		*d = *a;
		return TRUE;
	}

	d->left = a->left < b->left ? a->left : b->left;
	d->top = a->top < b->top ? a->top : b->top;
	d->right = a->right > b->right ? a->right : b->right;
	d->bottom = a->bottom > b->bottom ? a->bottom : b->bottom;

	return TRUE;
}

// What is left of a once b is taken away, when that is a rectangle.
inline BOOL SubtractRect( LPRECT d, const RECT *a, const RECT *b)
{
	RECT r = *a;
	RECT i;

	if( IsRectEmpty(a) )
	{
		SetRectEmpty(d);
		return FALSE;
	}

	if(IntersectRect( &i, a, b))
	{
		if( i.left <= r.left && i.right >= r.right )
		{
			if( i.top <= r.top )
				r.top = i.bottom;
			else if( i.bottom >= r.bottom )
				r.bottom = i.top;
		}
		else if( i.top <= r.top && i.bottom >= r.bottom )
		{
			if( i.left <= r.left )
				r.left = i.right;
			else if( i.right >= r.right )
				r.right = i.left;
		}
	}

	*d = r;
	if(IsRectEmpty(&r))
	{
		SetRectEmpty(d);
		return FALSE;
	}

	return TRUE;
}

inline int MulDiv( int nNumber, int nNumerator, int nDenominator)
{
	if(!nDenominator)
		return -1;

	long long nProduct = (long long) nNumber * nNumerator;
	long long nHalf = (nDenominator < 0 ? -nDenominator : nDenominator) / 2;

	// Rounded half away from zero, like Windows.
	if( (nProduct < 0) != (nDenominator < 0) )
		return int((nProduct - ((nProduct < 0) ? nHalf : -nHalf)) / nDenominator);

	return int((nProduct + ((nProduct < 0) ? -nHalf : nHalf)) / nDenominator);
}


//////////////////////////////////////////////////////////////////////
// The file API over POSIX: a HANDLE is the file descriptor + 1, so 0 stays NULL.

#define TWINCOMPAT_FD(h)			(int(LONG_PTR(h)) - 1)
#define TWINCOMPAT_HANDLE(fd)		((HANDLE) (LONG_PTR) ((fd) + 1))

inline DWORD TWinCompatError( int nErrno)
{
	switch(nErrno)
	{
	case 0			: return ERROR_SUCCESS;
	case ENOENT		: return ERROR_FILE_NOT_FOUND;
	case ENOTDIR	: return ERROR_PATH_NOT_FOUND;
	case EACCES		:
	case EPERM		: return ERROR_ACCESS_DENIED;
	case EROFS		: return ERROR_WRITE_PROTECT;
	case EMFILE		: return ERROR_TOO_MANY_OPEN_FILES;
	case ENOSPC		: return ERROR_DISK_FULL;
	case EBADF		: return ERROR_INVALID_HANDLE;
	case ENAMETOOLONG	: return ERROR_INVALID_NAME;
	}

	return DWORD(nErrno);
}

#ifdef __cplusplus
// The real path of a game path (TFile.h): '/' separators, the case on disk.
extern const char *TWinCompatPath( const char *pszPath);

inline HANDLE CreateFile( LPCSTR pszPath, DWORD dwAccess, DWORD, LPSECURITY_ATTRIBUTES, DWORD dwCreate, DWORD, HANDLE)
{
	int nFlags = 0;

	if( (dwAccess & GENERIC_READ) && (dwAccess & GENERIC_WRITE) )
		nFlags = O_RDWR;
	else if( dwAccess & GENERIC_WRITE )
		nFlags = O_WRONLY;
	else
		nFlags = O_RDONLY;

	switch(dwCreate)
	{
	case CREATE_ALWAYS		: nFlags |= O_CREAT|O_TRUNC; break;
	case OPEN_ALWAYS		: nFlags |= O_CREAT; break;
	case 1					: nFlags |= O_CREAT|O_EXCL; break;		// CREATE_NEW
	case 5					: nFlags |= O_TRUNC; break;				// TRUNCATE_EXISTING
	}

	int nFD = open( TWinCompatPath(pszPath), nFlags|O_CLOEXEC, 0644);
	if( nFD < 0 )
	{
		errno = int(TWinCompatError(errno));
		return INVALID_HANDLE_VALUE;
	}

	return TWINCOMPAT_HANDLE(nFD);
}

#define CreateFileA					CreateFile

inline BOOL CloseHandle( HANDLE hFile)
{
	return hFile && hFile != INVALID_HANDLE_VALUE && !close(TWINCOMPAT_FD(hFile));
}

inline BOOL ReadFile( HANDLE hFile, LPVOID pBuffer, DWORD dwCount, LPDWORD pRead, void *)
{
	ssize_t nRead = read( TWINCOMPAT_FD(hFile), pBuffer, dwCount);

	if( nRead < 0 )
	{
		errno = int(TWinCompatError(errno));
		return FALSE;
	}

	if(pRead)
		*pRead = DWORD(nRead);

	return TRUE;
}

inline BOOL WriteFile( HANDLE hFile, LPCVOID pBuffer, DWORD dwCount, LPDWORD pWritten, void *)
{
	ssize_t nWritten = write( TWINCOMPAT_FD(hFile), pBuffer, dwCount);

	if( nWritten < 0 )
	{
		errno = int(TWinCompatError(errno));
		return FALSE;
	}

	if(pWritten)
		*pWritten = DWORD(nWritten);

	return TRUE;
}

inline BOOL SetFilePointerEx( HANDLE hFile, LARGE_INTEGER vMove, PLARGE_INTEGER pNew, DWORD dwMethod)
{
	off_t nPos = lseek( TWINCOMPAT_FD(hFile), off_t(vMove.QuadPart), dwMethod == FILE_BEGIN ? SEEK_SET : dwMethod == FILE_CURRENT ? SEEK_CUR : SEEK_END);

	if( nPos < 0 )
	{
		errno = int(TWinCompatError(errno));
		return FALSE;
	}

	if(pNew)
		pNew->QuadPart = LONGLONG(nPos);

	return TRUE;
}

inline DWORD SetFilePointer( HANDLE hFile, LONG nMove, PLONG pMoveHigh, DWORD dwMethod)
{
	LARGE_INTEGER vMOVE;
	LARGE_INTEGER vNEW;

	vMOVE.QuadPart = pMoveHigh ? (LONGLONG(*pMoveHigh) << 32) | DWORD(nMove) : LONGLONG(nMove);
	if(!SetFilePointerEx( hFile, vMOVE, &vNEW, dwMethod))
		return INVALID_SET_FILE_POINTER;

	if(pMoveHigh)
		*pMoveHigh = vNEW.HighPart;

	return vNEW.LowPart;
}

inline BOOL GetFileSizeEx( HANDLE hFile, PLARGE_INTEGER pSize)
{
	struct stat vSTAT;

	if(fstat( TWINCOMPAT_FD(hFile), &vSTAT))
		return FALSE;

	pSize->QuadPart = LONGLONG(vSTAT.st_size);
	return TRUE;
}

inline DWORD GetFileSize( HANDLE hFile, LPDWORD pHigh)
{
	LARGE_INTEGER vSIZE;

	if(!GetFileSizeEx( hFile, &vSIZE))
		return 0xFFFFFFFF;

	if(pHigh)
		*pHigh = DWORD(vSIZE.HighPart);

	return vSIZE.LowPart;
}

inline BOOL SetEndOfFile( HANDLE hFile)
{
	off_t nPos = lseek( TWINCOMPAT_FD(hFile), 0, SEEK_CUR);
	return nPos >= 0 && !ftruncate( TWINCOMPAT_FD(hFile), nPos);
}

inline BOOL FlushFileBuffers( HANDLE hFile)
{
	return !fsync(TWINCOMPAT_FD(hFile));
}

inline BOOL DeleteFile( LPCSTR pszPath) { return !unlink(TWinCompatPath(pszPath)); }
inline BOOL MoveFile( LPCSTR pszOld, LPCSTR pszNew) { return !rename( TWinCompatPath(pszOld), TWinCompatPath(pszNew)); }
inline BOOL CreateDirectory( LPCSTR pszPath, LPSECURITY_ATTRIBUTES) { return !mkdir( TWinCompatPath(pszPath), 0755) || errno == EEXIST; }
#define DeleteFileA					DeleteFile
#define MoveFileA					MoveFile
#define CreateDirectoryA			CreateDirectory

inline void TWinCompatFileTime( time_t nTime, FILETIME *pTime)
{
	// 100 ns since 1601.
	ULONGLONG nValue = (ULONGLONG(nTime) + 11644473600ull) * 10000000ull;

	pTime->dwLowDateTime = DWORD(nValue);
	pTime->dwHighDateTime = DWORD(nValue >> 32);
}

inline DWORD GetFileAttributes( LPCSTR pszPath)
{
	struct stat vSTAT;

	if(stat( TWinCompatPath(pszPath), &vSTAT))
		return INVALID_FILE_ATTRIBUTES;

	return S_ISDIR(vSTAT.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
}

#define GetFileAttributesA			GetFileAttributes

inline BOOL GetFileAttributesEx( LPCSTR pszPath, GET_FILEEX_INFO_LEVELS, LPVOID pInfo)
{
	WIN32_FILE_ATTRIBUTE_DATA *pDATA = (WIN32_FILE_ATTRIBUTE_DATA *) pInfo;
	struct stat vSTAT;

	if(stat( TWinCompatPath(pszPath), &vSTAT))
		return FALSE;

	memset( pDATA, 0, sizeof(WIN32_FILE_ATTRIBUTE_DATA));
	pDATA->dwFileAttributes = S_ISDIR(vSTAT.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
	pDATA->nFileSizeHigh = DWORD(ULONGLONG(vSTAT.st_size) >> 32);
	pDATA->nFileSizeLow = DWORD(vSTAT.st_size);
	TWinCompatFileTime( vSTAT.st_mtime, &pDATA->ftLastWriteTime);
	TWinCompatFileTime( vSTAT.st_ctime, &pDATA->ftCreationTime);
	TWinCompatFileTime( vSTAT.st_atime, &pDATA->ftLastAccessTime);

	return TRUE;
}

#define PathFileExists(p)			(GetFileAttributes(p) != INVALID_FILE_ATTRIBUTES)

// FindFirstFile: the folder's entries matching the pattern (case-insensitively, as on Windows).
struct TWINCOMPAT_FIND
{
	DIR *m_pDIR;
	char m_szFOLDER[4096];
	char m_szPATTERN[256];
};

inline BOOL FindNextFile( HANDLE hFind, LPWIN32_FIND_DATA pData)
{
	TWINCOMPAT_FIND *pFIND = (TWINCOMPAT_FIND *) hFind;

	while( dirent *pENTRY = readdir(pFIND->m_pDIR) )
	{
		if(fnmatch( pFIND->m_szPATTERN, pENTRY->d_name, FNM_CASEFOLD))
			continue;

		char szPATH[4352];
		struct stat vSTAT;

		snprintf( szPATH, sizeof(szPATH), "%s/%s", pFIND->m_szFOLDER, pENTRY->d_name);
		if(stat( szPATH, &vSTAT))
			continue;

		memset( pData, 0, sizeof(WIN32_FIND_DATA));
		pData->dwFileAttributes = S_ISDIR(vSTAT.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
		pData->nFileSizeHigh = DWORD(ULONGLONG(vSTAT.st_size) >> 32);
		pData->nFileSizeLow = DWORD(vSTAT.st_size);
		TWinCompatFileTime( vSTAT.st_mtime, &pData->ftLastWriteTime);
		strncpy( pData->cFileName, pENTRY->d_name, sizeof(pData->cFileName) - 1);

		return TRUE;
	}

	return FALSE;
}

inline HANDLE FindFirstFile( LPCSTR pszPattern, LPWIN32_FIND_DATA pData)
{
	const char *pszPath = TWinCompatPath(pszPattern);
	const char *pszSlash = strrchr( pszPath, '/');
	TWINCOMPAT_FIND *pFIND = new TWINCOMPAT_FIND();

	if(pszSlash)
	{
		snprintf( pFIND->m_szFOLDER, sizeof(pFIND->m_szFOLDER), "%.*s", int(pszSlash - pszPath), pszPath);
		snprintf( pFIND->m_szPATTERN, sizeof(pFIND->m_szPATTERN), "%s", pszSlash + 1);
	}
	else
	{
		strcpy( pFIND->m_szFOLDER, ".");
		snprintf( pFIND->m_szPATTERN, sizeof(pFIND->m_szPATTERN), "%s", pszPath);
	}

	if(!pFIND->m_szFOLDER[0])
		strcpy( pFIND->m_szFOLDER, "/");

	// "*.*" is every name on Windows, dots or not.
	if(!strcmp( pFIND->m_szPATTERN, "*.*"))
		strcpy( pFIND->m_szPATTERN, "*");

	pFIND->m_pDIR = opendir(pFIND->m_szFOLDER);
	if( !pFIND->m_pDIR || !FindNextFile( (HANDLE) pFIND, pData) )
	{
		if(pFIND->m_pDIR)
			closedir(pFIND->m_pDIR);

		delete pFIND;
		return INVALID_HANDLE_VALUE;
	}

	return (HANDLE) pFIND;
}

inline BOOL FindClose( HANDLE hFind)
{
	TWINCOMPAT_FIND *pFIND = (TWINCOMPAT_FIND *) hFind;

	closedir(pFIND->m_pDIR);
	delete pFIND;

	return TRUE;
}

#define FindFirstFileA				FindFirstFile
#define FindNextFileA				FindNextFile

inline DWORD GetFullPathName( LPCSTR pszPath, DWORD dwSize, LPSTR pszFull, LPSTR *ppszName)
{
	char szFULL[4096];
	const char *pszReal = TWinCompatPath(pszPath);

	if( pszReal[0] == '/' )
		snprintf( szFULL, sizeof(szFULL), "%s", pszReal);
	else
	{
		char szCWD[4096];

		if(!getcwd( szCWD, sizeof(szCWD)))
			return 0;

		snprintf( szFULL, sizeof(szFULL), "%s/%s", szCWD, pszReal);
	}

	size_t nLength = strlen(szFULL);
	if( nLength >= dwSize )
		return DWORD(nLength + 1);

	strcpy( pszFull, szFULL);
	if(ppszName)
	{
		char *pszSlash = strrchr( pszFull, '/');
		*ppszName = pszSlash ? pszSlash + 1 : pszFull;
	}

	return DWORD(nLength);
}

inline DWORD GetCurrentDirectory( DWORD dwSize, LPSTR pszPath)
{
	return getcwd( pszPath, dwSize) ? DWORD(strlen(pszPath)) : 0;
}

inline BOOL SetCurrentDirectory( LPCSTR pszPath)
{
	return !chdir(TWinCompatPath(pszPath));
}

// The executable's path.
inline DWORD GetModuleFileName( HMODULE, LPSTR pszPath, DWORD dwSize)
{
	ssize_t nLength = readlink( "/proc/self/exe", pszPath, dwSize ? dwSize - 1 : 0);

	if( nLength < 0 )
		nLength = 0;

	if(dwSize)
		pszPath[nLength] = 0;

	return DWORD(nLength);
}

#define GetModuleFileNameA			GetModuleFileName

inline DWORD GetTempPath( DWORD dwSize, LPSTR pszPath)
{
	const char *pszTEMP = getenv("TMPDIR");
	int nLength = snprintf( pszPath, dwSize, "%s/", pszTEMP && *pszTEMP ? pszTEMP : "/tmp");

	return DWORD(nLength);
}

#define GetTempPathA				GetTempPath
#endif


//////////////////////////////////////////////////////////////////////
// System

inline BOOL GlobalMemoryStatusEx( LPMEMORYSTATUSEX pStatus)
{
	long nPages = sysconf(_SC_PHYS_PAGES);
	long nAvail = sysconf(_SC_AVPHYS_PAGES);
	long nPage = sysconf(_SC_PAGE_SIZE);

	pStatus->ullTotalPhys = ULONGLONG(nPages) * nPage;
	pStatus->ullAvailPhys = ULONGLONG(nAvail) * nPage;

	return TRUE;
}

// The executable's resources: a table its build generates from the .rc file's (TClient/CMakeLists.txt).
struct TWINCOMPAT_RESOURCE
{
	const char *m_pszType;
	UINT m_nID;
	const BYTE *m_pDATA;
	DWORD m_dwSIZE;
};

extern const TWINCOMPAT_RESOURCE g_vTWINCOMPAT_RESOURCE[];

#define MAKEINTRESOURCE(i)			((LPSTR) (ULONG_PTR) (WORD) (i))
#define MAKEINTRESOURCEA			MAKEINTRESOURCE
#define IS_INTRESOURCE(p)			((((ULONG_PTR) (p)) >> 16) == 0)

inline HRSRC FindResource( HMODULE, LPCSTR pszName, LPCSTR pszType)
{
	if( !IS_INTRESOURCE(pszName) || IS_INTRESOURCE(pszType) )
		return NULL;

	for( const TWINCOMPAT_RESOURCE *p = g_vTWINCOMPAT_RESOURCE; p->m_pszType; p++)
		if( p->m_nID == UINT(ULONG_PTR(pszName)) && !strcasecmp( p->m_pszType, pszType) )
			return (HRSRC) p;

	return NULL;
}

#define FindResourceA				FindResource

inline HGLOBAL LoadResource( HMODULE, HRSRC hResource) { return (HGLOBAL) hResource; }
inline DWORD SizeofResource( HMODULE, HRSRC hResource) { return hResource ? ((const TWINCOMPAT_RESOURCE *) hResource)->m_dwSIZE : 0; }
inline LPVOID LockResource( HGLOBAL hResource) { return hResource ? (LPVOID) ((const TWINCOMPAT_RESOURCE *) hResource)->m_pDATA : NULL; }

// No string table in the executable on these systems, nor GDI objects: fonts are their LOGFONT.
inline HMODULE GetModuleHandle( LPCSTR) { return NULL; }
inline HKL GetKeyboardLayout( DWORD) { return NULL; }
inline int LoadStringA( HINSTANCE, UINT, LPSTR pszBuffer, int nSize) { if( nSize > 0 ) pszBuffer[0] = 0; return 0; }
inline int LoadStringW( HINSTANCE, UINT, LPWSTR pszBuffer, int nSize) { if( nSize > 0 ) pszBuffer[0] = 0; return 0; }
inline HFONT CreateFontIndirect( const LOGFONT *) { return (HFONT) (LONG_PTR) 1; }
inline BOOL DeleteObject( HGDIOBJ) { return TRUE; }
inline int GetObject( HGDIOBJ, int, LPVOID) { return 0; }

inline void GetSystemInfo( LPSYSTEM_INFO pInfo)
{
	memset( pInfo, 0, sizeof(SYSTEM_INFO));
	pInfo->dwNumberOfProcessors = DWORD(sysconf(_SC_NPROCESSORS_ONLN));
	pInfo->dwPageSize = DWORD(sysconf(_SC_PAGE_SIZE));
}
