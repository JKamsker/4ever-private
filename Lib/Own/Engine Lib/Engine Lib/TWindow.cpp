// TWindow.cpp: implementation of the CTWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <windowsx.h>
#include <algorithm>
#include <SDL3/SDL.h>

#define TWINDOW_PROP				"TachyonWnd"
#define TWINDOW_PROPW				L"TachyonWnd"


static Uint32 s_dwMSGEVENT = 0;
static WORD s_wPROCESSKEY = 0;
static UINT s_nCHARFLAGS = 0;
static BOOL s_bQUIT = FALSE;


// The flags of a mouse message: buttons held and modifiers.
static UINT GetMouseFlags( DWORD dwButtons)
{
	SDL_Keymod nMod = SDL_GetModState();
	UINT nFlags = 0;

	if( dwButtons & SDL_BUTTON_LMASK )
		nFlags |= MK_LBUTTON;

	if( dwButtons & SDL_BUTTON_MMASK )
		nFlags |= MK_MBUTTON;

	if( dwButtons & SDL_BUTTON_RMASK )
		nFlags |= MK_RBUTTON;

	if( nMod & SDL_KMOD_SHIFT )
		nFlags |= MK_SHIFT;

	if( nMod & SDL_KMOD_CTRL )
		nFlags |= MK_CONTROL;

	return nFlags;
}

// The engine's own posted messages go through SDL's queue, in order with the other events.
static bool SDLCALL WindowsMessageHook( void *pUserData, MSG *pMSG)
{
	if( pMSG->message == WM_QUIT )
	{
		s_bQUIT = TRUE;
		return false;
	}

	CTWindow *pWnd = pMSG->hwnd ? (CTWindow *) ::GetPropW( pMSG->hwnd, TWINDOW_PROPW) : NULL;
	if(!pWnd)
		return true;

	SDL_Event event;
	SDL_zero(event);
	event.type = s_dwMSGEVENT;
	event.user.windowID = SDL_GetWindowID(pWnd->GetSDLWindow());
	event.user.code = Sint32(pMSG->message);
	event.user.data1 = (void *) pMSG->wParam;
	event.user.data2 = (void *) pMSG->lParam;

	switch(pMSG->message)
	{
	case WM_KEYDOWN				:
	case WM_SYSKEYDOWN			:
		// A key the IME takes: the key event says VK_PROCESSKEY, like Windows.
		s_wPROCESSKEY = pMSG->wParam == VK_PROCESSKEY ? HIWORD(pMSG->lParam) & 0x1FF : 0;
		return true;
	}

	if( pMSG->message >= WM_USER && pMSG->message < 0x8000 )
	{
		SDL_PushEvent(&event);
		return false;
	}

	return true;
}

// The Win32 messages the window handles itself instead of SDL. The IME's mode changes and the
// keyboard layout changes are the edits' to check, then SDL gets them (see DefWindowProc).
static BOOL IsWindowMessage( UINT message)
{
	switch(message)
	{
	case WM_IME_NOTIFY				:
	case WM_INPUTLANGCHANGEREQUEST	:
	case WM_INPUTLANGCHANGE			:
	case WM_NCHITTEST				:
	case WM_NCLBUTTONDOWN			:
	case WM_NCRBUTTONDOWN			:
	case WM_POWERBROADCAST			: return TRUE;
	}

	return message >= WM_USER && message < 0x8000;
}


CTWindow::CTWindow()
{
	m_hWnd = NULL;
	m_pSDL = NULL;
	m_pSDLProc = NULL;
	m_dwButtons = 0;

	ZeroMemory( &m_vCURRENT, sizeof(MSG));
}

CTWindow::~CTWindow()
{
	DestroyWindow();
}

