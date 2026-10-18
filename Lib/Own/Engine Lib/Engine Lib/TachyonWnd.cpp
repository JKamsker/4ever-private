// TachyonWnd.cpp: implementation of the CTachyonWnd class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DWORD CTachyonWnd::m_dwPing = 0;

CTachyonWnd::CTachyonWnd()
{
	m_bActivate = FALSE;
	m_bOnMovie = FALSE;

	m_pMainFrame = NULL;
	m_pTParser = NULL;

	m_vCOMMAND.clear();

	m_bDirectLogin = FALSE;
	m_bSiteCode = 0xFF;

}

CTachyonWnd::~CTachyonWnd()
{
	if(m_pTParser)
	{
		delete m_pTParser;
		m_pTParser = NULL;
	}

	FinishTachyonComponent();
}

BOOL CALLBACK CTachyonWnd::__Present( HWND hWnd, LPARAM lParam)
{
	CTachyonWnd *pMAIN = (CTachyonWnd *) lParam;
	CRect rect;

	// Visible child windows without windows of their own (the HTML views).
	if( !pMAIN || !::IsWindowVisible(hWnd) || ::GetWindow( hWnd, GW_CHILD) )
		return TRUE;

	::GetClientRect( hWnd, &rect);
	::MapWindowPoints( hWnd, pMAIN->m_hWnd, (LPPOINT) &rect, 2);

	if(FAILED(pMAIN->m_Device.m_pDevice->Present( &rect, NULL, hWnd, NULL)))
		return FALSE;

	return TRUE;
}

BOOL CTachyonWnd::Create()
{
	CSize vSCREEN = CTWindow::GetScreenSize();
	BOOL bCaption = FALSE;
	CString strTitle;

	// A window of another size than the screen has a caption; full screen stays on top.
	if( m_Device.m_option.m_bWindowedMode &&
		m_Device.m_option.m_dwScreenX != DWORD(vSCREEN.cx) &&
		m_Device.m_option.m_dwScreenY != DWORD(vSCREEN.cy) )
		bCaption = TRUE;

	strTitle.LoadString(AFX_IDS_APP_TITLE);

	BOOL bResult = CTWindow::Create(
		strTitle,
		m_Device.m_option.m_dwScreenX,
		m_Device.m_option.m_dwScreenY,
		bCaption,
		!m_Device.m_option.m_bWindowedMode);

#ifdef USE_CONSOLE
	CTConsole* pConsole = CTConsole::GetInstance();
	pConsole->Create("Output", true);
#endif

	if(bResult)
		ShowWindow(SW_SHOW);

	return bResult;
}

void CTachyonWnd::OnDestroy()
{
	if(m_pMainFrame)
	{
		delete m_pMainFrame;
		m_pMainFrame = NULL;
	}

	CTachyonMesh::ReleaseGlobalVB();
	CTachyonSFX::ReleaseCommon();
	CSFX::ReleaseTEXVB();

	m_bActivate = FALSE;
	m_Font.DeleteObject();
	m_FontFPS.Release();
	m_Device.ReleaseDevice();
	m_vCOMMAND.clear();

	CTWindow::OnDestroy();
}

// Like MFC, the end of the main window ends the application.
void CTachyonWnd::OnNcDestroy()
{
	CTWindow::OnNcDestroy();

	if( CTachyonApp::GetApp() && CTachyonApp::GetApp()->m_pMainWnd == this )
		CTWindow::PostQuit();
}

BOOL CTachyonWnd::InitDevices()
{
	if(!m_Device.InitDevices(this))
		return FALSE;
	CD3DLight::SetDevice(m_Device.m_pDevice);

	D3DMATERIAL9 mtrl;
	ZeroMemory( &mtrl, sizeof(D3DMATERIAL9));
	mtrl.Diffuse.r = mtrl.Ambient.r = 1.0f;
	mtrl.Diffuse.g = mtrl.Ambient.g = 1.0f;
	mtrl.Diffuse.b = mtrl.Ambient.b = 1.0f;
	mtrl.Diffuse.a = mtrl.Ambient.a = 1.0f;
	m_Device.m_pDevice->SetMaterial(&mtrl);
	m_Device.m_pDevice->SetVertexShader(NULL);

	m_Font.CreateFont(
		14, 0, 0, 0, 0,
		FALSE,
		FALSE,
		FALSE,
		DEFAULT_CHARSET,
		OUT_CHARACTER_PRECIS,
		CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY,
		FF_DONTCARE,
#if defined(GERMANY) || defined(ENGLISH) || defined(JAPAN)
		_T("Arial"));
#else
		_T("2002L"));
#endif

	m_FontFPS.SetFont(&m_Font);
	m_bActivate = TRUE;

	CTachyonInput::SetWindow(this);
	CTachyonObject::m_pMedia = &CTachyonRes::m_MEDIA;
	CTachyonMesh::InitGlobalVB();

	CTachyonRes::m_MEDIA.InitMEDIA();
	CTachyonRes::m_pDEVICE = &m_Device;
	CTachyonSFX::InitCommon();
	CSFX::InitTEXVB();

	return TRUE;
}

BOOL CTachyonWnd::AdjustWindowSize(INT nWidth, INT nHeight)
{
	CSize vSCREEN = CTWindow::GetScreenSize();

	SetCaption( m_Device.m_option.m_bWindowedMode &&
		m_Device.m_option.m_dwScreenX != DWORD(vSCREEN.cx) &&
		m_Device.m_option.m_dwScreenY != DWORD(vSCREEN.cy));
	SetFrameSize( nWidth, nHeight);

	return TRUE;
}

