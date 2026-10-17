#pragma once

// Keyboard and mouse queries of the platform layer. Every OS call for key
// states, the cursor position and the scan code mapping goes through here so
// the SDL3 port only has to replace this file. Key codes are Windows VK_
// values (the key settings store them) and points are in the client area of
// the main window.
class CTachyonInput
{
public:
	static HWND m_hWND;

public:
	static void SetWindow( HWND hWND);

	static BYTE IsKeyDown( BYTE bVKey);
	static BYTE IsCtrlDown();
	static BYTE IsAltDown();
	static BYTE IsShiftDown();
	static BYTE IsWinDown();
	static BYTE IsCapsLock();

	static CPoint GetCursorPos();
	static void SetCursorPos( CPoint point);
	static void ShowCursor( BOOL bShow);

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
	static CString m_strImeComposition;
	static CString m_strImeResult;
	static std::vector<BYTE> m_vImeAttr;
	static std::vector<DWORD> m_vImeClause;
	static VECTORSTRING m_vImeCandidate;
	static int m_nImeSelection;
};
