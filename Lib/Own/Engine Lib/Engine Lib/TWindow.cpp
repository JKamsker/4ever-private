// TWindow.cpp: implementation of the CTWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <windowsx.h>
#include <SDL3/SDL.h>

#define TWINDOW_PROP				"TachyonWnd"
#define TWINDOW_PROPW				L"TachyonWnd"


// The Win32 scan code of each SDL scancode is the index in this table, copied from SDL's
// src/events/scancodes_windows.h: 0x00-0x7F, then 0xE000-0xE07F (extended keys).
static const SDL_Scancode s_vWINSCAN[256] = {
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_ESCAPE, SDL_SCANCODE_1, SDL_SCANCODE_2,
	SDL_SCANCODE_3, SDL_SCANCODE_4, SDL_SCANCODE_5, SDL_SCANCODE_6,
	SDL_SCANCODE_7, SDL_SCANCODE_8, SDL_SCANCODE_9, SDL_SCANCODE_0,
	SDL_SCANCODE_MINUS, SDL_SCANCODE_EQUALS, SDL_SCANCODE_BACKSPACE, SDL_SCANCODE_TAB,
	SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_R,
	SDL_SCANCODE_T, SDL_SCANCODE_Y, SDL_SCANCODE_U, SDL_SCANCODE_I,
	SDL_SCANCODE_O, SDL_SCANCODE_P, SDL_SCANCODE_LEFTBRACKET, SDL_SCANCODE_RIGHTBRACKET,
	SDL_SCANCODE_RETURN, SDL_SCANCODE_LCTRL, SDL_SCANCODE_A, SDL_SCANCODE_S,
	SDL_SCANCODE_D, SDL_SCANCODE_F, SDL_SCANCODE_G, SDL_SCANCODE_H,
	SDL_SCANCODE_J, SDL_SCANCODE_K, SDL_SCANCODE_L, SDL_SCANCODE_SEMICOLON,
	SDL_SCANCODE_APOSTROPHE, SDL_SCANCODE_GRAVE, SDL_SCANCODE_LSHIFT, SDL_SCANCODE_BACKSLASH,
	SDL_SCANCODE_Z, SDL_SCANCODE_X, SDL_SCANCODE_C, SDL_SCANCODE_V,
	SDL_SCANCODE_B, SDL_SCANCODE_N, SDL_SCANCODE_M, SDL_SCANCODE_COMMA,
	SDL_SCANCODE_PERIOD, SDL_SCANCODE_SLASH, SDL_SCANCODE_RSHIFT, SDL_SCANCODE_KP_MULTIPLY,
	SDL_SCANCODE_LALT, SDL_SCANCODE_SPACE, SDL_SCANCODE_CAPSLOCK, SDL_SCANCODE_F1,
	SDL_SCANCODE_F2, SDL_SCANCODE_F3, SDL_SCANCODE_F4, SDL_SCANCODE_F5,
	SDL_SCANCODE_F6, SDL_SCANCODE_F7, SDL_SCANCODE_F8, SDL_SCANCODE_F9,
	SDL_SCANCODE_F10, SDL_SCANCODE_NUMLOCKCLEAR, SDL_SCANCODE_SCROLLLOCK, SDL_SCANCODE_KP_7,
	SDL_SCANCODE_KP_8, SDL_SCANCODE_KP_9, SDL_SCANCODE_KP_MINUS, SDL_SCANCODE_KP_4,
	SDL_SCANCODE_KP_5, SDL_SCANCODE_KP_6, SDL_SCANCODE_KP_PLUS, SDL_SCANCODE_KP_1,
	SDL_SCANCODE_KP_2, SDL_SCANCODE_KP_3, SDL_SCANCODE_KP_0, SDL_SCANCODE_KP_PERIOD,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_NONUSBACKSLASH, SDL_SCANCODE_F11,
	SDL_SCANCODE_F12, SDL_SCANCODE_KP_EQUALS, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_INTERNATIONAL6, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_F13, SDL_SCANCODE_F14, SDL_SCANCODE_F15, SDL_SCANCODE_F16,
	SDL_SCANCODE_F17, SDL_SCANCODE_F18, SDL_SCANCODE_F19, SDL_SCANCODE_F20,
	SDL_SCANCODE_F21, SDL_SCANCODE_F22, SDL_SCANCODE_F23, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_INTERNATIONAL2, SDL_SCANCODE_LANG2, SDL_SCANCODE_LANG1, SDL_SCANCODE_INTERNATIONAL1,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_F24, SDL_SCANCODE_LANG4,
	SDL_SCANCODE_LANG3, SDL_SCANCODE_INTERNATIONAL4, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_INTERNATIONAL5,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_INTERNATIONAL3, SDL_SCANCODE_KP_COMMA, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_AGAIN,
	SDL_SCANCODE_UNDO, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_PASTE, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_MEDIA_PREVIOUS_TRACK, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_CUT,
	SDL_SCANCODE_COPY, SDL_SCANCODE_MEDIA_NEXT_TRACK, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_KP_ENTER, SDL_SCANCODE_RCTRL, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_MUTE, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_MEDIA_PLAY_PAUSE, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_MEDIA_STOP, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_MEDIA_EJECT, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_VOLUMEDOWN, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_VOLUMEUP, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_AC_HOME, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_KP_DIVIDE, SDL_SCANCODE_RSHIFT, SDL_SCANCODE_PRINTSCREEN,
	SDL_SCANCODE_RALT, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_HELP,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_AC_NEW, SDL_SCANCODE_AC_OPEN,
	SDL_SCANCODE_AC_CLOSE, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_NUMLOCKCLEAR, SDL_SCANCODE_PAUSE, SDL_SCANCODE_HOME,
	SDL_SCANCODE_UP, SDL_SCANCODE_PAGEUP, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_LEFT,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_END,
	SDL_SCANCODE_DOWN, SDL_SCANCODE_PAGEDOWN, SDL_SCANCODE_INSERT, SDL_SCANCODE_DELETE,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_AC_SAVE,
	SDL_SCANCODE_AC_PRINT, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_LGUI,
	SDL_SCANCODE_RGUI, SDL_SCANCODE_APPLICATION, SDL_SCANCODE_POWER, SDL_SCANCODE_SLEEP,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_WAKE,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_AC_SEARCH, SDL_SCANCODE_AC_BOOKMARKS, SDL_SCANCODE_AC_REFRESH,
	SDL_SCANCODE_AC_STOP, SDL_SCANCODE_AC_FORWARD, SDL_SCANCODE_AC_BACK, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_MEDIA_SELECT, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN
};

