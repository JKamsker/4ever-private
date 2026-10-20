#include "StdAfx.h"
#include <SDL3/SDL.h>


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


// The virtual key of a Win32 key message for an SDL key.
UINT CTachyonInput::GetVirtualKey( int nScancode, int nKeycode, int nMod)
{
	SDL_Scancode nScan = SDL_Scancode(nScancode);
	SDL_Keycode nKey = SDL_Keycode(nKeycode);

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


HWND CTachyonInput::m_hWND = NULL;
SDL_Window *CTachyonInput::m_pWINDOW = NULL;
BOOL CTachyonInput::m_bWarpLock = FALSE;
CPoint CTachyonInput::m_vWarpPos( 0, 0);

// Wayland lets no client move the pointer, nor tells it where its window is.
static BOOL IsWaylandVideo()
{
	const char *pDRIVER = SDL_GetCurrentVideoDriver();
	return pDRIVER && !strcmp( pDRIVER, "wayland") ? TRUE : FALSE;
}

HTCURSOR CTachyonInput::m_hCURSOR = NULL;
int CTachyonInput::m_nShowCursor = 0;

CString CTachyonInput::m_strImeComposition;
CString CTachyonInput::m_strImeResult;
std::vector<BYTE> CTachyonInput::m_vImeAttr;
std::vector<DWORD> CTachyonInput::m_vImeClause;
VECTORSTRING CTachyonInput::m_vImeCandidate;
int CTachyonInput::m_nImeSelection = 0;

void CTachyonInput::InitPlatform()
{
	for( int i=0; i<256; i++)
		if( s_vWINSCAN[i] != SDL_SCANCODE_UNKNOWN && !s_vSCANCODE[s_vWINSCAN[i]] )
			s_vSCANCODE[s_vWINSCAN[i]] = WORD(i < 0x80 ? i : 0xE000 | (i - 0x80));

	// What Windows' key messages say for these two: Pause 0x45, Num Lock 0x45 extended.
	s_vSCANCODE[SDL_SCANCODE_PAUSE] = 0x45;
	s_vSCANCODE[SDL_SCANCODE_NUMLOCKCLEAR] = 0xE045;
}

void CTachyonInput::SetWindow( CTWindow *pWnd)
{
	m_hWND = pWnd ? pWnd->GetSafeHwnd() : NULL;
	m_pWINDOW = pWnd ? pWnd->GetSDLWindow() : NULL;
}

WORD CTachyonInput::GetPCScanCode( int nScancode)
{
	return nScancode > 0 && nScancode < SDL_SCANCODE_COUNT ? s_vSCANCODE[nScancode] : 0;
}

BYTE CTachyonInput::IsKeyDown( BYTE bVKey)
{
	const bool *pSTATE = SDL_GetKeyboardState(NULL);
	SDL_MouseButtonFlags dwButtons = SDL_GetMouseState( NULL, NULL);

	switch(bVKey)
	{
	case VK_LBUTTON		: return (dwButtons & SDL_BUTTON_LMASK) ? TRUE : FALSE;
	case VK_RBUTTON		: return (dwButtons & SDL_BUTTON_RMASK) ? TRUE : FALSE;
	case VK_MBUTTON		: return (dwButtons & SDL_BUTTON_MMASK) ? TRUE : FALSE;
	case VK_LCONTROL	: return pSTATE[SDL_SCANCODE_LCTRL] ? TRUE : FALSE;
	case VK_RCONTROL	: return pSTATE[SDL_SCANCODE_RCTRL] ? TRUE : FALSE;
	case VK_LSHIFT		: return pSTATE[SDL_SCANCODE_LSHIFT] ? TRUE : FALSE;
	case VK_RSHIFT		: return pSTATE[SDL_SCANCODE_RSHIFT] ? TRUE : FALSE;
	case VK_LMENU		: return pSTATE[SDL_SCANCODE_LALT] ? TRUE : FALSE;
	case VK_RMENU		: return pSTATE[SDL_SCANCODE_RALT] ? TRUE : FALSE;
	}

	SDL_Keymod nMod = SDL_GetModState();
	for( int i=1; i<SDL_SCANCODE_COUNT; i++)
		if( pSTATE[i] && GetVirtualKey( i, SDL_GetKeyFromScancode( SDL_Scancode(i), SDL_KMOD_NONE, false), nMod) == bVKey )
			return TRUE;

	return FALSE;
}

BYTE CTachyonInput::IsCtrlDown()
{
	return (SDL_GetModState() & SDL_KMOD_CTRL) ? TRUE : FALSE;
}

BYTE CTachyonInput::IsAltDown()
{
	return (SDL_GetModState() & SDL_KMOD_ALT) ? TRUE : FALSE;
}

BYTE CTachyonInput::IsShiftDown()
{
	return (SDL_GetModState() & SDL_KMOD_SHIFT) ? TRUE : FALSE;
}

BYTE CTachyonInput::IsWinDown()
{
	return (SDL_GetModState() & SDL_KMOD_GUI) ? TRUE : FALSE;
}

BYTE CTachyonInput::IsCapsLock()
{
	return (SDL_GetModState() & SDL_KMOD_CAPS) ? TRUE : FALSE;
}

CPoint CTachyonInput::GetCursorPos()
{
	float fX = 0.0f;
	float fY = 0.0f;
	int nX = 0;
	int nY = 0;

	if(m_bWarpLock)
		return m_vWarpPos;

	if(IsWaylandVideo())
	{
		SDL_GetMouseState( &fX, &fY);
		return CPoint( int(fX), int(fY));
	}

	// Outside the window too, like the Win32 call.
	SDL_GetGlobalMouseState( &fX, &fY);
	if(m_pWINDOW)
		SDL_GetWindowPosition( m_pWINDOW, &nX, &nY);

	return CPoint( int(fX) - nX, int(fY) - nY);
}

void CTachyonInput::SetCursorPos( CPoint point)
{
	if(!m_pWINDOW)
		return;

	if(!IsWaylandVideo())
	{
		SDL_WarpMouseInWindow( m_pWINDOW, float(point.x), float(point.y));
		return;
	}

	// The camera and the drags warp the cursor back while a button is held.
	if( !m_bWarpLock && SDL_GetMouseState( NULL, NULL) && SDL_SetWindowRelativeMouseMode( m_pWINDOW, true) )
		m_bWarpLock = TRUE;

	if(m_bWarpLock)
		m_vWarpPos = point;
}

CPoint CTachyonInput::MoveWarpPos( float fRelX, float fRelY)
{
	static float s_fRestX = 0.0f;
	static float s_fRestY = 0.0f;

	s_fRestX += fRelX;
	s_fRestY += fRelY;

	int nX = int(s_fRestX);
	int nY = int(s_fRestY);

	s_fRestX -= float(nX);
	s_fRestY -= float(nY);
	m_vWarpPos.Offset( nX, nY);

	return m_vWarpPos;
}

void CTachyonInput::EndWarpLock()
{
	if(!m_bWarpLock)
		return;

	m_bWarpLock = FALSE;
	if(m_pWINDOW)
		SDL_SetWindowRelativeMouseMode( m_pWINDOW, false);
}

void CTachyonInput::SetCapture( BOOL bCapture)
{
	SDL_CaptureMouse(bCapture ? true : false);
}

void CTachyonInput::ShowCursor( BOOL bShow)
{
	m_nShowCursor += bShow ? 1 : -1;
	UpdateCursor();
}

void CTachyonInput::SetCursor( HTCURSOR hCursor)
{
	m_hCURSOR = hCursor;
	UpdateCursor();
}

void CTachyonInput::UpdateCursor()
{
	if( m_nShowCursor >= 0 && m_hCURSOR )
	{
		// Setting the same cursor again would restart its animation.
		if( SDL_GetCursor() != m_hCURSOR )
			SDL_SetCursor(m_hCURSOR);

		if(!SDL_CursorVisible())
			SDL_ShowCursor();
	}
	else if(SDL_CursorVisible())
		SDL_HideCursor();
}

// One image of a .cur file: the first entry, a DIB of 1 to 32 bits with its AND mask.
static SDL_Surface *LoadCursorImage( const BYTE *pData, DWORD dwSize, int *pHotX, int *pHotY)
{
	if( dwSize < 22 || *(const WORD *) (pData + 2) != 2 )
		return NULL;

	const BYTE *pEntry = pData + 6;
	DWORD dwOffset = *(const DWORD *) (pEntry + 12);

	*pHotX = *(const WORD *) (pEntry + 4);
	*pHotY = *(const WORD *) (pEntry + 6);

	if( dwOffset + sizeof(BITMAPINFOHEADER) > dwSize )
		return NULL;

	const BITMAPINFOHEADER *pInfo = (const BITMAPINFOHEADER *) (pData + dwOffset);
	int nWidth = pInfo->biWidth;
	int nHeight = pInfo->biHeight / 2;
	int nBits = pInfo->biBitCount;
	int nColors = nBits <= 8 ? (pInfo->biClrUsed ? int(pInfo->biClrUsed) : 1 << nBits) : 0;

	const RGBQUAD *pPalette = (const RGBQUAD *) (pData + dwOffset + pInfo->biSize);
	const BYTE *pXOR = (const BYTE *) (pPalette + nColors);
	int nStride = ((nWidth * nBits + 31) / 32) * 4;
	const BYTE *pAND = pXOR + nStride * nHeight;
	int nMaskStride = ((nWidth + 31) / 32) * 4;

	if( nWidth <= 0 || nHeight <= 0 || pAND + nMaskStride * nHeight > pData + dwSize )
		return NULL;

	SDL_Surface *pSurface = SDL_CreateSurface( nWidth, nHeight, SDL_PIXELFORMAT_ARGB8888);
	if(!pSurface)
		return NULL;

	BOOL bAlpha = FALSE;
	if( nBits == 32 )
		for( int i=0; i<nWidth * nHeight && !bAlpha; i++)
			bAlpha = pXOR[i * 4 + 3] != 0;

	// Rows are bottom up. Without an alpha channel the mask decides: 1 is transparent.
	for( int y=0; y<nHeight; y++)
	{
		const BYTE *pROW = pXOR + nStride * (nHeight - 1 - y);
		const BYTE *pMASK = pAND + nMaskStride * (nHeight - 1 - y);
		Uint32 *pDEST = (Uint32 *) ((BYTE *) pSurface->pixels + pSurface->pitch * y);

		for( int x=0; x<nWidth; x++)
		{
			BYTE bB = 0;
			BYTE bG = 0;
			BYTE bR = 0;
			BYTE bA = 0xFF;

			switch(nBits)
			{
			case 32	: bB = pROW[x * 4]; bG = pROW[x * 4 + 1]; bR = pROW[x * 4 + 2]; bA = pROW[x * 4 + 3]; break;
			case 24	: bB = pROW[x * 3]; bG = pROW[x * 3 + 1]; bR = pROW[x * 3 + 2]; break;
			default	:
				{
					int nIndex = nBits == 8 ? pROW[x] : nBits == 4 ? (pROW[x / 2] >> (x & 1 ? 0 : 4)) & 0x0F : (pROW[x / 8] >> (7 - x % 8)) & 0x01;

					if( nIndex < nColors )
					{
						bB = pPalette[nIndex].rgbBlue;
						bG = pPalette[nIndex].rgbGreen;
						bR = pPalette[nIndex].rgbRed;
					}
				}

				break;
			}

			if(!bAlpha)
				bA = ((pMASK[x / 8] >> (7 - x % 8)) & 0x01) ? 0x00 : 0xFF;

			pDEST[x] = (Uint32(bA) << 24) | (Uint32(bR) << 16) | (Uint32(bG) << 8) | bB;
		}
	}

	return pSurface;
}

HTCURSOR CTachyonInput::LoadCursor( LPCVOID pData, DWORD dwSize)
{
	const BYTE *pBYTE = (const BYTE *) pData;
	int nHotX = 0;
	int nHotY = 0;

	if( dwSize < 12 || memcmp( pBYTE, "RIFF", 4) || memcmp( pBYTE + 8, "ACON", 4) )
	{
		SDL_Surface *pSurface = LoadCursorImage( pBYTE, dwSize, &nHotX, &nHotY);

		if(!pSurface)
			return NULL;

		HTCURSOR hCursor = SDL_CreateColorCursor( pSurface, nHotX, nHotY);
		SDL_DestroySurface(pSurface);

		return hCursor;
	}

	// An animated cursor: the frames ('icon' chunks of LIST 'fram'), shown in the order of 'seq '
	// for the times of 'rate', or of the header, in 1/60 s.
	std::vector<SDL_Surface *> vFRAME;
	std::vector<DWORD> vSEQ;
	std::vector<DWORD> vRATE;
	DWORD dwSteps = 0;
	DWORD dwRate = 0;

	for( DWORD dwPos = 12; dwPos + 8 <= dwSize;)
	{
		const BYTE *pCHUNK = pBYTE + dwPos;
		DWORD dwChunk = *(const DWORD *) (pCHUNK + 4);

		if( dwPos + 8 + dwChunk > dwSize )
			break;

		if( !memcmp( pCHUNK, "anih", 4) && dwChunk >= 36 )
		{
			dwSteps = *(const DWORD *) (pCHUNK + 8 + 8);
			dwRate = *(const DWORD *) (pCHUNK + 8 + 28);
		}
		else if(!memcmp( pCHUNK, "seq ", 4))
			vSEQ.assign( (const DWORD *) (pCHUNK + 8), (const DWORD *) (pCHUNK + 8) + dwChunk / 4);
		else if(!memcmp( pCHUNK, "rate", 4))
			vRATE.assign( (const DWORD *) (pCHUNK + 8), (const DWORD *) (pCHUNK + 8) + dwChunk / 4);
		else if( !memcmp( pCHUNK, "LIST", 4) && dwChunk >= 4 && !memcmp( pCHUNK + 8, "fram", 4) )
		{
			for( DWORD dwIcon = 12; dwIcon + 8 <= dwChunk + 8;)
			{
				const BYTE *pICON = pCHUNK + dwIcon;
				DWORD dwIconSize = *(const DWORD *) (pICON + 4);

				if( dwIcon + 8 + dwIconSize > dwChunk + 8 )
					break;

				if(!memcmp( pICON, "icon", 4))
				{
					int nX = 0;
					int nY = 0;
					SDL_Surface *pSurface = LoadCursorImage( pICON + 8, dwIconSize, &nX, &nY);

					if( pSurface && vFRAME.empty() )
					{
						nHotX = nX;
						nHotY = nY;
					}

					vFRAME.push_back(pSurface);
				}

				dwIcon += 8 + dwIconSize + (dwIconSize & 1);
			}
		}

		dwPos += 8 + dwChunk + (dwChunk & 1);
	}

	if(!dwSteps)
		dwSteps = DWORD(vFRAME.size());

	std::vector<SDL_CursorFrameInfo> vINFO;
	for( DWORD i=0; i<dwSteps; i++)
	{
		DWORD dwFrame = i < vSEQ.size() ? vSEQ[i] : i;
		DWORD dwTime = i < vRATE.size() ? vRATE[i] : dwRate;

		if( dwFrame < vFRAME.size() && vFRAME[dwFrame] )
		{
			SDL_CursorFrameInfo vFRAMEINFO = { vFRAME[dwFrame], Uint32(dwTime * 1000 / 60) };
			vINFO.push_back(vFRAMEINFO);
		}
	}

	HTCURSOR hCursor = vINFO.empty() ? NULL : SDL_CreateAnimatedCursor( &vINFO[0], int(vINFO.size()), nHotX, nHotY);
	for( size_t i=0; i<vFRAME.size(); i++)
		if(vFRAME[i])
			SDL_DestroySurface(vFRAME[i]);

	return hCursor;
}

void CTachyonInput::DestroyCursor( HTCURSOR hCursor)
{
	if(!hCursor)
		return;

	if( m_hCURSOR == hCursor )
		SetCursor(NULL);

	SDL_DestroyCursor(hCursor);
}

// nFlags is the scan code word of a key message (HIWORD of lParam). Like MapVirtualKey, the
// extended bit is not read: the keypad gives the moving keys.
WORD CTachyonInput::ScanToVKey( UINT nFlags)
{
	SDL_Scancode nScan = s_vWINSCAN[LOBYTE(nFlags) & 0x7F];
	return WORD(GetVirtualKey( nScan, SDL_GetKeyFromScancode( nScan, SDL_KMOD_NONE, false), SDL_KMOD_NONE));
}

CHAR CTachyonInput::ScanToChar( UINT nFlags)
{
	BYTE bUpper = IsCapsLock();

	if(IsShiftDown())
		bUpper = !bUpper;

	CHAR nChar[2] = {
		CHAR(ScanToVKey(nFlags)),
		NULL};

	if(!bUpper)
		_strlwr_s(nChar);

	return nChar[0];
}

// The game's strings are in its code page; SDL's text is UTF-8.
CString CTachyonInput::FromUTF8( LPCSTR lpszUTF8)
{
	CStringW strW;
	int nLength = CTCodePage::ToWide( lpszUTF8, -1, NULL, 0, TCP_UTF8);

	if( nLength <= 1 )
		return CString();

	CTCodePage::ToWide( lpszUTF8, -1, strW.GetBufferSetLength(nLength), nLength, TCP_UTF8);
	strW.ReleaseBuffer();

	return CString(strW);
}

void CTachyonInput::SetImeComposition( LPCSTR lpszUTF8, int nStart, int nLength)
{
	m_strImeComposition = FromUTF8(lpszUTF8);
	m_vImeAttr.assign( m_strImeComposition.GetLength(), BYTE(ATTR_INPUT));
	m_vImeClause.clear();

	if( m_strImeComposition.IsEmpty() )
		return;

	// SDL counts characters; IMM's attributes and clauses count bytes.
	CStringW strW(m_strImeComposition);
	int nEnd = nStart + nLength;

	if( nStart < 0 || nLength <= 0 || nEnd > strW.GetLength() )
	{
		m_vImeClause.push_back(0);
		m_vImeClause.push_back(DWORD(m_strImeComposition.GetLength()));

		return;
	}

	DWORD dwStart = DWORD(CString(strW.Left(nStart)).GetLength());
	DWORD dwEnd = DWORD(CString(strW.Left(nEnd)).GetLength());

	for( DWORD i=dwStart; i<dwEnd && i<m_vImeAttr.size(); i++)
		m_vImeAttr[i] = ATTR_TARGET_CONVERTED;

	m_vImeClause.push_back(0);
	if(dwStart)
		m_vImeClause.push_back(dwStart);

	m_vImeClause.push_back(dwEnd);
	if( dwEnd < DWORD(m_strImeComposition.GetLength()) )
		m_vImeClause.push_back(DWORD(m_strImeComposition.GetLength()));
}

void CTachyonInput::SetImeResult( LPCSTR lpszUTF8)
{
	m_strImeResult = FromUTF8(lpszUTF8);
	m_strImeComposition.Empty();
	m_vImeAttr.clear();
	m_vImeClause.clear();
}

void CTachyonInput::SetImeCandidates( const char * const *pUTF8, int nCount, int nSelection)
{
	m_vImeCandidate.clear();
	m_nImeSelection = nSelection < 0 ? 0 : nSelection;

	for( int i=0; i<nCount; i++)
		m_vImeCandidate.push_back(FromUTF8(pUTF8[i]));
}

CString CTachyonInput::GetImeComposition()
{
	return m_strImeComposition;
}

CString CTachyonInput::GetImeResult()
{
	return m_strImeResult;
}

void CTachyonInput::GetImeCompositionAttr( BYTE *pAttr, INT *pAttrLength, DWORD *pClause, INT *pClauseLength)
{
	if( pAttr && pAttrLength )
	{
		*pAttrLength = INT(m_vImeAttr.size());

		if(!m_vImeAttr.empty())
			memcpy( pAttr, &m_vImeAttr[0], m_vImeAttr.size());
		else
			pAttr[0] = 0;
	}

	if( pClause && pClauseLength )
	{
		*pClauseLength = INT(m_vImeClause.size());

		if(!m_vImeClause.empty())
			memcpy( pClause, &m_vImeClause[0], m_vImeClause.size() * sizeof(DWORD));
		else
			pClause[0] = 0;
	}
}

const VECTORSTRING& CTachyonInput::GetImeCandidates()
{
	return m_vImeCandidate;
}

int CTachyonInput::GetImeCandidateSelection()
{
	return m_nImeSelection;
}

void CTachyonInput::CompleteImeComposition()
{
	m_strImeResult = m_strImeComposition;
	CancelImeComposition();
}

void CTachyonInput::CancelImeComposition()
{
	m_strImeComposition.Empty();
	m_vImeAttr.clear();
	m_vImeClause.clear();

	SDL_Window *pWindow = SDL_GetKeyboardFocus();
	if(pWindow)
		SDL_ClearComposition(pWindow);
}

BOOL CTachyonInput::IsImeNativeMode()
{
	BOOL bResult = FALSE;

#ifndef _WIN32
	return FALSE;
#else
	if(!m_hWND)
		return FALSE;

	HIMC hIMC = ::ImmGetContext(m_hWND);
	DWORD dwConv = 0;
	DWORD dwSent = 0;

	if( hIMC && ::ImmGetConversionStatus( hIMC, &dwConv, &dwSent) )
		bResult = (dwConv & IME_CMODE_NATIVE) ? TRUE : FALSE;
	::ImmReleaseContext( m_hWND, hIMC);

	return bResult;
#endif
}

void CTachyonInput::SetImeNativeMode( BOOL bNative)
{
#ifdef _WIN32
	if(!m_hWND)
		return;

	HIMC hIMC = ::ImmGetContext(m_hWND);
	DWORD dwConv = 0;
	DWORD dwSent = 0;

	if( hIMC && ::ImmGetConversionStatus( hIMC, &dwConv, &dwSent) )
	{
		dwConv &= ~IME_CMODE_LANGUAGE;
		if(bNative)
			dwConv |= IME_CMODE_NATIVE;

		::ImmSetConversionStatus( hIMC, dwConv, dwSent);
	}
	::ImmReleaseContext( m_hWND, hIMC);
#endif
}
