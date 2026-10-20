// TShow.h: interface for the CDShow class.
//
//////////////////////////////////////////////////////////////////////

#if !defined __TSHOW_H
#define __TSHOW_H


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#ifdef _WIN32
class CTShow
{
public:
	static BYTE m_bMasterVolume;
	static BYTE m_bON;

protected:
	IGraphBuilder* m_pGB;
	IMediaSeeking* m_pMS;
	IMediaControl* m_pMC;
	IMediaEventEx* m_pME;
	IVideoWindow* m_pVW;

	CTWindow *m_pHost;
	CTWindow *m_pWnd;

public:
	CString m_strFILE;
	DWORD m_dwMessage;
	BYTE m_bVolume;

public:
	BYTE SetPos( LONGLONG nPos);

	LONGLONG GetLength();
	LONGLONG GetPos();

	BYTE ResetVolume();
	BYTE ResetGB();
	BYTE IsPlay();
	BYTE Toggle();
	BYTE Pause();
	BYTE Play();
	BYTE Stop();

public:
	void InitDShow(
		CTWindow *pHost,
		CTWindow *pWnd,
		LPTSTR szFile,
		DWORD dwMessage);

	void OnWindowPosition();
	void Release();

public:
	CTShow();
	virtual ~CTShow();
};
#else

// DirectShow plays the videos: on the other systems there is none, as if the file were missing.
class CTShow
{
public:
	static BYTE m_bMasterVolume;
	static BYTE m_bON;

public:
	CString m_strFILE;
	DWORD m_dwMessage;
	BYTE m_bVolume;

public:
	BYTE SetPos( LONGLONG nPos) { return FALSE; }
	LONGLONG GetLength() { return 0; }
	LONGLONG GetPos() { return 0; }
	BYTE ResetVolume() { return FALSE; }
	BYTE ResetGB() { return FALSE; }
	BYTE IsPlay() { return FALSE; }
	BYTE Toggle() { return FALSE; }
	BYTE Pause() { return FALSE; }
	BYTE Play() { return FALSE; }
	BYTE Stop() { return FALSE; }

public:
	void InitDShow(
		CTWindow *pHost,
		CTWindow *pWnd,
		LPTSTR szFile,
		DWORD dwMessage) { m_strFILE = szFile; m_dwMessage = dwMessage; }
	void OnWindowPosition() {}
	void Release() {}

public:
	CTShow() : m_dwMessage(0), m_bVolume(0) {}
	virtual ~CTShow() {}
};
#endif


#endif // !defined __TSHOW_H
