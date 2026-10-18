// TImageInfo.h: D3DXGetImageInfoFromFileInMemory without D3DX. Reads the size, levels and format
// of a DDS, PNG, JPEG, BMP or TGA image from its header, without decoding it.
//////////////////////////////////////////////////////////////////////

#pragma once

#include <string.h>

namespace TImageInfo
{
	inline DWORD ReadLE( const BYTE *p, int nBytes)
	{
		DWORD dwValue = 0;

		for( int i=nBytes - 1; i>=0; i--)
			dwValue = (dwValue << 8) | p[i];

		return dwValue;
	}

	inline DWORD ReadBE( const BYTE *p, int nBytes)
	{
		DWORD dwValue = 0;

		for( int i=0; i<nBytes; i++)
			dwValue = (dwValue << 8) | p[i];

		return dwValue;
	}

	// The format of a DDS pixel format (DDPIXELFORMAT at offset 76).
	inline D3DFORMAT GetDDSFormat( const BYTE *pPF)
	{
		DWORD dwFlags = ReadLE( pPF + 4, 4);
		DWORD dwFourCC = ReadLE( pPF + 8, 4);
		DWORD dwBits = ReadLE( pPF + 12, 4);
		DWORD dwR = ReadLE( pPF + 16, 4);
		DWORD dwG = ReadLE( pPF + 20, 4);
		DWORD dwB = ReadLE( pPF + 24, 4);
		DWORD dwA = ReadLE( pPF + 28, 4);
		BOOL bAlpha = (dwFlags & 0x00000001) && dwA ? TRUE : FALSE;

		// DDPF_FOURCC: a four character code, or a D3DFORMAT number.
		if( dwFlags & 0x00000004 )
			return D3DFORMAT(dwFourCC);

		// DDPF_RGB
		if( dwFlags & 0x00000040 )
			switch(dwBits)
			{
			case 32	:
				if( dwR == 0x00FF0000 && dwG == 0x0000FF00 && dwB == 0x000000FF )
					return bAlpha ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8;

				if( dwR == 0x000000FF && dwG == 0x0000FF00 && dwB == 0x00FF0000 )
					return bAlpha ? D3DFMT_A8B8G8R8 : D3DFMT_X8B8G8R8;

				break;

			case 24	: return D3DFMT_R8G8B8;
			case 16	:
				if( dwR == 0xF800 )
					return D3DFMT_R5G6B5;

				if( dwR == 0x7C00 )
					return bAlpha ? D3DFMT_A1R5G5B5 : D3DFMT_X1R5G5B5;

				if( dwR == 0x0F00 )
					return bAlpha ? D3DFMT_A4R4G4B4 : D3DFMT_X4R4G4B4;

				break;
			}

		// DDPF_LUMINANCE, DDPF_ALPHA
		if( dwFlags & 0x00020000 )
			return dwBits == 16 ? D3DFMT_A8L8 : D3DFMT_L8;

		if( (dwFlags & 0x00000002) && dwBits == 8 )
			return D3DFMT_A8;

		return D3DFMT_UNKNOWN;
	}

	inline BOOL GetDDS( const BYTE *p, UINT nSize, D3DXIMAGE_INFO *pInfo)
	{
		// The header and pixel format sizes, and a size a texture can have.
		if( nSize < 128 || ReadLE( p + 4, 4) != 124 || ReadLE( p + 76, 4) != 32 ||
			ReadLE( p + 12, 4) > 16384 || ReadLE( p + 16, 4) > 16384 )
			return FALSE;

		DWORD dwFlags = ReadLE( p + 8, 4);
		DWORD dwCaps2 = ReadLE( p + 112, 4);

		pInfo->Height = ReadLE( p + 12, 4);
		pInfo->Width = ReadLE( p + 16, 4);
		pInfo->Depth = (dwFlags & 0x00800000) && (dwCaps2 & 0x00200000) ? ReadLE( p + 24, 4) : 1;
		pInfo->MipLevels = (dwFlags & 0x00020000) && ReadLE( p + 28, 4) ? ReadLE( p + 28, 4) : 1;
		pInfo->Format = GetDDSFormat(p + 76);
		pInfo->ImageFileFormat = D3DXIFF_DDS;

		if( dwCaps2 & 0x00000200 )
			pInfo->ResourceType = D3DRTYPE_CUBETEXTURE;
		else if( dwCaps2 & 0x00200000 )
			pInfo->ResourceType = D3DRTYPE_VOLUMETEXTURE;
		else
			pInfo->ResourceType = D3DRTYPE_TEXTURE;

		return pInfo->Width && pInfo->Height ? TRUE : FALSE;
	}

