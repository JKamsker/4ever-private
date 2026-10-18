// The bgfx backend of the render layer (Settings/Renderer=BGFX in config.ini).
//
// It emulates the Direct3D 9 fixed-function pipeline the engine draws with:
// the D3D9 state is recorded as the engine sets it, and each draw call turns
// it into a bgfx state, sampler flags and the uniforms of the emulation
// shaders (shaders/ffp_vs.sc, ffp_fs.sc, compiled into TRenderShaders.h).
//
// Resources keep their data in memory, like D3D9's managed pool, and are
// uploaded when a draw call uses them after a change. bgfx is only called from
// the thread that draws: the loader thread fills resources in memory, and
// releases from any thread queue their bgfx handles until the next Present.
//
// Render targets live on the GPU only: the back buffer is read back through
// bgfx screenshots, and images are saved with GDI+ (bmp, jpg, png).

#define NOMINMAX
#include <windows.h>
#include "TD3D9.h"
#include <TRenderDevice.h>
#include "TThread.h"
#include "TString.h"
#include "TFile.h"

#include <bgfx/bgfx.h>
#include <bimg/decode.h>
#include <bx/allocator.h>

#include <algorithm>
#include <deque>
#include <map>
#include <mutex>
#include <vector>

// GDI+ needs min and max, which NOMINMAX takes away.
namespace Gdiplus
{
	using std::min;
	using std::max;
}
#include <objidl.h>
#include <gdiplus.h>

#include "TRenderShaders.h"

namespace
{
	bx::DefaultAllocator g_vALLOCATOR;

	// Bumped by every bgfx init, so handles of an earlier device are never destroyed.
	DWORD g_dwGeneration = 0;

	inline UINT TMax( UINT a, UINT b) { return a > b ? a : b; }
	inline FLOAT TMaxF( FLOAT a, FLOAT b) { return a > b ? a : b; }
	inline UINT TMin( UINT a, UINT b) { return a < b ? a : b; }

	inline FLOAT DWToFloat( DWORD dwValue) { FLOAT fValue; memcpy( &fValue, &dwValue, sizeof(FLOAT)); return fValue; }

	inline void ColorToVec4( D3DCOLOR dwColor, FLOAT *pOut)
	{
		pOut[0] = FLOAT((dwColor >> 16) & 0xFF) / 255.0f;
		pOut[1] = FLOAT((dwColor >> 8) & 0xFF) / 255.0f;
		pOut[2] = FLOAT(dwColor & 0xFF) / 255.0f;
		pOut[3] = FLOAT((dwColor >> 24) & 0xFF) / 255.0f;
	}

	inline void SetVec4( FLOAT *pOut, FLOAT x, FLOAT y, FLOAT z, FLOAT w)
	{
		pOut[0] = x;
		pOut[1] = y;
		pOut[2] = z;
		pOut[3] = w;
	}

	inline void SetColor( FLOAT *pOut, const D3DCOLORVALUE& vColor)
	{
		SetVec4( pOut, vColor.r, vColor.g, vColor.b, vColor.a);
	}


	// Pixel formats.

	BOOL IsBlockFormat( D3DFORMAT nFormat)
	{
		return nFormat == D3DFMT_DXT1 || nFormat == D3DFMT_DXT2 || nFormat == D3DFMT_DXT3 || nFormat == D3DFMT_DXT4 || nFormat == D3DFMT_DXT5;
	}

	UINT GetBitsPerPixel( D3DFORMAT nFormat)
	{
		switch(nFormat)
		{
		case D3DFMT_A32B32G32R32F	: return 128;
		case D3DFMT_A16B16G16R16F	: return 64;
		case D3DFMT_A8R8G8B8		:
		case D3DFMT_X8R8G8B8		:
		case D3DFMT_A8B8G8R8		:
		case D3DFMT_X8B8G8R8		:
		case D3DFMT_D32				:
		case D3DFMT_D24S8			:
		case D3DFMT_D24X8			:
		case D3DFMT_D24X4S4			:
		case D3DFMT_INDEX32			:
		case D3DFMT_R32F			: return 32;
		case D3DFMT_R8G8B8			: return 24;
		case D3DFMT_R5G6B5			:
		case D3DFMT_X1R5G5B5		:
		case D3DFMT_A1R5G5B5		:
		case D3DFMT_A4R4G4B4		:
		case D3DFMT_X4R4G4B4		:
		case D3DFMT_A8L8			:
		case D3DFMT_D16				:
		case D3DFMT_INDEX16			: return 16;
		case D3DFMT_A8				:
		case D3DFMT_L8				:
		case D3DFMT_P8				: return 8;
		}

		return 32;
	}

	// Bytes of one row of pixels (of 4x4 blocks for DXT) and number of rows.
	UINT GetPitch( D3DFORMAT nFormat, UINT nWidth)
	{
		if(IsBlockFormat(nFormat))
			return TMax( 1U, (nWidth + 3) / 4) * (nFormat == D3DFMT_DXT1 ? 8 : 16);

		return nWidth * GetBitsPerPixel(nFormat) / 8;
	}

	UINT GetRows( D3DFORMAT nFormat, UINT nHeight)
	{
		return IsBlockFormat(nFormat) ? TMax( 1U, (nHeight + 3) / 4) : nHeight;
	}

	UINT GetLevelCount( UINT nWidth, UINT nHeight)
	{
		UINT nCount = 1;

		while( nWidth > 1 || nHeight > 1 )
		{
			nWidth = TMax( 1U, nWidth / 2);
			nHeight = TMax( 1U, nHeight / 2);
			nCount++;
		}

		return nCount;
	}

	bimg::TextureFormat::Enum GetBlockFormat( D3DFORMAT nFormat)
	{
		switch(nFormat)
		{
		case D3DFMT_DXT1	: return bimg::TextureFormat::BC1;
		case D3DFMT_DXT2	:
		case D3DFMT_DXT3	: return bimg::TextureFormat::BC2;
		case D3DFMT_DXT4	:
		case D3DFMT_DXT5	: return bimg::TextureFormat::BC3;
		}

		return bimg::TextureFormat::Unknown;
	}

	inline BYTE Expand( DWORD dwValue, UINT nBits)
	{
		return BYTE(dwValue * 255 / ((1 << nBits) - 1));
	}

	// Decodes nWidth x nHeight pixels of a level into B, G, R, A bytes (nWidth * 4 per row).
	// A DXT level is decoded whole: pSrc must be its first block.
	BOOL DecodeToBGRA( D3DFORMAT nFormat, const BYTE *pSrc, UINT nPitch, UINT nWidth, UINT nHeight, BYTE *pDest)
	{
		if(IsBlockFormat(nFormat))
		{
			bimg::imageDecodeToBgra8( &g_vALLOCATOR, pDest, pSrc, nWidth, nHeight, nWidth * 4, GetBlockFormat(nFormat));
			return TRUE;
		}

		for( UINT y=0; y<nHeight; y++)
		{
			const BYTE *pRow = pSrc + y * nPitch;
			BYTE *pOut = pDest + y * nWidth * 4;

			for( UINT x=0; x<nWidth; x++, pOut += 4)
			{
				switch(nFormat)
				{
				case D3DFMT_A8R8G8B8	: memcpy( pOut, pRow + x * 4, 4); break;
				case D3DFMT_X8R8G8B8	: memcpy( pOut, pRow + x * 4, 3); pOut[3] = 0xFF; break;
				case D3DFMT_A8B8G8R8	:
				case D3DFMT_X8B8G8R8	:
					pOut[0] = pRow[x * 4 + 2];
					pOut[1] = pRow[x * 4 + 1];
					pOut[2] = pRow[x * 4];
					pOut[3] = nFormat == D3DFMT_A8B8G8R8 ? pRow[x * 4 + 3] : 0xFF;
					break;

				case D3DFMT_R8G8B8		: memcpy( pOut, pRow + x * 3, 3); pOut[3] = 0xFF; break;
				case D3DFMT_R5G6B5		:
					{
						WORD w = ((const WORD *) pRow)[x];

						pOut[0] = Expand( w & 0x1F, 5);
						pOut[1] = Expand( (w >> 5) & 0x3F, 6);
						pOut[2] = Expand( (w >> 11) & 0x1F, 5);
						pOut[3] = 0xFF;
					}

					break;

				case D3DFMT_X1R5G5B5	:
				case D3DFMT_A1R5G5B5	:
					{
						WORD w = ((const WORD *) pRow)[x];

						pOut[0] = Expand( w & 0x1F, 5);
						pOut[1] = Expand( (w >> 5) & 0x1F, 5);
						pOut[2] = Expand( (w >> 10) & 0x1F, 5);
						pOut[3] = nFormat == D3DFMT_A1R5G5B5 ? ((w & 0x8000) ? 0xFF : 0x00) : 0xFF;
					}

					break;

				case D3DFMT_A4R4G4B4	:
				case D3DFMT_X4R4G4B4	:
					{
						WORD w = ((const WORD *) pRow)[x];

						pOut[0] = Expand( w & 0xF, 4);
						pOut[1] = Expand( (w >> 4) & 0xF, 4);
						pOut[2] = Expand( (w >> 8) & 0xF, 4);
						pOut[3] = nFormat == D3DFMT_A4R4G4B4 ? Expand( (w >> 12) & 0xF, 4) : 0xFF;
					}

					break;

				case D3DFMT_A8			: pOut[0] = pOut[1] = pOut[2] = 0; pOut[3] = pRow[x]; break;
				case D3DFMT_L8			: pOut[0] = pOut[1] = pOut[2] = pRow[x]; pOut[3] = 0xFF; break;
				case D3DFMT_A8L8		: pOut[0] = pOut[1] = pOut[2] = pRow[x * 2]; pOut[3] = pRow[x * 2 + 1]; break;
				default					: return FALSE;
				}
			}
		}

		return TRUE;
	}

	// Writes B, G, R, A pixels (nWidth * 4 per row) into a level of a plain format.
	BOOL EncodeFromBGRA( D3DFORMAT nFormat, const BYTE *pSrc, UINT nWidth, UINT nHeight, BYTE *pDest, UINT nPitch)
	{
		for( UINT y=0; y<nHeight; y++)
		{
			const BYTE *pIn = pSrc + y * nWidth * 4;
			BYTE *pRow = pDest + y * nPitch;

			for( UINT x=0; x<nWidth; x++, pIn += 4)
			{
				switch(nFormat)
				{
				case D3DFMT_A8R8G8B8	:
				case D3DFMT_X8R8G8B8	: memcpy( pRow + x * 4, pIn, 4); break;
				case D3DFMT_A8B8G8R8	:
				case D3DFMT_X8B8G8R8	:
					pRow[x * 4] = pIn[2];
					pRow[x * 4 + 1] = pIn[1];
					pRow[x * 4 + 2] = pIn[0];
					pRow[x * 4 + 3] = pIn[3];
					break;

				case D3DFMT_R8G8B8		: memcpy( pRow + x * 3, pIn, 3); break;
				case D3DFMT_R5G6B5		: ((WORD *) pRow)[x] = WORD((pIn[0] >> 3) | ((pIn[1] >> 2) << 5) | ((pIn[2] >> 3) << 11)); break;
				case D3DFMT_X1R5G5B5	:
				case D3DFMT_A1R5G5B5	: ((WORD *) pRow)[x] = WORD((pIn[0] >> 3) | ((pIn[1] >> 3) << 5) | ((pIn[2] >> 3) << 10) | (pIn[3] >= 0x80 ? 0x8000 : 0)); break;
				case D3DFMT_A4R4G4B4	:
				case D3DFMT_X4R4G4B4	: ((WORD *) pRow)[x] = WORD((pIn[0] >> 4) | ((pIn[1] >> 4) << 4) | ((pIn[2] >> 4) << 8) | ((pIn[3] >> 4) << 12)); break;
				case D3DFMT_A8			: pRow[x] = pIn[3]; break;
				case D3DFMT_L8			: pRow[x] = pIn[2]; break;
				case D3DFMT_A8L8		: pRow[x * 2] = pIn[2]; pRow[x * 2 + 1] = pIn[3]; break;
				default					: return FALSE;
				}
			}
		}

		return TRUE;
	}

	// Box filter of a B, G, R, A level into the next one.
	// BC1/2/3 blocks of one color each: the average of the block's B, G, R, A pixels.
	void EncodeSolidBlocks( bimg::TextureFormat::Enum nFormat, const BYTE *pBGRA, UINT nWidth, UINT nHeight, std::vector<BYTE>& vOUT)
	{
		UINT nBlocksX = (nWidth + 3) / 4;
		UINT nBlocksY = (nHeight + 3) / 4;
		UINT nBlockSize = nFormat == bimg::TextureFormat::BC1 ? 8 : 16;

		vOUT.assign( nBlocksX * nBlocksY * nBlockSize, 0);

		for( UINT by=0; by<nBlocksY; by++)
			for( UINT bx=0; bx<nBlocksX; bx++)
			{
				UINT vSUM[4] = { 0, 0, 0, 0};
				UINT nCount = 0;

				for( UINT y=by*4; y<TMin( by * 4 + 4, nHeight); y++)
					for( UINT x=bx*4; x<TMin( bx * 4 + 4, nWidth); x++)
					{
						for( UINT c=0; c<4; c++)
							vSUM[c] += pBGRA[(y * nWidth + x) * 4 + c];
						nCount++;
					}

				BYTE vAVG[4];
				for( UINT c=0; c<4; c++)
					vAVG[c] = BYTE(nCount ? (vSUM[c] + nCount / 2) / nCount : 0);

				BYTE *pBlock = &vOUT[(by * nBlocksX + bx) * nBlockSize];
				WORD wColor = WORD((vAVG[0] >> 3) | ((vAVG[1] >> 2) << 5) | ((vAVG[2] >> 3) << 11));
				BYTE *pColor = pBlock + (nBlockSize - 8);

				// Equal end points: index 0 is the color (index 3 is transparent black in BC1).
				memcpy( pColor, &wColor, 2);
				memcpy( pColor + 2, &wColor, 2);

				if( nFormat == bimg::TextureFormat::BC1 && vAVG[3] < 0x80 )
					memset( pColor + 4, 0xFF, 4);

				if( nFormat == bimg::TextureFormat::BC2 )
					memset( pBlock, (vAVG[3] >> 4) * 0x11, 8);
				else if( nFormat == bimg::TextureFormat::BC3 )
				{
					pBlock[0] = vAVG[3];
					pBlock[1] = vAVG[3];
				}
			}
	}

	void Downsample( const BYTE *pSrc, UINT nWidth, UINT nHeight, BYTE *pDest)
	{
		UINT nDestW = TMax( 1U, nWidth / 2);
		UINT nDestH = TMax( 1U, nHeight / 2);

		for( UINT y=0; y<nDestH; y++)
			for( UINT x=0; x<nDestW; x++)
			{
				UINT x0 = TMin( x * 2, nWidth - 1);
				UINT x1 = TMin( x * 2 + 1, nWidth - 1);
				UINT y0 = TMin( y * 2, nHeight - 1);
				UINT y1 = TMin( y * 2 + 1, nHeight - 1);

				for( UINT c=0; c<4; c++)
				{
					UINT nSum =
						pSrc[(y0 * nWidth + x0) * 4 + c] +
						pSrc[(y0 * nWidth + x1) * 4 + c] +
						pSrc[(y1 * nWidth + x0) * 4 + c] +
						pSrc[(y1 * nWidth + x1) * 4 + c];

					pDest[(y * nDestW + x) * 4 + c] = BYTE((nSum + 2) / 4);
				}
			}
	}


	// bgfx handles released from any thread, destroyed by the drawing thread at Present.

	struct TPENDING
	{
		std::mutex m_mutex;
		std::vector<bgfx::TextureHandle> m_vTEXTURE;
		std::vector<bgfx::VertexBufferHandle> m_vVB;
		std::vector<bgfx::IndexBufferHandle> m_vIB;
		std::vector<bgfx::FrameBufferHandle> m_vFB;
	};

	TPENDING& Pending()
	{
		static TPENDING *pPENDING = new TPENDING();
		return *pPENDING;
	}

	template< class T > void DeferDestroy( std::vector<T>& vLIST, T hHandle, DWORD dwGeneration)
	{
		if( !bgfx::isValid(hHandle) || dwGeneration != g_dwGeneration )
			return;

		std::lock_guard<std::mutex> lock(Pending().m_mutex);
		vLIST.push_back(hHandle);
	}

	void FlushDestroy()
	{
		TPENDING& vPENDING = Pending();
		std::lock_guard<std::mutex> lock(vPENDING.m_mutex);

		// Frame buffers first: they reference textures.
		for( size_t i=0; i<vPENDING.m_vFB.size(); i++)
			bgfx::destroy(vPENDING.m_vFB[i]);

		for( size_t i=0; i<vPENDING.m_vTEXTURE.size(); i++)
			bgfx::destroy(vPENDING.m_vTEXTURE[i]);

		for( size_t i=0; i<vPENDING.m_vVB.size(); i++)
			bgfx::destroy(vPENDING.m_vVB[i]);

		for( size_t i=0; i<vPENDING.m_vIB.size(); i++)
			bgfx::destroy(vPENDING.m_vIB[i]);

		vPENDING.m_vTEXTURE.clear();
		vPENDING.m_vVB.clear();
		vPENDING.m_vIB.clear();
		vPENDING.m_vFB.clear();
	}


	class CTSurfaceBGFX;

	class CTTextureBGFX : public CTTexture
	{
	public:
		CTTextureBGFX( UINT nWidth, UINT nHeight, UINT nLevels, DWORD dwUsage, D3DFORMAT nFormat, D3DPOOL nPool)
			: m_nRef(1), m_nWidth(nWidth), m_nHeight(nHeight), m_dwUsage(dwUsage), m_nFormat(nFormat), m_nPool(nPool),
			m_hTexture(BGFX_INVALID_HANDLE), m_dwGeneration(0), m_bDirty(TRUE), m_nUpload(0), m_pDEPTH(NULL)
		{
			UINT nMax = GetLevelCount( nWidth, nHeight);

			m_nLevels = nLevels && nLevels < nMax ? nLevels : nMax;
			m_vLEVEL.resize(m_nLevels);
			m_vWRITTEN.resize( m_nLevels, FALSE);
			m_vSURFACE.resize( m_nLevels, (CTSurfaceBGFX *) NULL);
		}

		~CTTextureBGFX();

		BOOL IsTarget() { return (m_dwUsage & D3DUSAGE_RENDERTARGET) ? TRUE : FALSE; }

		ULONG AddRef() override { return InterlockedIncrement(&m_nRef); }

		ULONG Release() override
		{
			ULONG nRef = InterlockedDecrement(&m_nRef);

			if(!nRef)
				delete this;

			return nRef;
		}

		UINT GetWidth( UINT nLevel) { return TMax( 1U, m_nWidth >> nLevel); }
		UINT GetHeight( UINT nLevel) { return TMax( 1U, m_nHeight >> nLevel); }

		HRESULT GetLevelDesc( UINT Level, D3DSURFACE_DESC *pDesc) override
		{
			if( Level >= m_nLevels || !pDesc )
				return D3DERR_INVALIDCALL;

			ZeroMemory( pDesc, sizeof(D3DSURFACE_DESC));
			pDesc->Format = m_nFormat;
			pDesc->Type = D3DRTYPE_SURFACE;
			pDesc->Usage = m_dwUsage;
			pDesc->Pool = m_nPool;
			pDesc->Width = GetWidth(Level);
			pDesc->Height = GetHeight(Level);

			return D3D_OK;
		}

		HRESULT GetSurfaceLevel( UINT Level, CTSurface **ppSurface) override;

		// The memory of a level, allocated on first use.
		std::vector<BYTE>& GetLevel( UINT nLevel)
		{
			std::vector<BYTE>& vLEVEL = m_vLEVEL[nLevel];

			if(vLEVEL.empty())
				vLEVEL.resize( GetPitch( m_nFormat, GetWidth(nLevel)) * GetRows( m_nFormat, GetHeight(nLevel)));

			return vLEVEL;
		}

		// A level was (or may have been) written: upload the texture again before its next use.
		void Touch( UINT nLevel)
		{
			m_vWRITTEN[nLevel] = TRUE;
			m_bDirty = TRUE;
		}

		HRESULT LockRect( UINT Level, D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) override
		{
			if( Level >= m_nLevels || !pLockedRect )
				return D3DERR_INVALIDCALL;

			std::vector<BYTE>& vLEVEL = GetLevel(Level);
			UINT nPitch = GetPitch( m_nFormat, GetWidth(Level));
			UINT nOffset = 0;

			if(pRect)
				nOffset = (IsBlockFormat(m_nFormat) ? pRect->top / 4 : pRect->top) * nPitch + GetPitch( m_nFormat, pRect->left);

			pLockedRect->Pitch = nPitch;
			pLockedRect->pBits = &vLEVEL[nOffset];

			if(!(Flags & D3DLOCK_READONLY))
				Touch(Level);

			return D3D_OK;
		}

		HRESULT UnlockRect( UINT Level) override
		{
			return Level < m_nLevels ? D3D_OK : D3DERR_INVALIDCALL;
		}

