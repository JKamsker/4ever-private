#include "StdAfx.h"
#include <SDL3/SDL.h>


HWND CTachyonInput::m_hWND = NULL;

CString CTachyonInput::m_strImeComposition;
CString CTachyonInput::m_strImeResult;
std::vector<BYTE> CTachyonInput::m_vImeAttr;
std::vector<DWORD> CTachyonInput::m_vImeClause;
VECTORSTRING CTachyonInput::m_vImeCandidate;
int CTachyonInput::m_nImeSelection = 0;

void CTachyonInput::SetWindow( HWND hWND)
{
	m_hWND = hWND;
}

BYTE CTachyonInput::IsKeyDown( BYTE bVKey)
{
	return GetAsyncKeyState(bVKey) < 0 ? TRUE : FALSE;
}

BYTE CTachyonInput::IsCtrlDown()
{
	return GetKeyState(VK_CONTROL) < 0 ? TRUE : FALSE;
}

BYTE CTachyonInput::IsAltDown()
{
	return GetKeyState(VK_MENU) < 0 ? TRUE : FALSE;
}

BYTE CTachyonInput::IsShiftDown()
{
	return GetKeyState(VK_SHIFT) < 0 ? TRUE : FALSE;
}

BYTE CTachyonInput::IsWinDown()
{
	return GetKeyState(VK_LWIN) < 0 || GetKeyState(VK_RWIN) < 0 ? TRUE : FALSE;
}

BYTE CTachyonInput::IsCapsLock()
{
	return GetKeyState(VK_CAPITAL) & 0x0001 ? TRUE : FALSE;
}

CPoint CTachyonInput::GetCursorPos()
{
	CPoint point( 0, 0);

	::GetCursorPos(&point);
	if(m_hWND)
		::ScreenToClient( m_hWND, &point);

	return point;
}

void CTachyonInput::SetCursorPos( CPoint point)
{
	if(m_hWND)
		::ClientToScreen( m_hWND, &point);

	::SetCursorPos( point.x, point.y);
}

void CTachyonInput::ShowCursor( BOOL bShow)
{
	::ShowCursor(bShow);
}

// nFlags is the scan code word of a key message (HIWORD of lParam).
WORD CTachyonInput::ScanToVKey( UINT nFlags)
{
	return (WORD) MapVirtualKey( LOBYTE(nFlags), MAPVK_VSC_TO_VK);
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

// The game's strings are in the ANSI code page; SDL's text is UTF-8.
CString CTachyonInput::FromUTF8( LPCSTR lpszUTF8)
{
	CStringW strW;
	int nLength = ::MultiByteToWideChar( CP_UTF8, 0, lpszUTF8, -1, NULL, 0);

	if( nLength <= 1 )
		return CString();

	::MultiByteToWideChar( CP_UTF8, 0, lpszUTF8, -1, strW.GetBufferSetLength(nLength), nLength);
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

	if(!m_hWND)
		return FALSE;

	HIMC hIMC = ::ImmGetContext(m_hWND);
	DWORD dwConv = 0;
	DWORD dwSent = 0;

	if( hIMC && ::ImmGetConversionStatus( hIMC, &dwConv, &dwSent) )
		bResult = (dwConv & IME_CMODE_NATIVE) ? TRUE : FALSE;
	::ImmReleaseContext( m_hWND, hIMC);

	return bResult;
}

void CTachyonInput::SetImeNativeMode( BOOL bNative)
{
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
}
