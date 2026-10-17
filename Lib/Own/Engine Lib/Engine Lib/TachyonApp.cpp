// TachyonApp.cpp: implementation of the CTachyonApp class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "GameConfig.h"


/////////////////////////////////////////////////////////////////////////////
// CTachyonApp

// ---------------------------------------------------------------------------
// Profile-method forwarders: redirect all settings I/O to the config.ini file
// via CGameConfig instead of the Windows registry.
// ---------------------------------------------------------------------------
UINT CTachyonApp::GetProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nDefault)
{
	return (UINT)g_Config.GetInt(lpszSection, lpszEntry, nDefault);
}

CString CTachyonApp::GetProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault)
{
	return g_Config.GetString(lpszSection, lpszEntry, lpszDefault);
}

BOOL CTachyonApp::WriteProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nValue)
{
	g_Config.SetInt(lpszSection, lpszEntry, nValue);
	return TRUE;
}

BOOL CTachyonApp::WriteProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszValue)
{
	g_Config.SetString(lpszSection, lpszEntry, lpszValue);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CTachyonApp construction

CTachyonApp *CTachyonApp::m_pApp = NULL;

CTachyonApp::CTachyonApp()
{
	m_pTachyonWnd = NULL;
	m_pMainWnd = NULL;
	m_dwSLEEP = 0;
	m_hAccel = NULL;

	m_hInstance = NULL;
	m_lpCmdLine = NULL;
	m_nCmdShow = SW_SHOWNORMAL;

	m_pApp = this;
}

int CTachyonApp::Main( HINSTANCE hInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	if(!m_pApp)
		return -1;

	m_pApp->m_hInstance = hInstance;
	m_pApp->m_lpCmdLine = lpCmdLine;
	m_pApp->m_nCmdShow = nCmdShow;

	if(!CTWindow::InitPlatform())
		return -1;

	// Like MFC: a failed InitInstance still gets its ExitInstance, and Run returns the exit code.
	int nResult = m_pApp->InitInstance() ? m_pApp->Run() : m_pApp->ExitInstance();
	CTWindow::QuitPlatform();

	return nResult;
}

CTachyonApp::~CTachyonApp()
{
}

void CTachyonApp::LoadStdProfileSettings()
{
	if(!m_pTachyonWnd)
		return;

	CString strValue;
	DWORD dwValue;

	strValue = GetProfileString(
		_T("Settings"),
		_T("WindowedMode"),
		_T("NOT FOUND"));

	if( strValue == _T("NOT FOUND") )
	{
		WriteProfileString(
			_T("Settings"),
			_T("WindowedMode"),
#ifdef _DEBUG
			_T("TRUE"));
		m_pTachyonWnd->m_Device.m_option.m_bWindowedMode = TRUE;
	}
	else
		m_pTachyonWnd->m_Device.m_option.m_bWindowedMode = TRUE;
#else
			m_pTachyonWnd->m_Device.m_option.m_bWindowedMode ? _T("TRUE") : _T("FALSE"));
	}
	else
		m_pTachyonWnd->m_Device.m_option.m_bWindowedMode = strValue == _T("TRUE");
#endif

	strValue = GetProfileString(
		_T("Settings"),
		_T("UseShader"),
		_T("NOT FOUND"));

	if( strValue == _T("NOT FOUND") )
	{
		WriteProfileString(
			_T("Settings"),
			_T("UseShader"),
			m_pTachyonWnd->m_Device.m_option.m_bUseSHADER ? _T("TRUE") : _T("FALSE"));
	}
	else
		m_pTachyonWnd->m_Device.m_option.m_bUseSHADER = strValue == _T("TRUE");

	// BGFX (default) or D3D9: the backend of the render layer. Builds without
	// TCLIENT_BGFX (MSBuild) always draw with D3D9.
	m_pTachyonWnd->m_Device.m_option.m_bBGFX = GetProfileString(
		_T("Settings"),
		_T("Renderer"),
		_T("BGFX")).CompareNoCase(_T("BGFX")) == 0;

	dwValue = GetProfileInt(
		_T("Settings"),
		_T("SwapEffect"), 0);

	if(!dwValue)
	{
		WriteProfileInt(
			_T("Settings"),
			_T("SwapEffect"),
			m_pTachyonWnd->m_Device.m_option.m_dwSwapEffect);
	}
	else
		m_pTachyonWnd->m_Device.m_option.m_dwSwapEffect = dwValue;

	dwValue = GetProfileInt(
		_T("Settings"),
		_T("PresentationInterval"), 0);

	if(!dwValue)
	{
		WriteProfileInt(
			_T("Settings"),
			_T("PresentationInterval"),
			m_pTachyonWnd->m_Device.m_option.m_dwPresentationInterval);
	}
	else
		m_pTachyonWnd->m_Device.m_option.m_dwPresentationInterval = dwValue;

	dwValue = GetProfileInt(
		_T("Settings"),
		_T("Behavior"), 0);

	if(!dwValue)
	{
		WriteProfileInt(
			_T("Settings"),
			_T("Behavior"),
			m_pTachyonWnd->m_Device.m_option.m_dwBehavior);
	}
	else
		m_pTachyonWnd->m_Device.m_option.m_dwBehavior = dwValue;

	dwValue = GetProfileInt(
		_T("Settings"),
		_T("ScreenX"), 0);

	if(!dwValue)
	{
		// No resolution in config.ini -> adapt to the primary display on first run.
		m_pTachyonWnd->m_Device.m_option.m_dwScreenX = GetSystemMetrics(SM_CXSCREEN);
		WriteProfileInt(
			_T("Settings"),
			_T("ScreenX"),
			m_pTachyonWnd->m_Device.m_option.m_dwScreenX);
	}
	else
		m_pTachyonWnd->m_Device.m_option.m_dwScreenX = dwValue;

	dwValue = GetProfileInt(
		_T("Settings"),
		_T("ScreenY"), 0);

	if(!dwValue)
	{
		// No resolution in config.ini -> adapt to the primary display on first run.
		m_pTachyonWnd->m_Device.m_option.m_dwScreenY = GetSystemMetrics(SM_CYSCREEN);
		WriteProfileInt(
			_T("Settings"),
			_T("ScreenY"),
			m_pTachyonWnd->m_Device.m_option.m_dwScreenY);
	}
	else
		m_pTachyonWnd->m_Device.m_option.m_dwScreenY = dwValue;

	dwValue = GetProfileInt(
		_T("Settings"),
		_T("TextureDetail"), TEXTURE_DETAIL_COUNT);

	if(dwValue == TEXTURE_DETAIL_COUNT)
	{
		WriteProfileInt(_T("Settings"),
			_T("TextureDetail"),
			m_pTachyonWnd->m_Device.m_option.m_nTextureDetail);
	}
	else
	{
		m_pTachyonWnd->m_Device.m_option.m_nTextureDetail = (TEXTURE_DETAIL_LEVEL)dwValue;
	}

	if (!m_pTachyonWnd->m_Device.m_option.m_bWindowedMode)
	{
		m_pTachyonWnd->m_Device.m_option.m_bWindowedMode = TRUE;

		m_pTachyonWnd->m_Device.m_option.m_dwScreenX = GetSystemMetrics(SM_CXSCREEN);

		m_pTachyonWnd->m_Device.m_option.m_dwScreenY = GetSystemMetrics(SM_CYSCREEN);
	}

}