BOOL CTachyonWnd::Present()
{
	if(FAILED(m_Device.m_pDevice->Present( NULL, NULL, NULL, NULL)))
		return FALSE;

	if( m_Device.m_option.m_bWindowedMode && ::GetWindow( m_hWnd, GW_CHILD) &&
		!EnumChildWindows( GetSafeHwnd(), __Present, (LPARAM) this))
		return FALSE;

	return TRUE;
}

BOOL CTachyonWnd::ResetDevices()
{
	while(m_Device.m_pDevice)
		if(m_Device.Reset())
			return TRUE;

	return FALSE;
}

void CTachyonWnd::ProcessHotCommand( int nID)
{
}

void CTachyonWnd::Render()
{
}

void CTachyonWnd::DrawFPS(DWORD dwPing, int nPosX, int nPosY)
{
	static DWORD dwPrevTick = ::GetTickCount();
	static DWORD dwCount = 0;
	static CString strFPS(_T(""));

	DWORD dwTick = ::GetTickCount();

	if( dwTick - dwPrevTick > 1000 )
	{
		while( dwTick - dwPrevTick > 1000 )
			dwPrevTick += 1000;

		strFPS.Format( _T("[FPS : %d]"), dwCount);
		dwCount = 0;
	}
	else
		dwCount++;

	m_dwPing = dwPing;

	CString strPing;
	strPing.Format("\n[Ping : %dms]", dwPing);

	CString strText;
	CString strParticle;
	strParticle.Format( "\n[Particle : %d]", CTachyonSFX::m_dwParticleNum );

	strText = strFPS + strPing;

	m_FontFPS.TextOut(
		m_Device.m_pDevice,
		strText,
		nPosX,
		nPosY);
}

void CTachyonWnd::OnCommand( TCOMMAND cmd)
{
}

void CTachyonWnd::OnSessionMsg( CTachyonSession *pSession, CPacket *pPacket)
{
}

void CTachyonWnd::OnConnect( CTachyonSession *pSession, int nErrorCode)
{
}

void CTachyonWnd::OnClose( CTachyonSession *pSession, int nErrorCode)
{
}

class CTachyonTCMLParserProgress : public TCMLParserProgress
{
public:
	virtual void OnProgress(float percent)
	{
		CTachyonRes::RenderBACK( 9, (INT)(percent * 100) );
	}
};

void CTachyonWnd::InitResource( CString strGroupID,
							    BYTE bGroupID)
{
	m_RES.Init( strGroupID, bGroupID);

	if(m_pTParser)
		delete m_pTParser;

	CTachyonTCMLParserProgress progress;
	progress.OnProgress(0.0f);

	m_pTParser = new TCMLParser();
	m_pTParser->Load((char *) LPCTSTR(strGroupID + _T("Cmd.TIF")), &progress );

	InitTachyonComponent( &m_RES, &m_RES, m_pTParser);
	TFrame::m_pCommandHandler = this;
}

void CTachyonWnd::OnImeComposition( HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if(m_pMainFrame)
		m_pMainFrame->OnImeComposition( hWnd, wParam, lParam);
}

void CTachyonWnd::OnImeNotify( HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if(m_pMainFrame)
		m_pMainFrame->OnImeNotify( hWnd, wParam, lParam);
}

BOOL CTachyonWnd::OnInputLangChangeRequest( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
	if(m_pMainFrame)
		return m_pMainFrame->OnInputLangChangeRequest( hWnd, wParam, lParam );
	return TRUE;
}

void CTachyonWnd::OnChar( UINT nChar, UINT nRep, UINT nFlags)
{
	if(m_pMainFrame)
		m_pMainFrame->OnChar(nChar, nRep, nFlags);
}

void CTachyonWnd::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(m_pMainFrame)
		m_pMainFrame->OnKeyDown( nChar, nRepCnt, nFlags);
}

void CTachyonWnd::OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(m_pMainFrame)
		m_pMainFrame->OnKeyUp( nChar, nRepCnt, nFlags);
}

LRESULT CTachyonWnd::OnNcHitTest( CPoint point)
{
	LRESULT nHIT = CTWindow::OnNcHitTest(point);

	if( nHIT == HTSYSMENU )
		nHIT = HTCAPTION;

	return nHIT;
}

void CTachyonWnd::OnActivate( UINT nState, HWND hWndOther, BOOL bMinimized)
{
	CTWindow::OnActivate( nState, hWndOther, bMinimized);

	BYTE bBACK = IsForeground() ? FALSE : TRUE;
	if(m_bActivate)
	{
		if(m_pMainFrame)
			if(bBACK)
				m_pMainFrame->OnLoseFocus(NULL);
			else
				m_pMainFrame->OnSetFocus(NULL);

		if( CTachyonMedia::m_bBACK != bBACK )
			CTachyonRes::m_MEDIA.OnSetFocus(bBACK);
	}
	else if( CTachyonMedia::m_bBACK != bBACK )
		CTachyonMedia::m_bBACK = bBACK;

	if( !m_Device.m_option.m_bWindowedMode &&
		!bMinimized && bBACK )
		ShowWindow(SW_MINIMIZE);
}

void CTachyonWnd::OnNcRButtonDown( UINT nHitTest, CPoint point)
{
}
