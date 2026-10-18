// D3DFont.cpp: implementation of the CD3DFont class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

int CD3DFont::m_nRenderingHint = TTEXTHINT_ANTIALIAS;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CD3DFont::CD3DFont()
{
	m_pTEX[0] = NULL;
	m_pTEX[1] = NULL;

	m_pVB3D = NULL;
	m_pVB = NULL;

	m_strTEXT = _T("");
	m_nPartCount = 0;
	m_nFormat = 0;
	m_nHeight = 0;
	m_nWidth = 0;
	m_nPosX = 0;
	m_nPosY = 0;

	m_ppPosBufX = NULL;
	m_ppPosBufY = NULL;

	m_fScaleX = 1.0f;
	m_fScaleY = 1.0f;
	m_fPrvScaleX = 1.0f;
	m_fPrvScaleY = 1.0f;

	m_dwLineColor = 0xFF000000;
	m_dwColor = 0xFFFFFFFF;

	m_bOutLine = TRUE;
	m_bShadow = FALSE;
}

CD3DFont::~CD3DFont()
{
	Release();
}

void CD3DFont::Release()
{
	if(m_pTEX[0])
	{
		for( int i=0; i<m_nPartCount; i++)
			SAFE_RELEASE(m_pTEX[0][i]);

		delete[] m_pTEX[0];
		m_pTEX[0] = NULL;
	}

	if(m_pTEX[1])
	{
		for( int i=0; i<m_nPartCount; i++)
			SAFE_RELEASE(m_pTEX[1][i]);

		delete[] m_pTEX[1];
		m_pTEX[1] = NULL;
	}

	if(m_pVB3D)
	{
		for( int i=0; i<m_nPartCount; i++)
			SAFE_RELEASE(m_pVB3D[i]);

		delete[] m_pVB3D;
		m_pVB3D = NULL;
	}

	if(m_pVB)
	{
		for( int i=0; i<m_nPartCount; i++)
			SAFE_RELEASE(m_pVB[i]);

		delete[] m_pVB;
		m_pVB = NULL;
	}

	if(m_ppPosBufX)
	{
		for( int i=0; i<m_nPartCount; i++)
			if(m_ppPosBufX[i])
				delete[] m_ppPosBufX[i];

		delete[] m_ppPosBufX;
		m_ppPosBufX = NULL;
	}

	if(m_ppPosBufY)
	{
		for( int i=0; i<m_nPartCount; i++)
			if(m_ppPosBufY[i])
				delete[] m_ppPosBufY[i];

		delete[] m_ppPosBufY;
		m_ppPosBufY = NULL;
	}

	m_strTEXT = _T("");
	m_nPartCount = 0;
	m_nHeight = 0;
	m_nWidth = 0;
	m_nPosX = 0;
	m_nPosY = 0;
}

void CD3DFont::SetFont( CFont *pFont)
{
	m_pFont = pFont;
}

