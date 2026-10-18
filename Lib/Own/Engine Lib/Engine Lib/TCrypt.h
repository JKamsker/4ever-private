// TCrypt.h: MD5 (RFC 1321) and RC4, portable.
//
// The client's packets are RC4 encrypted with the MD5 of the shared secret as key: what
// CryptDeriveKey( CALG_RC4) of a CALG_MD5 hash gives (the whole 128 bit hash, no salt).
//////////////////////////////////////////////////////////////////////

#pragma once

#define TMD5_LENGTH					16

class CTMD5
{
public:
	CTMD5();

	void Update( const void *pDATA, size_t nLength);
	void Final( BYTE pDIGEST[TMD5_LENGTH]);

	static void Digest( const void *pDATA, size_t nLength, BYTE pDIGEST[TMD5_LENGTH]);

	// Lowercase hex of the digest, 32 characters.
	static CString HexDigest( const void *pDATA, size_t nLength);

protected:
	void Transform( const BYTE *pBLOCK);

protected:
	DWORD m_vSTATE[4];
	UINT64 m_nBYTES;
	BYTE m_vBUF[64];
};

// RC4 over pBUF in place, with a fresh key schedule (no state kept between calls).
void TRC4( const BYTE *pKEY, size_t nKEY, LPBYTE pBUF, size_t nLength);
