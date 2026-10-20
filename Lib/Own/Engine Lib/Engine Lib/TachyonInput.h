#pragma once

struct SDL_Cursor;
struct SDL_Window;

// A mouse cursor image (CTachyonInput::LoadCursor).
typedef SDL_Cursor *HTCURSOR;

// Keyboard and mouse of the platform layer, on SDL. Key codes are Windows VK_
// values (the key settings store them), the key messages' flags carry the PC
// scan code of the key, and points are in the client area of the main window.
class CTachyonInput
{
public:
	static HWND m_hWND;
	static SDL_Window *m_pWINDOW;

	// Where the pointer cannot be warped (Wayland), SetCursorPos locks it in relative mode
	// until the buttons are released, and the engine moves this cursor instead.
	static BOOL m_bWarpLock;
	static CPoint m_vWarpPos;

public:
	// After SDL_Init: the scan code tables.
	static void InitPlatform();
	static void SetWindow( CTWindow *pWnd);

	// The VK_ code and the PC scan code (0xE0xx when extended) of an SDL key.
	static UINT GetVirtualKey( int nScancode, int nKeycode, int nMod);
	static WORD GetPCScanCode( int nScancode);

	static BYTE IsKeyDown( BYTE bVKey);
	static BYTE IsCtrlDown();
	static BYTE IsAltDown();
	static BYTE IsShiftDown();
	static BYTE IsWinDown();
	static BYTE IsCapsLock();

	static CPoint GetCursorPos();
	static void SetCursorPos( CPoint point);

	// The locked cursor (m_bWarpLock): moved by a motion's delta, released with the buttons.
	static CPoint MoveWarpPos( float fRelX, float fRelY);
	static void EndWarpLock();
	static void SetCapture( BOOL bCapture);

	// Like Win32: ShowCursor counts, SetCursor(NULL) hides the image.
	static void ShowCursor( BOOL bShow);
	static void SetCursor( HTCURSOR hCursor);

	// A .cur or an animated .ani file in memory.
	static HTCURSOR LoadCursor( LPCVOID pData, DWORD dwSize);
	static void DestroyCursor( HTCURSOR hCursor);

	static WORD ScanToVKey( UINT nFlags);
	static CHAR ScanToChar( UINT nFlags);

public:
	// Text input. The window keeps the IME state from its text events, in the ANSI code page
	// of the game's strings, the way IMM gave it: the composition with one attribute per byte
	// (ATTR_*) and its clauses (byte offsets), the last result, the candidates of the page.
	static void SetImeComposition( LPCSTR lpszUTF8, int nStart, int nLength);
	static void SetImeResult( LPCSTR lpszUTF8);
	static void SetImeCandidates( const char * const *pUTF8, int nCount, int nSelection);

	static CString GetImeComposition();
	static CString GetImeResult();
	static void GetImeCompositionAttr( BYTE *pAttr, INT *pAttrLength, DWORD *pClause, INT *pClauseLength);
	static const VECTORSTRING& GetImeCandidates();
	static int GetImeCandidateSelection();

	// The composition becomes the result (an edit losing the focus keeps it), or is dropped.
	static void CompleteImeComposition();
	static void CancelImeComposition();

	// The IME's native mode (Hangul, Kana...) or alphanumeric.
	static BOOL IsImeNativeMode();
	static void SetImeNativeMode( BOOL bNative);

	static CString FromUTF8( LPCSTR lpszUTF8);

protected:
	static void UpdateCursor();

	static HTCURSOR m_hCURSOR;
	static int m_nShowCursor;

	static CString m_strImeComposition;
	static CString m_strImeResult;
	static std::vector<BYTE> m_vImeAttr;
	static std::vector<DWORD> m_vImeClause;
	static VECTORSTRING m_vImeCandidate;
	static int m_nImeSelection;
};
