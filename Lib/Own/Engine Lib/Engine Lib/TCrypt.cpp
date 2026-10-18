#include "StdAfx.h"


//////////////////////////////////////////////////////////////////////
// CTMD5

static const DWORD g_vMD5K[64] = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };

static const BYTE g_vMD5S[64] = {
	7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
	5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
	4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
	6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21 };

CTMD5::CTMD5()
{
	m_vSTATE[0] = 0x67452301;
	m_vSTATE[1] = 0xefcdab89;
	m_vSTATE[2] = 0x98badcfe;
	m_vSTATE[3] = 0x10325476;
	m_nBYTES = 0;
}

void CTMD5::Transform( const BYTE *pBLOCK)
{
	DWORD vX[16];

	for( int i=0; i<16; i++)
		vX[i] = DWORD(pBLOCK[i * 4]) | (DWORD(pBLOCK[i * 4 + 1]) << 8) | (DWORD(pBLOCK[i * 4 + 2]) << 16) | (DWORD(pBLOCK[i * 4 + 3]) << 24);

	DWORD a = m_vSTATE[0];
	DWORD b = m_vSTATE[1];
	DWORD c = m_vSTATE[2];
	DWORD d = m_vSTATE[3];

	for( int i=0; i<64; i++)
	{
		DWORD f;
		int g;

		switch(i / 16)
		{
		case 0	: f = (b & c) | (~b & d); g = i; break;
		case 1	: f = (d & b) | (~d & c); g = (5 * i + 1) % 16; break;
		case 2	: f = b ^ c ^ d; g = (3 * i + 5) % 16; break;
		default	: f = c ^ (b | ~d); g = (7 * i) % 16; break;
		}

		DWORD t = a + f + g_vMD5K[i] + vX[g];
		a = d;
		d = c;
		c = b;
		b += (t << g_vMD5S[i]) | (t >> (32 - g_vMD5S[i]));
	}

	m_vSTATE[0] += a;
	m_vSTATE[1] += b;
	m_vSTATE[2] += c;
	m_vSTATE[3] += d;
}

void CTMD5::Update( const void *pDATA, size_t nLength)
{
	const BYTE *pBYTE = (const BYTE *) pDATA;
	size_t nUSED = size_t(m_nBYTES % 64);

	m_nBYTES += nLength;

	if(nUSED)
	{
		size_t nCOPY = (std::min)( nLength, 64 - nUSED);

		memcpy( m_vBUF + nUSED, pBYTE, nCOPY);
		pBYTE += nCOPY;
		nLength -= nCOPY;

		if( nUSED + nCOPY < 64 )
			return;

		Transform(m_vBUF);
	}

	for( ; nLength >= 64; pBYTE += 64, nLength -= 64)
		Transform(pBYTE);

	memcpy( m_vBUF, pBYTE, nLength);
}

void CTMD5::Final( BYTE pDIGEST[TMD5_LENGTH])
{
	UINT64 nBITS = m_nBYTES * 8;
	BYTE vPAD[72] = { 0x80 };
	size_t nUSED = size_t(m_nBYTES % 64);
	size_t nPAD = nUSED < 56 ? 56 - nUSED : 120 - nUSED;

	for( int i=0; i<8; i++)
		vPAD[nPAD + i] = BYTE(nBITS >> (8 * i));

	Update( vPAD, nPAD + 8);

	for( int i=0; i<16; i++)
		pDIGEST[i] = BYTE(m_vSTATE[i / 4] >> (8 * (i % 4)));
}

void CTMD5::Digest( const void *pDATA, size_t nLength, BYTE pDIGEST[TMD5_LENGTH])
{
	CTMD5 vMD5;

	vMD5.Update( pDATA, nLength);
	vMD5.Final(pDIGEST);
}

CString CTMD5::HexDigest( const void *pDATA, size_t nLength)
{
	BYTE vDIGEST[TMD5_LENGTH];
	CString strRESULT;

	Digest( pDATA, nLength, vDIGEST);

	for( int i=0; i<TMD5_LENGTH; i++)
		strRESULT.AppendFormat( _T("%02x"), vDIGEST[i]);

	return strRESULT;
}


//////////////////////////////////////////////////////////////////////
// RC4

void TRC4( const BYTE *pKEY, size_t nKEY, LPBYTE pBUF, size_t nLength)
{
	BYTE vS[256];

	for( int i=0; i<256; i++)
		vS[i] = BYTE(i);

	for( int i=0, j=0; i<256; i++)
	{
		j = (j + vS[i] + pKEY[i % nKEY]) & 0xFF;
		std::swap( vS[i], vS[j]);
	}

	for( size_t n=0, i=0, j=0; n<nLength; n++)
	{
		i = (i + 1) & 0xFF;
		j = (j + vS[i]) & 0xFF;
		std::swap( vS[i], vS[j]);

		pBUF[n] ^= vS[(vS[i] + vS[j]) & 0xFF];
	}
}
