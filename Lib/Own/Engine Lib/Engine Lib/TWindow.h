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

	// The size of the main display, in pixels, and its modes at the current refresh rate
	// (smallest first).
	static CSize GetScreenSize();
	static void GetDisplayModes( std::vector<CSize>& vMODES);

	// MB_ styles and ID results, like the Win32 call; texts in the game's ANSI code page.
	static int ShowMessageBox( LPCTSTR lpszText, LPCTSTR lpszCaption = NULL, UINT nType = MB_OK, SDL_Window *pParent = NULL);
	static BOOL OpenURL( LPCTSTR lpszURL);
	static BOOL SetClipboardText( LPCTSTR lpszText);

public:
	// A hidden window of this client size, with a caption or borderless.
	BOOL Create( LPCTSTR lpszTitle, int nWidth, int nHeight, BOOL bCaption, BOOL bTopMost);
	BOOL DestroyWindow();

	HWND GetSafeHwnd() const { return m_hWnd; }
	SDL_Window *GetSDLWindow() const { return m_pSDL; }

	void SetWindowText( LPCTSTR lpszText);
	void GetWindowText( CString& strText) const;
	int MessageBox( LPCTSTR lpszText, LPCTSTR lpszCaption = NULL, UINT nType = MB_OK) { return ShowMessageBox( lpszText, lpszCaption, nType, m_pSDL); }
	HICON SetIcon( HICON hIcon, BOOL bBigIcon) { return (HICON) ::SendMessage( m_hWnd, WM_SETICON, bBigIcon ? ICON_BIG : ICON_SMALL, (LPARAM) hIcon); }

	// SW_HIDE, SW_SHOW, SW_SHOWNORMAL, SW_RESTORE, SW_MINIMIZE, SW_MAXIMIZE.
	BOOL ShowWindow( int nCmdShow);
	BOOL IsWindowVisible() const;
	BOOL IsIconic() const;
	BOOL IsForeground() const;
	void SetFocus();

	// The caption, the client size with the frame at the top left of the screen, topmost or not.
	void SetCaption( BOOL bCaption);
	void SetFrameSize( int nWidth, int nHeight);
	void SetTopMost( BOOL bTopMost);

	// The window rectangle includes the frame, like Win32's.
	void MoveWindow( int x, int y, int nWidth, int nHeight);
	void MoveWindow( LPCRECT lpRect) { MoveWindow( lpRect->left, lpRect->top, lpRect->right - lpRect->left, lpRect->bottom - lpRect->top); }

	void GetClientRect( LPRECT lpRect) const;
	void GetWindowRect( LPRECT lpRect) const;

	void ClientToScreen( LPPOINT lpPoint) const;
	void ClientToScreen( LPRECT lpRect) const;
	void ScreenToClient( LPPOINT lpPoint) const;
	void ScreenToClient( LPRECT lpRect) const;

	BOOL ModifyStyle( DWORD dwRemove, DWORD dwAdd) { return ::SetWindowLongPtr( m_hWnd, GWL_STYLE, (::GetWindowLongPtr( m_hWnd, GWL_STYLE) & ~LONG_PTR(dwRemove)) | dwAdd) != 0; }

	// To this window's WindowProc: now, or through the event queue (WM_QUIT quits).
	LRESULT SendMessage( UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
	BOOL PostMessage( UINT message, WPARAM wParam = 0, LPARAM lParam = 0);

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