void CD3DFont::MakeText( LPTRENDERDEVICE pDevice,
						 CString strText,
						 CRect& rect,
						 UINT nFormat,
						 CPoint Hide)
{
	if( m_nFormat == (nFormat&~DT_CALCRECT) &&
		m_strTEXT == strText )
		return;

	WCHAR pBUF[TGDIBUFFER_SIZE];
	Release();

	CTCodePage::ToWide(
		LPCSTR(strText),
		strText.GetLength() + 1,
		pBUF, TGDIBUFFER_SIZE);

	if(nFormat&DT_CALCRECT)
	{
		FLOAT fWidth = 0.0f;
		FLOAT fHeight = 0.0f;

		CTTextRaster::Measure(
			m_pFont,
			pBUF,
			FALSE,
			m_nRenderingHint,
			&fWidth,
			&fHeight);

		rect.bottom = INT(rect.top + fHeight - 1) - Hide.y;
		rect.right = INT(rect.left + fWidth + 1) - Hide.x;

		nFormat &= ~DT_CALCRECT;
	}

	m_nHeight = rect.Height() - Hide.y;
	m_nWidth = rect.Width() - Hide.x;

	int nPartCountX = 0;
	int nPartCountY = 0;
	int nHeight = 0;
	int nWidth = 0;

	while( nHeight < m_nHeight )
	{
		nHeight += CTMath::GetNearPow(m_nHeight - nHeight);
		nPartCountY++;
	}

	while( nWidth < m_nWidth )
	{
		nWidth += CTMath::GetNearPow(m_nWidth - nWidth);
		nPartCountX++;
	}
	m_nPartCount = nPartCountX * nPartCountY;

	m_pVB3D = new LPTVERTEXBUFFER[m_nPartCount];
	m_pVB = new LPTVERTEXBUFFER[m_nPartCount];

	m_pTEX[0] = new LPTTEXTURE[m_nPartCount];
	m_pTEX[1] = new LPTTEXTURE[m_nPartCount];

	m_ppPosBufX = new float*[m_nPartCount];
	m_ppPosBufY = new float*[m_nPartCount];

	ZeroMemory( m_pVB3D, m_nPartCount * sizeof(LPTVERTEXBUFFER));
	ZeroMemory( m_pVB, m_nPartCount * sizeof(LPTVERTEXBUFFER));

	ZeroMemory( m_pTEX[0], m_nPartCount * sizeof(LPTTEXTURE));
	ZeroMemory( m_pTEX[1], m_nPartCount * sizeof(LPTTEXTURE));

	ZeroMemory( m_ppPosBufX, m_nPartCount * sizeof(float *));
	ZeroMemory( m_ppPosBufY, m_nPartCount * sizeof(float *));

	// The text's coverage, white on black.
	std::vector<BYTE> vCOVER;

	if( m_nWidth > 0 && m_nHeight > 0 )
	{
		vCOVER.resize( size_t(m_nWidth) * m_nHeight);

		CTTextRaster::Draw(
			m_pFont,
			pBUF,
			m_nRenderingHint,
			0.0f, 1.0f,
			FLOAT(m_nWidth),
			FLOAT(m_nHeight - 1.0f),
			nFormat,
			m_nWidth,
			m_nHeight,
			&vCOVER[0]);
	}
	LPBYTE pCOVER = vCOVER.empty() ? NULL : &vCOVER[0];

	int nIndex = 0;
	nHeight = 0;
	nWidth = 0;

	while( nHeight < m_nHeight )
	{
		int nBottom = nHeight + CTMath::GetNearPow(m_nHeight - nHeight);

		FLOAT fV = nBottom > m_nHeight ? ((FLOAT) m_nHeight - nHeight) / ((FLOAT) nBottom - nHeight) : 1.0f;
		FLOAT fBottom = ((FLOAT) min( m_nHeight, nBottom)) - 0.5f;

		while( nWidth < m_nWidth )
		{
			int nRight = nWidth + CTMath::GetNearPow(m_nWidth - nWidth);

			FLOAT fU = nRight > m_nWidth ? ((FLOAT) m_nWidth - nWidth) / ((FLOAT) nRight - nWidth) : 1.0f;
			FLOAT fRight = ((FLOAT) min( m_nWidth, nRight)) - 0.5f;
			FLOAT vPoint[4][4] = {
				{ ((FLOAT) nWidth) - 0.5f, ((FLOAT) nHeight) - 0.5f, 0.0f, 0.0f},
				{ fRight, ((FLOAT) nHeight) - 0.5f, fU, 0.0f},
				{ ((FLOAT) nWidth) - 0.5f, fBottom, 0.0f, fV},
				{ fRight, fBottom, fU, fV}};

			HRESULT hRESULT = pDevice->CreateVertexBuffer(
				4 * sizeof(TVERTEX),
				0, T3DFVF_TVERTEX,
				D3DPOOL_MANAGED,
				&m_pVB[nIndex], NULL);

			if( FAILED(hRESULT) || !m_pVB[nIndex] )
			{
				Release();

				return;
			}

			hRESULT = pDevice->CreateVertexBuffer(
				4 * sizeof(PVERTEX),
				0, T3DFVF_PVERTEX,
				D3DPOOL_MANAGED,
				&m_pVB3D[nIndex], NULL);

			if( FAILED(hRESULT) || !m_pVB3D[nIndex] )
			{
				Release();

				return;
			}

			LPPVERTEX pBUF3D = NULL;
			LPTVERTEX pBUF = NULL;

			m_ppPosBufX[nIndex] = new float[4];
			m_ppPosBufY[nIndex] = new float[4];

			if(FAILED(m_pVB3D[nIndex]->Lock( 0, 4 * sizeof(PVERTEX), (LPVOID *) &pBUF3D, 0)))
			{
				Release();

				return;
			}

			if(FAILED(m_pVB[nIndex]->Lock( 0, 4 * sizeof(TVERTEX), (LPVOID *) &pBUF, 0)))
			{
				m_pVB3D[nIndex]->Unlock();
				Release();

				return;
			}

			for( int i=0; i<4; i++)
			{
				pBUF[i].m_fPosX = vPoint[i][0];
				pBUF[i].m_fPosY = vPoint[i][1];
				pBUF[i].m_fPosZ = 0.5f;
				pBUF[i].m_fRHW = 1.0f;
				pBUF[i].m_fU = vPoint[i][2];
				pBUF[i].m_fV = vPoint[i][3];

				pBUF3D[i].m_fPosX = vPoint[i][0] + 0.5f - FLOAT(m_nWidth) / 2.0f;
				pBUF3D[i].m_fPosY = FLOAT(m_nHeight) - vPoint[i][1] - 0.5f;
				pBUF3D[i].m_fPosZ = 0.0f;
				pBUF3D[i].m_fU = vPoint[i][2];
				pBUF3D[i].m_fV = vPoint[i][3];

				m_ppPosBufX[nIndex][i] = vPoint[i][0];
				m_ppPosBufY[nIndex][i] = vPoint[i][1];
			}

			m_pVB[nIndex]->Unlock();
			m_pVB3D[nIndex]->Unlock();

			hRESULT = D3DXCreateTexture(
				pDevice,
				nRight - nWidth,
				nBottom - nHeight,
				1, 0,
				D3DFMT_A4R4G4B4,
				D3DPOOL_MANAGED,
				&m_pTEX[0][nIndex]);

			if( FAILED(hRESULT) || !m_pTEX[0][nIndex] )
			{
				Release();

				return;
			}

			hRESULT = D3DXCreateTexture(
				pDevice,
				nRight - nWidth,
				nBottom - nHeight,
				1, 0,
				D3DFMT_A4R4G4B4,
				D3DPOOL_MANAGED,
				&m_pTEX[1][nIndex]);

			if( FAILED(hRESULT) || !m_pTEX[1][nIndex] )
			{
				Release();

				return;
			}

			D3DLOCKED_RECT lr[2];
			if(FAILED(m_pTEX[0][nIndex]->LockRect( 0, &lr[0], NULL, 0)))
			{
				Release();

				return;
			}

			if(FAILED(m_pTEX[1][nIndex]->LockRect( 0, &lr[1], NULL, 0)))
			{
				m_pTEX[0][nIndex]->UnlockRect(0);
				Release();

				return;
			}

			LPBYTE pDestRow[2] = {
				(LPBYTE) lr[0].pBits,
				(LPBYTE) lr[1].pBits};
			LPWORD pDst16[2];

			for( int nPosY = 0; nPosY < min( nBottom, m_nHeight) - nHeight; nPosY++)
			{
				pDst16[0] = (LPWORD) pDestRow[0];
				pDst16[1] = (LPWORD) pDestRow[1];

				for( int nPosX = 0; nPosX < min( nRight, m_nWidth) - nWidth; nPosX++)
				{
					int nIndex = m_nWidth * (nHeight + nPosY) + nWidth + nPosX;
					BYTE bColor = (BYTE) (pCOVER[nIndex] >> 4);

					if( bColor > 0 )
					{
						bColor += TGDICOLOR_OFFSET;

						bColor = max( TGDITEXTCLR_MIN, bColor);
						bColor = min( TGDITEXTCLR_MAX, bColor);

						*pDst16[0] = (WORD(bColor) << 12)|0x0FFF;
					}
					else
						*pDst16[0] = GetColorByLinePixel( pCOVER, nWidth + nPosX, nHeight + nPosY);

					bColor = ((*pDst16[0]) & 0xF000) >> 12;
					if( !bColor || bColor > 0x07 )
						*pDst16[1] = 0x0000;
					else
						*pDst16[1] = 0xFFFF;

					pDst16[0]++;
					pDst16[1]++;
				}

				pDestRow[0] += lr[0].Pitch;
				pDestRow[1] += lr[1].Pitch;
			}

			m_pTEX[0][nIndex]->UnlockRect(0);
			m_pTEX[1][nIndex]->UnlockRect(0);

			nWidth = nRight;
			nIndex++;
		}

		nHeight = nBottom;
		nWidth = 0;
	}

	m_nFormat = nFormat;
	m_strTEXT = strText;
	m_nPosX = 0;
	m_nPosY = 0;
}

