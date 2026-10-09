// The bgfx backend of the render layer (Settings/Renderer=BGFX in config.ini).
//
// Step 3c-1: bgfx runs on the game window and clears and presents it. The
// resources live in memory (images already decoded with bimg) and the D3D9
// state is recorded, but nothing is drawn yet.

#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <TD3DXMath.h>
#include <d3dx9.h>
#include <TRenderDevice.h>

#include <bgfx/bgfx.h>
#include <bimg/decode.h>
#include <bx/allocator.h>

#include <map>
#include <mutex>
#include <vector>

namespace
{
	bx::DefaultAllocator g_vALLOCATOR;

	inline UINT TMax( UINT a, UINT b) { return a > b ? a : b; }
	inline size_t TMin( size_t a, size_t b) { return a < b ? a : b; }

	// Size of one row of blocks (pixels for plain formats) and number of rows.
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


	class CTSurfaceBGFX;

	class CTTextureBGFX : public CTTexture
	{
	public:
		CTTextureBGFX( UINT nWidth, UINT nHeight, UINT nLevels, DWORD dwUsage, D3DFORMAT nFormat, D3DPOOL nPool)
			: m_nRef(1), m_nWidth(nWidth), m_nHeight(nHeight), m_dwUsage(dwUsage), m_nFormat(nFormat), m_nPool(nPool)
		{
			UINT nMax = GetLevelCount( nWidth, nHeight);

			m_nLevels = nLevels && nLevels < nMax ? nLevels : nMax;
			if( dwUsage & D3DUSAGE_AUTOGENMIPMAP )
				m_nLevels = nLevels ? nLevels : nMax;

			m_vLEVEL.resize(m_nLevels);
			m_vSURFACE.resize( m_nLevels, (CTSurfaceBGFX *) NULL);
		}

		ULONG AddRef() override { return InterlockedIncrement(&m_nRef); }
		ULONG Release() override;

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

			return D3D_OK;
		}

		HRESULT UnlockRect( UINT Level) override
		{
			return Level < m_nLevels ? D3D_OK : D3DERR_INVALIDCALL;
		}

		void GenerateMipSubLevels() override {}

	public:
		LONG m_nRef;
		UINT m_nWidth;
		UINT m_nHeight;
		UINT m_nLevels;
		DWORD m_dwUsage;
		D3DFORMAT m_nFormat;
		D3DPOOL m_nPool;

