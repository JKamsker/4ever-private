#pragma once

#ifdef _WIN32


class CTMiniDump
{
public:
	CTMiniDump();
	~CTMiniDump();

	static void SetOption(_MINIDUMP_TYPE nDumpType);
	static void SetUserInfo(const char *pszTemp);
	static void SetLogID(const char *pszID);
	static void SetCharID(const char *pszID);
	static void SetSocket(SOCKET sock);

	static void SetHWND(HWND hWnd);
	static void SnapMemoryDump();

	static void LogPlayerInfo(CString& outResult);
	static BOOL GetExceptionDetail(_EXCEPTION_POINTERS* pExceptionInfo, CString& outResult, CString& outFirstStack) ;

	static LONG WINAPI TopLevelExceptionFilter(_EXCEPTION_POINTERS *pException);

private:
	static char* GetFaultReason(_EXCEPTION_POINTERS* pExPtrs);
};

extern CTMiniDump theTMiniDump;

#else

// The crash dump is a Windows one: elsewhere the client only keeps the calls.
class CTMiniDump
{
public:
	static void SetOption( int nDumpType) {}
	static void SetUserInfo( const char *pszTemp) {}
	static void SetLogID( const char *pszID) {}
	static void SetCharID( const char *pszID) {}
	static void SetSocket( SOCKET sock) {}

	static void SetHWND( HWND hWnd) {}
	static void SnapMemoryDump() {}
};

#define MiniDumpNormal				0
#endif