WORD CD3DFont::GetColorByLinePixel( LPBYTE pCOVER, int nPosX, int nPosY)
{
	if( m_bOutLine || m_bShadow )
	{
		static int nPOS[4][2] = {
			{ -1,  0},
			{  0, -1},
			{  1,  0},
			{  0,  1}};

		CRect rect(
			0, 0,
			m_nWidth,
			m_nHeight);

		for( int i=0; i<4; i++)
		{
			int nCurX = nPosX + nPOS[i][0];
			int nCurY = nPosY + nPOS[i][1];

			if(rect.PtInRect(CPoint( nCurX, nCurY)))
			{
				int nIndex = m_nWidth * nCurY + nCurX;
				BYTE bColor = (BYTE) (pCOVER[nIndex] >> 4);

				if( bColor > 0 && (m_bOutLine || i < 2) )
					return 0x3FFF;
			}
		}
	}

	return 0x0000;
}

void CD3DFont::TextOut( LPTRENDERDEVICE pDevice,
					    CString strText,
					    CRect& rect,
						UINT nFormat,
						BOOL bOpaque,
						CPoint Hide)
{
	DWORD dwLineREF = ((m_dwLineColor & 0xFF000000) >> 24) * (TGDITEXTCLR_MIN - 1) / TGDITEXTCLR_MAX;
	DWORD dwTextREF = ((m_dwColor & 0xFF000000) >> 24) * (TGDITEXTCLR_MIN - 1) / TGDITEXTCLR_MAX;
	BYTE bApplyVB = FALSE;

	if(!pDevice)
		return;

	MakeText( pDevice, strText, rect, nFormat, Hide);
	if(m_strTEXT.IsEmpty())
		return;

	if( rect.left != m_nPosX ||
		rect.top != m_nPosY )
	{
		int nMoveX = rect.left - m_nPosX;
		int nMoveY = rect.top - m_nPosY;

		for( int i=0; i<m_nPartCount; i++)
			for( int j=0; j<4; j++)
			{
				m_ppPosBufX[i][j] += (FLOAT) nMoveX;
				m_ppPosBufY[i][j] += (FLOAT) nMoveY;
			}

		m_nPosX = rect.left;
		m_nPosY = rect.top;
		bApplyVB = TRUE;
	}

	if( bApplyVB ||
		m_fScaleX != m_fPrvScaleX ||
		m_fScaleY != m_fPrvScaleY )
	{
		LPTVERTEX pData = NULL;

		float fBasePosX = (float) m_nPosX;
		float fBasePosY = (float) m_nPosY;

		for( int i=0; i<m_nPartCount; i++)
		{
			if(FAILED(m_pVB[i]->Lock( 0, 4 * sizeof(TVERTEX), (LPVOID *) &pData, 0)))
			{
				Release();
				return;
			}

			for( int j=0; j<4; j++)
			{
				pData[j].m_fPosX = m_ppPosBufX[i][j] * m_fScaleX;
				pData[j].m_fPosY = m_ppPosBufY[i][j] * m_fScaleY;
			}

			m_pVB[i]->Unlock();
		}

		m_fPrvScaleX = m_fScaleX;
		m_fPrvScaleY = m_fScaleY;
	}

	pDevice->SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
	pDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	pDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TFACTOR);

	pDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	pDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
	pDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	pDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	pDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE);

	pDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	pDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	pDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	pDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID);

	pDevice->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	pDevice->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE);

	pDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	pDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE);
	pDevice->SetRenderState( D3DRS_ZENABLE, FALSE);
	pDevice->SetFVF(T3DFVF_TVERTEX);

	for( int i=0; i<m_nPartCount; i++)
	{
		pDevice->SetStreamSource( 0, m_pVB[i], 0, sizeof(TVERTEX));

		if( m_bShadow || m_bOutLine )
		{
			pDevice->SetRenderState( D3DRS_TEXTUREFACTOR, m_dwLineColor);
			pDevice->SetRenderState( D3DRS_ALPHAREF, 0);
			pDevice->SetTexture( 0, m_pTEX[1][i]);

			pDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2);
		}

		pDevice->SetRenderState( D3DRS_TEXTUREFACTOR, m_dwColor);
		pDevice->SetRenderState( D3DRS_ALPHAREF, dwTextREF);
		pDevice->SetTexture( 0, m_pTEX[0][i]);

		pDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2);
	}

	pDevice->SetRenderState( D3DRS_ALPHATESTENABLE, FALSE);
	pDevice->SetRenderState( D3DRS_ZENABLE, TRUE);
}