		void GenerateMipSubLevels() override {}

		// The bgfx texture, uploaded first if the data changed.
		bgfx::TextureHandle GetHandle()
		{
			if(IsTarget())
				return GetTargetHandle();

			if( !m_bDirty && bgfx::isValid(m_hTexture) && m_dwGeneration == g_dwGeneration )
				return m_hTexture;

			if( m_dwGeneration != g_dwGeneration )
				m_hTexture = BGFX_INVALID_HANDLE;

			Upload();
			m_bDirty = FALSE;

			return m_hTexture;
		}

		// The frame buffer drawing into level 0 of a render target texture, with a depth
		// buffer or none.
		bgfx::FrameBufferHandle GetFrameBuffer( CTSurfaceBGFX *pDepth);

	private:
		// A render target lives on the GPU. Memory written by the engine (a copy into the
		// render target) is uploaded over it.
		bgfx::TextureHandle GetTargetHandle()
		{
			if( m_dwGeneration != g_dwGeneration )
			{
				m_hTexture = BGFX_INVALID_HANDLE;
				m_mapFB.clear();
			}

			if(!bgfx::isValid(m_hTexture))
			{
				m_hTexture = bgfx::createTexture2D( uint16_t(m_nWidth), uint16_t(m_nHeight), false, 1, bgfx::TextureFormat::BGRA8, BGFX_TEXTURE_RT | BGFX_TEXTURE_BLIT_DST, NULL);
				m_nHandleFormat = bgfx::TextureFormat::BGRA8;
				m_dwGeneration = g_dwGeneration;
				m_nUpload++;
			}

			if( m_bDirty && m_vWRITTEN[0] )
			{
				std::vector<BYTE>& vLEVEL = GetLevel(0);
				std::vector<BYTE> vBGRA( m_nWidth * m_nHeight * 4);

				if(DecodeToBGRA( m_nFormat, &vLEVEL[0], GetPitch( m_nFormat, m_nWidth), m_nWidth, m_nHeight, &vBGRA[0]))
					bgfx::updateTexture2D( m_hTexture, 0, 0, 0, 0, uint16_t(m_nWidth), uint16_t(m_nHeight), bgfx::copy( &vBGRA[0], uint32_t(vBGRA.size())));

				m_vWRITTEN[0] = FALSE;
			}
			m_bDirty = FALSE;

			return m_hTexture;
		}

		void ClearFrameBuffers()
		{
			std::map< UINT64, bgfx::FrameBufferHandle>::iterator it;

			for( it = m_mapFB.begin(); it != m_mapFB.end(); it++)
				DeferDestroy( Pending().m_vFB, (*it).second, m_dwGeneration);

			m_mapFB.clear();
		}

		void Upload()
		{
			if( m_nFormat == D3DFMT_A32B32G32R32F || m_nFormat == D3DFMT_A16B16G16R16F || m_nFormat == D3DFMT_R32F )
				return;

			UINT nFull = GetLevelCount( m_nWidth, m_nHeight);
			BOOL bMips = m_nLevels > 1;
			BOOL bBlock = IsBlockFormat(m_nFormat);

			// DXT stays compressed when its levels are all there. bgfx wants a full chain: the
			// small levels a file does not hold (the game's DDS files keep 5) are filtered
			// from the last one and written as blocks of one color each. Anything else becomes
			// B, G, R, A with the missing levels filtered from the ones above.
			if(bBlock)
			{
				BOOL bComplete = TRUE;

				for( UINT i=1; i<m_nLevels && bComplete; i++)
					if(!m_vWRITTEN[i])
						bComplete = FALSE;

				if(bComplete)
				{
					bimg::TextureFormat::Enum nBlock = GetBlockFormat(m_nFormat);
					std::vector<BYTE> vPREV;
					std::vector<BYTE> vCUR;
					std::vector<BYTE> vBLOCKS;

					CreateHandle( nBlock == bimg::TextureFormat::BC1 ? bgfx::TextureFormat::BC1 :
						nBlock == bimg::TextureFormat::BC2 ? bgfx::TextureFormat::BC2 : bgfx::TextureFormat::BC3, bMips);

					for( UINT i=0; i<(bMips ? nFull : 1); i++)
					{
						UINT nWidth = GetWidth(i);
						UINT nHeight = GetHeight(i);

						if( i < m_nLevels )
						{
							std::vector<BYTE>& vLEVEL = GetLevel(i);
							bgfx::updateTexture2D( m_hTexture, 0, uint8_t(i), 0, 0, uint16_t(nWidth), uint16_t(nHeight), bgfx::copy( &vLEVEL[0], uint32_t(vLEVEL.size())));

							if( i + 1 == m_nLevels && bMips && m_nLevels < nFull )
							{
								vPREV.resize( nWidth * nHeight * 4);

								if(!DecodeToBGRA( m_nFormat, &vLEVEL[0], GetPitch( m_nFormat, nWidth), nWidth, nHeight, &vPREV[0]))
									memset( &vPREV[0], 0, vPREV.size());
							}

							continue;
						}

						vCUR.resize( nWidth * nHeight * 4);
						Downsample( &vPREV[0], GetWidth(i - 1), GetHeight(i - 1), &vCUR[0]);
						EncodeSolidBlocks( nBlock, &vCUR[0], nWidth, nHeight, vBLOCKS);

						bgfx::updateTexture2D( m_hTexture, 0, uint8_t(i), 0, 0, uint16_t(nWidth), uint16_t(nHeight), bgfx::copy( &vBLOCKS[0], uint32_t(vBLOCKS.size())));
						vPREV.swap(vCUR);
					}

					return;
				}
			}

			CreateHandle( bgfx::TextureFormat::BGRA8, bMips);

			std::vector<BYTE> vPREV;
			std::vector<BYTE> vCUR;

			for( UINT i=0; i<(bMips ? nFull : 1); i++)
			{
				UINT nWidth = GetWidth(i);
				UINT nHeight = GetHeight(i);

				vCUR.resize( nWidth * nHeight * 4);

				if( i < m_nLevels && (m_vWRITTEN[i] || i == 0) )
				{
					std::vector<BYTE>& vLEVEL = GetLevel(i);

					if(!DecodeToBGRA( m_nFormat, &vLEVEL[0], GetPitch( m_nFormat, nWidth), nWidth, nHeight, &vCUR[0]))
						memset( &vCUR[0], 0, vCUR.size());
				}
				else
					Downsample( &vPREV[0], GetWidth(i - 1), GetHeight(i - 1), &vCUR[0]);

				bgfx::updateTexture2D( m_hTexture, 0, uint8_t(i), 0, 0, uint16_t(nWidth), uint16_t(nHeight), bgfx::copy( &vCUR[0], uint32_t(vCUR.size())));
				vPREV.swap(vCUR);
			}
		}

		void CreateHandle( bgfx::TextureFormat::Enum nFormat, BOOL bMips)
		{
			if( bgfx::isValid(m_hTexture) && nFormat == m_nHandleFormat )
				return;

			if(bgfx::isValid(m_hTexture))
				bgfx::destroy(m_hTexture);

			m_hTexture = bgfx::createTexture2D( uint16_t(m_nWidth), uint16_t(m_nHeight), bMips ? true : false, 1, nFormat, BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE, NULL);
			m_nHandleFormat = nFormat;
			m_dwGeneration = g_dwGeneration;
			m_nUpload++;
		}

	public:
		LONG m_nRef;
		UINT m_nWidth;
		UINT m_nHeight;
		UINT m_nLevels;
		DWORD m_dwUsage;
		D3DFORMAT m_nFormat;
		D3DPOOL m_nPool;

		std::vector< std::vector<BYTE> > m_vLEVEL;
		std::vector<BOOL> m_vWRITTEN;
		std::vector<CTSurfaceBGFX *> m_vSURFACE;
		std::mutex m_mutex;

		bgfx::TextureHandle m_hTexture;
		bgfx::TextureFormat::Enum m_nHandleFormat;
		DWORD m_dwGeneration;
		BOOL m_bDirty;
		UINT m_nUpload;

		// Frame buffers of a render target, by depth buffer (0: none), and the depth
		// buffer used in place of one of another size.
		std::map< UINT64, bgfx::FrameBufferHandle> m_mapFB;
		CTSurfaceBGFX *m_pDEPTH;
	};

	// A level of a texture, or a standalone surface (back buffer, depth buffer,
	// offscreen plain surface) with its own memory.
	class CTSurfaceBGFX : public CTSurface
	{
	public:
		CTSurfaceBGFX( CTTextureBGFX *pTexture, UINT nLevel)
			: m_nRef(1), m_pTexture(pTexture), m_nLevel(nLevel), m_dwSerial(NewSerial()), m_hDepth(BGFX_INVALID_HANDLE), m_dwGeneration(0), m_vREADY( TRUE, TRUE)
		{
			pTexture->AddRef();
			pTexture->GetLevelDesc( nLevel, &m_vDESC);
		}

		CTSurfaceBGFX( UINT nWidth, UINT nHeight, D3DFORMAT nFormat, DWORD dwUsage, D3DPOOL nPool)
			: m_nRef(1), m_pTexture(NULL), m_nLevel(0), m_dwSerial(NewSerial()), m_hDepth(BGFX_INVALID_HANDLE), m_dwGeneration(0), m_vREADY( TRUE, TRUE)
		{
			ZeroMemory( &m_vDESC, sizeof(D3DSURFACE_DESC));
			m_vDESC.Format = nFormat;
			m_vDESC.Type = D3DRTYPE_SURFACE;
			m_vDESC.Usage = dwUsage;
			m_vDESC.Pool = nPool;
			m_vDESC.Width = nWidth;
			m_vDESC.Height = nHeight;
		}

		~CTSurfaceBGFX()
		{
			DeferDestroy( Pending().m_vTEXTURE, m_hDepth, m_dwGeneration);
		}

		// A read back into this surface is coming: readers wait for it (see WaitReady).
		void BeginReadBack()
		{
			m_vREADY.Reset();
		}

		void EndReadBack()
		{
			m_vREADY.Set();
		}

		// Read backs end when bgfx renders the frame, so only another thread can wait for them.
		void WaitReady()
		{
			m_vREADY.Wait(3000);
		}

		static DWORD NewSerial()
		{
			static LONG s_nSerial = 0;
			return DWORD(InterlockedIncrement(&s_nSerial));
		}

		// The GPU depth buffer of a depth stencil surface.
		bgfx::TextureHandle GetDepthHandle()
		{
			if( m_dwGeneration != g_dwGeneration )
				m_hDepth = BGFX_INVALID_HANDLE;

			if(!bgfx::isValid(m_hDepth))
			{
				m_hDepth = bgfx::createTexture2D( uint16_t(m_vDESC.Width), uint16_t(m_vDESC.Height), false, 1, bgfx::TextureFormat::D24S8, BGFX_TEXTURE_RT_WRITE_ONLY, NULL);
				m_dwGeneration = g_dwGeneration;
			}

			return m_hDepth;
		}

		// The texture keeps the surface of each level (one object per level), so the
		// count of a level surface changes under the texture's lock.
		ULONG AddRef() override
		{
			if(!m_pTexture)
				return InterlockedIncrement(&m_nRef);

			std::lock_guard<std::mutex> lock(m_pTexture->m_mutex);
			return ++m_nRef;
		}

		ULONG Release() override
		{
			if(!m_pTexture)
			{
				ULONG nRef = InterlockedDecrement(&m_nRef);

				if(!nRef)
					delete this;

				return nRef;
			}

			CTTextureBGFX *pTexture = m_pTexture;
			ULONG nRef;
			{
				std::lock_guard<std::mutex> lock(pTexture->m_mutex);

				nRef = --m_nRef;
				if(!nRef)
					pTexture->m_vSURFACE[m_nLevel] = NULL;
			}

			if(!nRef)
			{
				delete this;
				pTexture->Release();
			}

			return nRef;
		}

		HRESULT GetDesc( D3DSURFACE_DESC *pDesc) override
		{
			if(!pDesc)
				return D3DERR_INVALIDCALL;
			*pDesc = m_vDESC;

			return D3D_OK;
		}

		// The memory of the surface and its row pitch.
		BYTE *GetBits( UINT *pPitch)
		{
			*pPitch = GetPitch( m_vDESC.Format, m_vDESC.Width);

			if(m_pTexture)
				return &m_pTexture->GetLevel(m_nLevel)[0];

			if(m_vDATA.empty())
				m_vDATA.resize( *pPitch * GetRows( m_vDESC.Format, m_vDESC.Height));

			return &m_vDATA[0];
		}

		void Touch()
		{
			if(m_pTexture)
				m_pTexture->Touch(m_nLevel);
		}

		HRESULT LockRect( D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) override
		{
			if(m_pTexture)
				return m_pTexture->LockRect( m_nLevel, pLockedRect, pRect, Flags);

			if(!pLockedRect)
				return D3DERR_INVALIDCALL;

			UINT nPitch;
			BYTE *pBits = GetBits(&nPitch);

			if(pRect)
				pBits += (IsBlockFormat(m_vDESC.Format) ? pRect->top / 4 : pRect->top) * nPitch + GetPitch( m_vDESC.Format, pRect->left);

			pLockedRect->Pitch = nPitch;
			pLockedRect->pBits = pBits;

			return D3D_OK;
		}

		HRESULT UnlockRect() override
		{
			return m_pTexture ? m_pTexture->UnlockRect(m_nLevel) : D3D_OK;
		}

	public:
		LONG m_nRef;
		CTTextureBGFX *m_pTexture;
		UINT m_nLevel;
		DWORD m_dwSerial;

		D3DSURFACE_DESC m_vDESC;
		std::vector<BYTE> m_vDATA;

		bgfx::TextureHandle m_hDepth;
		DWORD m_dwGeneration;
		CTEvent m_vREADY;
	};

	// bgfx's callbacks: errors, and the screenshots that read the back buffer back.
	class CTCallbackBGFX : public bgfx::CallbackI
	{
	public:
		void fatal( const char *pFilePath, uint16_t nLine, bgfx::Fatal::Enum nCode, const char *pText) override
		{
			OutputDebugStringA(pText);
			OutputDebugStringA("\n");

			if( nCode != bgfx::Fatal::DebugCheck )
				abort();
		}

		void traceVargs( const char *pFilePath, uint16_t nLine, const char *pFormat, va_list vArgs) override
		{
			char vTEXT[1024];

			vsnprintf_s( vTEXT, sizeof(vTEXT), _TRUNCATE, pFormat, vArgs);
			OutputDebugStringA(vTEXT);
		}

		void profilerBegin( const char *pName, uint32_t dwColor, const char *pFilePath, uint16_t nLine) override {}
		void profilerBeginLiteral( const char *pName, uint32_t dwColor, const char *pFilePath, uint16_t nLine) override {}
		void profilerEnd() override {}

		uint32_t cacheReadSize( uint64_t qwID) override { return 0; }
		bool cacheRead( uint64_t qwID, void *pData, uint32_t nSize) override { return false; }
		void cacheWrite( uint64_t qwID, const void *pData, uint32_t nSize) override {}

		void captureBegin( uint32_t nWidth, uint32_t nHeight, uint32_t nPitch, bgfx::TextureFormat::Enum nFormat, bool bFlipY) override {}
		void captureEnd() override {}
		void captureFrame( const void *pData, uint32_t nSize) override {}

		// A surface waiting for the next screenshot.
		void PushReadBack( CTSurfaceBGFX *pDEST)
		{
			std::lock_guard<std::mutex> lock(m_mutex);

			pDEST->AddRef();
			pDEST->BeginReadBack();
			m_vREADBACK.push_back(pDEST);
		}

		// The back buffer, B G R A: it goes to every waiting surface.
		void screenShot( const char *pFilePath, uint32_t nWidth, uint32_t nHeight, uint32_t nPitch, bgfx::TextureFormat::Enum nFormat, const void *pData, uint32_t nSize, bool bFlipY) override
		{
			std::deque<CTSurfaceBGFX *> vREADBACK;
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				vREADBACK.swap(m_vREADBACK);
			}

			for( size_t i=0; i<vREADBACK.size(); i++)
			{
				CTSurfaceBGFX *pDEST = vREADBACK[i];
				UINT nWidth2 = TMin( nWidth, pDEST->m_vDESC.Width);
				UINT nHeight2 = TMin( nHeight, pDEST->m_vDESC.Height);
				UINT nDestPitch;
				BYTE *pBits = pDEST->GetBits(&nDestPitch);

				for( UINT y=0; y<nHeight2; y++)
				{
					const BYTE *pRow = (const BYTE *) pData + (bFlipY ? nHeight - 1 - y : y) * nPitch;
					EncodeFromBGRA( pDEST->m_vDESC.Format, pRow, nWidth2, 1, pBits + y * nDestPitch, nDestPitch);
				}

				pDEST->Touch();
				pDEST->EndReadBack();
				pDEST->Release();
			}
		}

