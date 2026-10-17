// TWindow.cpp: implementation of the CTWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <windowsx.h>


CTWindow::CTWindow()
{
	m_hWnd = NULL;
	ZeroMemory( &m_vCURRENT, sizeof(MSG));
}

CTWindow::~CTWindow()
{
	DestroyWindow();
}

LPCTSTR CTWindow::GetWndClass()
{
	static ATOM s_wCLASS = 0;

	if(!s_wCLASS)
	{
		WNDCLASSEX vCLASS;

		ZeroMemory( &vCLASS, sizeof(WNDCLASSEX));
		vCLASS.cbSize = sizeof(WNDCLASSEX);
		vCLASS.style = CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS;
		vCLASS.lpfnWndProc = WndProc;
		vCLASS.hInstance = GetModuleHandle(NULL);
		vCLASS.lpszClassName = _T("TachyonWnd");

		s_wCLASS = RegisterClassEx(&vCLASS);
	}

	return _T("TachyonWnd");
}

BOOL CTWindow::CreateEx( DWORD dwExStyle,
						 LPCTSTR lpszTitle,
						 DWORD dwStyle,
						 const RECT& rect,
						 HWND hParent)
{
	HWND hWnd = ::CreateWindowEx(
		dwExStyle,
		GetWndClass(),
		lpszTitle,
		dwStyle,
		rect.left,
		rect.top,
		rect.right - rect.left,
		rect.bottom - rect.top,
		hParent,
		NULL,
		GetModuleHandle(NULL),
		this);

	return hWnd ? TRUE : FALSE;
}

void CTWindow::GetWindowText( CString& strText) const
{
	int nLength = ::GetWindowTextLength(m_hWnd);

	::GetWindowText( m_hWnd, strText.GetBufferSetLength(nLength + 1), nLength + 1);
	strText.ReleaseBuffer();
}

BOOL CTWindow::DestroyWindow()
{
	if(!m_hWnd)
		return FALSE;

	return ::DestroyWindow(m_hWnd);
}

LRESULT CALLBACK CTWindow::WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CTWindow *pWnd = (CTWindow *) GetWindowLongPtr( hWnd, GWLP_USERDATA);

	if( message == WM_NCCREATE )
	{
		pWnd = (CTWindow *) ((LPCREATESTRUCT) lParam)->lpCreateParams;
		pWnd->m_hWnd = hWnd;
		SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR) pWnd);
	}

	if(!pWnd)
		return ::DefWindowProc( hWnd, message, wParam, lParam);

	// The current message, for Default(); a nested message restores it after.
	MSG vPREV = pWnd->m_vCURRENT;
	pWnd->m_vCURRENT.hwnd = hWnd;
	pWnd->m_vCURRENT.message = message;
	pWnd->m_vCURRENT.wParam = wParam;
	pWnd->m_vCURRENT.lParam = lParam;

	LRESULT lResult = pWnd->WindowProc( message, wParam, lParam);
	pWnd->m_vCURRENT = vPREV;

	if( message == WM_NCDESTROY )
	{
		SetWindowLongPtr( hWnd, GWLP_USERDATA, 0);
		pWnd->m_hWnd = NULL;
	}

	return lResult;
}

LRESULT CTWindow::Default()
{
	return DefWindowProc( m_vCURRENT.message, m_vCURRENT.wParam, m_vCURRENT.lParam);
}

LRESULT CTWindow::DefWindowProc( UINT message, WPARAM wParam, LPARAM lParam)
{
	return ::DefWindowProc( m_hWnd, message, wParam, lParam);
}

LRESULT CTWindow::WindowProc( UINT message, WPARAM wParam, LPARAM lParam)
{
	CPoint point( GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	UINT nRepCnt = LOWORD(lParam);
	UINT nFlags = HIWORD(lParam);

	switch(message)
	{
	case WM_ACTIVATE		: OnActivate( LOWORD(wParam), (HWND) lParam, HIWORD(wParam)); return 0;
	case WM_DESTROY			: OnDestroy(); return 0;
	case WM_NCDESTROY		: OnNcDestroy(); return 0;

	case WM_KEYDOWN			: OnKeyDown( UINT(wParam), nRepCnt, nFlags); return 0;
	case WM_KEYUP			: OnKeyUp( UINT(wParam), nRepCnt, nFlags); return 0;
	case WM_SYSKEYDOWN		: OnSysKeyDown( UINT(wParam), nRepCnt, nFlags); return 0;
	case WM_SYSKEYUP		: OnSysKeyUp( UINT(wParam), nRepCnt, nFlags); return 0;

	case WM_MOUSEMOVE		: OnMouseMove( UINT(wParam), point); return 0;
	case WM_LBUTTONDOWN		: OnLButtonDown( UINT(wParam), point); return 0;
	case WM_LBUTTONUP		: OnLButtonUp( UINT(wParam), point); return 0;
	case WM_LBUTTONDBLCLK	: OnLButtonDblClk( UINT(wParam), point); return 0;
	case WM_MBUTTONDOWN		: OnMButtonDown( UINT(wParam), point); return 0;
	case WM_MBUTTONUP		: OnMButtonUp( UINT(wParam), point); return 0;
	case WM_MBUTTONDBLCLK	: OnMButtonDblClk( UINT(wParam), point); return 0;
	case WM_RBUTTONDOWN		: OnRButtonDown( UINT(wParam), point); return 0;
	case WM_RBUTTONUP		: OnRButtonUp( UINT(wParam), point); return 0;
	case WM_RBUTTONDBLCLK	: OnRButtonDblClk( UINT(wParam), point); return 0;

	// The wheel position is in screen coordinates, like MFC's.
	case WM_MOUSEWHEEL		: return OnMouseWheel( GET_KEYSTATE_WPARAM(wParam), GET_WHEEL_DELTA_WPARAM(wParam), point);
	case WM_SETCURSOR		: return OnSetCursor( (HWND) wParam, LOWORD(lParam), HIWORD(lParam));

	case WM_NCHITTEST		: return OnNcHitTest(point);
	case WM_NCLBUTTONDOWN	: OnNcLButtonDown( UINT(wParam), point); return 0;
	case WM_NCRBUTTONDOWN	: OnNcRButtonDown( UINT(wParam), point); return 0;
	}

	return DefWindowProc( message, wParam, lParam);
}
