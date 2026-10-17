// TWindow.h: the engine's window, an SDL3 window.
//
// SDL's events go to virtual handlers named like MFC's (OnKeyDown, OnLButtonDown...) with the
// Win32 parameters the engine always had: virtual key codes, the scan code word of the key
// messages, MK_ flags and client coordinates. The handlers' default does nothing more, SDL has
// already given the message its default processing.
//
// SDL's text events become OnChar (a character per byte, in the game's ANSI code page, with
// the control characters of Enter, Esc, Backspace, Tab and Ctrl+letter that Windows sends),
// OnImeComposition (GCS_ flags; the strings are in CTachyonInput) and OnImeNotify (candidates).
//
// On Windows, the cursor, the caption drag, the IME's mode changes and the engine's own posted
// messages still take the Win32 path: a subclass of SDL's window procedure sends them to
// WindowProc, and posted ones go through SDL's event queue to keep their order.
//////////////////////////////////////////////////////////////////////

#if !defined __TWINDOW_H
#define __TWINDOW_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


struct SDL_Window;
union SDL_Event;

// What CTWindow::PeekEvent found.
#define TWINDOW_NOEVENT				0
#define TWINDOW_EVENT				1
#define TWINDOW_QUIT				2


class CTWindow
{
public:
	CTWindow();
	virtual ~CTWindow();

public:
	// SDL for the whole application: before the first window, after the last one.
	static BOOL InitPlatform();
	static void QuitPlatform();

	// Handles one pending event. TWINDOW_QUIT once after PostQuit (or a posted WM_QUIT).
	static BYTE PeekEvent();

	// Waits for an event, at most dwTimeout ms. The event stays queued.
	static void WaitEvent( DWORD dwTimeout);
	static void PostQuit();

public:
	BOOL CreateEx(
		DWORD dwExStyle,
		LPCTSTR lpszTitle,
		DWORD dwStyle,
		const RECT& rect,
		HWND hParent);

	BOOL DestroyWindow();

	HWND GetSafeHwnd() const { return m_hWnd; }
	SDL_Window *GetSDLWindow() const { return m_pSDL; }

	void SetWindowText( LPCTSTR lpszText) { ::SetWindowText( m_hWnd, lpszText); }
	void GetWindowText( CString& strText) const;
	int MessageBox( LPCTSTR lpszText, LPCTSTR lpszCaption = NULL, UINT nType = MB_OK) { return ::MessageBox( m_hWnd, lpszText, lpszCaption, nType); }
	BOOL UpdateWindow() { return ::UpdateWindow(m_hWnd); }
	HICON SetIcon( HICON hIcon, BOOL bBigIcon) { return (HICON) ::SendMessage( m_hWnd, WM_SETICON, bBigIcon ? ICON_BIG : ICON_SMALL, (LPARAM) hIcon); }

	BOOL ShowWindow( int nCmdShow) { return ::ShowWindow( m_hWnd, nCmdShow); }
	BOOL IsWindowVisible() const { return ::IsWindowVisible(m_hWnd); }
	BOOL IsIconic() const { return ::IsIconic(m_hWnd); }

	void MoveWindow( int x, int y, int nWidth, int nHeight, BOOL bRepaint = TRUE) { ::MoveWindow( m_hWnd, x, y, nWidth, nHeight, bRepaint); }
	void MoveWindow( LPCRECT lpRect, BOOL bRepaint = TRUE) { MoveWindow( lpRect->left, lpRect->top, lpRect->right - lpRect->left, lpRect->bottom - lpRect->top, bRepaint); }
	BOOL SetWindowPos( HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT nFlags) { return ::SetWindowPos( m_hWnd, hWndInsertAfter, x, y, cx, cy, nFlags); }

	void GetClientRect( LPRECT lpRect) const { ::GetClientRect( m_hWnd, lpRect); }
	void GetWindowRect( LPRECT lpRect) const { ::GetWindowRect( m_hWnd, lpRect); }

	void ClientToScreen( LPPOINT lpPoint) const { ::ClientToScreen( m_hWnd, lpPoint); }
	void ClientToScreen( LPRECT lpRect) const { ::MapWindowPoints( m_hWnd, NULL, (LPPOINT) lpRect, 2); }
	void ScreenToClient( LPPOINT lpPoint) const { ::ScreenToClient( m_hWnd, lpPoint); }
	void ScreenToClient( LPRECT lpRect) const { ::MapWindowPoints( NULL, m_hWnd, (LPPOINT) lpRect, 2); }