	private:
		std::mutex m_mutex;
		std::deque<CTSurfaceBGFX *> m_vREADBACK;
	};

	CTCallbackBGFX& Callback()
	{
		static CTCallbackBGFX *pCALLBACK = new CTCallbackBGFX();
		return *pCALLBACK;
	}

	// Saves B G R A pixels with GDI+ (bmp, jpg, png).
	HRESULT SaveBGRA( LPCSTR pDestFile, D3DXIMAGE_FILEFORMAT nFormat, const BYTE *pBGRA, UINT nWidth, UINT nHeight)
	{
		const WCHAR *pMIME = NULL;

		switch(nFormat)
		{
		case D3DXIFF_BMP	: pMIME = L"image/bmp"; break;
		case D3DXIFF_JPG	: pMIME = L"image/jpeg"; break;
		case D3DXIFF_PNG	: pMIME = L"image/png"; break;
		default				: return D3DERR_NOTAVAILABLE;
		}

		Gdiplus::GdiplusStartupInput vINPUT;
		ULONG_PTR dwToken = 0;

		if( Gdiplus::GdiplusStartup( &dwToken, &vINPUT, NULL) != Gdiplus::Ok )
			return E_FAIL;

		HRESULT hr = E_FAIL;
		{
			UINT nCount = 0;
			UINT nSize = 0;
			CLSID vCLSID;
			BOOL bFound = FALSE;

			Gdiplus::GetImageEncodersSize( &nCount, &nSize);
			std::vector<BYTE> vBUF(nSize);
			Gdiplus::ImageCodecInfo *pCODEC = (Gdiplus::ImageCodecInfo *) (nSize ? &vBUF[0] : NULL);

			if( pCODEC && Gdiplus::GetImageEncoders( nCount, nSize, pCODEC) == Gdiplus::Ok )
				for( UINT i=0; i<nCount && !bFound; i++)
					if(!wcscmp( pCODEC[i].MimeType, pMIME))
					{
						vCLSID = pCODEC[i].Clsid;
						bFound = TRUE;
					}

			WCHAR vFILE[MAX_PATH];
			if( bFound && MultiByteToWideChar( CP_ACP, 0, pDestFile, -1, vFILE, MAX_PATH) )
			{
				Gdiplus::Bitmap vBITMAP( INT(nWidth), INT(nHeight), INT(nWidth * 4), PixelFormat32bppARGB, (BYTE *) pBGRA);

				if( vBITMAP.Save( vFILE, &vCLSID, NULL) == Gdiplus::Ok )
					hr = D3D_OK;
			}
		}

		Gdiplus::GdiplusShutdown(dwToken);
		return hr;
	}

	bgfx::FrameBufferHandle CTTextureBGFX::GetFrameBuffer( CTSurfaceBGFX *pDepth)
	{
		bgfx::TextureHandle hColor = GetTargetHandle();

		// A depth buffer of another size (the back buffer's) gets one of the render
		// target's size, kept by the render target.
		BOOL bFit = pDepth && pDepth->m_vDESC.Width == m_nWidth && pDepth->m_vDESC.Height == m_nHeight;
		UINT64 qwKey = pDepth ? (bFit ? UINT64(pDepth->m_dwSerial) : ~UINT64(0)) : 0;

		std::map< UINT64, bgfx::FrameBufferHandle>::iterator finder = m_mapFB.find(qwKey);
		if( finder != m_mapFB.end() )
			return (*finder).second;

		bgfx::TextureHandle vHANDLE[2] = { hColor, BGFX_INVALID_HANDLE };
		uint8_t nCount = 1;

		if(pDepth)
		{
			if(bFit)
				vHANDLE[1] = pDepth->GetDepthHandle();
			else
			{
				if(!m_pDEPTH)
					m_pDEPTH = new CTSurfaceBGFX( m_nWidth, m_nHeight, D3DFMT_D24S8, D3DUSAGE_DEPTHSTENCIL, D3DPOOL_DEFAULT);

				vHANDLE[1] = m_pDEPTH->GetDepthHandle();
			}

			nCount = 2;
		}

		bgfx::FrameBufferHandle hFB = bgfx::createFrameBuffer( nCount, vHANDLE, false);
		m_mapFB[qwKey] = hFB;

		return hFB;
	}

	CTTextureBGFX::~CTTextureBGFX()
	{
		ClearFrameBuffers();
		DeferDestroy( Pending().m_vTEXTURE, m_hTexture, m_dwGeneration);

		if(m_pDEPTH)
			m_pDEPTH->Release();
	}

	HRESULT CTTextureBGFX::GetSurfaceLevel( UINT Level, CTSurface **ppSurface)
	{
		if( Level >= m_nLevels || !ppSurface )
			return D3DERR_INVALIDCALL;

		// One surface object per level, like D3D9, so the engine can compare them.
		std::lock_guard<std::mutex> lock(m_mutex);

		if(m_vSURFACE[Level])
			m_vSURFACE[Level]->m_nRef++;
		else
			m_vSURFACE[Level] = new CTSurfaceBGFX( this, Level);
		*ppSurface = m_vSURFACE[Level];

		return D3D_OK;
	}


	// Vertex and index buffers. A buffer that changes often (D3DUSAGE_DYNAMIC, or
	// rewritten after its upload more than a few times) is copied into a
	// transient buffer at each draw call instead of being uploaded.
	template< class BASE, class DESC > class TBufferBGFX : public BASE
	{
	public:
		TBufferBGFX( UINT nLength, const DESC& vDESC)
			: m_nRef(1), m_vDATA(nLength), m_vDESC(vDESC), m_bDirty(TRUE), m_nUpload(0)
		{
			m_bDynamic = (vDESC.Usage & D3DUSAGE_DYNAMIC) ? TRUE : FALSE;
		}

		ULONG AddRef() override { return InterlockedIncrement(&m_nRef); }

		ULONG Release() override
		{
			ULONG nRef = InterlockedDecrement(&m_nRef);

			if(!nRef)
				delete this;

			return nRef;
		}

		HRESULT GetDesc( DESC *pDesc) override
		{
			if(!pDesc)
				return D3DERR_INVALIDCALL;
			*pDesc = m_vDESC;

			return D3D_OK;
		}

		HRESULT Lock( UINT OffsetToLock, UINT SizeToLock, void **ppbData, DWORD Flags) override
		{
			if( !ppbData || OffsetToLock > m_vDATA.size() )
				return D3DERR_INVALIDCALL;
			*ppbData = m_vDATA.empty() ? NULL : &m_vDATA[OffsetToLock];

			if(!(Flags & D3DLOCK_READONLY))
			{
				if( !m_bDirty && m_nUpload >= 4 )
					m_bDynamic = TRUE;

				m_bDirty = TRUE;
			}

			return D3D_OK;
		}

		HRESULT Unlock() override { return D3D_OK; }

	public:
		LONG m_nRef;
		std::vector<BYTE> m_vDATA;
		DESC m_vDESC;

		BOOL m_bDynamic;
		BOOL m_bDirty;
		UINT m_nUpload;
	};

	class CTVertexBufferBGFX : public TBufferBGFX< CTVertexBuffer, D3DVERTEXBUFFER_DESC>
	{
	public:
		CTVertexBufferBGFX( UINT nLength, const D3DVERTEXBUFFER_DESC& vDESC) : TBufferBGFX( nLength, vDESC) {}

		~CTVertexBufferBGFX()
		{
			ClearHandles();
		}

		// The bgfx buffer of the data seen through a vertex layout.
		bgfx::VertexBufferHandle GetHandle( UINT64 qwLayout, const bgfx::VertexLayout& vLayout)
		{
			if(m_bDirty)
			{
				ClearHandles();
				m_bDirty = FALSE;
				m_nUpload++;
			}

			std::map< UINT64, std::pair< bgfx::VertexBufferHandle, DWORD> >::iterator finder = m_mapHANDLE.find(qwLayout);
			if( finder != m_mapHANDLE.end() && (*finder).second.second == g_dwGeneration )
				return (*finder).second.first;

			bgfx::VertexBufferHandle hBuffer = bgfx::createVertexBuffer( bgfx::copy( &m_vDATA[0], uint32_t(m_vDATA.size())), vLayout);
			m_mapHANDLE[qwLayout] = std::make_pair( hBuffer, g_dwGeneration);

			return hBuffer;
		}

	private:
		void ClearHandles()
		{
			std::map< UINT64, std::pair< bgfx::VertexBufferHandle, DWORD> >::iterator it;

			for( it = m_mapHANDLE.begin(); it != m_mapHANDLE.end(); it++)
				DeferDestroy( Pending().m_vVB, (*it).second.first, (*it).second.second);

			m_mapHANDLE.clear();
		}

		std::map< UINT64, std::pair< bgfx::VertexBufferHandle, DWORD> > m_mapHANDLE;
	};

	class CTIndexBufferBGFX : public TBufferBGFX< CTIndexBuffer, D3DINDEXBUFFER_DESC>
	{
	public:
		CTIndexBufferBGFX( UINT nLength, const D3DINDEXBUFFER_DESC& vDESC)
			: TBufferBGFX( nLength, vDESC), m_hBuffer(BGFX_INVALID_HANDLE), m_dwGeneration(0) {}

		~CTIndexBufferBGFX()
		{
			DeferDestroy( Pending().m_vIB, m_hBuffer, m_dwGeneration);
		}

		BOOL Is32() { return m_vDESC.Format == D3DFMT_INDEX32; }

		bgfx::IndexBufferHandle GetHandle()
		{
			if( !m_bDirty && bgfx::isValid(m_hBuffer) && m_dwGeneration == g_dwGeneration )
				return m_hBuffer;

			DeferDestroy( Pending().m_vIB, m_hBuffer, m_dwGeneration);
			m_hBuffer = bgfx::createIndexBuffer( bgfx::copy( &m_vDATA[0], uint32_t(m_vDATA.size())), Is32() ? BGFX_BUFFER_INDEX32 : BGFX_BUFFER_NONE);
			m_dwGeneration = g_dwGeneration;
			m_bDirty = FALSE;
			m_nUpload++;

			return m_hBuffer;
		}

	private:
		bgfx::IndexBufferHandle m_hBuffer;
		DWORD m_dwGeneration;
	};


	// The bgfx vertex layout of an FVF code, and the emulation shader variant it needs.
	struct TLAYOUT
	{
		bgfx::VertexLayout m_vLayout;
		DWORD m_dwShader;
		BOOL m_bValid;
	};

	TLAYOUT MakeLayout( DWORD dwFVF, UINT nStride)
	{
		TLAYOUT vResult;
		UINT nSize = 0;

		vResult.m_bValid = TRUE;
		vResult.m_dwShader = 0;
		vResult.m_vLayout.begin();

		switch( dwFVF & D3DFVF_POSITION_MASK )
		{
		case D3DFVF_XYZ		:
			vResult.m_vLayout.add( bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
			nSize += 12;
			break;

		case D3DFVF_XYZRHW	:
			vResult.m_vLayout.add( bgfx::Attrib::Position, 4, bgfx::AttribType::Float);
			vResult.m_dwShader |= 1;
			nSize += 16;
			break;

		case D3DFVF_XYZB1	:
		case D3DFVF_XYZB2	:
		case D3DFVF_XYZB3	:
		case D3DFVF_XYZB4	:
		case D3DFVF_XYZB5	:
			{
				UINT nBetas = ((dwFVF & D3DFVF_POSITION_MASK) - D3DFVF_XYZB1) / 2 + 1;

				vResult.m_vLayout.add( bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
				nSize += 12;

				if( dwFVF & D3DFVF_LASTBETA_UBYTE4 )
				{
					// The weights, then the matrix indices in the last beta.
					if( nBetas > 1 )
						vResult.m_vLayout.add( bgfx::Attrib::Weight, uint8_t(TMin( nBetas - 1, 4U)), bgfx::AttribType::Float);

					if( nBetas > 5 )
						vResult.m_vLayout.skip( uint8_t((nBetas - 5) * 4));

					vResult.m_vLayout.add( bgfx::Attrib::Indices, 4, bgfx::AttribType::Uint8, true);
					vResult.m_dwShader |= 2;
				}
				else
				{
					// Weights without indices: drawn with world matrix 0.
					vResult.m_vLayout.skip( uint8_t(nBetas * 4));
				}

				nSize += nBetas * 4;
			}

			break;

		default	:
			vResult.m_bValid = FALSE;
			return vResult;
		}

		if( dwFVF & D3DFVF_NORMAL )
		{
			vResult.m_vLayout.add( bgfx::Attrib::Normal, 3, bgfx::AttribType::Float);
			vResult.m_dwShader |= 1 << 2;
			nSize += 12;
		}

		if( dwFVF & D3DFVF_PSIZE )
		{
			vResult.m_vLayout.skip(4);
			nSize += 4;
		}

		if( dwFVF & D3DFVF_DIFFUSE )
		{
			vResult.m_vLayout.add( bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true);
			vResult.m_dwShader |= 1 << 3;
			nSize += 4;
		}

		if( dwFVF & D3DFVF_SPECULAR )
		{
			vResult.m_vLayout.skip(4);
			nSize += 4;
		}

		UINT nTexCount = (dwFVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;

		for( UINT i=0; i<nTexCount; i++)
		{
			UINT nCoords = 2;

			switch( (dwFVF >> (i * 2 + 16)) & 3 )
			{
			case D3DFVF_TEXTUREFORMAT1	: nCoords = 1; break;
			case D3DFVF_TEXTUREFORMAT3	: nCoords = 3; break;
			case D3DFVF_TEXTUREFORMAT4	: nCoords = 4; break;
			}

			if( i < 2 && nCoords == 2 )
				vResult.m_vLayout.add( i ? bgfx::Attrib::TexCoord1 : bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float);
			else
				vResult.m_vLayout.skip( uint8_t(nCoords * 4));

			nSize += nCoords * 4;
		}

		vResult.m_dwShader |= TMin( nTexCount, 2U) << 4;

		if( nStride > nSize )
			vResult.m_vLayout.skip( uint8_t(nStride - nSize));
		vResult.m_vLayout.end();

		return vResult;
	}

	UINT GetDeclTypeSize( BYTE bType)
	{
		switch(bType)
		{
		case D3DDECLTYPE_FLOAT1		: return 4;
		case D3DDECLTYPE_FLOAT2		: return 8;
		case D3DDECLTYPE_FLOAT3		: return 12;
		case D3DDECLTYPE_FLOAT4		: return 16;
		case D3DDECLTYPE_D3DCOLOR	:
		case D3DDECLTYPE_UBYTE4		:
		case D3DDECLTYPE_UBYTE4N	:
		case D3DDECLTYPE_SHORT2		:
		case D3DDECLTYPE_SHORT2N	:
		case D3DDECLTYPE_USHORT2N	:
		case D3DDECLTYPE_UDEC3		:
		case D3DDECLTYPE_DEC3N		:
		case D3DDECLTYPE_FLOAT16_2	: return 4;
		case D3DDECLTYPE_SHORT4		:
		case D3DDECLTYPE_SHORT4N	:
		case D3DDECLTYPE_USHORT4N	:
		case D3DDECLTYPE_FLOAT16_4	: return 8;
		}

		return 0;
	}

	// A vertex declaration: its elements, and the bgfx layout of each stream for the
	// strides it is drawn with. The terrain draws its positions, colors and texture
	// coordinates from three streams.
	class CTVertexDeclBGFX : public IDirect3DVertexDeclaration9
	{
	public:
		CTVertexDeclBGFX( const D3DVERTEXELEMENT9 *pElements)
			: m_nRef(1), m_dwSerial(DWORD(InterlockedIncrement(&s_nSerial))), m_dwStreams(0)
		{
			BOOL bPosition = FALSE;
			BOOL bRHW = FALSE;
			BOOL bIndices = FALSE;
			UINT nTexCount = 0;

			m_vMAIN.m_dwShader = 0;

			for( ; pElements->Stream != 0xFF; pElements++)
			{
				m_vELEMENT.push_back(*pElements);
				m_dwStreams |= 1 << (pElements->Stream & 0x1F);

				switch(pElements->Usage)
				{
				case D3DDECLUSAGE_POSITION		: bPosition = TRUE; break;
				case D3DDECLUSAGE_POSITIONT		: bRHW = TRUE; break;
				case D3DDECLUSAGE_BLENDINDICES	: bIndices = TRUE; break;
				case D3DDECLUSAGE_NORMAL		: m_vMAIN.m_dwShader |= 1 << 2; break;
				case D3DDECLUSAGE_COLOR			:
					if(!pElements->UsageIndex)
						m_vMAIN.m_dwShader |= 1 << 3;

					break;

				case D3DDECLUSAGE_TEXCOORD		:
					if( pElements->UsageIndex < 2 && pElements->Type == D3DDECLTYPE_FLOAT2 )
						nTexCount = TMax( nTexCount, UINT(pElements->UsageIndex) + 1);

					break;
				}
			}

			// Pretransformed vertices have no lighting, so no variant with normals.
			if(bRHW)
				m_vMAIN.m_dwShader = (m_vMAIN.m_dwShader & ~(1 << 2)) | 1;
			else if(bIndices)
				m_vMAIN.m_dwShader |= 2;

			m_vMAIN.m_dwShader |= nTexCount << 4;
			m_vMAIN.m_bValid = (bPosition || bRHW) && m_dwStreams < (1 << 4);
		}

#ifndef TCLIENT_NO_D3D9
		STDMETHOD(QueryInterface)( REFIID riid, void **ppvObj) override
		{
			if(!ppvObj)
				return E_POINTER;

			if( riid == __uuidof(IUnknown) )
			{
				AddRef();
				*ppvObj = this;

				return S_OK;
			}

			*ppvObj = NULL;
			return E_NOINTERFACE;
		}

#endif

		STDMETHOD_(ULONG, AddRef)() override
		{
			return ULONG(InterlockedIncrement(&m_nRef));
		}

		STDMETHOD_(ULONG, Release)() override
		{
			LONG nRef = InterlockedDecrement(&m_nRef);

			if(!nRef)
				delete this;

			return ULONG(nRef);
		}

#ifndef TCLIENT_NO_D3D9
		STDMETHOD(GetDevice)( IDirect3DDevice9 **ppDevice) override
		{
			if(ppDevice)
				*ppDevice = NULL;

			return D3DERR_INVALIDCALL;
		}
#endif

		STDMETHOD(GetDeclaration)( D3DVERTEXELEMENT9 *pElement, UINT *pNumElements) override
		{
			if(!pNumElements)
				return D3DERR_INVALIDCALL;

			if(pElement)
			{
				D3DVERTEXELEMENT9 vEND = D3DDECL_END();

				for( size_t i=0; i<m_vELEMENT.size(); i++)
					pElement[i] = m_vELEMENT[i];
				pElement[m_vELEMENT.size()] = vEND;
			}

			*pNumElements = UINT(m_vELEMENT.size() + 1);
			return D3D_OK;
		}

		// The layout of one stream of the declaration.
		TLAYOUT& GetLayout( UINT nStream, UINT nStride)
		{
			DWORD dwKey = nStream | (nStride << 8);
			std::map< DWORD, TLAYOUT>::iterator finder = m_mapLAYOUT.find(dwKey);

			if( finder != m_mapLAYOUT.end() )
				return (*finder).second;

			std::vector<D3DVERTEXELEMENT9> vSTREAM;
			TLAYOUT vResult;
			UINT nPos = 0;

			for( size_t i=0; i<m_vELEMENT.size(); i++)
				if( m_vELEMENT[i].Stream == nStream )
				{
					size_t j = vSTREAM.size();

					vSTREAM.push_back(m_vELEMENT[i]);
					for( ; j > 0 && vSTREAM[j - 1].Offset > vSTREAM[j].Offset; j--)
						std::swap( vSTREAM[j - 1], vSTREAM[j]);
				}

			vResult.m_dwShader = m_vMAIN.m_dwShader;
			vResult.m_bValid = TRUE;
			vResult.m_vLayout.begin();

			for( size_t i=0; i<vSTREAM.size() && vResult.m_bValid; i++)
			{
				D3DVERTEXELEMENT9& vElement = vSTREAM[i];
				UINT nSize = GetDeclTypeSize(vElement.Type);

				if( !nSize || vElement.Offset < nPos )
				{
					vResult.m_bValid = FALSE;
					break;
				}

				if( vElement.Offset > nPos )
					vResult.m_vLayout.skip( uint8_t(vElement.Offset - nPos));
				nPos = vElement.Offset + nSize;

				bgfx::Attrib::Enum nAttrib = bgfx::Attrib::Count;
				switch(vElement.Usage)
				{
				case D3DDECLUSAGE_POSITION		:
				case D3DDECLUSAGE_POSITIONT		: nAttrib = bgfx::Attrib::Position; break;
				case D3DDECLUSAGE_NORMAL		: nAttrib = bgfx::Attrib::Normal; break;
				case D3DDECLUSAGE_BLENDWEIGHT	: nAttrib = bgfx::Attrib::Weight; break;
				case D3DDECLUSAGE_BLENDINDICES	: nAttrib = bgfx::Attrib::Indices; break;
				case D3DDECLUSAGE_COLOR			:
					if(!vElement.UsageIndex)
						nAttrib = bgfx::Attrib::Color0;

					break;

				case D3DDECLUSAGE_TEXCOORD		:
					if( vElement.UsageIndex < 2 && vElement.Type == D3DDECLTYPE_FLOAT2 )
						nAttrib = vElement.UsageIndex ? bgfx::Attrib::TexCoord1 : bgfx::Attrib::TexCoord0;

					break;
				}

				switch( nAttrib == bgfx::Attrib::Count ? 0xFF : vElement.Type )
				{
				case D3DDECLTYPE_FLOAT1		:
				case D3DDECLTYPE_FLOAT2		:
				case D3DDECLTYPE_FLOAT3		:
				case D3DDECLTYPE_FLOAT4		: vResult.m_vLayout.add( nAttrib, uint8_t(nSize / 4), bgfx::AttribType::Float); break;

				// The emulation shader reads blend indices as normalized bytes.
				case D3DDECLTYPE_D3DCOLOR	:
				case D3DDECLTYPE_UBYTE4		:
				case D3DDECLTYPE_UBYTE4N	: vResult.m_vLayout.add( nAttrib, 4, bgfx::AttribType::Uint8, true); break;
				case D3DDECLTYPE_SHORT2		: vResult.m_vLayout.add( nAttrib, 2, bgfx::AttribType::Int16); break;
				case D3DDECLTYPE_SHORT2N	: vResult.m_vLayout.add( nAttrib, 2, bgfx::AttribType::Int16, true); break;
				case D3DDECLTYPE_SHORT4		: vResult.m_vLayout.add( nAttrib, 4, bgfx::AttribType::Int16); break;
				case D3DDECLTYPE_SHORT4N	: vResult.m_vLayout.add( nAttrib, 4, bgfx::AttribType::Int16, true); break;
				case D3DDECLTYPE_FLOAT16_2	: vResult.m_vLayout.add( nAttrib, 2, bgfx::AttribType::Half); break;
				case D3DDECLTYPE_FLOAT16_4	: vResult.m_vLayout.add( nAttrib, 4, bgfx::AttribType::Half); break;
				default						: vResult.m_vLayout.skip( uint8_t(nSize)); break;
				}
			}

			if( nStride < nPos )
				vResult.m_bValid = FALSE;
			else if( nStride > nPos )
				vResult.m_vLayout.skip( uint8_t(nStride - nPos));
			vResult.m_vLayout.end();

			return m_mapLAYOUT[dwKey] = vResult;
		}

	public:
		LONG m_nRef;
		DWORD m_dwSerial;
		DWORD m_dwStreams;

		std::vector<D3DVERTEXELEMENT9> m_vELEMENT;
		std::map< DWORD, TLAYOUT> m_mapLAYOUT;

		// The shader variant of the whole declaration.
		TLAYOUT m_vMAIN;

		static LONG s_nSerial;
	};

	LONG CTVertexDeclBGFX::s_nSerial = 0;

	UINT GetVertexCount( D3DPRIMITIVETYPE nType, UINT nCount)
	{
		switch(nType)
		{
		case D3DPT_POINTLIST		: return nCount;
		case D3DPT_LINELIST			: return nCount * 2;
		case D3DPT_LINESTRIP		: return nCount + 1;
		case D3DPT_TRIANGLELIST		: return nCount * 3;
		case D3DPT_TRIANGLESTRIP	:
		case D3DPT_TRIANGLEFAN		: return nCount + 2;
		}

		return 0;
	}


	class CTRenderDeviceBGFX : public CTRenderDevice
	{
	public:
		CTRenderDeviceBGFX( HWND hWnd, const D3DPRESENT_PARAMETERS& vPresent, const D3DCAPS9& vCaps)
			: m_pDUMP(NULL), m_pDumpData(NULL), m_nDumpCount(0), m_nDumpStride(0), m_nDumpPrims(0), m_vPRESENT(vPresent), m_vCAPS(vCaps), m_bREADY(FALSE), m_pVDECL(NULL), m_dwFVF(0)
		{
			m_pBACKBUF = new CTSurfaceBGFX( vPresent.BackBufferWidth, vPresent.BackBufferHeight, D3DFMT_X8R8G8B8, D3DUSAGE_RENDERTARGET, D3DPOOL_DEFAULT);
			m_pZBUF = new CTSurfaceBGFX( vPresent.BackBufferWidth, vPresent.BackBufferHeight, vPresent.AutoDepthStencilFormat, D3DUSAGE_DEPTHSTENCIL, D3DPOOL_DEFAULT);
			m_pRTARGET = m_pBACKBUF;
			m_pRTARGET->AddRef();
			m_pDEPTH = m_pZBUF;
			m_pDEPTH->AddRef();

			ZeroMemory( m_pTEXTURE, sizeof(m_pTEXTURE));
			ZeroMemory( m_vSTREAM, sizeof(m_vSTREAM));
			m_pINDEX = NULL;

			ResetStates();
			ResetViewport();

			bgfx::Init vINIT;
			vINIT.type = bgfx::RendererType::Count;
			vINIT.swapChain.nwh = hWnd;
			vINIT.swapChain.width = vPresent.BackBufferWidth;
			vINIT.swapChain.height = vPresent.BackBufferHeight;
			vINIT.reset = GetResetFlags(vPresent);
			vINIT.callback = &Callback();
			vINIT.limits.maxTransientVbSize = 64 << 20;
			vINIT.limits.maxTransientIbSize = 16 << 20;
			m_vSWAPCHAIN = vINIT.swapChain;

			g_dwGeneration++;
			m_bREADY = bgfx::init(vINIT);

			if(m_bREADY)
				InitPipeline();
		}

		~CTRenderDeviceBGFX()
		{
			for( int i=0; i<TEXTURE_STAGES; i++)
				if(m_pTEXTURE[i])
					m_pTEXTURE[i]->Release();

			for( int i=0; i<STREAMS; i++)
				if(m_vSTREAM[i].m_pVB)
					m_vSTREAM[i].m_pVB->Release();

			if(m_pINDEX)
				m_pINDEX->Release();

			if(m_pVDECL)
				m_pVDECL->Release();

			m_pRTARGET->Release();
			if(m_pDEPTH)
				m_pDEPTH->Release();
			m_pBACKBUF->Release();
			m_pZBUF->Release();

			if(m_bREADY)
			{
				FlushDestroy();

				std::map< DWORD, bgfx::ProgramHandle>::iterator it;
				for( it = m_mapPROGRAM.begin(); it != m_mapPROGRAM.end(); it++)
					bgfx::destroy((*it).second);

				for( size_t i=0; i<m_vSHADER.size(); i++)
					bgfx::destroy(m_vSHADER[i]);

				for( size_t i=0; i<m_vUNIFORM.size(); i++)
					bgfx::destroy(m_vUNIFORM[i]);

				bgfx::shutdown();
			}

			// Handles of this device are gone with it.
			g_dwGeneration++;
		}

		BOOL IsReady() { return m_bREADY; }

		ULONG Release() override { delete this; return 0; }

		HRESULT TestCooperativeLevel() override { return D3D_OK; }
		UINT GetAvailableTextureMem() override { return 0x7FF00000; }
		HRESULT GetDeviceCaps( D3DCAPS9 *pCaps) override { *pCaps = m_vCAPS; return D3D_OK; }

		HRESULT Reset( D3DPRESENT_PARAMETERS *pPresentationParameters) override
		{
			m_vPRESENT = *pPresentationParameters;
			m_vSWAPCHAIN.width = m_vPRESENT.BackBufferWidth;
			m_vSWAPCHAIN.height = m_vPRESENT.BackBufferHeight;
			bgfx::reset( GetResetFlags(m_vPRESENT), &m_vSWAPCHAIN);

			m_pBACKBUF->m_vDESC.Width = m_pZBUF->m_vDESC.Width = m_vPRESENT.BackBufferWidth;
			m_pBACKBUF->m_vDESC.Height = m_pZBUF->m_vDESC.Height = m_vPRESENT.BackBufferHeight;
			m_pBACKBUF->m_vDATA.clear();
			m_pZBUF->m_vDATA.clear();

			// Like D3D9, a reset returns every state to its default.
			ResetStates();
			ResetViewport();

			return D3D_OK;
		}

		HRESULT Present( const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion) override
		{
			bgfx::dbgTextClear();
			bgfx::dbgTextPrintf( 1, 1, 0x0f, "bgfx %s - 3c-5 | views %u draws %u skipped %u",
				bgfx::getRendererName(bgfx::getRendererType()), m_nView + 1, m_nDraws, m_nSkipped);

			// View 0 is the first one of the frame: make sure the back buffer is cleared even
			// when the engine draws nothing.
			bgfx::touch(0);
			bgfx::frame();

			FlushDestroy();
			m_nView = 0;
			m_bViewOpen = FALSE;
			m_nDraws = 0;
			m_nSkipped = 0;
			UpdateDump();

			{
				char vPATH[MAX_PATH];
				char vFILE[MAX_PATH];

				GetTempPathA( MAX_PATH, vPATH);
				sprintf_s( vFILE, "%stclient-bgfx-highlight.flag", vPATH);
				FILE *pFLAG = NULL;

				m_nHighlight = 0;
				if( (pFLAG = TFileOpen( vFILE, "r")) != NULL )
				{
					if( fscanf_s( pFLAG, "%d", &m_nHighlight) != 1 )
						m_nHighlight = 1;
					fclose(pFLAG);
				}
				m_bHighlight = m_nHighlight > 0;
			}

			return D3D_OK;
		}

		void SetGammaRamp( UINT iSwapChain, DWORD Flags, const D3DGAMMARAMP *pRamp) override {}

		HRESULT CreateTexture( UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, LPTTEXTURE *ppTexture, HANDLE *pSharedHandle) override
		{
			if( !ppTexture || !Width || !Height )
				return D3DERR_INVALIDCALL;
			*ppTexture = new CTTextureBGFX( Width, Height, (Usage & D3DUSAGE_AUTOGENMIPMAP) ? 0 : Levels, Usage, Format, Pool);

			return D3D_OK;
		}

		HRESULT CreateVertexBuffer( UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, LPTVERTEXBUFFER *ppVertexBuffer, HANDLE *pSharedHandle) override
		{
			if( !ppVertexBuffer || !Length )
				return D3DERR_INVALIDCALL;

			D3DVERTEXBUFFER_DESC vDESC;
			ZeroMemory( &vDESC, sizeof(vDESC));
			vDESC.Format = D3DFMT_VERTEXDATA;
			vDESC.Type = D3DRTYPE_VERTEXBUFFER;
			vDESC.Usage = Usage;
			vDESC.Pool = Pool;
			vDESC.Size = Length;
			vDESC.FVF = FVF;
			*ppVertexBuffer = new CTVertexBufferBGFX( Length, vDESC);

			return D3D_OK;
		}

		HRESULT CreateIndexBuffer( UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, LPTINDEXBUFFER *ppIndexBuffer, HANDLE *pSharedHandle) override
		{
			if( !ppIndexBuffer || !Length )
				return D3DERR_INVALIDCALL;

			D3DINDEXBUFFER_DESC vDESC;
			ZeroMemory( &vDESC, sizeof(vDESC));
			vDESC.Format = Format;
			vDESC.Type = D3DRTYPE_INDEXBUFFER;
			vDESC.Usage = Usage;
			vDESC.Pool = Pool;
			vDESC.Size = Length;
			*ppIndexBuffer = new CTIndexBufferBGFX( Length, vDESC);

			return D3D_OK;
		}

		HRESULT CreateDepthStencilSurface( UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, LPTSURFACE *ppSurface, HANDLE *pSharedHandle) override
		{
			if(!ppSurface)
				return D3DERR_INVALIDCALL;
			*ppSurface = new CTSurfaceBGFX( Width, Height, Format, D3DUSAGE_DEPTHSTENCIL, D3DPOOL_DEFAULT);

			return D3D_OK;
		}

		HRESULT CreateOffscreenPlainSurface( UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, LPTSURFACE *ppSurface, HANDLE *pSharedHandle) override
		{
			if(!ppSurface)
				return D3DERR_INVALIDCALL;
			*ppSurface = new CTSurfaceBGFX( Width, Height, Format, 0, Pool);

			return D3D_OK;
		}

		// Only the back buffer is read back (screenshots): the surface is filled when bgfx
		// renders the frame, and saving it waits for that.
		HRESULT GetRenderTargetData( LPTSURFACE pRenderTarget, LPTSURFACE pDestSurface) override
		{
			if( !pRenderTarget || !pDestSurface )
				return D3DERR_INVALIDCALL;

			if( pRenderTarget != m_pBACKBUF )
				return D3DERR_NOTAVAILABLE;

			Callback().PushReadBack((CTSurfaceBGFX *) pDestSurface);
			bgfx::requestScreenShot( BGFX_INVALID_HANDLE, "TClient");

			return D3D_OK;
		}

		HRESULT SetRenderTarget( DWORD RenderTargetIndex, LPTSURFACE pRenderTarget) override
		{
			if( RenderTargetIndex || !pRenderTarget )
				return D3DERR_INVALIDCALL;

			pRenderTarget->AddRef();
			m_pRTARGET->Release();
			m_pRTARGET = (CTSurfaceBGFX *) pRenderTarget;

			// Like D3D9, a new render target resets the viewport to its size.
			ResetViewport();
			m_bViewDirty = TRUE;

			return D3D_OK;
		}

		HRESULT GetRenderTarget( DWORD RenderTargetIndex, LPTSURFACE *ppRenderTarget) override
		{
			if( RenderTargetIndex || !ppRenderTarget )
				return D3DERR_INVALIDCALL;

			m_pRTARGET->AddRef();
			*ppRenderTarget = m_pRTARGET;

			return D3D_OK;
		}

		HRESULT SetDepthStencilSurface( LPTSURFACE pNewZStencil) override
		{
			if(pNewZStencil)
				pNewZStencil->AddRef();

			if(m_pDEPTH)
				m_pDEPTH->Release();
			m_pDEPTH = (CTSurfaceBGFX *) pNewZStencil;
			m_bViewDirty = TRUE;

			return D3D_OK;
		}

		HRESULT GetDepthStencilSurface( LPTSURFACE *ppZStencilSurface) override
		{
			if( !ppZStencilSurface || !m_pDEPTH )
				return D3DERR_NOTFOUND;

			m_pDEPTH->AddRef();
			*ppZStencilSurface = m_pDEPTH;

			return D3D_OK;
		}

		HRESULT BeginScene() override { return D3D_OK; }
		HRESULT EndScene() override { return D3D_OK; }

		HRESULT Clear( DWORD Count, const D3DRECT *pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) override
		{
			bgfx::FrameBufferHandle hTarget;
			if(!GetTarget(&hTarget))
				return D3D_OK;

			// A target without alpha reads as opaque, like D3D9's X8R8G8B8.
			if(!HasAlpha(m_pRTARGET->m_vDESC.Format))
				Color |= 0xFF000000;

			uint16_t nFlags = 0;
			if( Flags & D3DCLEAR_TARGET )
				nFlags |= BGFX_CLEAR_COLOR;

			if( Flags & D3DCLEAR_ZBUFFER )
				nFlags |= BGFX_CLEAR_DEPTH;

			if( Flags & D3DCLEAR_STENCIL )
				nFlags |= BGFX_CLEAR_STENCIL;

			D3DVIEWPORT9 vRECT = m_vVIEWPORT;
			if( Count && pRects )
			{
				vRECT.X = pRects->x1;
				vRECT.Y = pRects->y1;
				vRECT.Width = pRects->x2 - pRects->x1;
				vRECT.Height = pRects->y2 - pRects->y1;
			}

			// A clear is the start of a new view: bgfx clears a view before its draw calls.
			// D3DCOLOR is ARGB, bgfx wants RGBA.
			bgfx::ViewId nView = OpenView( vRECT, TRUE, hTarget);
			bgfx::setViewClear( nView, nFlags, (Color << 8) | (Color >> 24), Z, uint8_t(Stencil));
			bgfx::touch(nView);

			// The next draw call needs the viewport again if the clear used other rectangles.
			m_bViewDirty = Count && pRects ? TRUE : FALSE;

			return D3D_OK;
		}

		HRESULT SetTransform( D3DTRANSFORMSTATETYPE State, const D3DMATRIX *pMatrix) override
		{
			if(!pMatrix)
				return D3DERR_INVALIDCALL;

			UINT nState = UINT(State);

			if( nState >= 256 && nState < 256 + MAX_WORLD )
			{
				m_vWORLD[nState - 256] = *pMatrix;
				m_nWorldCount = TMax( m_nWorldCount, nState - 256 + 1);
			}
			else if( nState == D3DTS_VIEW )
				m_vVIEW = *pMatrix;
			else if( nState == D3DTS_PROJECTION )
				m_vPROJ = *pMatrix;
			else if( nState >= D3DTS_TEXTURE0 && nState <= D3DTS_TEXTURE7 )
				m_vTEXMAT[nState - D3DTS_TEXTURE0] = *pMatrix;

			return D3D_OK;
		}

		HRESULT SetViewport( const D3DVIEWPORT9 *pViewport) override
		{
			if(!pViewport)
				return D3DERR_INVALIDCALL;

			if(memcmp( &m_vVIEWPORT, pViewport, sizeof(D3DVIEWPORT9)))
			{
				m_vVIEWPORT = *pViewport;
				m_bViewDirty = TRUE;
			}

			return D3D_OK;
		}

		HRESULT GetViewport( D3DVIEWPORT9 *pViewport) override
		{
			if(!pViewport)
				return D3DERR_INVALIDCALL;
			*pViewport = m_vVIEWPORT;

			return D3D_OK;
		}

		HRESULT SetMaterial( const D3DMATERIAL9 *pMaterial) override { if(pMaterial) m_vMATERIAL = *pMaterial; return D3D_OK; }
		HRESULT GetMaterial( D3DMATERIAL9 *pMaterial) override { if(pMaterial) *pMaterial = m_vMATERIAL; return D3D_OK; }
		HRESULT SetLight( DWORD Index, const D3DLIGHT9 *pLight) override { if(pLight) m_mapLIGHT[Index] = *pLight; return D3D_OK; }

		HRESULT GetLight( DWORD Index, D3DLIGHT9 *pLight) override
		{
			std::map< DWORD, D3DLIGHT9>::iterator finder = m_mapLIGHT.find(Index);

			if( !pLight || finder == m_mapLIGHT.end() )
				return D3DERR_INVALIDCALL;
			*pLight = (*finder).second;

			return D3D_OK;
		}

		HRESULT LightEnable( DWORD Index, BOOL Enable) override
		{
			// Like D3D9, enabling a light that was never set gives it the default light.
			if( Enable && m_mapLIGHT.find(Index) == m_mapLIGHT.end() )
			{
				D3DLIGHT9 vLIGHT;

				ZeroMemory( &vLIGHT, sizeof(vLIGHT));
				vLIGHT.Type = D3DLIGHT_DIRECTIONAL;
				vLIGHT.Diffuse.r = vLIGHT.Diffuse.g = vLIGHT.Diffuse.b = 1.0f;
				vLIGHT.Direction.z = 1.0f;
				m_mapLIGHT[Index] = vLIGHT;
			}

			m_mapLIGHTON[Index] = Enable;
			return D3D_OK;
		}

		HRESULT GetLightEnable( DWORD Index, BOOL *pEnable) override
		{
			std::map< DWORD, BOOL>::iterator finder = m_mapLIGHTON.find(Index);

			if( !pEnable || finder == m_mapLIGHTON.end() )
				return D3DERR_INVALIDCALL;
			*pEnable = (*finder).second;

			return D3D_OK;
		}

		HRESULT SetRenderState( D3DRENDERSTATETYPE State, DWORD Value) override
		{
			if( UINT(State) < RENDER_STATES )
				m_vRS[State] = Value;

			return D3D_OK;
		}

		HRESULT GetRenderState( D3DRENDERSTATETYPE State, DWORD *pValue) override
		{
			if( !pValue || UINT(State) >= RENDER_STATES )
				return D3DERR_INVALIDCALL;
			*pValue = m_vRS[State];

			return D3D_OK;
		}

		HRESULT SetTexture( DWORD Stage, LPTTEXTURE pTexture) override
		{
			if( Stage >= TEXTURE_STAGES )
				return D3D_OK;

			if(pTexture)
				pTexture->AddRef();

			if(m_pTEXTURE[Stage])
				m_pTEXTURE[Stage]->Release();
			m_pTEXTURE[Stage] = pTexture;

			return D3D_OK;
		}

		HRESULT SetTextureStageState( DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) override
		{
			if( Stage < TEXTURE_STAGES && UINT(Type) < STAGE_STATES )
				m_vTSS[Stage][Type] = Value;

			return D3D_OK;
		}

		HRESULT SetSamplerState( DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) override
		{
			if( Sampler < TEXTURE_STAGES && UINT(Type) < SAMPLER_STATES )
				m_vSAMP[Sampler][Type] = Value;

			return D3D_OK;
		}

		HRESULT SetSoftwareVertexProcessing( BOOL bSoftware) override { return D3D_OK; }

		HRESULT DrawPrimitive( D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) override
		{
			UINT nCount = GetVertexCount( PrimitiveType, PrimitiveCount);

			if( !nCount || !BeginDraw() )
				return D3D_OK;

			TLAYOUT *pLayout = SetStreams( INT(StartVertex), nCount, FALSE);
			if(!pLayout)
				return Skip("DP streams");

			if( PrimitiveType == D3DPT_TRIANGLEFAN && !SetFanIndices( nCount, NULL, FALSE))
				return Skip("DP fan");

			SetDumpStream( INT(StartVertex), nCount, PrimitiveCount);
			return Submit( PrimitiveType, *pLayout);
		}

		HRESULT DrawIndexedPrimitive( D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) override
		{
			UINT nCount = GetVertexCount( Type, primCount);

			if( !m_pINDEX || !nCount || !BeginDraw() )
				return D3D_OK;

			BOOL b32 = m_pINDEX->Is32();
			UINT nIndexSize = b32 ? 4 : 2;

			if( (startIndex + nCount) * nIndexSize > m_pINDEX->m_vDATA.size() )
				return Skip("DIP index range");

			// Indices are relative to BaseVertexIndex (bgfx's start vertex of an indexed draw).
			TLAYOUT *pLayout = SetStreams( BaseVertexIndex, MinVertexIndex + NumVertices, TRUE);
			if(!pLayout)
				return Skip("DIP streams");

			const BYTE *pIndex = &m_pINDEX->m_vDATA[startIndex * nIndexSize];

			if( Type == D3DPT_TRIANGLEFAN )
			{
				if(!SetFanIndices( nCount, pIndex, b32))
					return Skip("DIP fan");
			}
			else if(m_pINDEX->m_bDynamic)
			{
				if( bgfx::getAvailTransientIndexBuffer( nCount, b32 ? true : false) < nCount )
					return Skip("DIP transient indices");

				bgfx::TransientIndexBuffer vTIB;
				bgfx::allocTransientIndexBuffer( &vTIB, nCount, b32 ? true : false);
				memcpy( vTIB.data, pIndex, nCount * nIndexSize);
				bgfx::setIndexBuffer(&vTIB);
			}
			else
				bgfx::setIndexBuffer( m_pINDEX->GetHandle(), startIndex, nCount);

			SetDumpStream( BaseVertexIndex + INT(MinVertexIndex), NumVertices, primCount);
			return Submit( Type, *pLayout);
		}

		HRESULT DrawPrimitiveUP( D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void *pVertexStreamZeroData, UINT VertexStreamZeroStride) override
		{
			UINT nCount = GetVertexCount( PrimitiveType, PrimitiveCount);

			if( !pVertexStreamZeroData || !VertexStreamZeroStride || !nCount || !BeginDraw() )
				return D3D_OK;

			// A declaration can only be used here when all its elements are in stream 0.
			if( m_pVDECL && m_pVDECL->m_dwStreams != 1 )
				return Skip("UP declaration");

			TLAYOUT& vLayout = m_pVDECL ? m_pVDECL->GetLayout( 0, VertexStreamZeroStride) : GetLayout( m_dwFVF, VertexStreamZeroStride);
			if( !vLayout.m_bValid || bgfx::getAvailTransientVertexBuffer( nCount, vLayout.m_vLayout) < nCount )
				return Skip("UP layout");

			bgfx::TransientVertexBuffer vTVB;
			bgfx::allocTransientVertexBuffer( &vTVB, nCount, vLayout.m_vLayout);
			memcpy( vTVB.data, pVertexStreamZeroData, nCount * VertexStreamZeroStride);
			bgfx::setVertexBuffer( 0, &vTVB);

			if( PrimitiveType == D3DPT_TRIANGLEFAN && !SetFanIndices( nCount, NULL, FALSE))
				return Skip("UP fan");

			// Like D3D9, a UP draw unbinds stream 0.
			SetStreamSource( 0, NULL, 0, 0);

			SetDumpVertices( (const BYTE *) pVertexStreamZeroData, nCount, VertexStreamZeroStride, PrimitiveCount);
			return Submit( PrimitiveType, vLayout);
		}

		HRESULT CreateVertexDeclaration( const D3DVERTEXELEMENT9 *pVertexElements, IDirect3DVertexDeclaration9 **ppDecl) override
		{
			if( !pVertexElements || !ppDecl )
				return D3DERR_INVALIDCALL;

			*ppDecl = new CTVertexDeclBGFX(pVertexElements);
			return D3D_OK;
		}

		// Like D3D9, the last of SetFVF and SetVertexDeclaration gives the vertex format.
		HRESULT SetVertexDeclaration( IDirect3DVertexDeclaration9 *pDecl) override
		{
			if(pDecl)
				pDecl->AddRef();

			if(m_pVDECL)
				m_pVDECL->Release();
			m_pVDECL = (CTVertexDeclBGFX *) pDecl;

			return D3D_OK;
		}

		HRESULT SetFVF( DWORD FVF) override
		{
			SetVertexDeclaration(NULL);
			m_dwFVF = FVF;

			return D3D_OK;
		}

		// The engine's D3D9 shaders are not used with bgfx: CD3DDevice turns them off.
		HRESULT CreateVertexShader( const DWORD *pFunction, IDirect3DVertexShader9 **ppShader) override { if(ppShader) *ppShader = NULL; return D3DERR_NOTAVAILABLE; }
		HRESULT SetVertexShader( IDirect3DVertexShader9 *pShader) override { return D3D_OK; }
		HRESULT SetVertexShaderConstantF( UINT StartRegister, const float *pConstantData, UINT Vector4fCount) override { return D3D_OK; }
		HRESULT SetVertexShaderConstantI( UINT StartRegister, const int *pConstantData, UINT Vector4iCount) override { return D3D_OK; }
		HRESULT SetVertexShaderConstantB( UINT StartRegister, const BOOL *pConstantData, UINT BoolCount) override { return D3D_OK; }

		HRESULT SetStreamSource( UINT StreamNumber, LPTVERTEXBUFFER pStreamData, UINT OffsetInBytes, UINT Stride) override
		{
			if( StreamNumber >= STREAMS )
				return D3DERR_INVALIDCALL;

			if(pStreamData)
				pStreamData->AddRef();

			if(m_vSTREAM[StreamNumber].m_pVB)
				m_vSTREAM[StreamNumber].m_pVB->Release();

			m_vSTREAM[StreamNumber].m_pVB = (CTVertexBufferBGFX *) pStreamData;
			m_vSTREAM[StreamNumber].m_nOffset = OffsetInBytes;
			m_vSTREAM[StreamNumber].m_nStride = Stride;

			return D3D_OK;
		}

		HRESULT SetIndices( LPTINDEXBUFFER pIndexData) override
		{
			if(pIndexData)
				pIndexData->AddRef();

			if(m_pINDEX)
				m_pINDEX->Release();
			m_pINDEX = (CTIndexBufferBGFX *) pIndexData;

			return D3D_OK;
		}

		HRESULT CreatePixelShader( const DWORD *pFunction, IDirect3DPixelShader9 **ppShader) override { if(ppShader) *ppShader = NULL; return D3DERR_NOTAVAILABLE; }
		HRESULT SetPixelShader( IDirect3DPixelShader9 *pShader) override { return D3D_OK; }
		HRESULT SetPixelShaderConstantF( UINT StartRegister, const float *pConstantData, UINT Vector4fCount) override { return D3D_OK; }

		HRESULT CreateTextureD3DX( UINT Width, UINT Height, UINT MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, LPTTEXTURE *ppTexture) override
		{
			return CreateTexture( Width, Height, MipLevels == D3DX_DEFAULT ? 0 : MipLevels, Usage, Format, Pool, ppTexture, NULL);
		}

		HRESULT CreateTextureFromFile( LPCSTR pSrcFile, LPTTEXTURE *ppTexture) override
		{
			std::vector<BYTE> vDATA;

			if(!ReadFile( pSrcFile, vDATA))
				return D3DERR_NOTFOUND;

			return CreateTextureFromFileInMemoryEx( &vDATA[0], UINT(vDATA.size()), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, ppTexture);
		}

		// The image keeps its own size and format (D3DX would convert it to the
		// requested one); BC1/2/3 stay compressed, the rest becomes A8R8G8B8.
		HRESULT CreateTextureFromFileInMemoryEx( LPCVOID pSrcData, UINT SrcDataSize, UINT Width, UINT Height, UINT MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, DWORD Filter, DWORD MipFilter, D3DCOLOR ColorKey, D3DXIMAGE_INFO *pSrcInfo, PALETTEENTRY *pPalette, LPTTEXTURE *ppTexture) override
		{
			if( !ppTexture || !pSrcData || !SrcDataSize )
				return D3DERR_INVALIDCALL;

			bimg::ImageContainer *pIMAGE = ParseImage( pSrcData, SrcDataSize);
			if(!pIMAGE)
				return D3DXERR_INVALIDDATA;

			D3DFORMAT nFormat = GetD3DFormat(pIMAGE->m_format);
			UINT nLevels = GetFileLevels( pIMAGE, pSrcData, SrcDataSize);

			// D3DX keeps the levels asked for (the engine asks for the file's own count).
			if( MipLevels && MipLevels != D3DX_DEFAULT )
				nLevels = TMin( nLevels, MipLevels);

			CTTextureBGFX *pTexture = new CTTextureBGFX( pIMAGE->m_width, pIMAGE->m_height, nLevels, Usage, nFormat, Pool);

			for( UINT i=0; i<pTexture->m_nLevels; i++)
			{
				bimg::ImageMip vMIP;

				if(bimg::imageGetRawData( *pIMAGE, 0, uint8_t(i), pIMAGE->m_data, pIMAGE->m_size, vMIP))
				{
					std::vector<BYTE>& vLEVEL = pTexture->GetLevel(i);

					memcpy( &vLEVEL[0], vMIP.m_data, TMin( UINT(vMIP.m_size), UINT(vLEVEL.size())));
					pTexture->Touch(i);
				}
			}

			if(pSrcInfo)
				FillImageInfo( pIMAGE, nFormat, pSrcData, SrcDataSize, pSrcInfo);

			bimg::imageFree(pIMAGE);
			*ppTexture = pTexture;

			return D3D_OK;
		}

		HRESULT LoadSurfaceFromSurface( LPTSURFACE pDestSurface, const PALETTEENTRY *pDestPalette, const RECT *pDestRect, LPTSURFACE pSrcSurface, const PALETTEENTRY *pSrcPalette, const RECT *pSrcRect, DWORD Filter, D3DCOLOR ColorKey) override
		{
			CTSurfaceBGFX *pSRC = (CTSurfaceBGFX *) pSrcSurface;

			if( !pDestSurface || !pSRC )
				return D3DERR_INVALIDCALL;

			// Decode the whole source (a DXT level is decoded whole), then copy the rectangle.
			UINT nWidth = pSRC->m_vDESC.Width;
			UINT nHeight = pSRC->m_vDESC.Height;
			UINT nPitch;
			const BYTE *pBits = pSRC->GetBits(&nPitch);
			std::vector<BYTE> vBGRA( nWidth * nHeight * 4);

			if(!DecodeToBGRA( pSRC->m_vDESC.Format, pBits, nPitch, nWidth, nHeight, &vBGRA[0]))
				return D3DERR_NOTAVAILABLE;

			RECT vSRC = { 0, 0, LONG(nWidth), LONG(nHeight)};
			if(pSrcRect)
				vSRC = *pSrcRect;

			return WriteSurface( (CTSurfaceBGFX *) pDestSurface, pDestRect, &vBGRA[0], nWidth, nHeight, vSRC);
		}

		HRESULT LoadSurfaceFromFileInMemory( LPTSURFACE pDestSurface, const PALETTEENTRY *pDestPalette, const RECT *pDestRect, LPCVOID pSrcData, UINT SrcDataSize, const RECT *pSrcRect, DWORD Filter, D3DCOLOR ColorKey, D3DXIMAGE_INFO *pSrcInfo) override
		{
			if( !pDestSurface || !pSrcData || !SrcDataSize )
				return D3DERR_INVALIDCALL;

			bimg::ImageContainer *pIMAGE = bimg::imageParse( &g_vALLOCATOR, pSrcData, SrcDataSize, bimg::TextureFormat::BGRA8);
			if(!pIMAGE)
			{
				LogImageFailure( pSrcData, SrcDataSize, -2);
				return D3DXERR_INVALIDDATA;
			}

			if(pSrcInfo)
				FillImageInfo( pIMAGE, D3DFMT_A8R8G8B8, pSrcData, SrcDataSize, pSrcInfo);

			bimg::ImageMip vMIP;
			HRESULT hr = D3DXERR_INVALIDDATA;

			if(bimg::imageGetRawData( *pIMAGE, 0, 0, pIMAGE->m_data, pIMAGE->m_size, vMIP))
			{
				RECT vSRC = { 0, 0, LONG(pIMAGE->m_width), LONG(pIMAGE->m_height)};

				if(pSrcRect)
					vSRC = *pSrcRect;

				hr = WriteSurface( (CTSurfaceBGFX *) pDestSurface, pDestRect, vMIP.m_data, pIMAGE->m_width, pIMAGE->m_height, vSRC);
			}

			bimg::imageFree(pIMAGE);
			return hr;
		}

		HRESULT SaveSurfaceToFile( LPCSTR pDestFile, D3DXIMAGE_FILEFORMAT DestFormat, LPTSURFACE pSrcSurface, const PALETTEENTRY *pSrcPalette, const RECT *pSrcRect) override
		{
			CTSurfaceBGFX *pSRC = (CTSurfaceBGFX *) pSrcSurface;

			if( !pDestFile || !pSRC )
				return D3DERR_INVALIDCALL;

			// Render targets and depth buffers have no copy in memory.
			if( (pSRC->m_pTexture && pSRC->m_pTexture->IsTarget()) || pSRC == m_pBACKBUF || (pSRC->m_vDESC.Usage & D3DUSAGE_DEPTHSTENCIL) )
				return D3DERR_NOTAVAILABLE;

			pSRC->WaitReady();

			RECT vRECT = { 0, 0, LONG(pSRC->m_vDESC.Width), LONG(pSRC->m_vDESC.Height)};
			if(pSrcRect)
				vRECT = *pSrcRect;

			if( vRECT.left < 0 || vRECT.top < 0 || vRECT.right > LONG(pSRC->m_vDESC.Width) || vRECT.bottom > LONG(pSRC->m_vDESC.Height) || vRECT.right <= vRECT.left || vRECT.bottom <= vRECT.top )
				return D3DERR_INVALIDCALL;

			D3DFORMAT nFormat = pSRC->m_vDESC.Format;
			UINT nWidth = UINT(vRECT.right - vRECT.left);
			UINT nHeight = UINT(vRECT.bottom - vRECT.top);
			UINT nPitch;
			const BYTE *pBits = pSRC->GetBits(&nPitch);
			std::vector<BYTE> vBGRA( nWidth * nHeight * 4);

			if(IsBlockFormat(nFormat))
			{
				// A compressed surface is decoded whole, then cut.
				std::vector<BYTE> vFULL( pSRC->m_vDESC.Width * pSRC->m_vDESC.Height * 4);

				if(!DecodeToBGRA( nFormat, pBits, nPitch, pSRC->m_vDESC.Width, pSRC->m_vDESC.Height, &vFULL[0]))
					return D3DERR_NOTAVAILABLE;

				for( UINT y=0; y<nHeight; y++)
					memcpy( &vBGRA[y * nWidth * 4], &vFULL[((vRECT.top + y) * pSRC->m_vDESC.Width + vRECT.left) * 4], nWidth * 4);
			}
			else if(!DecodeToBGRA( nFormat, pBits + vRECT.top * nPitch + GetPitch( nFormat, vRECT.left), nPitch, nWidth, nHeight, &vBGRA[0]))
				return D3DERR_NOTAVAILABLE;

			return SaveBGRA( pDestFile, DestFormat, &vBGRA[0], nWidth, nHeight);
		}
		HRESULT SaveSurfaceToFileInMemory( LPD3DXBUFFER *ppDestBuf, D3DXIMAGE_FILEFORMAT DestFormat, LPTSURFACE pSrcSurface, const PALETTEENTRY *pSrcPalette, const RECT *pSrcRect) override { return D3DERR_NOTAVAILABLE; }
		HRESULT SaveTextureToFile( LPCSTR pDestFile, D3DXIMAGE_FILEFORMAT DestFormat, LPTTEXTURE pSrcTexture, const PALETTEENTRY *pSrcPalette) override
		{
			LPTSURFACE pSURF = NULL;

			if( !pSrcTexture || FAILED(pSrcTexture->GetSurfaceLevel( 0, &pSURF)) )
				return D3DERR_INVALIDCALL;

			HRESULT hr = SaveSurfaceToFile( pDestFile, DestFormat, pSURF, pSrcPalette, NULL);
			pSURF->Release();

			return hr;
		}

	private:
		enum
		{
			RENDER_STATES = 256,
			STAGE_STATES = 33,
			SAMPLER_STATES = 14,
			TEXTURE_STAGES = 8,
			SHADER_STAGES = 4,
			STREAMS = 4,
			MAX_WORLD = 256,
			MAX_LIGHTS = 8,
			VS_PARAMS = 9,
			MAX_VIEWS = 256,		// BGFX_CONFIG_MAX_VIEWS
			PS_PARAMS = 9
		};

		struct TSTREAM
		{
			CTVertexBufferBGFX *m_pVB;
			UINT m_nOffset;
			UINT m_nStride;
		};

		// A uniform and the last values sent, so unchanged values are not sent again.
		struct TUNIFORM
		{
			bgfx::UniformHandle m_hUniform;
			std::vector<FLOAT> m_vSENT;
		};

		static uint32_t GetResetFlags( const D3DPRESENT_PARAMETERS& vPresent)
		{
			return BGFX_RESET_MAXANISOTROPY | (vPresent.PresentationInterval == D3DPRESENT_INTERVAL_IMMEDIATE ? BGFX_RESET_NONE : BGFX_RESET_VSYNC);
		}

		static BOOL ReadFile( LPCSTR pFileName, std::vector<BYTE>& vDATA)
		{
			FILE *pFILE = NULL;

			if( !(pFILE = TFileOpen( pFileName, "rb")) )
				return FALSE;

			BYTE vBUF[4096];
			size_t nRead;

			while( (nRead = fread( vBUF, 1, sizeof(vBUF), pFILE)) > 0 )
				vDATA.insert( vDATA.end(), vBUF, vBUF + nRead);
			fclose(pFILE);

			return !vDATA.empty();
		}

		static D3DFORMAT GetD3DFormat( bimg::TextureFormat::Enum nFormat)
		{
			switch(nFormat)
			{
			case bimg::TextureFormat::BC1	: return D3DFMT_DXT1;
			case bimg::TextureFormat::BC2	: return D3DFMT_DXT3;
			case bimg::TextureFormat::BC3	: return D3DFMT_DXT5;
			}

			return D3DFMT_A8R8G8B8;
		}

		// An image whose format is BC1/2/3 or B, G, R, A.
		static bimg::ImageContainer *ParseImage( LPCVOID pData, UINT nSize)
		{
			bimg::ImageContainer *pIMAGE = bimg::imageParse( &g_vALLOCATOR, pData, nSize);
			int nSource = pIMAGE ? int(pIMAGE->m_format) : -1;

			if( pIMAGE && pIMAGE->m_format != bimg::TextureFormat::BC1 && pIMAGE->m_format != bimg::TextureFormat::BC2 &&
				pIMAGE->m_format != bimg::TextureFormat::BC3 && pIMAGE->m_format != bimg::TextureFormat::BGRA8 )
			{
				bimg::ImageContainer *pBGRA = bimg::imageConvert( &g_vALLOCATOR, bimg::TextureFormat::BGRA8, *pIMAGE);

				bimg::imageFree(pIMAGE);
				pIMAGE = pBGRA;
			}

			if(!pIMAGE)
				LogImageFailure( pData, nSize, nSource);

			return pIMAGE;
		}

		// Images bimg can not read are listed in %TEMP%\tclient-bgfx.log.
		static void LogImageFailure( LPCVOID pData, UINT nSize, int nSource)
		{
			static std::mutex s_mutex;
			std::lock_guard<std::mutex> lock(s_mutex);

			char vPATH[MAX_PATH];
			char vFILE[MAX_PATH];
			FILE *pFILE = NULL;

			GetTempPathA( MAX_PATH, vPATH);
			sprintf_s( vFILE, "%stclient-bgfx.log", vPATH);

			if( !(pFILE = TFileOpen( vFILE, "a")) )
				return;

			const BYTE *p = (const BYTE *) pData;
			fprintf( pFILE, "image not read: %u bytes, bimg format %d, head", nSize, nSource);

			for( UINT i=0; i<TMin( nSize, 16U); i++)
				fprintf( pFILE, " %02x", p[i]);

			// DDS: flags, four cc, bit count and masks of the pixel format.
			if( nSize >= 128 && !memcmp( p, "DDS ", 4) )
			{
				const DWORD *pPF = (const DWORD *) (p + 76);
				fprintf( pFILE, " | dds %ux%u mips %u pf flags %x fourcc %.4s bits %u masks %08x %08x %08x %08x caps2 %x",
					((const DWORD *) p)[4], ((const DWORD *) p)[3], ((const DWORD *) p)[7],
					pPF[1], (const char *) &pPF[2], pPF[3], pPF[4], pPF[5], pPF[6], pPF[7], ((const DWORD *) p)[28]);
			}

			fprintf( pFILE, "\n");
			fclose(pFILE);
		}

		// The levels a file really holds. bimg counts a full chain for any DDS with mipmaps,
		// while the game's DDS files hold 5 levels (DDSD_MIPMAPCOUNT): the levels past them
		// would be read beyond the data.
		static UINT GetFileLevels( bimg::ImageContainer *pIMAGE, LPCVOID pData, UINT nSize)
		{
			const BYTE *p = (const BYTE *) pData;
			UINT nLevels = pIMAGE->m_numMips;

			if( nSize >= 128 && !memcmp( p, "DDS ", 4) )
			{
				DWORD dwFlags = ((const DWORD *) p)[2];
				DWORD dwCount = ((const DWORD *) p)[7];

				nLevels = TMin( nLevels, (dwFlags & 0x00020000) && dwCount ? UINT(dwCount) : 1U);
			}

			return TMax( nLevels, 1U);
		}

		static void FillImageInfo( bimg::ImageContainer *pIMAGE, D3DFORMAT nFormat, LPCVOID pData, UINT nSize, D3DXIMAGE_INFO *pInfo)
		{
			const BYTE *p = (const BYTE *) pData;

			ZeroMemory( pInfo, sizeof(D3DXIMAGE_INFO));
			pInfo->Width = pIMAGE->m_width;
			pInfo->Height = pIMAGE->m_height;
			pInfo->Depth = 1;
			pInfo->MipLevels = GetFileLevels( pIMAGE, pData, nSize);
			pInfo->Format = nFormat;
			pInfo->ResourceType = D3DRTYPE_TEXTURE;

			if( nSize >= 4 && !memcmp( p, "DDS ", 4) )
				pInfo->ImageFileFormat = D3DXIFF_DDS;
			else if( nSize >= 4 && !memcmp( p, "\x89PNG", 4) )
				pInfo->ImageFileFormat = D3DXIFF_PNG;
			else if( nSize >= 2 && p[0] == 0xFF && p[1] == 0xD8 )
				pInfo->ImageFileFormat = D3DXIFF_JPG;
			else if( nSize >= 2 && p[0] == 'B' && p[1] == 'M' )
				pInfo->ImageFileFormat = D3DXIFF_BMP;
			else
				pInfo->ImageFileFormat = D3DXIFF_TGA;
		}

		// Copies the rectangle vSRC of a B, G, R, A image into a surface (nearest pixel when
		// the sizes differ, like D3DX_FILTER_POINT).
		static HRESULT WriteSurface( CTSurfaceBGFX *pDEST, const RECT *pDestRect, const BYTE *pBGRA, UINT nWidth, UINT nHeight, RECT vSRC)
		{
			D3DFORMAT nFormat = pDEST->m_vDESC.Format;
			UINT nDestW = pDEST->m_vDESC.Width;
			UINT nDestH = pDEST->m_vDESC.Height;
			RECT vDEST = { 0, 0, LONG(nDestW), LONG(nDestH)};

			if(pDestRect)
				vDEST = *pDestRect;

			vSRC.left = TMin( UINT(vSRC.left), nWidth);
			vSRC.right = TMin( UINT(vSRC.right), nWidth);
			vSRC.top = TMin( UINT(vSRC.top), nHeight);
			vSRC.bottom = TMin( UINT(vSRC.bottom), nHeight);
			vDEST.right = TMin( UINT(vDEST.right), nDestW);
			vDEST.bottom = TMin( UINT(vDEST.bottom), nDestH);

			LONG nW = vDEST.right - vDEST.left;
			LONG nH = vDEST.bottom - vDEST.top;
			LONG nSrcW = vSRC.right - vSRC.left;
			LONG nSrcH = vSRC.bottom - vSRC.top;

			if( nW <= 0 || nH <= 0 || nSrcW <= 0 || nSrcH <= 0 )
				return D3D_OK;

			// A DXT destination is decoded, written and stays B, G, R, A in memory only if
			// it can be encoded back, which it cannot: such copies are not supported.
			if(IsBlockFormat(nFormat))
				return D3DERR_NOTAVAILABLE;

			std::vector<BYTE> vROW( nW * 4);
			UINT nPitch;
			BYTE *pBits = pDEST->GetBits(&nPitch);
			UINT nBytes = GetBitsPerPixel(nFormat) / 8;

			for( LONG y=0; y<nH; y++)
			{
				LONG nSrcY = vSRC.top + y * nSrcH / nH;

				for( LONG x=0; x<nW; x++)
				{
					LONG nSrcX = vSRC.left + x * nSrcW / nW;
					memcpy( &vROW[x * 4], pBGRA + (nSrcY * nWidth + nSrcX) * 4, 4);
				}

				EncodeFromBGRA( nFormat, &vROW[0], nW, 1, pBits + (vDEST.top + y) * nPitch + vDEST.left * nBytes, nPitch);
			}

			pDEST->Touch();
			return D3D_OK;
		}

		void InitPipeline()
		{
			bgfx::setDebug(BGFX_DEBUG_TEXT);
			bgfx::setPaletteColor( 0, 0.0f, 0.0f, 0.0f, 0.0f);

			m_nRenderer = bgfx::getRendererType();
			m_bHomogeneousDepth = bgfx::getCaps()->homogeneousDepth ? TRUE : FALSE;
			m_hFS = CreateShader(g_vFFPFS);

			m_vWORLDU.m_hUniform = CreateUniform( "u_tWorld", MAX_WORLD * 3);
			m_vVIEWPROJU.m_hUniform = CreateUniform( "u_tViewProj", 4);
			m_vVIEWU.m_hUniform = CreateUniform( "u_tView", 4);
			m_vVSU.m_hUniform = CreateUniform( "u_tVS", VS_PARAMS);
			m_vMATERIALU.m_hUniform = CreateUniform( "u_tMaterial", 4);
			m_vLIGHTSU.m_hUniform = CreateUniform( "u_tLights", MAX_LIGHTS * 7);
			m_vTEXXFORMU.m_hUniform = CreateUniform( "u_tTexXform", 16);
			m_vSTAGEU.m_hUniform = CreateUniform( "u_tStage", 16);
			m_vPSU.m_hUniform = CreateUniform( "u_tPS", PS_PARAMS);

			static const char *s_vSAMPLER[SHADER_STAGES] = { "s_tex0", "s_tex1", "s_tex2", "s_tex3" };
			for( int i=0; i<SHADER_STAGES; i++)
			{
				m_hSAMPLER[i] = bgfx::createUniform( s_vSAMPLER[i], bgfx::UniformType::Sampler);
				m_vUNIFORM.push_back(m_hSAMPLER[i]);
			}

			m_nView = 0;
			m_bViewOpen = FALSE;
			m_bViewDirty = TRUE;
			m_bFlipY = FALSE;
			m_bHighlight = FALSE;
			m_nHighlight = 0;
			m_nDraws = 0;
			m_nSkipped = 0;
		}

		bgfx::UniformHandle CreateUniform( const char *pName, uint16_t nCount)
		{
			bgfx::UniformHandle hUniform = bgfx::createUniform( pName, bgfx::UniformType::Vec4, nCount);

			m_vUNIFORM.push_back(hUniform);
			return hUniform;
		}

		bgfx::ShaderHandle CreateShader( const TFFPSHADER& vShader)
		{
			const unsigned char *pData = vShader.m_pDX11;
			unsigned int nSize = vShader.m_nDX11;

			if( m_nRenderer == bgfx::RendererType::Vulkan )
			{
				pData = vShader.m_pSPIRV;
				nSize = vShader.m_nSPIRV;
			}
			else if( m_nRenderer == bgfx::RendererType::OpenGL )
			{
				pData = vShader.m_pGLSL;
				nSize = vShader.m_nGLSL;
			}

			bgfx::ShaderHandle hShader = bgfx::createShader(bgfx::makeRef( pData, nSize));
			m_vSHADER.push_back(hShader);

			return hShader;
		}

		bgfx::ProgramHandle GetProgram( DWORD dwShader)
		{
			std::map< DWORD, bgfx::ProgramHandle>::iterator finder = m_mapPROGRAM.find(dwShader);

			if( finder != m_mapPROGRAM.end() )
				return (*finder).second;

			bgfx::ProgramHandle hProgram = BGFX_INVALID_HANDLE;
			for( size_t i=0; i<sizeof(g_vFFPVS) / sizeof(g_vFFPVS[0]); i++)
				if( g_vFFPVS[i].m_dwKey == dwShader )
				{
					hProgram = bgfx::createProgram( CreateShader(g_vFFPVS[i]), m_hFS, false);
					break;
				}

			m_mapPROGRAM[dwShader] = hProgram;
			return hProgram;
		}

		TLAYOUT& GetLayout( DWORD dwFVF, UINT nStride)
		{
			DWORD dwKey = dwFVF ^ (nStride << 24);
			std::map< DWORD, TLAYOUT>::iterator finder = m_mapLAYOUT.find(dwKey);

			if( finder != m_mapLAYOUT.end() )
				return (*finder).second;

			return m_mapLAYOUT[dwKey] = MakeLayout( dwFVF, nStride);
		}

		void ResetStates()
		{
			ZeroMemory( m_vRS, sizeof(m_vRS));
			ZeroMemory( m_vTSS, sizeof(m_vTSS));
			ZeroMemory( m_vSAMP, sizeof(m_vSAMP));
			ZeroMemory( &m_vMATERIAL, sizeof(m_vMATERIAL));
			m_mapLIGHT.clear();
			m_mapLIGHTON.clear();

			// The D3D9 defaults the engine relies on.
			m_vRS[D3DRS_ZENABLE] = D3DZB_TRUE;
			m_vRS[D3DRS_FILLMODE] = D3DFILL_SOLID;
			m_vRS[D3DRS_SHADEMODE] = D3DSHADE_GOURAUD;
			m_vRS[D3DRS_ZWRITEENABLE] = TRUE;
			m_vRS[D3DRS_SRCBLEND] = D3DBLEND_ONE;
			m_vRS[D3DRS_DESTBLEND] = D3DBLEND_ZERO;
			m_vRS[D3DRS_CULLMODE] = D3DCULL_CCW;
			m_vRS[D3DRS_ZFUNC] = D3DCMP_LESSEQUAL;
			m_vRS[D3DRS_ALPHAFUNC] = D3DCMP_ALWAYS;
			m_vRS[D3DRS_FOGEND] = 0x3F800000;
			m_vRS[D3DRS_FOGDENSITY] = 0x3F800000;
			m_vRS[D3DRS_TEXTUREFACTOR] = 0xFFFFFFFF;
			m_vRS[D3DRS_LIGHTING] = TRUE;
			m_vRS[D3DRS_COLORVERTEX] = TRUE;
			m_vRS[D3DRS_LOCALVIEWER] = TRUE;
			m_vRS[D3DRS_DIFFUSEMATERIALSOURCE] = D3DMCS_COLOR1;
			m_vRS[D3DRS_SPECULARMATERIALSOURCE] = D3DMCS_COLOR2;
			m_vRS[D3DRS_AMBIENTMATERIALSOURCE] = D3DMCS_MATERIAL;
			m_vRS[D3DRS_EMISSIVEMATERIALSOURCE] = D3DMCS_MATERIAL;
			m_vRS[D3DRS_COLORWRITEENABLE] = 0x0F;
			m_vRS[D3DRS_BLENDOP] = D3DBLENDOP_ADD;
			m_vRS[D3DRS_SRCBLENDALPHA] = D3DBLEND_ONE;
			m_vRS[D3DRS_DESTBLENDALPHA] = D3DBLEND_ZERO;
			m_vRS[D3DRS_BLENDOPALPHA] = D3DBLENDOP_ADD;
			m_vRS[D3DRS_BLENDFACTOR] = 0xFFFFFFFF;
			m_vRS[D3DRS_POINTSIZE] = 0x3F800000;

			for( int i=0; i<TEXTURE_STAGES; i++)
			{
				m_vTSS[i][D3DTSS_COLOROP] = i ? D3DTOP_DISABLE : D3DTOP_MODULATE;
				m_vTSS[i][D3DTSS_ALPHAOP] = i ? D3DTOP_DISABLE : D3DTOP_SELECTARG1;
				m_vTSS[i][D3DTSS_COLORARG1] = D3DTA_TEXTURE;
				m_vTSS[i][D3DTSS_COLORARG2] = D3DTA_CURRENT;
				m_vTSS[i][D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
				m_vTSS[i][D3DTSS_ALPHAARG2] = D3DTA_CURRENT;
				m_vTSS[i][D3DTSS_COLORARG0] = D3DTA_CURRENT;
				m_vTSS[i][D3DTSS_ALPHAARG0] = D3DTA_CURRENT;
				m_vTSS[i][D3DTSS_RESULTARG] = D3DTA_CURRENT;
				m_vTSS[i][D3DTSS_TEXCOORDINDEX] = i;
				m_vTSS[i][D3DTSS_TEXTURETRANSFORMFLAGS] = D3DTTFF_DISABLE;

				m_vSAMP[i][D3DSAMP_ADDRESSU] = D3DTADDRESS_WRAP;
				m_vSAMP[i][D3DSAMP_ADDRESSV] = D3DTADDRESS_WRAP;
				m_vSAMP[i][D3DSAMP_ADDRESSW] = D3DTADDRESS_WRAP;
				m_vSAMP[i][D3DSAMP_MAGFILTER] = D3DTEXF_POINT;
				m_vSAMP[i][D3DSAMP_MINFILTER] = D3DTEXF_POINT;
				m_vSAMP[i][D3DSAMP_MIPFILTER] = D3DTEXF_NONE;
				m_vSAMP[i][D3DSAMP_MAXANISOTROPY] = 1;
			}

			D3DXMATRIX vIDENTITY;
			D3DXMatrixIdentity(&vIDENTITY);

			for( int i=0; i<MAX_WORLD; i++)
				m_vWORLD[i] = vIDENTITY;

			for( int i=0; i<TEXTURE_STAGES; i++)
				m_vTEXMAT[i] = vIDENTITY;

			m_vVIEW = vIDENTITY;
			m_vPROJ = vIDENTITY;
			m_nWorldCount = 1;
		}

		void ResetViewport()
		{
			m_vVIEWPORT.X = 0;
			m_vVIEWPORT.Y = 0;
			m_vVIEWPORT.Width = m_pRTARGET ? m_pRTARGET->m_vDESC.Width : m_vPRESENT.BackBufferWidth;
			m_vVIEWPORT.Height = m_pRTARGET ? m_pRTARGET->m_vDESC.Height : m_vPRESENT.BackBufferHeight;
			m_vVIEWPORT.MinZ = 0.0f;
			m_vVIEWPORT.MaxZ = 1.0f;
			m_bViewDirty = TRUE;
		}

		static BOOL HasAlpha( D3DFORMAT nFormat)
		{
			return nFormat == D3DFMT_A8R8G8B8 || nFormat == D3DFMT_A8B8G8R8 || nFormat == D3DFMT_A4R4G4B4 || nFormat == D3DFMT_A1R5G5B5 || nFormat == D3DFMT_A16B16G16R16F || nFormat == D3DFMT_A32B32G32R32F;
		}

		// The frame buffer of the render target: the back buffer (an invalid handle) or level 0
		// of a render target texture. FALSE when it can not be drawn into.
		BOOL GetTarget( bgfx::FrameBufferHandle *pTarget)
		{
			*pTarget = BGFX_INVALID_HANDLE;

			if( m_pRTARGET == m_pBACKBUF )
				return TRUE;

			CTTextureBGFX *pTexture = m_pRTARGET->m_pTexture;
			if( !pTexture || !pTexture->IsTarget() || m_pRTARGET->m_nLevel )
				return FALSE;

			*pTarget = pTexture->GetFrameBuffer(m_pDEPTH);
			return bgfx::isValid(*pTarget);
		}

		// A new bgfx view on the render target: bgfx keeps the order of the draw calls of a
		// sequential view, views run in order, and a view has one rectangle and one clear.
		bgfx::ViewId OpenView( const D3DVIEWPORT9& vRECT, BOOL bClear, bgfx::FrameBufferHandle hTarget)
		{
			if( m_bViewOpen && m_nView + 1 < MAX_VIEWS )
				m_nView++;

			bgfx::ViewId nView = bgfx::ViewId(m_nView);

			// OpenGL textures have their origin at the bottom: render targets are drawn upside
			// down so they are sampled like D3D9's.
			m_bFlipY = bgfx::isValid(hTarget) && bgfx::getCaps()->originBottomLeft;
			bgfx::setViewFrameBuffer( nView, hTarget);
			bgfx::setViewMode( nView, bgfx::ViewMode::Sequential);
			bgfx::setViewRect( nView, uint16_t(vRECT.X), uint16_t(vRECT.Y), uint16_t(vRECT.Width), uint16_t(vRECT.Height));

			if(!bClear)
				bgfx::setViewClear( nView, BGFX_CLEAR_NONE);

			m_bViewOpen = TRUE;
			m_bViewDirty = FALSE;

			return nView;
		}

		// Checks a draw call can be done, and opens a view for it when needed.
		BOOL BeginDraw()
		{
			bgfx::FrameBufferHandle hTarget;

			if(!GetTarget(&hTarget))
			{
				Skip("render target");
				return FALSE;
			}

			if( !m_bViewOpen || m_bViewDirty )
				OpenView( m_vVIEWPORT, FALSE, hTarget);

			return TRUE;
		}

		// A draw call that can not be done. Anything already set for it is dropped.
		HRESULT Skip( const char *pReason)
		{
			bgfx::discard();
			m_nSkipped++;

			if(m_pDUMP)
				fprintf( m_pDUMP, "skip %s: fvf=%x decl=%u stride=%u vb=%p\n", pReason, m_dwFVF, m_pVDECL ? m_pVDECL->m_dwStreams : 0, m_vSTREAM[0].m_nStride, (void *) m_vSTREAM[0].m_pVB);

			return D3D_OK;
		}

		// Binds the vertex streams of a draw call: stream 0 seen through the FVF, or the
		// streams of the vertex declaration. nStart is added to each stream's first vertex
		// and nCount vertices are needed from there; bAll binds static buffers up to their
		// end, for indexed draws. Returns the layout giving the shader variant, or NULL.
		TLAYOUT *SetStreams( INT nStart, UINT nCount, BOOL bAll)
		{
			if(!m_pVDECL)
			{
				TSTREAM& vStream = m_vSTREAM[0];

				if( !vStream.m_pVB || !vStream.m_nStride )
					return NULL;

				TLAYOUT& vLayout = GetLayout( m_dwFVF, vStream.m_nStride);
				if( !vLayout.m_bValid || !SetStream( 0, vStream, vLayout, m_dwFVF ^ (vStream.m_nStride << 24), nStart, nCount, bAll) )
					return NULL;

				return &vLayout;
			}

			if(!m_pVDECL->m_vMAIN.m_bValid)
				return NULL;

			uint8_t nSlot = 0;
			for( UINT i=0; i<STREAMS; i++)
				if( m_pVDECL->m_dwStreams & (1 << i) )
				{
					TSTREAM& vStream = m_vSTREAM[i];

					if( !vStream.m_pVB || !vStream.m_nStride )
						return NULL;

					// Layout keys of declarations are above the 32-bit FVF keys.
					TLAYOUT& vLayout = m_pVDECL->GetLayout( i, vStream.m_nStride);
					UINT64 qwKey = (UINT64(m_pVDECL->m_dwSerial) << 40) | (UINT64(i) << 32) | vStream.m_nStride;

					if( !vLayout.m_bValid || !SetStream( nSlot++, vStream, vLayout, qwKey, nStart, nCount, bAll) )
						return NULL;
				}

			return &m_pVDECL->m_vMAIN;
		}

		BOOL SetStream( uint8_t nSlot, TSTREAM& vStream, TLAYOUT& vLayout, UINT64 qwKey, INT nStart, UINT nCount, BOOL bAll)
		{
			CTVertexBufferBGFX *pVB = vStream.m_pVB;
			INT nFirst = INT(vStream.m_nOffset / vStream.m_nStride) + nStart;
			UINT nTotal = UINT(pVB->m_vDATA.size() / vStream.m_nStride);

			if( nFirst < 0 || UINT(nFirst) + nCount > nTotal )
				return FALSE;

			if(pVB->m_bDynamic)
			{
				if( bgfx::getAvailTransientVertexBuffer( nCount, vLayout.m_vLayout) < nCount )
					return FALSE;

				bgfx::TransientVertexBuffer vTVB;
				bgfx::allocTransientVertexBuffer( &vTVB, nCount, vLayout.m_vLayout);
				memcpy( vTVB.data, &pVB->m_vDATA[nFirst * vStream.m_nStride], nCount * vStream.m_nStride);
				bgfx::setVertexBuffer( nSlot, &vTVB);
			}
			else
				bgfx::setVertexBuffer( nSlot, pVB->GetHandle( qwKey, vLayout.m_vLayout), UINT(nFirst), bAll ? nTotal - UINT(nFirst) : nCount);

			return TRUE;
		}

		void SetDumpStream( INT nStart, UINT nCount, UINT nPrimitives)
		{
			TSTREAM& vStream = m_vSTREAM[0];
			INT nFirst = vStream.m_nStride ? INT(vStream.m_nOffset / vStream.m_nStride) + nStart : -1;

			if( !vStream.m_pVB || nFirst < 0 || (UINT(nFirst) + nCount) * vStream.m_nStride > vStream.m_pVB->m_vDATA.size() )
				SetDumpVertices( NULL, 0, 0, nPrimitives);
			else
				SetDumpVertices( &vStream.m_pVB->m_vDATA[nFirst * vStream.m_nStride], nCount, vStream.m_nStride, nPrimitives);
		}

		// bgfx has no triangle fans: they become indexed triangle lists.
		BOOL SetFanIndices( UINT nCount, const BYTE *pIndex, BOOL b32)
		{
			UINT nTriangles = nCount - 2;

			if( bgfx::getAvailTransientIndexBuffer( nTriangles * 3, true) < nTriangles * 3 )
				return FALSE;

			bgfx::TransientIndexBuffer vTIB;
			bgfx::allocTransientIndexBuffer( &vTIB, nTriangles * 3, true);
			uint32_t *pOut = (uint32_t *) vTIB.data;

			for( UINT i=0; i<nTriangles; i++)
			{
				UINT vFAN[3] = { 0, i + 1, i + 2};

				for( int j=0; j<3; j++)
				{
					UINT n = vFAN[j];
					*pOut++ = pIndex ? (b32 ? ((const uint32_t *) pIndex)[n] : ((const uint16_t *) pIndex)[n]) : n;
				}
			}

			bgfx::setIndexBuffer(&vTIB);
			return TRUE;
		}

		void SendUniform( TUNIFORM& vUniform, const FLOAT *pData, UINT nCount)
		{
			UINT nFloats = nCount * 4;

			if( vUniform.m_vSENT.size() >= nFloats && !memcmp( &vUniform.m_vSENT[0], pData, nFloats * sizeof(FLOAT)) )
				return;

			if( vUniform.m_vSENT.size() < nFloats )
				vUniform.m_vSENT.resize(nFloats);
			memcpy( &vUniform.m_vSENT[0], pData, nFloats * sizeof(FLOAT));

			bgfx::setUniform( vUniform.m_hUniform, pData, uint16_t(nCount));
		}

		// The transpose rows of a D3D matrix: row j holds column j, so p * M is dot(p, row j).
		static void MatrixRows( const D3DMATRIX& m, FLOAT *pOut, UINT nRows)
		{
			for( UINT j=0; j<nRows; j++)
				SetVec4( pOut + j * 4, m.m[0][j], m.m[1][j], m.m[2][j], m.m[3][j]);
		}

		static FLOAT GetMaterialSource( DWORD dwSource)
		{
			return dwSource == D3DMCS_COLOR1 ? 1.0f : dwSource == D3DMCS_COLOR2 ? 2.0f : 0.0f;
		}

		void SetVertexUniforms( DWORD dwShader)
		{
			BOOL bBlend = (dwShader & 3) == 2;
			DWORD dwBlend = m_vRS[D3DRS_VERTEXBLEND];
			FLOAT fMode = 0.0f;
			FLOAT fWeights = 0.0f;

			if( bBlend && dwBlend != D3DVBF_DISABLE && dwBlend != D3DVBF_TWEENING )
			{
				fMode = m_vRS[D3DRS_INDEXEDVERTEXBLENDENABLE] ? 2.0f : 1.0f;
				fWeights = dwBlend == D3DVBF_0WEIGHTS ? 0.0f : FLOAT(dwBlend);
			}

			// The palette: the whole used range when blending, else only world matrix 0.
			static FLOAT vWORLD[MAX_WORLD * 3 * 4];
			UINT nWorld = fMode > 0.0f ? (fMode > 1.5f ? m_nWorldCount : 4) : 1;

			for( UINT i=0; i<nWorld; i++)
				MatrixRows( m_vWORLD[i], &vWORLD[i * 12], 3);
			SendUniform( m_vWORLDU, vWORLD, nWorld * 3);

			D3DXMATRIX vVIEWPROJ = D3DXMATRIX(m_vVIEW) * D3DXMATRIX(m_vPROJ);
			FLOAT vMATRIX[16];

			MatrixRows( vVIEWPROJ, vMATRIX, 4);
			SendUniform( m_vVIEWPROJU, vMATRIX, 4);

			MatrixRows( m_vVIEW, vMATRIX, 4);
			SendUniform( m_vVIEWU, vMATRIX, 4);

			FLOAT vVS[VS_PARAMS * 4];
			D3DXMATRIX vINVVIEW;

			if(!D3DXMatrixInverse( &vINVVIEW, NULL, (const D3DXMATRIX *) &m_vVIEW))
				D3DXMatrixIdentity(&vINVVIEW);

			DWORD dwFogMode = m_vRS[D3DRS_FOGTABLEMODE] != D3DFOG_NONE ? m_vRS[D3DRS_FOGTABLEMODE] : m_vRS[D3DRS_FOGVERTEXMODE];
			FLOAT fFogMode = dwFogMode == D3DFOG_LINEAR ? 3.0f : dwFogMode == D3DFOG_EXP2 ? 2.0f : dwFogMode == D3DFOG_EXP ? 1.0f : 0.0f;

			SetVec4( &vVS[0], fMode, fWeights, m_vRS[D3DRS_LIGHTING] ? 1.0f : 0.0f, m_vRS[D3DRS_NORMALIZENORMALS] ? 1.0f : 0.0f);
			SetVec4( &vVS[4],
				GetMaterialSource(m_vRS[D3DRS_DIFFUSEMATERIALSOURCE]),
				GetMaterialSource(m_vRS[D3DRS_SPECULARMATERIALSOURCE]),
				GetMaterialSource(m_vRS[D3DRS_AMBIENTMATERIALSOURCE]),
				GetMaterialSource(m_vRS[D3DRS_EMISSIVEMATERIALSOURCE]));
			SetVec4( &vVS[8], m_vRS[D3DRS_COLORVERTEX] ? 1.0f : 0.0f, m_vRS[D3DRS_SPECULARENABLE] ? 1.0f : 0.0f, 0.0f, m_vMATERIAL.Power);
			ColorToVec4( m_vRS[D3DRS_AMBIENT], &vVS[12]);
			SetVec4( &vVS[16], FLOAT(m_vVIEWPORT.X), FLOAT(m_vVIEWPORT.Y), FLOAT(TMax( 1U, m_vVIEWPORT.Width)), FLOAT(TMax( 1U, m_vVIEWPORT.Height)));
			SetVec4( &vVS[20], m_vVIEWPORT.MinZ, m_vVIEWPORT.MaxZ, m_bHomogeneousDepth ? 1.0f : 0.0f, DWToFloat(m_vRS[D3DRS_DEPTHBIAS]));
			SetVec4( &vVS[24], fFogMode, DWToFloat(m_vRS[D3DRS_FOGSTART]), DWToFloat(m_vRS[D3DRS_FOGEND]), DWToFloat(m_vRS[D3DRS_FOGDENSITY]));
			SetVec4( &vVS[28], m_vRS[D3DRS_RANGEFOGENABLE] ? 1.0f : 0.0f, m_vRS[D3DRS_FOGENABLE] ? 1.0f : 0.0f, m_bFlipY ? 1.0f : 0.0f, 0.0f);
			SetVec4( &vVS[32], vINVVIEW._41, vINVVIEW._42, vINVVIEW._43, 1.0f);
			SendUniform( m_vVSU, vVS, VS_PARAMS);

			FLOAT vMATERIAL[16];
			SetColor( &vMATERIAL[0], m_vMATERIAL.Diffuse);
			SetColor( &vMATERIAL[4], m_vMATERIAL.Ambient);
			SetColor( &vMATERIAL[8], m_vMATERIAL.Specular);
			SetColor( &vMATERIAL[12], m_vMATERIAL.Emissive);
			SendUniform( m_vMATERIALU, vMATERIAL, 4);

			FLOAT vLIGHTS[MAX_LIGHTS * 7 * 4];
			ZeroMemory( vLIGHTS, sizeof(vLIGHTS));

			for( DWORD i=0; i<MAX_LIGHTS; i++)
			{
				std::map< DWORD, BOOL>::iterator itON = m_mapLIGHTON.find(i);
				std::map< DWORD, D3DLIGHT9>::iterator itLIGHT = m_mapLIGHT.find(i);

				if( itON == m_mapLIGHTON.end() || !(*itON).second || itLIGHT == m_mapLIGHT.end() )
					continue;

				const D3DLIGHT9& vLIGHT = (*itLIGHT).second;
				FLOAT *pLIGHT = &vLIGHTS[i * 28];

				SetVec4( &pLIGHT[0], FLOAT(vLIGHT.Type), 1.0f, vLIGHT.Range > 0.0f ? vLIGHT.Range : 1.0e30f, 0.0f);
				SetVec4( &pLIGHT[4], vLIGHT.Position.x, vLIGHT.Position.y, vLIGHT.Position.z, 1.0f);
				SetVec4( &pLIGHT[8], vLIGHT.Direction.x, vLIGHT.Direction.y, vLIGHT.Direction.z, 0.0f);
				SetColor( &pLIGHT[12], vLIGHT.Diffuse);
				SetColor( &pLIGHT[16], vLIGHT.Specular);
				SetColor( &pLIGHT[20], vLIGHT.Ambient);
				SetVec4( &pLIGHT[24], vLIGHT.Attenuation0, vLIGHT.Attenuation1, vLIGHT.Attenuation2, vLIGHT.Falloff);
			}
			SendUniform( m_vLIGHTSU, vLIGHTS, MAX_LIGHTS * 7);

			FLOAT vTEX[16 * 4];
			ZeroMemory( vTEX, sizeof(vTEX));

			for( int i=0; i<SHADER_STAGES; i++)
			{
				BOOL bTransform = (m_vTSS[i][D3DTSS_TEXTURETRANSFORMFLAGS] & 0xFF) != D3DTTFF_DISABLE;

				MatrixRows( m_vTEXMAT[i], &vTEX[i * 16], 2);
				SetVec4( &vTEX[i * 16 + 8], FLOAT(m_vTSS[i][D3DTSS_TEXCOORDINDEX] & 0xFFFF), bTransform ? 1.0f : 0.0f, 0.0f, 0.0f);
			}
			SendUniform( m_vTEXXFORMU, vTEX, 16);
		}

		// D3DTA_* as select (0 diffuse, 1 current, 2 texture, 3 factor, 4 specular, 5 temp,
		// 6 constant) and modifier (complement 1 + alpha replicate 2).
		static void StageArg( DWORD dwArg, FLOAT *pSelect, FLOAT *pModifier)
		{
			*pSelect = FLOAT(dwArg & D3DTA_SELECTMASK);
			*pModifier = ((dwArg & D3DTA_COMPLEMENT) ? 1.0f : 0.0f) + ((dwArg & D3DTA_ALPHAREPLICATE) ? 2.0f : 0.0f);
		}

		void SetPixelUniforms()
		{
			FLOAT vSTAGE[16 * 4];
			BOOL bDisabled = FALSE;

			for( int i=0; i<SHADER_STAGES; i++)
			{
				FLOAT *p = &vSTAGE[i * 16];
				DWORD *pTSS = m_vTSS[i];
				DWORD dwColorOp = bDisabled ? D3DTOP_DISABLE : pTSS[D3DTSS_COLOROP];

				if( dwColorOp == D3DTOP_DISABLE )
					bDisabled = TRUE;

				p[0] = FLOAT(dwColorOp);
				StageArg( pTSS[D3DTSS_COLORARG1], &p[1], &p[4]);
				StageArg( pTSS[D3DTSS_COLORARG2], &p[2], &p[5]);
				StageArg( pTSS[D3DTSS_COLORARG0], &p[3], &p[6]);
				p[7] = 0.0f;

				p[8] = FLOAT(pTSS[D3DTSS_ALPHAOP]);
				StageArg( pTSS[D3DTSS_ALPHAARG1], &p[9], &p[12]);
				StageArg( pTSS[D3DTSS_ALPHAARG2], &p[10], &p[13]);
				StageArg( pTSS[D3DTSS_ALPHAARG0], &p[11], &p[14]);
				p[15] = 0.0f;
			}
			SendUniform( m_vSTAGEU, vSTAGE, 16);

			FLOAT vPS[PS_PARAMS * 4];
			ColorToVec4( m_vRS[D3DRS_TEXTUREFACTOR], &vPS[0]);
			SetVec4( &vPS[4], m_vRS[D3DRS_ALPHATESTENABLE] ? 1.0f : 0.0f, FLOAT(m_vRS[D3DRS_ALPHAFUNC]), FLOAT(m_vRS[D3DRS_ALPHAREF] & 0xFF), m_vRS[D3DRS_SPECULARENABLE] ? 1.0f : 0.0f);
			ColorToVec4( m_vRS[D3DRS_FOGCOLOR], &vPS[8]);
			vPS[11] = m_vRS[D3DRS_FOGENABLE] ? 1.0f : 0.0f;

			for( int i=0; i<SHADER_STAGES; i++)
			{
				vPS[12 + i] = m_vSTAGEBOUND[i] ? 1.0f : 0.0f;
				vPS[16 + i] = DWToFloat(m_vSAMP[i][D3DSAMP_MIPMAPLODBIAS]);
				ColorToVec4( m_vTSS[i][D3DTSS_CONSTANT], &vPS[20 + i * 4]);
			}
			SendUniform( m_vPSU, vPS, PS_PARAMS);
		}

		static uint32_t GetAddressFlags( DWORD dwAddress, uint32_t dwMirror, uint32_t dwClamp, uint32_t dwBorder)
		{
			switch(dwAddress)
			{
			case D3DTADDRESS_MIRROR		:
			case D3DTADDRESS_MIRRORONCE	: return dwMirror;
			case D3DTADDRESS_CLAMP		: return dwClamp;
			case D3DTADDRESS_BORDER		: return dwBorder;
			}

			return 0;
		}

		uint32_t GetSamplerFlags( int nStage)
		{
			DWORD *pSAMP = m_vSAMP[nStage];
			uint32_t dwFlags = BGFX_SAMPLER_BORDER_COLOR(0);

			dwFlags |= GetAddressFlags( pSAMP[D3DSAMP_ADDRESSU], BGFX_SAMPLER_U_MIRROR, BGFX_SAMPLER_U_CLAMP, BGFX_SAMPLER_U_BORDER);
			dwFlags |= GetAddressFlags( pSAMP[D3DSAMP_ADDRESSV], BGFX_SAMPLER_V_MIRROR, BGFX_SAMPLER_V_CLAMP, BGFX_SAMPLER_V_BORDER);

			if( pSAMP[D3DSAMP_MAGFILTER] == D3DTEXF_POINT )
				dwFlags |= BGFX_SAMPLER_MAG_POINT;
			else if( pSAMP[D3DSAMP_MAGFILTER] == D3DTEXF_ANISOTROPIC )
				dwFlags |= BGFX_SAMPLER_MAG_ANISOTROPIC;

			if( pSAMP[D3DSAMP_MINFILTER] == D3DTEXF_POINT )
				dwFlags |= BGFX_SAMPLER_MIN_POINT;
			else if( pSAMP[D3DSAMP_MINFILTER] == D3DTEXF_ANISOTROPIC )
				dwFlags |= BGFX_SAMPLER_MIN_ANISOTROPIC;

			if( pSAMP[D3DSAMP_MIPFILTER] != D3DTEXF_LINEAR )
				dwFlags |= BGFX_SAMPLER_MIP_POINT;

			return dwFlags;
		}

		// Without an alpha channel in the render target (the X8R8G8B8 back buffer), D3D9 reads
		// the destination alpha as 1.
		static uint64_t GetBlend( DWORD dwBlend, BOOL bDestAlpha)
		{
			if(!bDestAlpha)
				switch(dwBlend)
				{
				case D3DBLEND_DESTALPHA		: return BGFX_STATE_BLEND_ONE;
				case D3DBLEND_INVDESTALPHA	:
				case D3DBLEND_SRCALPHASAT	: return BGFX_STATE_BLEND_ZERO;
				}

			switch(dwBlend)
			{
			case D3DBLEND_ZERO				: return BGFX_STATE_BLEND_ZERO;
			case D3DBLEND_ONE				: return BGFX_STATE_BLEND_ONE;
			case D3DBLEND_SRCCOLOR			: return BGFX_STATE_BLEND_SRC_COLOR;
			case D3DBLEND_INVSRCCOLOR		: return BGFX_STATE_BLEND_INV_SRC_COLOR;
			case D3DBLEND_SRCALPHA			: return BGFX_STATE_BLEND_SRC_ALPHA;
			case D3DBLEND_INVSRCALPHA		: return BGFX_STATE_BLEND_INV_SRC_ALPHA;
			case D3DBLEND_DESTALPHA			: return BGFX_STATE_BLEND_DST_ALPHA;
			case D3DBLEND_INVDESTALPHA		: return BGFX_STATE_BLEND_INV_DST_ALPHA;
			case D3DBLEND_DESTCOLOR			: return BGFX_STATE_BLEND_DST_COLOR;
			case D3DBLEND_INVDESTCOLOR		: return BGFX_STATE_BLEND_INV_DST_COLOR;
			case D3DBLEND_SRCALPHASAT		: return BGFX_STATE_BLEND_SRC_ALPHA_SAT;
			case D3DBLEND_BLENDFACTOR		: return BGFX_STATE_BLEND_FACTOR;
			case D3DBLEND_INVBLENDFACTOR	: return BGFX_STATE_BLEND_INV_FACTOR;
			}

			return BGFX_STATE_BLEND_ONE;
		}

		static uint64_t GetBlendEquation( DWORD dwOp)
		{
			switch(dwOp)
			{
			case D3DBLENDOP_SUBTRACT	: return BGFX_STATE_BLEND_EQUATION_SUB;
			case D3DBLENDOP_REVSUBTRACT	: return BGFX_STATE_BLEND_EQUATION_REVSUB;
			case D3DBLENDOP_MIN			: return BGFX_STATE_BLEND_EQUATION_MIN;
			case D3DBLENDOP_MAX			: return BGFX_STATE_BLEND_EQUATION_MAX;
			}

			return BGFX_STATE_BLEND_EQUATION_ADD;
		}

		static uint64_t GetDepthTest( DWORD dwFunc)
		{
			switch(dwFunc)
			{
			case D3DCMP_NEVER			: return BGFX_STATE_DEPTH_TEST_NEVER;
			case D3DCMP_LESS			: return BGFX_STATE_DEPTH_TEST_LESS;
			case D3DCMP_EQUAL			: return BGFX_STATE_DEPTH_TEST_EQUAL;
			case D3DCMP_LESSEQUAL		: return BGFX_STATE_DEPTH_TEST_LEQUAL;
			case D3DCMP_GREATER			: return BGFX_STATE_DEPTH_TEST_GREATER;
			case D3DCMP_NOTEQUAL		: return BGFX_STATE_DEPTH_TEST_NOTEQUAL;
			case D3DCMP_GREATEREQUAL	: return BGFX_STATE_DEPTH_TEST_GEQUAL;
			}

			return BGFX_STATE_DEPTH_TEST_ALWAYS;
		}

		uint64_t GetState( D3DPRIMITIVETYPE nType, uint32_t *pFactor)
		{
			uint64_t dwState = 0;
			DWORD dwWrite = m_vRS[D3DRS_COLORWRITEENABLE];

			if( dwWrite & D3DCOLORWRITEENABLE_RED ) dwState |= BGFX_STATE_WRITE_R;
			if( dwWrite & D3DCOLORWRITEENABLE_GREEN ) dwState |= BGFX_STATE_WRITE_G;
			if( dwWrite & D3DCOLORWRITEENABLE_BLUE ) dwState |= BGFX_STATE_WRITE_B;
			if( (dwWrite & D3DCOLORWRITEENABLE_ALPHA) && HasAlpha(m_pRTARGET->m_vDESC.Format) ) dwState |= BGFX_STATE_WRITE_A;

			if( m_vRS[D3DRS_ZENABLE] && m_pDEPTH )
			{
				dwState |= GetDepthTest(m_vRS[D3DRS_ZFUNC]);

				if(m_vRS[D3DRS_ZWRITEENABLE])
					dwState |= BGFX_STATE_WRITE_Z;
			}

			// A flipped picture turns the winding around.
			if( m_vRS[D3DRS_CULLMODE] == (m_bFlipY ? D3DCULL_CCW : D3DCULL_CW) )
				dwState |= BGFX_STATE_CULL_CW;
			else if( m_vRS[D3DRS_CULLMODE] == (m_bFlipY ? D3DCULL_CW : D3DCULL_CCW) )
				dwState |= BGFX_STATE_CULL_CCW;

			*pFactor = 0;
			if(m_vRS[D3DRS_ALPHABLENDENABLE])
			{
				BOOL bDestAlpha = HasAlpha(m_pRTARGET->m_vDESC.Format);
				uint64_t dwSrc = GetBlend( m_vRS[D3DRS_SRCBLEND], bDestAlpha);
				uint64_t dwDest = GetBlend( m_vRS[D3DRS_DESTBLEND], bDestAlpha);
				uint64_t dwSrcA = dwSrc;
				uint64_t dwDestA = dwDest;
				uint64_t dwOp = GetBlendEquation(m_vRS[D3DRS_BLENDOP]);
				uint64_t dwOpA = dwOp;

				// BOTHSRCALPHA / BOTHINVSRCALPHA set the source and destination at once.
				if( m_vRS[D3DRS_SRCBLEND] == D3DBLEND_BOTHSRCALPHA || m_vRS[D3DRS_SRCBLEND] == D3DBLEND_BOTHINVSRCALPHA )
				{
					BOOL bInv = m_vRS[D3DRS_SRCBLEND] == D3DBLEND_BOTHINVSRCALPHA;

					dwSrc = dwSrcA = bInv ? BGFX_STATE_BLEND_INV_SRC_ALPHA : BGFX_STATE_BLEND_SRC_ALPHA;
					dwDest = dwDestA = bInv ? BGFX_STATE_BLEND_SRC_ALPHA : BGFX_STATE_BLEND_INV_SRC_ALPHA;
				}

				if(m_vRS[D3DRS_SEPARATEALPHABLENDENABLE])
				{
					dwSrcA = GetBlend( m_vRS[D3DRS_SRCBLENDALPHA], bDestAlpha);
					dwDestA = GetBlend( m_vRS[D3DRS_DESTBLENDALPHA], bDestAlpha);
					dwOpA = GetBlendEquation(m_vRS[D3DRS_BLENDOPALPHA]);
				}

				dwState |= BGFX_STATE_BLEND_FUNC_SEPARATE( dwSrc, dwDest, dwSrcA, dwDestA) | BGFX_STATE_BLEND_EQUATION_SEPARATE( dwOp, dwOpA);

				D3DCOLOR dwFactor = m_vRS[D3DRS_BLENDFACTOR];
				*pFactor = (dwFactor << 8) | (dwFactor >> 24);
			}

			switch(nType)
			{
			case D3DPT_TRIANGLESTRIP	: dwState |= BGFX_STATE_PT_TRISTRIP; break;
			case D3DPT_LINELIST			: dwState |= BGFX_STATE_PT_LINES; break;
			case D3DPT_LINESTRIP		: dwState |= BGFX_STATE_PT_LINESTRIP; break;
			case D3DPT_POINTLIST		: dwState |= BGFX_STATE_PT_POINTS; break;
			}

			return dwState;
		}

		// Diagnostics: while %TEMP%\tclient-bgfx-highlight.flag exists, the draw calls it
		// selects are drawn solid magenta, over everything.
		HRESULT Submit( D3DPRIMITIVETYPE nType, TLAYOUT& vLayout)
		{
			if( !m_bHighlight || m_vTSS[1][D3DTSS_COLOROP] != D3DTOP_MODULATE2X ||
				m_vRS[D3DRS_SRCBLEND] != D3DBLEND_SRCALPHA || m_vRS[D3DRS_DESTBLEND] != D3DBLEND_DESTALPHA )
				return SubmitDraw( nType, vLayout);

			std::vector<DWORD> vRS( m_vRS, m_vRS + RENDER_STATES);
			std::vector<DWORD> vTSS( &m_vTSS[0][0], &m_vTSS[0][0] + TEXTURE_STAGES * STAGE_STATES);

			// 1 magenta, 2 own colors opaque, 3 no depth test, 4 no alpha test, 5 no fog,
			// 6 no stage 1, 7 no lighting.
			switch(m_nHighlight)
			{
			case 8	:
			case 9	:
				// 8 the texture alone, 9 the lit diffuse color alone, opaque.
				m_vTSS[0][D3DTSS_COLOROP] = D3DTOP_SELECTARG1;
				m_vTSS[0][D3DTSS_COLORARG1] = m_nHighlight == 8 ? D3DTA_TEXTURE : D3DTA_DIFFUSE;
				m_vTSS[0][D3DTSS_ALPHAOP] = D3DTOP_SELECTARG1;
				m_vTSS[0][D3DTSS_ALPHAARG1] = D3DTA_TFACTOR;
				m_vTSS[1][D3DTSS_COLOROP] = D3DTOP_DISABLE;
				m_vRS[D3DRS_ALPHABLENDENABLE] = FALSE;
				m_vRS[D3DRS_ALPHATESTENABLE] = FALSE;
				m_vRS[D3DRS_ZENABLE] = FALSE;
				m_vRS[D3DRS_FOGENABLE] = FALSE;
				m_vRS[D3DRS_CULLMODE] = D3DCULL_NONE;
				break;

			case 1	:
				m_vRS[D3DRS_TEXTUREFACTOR] = 0xFFFF00FF;
				m_vTSS[0][D3DTSS_COLOROP] = D3DTOP_SELECTARG1;
				m_vTSS[0][D3DTSS_COLORARG1] = D3DTA_TFACTOR;
				m_vTSS[0][D3DTSS_ALPHAOP] = D3DTOP_SELECTARG1;
				m_vTSS[0][D3DTSS_ALPHAARG1] = D3DTA_TFACTOR;
				m_vTSS[1][D3DTSS_COLOROP] = D3DTOP_DISABLE;

			case 2	:
				m_vRS[D3DRS_ALPHABLENDENABLE] = FALSE;
				m_vRS[D3DRS_ALPHATESTENABLE] = FALSE;
				m_vRS[D3DRS_ZENABLE] = FALSE;
				m_vRS[D3DRS_FOGENABLE] = FALSE;
				m_vRS[D3DRS_CULLMODE] = D3DCULL_NONE;
				break;

			case 3	: m_vRS[D3DRS_ZENABLE] = FALSE; break;
			case 4	: m_vRS[D3DRS_ALPHATESTENABLE] = FALSE; break;
			case 5	: m_vRS[D3DRS_FOGENABLE] = FALSE; break;
			case 6	: m_vTSS[1][D3DTSS_COLOROP] = D3DTOP_DISABLE; break;
			case 7	: m_vRS[D3DRS_LIGHTING] = FALSE; break;
			}

			HRESULT hr = SubmitDraw( nType, vLayout);

			memcpy( m_vRS, &vRS[0], vRS.size() * sizeof(DWORD));
			memcpy( &m_vTSS[0][0], &vTSS[0], vTSS.size() * sizeof(DWORD));

			return hr;
		}

		HRESULT SubmitDraw( D3DPRIMITIVETYPE nType, TLAYOUT& vLayout)
		{
			bgfx::ProgramHandle hProgram = GetProgram(vLayout.m_dwShader);

			if(!bgfx::isValid(hProgram))
			{
				return Skip("no program");
			}

			for( int i=0; i<SHADER_STAGES; i++)
			{
				m_vSTAGEBOUND[i] = FALSE;

				if(m_pTEXTURE[i])
				{
					bgfx::TextureHandle hTexture = ((CTTextureBGFX *) m_pTEXTURE[i])->GetHandle();

					if(bgfx::isValid(hTexture))
					{
						bgfx::setTexture( uint8_t(i), m_hSAMPLER[i], hTexture, GetSamplerFlags(i));
						m_vSTAGEBOUND[i] = TRUE;
					}
				}
			}

			SetVertexUniforms(vLayout.m_dwShader);
			SetPixelUniforms();

			uint32_t dwFactor;
			uint64_t dwState = GetState( nType, &dwFactor);

			bgfx::setState( dwState, dwFactor);
			bgfx::submit( bgfx::ViewId(m_nView), hProgram);

			if(m_pDUMP)
				DumpDraw( nType, vLayout);
			m_nDraws++;

			return D3D_OK;
		}

		// Diagnostics: when %TEMP%\tclient-bgfx-dump.flag exists, the next frame's draw calls
		// are written to %TEMP%\tclient-bgfx-dump.txt (states, textures, screen rectangle).
		void UpdateDump()
		{
			char vPATH[MAX_PATH];
			char vFILE[MAX_PATH];

			if(m_pDUMP)
			{
				fclose(m_pDUMP);
				m_pDUMP = NULL;
			}

			// The flag holds the number of frames to dump, one per second (default 1).
			// A series goes to numbered files.
			static int s_nLeft = 0;
			static int s_nIndex = 0;
			static int s_nCount = 0;
			static DWORD s_dwNext = 0;

			GetTempPathA( MAX_PATH, vPATH);
			sprintf_s( vFILE, "%stclient-bgfx-dump.flag", vPATH);

			if( TFileExists(vFILE) )
			{
				FILE *pFLAG = NULL;

				s_nCount = 1;
				if( (pFLAG = TFileOpen( vFILE, "r")) != NULL )
				{
					if( fscanf_s( pFLAG, "%d", &s_nCount) != 1 || s_nCount < 1 )
						s_nCount = 1;
					fclose(pFLAG);
				}

				TFileDelete(vFILE);
				s_nLeft = s_nCount;
				s_nIndex = 0;
				s_dwNext = 0;
			}

			if( s_nLeft <= 0 || GetTickCount() < s_dwNext )
				return;

			if( s_nCount > 1 )
				sprintf_s( vFILE, "%stclient-bgfx-dump-%02d.txt", vPATH, s_nIndex);
			else
				sprintf_s( vFILE, "%stclient-bgfx-dump.txt", vPATH);

			s_nLeft--;
			s_nIndex++;
			s_dwNext = GetTickCount() + 1000;

			if( !(m_pDUMP = TFileOpen( vFILE, "w")) )
				m_pDUMP = NULL;

			if(m_pDUMP)
				fprintf( m_pDUMP, "renderer %s backbuffer %ux%u\n", bgfx::getRendererName(m_nRenderer), m_vPRESENT.BackBufferWidth, m_vPRESENT.BackBufferHeight);
		}

		void SetDumpVertices( const BYTE *pData, UINT nCount, UINT nStride, UINT nPrimitives)
		{
			m_pDumpData = pData;
			m_nDumpCount = nCount;
			m_nDumpStride = nStride;
			m_nDumpPrims = nPrimitives;
		}

		void DumpDraw( D3DPRIMITIVETYPE nType, TLAYOUT& vLayout)
		{
			FLOAT fMinX = 1e9f, fMinY = 1e9f, fMaxX = -1e9f, fMaxY = -1e9f;
			D3DXMATRIX vWVP = D3DXMATRIX(m_vWORLD[0]) * D3DXMATRIX(m_vVIEW) * D3DXMATRIX(m_vPROJ);

			for( UINT i=0; i<m_nDumpCount && i<256 && m_pDumpData; i++)
			{
				const FLOAT *pPos = (const FLOAT *) (m_pDumpData + i * m_nDumpStride);
				FLOAT x, y;

				if( (vLayout.m_dwShader & 3) == 1 )
				{
					x = pPos[0];
					y = pPos[1];
				}
				else
				{
					D3DXVECTOR4 vOut;
					D3DXVec3Transform( &vOut, (const D3DXVECTOR3 *) pPos, &vWVP);

					if( vOut.w <= 0.0001f )
						continue;

					x = m_vVIEWPORT.X + (vOut.x / vOut.w * 0.5f + 0.5f) * m_vVIEWPORT.Width;
					y = m_vVIEWPORT.Y + (0.5f - vOut.y / vOut.w * 0.5f) * m_vVIEWPORT.Height;
				}

				fMinX = x < fMinX ? x : fMinX;
				fMinY = y < fMinY ? y : fMinY;
				fMaxX = x > fMaxX ? x : fMaxX;
				fMaxY = y > fMaxY ? y : fMaxY;
			}

			fprintf( m_pDUMP, "#%u v%u fvf=%x st=%u key=%u prim=%d n=%u | L=%u CV=%u blend=%u %u>%u op%u at=%u f%u r%u z=%u%u f%u cull=%u fog=%u tf=%08x | md=%.2f,%.2f,%.2f,%.2f ma=%.2f,%.2f,%.2f me=%.2f,%.2f,%.2f amb=%08x src=%u,%u,%u,%u | vb=%u,%u",
				m_nDraws, m_nView, m_pVDECL ? 0xDEC00000 | m_pVDECL->m_dwStreams : m_dwFVF, m_nDumpStride, vLayout.m_dwShader, nType, m_nDumpPrims,
				m_vRS[D3DRS_LIGHTING], m_vRS[D3DRS_COLORVERTEX],
				m_vRS[D3DRS_ALPHABLENDENABLE], m_vRS[D3DRS_SRCBLEND], m_vRS[D3DRS_DESTBLEND], m_vRS[D3DRS_BLENDOP],
				m_vRS[D3DRS_ALPHATESTENABLE], m_vRS[D3DRS_ALPHAFUNC], m_vRS[D3DRS_ALPHAREF],
				m_vRS[D3DRS_ZENABLE], m_vRS[D3DRS_ZWRITEENABLE], m_vRS[D3DRS_ZFUNC], m_vRS[D3DRS_CULLMODE], m_vRS[D3DRS_FOGENABLE], m_vRS[D3DRS_TEXTUREFACTOR],
				m_vMATERIAL.Diffuse.r, m_vMATERIAL.Diffuse.g, m_vMATERIAL.Diffuse.b, m_vMATERIAL.Diffuse.a,
				m_vMATERIAL.Ambient.r, m_vMATERIAL.Ambient.g, m_vMATERIAL.Ambient.b,
				m_vMATERIAL.Emissive.r, m_vMATERIAL.Emissive.g, m_vMATERIAL.Emissive.b, m_vRS[D3DRS_AMBIENT],
				m_vRS[D3DRS_DIFFUSEMATERIALSOURCE], m_vRS[D3DRS_AMBIENTMATERIALSOURCE], m_vRS[D3DRS_SPECULARMATERIALSOURCE], m_vRS[D3DRS_EMISSIVEMATERIALSOURCE],
				m_vRS[D3DRS_VERTEXBLEND], m_vRS[D3DRS_INDEXEDVERTEXBLENDENABLE]);

			int nLights = 0;
			for( std::map< DWORD, BOOL>::iterator it = m_mapLIGHTON.begin(); it != m_mapLIGHTON.end(); it++)
				if((*it).second)
					nLights++;
			fprintf( m_pDUMP, " lights=%d", nLights);

			for( int i=0; i<2; i++)
			{
				CTTextureBGFX *pTEX = (CTTextureBGFX *) m_pTEXTURE[i];

				fprintf( m_pDUMP, " | s%d c=%u(%x,%x,%x) a=%u(%x,%x) tex=%p", i,
					m_vTSS[i][D3DTSS_COLOROP], m_vTSS[i][D3DTSS_COLORARG1], m_vTSS[i][D3DTSS_COLORARG2], m_vTSS[i][D3DTSS_COLORARG0],
					m_vTSS[i][D3DTSS_ALPHAOP], m_vTSS[i][D3DTSS_ALPHAARG1], m_vTSS[i][D3DTSS_ALPHAARG2], (void *) pTEX);

				if(pTEX)
					fprintf( m_pDUMP, " fmt=%u %ux%u lv%u up%u", pTEX->m_nFormat, pTEX->m_nWidth, pTEX->m_nHeight, pTEX->m_nLevels, pTEX->m_nUpload);

				if( pTEX && !pTEX->IsTarget() )
				{
					// The average B G R A of level 0 and whether it was written: an empty texture
					// draws nothing.
					std::vector<BYTE>& vLEVEL = pTEX->GetLevel(0);
					std::vector<BYTE> vBGRA( pTEX->m_nWidth * pTEX->m_nHeight * 4);
					UINT64 vSUM[4] = { 0, 0, 0, 0};

					if(DecodeToBGRA( pTEX->m_nFormat, &vLEVEL[0], GetPitch( pTEX->m_nFormat, pTEX->m_nWidth), pTEX->m_nWidth, pTEX->m_nHeight, &vBGRA[0]))
						for( size_t k=0; k<vBGRA.size(); k++)
							vSUM[k % 4] += vBGRA[k];

					UINT nPixels = TMax( 1U, pTEX->m_nWidth * pTEX->m_nHeight);
					fprintf( m_pDUMP, " avg=%02x%02x%02x%02x w%u", UINT(vSUM[3] / nPixels), UINT(vSUM[2] / nPixels), UINT(vSUM[1] / nPixels), UINT(vSUM[0] / nPixels), pTEX->m_vWRITTEN[0] ? 1 : 0);
				}

				fprintf( m_pDUMP, " mag%u min%u mip%u adr%u,%u ttff%u tci%u", m_vSAMP[i][D3DSAMP_MAGFILTER], m_vSAMP[i][D3DSAMP_MINFILTER], m_vSAMP[i][D3DSAMP_MIPFILTER],
					m_vSAMP[i][D3DSAMP_ADDRESSU], m_vSAMP[i][D3DSAMP_ADDRESSV], m_vTSS[i][D3DTSS_TEXTURETRANSFORMFLAGS], m_vTSS[i][D3DTSS_TEXCOORDINDEX]);
			}

			fprintf( m_pDUMP, " | rect=%.0f,%.0f-%.0f,%.0f\n", fMinX, fMinY, fMaxX, fMaxY);

			// Lit draws with a normal: the lights, the world scale and vertex 0 lit the D3D9 way.
			if( m_vRS[D3DRS_LIGHTING] && (vLayout.m_dwShader & (1 << 2)) && m_pDumpData && m_nDumpCount && (vLayout.m_dwShader & 3) == 0 )
			{
				const D3DMATRIX& w = m_vWORLD[0];
				const FLOAT *pPos = (const FLOAT *) m_pDumpData;
				const FLOAT *pNor = pPos + 3;
				D3DXVECTOR3 vP( pPos[0], pPos[1], pPos[2]);
				D3DXVECTOR3 vN( pNor[0], pNor[1], pNor[2]);
				D3DXMATRIX vW(w);
				D3DXMATRIX vINV;
				D3DXVECTOR3 vWP;
				D3DXVECTOR3 vWN;

				D3DXVec3TransformCoord( &vWP, &vP, &vW);
				D3DXMatrixInverse( &vINV, NULL, &vW);
				D3DXMatrixTranspose( &vINV, &vINV);
				vWN = D3DXVECTOR3(
					vN.x * vINV._11 + vN.y * vINV._21 + vN.z * vINV._31,
					vN.x * vINV._12 + vN.y * vINV._22 + vN.z * vINV._32,
					vN.x * vINV._13 + vN.y * vINV._23 + vN.z * vINV._33);

				fprintf( m_pDUMP, "    light: scale %.3f %.3f %.3f n0=(%.2f %.2f %.2f) wn=(%.2f %.2f %.2f) normalize=%u",
					sqrtf( w._11 * w._11 + w._12 * w._12 + w._13 * w._13), sqrtf( w._21 * w._21 + w._22 * w._22 + w._23 * w._23), sqrtf( w._31 * w._31 + w._32 * w._32 + w._33 * w._33),
					vN.x, vN.y, vN.z, vWN.x, vWN.y, vWN.z, m_vRS[D3DRS_NORMALIZENORMALS]);

				D3DXVECTOR3 vColor( m_vMATERIAL.Emissive.r, m_vMATERIAL.Emissive.g, m_vMATERIAL.Emissive.b);
				D3DXCOLOR vAMB(m_vRS[D3DRS_AMBIENT]);
				vColor += D3DXVECTOR3( vAMB.r * m_vMATERIAL.Ambient.r, vAMB.g * m_vMATERIAL.Ambient.g, vAMB.b * m_vMATERIAL.Ambient.b);

				for( std::map< DWORD, BOOL>::iterator it = m_mapLIGHTON.begin(); it != m_mapLIGHTON.end(); it++)
				{
					if(!(*it).second)
						continue;

					D3DLIGHT9& l = m_mapLIGHT[(*it).first];
					D3DXVECTOR3 vL;
					FLOAT fAtt = 1.0f;

					if( l.Type == D3DLIGHT_DIRECTIONAL )
						vL = -D3DXVECTOR3( l.Direction.x, l.Direction.y, l.Direction.z);
					else
					{
						vL = D3DXVECTOR3( l.Position.x, l.Position.y, l.Position.z) - vWP;
						FLOAT d = D3DXVec3Length(&vL);
						fAtt = d > l.Range ? 0.0f : 1.0f / TMaxF( l.Attenuation0 + l.Attenuation1 * d + l.Attenuation2 * d * d, 0.0001f);
					}
					D3DXVec3Normalize( &vL, &vL);

					FLOAT fDot = TMaxF( D3DXVec3Dot( &vWN, &vL), 0.0f);
					vColor += fAtt * D3DXVECTOR3(
						l.Ambient.r * m_vMATERIAL.Ambient.r + l.Diffuse.r * m_vMATERIAL.Diffuse.r * fDot,
						l.Ambient.g * m_vMATERIAL.Ambient.g + l.Diffuse.g * m_vMATERIAL.Diffuse.g * fDot,
						l.Ambient.b * m_vMATERIAL.Ambient.b + l.Diffuse.b * m_vMATERIAL.Diffuse.b * fDot);

					fprintf( m_pDUMP, " | L%u t%u dif=%.2f,%.2f,%.2f amb=%.2f,%.2f,%.2f dir=%.2f,%.2f,%.2f range=%.0f att=%.2f,%.3f,%.4f dot=%.2f",
						(*it).first, l.Type, l.Diffuse.r, l.Diffuse.g, l.Diffuse.b, l.Ambient.r, l.Ambient.g, l.Ambient.b,
						l.Direction.x, l.Direction.y, l.Direction.z, l.Range, l.Attenuation0, l.Attenuation1, l.Attenuation2, fDot);
				}

				fprintf( m_pDUMP, " | d3d9 color0 %.2f %.2f %.2f a %.2f\n", vColor.x, vColor.y, vColor.z, m_vMATERIAL.Diffuse.a);
			}
		}

		FILE *m_pDUMP;
		const BYTE *m_pDumpData;
		UINT m_nDumpCount;
		UINT m_nDumpStride;
		UINT m_nDumpPrims;

		D3DPRESENT_PARAMETERS m_vPRESENT;
		bgfx::SwapChain m_vSWAPCHAIN;
		D3DCAPS9 m_vCAPS;
		BOOL m_bREADY;

		bgfx::RendererType::Enum m_nRenderer;
		BOOL m_bHomogeneousDepth;

		CTSurfaceBGFX *m_pBACKBUF;
		CTSurfaceBGFX *m_pZBUF;
		CTSurfaceBGFX *m_pRTARGET;
		CTSurfaceBGFX *m_pDEPTH;
		LPTTEXTURE m_pTEXTURE[TEXTURE_STAGES];
		BOOL m_vSTAGEBOUND[SHADER_STAGES];
		TSTREAM m_vSTREAM[STREAMS];
		CTIndexBufferBGFX *m_pINDEX;

		DWORD m_vRS[RENDER_STATES];
		DWORD m_vTSS[TEXTURE_STAGES][STAGE_STATES];
		DWORD m_vSAMP[TEXTURE_STAGES][SAMPLER_STATES];
		CTVertexDeclBGFX *m_pVDECL;
		DWORD m_dwFVF;

		D3DVIEWPORT9 m_vVIEWPORT;
		D3DMATERIAL9 m_vMATERIAL;
		D3DMATRIX m_vWORLD[MAX_WORLD];
		D3DMATRIX m_vVIEW;
		D3DMATRIX m_vPROJ;
		D3DMATRIX m_vTEXMAT[TEXTURE_STAGES];
		UINT m_nWorldCount;
		std::map< DWORD, D3DLIGHT9> m_mapLIGHT;
		std::map< DWORD, BOOL> m_mapLIGHTON;

		std::map< DWORD, TLAYOUT> m_mapLAYOUT;
		std::map< DWORD, bgfx::ProgramHandle> m_mapPROGRAM;
		std::vector<bgfx::ShaderHandle> m_vSHADER;
		std::vector<bgfx::UniformHandle> m_vUNIFORM;
		bgfx::ShaderHandle m_hFS;
		bgfx::UniformHandle m_hSAMPLER[SHADER_STAGES];

		TUNIFORM m_vWORLDU;
		TUNIFORM m_vVIEWPROJU;
		TUNIFORM m_vVIEWU;
		TUNIFORM m_vVSU;
		TUNIFORM m_vMATERIALU;
		TUNIFORM m_vLIGHTSU;
		TUNIFORM m_vTEXXFORMU;
		TUNIFORM m_vSTAGEU;
		TUNIFORM m_vPSU;

		UINT m_nView;
		BOOL m_bViewOpen;
		BOOL m_bViewDirty;
		BOOL m_bFlipY;
		BOOL m_bHighlight;
		int m_nHighlight;
		UINT m_nDraws;
		UINT m_nSkipped;
	};
}

LPTRENDERDEVICE CreateRenderDeviceBGFX( HWND hWnd, const D3DPRESENT_PARAMETERS *pPresent, const D3DCAPS9 *pCaps)
{
	CTRenderDeviceBGFX *pDevice = new CTRenderDeviceBGFX( hWnd, *pPresent, *pCaps);

	if(!pDevice->IsReady())
	{
		pDevice->Release();
		return NULL;
	}

	return pDevice;
}