void CD3DFont::TextOut( LPTRENDERDEVICE pDevice,
					    CString strText,
					    int nPosX,
						int nPosY,
						BOOL bOpaque,
						CPoint Hide)
{
	CRect rect( nPosX, nPosY, 0, 0);
	TextOut( pDevice, strText, rect, DT_CALCRECT, bOpaque, Hide);
}

void CD3DFont::TextOut( LPTRENDERDEVICE pDevice,
					    const D3DXMATRIX& vWorld,
					    CString strText,
						FLOAT fTextHeight,
						UINT nFormat)
{
	DWORD dwLineREF = ((m_dwLineColor & 0xFF000000) >> 24) * (TGDITEXTCLR_MIN - 1) / TGDITEXTCLR_MAX;
	DWORD dwTextREF = ((m_dwColor & 0xFF000000) >> 24) * (TGDITEXTCLR_MIN - 1) / TGDITEXTCLR_MAX;

	FLOAT fDepthBias;
	D3DXMATRIX vMat;

	if(!pDevice)
		return;

	CRect rect( 0, 0, m_nWidth, m_nHeight);
	MakeText( pDevice, strText, rect, DT_CALCRECT|nFormat);
	if(m_strTEXT.IsEmpty())
		return;

	pDevice->SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
	pDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	pDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TFACTOR);

	pDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	pDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
	pDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	pDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	pDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE);

	pDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	pDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	pDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	pDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID);

	pDevice->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	pDevice->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE);

	pDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	pDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE);
	pDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE);
	pDevice->SetRenderState( D3DRS_ZENABLE, TRUE);
	pDevice->SetFVF(T3DFVF_PVERTEX);

	D3DXMatrixIdentity(&vMat);
	vMat._11 = fTextHeight / ((FLOAT) m_nHeight);
	vMat._22 = fTextHeight / ((FLOAT) m_nHeight);
	vMat._33 = fTextHeight / ((FLOAT) m_nHeight);
	vMat *= vWorld;
	pDevice->SetTransform( D3DTS_WORLD, &vMat);

	for( int i=0; i<m_nPartCount; i++)
	{
		pDevice->SetStreamSource( 0, m_pVB3D[i], 0, sizeof(PVERTEX));
		fDepthBias = 2.0f * EXTBIAS_BASE;

		if( m_bShadow || m_bOutLine )
		{
			pDevice->SetRenderState( D3DRS_DEPTHBIAS, *((LPDWORD) &fDepthBias));
			pDevice->SetRenderState( D3DRS_TEXTUREFACTOR, m_dwLineColor);
			pDevice->SetRenderState( D3DRS_ALPHAREF, 0);
			pDevice->SetTexture( 0, m_pTEX[1][i]);

			pDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2);
		}
		fDepthBias -= EXTBIAS_BASE;

		pDevice->SetRenderState( D3DRS_DEPTHBIAS, *((LPDWORD) &fDepthBias));
		pDevice->SetRenderState( D3DRS_TEXTUREFACTOR, m_dwColor);
		pDevice->SetRenderState( D3DRS_ALPHAREF, dwTextREF);
		pDevice->SetTexture( 0, m_pTEX[0][i]);

		pDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2);
	}
	fDepthBias = EXTBIAS_BASE;

	pDevice->SetRenderState( D3DRS_DEPTHBIAS, *((LPDWORD) &fDepthBias));
	pDevice->SetRenderState( D3DRS_ALPHATESTENABLE, FALSE);
}

void CD3DFont::SetScaleX( float fScale)
{
	m_fScaleX = fScale;
}

void CD3DFont::SetScaleY( float fScale)
{
	m_fScaleY = fScale;
}