	inline BOOL GetPNG( const BYTE *p, UINT nSize, D3DXIMAGE_INFO *pInfo)
	{
		if( nSize < 29 || memcmp( p + 12, "IHDR", 4) )
			return FALSE;

		BYTE bDepth = p[24];
		BYTE bColor = p[25];

		pInfo->Width = ReadBE( p + 16, 4);
		pInfo->Height = ReadBE( p + 20, 4);

		if( bColor == 3 )
			pInfo->Format = D3DFMT_P8;
		else if( bDepth == 16 && (bColor == 4 || bColor == 6) )
			pInfo->Format = D3DFMT_A16B16G16R16;
		else
			pInfo->Format = bColor == 4 || bColor == 6 ? D3DFMT_A8R8G8B8 : bColor == 0 ? D3DFMT_L8 : D3DFMT_X8R8G8B8;
		pInfo->ImageFileFormat = D3DXIFF_PNG;

		return TRUE;
	}

	// The size is in the first start of frame segment.
	inline BOOL GetJPG( const BYTE *p, UINT nSize, D3DXIMAGE_INFO *pInfo)
	{
		UINT nPos = 2;

		while( nPos + 9 < nSize )
		{
			if( p[nPos] != 0xFF )
				return FALSE;

			BYTE bMarker = p[nPos + 1];

			if( bMarker == 0xFF )
			{
				nPos++;
				continue;
			}

			if( bMarker >= 0xC0 && bMarker <= 0xCF && bMarker != 0xC4 && bMarker != 0xC8 && bMarker != 0xCC )
			{
				pInfo->Height = ReadBE( p + nPos + 5, 2);
				pInfo->Width = ReadBE( p + nPos + 7, 2);
				pInfo->Format = p[nPos + 9] == 1 ? D3DFMT_L8 : D3DFMT_X8R8G8B8;
				pInfo->ImageFileFormat = D3DXIFF_JPG;

				return TRUE;
			}

			nPos += 2 + ReadBE( p + nPos + 2, 2);
		}

		return FALSE;
	}

	inline BOOL GetBMP( const BYTE *p, UINT nSize, D3DXIMAGE_INFO *pInfo)
	{
		if( nSize < 30 )
			return FALSE;

		int nHeight = int(ReadLE( p + 22, 4));
		DWORD dwBits = ReadLE( p + 28, 2);

		pInfo->Width = ReadLE( p + 18, 4);
		pInfo->Height = UINT(nHeight < 0 ? -nHeight : nHeight);
		pInfo->Format = dwBits == 32 ? D3DFMT_A8R8G8B8 : dwBits == 24 ? D3DFMT_R8G8B8 : dwBits == 16 ? D3DFMT_X1R5G5B5 : D3DFMT_P8;
		pInfo->ImageFileFormat = D3DXIFF_BMP;

		return TRUE;
	}

	inline BOOL GetTGA( const BYTE *p, UINT nSize, D3DXIMAGE_INFO *pInfo)
	{
		if( nSize < 18 )
			return FALSE;

		BYTE bType = p[2] & ~8;
		BYTE bBits = p[16];

		// Color mapped, true color or grayscale.
		if( bType < 1 || bType > 3 )
			return FALSE;

		pInfo->Width = ReadLE( p + 12, 2);
		pInfo->Height = ReadLE( p + 14, 2);
		pInfo->ImageFileFormat = D3DXIFF_TGA;

		if( bType == 1 )
			pInfo->Format = D3DFMT_P8;
		else if( bType == 3 )
			pInfo->Format = D3DFMT_L8;
		else
			pInfo->Format = bBits == 32 ? D3DFMT_A8R8G8B8 : bBits == 24 ? D3DFMT_R8G8B8 : (p[17] & 0x0F) ? D3DFMT_A1R5G5B5 : D3DFMT_X1R5G5B5;

		return pInfo->Width && pInfo->Height ? TRUE : FALSE;
	}

	inline HRESULT Get( LPCVOID pData, UINT nSize, D3DXIMAGE_INFO *pInfo)
	{
		const BYTE *p = (const BYTE *) pData;
		BOOL bResult = FALSE;

		if( !p || !pInfo )
			return D3DERR_INVALIDCALL;

		memset( pInfo, 0, sizeof(D3DXIMAGE_INFO));
		pInfo->Depth = 1;
		pInfo->MipLevels = 1;
		pInfo->ResourceType = D3DRTYPE_TEXTURE;

		if( nSize >= 4 && !memcmp( p, "DDS ", 4) )
			bResult = GetDDS( p, nSize, pInfo);
		else if( nSize >= 8 && !memcmp( p, "\x89PNG\r\n\x1A\n", 8) )
			bResult = GetPNG( p, nSize, pInfo);
		else if( nSize >= 3 && p[0] == 0xFF && p[1] == 0xD8 && p[2] == 0xFF )
			bResult = GetJPG( p, nSize, pInfo);
		else if( nSize >= 2 && p[0] == 'B' && p[1] == 'M' )
			bResult = GetBMP( p, nSize, pInfo);
		else
			bResult = GetTGA( p, nSize, pInfo);

		return bResult ? D3D_OK : D3DXERR_INVALIDDATA;
	}
}