static WORD s_vSCANCODE[SDL_SCANCODE_COUNT];
static Uint32 s_dwMSGEVENT = 0;
static WORD s_wPROCESSKEY = 0;
static UINT s_nCHARFLAGS = 0;
static BOOL s_bQUIT = FALSE;


// The virtual key of a Win32 key message for an SDL key.
static UINT GetVirtualKey( SDL_Scancode nScan, SDL_Keycode nKey, SDL_Keymod nMod)
{
	// Letters follow the keyboard layout, like Windows' virtual keys.
	if( nKey >= 'a' && nKey <= 'z' )
		return UINT(nKey - 'a' + 'A');

	if( nScan >= SDL_SCANCODE_1 && nScan <= SDL_SCANCODE_9 )
		return UINT(nScan - SDL_SCANCODE_1 + '1');

	if( nScan >= SDL_SCANCODE_F1 && nScan <= SDL_SCANCODE_F12 )
		return UINT(nScan - SDL_SCANCODE_F1 + VK_F1);

	if( nScan >= SDL_SCANCODE_F13 && nScan <= SDL_SCANCODE_F24 )
		return UINT(nScan - SDL_SCANCODE_F13 + VK_F13);

	// The keypad without Num Lock moves, like Windows.
	if(!(nMod & SDL_KMOD_NUM))
	{
		switch(nScan)
		{
		case SDL_SCANCODE_KP_7			: return VK_HOME;
		case SDL_SCANCODE_KP_8			: return VK_UP;
		case SDL_SCANCODE_KP_9			: return VK_PRIOR;
		case SDL_SCANCODE_KP_4			: return VK_LEFT;
		case SDL_SCANCODE_KP_5			: return VK_CLEAR;
		case SDL_SCANCODE_KP_6			: return VK_RIGHT;
		case SDL_SCANCODE_KP_1			: return VK_END;
		case SDL_SCANCODE_KP_2			: return VK_DOWN;
		case SDL_SCANCODE_KP_3			: return VK_NEXT;
		case SDL_SCANCODE_KP_0			: return VK_INSERT;
		case SDL_SCANCODE_KP_PERIOD		: return VK_DELETE;
		default							: break;
		}
	}

	if( nScan >= SDL_SCANCODE_KP_1 && nScan <= SDL_SCANCODE_KP_9 )
		return UINT(nScan - SDL_SCANCODE_KP_1 + VK_NUMPAD1);

	switch(nScan)
	{
	case SDL_SCANCODE_0					: return '0';
	case SDL_SCANCODE_RETURN			:
	case SDL_SCANCODE_KP_ENTER			: return VK_RETURN;
	case SDL_SCANCODE_ESCAPE			: return VK_ESCAPE;
	case SDL_SCANCODE_BACKSPACE			: return VK_BACK;
	case SDL_SCANCODE_TAB				: return VK_TAB;
	case SDL_SCANCODE_SPACE				: return VK_SPACE;
	case SDL_SCANCODE_MINUS				: return VK_OEM_MINUS;
	case SDL_SCANCODE_EQUALS			: return VK_OEM_PLUS;
	case SDL_SCANCODE_LEFTBRACKET		: return VK_OEM_4;
	case SDL_SCANCODE_RIGHTBRACKET		: return VK_OEM_6;
	case SDL_SCANCODE_BACKSLASH			:
	case SDL_SCANCODE_NONUSHASH			: return VK_OEM_5;
	case SDL_SCANCODE_SEMICOLON			: return VK_OEM_1;
	case SDL_SCANCODE_APOSTROPHE		: return VK_OEM_7;
	case SDL_SCANCODE_GRAVE				: return VK_OEM_3;
	case SDL_SCANCODE_COMMA				: return VK_OEM_COMMA;
	case SDL_SCANCODE_PERIOD			: return VK_OEM_PERIOD;
	case SDL_SCANCODE_SLASH				: return VK_OEM_2;
	case SDL_SCANCODE_NONUSBACKSLASH	: return VK_OEM_102;
	case SDL_SCANCODE_CAPSLOCK			: return VK_CAPITAL;
	case SDL_SCANCODE_PRINTSCREEN		: return VK_SNAPSHOT;
	case SDL_SCANCODE_SCROLLLOCK		: return VK_SCROLL;
	case SDL_SCANCODE_PAUSE				: return VK_PAUSE;
	case SDL_SCANCODE_INSERT			: return VK_INSERT;
	case SDL_SCANCODE_HOME				: return VK_HOME;
	case SDL_SCANCODE_PAGEUP			: return VK_PRIOR;
	case SDL_SCANCODE_DELETE			: return VK_DELETE;
	case SDL_SCANCODE_END				: return VK_END;
	case SDL_SCANCODE_PAGEDOWN			: return VK_NEXT;
	case SDL_SCANCODE_RIGHT				: return VK_RIGHT;
	case SDL_SCANCODE_LEFT				: return VK_LEFT;
	case SDL_SCANCODE_DOWN				: return VK_DOWN;
	case SDL_SCANCODE_UP				: return VK_UP;
	case SDL_SCANCODE_NUMLOCKCLEAR		: return VK_NUMLOCK;
	case SDL_SCANCODE_KP_DIVIDE			: return VK_DIVIDE;
	case SDL_SCANCODE_KP_MULTIPLY		: return VK_MULTIPLY;
	case SDL_SCANCODE_KP_MINUS			: return VK_SUBTRACT;
	case SDL_SCANCODE_KP_PLUS			: return VK_ADD;
	case SDL_SCANCODE_KP_0				: return VK_NUMPAD0;
	case SDL_SCANCODE_KP_PERIOD			: return VK_DECIMAL;
	case SDL_SCANCODE_APPLICATION		: return VK_APPS;
	case SDL_SCANCODE_LCTRL				:
	case SDL_SCANCODE_RCTRL				: return VK_CONTROL;
	case SDL_SCANCODE_LSHIFT			:
	case SDL_SCANCODE_RSHIFT			: return VK_SHIFT;
	case SDL_SCANCODE_LALT				:
	case SDL_SCANCODE_RALT				: return VK_MENU;
	case SDL_SCANCODE_LGUI				: return VK_LWIN;
	case SDL_SCANCODE_RGUI				: return VK_RWIN;
	case SDL_SCANCODE_LANG1				: return VK_HANGUL;
	case SDL_SCANCODE_LANG2				: return VK_HANJA;
	default								: break;
	}

	return 0;
}

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
	case WM_SETCURSOR				:
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

	for( int i=0; i<256; i++)
		if( s_vWINSCAN[i] != SDL_SCANCODE_UNKNOWN && !s_vSCANCODE[s_vWINSCAN[i]] )
			s_vSCANCODE[s_vWINSCAN[i]] = WORD(i < 0x80 ? i : 0xE000 | (i - 0x80));

	// What Windows' key messages say for these two: Pause 0x45, Num Lock 0x45 extended.
	s_vSCANCODE[SDL_SCANCODE_PAUSE] = 0x45;
	s_vSCANCODE[SDL_SCANCODE_NUMLOCKCLEAR] = 0xE045;

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