BOOL CTWindow::InitPlatform()
{
	// Win32 behavior: Alt+F4 is the game's key, a click that activates the window counts.
	// The game draws the IME's composition and candidates itself.
	SDL_SetHint( SDL_HINT_WINDOWS_CLOSE_ON_ALT_F4, "0");
	SDL_SetHint( SDL_HINT_IME_IMPLEMENTED_UI, "composition,candidates");
	SDL_SetHint( SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
	SDL_SetHint( SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");

	if(!SDL_Init(SDL_INIT_VIDEO))
		return FALSE;

	CTachyonInput::InitPlatform();
	s_dwMSGEVENT = SDL_RegisterEvents(1);
	SDL_SetWindowsMessageHook( WindowsMessageHook, NULL);

	return TRUE;
}

void CTWindow::QuitPlatform()
{
	SDL_SetWindowsMessageHook( NULL, NULL);
	SDL_Quit();
}

BYTE CTWindow::PeekEvent()
{
	SDL_Event event;

	if(!s_bQUIT)
	{
		if(!SDL_PollEvent(&event))
		{
			if(!s_bQUIT)
				return TWINDOW_NOEVENT;
		}
		else
		{
			CTWindow *pWnd = FromSDLWindow(SDL_GetWindowFromEvent(&event));

			if(pWnd)
				pWnd->Dispatch(event);
			else if( event.type == SDL_EVENT_QUIT )
				s_bQUIT = TRUE;

			return TWINDOW_EVENT;
		}
	}

	s_bQUIT = FALSE;
	return TWINDOW_QUIT;
}

void CTWindow::WaitEvent( DWORD dwTimeout)
{
	SDL_WaitEventTimeout( NULL, Sint32(dwTimeout));
}

void CTWindow::PostQuit()
{
	s_bQUIT = TRUE;
}

CTWindow *CTWindow::FromSDLWindow( SDL_Window *pSDL)
{
	return pSDL ? (CTWindow *) SDL_GetPointerProperty( SDL_GetWindowProperties(pSDL), TWINDOW_PROP, NULL) : NULL;
}

// SDL's strings are UTF-8; the game's are in its code page.
static CStringA ToUTF8( LPCTSTR lpszText)
{
	CStringW strW(lpszText ? lpszText : _T(""));
	CStringA strUTF8;
	int nLength = CTCodePage::FromWide( strW, -1, NULL, 0, TCP_UTF8);

	CTCodePage::FromWide( strW, -1, strUTF8.GetBufferSetLength(nLength), nLength, TCP_UTF8);
	strUTF8.ReleaseBuffer();

	return strUTF8;
}

CSize CTWindow::GetScreenSize()
{
	const SDL_DisplayMode *pMode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
	return pMode ? CSize( pMode->w, pMode->h) : CSize( 0, 0);
}

void CTWindow::GetDisplayModes( std::vector<CSize>& vMODES)
{
	SDL_DisplayID nDisplay = SDL_GetPrimaryDisplay();
	const SDL_DisplayMode *pCurrent = SDL_GetCurrentDisplayMode(nDisplay);
	int nCount = 0;
	SDL_DisplayMode **pModes = SDL_GetFullscreenDisplayModes( nDisplay, &nCount);

	vMODES.clear();
	for( int i=nCount - 1; i >= 0 && pCurrent; --i )
	{
		CSize vMODE( pModes[i]->w, pModes[i]->h);

		if( pModes[i]->refresh_rate == pCurrent->refresh_rate &&
			std::find( vMODES.begin(), vMODES.end(), vMODE) == vMODES.end() )
			vMODES.push_back(vMODE);
	}

	SDL_free(pModes);
}

int CTWindow::ShowMessageBox( LPCTSTR lpszText, LPCTSTR lpszCaption, UINT nType, SDL_Window *pParent)
{
	static const SDL_MessageBoxButtonData vOK[] = {
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT|SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDOK, "OK" }};
	static const SDL_MessageBoxButtonData vOKCANCEL[] = {
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDOK, "OK" },
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDCANCEL, "Cancel" }};
	static const SDL_MessageBoxButtonData vABORTRETRYIGNORE[] = {
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDABORT, "Abort" },
		{ 0, IDRETRY, "Retry" },
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDIGNORE, "Ignore" }};
	static const SDL_MessageBoxButtonData vYESNOCANCEL[] = {
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDYES, "Yes" },
		{ 0, IDNO, "No" },
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDCANCEL, "Cancel" }};
	static const SDL_MessageBoxButtonData vYESNO[] = {
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDYES, "Yes" },
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDNO, "No" }};
	static const SDL_MessageBoxButtonData vRETRYCANCEL[] = {
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDRETRY, "Retry" },
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDCANCEL, "Cancel" }};

	CStringA strText = ToUTF8(lpszText);
	CStringA strCaption = ToUTF8(lpszCaption ? lpszCaption : _T("Error"));

	SDL_MessageBoxData vDATA;
	SDL_zero(vDATA);
	vDATA.window = pParent;
	vDATA.title = strCaption;
	vDATA.message = strText;

	switch( nType & MB_ICONMASK )
	{
	case MB_ICONERROR		: vDATA.flags = SDL_MESSAGEBOX_ERROR; break;
	case MB_ICONWARNING		: vDATA.flags = SDL_MESSAGEBOX_WARNING; break;
	default					: vDATA.flags = SDL_MESSAGEBOX_INFORMATION; break;
	}

	switch( nType & MB_TYPEMASK )
	{
	case MB_OKCANCEL			: vDATA.buttons = vOKCANCEL; vDATA.numbuttons = 2; break;
	case MB_ABORTRETRYIGNORE	: vDATA.buttons = vABORTRETRYIGNORE; vDATA.numbuttons = 3; break;
	case MB_YESNOCANCEL			: vDATA.buttons = vYESNOCANCEL; vDATA.numbuttons = 3; break;
	case MB_YESNO				: vDATA.buttons = vYESNO; vDATA.numbuttons = 2; break;
	case MB_RETRYCANCEL			: vDATA.buttons = vRETRYCANCEL; vDATA.numbuttons = 2; break;
	default						: vDATA.buttons = vOK; vDATA.numbuttons = 1; break;
	}

	int nButton = 0;
	if(!SDL_ShowMessageBox( &vDATA, &nButton))
		return 0;

	return nButton;
}