	BOOL ModifyStyle( DWORD dwRemove, DWORD dwAdd) { return ::SetWindowLongPtr( m_hWnd, GWL_STYLE, (::GetWindowLongPtr( m_hWnd, GWL_STYLE) & ~LONG_PTR(dwRemove)) | dwAdd) != 0; }

	HWND SetCapture() { return ::SetCapture(m_hWnd); }
	HWND SetFocus() { return ::SetFocus(m_hWnd); }

	LRESULT SendMessage( UINT message, WPARAM wParam = 0, LPARAM lParam = 0) { return ::SendMessage( m_hWnd, message, wParam, lParam); }
	BOOL PostMessage( UINT message, WPARAM wParam = 0, LPARAM lParam = 0) { return ::PostMessage( m_hWnd, message, wParam, lParam); }

	// Whether hWnd is this window (MFC code compared CWnd pointers).
	BOOL IsThis( HWND hWnd) const { return m_hWnd && hWnd == m_hWnd; }

	// Public like MFC's handlers: other code calls them.
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam);
	virtual LRESULT DefWindowProc( UINT message, WPARAM wParam, LPARAM lParam);

	// DefWindowProc of the Win32 message being handled; 0 for an SDL event.
	LRESULT Default();

	virtual void OnActivate( UINT nState, HWND hWndOther, BOOL bMinimized) { Default(); }
	virtual void OnDestroy() {}
	virtual void OnNcDestroy() {}

	virtual void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags) { Default(); }
	virtual void OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags) { Default(); }
	virtual void OnSysKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags) { Default(); }
	virtual void OnSysKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags) { Default(); }
	virtual void OnChar( UINT nChar, UINT nRepCnt, UINT nFlags) { Default(); }
	virtual void OnImeComposition( HWND hWnd, WPARAM wParam, LPARAM lParam) { Default(); }
	virtual void OnImeNotify( HWND hWnd, WPARAM wParam, LPARAM lParam) { Default(); }

	virtual void OnMouseMove( UINT nFlags, CPoint point) { Default(); }
	virtual void OnLButtonDown( UINT nFlags, CPoint point) { Default(); }
	virtual void OnLButtonUp( UINT nFlags, CPoint point) { Default(); }
	virtual void OnLButtonDblClk( UINT nFlags, CPoint point) { Default(); }
	virtual void OnMButtonDown( UINT nFlags, CPoint point) { Default(); }
	virtual void OnMButtonUp( UINT nFlags, CPoint point) { Default(); }
	virtual void OnMButtonDblClk( UINT nFlags, CPoint point) { Default(); }
	virtual void OnRButtonDown( UINT nFlags, CPoint point) { Default(); }
	virtual void OnRButtonUp( UINT nFlags, CPoint point) { Default(); }
	virtual void OnRButtonDblClk( UINT nFlags, CPoint point) { Default(); }
	virtual BOOL OnMouseWheel( UINT nFlags, short zDelta, CPoint pt) { return (BOOL) Default(); }
	virtual BOOL OnSetCursor( HWND hWnd, UINT nHitTest, UINT message) { return (BOOL) Default(); }

	virtual LRESULT OnNcHitTest( CPoint point) { return Default(); }
	virtual void OnNcLButtonDown( UINT nHitTest, CPoint point) { Default(); }
	virtual void OnNcRButtonDown( UINT nHitTest, CPoint point) { Default(); }

private:
	void Dispatch( const SDL_Event& event);
	void DispatchKey( const SDL_Event& event);
	void DispatchMouse( const SDL_Event& event);
	void DispatchText( const SDL_Event& event);

	LRESULT CallWindowProc( UINT message, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK SubclassProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static CTWindow *FromSDLWindow( SDL_Window *pSDL);

public:
	HWND m_hWnd;

private:
	SDL_Window *m_pSDL;
	WNDPROC m_pSDLProc;
	DWORD m_dwButtons;
	MSG m_vCURRENT;
};


inline HWND TSAFE_HWND( const CTWindow* pWnd )
{
	return pWnd ? pWnd->m_hWnd : NULL;
}

#endif // !defined __TWINDOW_H
