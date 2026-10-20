// TachyonApp.h: interface for the CTachyonApp class.
//
//////////////////////////////////////////////////////////////////////

#if !defined __TACHYONAPP_H
#define __TACHYONAPP_H


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// The application, without MFC's CWinApp: the client's WinMain calls Main, which
// runs InitInstance then Run, like MFC did. There is one application object.
class CTachyonApp
{
public:
	CTachyonApp();
	virtual ~CTachyonApp();

	static int Main( HINSTANCE hInstance, LPTSTR lpCmdLine, int nCmdShow);
	static CTachyonApp *GetApp() { return m_pApp; }

// Overrides
public:
	virtual BOOL InitInstance() { return TRUE; }
	virtual int ExitInstance() { return 0; }
	virtual int Run();

	virtual void LoadStdProfileSettings();
	virtual void SaveStdProfileSettings();
	virtual BYTE MainProc();

	// Settings are stored in a config.ini file (see CGameConfig), not the Windows registry.
	// These shadow the CWinApp profile methods so every existing GetProfile*/WriteProfile*
	// call site is transparently redirected to the file-based loader.
	UINT    GetProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nDefault);
	CString GetProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault = NULL);
	BOOL    WriteProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nValue);
	BOOL    WriteProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszValue);

#ifdef _WIN32
	HICON LoadIcon( UINT nIDResource) const { return ::LoadIcon( m_hInstance, MAKEINTRESOURCE(nIDResource)); }
#else
	HICON LoadIcon( UINT nIDResource) const { return NULL; }
#endif

public:
	HINSTANCE m_hInstance;
	LPTSTR m_lpCmdLine;
	int m_nCmdShow;
	CTWindow *m_pMainWnd;

protected:
	CTachyonWnd *m_pTachyonWnd;

	HACCEL m_hAccel;
	DWORD m_dwSLEEP;

private:
	static CTachyonApp *m_pApp;
};

#endif // !defined __TACHYONAPP_H