BOOL CTWindow::OpenURL( LPCTSTR lpszURL)
{
	return SDL_OpenURL(ToUTF8(lpszURL)) ? TRUE : FALSE;
}

BOOL CTWindow::SetClipboardText( LPCTSTR lpszText)
{
	return SDL_SetClipboardText(ToUTF8(lpszText)) ? TRUE : FALSE;
}

BOOL CTWindow::Create( LPCTSTR lpszTitle, int nWidth, int nHeight, BOOL bCaption, BOOL bTopMost)
{
	SDL_WindowFlags dwFlags = SDL_WINDOW_HIDDEN;

	if(!bCaption)
		dwFlags |= SDL_WINDOW_BORDERLESS;

	if(bTopMost)
		dwFlags |= SDL_WINDOW_ALWAYS_ON_TOP;

	m_pSDL = SDL_CreateWindow( ToUTF8(lpszTitle), nWidth, nHeight, dwFlags);
	if(!m_pSDL)
		return FALSE;

	SDL_SetPointerProperty( SDL_GetWindowProperties(m_pSDL), TWINDOW_PROP, this);
	m_hWnd = (HWND) SDL_GetPointerProperty( SDL_GetWindowProperties(m_pSDL), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

	::SetPropW( m_hWnd, TWINDOW_PROPW, this);
	m_pSDLProc = (WNDPROC) ::SetWindowLongPtrW( m_hWnd, GWLP_WNDPROC, (LONG_PTR) SubclassProc);

	// SDL keeps the IME off until text input starts.
	SDL_StartTextInput(m_pSDL);
	SetFrameSize( nWidth, nHeight);

	return TRUE;
}

void CTWindow::SetWindowText( LPCTSTR lpszText)
{
	if(m_pSDL)
		SDL_SetWindowTitle( m_pSDL, ToUTF8(lpszText));
}

void CTWindow::GetWindowText( CString& strText) const
{
	strText = m_pSDL ? CTachyonInput::FromUTF8(SDL_GetWindowTitle(m_pSDL)) : CString();
}

BOOL CTWindow::ShowWindow( int nCmdShow)
{
	BOOL bVisible = IsWindowVisible();

	if(!m_pSDL)
		return FALSE;

	switch(nCmdShow)
	{
	case SW_HIDE			: SDL_HideWindow(m_pSDL); break;
	case SW_MINIMIZE		:
	case SW_SHOWMINIMIZED	: SDL_MinimizeWindow(m_pSDL); break;
	case SW_RESTORE			:
	case SW_SHOWNORMAL		: SDL_ShowWindow(m_pSDL); SDL_RestoreWindow(m_pSDL); break;
	case SW_MAXIMIZE		:
		SDL_ShowWindow(m_pSDL);

		// A borderless window already covers the screen (SetFrameSize).
		if(!(SDL_GetWindowFlags(m_pSDL) & SDL_WINDOW_BORDERLESS))
			SDL_MaximizeWindow(m_pSDL);

		break;

	default					: SDL_ShowWindow(m_pSDL); break;
	}

	return bVisible;
}

BOOL CTWindow::IsWindowVisible() const
{
	return m_pSDL && !(SDL_GetWindowFlags(m_pSDL) & SDL_WINDOW_HIDDEN) ? TRUE : FALSE;
}

BOOL CTWindow::IsIconic() const
{
	return m_pSDL && (SDL_GetWindowFlags(m_pSDL) & SDL_WINDOW_MINIMIZED) ? TRUE : FALSE;
}

BOOL CTWindow::IsForeground() const
{
	return m_pSDL && (SDL_GetWindowFlags(m_pSDL) & SDL_WINDOW_INPUT_FOCUS) ? TRUE : FALSE;
}

void CTWindow::SetFocus()
{
	if(m_pSDL)
		SDL_RaiseWindow(m_pSDL);
}

void CTWindow::SetCaption( BOOL bCaption)
{
	if(m_pSDL)
		SDL_SetWindowBordered( m_pSDL, bCaption ? true : false);
}

void CTWindow::SetFrameSize( int nWidth, int nHeight)
{
	int nTop = 0;
	int nLeft = 0;

	if(!m_pSDL)
		return;

	SDL_SetWindowSize( m_pSDL, nWidth, nHeight);
	SDL_GetWindowBordersSize( m_pSDL, &nTop, &nLeft, NULL, NULL);
	SDL_SetWindowPosition( m_pSDL, nLeft, nTop);
}

void CTWindow::SetTopMost( BOOL bTopMost)
{
	if(m_pSDL)
		SDL_SetWindowAlwaysOnTop( m_pSDL, bTopMost ? true : false);
}

void CTWindow::MoveWindow( int x, int y, int nWidth, int nHeight)
{
	int nTop = 0;
	int nLeft = 0;
	int nBottom = 0;
	int nRight = 0;

	if(!m_pSDL)
		return;

	SDL_GetWindowBordersSize( m_pSDL, &nTop, &nLeft, &nBottom, &nRight);
	SDL_SetWindowPosition( m_pSDL, x + nLeft, y + nTop);
	SDL_SetWindowSize( m_pSDL, nWidth - nLeft - nRight, nHeight - nTop - nBottom);
}

void CTWindow::GetClientRect( LPRECT lpRect) const
{
	int nWidth = 0;
	int nHeight = 0;

	if(m_pSDL)
		SDL_GetWindowSize( m_pSDL, &nWidth, &nHeight);

	::SetRect( lpRect, 0, 0, nWidth, nHeight);
}

void CTWindow::GetWindowRect( LPRECT lpRect) const
{
	int nTop = 0;
	int nLeft = 0;
	int nBottom = 0;
	int nRight = 0;
	CRect rect;

	GetClientRect(&rect);
	ClientToScreen(&rect);

	if(m_pSDL)
		SDL_GetWindowBordersSize( m_pSDL, &nTop, &nLeft, &nBottom, &nRight);

	::SetRect( lpRect, rect.left - nLeft, rect.top - nTop, rect.right + nRight, rect.bottom + nBottom);
}

void CTWindow::ClientToScreen( LPPOINT lpPoint) const
{
	int nX = 0;
	int nY = 0;

	if(m_pSDL)
		SDL_GetWindowPosition( m_pSDL, &nX, &nY);

	lpPoint->x += nX;
	lpPoint->y += nY;
}

void CTWindow::ClientToScreen( LPRECT lpRect) const
{
	ClientToScreen((LPPOINT) lpRect);
	ClientToScreen(((LPPOINT) lpRect) + 1);
}

void CTWindow::ScreenToClient( LPPOINT lpPoint) const
{
	int nX = 0;
	int nY = 0;

	if(m_pSDL)
		SDL_GetWindowPosition( m_pSDL, &nX, &nY);

	lpPoint->x -= nX;
	lpPoint->y -= nY;
}

void CTWindow::ScreenToClient( LPRECT lpRect) const
{
	ScreenToClient((LPPOINT) lpRect);
	ScreenToClient(((LPPOINT) lpRect) + 1);
}

LRESULT CTWindow::SendMessage( UINT message, WPARAM wParam, LPARAM lParam)
{
	return CallWindowProc( message, wParam, lParam);
}

BOOL CTWindow::PostMessage( UINT message, WPARAM wParam, LPARAM lParam)
{
	if( message == WM_QUIT )
	{
		PostQuit();
		return TRUE;
	}

	SDL_Event event;
	SDL_zero(event);
	event.type = s_dwMSGEVENT;
	event.user.windowID = SDL_GetWindowID(m_pSDL);
	event.user.code = Sint32(message);
	event.user.data1 = (void *) wParam;
	event.user.data2 = (void *) lParam;

	return SDL_PushEvent(&event) ? TRUE : FALSE;
}

BOOL CTWindow::DestroyWindow()
{
	if(!m_pSDL)
		return FALSE;

	MSG vPREV = m_vCURRENT;
	ZeroMemory( &m_vCURRENT, sizeof(MSG));
	OnDestroy();

	::SetWindowLongPtrW( m_hWnd, GWLP_WNDPROC, (LONG_PTR) m_pSDLProc);
	::RemovePropW( m_hWnd, TWINDOW_PROPW);

	SDL_StopTextInput(m_pSDL);
	SDL_DestroyWindow(m_pSDL);

	m_pSDLProc = NULL;
	m_pSDL = NULL;
	OnNcDestroy();

	m_vCURRENT = vPREV;
	m_hWnd = NULL;

	return TRUE;
}

LRESULT CALLBACK CTWindow::SubclassProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CTWindow *pWnd = (CTWindow *) ::GetPropW( hWnd, TWINDOW_PROPW);

	if(!pWnd)
		return ::DefWindowProcW( hWnd, message, wParam, lParam);

	if(IsWindowMessage(message))
		return pWnd->CallWindowProc( message, wParam, lParam);

	return ::CallWindowProcW( pWnd->m_pSDLProc, hWnd, message, wParam, lParam);
}