BOOL CTWindow::CreateEx( DWORD dwExStyle,
						 LPCTSTR lpszTitle,
						 DWORD dwStyle,
						 const RECT& rect,
						 HWND hParent)
{
	CRect rcFRAME( 0, 0, 0, 0);
	CStringW strTitleW(lpszTitle);
	CStringA strTitle;

	// The Win32 style gives the frame: the client size is what the window keeps.
	::AdjustWindowRectEx( &rcFRAME, dwStyle, FALSE, dwExStyle);

	SDL_WindowFlags dwFlags = SDL_WINDOW_HIDDEN;
	if( (dwStyle & WS_CAPTION) != WS_CAPTION )
		dwFlags |= SDL_WINDOW_BORDERLESS;

	if( dwExStyle & WS_EX_TOPMOST )
		dwFlags |= SDL_WINDOW_ALWAYS_ON_TOP;

	int nLength = ::WideCharToMultiByte( CP_UTF8, 0, strTitleW, -1, NULL, 0, NULL, NULL);
	::WideCharToMultiByte( CP_UTF8, 0, strTitleW, -1, strTitle.GetBufferSetLength(nLength), nLength, NULL, NULL);
	strTitle.ReleaseBuffer();

	m_pSDL = SDL_CreateWindow(
		strTitle,
		rect.right - rect.left - rcFRAME.Width(),
		rect.bottom - rect.top - rcFRAME.Height(),
		dwFlags);

	if(!m_pSDL)
		return FALSE;

	SDL_SetPointerProperty( SDL_GetWindowProperties(m_pSDL), TWINDOW_PROP, this);
	m_hWnd = (HWND) SDL_GetPointerProperty( SDL_GetWindowProperties(m_pSDL), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

	::SetPropW( m_hWnd, TWINDOW_PROPW, this);
	m_pSDLProc = (WNDPROC) ::SetWindowLongPtrW( m_hWnd, GWLP_WNDPROC, (LONG_PTR) SubclassProc);

	// SDL keeps the IME off until text input starts.
	SDL_StartTextInput(m_pSDL);
	SDL_SetWindowPosition( m_pSDL, rect.left - rcFRAME.left, rect.top - rcFRAME.top);

	if( dwStyle & WS_VISIBLE )
		SDL_ShowWindow(m_pSDL);

	return TRUE;
}

void CTWindow::GetWindowText( CString& strText) const
{
	int nLength = ::GetWindowTextLength(m_hWnd);

	::GetWindowText( m_hWnd, strText.GetBufferSetLength(nLength + 1), nLength + 1);
	strText.ReleaseBuffer();
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
	case WM_SETCURSOR		: return OnSetCursor( (HWND) wParam, LOWORD(lParam), HIWORD(lParam));
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
	WORD wScan = s_vSCANCODE[event.key.scancode];

	UINT nChar = GetVirtualKey( event.key.scancode, event.key.key, nMod);
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
				bDBCS = IsDBCSLeadByte(BYTE(strText[i]));

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

	if( event.type == SDL_EVENT_MOUSE_MOTION )
	{
		OnMouseMove( GetMouseFlags(m_dwButtons), CPoint( int(event.motion.x), int(event.motion.y)));
		return;
	}

	BOOL bDown = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
	BOOL bDouble = bDown && event.button.clicks >= 2 && !(event.button.clicks & 1);
	CPoint point( int(event.button.x), int(event.button.y));

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