void CTachyonApp::SaveStdProfileSettings()
{
	WriteProfileString(
		_T("Settings"),
		_T("WindowedMode"),
		m_pTachyonWnd->m_Device.m_option.m_bWindowedMode ? _T("TRUE") : _T("FALSE"));

	WriteProfileString(
		_T("Settings"),
		_T("UseShader"),
		m_pTachyonWnd->m_Device.m_option.m_bUseSHADER ? _T("TRUE") : _T("FALSE"));

	WriteProfileInt(
		_T("Settings"),
		_T("SwapEffect"),
		m_pTachyonWnd->m_Device.m_option.m_dwSwapEffect);

	WriteProfileInt(
		_T("Settings"),
		_T("PresentationInterval"),
		m_pTachyonWnd->m_Device.m_option.m_dwPresentationInterval);

	WriteProfileInt(
		_T("Settings"),
		_T("Behavior"),
		m_pTachyonWnd->m_Device.m_option.m_dwBehavior);

	WriteProfileInt(
		_T("Settings"),
		_T("ScreenX"),
		m_pTachyonWnd->m_Device.m_option.m_dwScreenX);

	WriteProfileInt(
		_T("Settings"),
		_T("ScreenY"),
		m_pTachyonWnd->m_Device.m_option.m_dwScreenY);

	WriteProfileInt(
		_T("Settings"),
		_T("TextureDetail"),
		m_pTachyonWnd->m_Device.m_option.m_nTextureDetail);
}

int CTachyonApp::Run()
{
	while(m_pTachyonWnd)
	{
		if(m_dwSLEEP)
		{
			Sleep(m_dwSLEEP);
			m_dwSLEEP = 0;
		}

		if(!MainProc())
			break;
	}

	return FALSE;
}

BYTE CTachyonApp::MainProc()
{
	CTachyonSession::PollAll();

	BYTE bEVENT = CTWindow::PeekEvent();
	if( bEVENT == TWINDOW_QUIT )
	{
		SaveStdProfileSettings();
		m_pTachyonWnd->DestroyWindow();

		delete m_pTachyonWnd;
		m_pTachyonWnd = NULL;

		return FALSE;
	}

	// One event per pass; a frame when none is waiting.
	if( bEVENT == TWINDOW_NOEVENT && m_pTachyonWnd->GetSafeHwnd() )
	{
		if(!m_pTachyonWnd->m_bActivate)
		{
			// Wake up for an event or after a short delay, so the sessions are still polled.
			CTWindow::WaitEvent(TSESSION_IDLE_WAIT);

#ifndef TEST_MODE
			if(!m_pTachyonWnd->IsThis(::GetForegroundWindow()))
				m_dwSLEEP = 100;
#endif
		}
		else if(!m_pTachyonWnd->m_bOnMovie)
		{
#ifndef TEST_MODE
			if(!m_pTachyonWnd->IsThis(::GetForegroundWindow()))
				m_dwSLEEP = 10;
#endif

			m_pTachyonWnd->Render();

#ifdef TEST_MODE
			CTProfileManager::Increment_Frame_Counter();
#endif
		}
	}

	while(!m_pTachyonWnd->m_vCOMMAND.empty())
	{
		m_pTachyonWnd->OnCommand(*(m_pTachyonWnd->m_vCOMMAND.begin()));
		m_pTachyonWnd->m_vCOMMAND.erase(m_pTachyonWnd->m_vCOMMAND.begin());
	}

	return TRUE;
}