LRESULT CTWindow::CallWindowProc( UINT message, WPARAM wParam, LPARAM lParam)
{
	// The current message, for Default(); a nested message restores it after.
	MSG vPREV = m_vCURRENT;

	m_vCURRENT.hwnd = m_hWnd;
	m_vCURRENT.message = message;
	m_vCURRENT.wParam = wParam;
	m_vCURRENT.lParam = lParam;

	LRESULT lResult = WindowProc( message, wParam, lParam);
	m_vCURRENT = vPREV;

	return lResult;
}

LRESULT CTWindow::Default()
{
	return m_vCURRENT.message ? DefWindowProc( m_vCURRENT.message, m_vCURRENT.wParam, m_vCURRENT.lParam) : 0;
}

LRESULT CTWindow::DefWindowProc( UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_IME_NOTIFY				:
	case WM_INPUTLANGCHANGEREQUEST	:
	case WM_INPUTLANGCHANGE			: return ::CallWindowProcW( m_pSDLProc, m_hWnd, message, wParam, lParam);
	}

	return ::DefWindowProcW( m_hWnd, message, wParam, lParam);
}

LRESULT CTWindow::WindowProc( UINT message, WPARAM wParam, LPARAM lParam)
{
	CPoint point( GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

	switch(message)
	{
	case WM_NCHITTEST		: return OnNcHitTest(point);
	case WM_NCLBUTTONDOWN	: OnNcLButtonDown( UINT(wParam), point); return 0;
	case WM_NCRBUTTONDOWN	: OnNcRButtonDown( UINT(wParam), point); return 0;
	}

	return DefWindowProc( message, wParam, lParam);
}

void CTWindow::Dispatch( const SDL_Event& event)
{
	// An SDL event: the handlers' Default() has nothing more to do.
	MSG vPREV = m_vCURRENT;
	ZeroMemory( &m_vCURRENT, sizeof(MSG));

	if( event.type == s_dwMSGEVENT )
		WindowProc( UINT(event.user.code), (WPARAM) event.user.data1, (LPARAM) event.user.data2);
	else switch(event.type)
	{
	case SDL_EVENT_KEY_DOWN					:
	case SDL_EVENT_KEY_UP					: DispatchKey(event); break;

	case SDL_EVENT_TEXT_INPUT				:
	case SDL_EVENT_TEXT_EDITING				:
	case SDL_EVENT_TEXT_EDITING_CANDIDATES	: DispatchText(event); break;

	case SDL_EVENT_MOUSE_MOTION				:
	case SDL_EVENT_MOUSE_BUTTON_DOWN		:
	case SDL_EVENT_MOUSE_BUTTON_UP			:
	case SDL_EVENT_MOUSE_WHEEL				: DispatchMouse(event); break;

	case SDL_EVENT_WINDOW_FOCUS_GAINED		: OnActivate( WA_ACTIVE, NULL, FALSE); break;
	case SDL_EVENT_WINDOW_FOCUS_LOST		: OnActivate( WA_INACTIVE, NULL, IsIconic()); break;
	case SDL_EVENT_WINDOW_CLOSE_REQUESTED	: DestroyWindow(); break;
	}

	m_vCURRENT = vPREV;
}

void CTWindow::DispatchKey( const SDL_Event& event)
{
	BOOL bDown = event.type == SDL_EVENT_KEY_DOWN;
	SDL_Keymod nMod = event.key.mod;
	WORD wScan = CTachyonInput::GetPCScanCode(event.key.scancode);

	UINT nChar = CTachyonInput::GetVirtualKey( event.key.scancode, event.key.key, nMod);
	if( bDown && s_wPROCESSKEY && s_wPROCESSKEY == ((wScan & 0xFF) | (wScan & 0xE000 ? KF_EXTENDED : 0)) )
		nChar = VK_PROCESSKEY;

	if(!nChar)
		return;

	// The scan code word of a Win32 key message (HIWORD of lParam).
	UINT nFlags = LOBYTE(wScan);
	if( wScan & 0xE000 )
		nFlags |= KF_EXTENDED;

	if( nMod & SDL_KMOD_ALT )
		nFlags |= KF_ALTDOWN;

	if( !bDown || event.key.repeat )
		nFlags |= KF_REPEAT;

	if(!bDown)
		nFlags |= KF_UP;

	// Windows sends the Alt combinations and F10 as system keys.
	BOOL bSystem = nChar == VK_MENU || nChar == VK_F10 || ((nMod & SDL_KMOD_ALT) && !(nMod & SDL_KMOD_CTRL));

	if(!bDown)
	{
		if(bSystem)
			OnSysKeyUp( nChar, 1, nFlags);
		else
			OnKeyUp( nChar, 1, nFlags);

		return;
	}

	if(bSystem)
	{
		OnSysKeyDown( nChar, 1, nFlags);
		return;
	}

	OnKeyDown( nChar, 1, nFlags);
	s_nCHARFLAGS = nFlags;

	// The control characters Windows sends after the key: SDL's text has none.
	UINT nControl = 0;

	switch(nChar)
	{
	case VK_BACK	: nControl = 0x08; break;
	case VK_TAB		: nControl = 0x09; break;
	case VK_ESCAPE	: nControl = 0x1B; break;
	case VK_RETURN	: nControl = (nMod & SDL_KMOD_CTRL) ? 0x0A : 0x0D; break;
	default			:
		if( (nMod & SDL_KMOD_CTRL) && nChar >= 'A' && nChar <= 'Z' )
			nControl = nChar - 'A' + 1;

		break;
	}

	if(nControl)
		OnChar( nControl, 1, nFlags);
}

void CTWindow::DispatchText( const SDL_Event& event)
{
	switch(event.type)
	{
	case SDL_EVENT_TEXT_EDITING				:
		CTachyonInput::SetImeComposition( event.edit.text, event.edit.start, event.edit.length);
		OnImeComposition( m_hWnd, 0, CTachyonInput::GetImeComposition().IsEmpty() ? 0 : GCS_COMPSTR|GCS_COMPATTR|GCS_COMPCLAUSE);

		break;

	case SDL_EVENT_TEXT_EDITING_CANDIDATES	:
		CTachyonInput::SetImeCandidates(
			event.edit_candidates.candidates,
			event.edit_candidates.num_candidates,
			event.edit_candidates.selected_candidate);

		OnImeNotify( m_hWnd, event.edit_candidates.num_candidates ? IMN_CHANGECANDIDATE : IMN_CLOSECANDIDATE, 0);
		break;

	case SDL_EVENT_TEXT_INPUT				:
		{
			BOOL bComposing = !CTachyonInput::GetImeComposition().IsEmpty();
			CString strText = CTachyonInput::FromUTF8(event.text.text);
			BOOL bDBCS = FALSE;

			for( int i=0; i<strText.GetLength() && !bDBCS; i++)
				bDBCS = CTCodePage::IsLeadByte(BYTE(strText[i]));

			// What the IME made is its result; a key's own character is a WM_CHAR per byte.
			if( bComposing || bDBCS )
			{
				CTachyonInput::SetImeResult(event.text.text);
				OnImeComposition( m_hWnd, 0, GCS_RESULTSTR);
			}
			else
			{
				for( int i=0; i<strText.GetLength(); i++)
					OnChar( BYTE(strText[i]), 1, s_nCHARFLAGS);
			}
		}

		break;
	}
}

void CTWindow::DispatchMouse( const SDL_Event& event)
{
	if( event.type == SDL_EVENT_MOUSE_WHEEL )
	{
		// The wheel position is in screen coordinates, like MFC's.
		CPoint point( int(event.wheel.mouse_x), int(event.wheel.mouse_y));
		float fDelta = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -event.wheel.y : event.wheel.y;

		ClientToScreen(&point);
		OnMouseWheel( GetMouseFlags(m_dwButtons), short(fDelta * WHEEL_DELTA), point);

		return;
	}

	// Like WM_SETCURSOR, before each mouse message: the handler picks the cursor image.
	if( event.type == SDL_EVENT_MOUSE_MOTION )
	{
		OnSetCursor( m_hWnd, HTCLIENT, WM_MOUSEMOVE);
		OnMouseMove( GetMouseFlags(m_dwButtons), CPoint( int(event.motion.x), int(event.motion.y)));

		return;
	}

	BOOL bDown = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
	BOOL bDouble = bDown && event.button.clicks >= 2 && !(event.button.clicks & 1);
	CPoint point( int(event.button.x), int(event.button.y));

	if(bDown)
		OnSetCursor( m_hWnd, HTCLIENT, event.button.button == SDL_BUTTON_LEFT ? WM_LBUTTONDOWN : event.button.button == SDL_BUTTON_RIGHT ? WM_RBUTTONDOWN : WM_MBUTTONDOWN);

	// The flags tell the buttons held after this one, like Windows.
	if(bDown)
		m_dwButtons |= SDL_BUTTON_MASK(event.button.button);
	else
		m_dwButtons &= ~SDL_BUTTON_MASK(event.button.button);
	UINT nFlags = GetMouseFlags(m_dwButtons);

	switch(event.button.button)
	{
	case SDL_BUTTON_LEFT	:
		if(!bDown)
			OnLButtonUp( nFlags, point);
		else if(bDouble)
			OnLButtonDblClk( nFlags, point);
		else
			OnLButtonDown( nFlags, point);

		break;

	case SDL_BUTTON_MIDDLE	:
		if(!bDown)
			OnMButtonUp( nFlags, point);
		else if(bDouble)
			OnMButtonDblClk( nFlags, point);
		else
			OnMButtonDown( nFlags, point);

		break;

	case SDL_BUTTON_RIGHT	:
		if(!bDown)
			OnRButtonUp( nFlags, point);
		else if(bDouble)
			OnRButtonDblClk( nFlags, point);
		else
			OnRButtonDown( nFlags, point);

		break;
	}
}
