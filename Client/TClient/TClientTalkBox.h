#pragma once


class CTClientTalkBox : public CD3DFont
{
public:
	static LPTEXTURESET m_pSKIN;

	static CT3DVertex m_vVB;
	static CT3DIndex m_vIB;
	static CFont *m_pFONT;

public:
	D3DXMATRIX m_vWORLD;

public:
	static void ReleaseTalkBox();
	static void InitTalkBox(
		LPTRENDERDEVICE pDevice,
		CTachyonRes *pRES);

public:
	void SetTALK(
		LPTRENDERDEVICE pDevice,
		CString& strTALK);

	void Render(
		LPTRENDERDEVICE pDevice,
		LPD3DXVECTOR3 pPOS);

	CRect GetRect();

public:
	CTClientTalkBox();
	virtual ~CTClientTalkBox();
};
