// TachyonWnd.h: interface for the CTachyonWnd class.
//
//////////////////////////////////////////////////////////////////////

#if !defined __TACHYONWND_H
#define __TACHYONWND_H


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#define ON_GM_COMMAND(x)			case x	: nERROR = On##x(); break;

class CTachyonWnd : public CTWindow
{
public:
	CTachyonWnd();
	virtual ~CTachyonWnd();

// Attributes
public:
	VTCOMMAND m_vCOMMAND;
	CD3DDevice m_Device;

	BYTE m_bDirectLogin;
	BYTE m_bSiteCode;
	BYTE m_bActivate;
	BYTE m_bOnMovie;

	CString m_strIPAddr;
	CString m_strID;
	CString m_strPW;

	DWORD m_dwPort;
	static DWORD m_dwPing;

protected:
	static DWORD WINAPI __DeviceCtrl( LPVOID lpParam);

	static BOOL CALLBACK __Present(
		HWND hWnd,
		LPARAM lParam);

// Operations
public:
	virtual void OnSessionMsg( CTachyonSession *pSession, CPacket *pPacket);
	virtual void OnConnect( CTachyonSession *pSession, int nErrorCode);
	virtual void OnClose( CTachyonSession *pSession, int nErrorCode);

	virtual void ProcessHotCommand( int nID);
	virtual void OnCommand( TCOMMAND cmd);
	virtual void Render();
	virtual void DrawFPS(
		DWORD dwPing,
		int nPosX,
		int nPosY);

	virtual void OnChar(
		UINT nChar,
		UINT nRep,
		UINT nFlags);

	virtual void OnImeComposition(
		HWND hWnd,
		WPARAM wParam,
		LPARAM lParam);

	virtual void OnImeNotify(
		HWND hWnd,
		WPARAM wParam,
		LPARAM lParam);

	virtual BOOL OnInputLangChangeRequest(
		HWND hWnd,
		WPARAM wParam,
		LPARAM lParam);

	virtual void InitResource(
		CString strGroupID,
		BYTE bGroupID);

	virtual void ReleaseThreadDevices();
	virtual BOOL ResetThreadDevices();
	virtual BOOL InitThreadDevices();

	virtual BOOL ResetDevices();
	virtual BOOL InitDevices();
	virtual BOOL Present();

	virtual BOOL AdjustWindowSize( INT nWidth, INT nHeight);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTachyonWnd)
	public:
	virtual BOOL Create();
	//}}AFX_VIRTUAL

// Generated message map functions
protected:



	void SetDeviceThreadFlag( BYTE bRUN);
	BYTE GetDeviceThreadFlag();

public:
	CTachyonRes m_RES;

	TCMLParser *m_pTParser;
	TFrame *m_pMainFrame;

	CD3DFont m_FontFPS;
	CFont m_Font;

	HANDLE m_hDEVICE;
	HANDLE m_hRESET;
	HANDLE m_hINIT;
	BYTE m_bRUN;

public:
	virtual void OnActivate(UINT nState, HWND hWndOther, BOOL bMinimized);
	virtual void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual void OnDestroy();
	virtual void OnNcDestroy();
	virtual LRESULT OnNcHitTest(CPoint point);
	virtual void OnNcRButtonDown(UINT nHitTest, CPoint point);
};


#endif // !defined __TACHYONWND_H