		std::vector< std::vector<BYTE> > m_vLEVEL;
		std::vector<CTSurfaceBGFX *> m_vSURFACE;
		std::mutex m_mutex;
	};

	// A level of a texture, or a standalone surface (back buffer, depth buffer,
	// offscreen plain surface) with its own memory.
	class CTSurfaceBGFX : public CTSurface
	{
	public:
		CTSurfaceBGFX( CTTextureBGFX *pTexture, UINT nLevel)
			: m_nRef(1), m_pTexture(pTexture), m_nLevel(nLevel)
		{
			pTexture->AddRef();
			pTexture->GetLevelDesc( nLevel, &m_vDESC);
		}

		CTSurfaceBGFX( UINT nWidth, UINT nHeight, D3DFORMAT nFormat, DWORD dwUsage, D3DPOOL nPool)
			: m_nRef(1), m_pTexture(NULL), m_nLevel(0)
		{
			ZeroMemory( &m_vDESC, sizeof(D3DSURFACE_DESC));
			m_vDESC.Format = nFormat;
			m_vDESC.Type = D3DRTYPE_SURFACE;
			m_vDESC.Usage = dwUsage;
			m_vDESC.Pool = nPool;
			m_vDESC.Width = nWidth;
			m_vDESC.Height = nHeight;
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

		HRESULT LockRect( D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) override
		{
			if(m_pTexture)
				return m_pTexture->LockRect( m_nLevel, pLockedRect, pRect, Flags);

			if(!pLockedRect)
				return D3DERR_INVALIDCALL;

			if(m_vDATA.empty())
				m_vDATA.resize( GetPitch( m_vDESC.Format, m_vDESC.Width) * GetRows( m_vDESC.Format, m_vDESC.Height));

			UINT nPitch = GetPitch( m_vDESC.Format, m_vDESC.Width);
			UINT nOffset = 0;

			if(pRect)
				nOffset = (IsBlockFormat(m_vDESC.Format) ? pRect->top / 4 : pRect->top) * nPitch + GetPitch( m_vDESC.Format, pRect->left);

			pLockedRect->Pitch = nPitch;
			pLockedRect->pBits = &m_vDATA[nOffset];

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

		D3DSURFACE_DESC m_vDESC;
		std::vector<BYTE> m_vDATA;
	};

	ULONG CTTextureBGFX::Release()
	{
		ULONG nRef = InterlockedDecrement(&m_nRef);

		if(!nRef)
			delete this;

		return nRef;
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


	template< class BASE, class DESC > class TBufferBGFX : public BASE
	{
	public:
		TBufferBGFX( UINT nLength, const DESC& vDESC) : m_nRef(1), m_vDATA(nLength), m_vDESC(vDESC) {}

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

			return D3D_OK;
		}

		HRESULT Unlock() override { return D3D_OK; }

	public:
		LONG m_nRef;
		std::vector<BYTE> m_vDATA;
		DESC m_vDESC;
	};

	typedef TBufferBGFX< CTVertexBuffer, D3DVERTEXBUFFER_DESC> CTVertexBufferBGFX;
	typedef TBufferBGFX< CTIndexBuffer, D3DINDEXBUFFER_DESC> CTIndexBufferBGFX;


	D3DFORMAT GetD3DFormat( bimg::TextureFormat::Enum nFormat)
	{
		switch(nFormat)
		{
		case bimg::TextureFormat::BC1	: return D3DFMT_DXT1;
		case bimg::TextureFormat::BC2	: return D3DFMT_DXT3;
		case bimg::TextureFormat::BC3	: return D3DFMT_DXT5;
		case bimg::TextureFormat::BGRA8	: return D3DFMT_A8R8G8B8;
		}

		return D3DFMT_UNKNOWN;
	}

	D3DXIMAGE_FILEFORMAT GetFileFormat( LPCVOID pData, UINT nSize)
	{
		const BYTE *p = (const BYTE *) pData;

		if( nSize >= 4 && !memcmp( p, "DDS ", 4) )
			return D3DXIFF_DDS;

		if( nSize >= 8 && !memcmp( p, "\x89PNG", 4) )
			return D3DXIFF_PNG;

		if( nSize >= 2 && p[0] == 0xFF && p[1] == 0xD8 )
			return D3DXIFF_JPG;

		if( nSize >= 2 && p[0] == 'B' && p[1] == 'M' )
			return D3DXIFF_BMP;

		return D3DXIFF_TGA;
	}


	class CTRenderDeviceBGFX : public CTRenderDevice
	{
	public:
		CTRenderDeviceBGFX( HWND hWnd, const D3DPRESENT_PARAMETERS& vPresent, const D3DCAPS9& vCaps)
			: m_vPRESENT(vPresent), m_vCAPS(vCaps), m_bREADY(FALSE), m_dwFrame(0)
		{
			m_pBACKBUF = new CTSurfaceBGFX( vPresent.BackBufferWidth, vPresent.BackBufferHeight, D3DFMT_X8R8G8B8, D3DUSAGE_RENDERTARGET, D3DPOOL_DEFAULT);
			m_pZBUF = new CTSurfaceBGFX( vPresent.BackBufferWidth, vPresent.BackBufferHeight, vPresent.AutoDepthStencilFormat, D3DUSAGE_DEPTHSTENCIL, D3DPOOL_DEFAULT);
			m_pRTARGET = m_pBACKBUF;
			m_pRTARGET->AddRef();
			m_pDEPTH = m_pZBUF;
			m_pDEPTH->AddRef();

			ZeroMemory( m_vRS, sizeof(m_vRS));
			ZeroMemory( m_vTSS, sizeof(m_vTSS));
			ZeroMemory( m_vSAMP, sizeof(m_vSAMP));
			ZeroMemory( &m_vMATERIAL, sizeof(m_vMATERIAL));
			ZeroMemory( m_pTEXTURE, sizeof(m_pTEXTURE));
			ResetViewport();

			bgfx::Init vINIT;
			vINIT.type = bgfx::RendererType::Count;
			vINIT.swapChain.nwh = hWnd;
			vINIT.swapChain.width = vPresent.BackBufferWidth;
			vINIT.swapChain.height = vPresent.BackBufferHeight;
			vINIT.reset = GetResetFlags(vPresent);
			m_vSWAPCHAIN = vINIT.swapChain;

			m_bREADY = bgfx::init(vINIT);
			if(m_bREADY)
				bgfx::setDebug(BGFX_DEBUG_TEXT);
		}

		~CTRenderDeviceBGFX()
		{
			for( int i=0; i<TEXTURE_STAGES; i++)
				if(m_pTEXTURE[i])
					m_pTEXTURE[i]->Release();

			m_pRTARGET->Release();
			m_pDEPTH->Release();
			m_pBACKBUF->Release();
			m_pZBUF->Release();

			if(m_bREADY)
				bgfx::shutdown();
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
			ResetViewport();

			return D3D_OK;
		}

		HRESULT Present( const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion) override
		{
			bgfx::dbgTextClear();
			bgfx::dbgTextPrintf( 1, 1, 0x0f, "bgfx (%s) - step 3c-1: clear and present only", bgfx::getRendererName(bgfx::getRendererType()));
			bgfx::touch(0);
			bgfx::frame();
			m_dwFrame++;

			return D3D_OK;
		}

		void SetGammaRamp( UINT iSwapChain, DWORD Flags, const D3DGAMMARAMP *pRamp) override {}

		HRESULT CreateTexture( UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, LPTTEXTURE *ppTexture, HANDLE *pSharedHandle) override
		{
			if( !ppTexture || !Width || !Height )
				return D3DERR_INVALIDCALL;
			*ppTexture = new CTTextureBGFX( Width, Height, Levels, Usage, Format, Pool);

			return D3D_OK;
		}

		HRESULT CreateVertexBuffer( UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, LPTVERTEXBUFFER *ppVertexBuffer, HANDLE *pSharedHandle) override
		{
			if(!ppVertexBuffer)
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
			if(!ppIndexBuffer)
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

		HRESULT GetRenderTargetData( LPTSURFACE pRenderTarget, LPTSURFACE pDestSurface) override { return D3D_OK; }

		HRESULT SetRenderTarget( DWORD RenderTargetIndex, LPTSURFACE pRenderTarget) override
		{
			if( RenderTargetIndex || !pRenderTarget )
				return D3DERR_INVALIDCALL;

			pRenderTarget->AddRef();
			m_pRTARGET->Release();
			m_pRTARGET = (CTSurfaceBGFX *) pRenderTarget;

			// Like D3D9, a new render target resets the viewport to its size.
			ResetViewport();

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
			if( m_pRTARGET != m_pBACKBUF )
				return D3D_OK;

			uint16_t nFlags = 0;
			if( Flags & D3DCLEAR_TARGET )
				nFlags |= BGFX_CLEAR_COLOR;

			if( Flags & D3DCLEAR_ZBUFFER )
				nFlags |= BGFX_CLEAR_DEPTH;

			if( Flags & D3DCLEAR_STENCIL )
				nFlags |= BGFX_CLEAR_STENCIL;

			// D3DCOLOR is ARGB, bgfx wants RGBA.
			uint32_t dwRGBA = (Color << 8) | (Color >> 24);

			bgfx::setViewRect( 0, 0, 0, uint16_t(m_vPRESENT.BackBufferWidth), uint16_t(m_vPRESENT.BackBufferHeight));
			bgfx::setViewClear( 0, nFlags, dwRGBA, Z, uint8_t(Stencil));
			bgfx::touch(0);

			return D3D_OK;
		}

		HRESULT SetTransform( D3DTRANSFORMSTATETYPE State, const D3DMATRIX *pMatrix) override
		{
			if(!pMatrix)
				return D3DERR_INVALIDCALL;
			m_mapTRANSFORM[State] = *pMatrix;

			return D3D_OK;
		}

		HRESULT SetViewport( const D3DVIEWPORT9 *pViewport) override
		{
			if(!pViewport)
				return D3DERR_INVALIDCALL;
			m_vVIEWPORT = *pViewport;

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

		HRESULT LightEnable( DWORD Index, BOOL Enable) override { m_mapLIGHTON[Index] = Enable; return D3D_OK; }

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

		HRESULT DrawPrimitive( D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) override { return D3D_OK; }
		HRESULT DrawIndexedPrimitive( D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) override { return D3D_OK; }
		HRESULT DrawPrimitiveUP( D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void *pVertexStreamZeroData, UINT VertexStreamZeroStride) override { return D3D_OK; }

		// The engine's D3D9 shaders are not used with bgfx: CD3DDevice turns them off.
		HRESULT CreateVertexDeclaration( const D3DVERTEXELEMENT9 *pVertexElements, IDirect3DVertexDeclaration9 **ppDecl) override { if(ppDecl) *ppDecl = NULL; return D3DERR_NOTAVAILABLE; }
		HRESULT SetVertexDeclaration( IDirect3DVertexDeclaration9 *pDecl) override { return D3D_OK; }
		HRESULT SetFVF( DWORD FVF) override { m_dwFVF = FVF; return D3D_OK; }
		HRESULT CreateVertexShader( const DWORD *pFunction, IDirect3DVertexShader9 **ppShader) override { if(ppShader) *ppShader = NULL; return D3DERR_NOTAVAILABLE; }
		HRESULT SetVertexShader( IDirect3DVertexShader9 *pShader) override { return D3D_OK; }
		HRESULT SetVertexShaderConstantF( UINT StartRegister, const float *pConstantData, UINT Vector4fCount) override { return D3D_OK; }
		HRESULT SetVertexShaderConstantI( UINT StartRegister, const int *pConstantData, UINT Vector4iCount) override { return D3D_OK; }
		HRESULT SetVertexShaderConstantB( UINT StartRegister, const BOOL *pConstantData, UINT BoolCount) override { return D3D_OK; }
		HRESULT SetStreamSource( UINT StreamNumber, LPTVERTEXBUFFER pStreamData, UINT OffsetInBytes, UINT Stride) override { return D3D_OK; }
		HRESULT SetIndices( LPTINDEXBUFFER pIndexData) override { return D3D_OK; }
		HRESULT CreatePixelShader( const DWORD *pFunction, IDirect3DPixelShader9 **ppShader) override { if(ppShader) *ppShader = NULL; return D3DERR_NOTAVAILABLE; }
		HRESULT SetPixelShader( IDirect3DPixelShader9 *pShader) override { return D3D_OK; }
		HRESULT SetPixelShaderConstantF( UINT StartRegister, const float *pConstantData, UINT Vector4fCount) override { return D3D_OK; }

		HRESULT CreateTextureD3DX( UINT Width, UINT Height, UINT MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, LPTTEXTURE *ppTexture) override
		{
			return CreateTexture( Width, Height, MipLevels == D3DX_DEFAULT ? 0 : MipLevels, Usage, Format, Pool, ppTexture, NULL);
		}

		HRESULT CreateTextureFromFile( LPCSTR pSrcFile, LPTTEXTURE *ppTexture) override
		{
			FILE *pFILE = NULL;

			if( fopen_s( &pFILE, pSrcFile, "rb") || !pFILE )
				return D3DERR_NOTFOUND;

			std::vector<BYTE> vDATA;
			BYTE vBUF[4096];
			size_t nRead;

			while( (nRead = fread( vBUF, 1, sizeof(vBUF), pFILE)) > 0 )
				vDATA.insert( vDATA.end(), vBUF, vBUF + nRead);
			fclose(pFILE);

			if(vDATA.empty())
				return D3DXERR_INVALIDDATA;

			return CreateTextureFromFileInMemoryEx( &vDATA[0], UINT(vDATA.size()), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, ppTexture);
		}

		// The image keeps its own size and format (D3DX would convert it to the
		// requested one); BC1/2/3 stay compressed, the rest becomes A8R8G8B8.
		HRESULT CreateTextureFromFileInMemoryEx( LPCVOID pSrcData, UINT SrcDataSize, UINT Width, UINT Height, UINT MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, DWORD Filter, DWORD MipFilter, D3DCOLOR ColorKey, D3DXIMAGE_INFO *pSrcInfo, PALETTEENTRY *pPalette, LPTTEXTURE *ppTexture) override
		{
			if( !ppTexture || !pSrcData || !SrcDataSize )
				return D3DERR_INVALIDCALL;

			bimg::ImageContainer *pIMAGE = bimg::imageParse( &g_vALLOCATOR, pSrcData, SrcDataSize);
			if(!pIMAGE)
				return D3DXERR_INVALIDDATA;

			if( GetD3DFormat(pIMAGE->m_format) == D3DFMT_UNKNOWN )
			{
				bimg::ImageContainer *pBGRA = bimg::imageConvert( &g_vALLOCATOR, bimg::TextureFormat::BGRA8, *pIMAGE);

				bimg::imageFree(pIMAGE);
				pIMAGE = pBGRA;

				if(!pIMAGE)
					return D3DXERR_INVALIDDATA;
			}

			D3DFORMAT nFormat = GetD3DFormat(pIMAGE->m_format);
			CTTextureBGFX *pTexture = new CTTextureBGFX( pIMAGE->m_width, pIMAGE->m_height, pIMAGE->m_numMips, Usage, nFormat, Pool);

			for( UINT i=0; i<pTexture->m_nLevels; i++)
			{
				bimg::ImageMip vMIP;

				if(bimg::imageGetRawData( *pIMAGE, 0, uint8_t(i), pIMAGE->m_data, pIMAGE->m_size, vMIP))
				{
					std::vector<BYTE>& vLEVEL = pTexture->GetLevel(i);
					memcpy( &vLEVEL[0], vMIP.m_data, TMin( size_t(vMIP.m_size), vLEVEL.size()));
				}
			}

			if(pSrcInfo)
			{
				ZeroMemory( pSrcInfo, sizeof(D3DXIMAGE_INFO));
				pSrcInfo->Width = pIMAGE->m_width;
				pSrcInfo->Height = pIMAGE->m_height;
				pSrcInfo->Depth = 1;
				pSrcInfo->MipLevels = pIMAGE->m_numMips;
				pSrcInfo->Format = nFormat;
				pSrcInfo->ResourceType = D3DRTYPE_TEXTURE;
				pSrcInfo->ImageFileFormat = GetFileFormat( pSrcData, SrcDataSize);
			}

			bimg::imageFree(pIMAGE);
			*ppTexture = pTexture;

			return D3D_OK;
		}

		HRESULT LoadSurfaceFromSurface( LPTSURFACE pDestSurface, const PALETTEENTRY *pDestPalette, const RECT *pDestRect, LPTSURFACE pSrcSurface, const PALETTEENTRY *pSrcPalette, const RECT *pSrcRect, DWORD Filter, D3DCOLOR ColorKey) override { return D3D_OK; }
		HRESULT LoadSurfaceFromFileInMemory( LPTSURFACE pDestSurface, const PALETTEENTRY *pDestPalette, const RECT *pDestRect, LPCVOID pSrcData, UINT SrcDataSize, const RECT *pSrcRect, DWORD Filter, D3DCOLOR ColorKey, D3DXIMAGE_INFO *pSrcInfo) override { return D3D_OK; }
		HRESULT SaveSurfaceToFile( LPCSTR pDestFile, D3DXIMAGE_FILEFORMAT DestFormat, LPTSURFACE pSrcSurface, const PALETTEENTRY *pSrcPalette, const RECT *pSrcRect) override { return D3DERR_NOTAVAILABLE; }
		HRESULT SaveSurfaceToFileInMemory( LPD3DXBUFFER *ppDestBuf, D3DXIMAGE_FILEFORMAT DestFormat, LPTSURFACE pSrcSurface, const PALETTEENTRY *pSrcPalette, const RECT *pSrcRect) override { return D3DERR_NOTAVAILABLE; }
		HRESULT SaveTextureToFile( LPCSTR pDestFile, D3DXIMAGE_FILEFORMAT DestFormat, LPTTEXTURE pSrcTexture, const PALETTEENTRY *pSrcPalette) override { return D3DERR_NOTAVAILABLE; }

	private:
		static uint32_t GetResetFlags( const D3DPRESENT_PARAMETERS& vPresent)
		{
			return vPresent.PresentationInterval == D3DPRESENT_INTERVAL_IMMEDIATE ? BGFX_RESET_NONE : BGFX_RESET_VSYNC;
		}

		void ResetViewport()
		{
			m_vVIEWPORT.X = 0;
			m_vVIEWPORT.Y = 0;
			m_vVIEWPORT.Width = m_pRTARGET ? m_pRTARGET->m_vDESC.Width : m_vPRESENT.BackBufferWidth;
			m_vVIEWPORT.Height = m_pRTARGET ? m_pRTARGET->m_vDESC.Height : m_vPRESENT.BackBufferHeight;
			m_vVIEWPORT.MinZ = 0.0f;
			m_vVIEWPORT.MaxZ = 1.0f;
		}

		enum { RENDER_STATES = 256, STAGE_STATES = 33, SAMPLER_STATES = 14, TEXTURE_STAGES = 8 };

		D3DPRESENT_PARAMETERS m_vPRESENT;
		bgfx::SwapChain m_vSWAPCHAIN;
		D3DCAPS9 m_vCAPS;
		BOOL m_bREADY;
		DWORD m_dwFrame;

		CTSurfaceBGFX *m_pBACKBUF;
		CTSurfaceBGFX *m_pZBUF;
		CTSurfaceBGFX *m_pRTARGET;
		CTSurfaceBGFX *m_pDEPTH;
		LPTTEXTURE m_pTEXTURE[TEXTURE_STAGES];

		DWORD m_vRS[RENDER_STATES];
		DWORD m_vTSS[TEXTURE_STAGES][STAGE_STATES];
		DWORD m_vSAMP[TEXTURE_STAGES][SAMPLER_STATES];
		DWORD m_dwFVF;

		D3DVIEWPORT9 m_vVIEWPORT;
		D3DMATERIAL9 m_vMATERIAL;
		std::map< DWORD, D3DMATRIX> m_mapTRANSFORM;
		std::map< DWORD, D3DLIGHT9> m_mapLIGHT;
		std::map< DWORD, BOOL> m_mapLIGHTON;
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
