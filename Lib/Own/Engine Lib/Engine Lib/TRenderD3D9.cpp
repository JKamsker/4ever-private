#include "StdAfx.h"

// The Direct3D 9 backend of the render layer: every call forwards to D3D9.

#include <map>
#include <mutex>

CTRenderDevice *CTRenderDevice::m_pCURRENT = NULL;

namespace
{
	// A D3D9 object behind the layer. There is exactly one wrapper per D3D9
	// object (D3D9 hands out the same surface pointer twice, and the engine
	// compares them), and the wrapper holds one D3D9 reference for all of its own.
	template< class T, class RAW, class BASE > class TRenderResource : public BASE
	{
	public:
		RAW *GetD3D9() { return m_pD3D9; }

		ULONG AddRef() override
		{
			std::lock_guard<std::mutex> lock(Mutex());
			return ++m_nRef;
		}

		ULONG Release() override
		{
			ULONG nRef;
			{
				std::lock_guard<std::mutex> lock(Mutex());

				nRef = --m_nRef;
				if(!nRef)
					Map().erase(m_pD3D9);
			}

			if(!nRef)
			{
				m_pD3D9->Release();
				delete this;
			}

			return nRef;
		}

		// Takes over one reference of pD3D9 and returns its wrapper.
		static T *Wrap( RAW *pD3D9)
		{
			if(!pD3D9)
				return NULL;

			std::lock_guard<std::mutex> lock(Mutex());
			typename std::map< RAW *, T *>::iterator finder = Map().find(pD3D9);

			if( finder != Map().end() )
			{
				(*finder).second->m_nRef++;
				pD3D9->Release();

				return (*finder).second;
			}

			T *pResult = new T(pD3D9);
			Map().insert( std::make_pair( pD3D9, pResult));

			return pResult;
		}

		static RAW *D3D9( BASE *pResource)
		{
			return pResource ? static_cast<T *>(pResource)->m_pD3D9 : NULL;
		}

	protected:
		TRenderResource( RAW *pD3D9) : m_pD3D9(pD3D9), m_nRef(1) {}

		// Never destroyed: static objects still release resources while the
		// process exits, after function statics would be gone.
		static std::map< RAW *, T *>& Map()
		{
			static std::map< RAW *, T *> *pWRAPPER = new std::map< RAW *, T *>();
			return *pWRAPPER;
		}

		static std::mutex& Mutex()
		{
			static std::mutex *pMUTEX = new std::mutex();
			return *pMUTEX;
		}

		RAW *m_pD3D9;
		ULONG m_nRef;
	};


	class CTSurfaceD3D9 : public TRenderResource< CTSurfaceD3D9, IDirect3DSurface9, CTSurface>
	{
	public:
		CTSurfaceD3D9( IDirect3DSurface9 *pD3D9) : TRenderResource(pD3D9) {}

		HRESULT GetDesc( D3DSURFACE_DESC *pDesc) override { return m_pD3D9->GetDesc(pDesc); }
		HRESULT LockRect( D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) override { return m_pD3D9->LockRect( pLockedRect, pRect, Flags); }
		HRESULT UnlockRect() override { return m_pD3D9->UnlockRect(); }
	};

	class CTTextureD3D9 : public TRenderResource< CTTextureD3D9, IDirect3DTexture9, CTTexture>
	{
	public:
		CTTextureD3D9( IDirect3DTexture9 *pD3D9) : TRenderResource(pD3D9) {}

		HRESULT GetLevelDesc( UINT Level, D3DSURFACE_DESC *pDesc) override { return m_pD3D9->GetLevelDesc( Level, pDesc); }
		HRESULT LockRect( UINT Level, D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) override { return m_pD3D9->LockRect( Level, pLockedRect, pRect, Flags); }
		HRESULT UnlockRect( UINT Level) override { return m_pD3D9->UnlockRect(Level); }
		void GenerateMipSubLevels() override { m_pD3D9->GenerateMipSubLevels(); }

		HRESULT GetSurfaceLevel( UINT Level, CTSurface **ppSurface) override
		{
			IDirect3DSurface9 *pSurface = NULL;
			HRESULT hr = m_pD3D9->GetSurfaceLevel( Level, &pSurface);

			if(SUCCEEDED(hr))
				*ppSurface = CTSurfaceD3D9::Wrap(pSurface);

			return hr;
		}
	};

	class CTVertexBufferD3D9 : public TRenderResource< CTVertexBufferD3D9, IDirect3DVertexBuffer9, CTVertexBuffer>
	{
	public:
		CTVertexBufferD3D9( IDirect3DVertexBuffer9 *pD3D9) : TRenderResource(pD3D9) {}

		HRESULT GetDesc( D3DVERTEXBUFFER_DESC *pDesc) override { return m_pD3D9->GetDesc(pDesc); }
		HRESULT Lock( UINT OffsetToLock, UINT SizeToLock, void **ppbData, DWORD Flags) override { return m_pD3D9->Lock( OffsetToLock, SizeToLock, ppbData, Flags); }
		HRESULT Unlock() override { return m_pD3D9->Unlock(); }
	};

	class CTIndexBufferD3D9 : public TRenderResource< CTIndexBufferD3D9, IDirect3DIndexBuffer9, CTIndexBuffer>
	{
	public:
		CTIndexBufferD3D9( IDirect3DIndexBuffer9 *pD3D9) : TRenderResource(pD3D9) {}

		HRESULT GetDesc( D3DINDEXBUFFER_DESC *pDesc) override { return m_pD3D9->GetDesc(pDesc); }
		HRESULT Lock( UINT OffsetToLock, UINT SizeToLock, void **ppbData, DWORD Flags) override { return m_pD3D9->Lock( OffsetToLock, SizeToLock, ppbData, Flags); }
		HRESULT Unlock() override { return m_pD3D9->Unlock(); }
	};

	inline IDirect3DSurface9 *D3D9( CTSurface *p) { return CTSurfaceD3D9::D3D9(p); }
	inline IDirect3DTexture9 *D3D9( CTTexture *p) { return CTTextureD3D9::D3D9(p); }
	inline IDirect3DVertexBuffer9 *D3D9( CTVertexBuffer *p) { return CTVertexBufferD3D9::D3D9(p); }
	inline IDirect3DIndexBuffer9 *D3D9( CTIndexBuffer *p) { return CTIndexBufferD3D9::D3D9(p); }

	// Stores the wrapper of the D3D9 object in *ppD3D9 through ppOut when hr succeeded.
	// ppD3D9 is read only here, after the call that fills it (the order in which
	// function arguments are evaluated is unspecified).
	template< class T, class RAW, class BASE > inline HRESULT Wrap( HRESULT hr, RAW **ppD3D9, BASE **ppOut)
	{
		if(SUCCEEDED(hr))
			*ppOut = T::Wrap(*ppD3D9);

		return hr;
	}


	class CTRenderDeviceD3D9 : public CTRenderDevice
	{
	public:
		CTRenderDeviceD3D9( LPDIRECT3DDEVICE9 pD3D9) : m_pD3D9(pD3D9), m_pDUMP(NULL), m_nDraws(0) {}

		ULONG Release() override { ULONG nCount = m_pD3D9->Release(); delete this; return nCount; }

		HRESULT TestCooperativeLevel() override { return m_pD3D9->TestCooperativeLevel(); }
		UINT GetAvailableTextureMem() override { return m_pD3D9->GetAvailableTextureMem(); }
		HRESULT GetDeviceCaps( D3DCAPS9 *pCaps) override { return m_pD3D9->GetDeviceCaps(pCaps); }
		HRESULT Reset( D3DPRESENT_PARAMETERS *pPresentationParameters) override { return m_pD3D9->Reset(pPresentationParameters); }
		HRESULT Present( const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion) override
		{
			UpdateDump();
			return m_pD3D9->Present( pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
		}
		void SetGammaRamp( UINT iSwapChain, DWORD Flags, const D3DGAMMARAMP *pRamp) override { m_pD3D9->SetGammaRamp( iSwapChain, Flags, pRamp); }

		HRESULT CreateTexture( UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, LPTTEXTURE *ppTexture, HANDLE *pSharedHandle) override
		{
			IDirect3DTexture9 *pD3D9 = NULL;
			return Wrap<CTTextureD3D9>( m_pD3D9->CreateTexture( Width, Height, Levels, Usage, Format, Pool, &pD3D9, pSharedHandle), &pD3D9, ppTexture);
		}

		HRESULT CreateVertexBuffer( UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, LPTVERTEXBUFFER *ppVertexBuffer, HANDLE *pSharedHandle) override
		{
			IDirect3DVertexBuffer9 *pD3D9 = NULL;
			return Wrap<CTVertexBufferD3D9>( m_pD3D9->CreateVertexBuffer( Length, Usage, FVF, Pool, &pD3D9, pSharedHandle), &pD3D9, ppVertexBuffer);
		}

		HRESULT CreateIndexBuffer( UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, LPTINDEXBUFFER *ppIndexBuffer, HANDLE *pSharedHandle) override
		{
			IDirect3DIndexBuffer9 *pD3D9 = NULL;
			return Wrap<CTIndexBufferD3D9>( m_pD3D9->CreateIndexBuffer( Length, Usage, Format, Pool, &pD3D9, pSharedHandle), &pD3D9, ppIndexBuffer);
		}

		HRESULT CreateDepthStencilSurface( UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, LPTSURFACE *ppSurface, HANDLE *pSharedHandle) override
		{
			IDirect3DSurface9 *pD3D9 = NULL;
			return Wrap<CTSurfaceD3D9>( m_pD3D9->CreateDepthStencilSurface( Width, Height, Format, MultiSample, MultisampleQuality, Discard, &pD3D9, pSharedHandle), &pD3D9, ppSurface);
		}

		HRESULT CreateOffscreenPlainSurface( UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, LPTSURFACE *ppSurface, HANDLE *pSharedHandle) override
		{
			IDirect3DSurface9 *pD3D9 = NULL;
			return Wrap<CTSurfaceD3D9>( m_pD3D9->CreateOffscreenPlainSurface( Width, Height, Format, Pool, &pD3D9, pSharedHandle), &pD3D9, ppSurface);
		}

		HRESULT GetRenderTargetData( LPTSURFACE pRenderTarget, LPTSURFACE pDestSurface) override { return m_pD3D9->GetRenderTargetData( D3D9(pRenderTarget), D3D9(pDestSurface)); }
		HRESULT SetRenderTarget( DWORD RenderTargetIndex, LPTSURFACE pRenderTarget) override { return m_pD3D9->SetRenderTarget( RenderTargetIndex, D3D9(pRenderTarget)); }
		HRESULT SetDepthStencilSurface( LPTSURFACE pNewZStencil) override { return m_pD3D9->SetDepthStencilSurface(D3D9(pNewZStencil)); }

		HRESULT GetRenderTarget( DWORD RenderTargetIndex, LPTSURFACE *ppRenderTarget) override
		{
			IDirect3DSurface9 *pD3D9 = NULL;
			return Wrap<CTSurfaceD3D9>( m_pD3D9->GetRenderTarget( RenderTargetIndex, &pD3D9), &pD3D9, ppRenderTarget);
		}

		HRESULT GetDepthStencilSurface( LPTSURFACE *ppZStencilSurface) override
		{
			IDirect3DSurface9 *pD3D9 = NULL;
			return Wrap<CTSurfaceD3D9>( m_pD3D9->GetDepthStencilSurface(&pD3D9), &pD3D9, ppZStencilSurface);
		}

		HRESULT BeginScene() override { return m_pD3D9->BeginScene(); }
		HRESULT EndScene() override { return m_pD3D9->EndScene(); }
		HRESULT Clear( DWORD Count, const D3DRECT *pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) override { return m_pD3D9->Clear( Count, pRects, Flags, Color, Z, Stencil); }
		HRESULT SetTransform( D3DTRANSFORMSTATETYPE State, const D3DMATRIX *pMatrix) override { return m_pD3D9->SetTransform( State, pMatrix); }
		HRESULT SetViewport( const D3DVIEWPORT9 *pViewport) override { return m_pD3D9->SetViewport(pViewport); }
		HRESULT GetViewport( D3DVIEWPORT9 *pViewport) override { return m_pD3D9->GetViewport(pViewport); }
		HRESULT SetMaterial( const D3DMATERIAL9 *pMaterial) override { return m_pD3D9->SetMaterial(pMaterial); }
		HRESULT GetMaterial( D3DMATERIAL9 *pMaterial) override { return m_pD3D9->GetMaterial(pMaterial); }
		HRESULT SetLight( DWORD Index, const D3DLIGHT9 *pLight) override { return m_pD3D9->SetLight( Index, pLight); }
		HRESULT GetLight( DWORD Index, D3DLIGHT9 *pLight) override { return m_pD3D9->GetLight( Index, pLight); }
		HRESULT LightEnable( DWORD Index, BOOL Enable) override { return m_pD3D9->LightEnable( Index, Enable); }
		HRESULT GetLightEnable( DWORD Index, BOOL *pEnable) override { return m_pD3D9->GetLightEnable( Index, pEnable); }
		HRESULT SetRenderState( D3DRENDERSTATETYPE State, DWORD Value) override { return m_pD3D9->SetRenderState( State, Value); }
		HRESULT GetRenderState( D3DRENDERSTATETYPE State, DWORD *pValue) override { return m_pD3D9->GetRenderState( State, pValue); }
		HRESULT SetTexture( DWORD Stage, LPTTEXTURE pTexture) override { return m_pD3D9->SetTexture( Stage, D3D9(pTexture)); }
		HRESULT SetTextureStageState( DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) override { return m_pD3D9->SetTextureStageState( Stage, Type, Value); }
		HRESULT SetSamplerState( DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) override { return m_pD3D9->SetSamplerState( Sampler, Type, Value); }
		HRESULT SetSoftwareVertexProcessing( BOOL bSoftware) override { return m_pD3D9->SetSoftwareVertexProcessing(bSoftware); }

		HRESULT DrawPrimitive( D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) override { DumpDraw( PrimitiveType, PrimitiveCount, 0); return m_pD3D9->DrawPrimitive( PrimitiveType, StartVertex, PrimitiveCount); }
		HRESULT DrawIndexedPrimitive( D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) override { DumpDraw( Type, primCount, 0); return m_pD3D9->DrawIndexedPrimitive( Type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount); }
		HRESULT DrawPrimitiveUP( D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void *pVertexStreamZeroData, UINT VertexStreamZeroStride) override { DumpDraw( PrimitiveType, PrimitiveCount, VertexStreamZeroStride); return m_pD3D9->DrawPrimitiveUP( PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride); }

		// Diagnostics, like the bgfx backend's: when %TEMP%\tclient-d3d9-dump.flag exists, the
		// next frame's draw calls are written to %TEMP%\tclient-d3d9-dump.txt.
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
			sprintf_s( vFILE, "%stclient-d3d9-dump.flag", vPATH);

			if( GetFileAttributesA(vFILE) != INVALID_FILE_ATTRIBUTES )
			{
				FILE *pFLAG = NULL;

				s_nCount = 1;
				if( !fopen_s( &pFLAG, vFILE, "r") && pFLAG )
				{
					if( fscanf_s( pFLAG, "%d", &s_nCount) != 1 || s_nCount < 1 )
						s_nCount = 1;
					fclose(pFLAG);
				}

				DeleteFileA(vFILE);
				s_nLeft = s_nCount;
				s_nIndex = 0;
				s_dwNext = 0;
			}

			if( s_nLeft <= 0 || GetTickCount() < s_dwNext )
				return;

			if( s_nCount > 1 )
				sprintf_s( vFILE, "%stclient-d3d9-dump-%02d.txt", vPATH, s_nIndex);
			else
				sprintf_s( vFILE, "%stclient-d3d9-dump.txt", vPATH);

			s_nLeft--;
			s_nIndex++;
			s_dwNext = GetTickCount() + 1000;

			if( fopen_s( &m_pDUMP, vFILE, "w") )
				m_pDUMP = NULL;
			m_nDraws = 0;
		}

		void DumpDraw( D3DPRIMITIVETYPE nType, UINT nCount, UINT nStrideUP)
		{
			if(!m_pDUMP)
				return;

			DWORD dwFVF = 0;
			IDirect3DVertexBuffer9 *pVB = NULL;
			IDirect3DVertexDeclaration9 *pDECL = NULL;
			UINT nOffset = 0;
			UINT nStride = nStrideUP;
			DWORD vRS[8];
			static const D3DRENDERSTATETYPE vSTATE[8] = { D3DRS_LIGHTING, D3DRS_ALPHABLENDENABLE, D3DRS_SRCBLEND, D3DRS_DESTBLEND, D3DRS_BLENDOP, D3DRS_ALPHATESTENABLE, D3DRS_ZENABLE, D3DRS_TEXTUREFACTOR };

			m_pD3D9->GetFVF(&dwFVF);
			m_pD3D9->GetVertexDeclaration(&pDECL);
			if(!nStrideUP)
				m_pD3D9->GetStreamSource( 0, &pVB, &nOffset, &nStride);

			for( int i=0; i<8; i++)
				m_pD3D9->GetRenderState( vSTATE[i], &vRS[i]);

			fprintf( m_pDUMP, "#%u fvf=%x decl=%p st=%u prim=%d n=%u | L=%u blend=%u %u>%u op%u at=%u z=%u tf=%08x", m_nDraws++,
				dwFVF, (void *) pDECL, nStride, nType, nCount, vRS[0], vRS[1], vRS[2], vRS[3], vRS[4], vRS[5], vRS[6], vRS[7]);

			for( DWORD i=0; i<2; i++)
			{
				DWORD dwColorOp, dwAlphaOp, dwArg1, dwArg2;
				IDirect3DBaseTexture9 *pBASE = NULL;

				m_pD3D9->GetTextureStageState( i, D3DTSS_COLOROP, &dwColorOp);
				m_pD3D9->GetTextureStageState( i, D3DTSS_COLORARG1, &dwArg1);
				m_pD3D9->GetTextureStageState( i, D3DTSS_COLORARG2, &dwArg2);
				m_pD3D9->GetTextureStageState( i, D3DTSS_ALPHAOP, &dwAlphaOp);
				m_pD3D9->GetTexture( i, &pBASE);
				fprintf( m_pDUMP, " | s%u c=%u(%x,%x) a=%u tex=%p", i, dwColorOp, dwArg1, dwArg2, dwAlphaOp, (void *) pBASE);

				if(pBASE)
				{
					if( pBASE->GetType() == D3DRTYPE_TEXTURE )
					{
						IDirect3DTexture9 *pTEX = (IDirect3DTexture9 *) pBASE;
						D3DSURFACE_DESC vDESC;

						pTEX->GetLevelDesc( 0, &vDESC);
						fprintf( m_pDUMP, " fmt=%u %ux%u lv%u", vDESC.Format, vDESC.Width, vDESC.Height, pTEX->GetLevelCount());
					}

					pBASE->Release();
				}
			}

			fprintf( m_pDUMP, "\n");

			if(pVB)
				pVB->Release();

			if(pDECL)
				pDECL->Release();
		}

		FILE *m_pDUMP;
		UINT m_nDraws;

		HRESULT CreateVertexDeclaration( const D3DVERTEXELEMENT9 *pVertexElements, IDirect3DVertexDeclaration9 **ppDecl) override { return m_pD3D9->CreateVertexDeclaration( pVertexElements, ppDecl); }
		HRESULT SetVertexDeclaration( IDirect3DVertexDeclaration9 *pDecl) override { return m_pD3D9->SetVertexDeclaration(pDecl); }
		HRESULT SetFVF( DWORD FVF) override { return m_pD3D9->SetFVF(FVF); }
		HRESULT CreateVertexShader( const DWORD *pFunction, IDirect3DVertexShader9 **ppShader) override { return m_pD3D9->CreateVertexShader( pFunction, ppShader); }
		HRESULT SetVertexShader( IDirect3DVertexShader9 *pShader) override { return m_pD3D9->SetVertexShader(pShader); }
		HRESULT SetVertexShaderConstantF( UINT StartRegister, const float *pConstantData, UINT Vector4fCount) override { return m_pD3D9->SetVertexShaderConstantF( StartRegister, pConstantData, Vector4fCount); }
		HRESULT SetVertexShaderConstantI( UINT StartRegister, const int *pConstantData, UINT Vector4iCount) override { return m_pD3D9->SetVertexShaderConstantI( StartRegister, pConstantData, Vector4iCount); }
		HRESULT SetVertexShaderConstantB( UINT StartRegister, const BOOL *pConstantData, UINT BoolCount) override { return m_pD3D9->SetVertexShaderConstantB( StartRegister, pConstantData, BoolCount); }
		HRESULT SetStreamSource( UINT StreamNumber, LPTVERTEXBUFFER pStreamData, UINT OffsetInBytes, UINT Stride) override { return m_pD3D9->SetStreamSource( StreamNumber, D3D9(pStreamData), OffsetInBytes, Stride); }
		HRESULT SetIndices( LPTINDEXBUFFER pIndexData) override { return m_pD3D9->SetIndices(D3D9(pIndexData)); }
		HRESULT CreatePixelShader( const DWORD *pFunction, IDirect3DPixelShader9 **ppShader) override { return m_pD3D9->CreatePixelShader( pFunction, ppShader); }
		HRESULT SetPixelShader( IDirect3DPixelShader9 *pShader) override { return m_pD3D9->SetPixelShader(pShader); }
		HRESULT SetPixelShaderConstantF( UINT StartRegister, const float *pConstantData, UINT Vector4fCount) override { return m_pD3D9->SetPixelShaderConstantF( StartRegister, pConstantData, Vector4fCount); }

		HRESULT CreateTextureD3DX( UINT Width, UINT Height, UINT MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, LPTTEXTURE *ppTexture) override
		{
			IDirect3DTexture9 *pD3D9 = NULL;
			return Wrap<CTTextureD3D9>( ::D3DXCreateTexture( m_pD3D9, Width, Height, MipLevels, Usage, Format, Pool, &pD3D9), &pD3D9, ppTexture);
		}

		HRESULT CreateTextureFromFile( LPCSTR pSrcFile, LPTTEXTURE *ppTexture) override
		{
			IDirect3DTexture9 *pD3D9 = NULL;
			return Wrap<CTTextureD3D9>( ::D3DXCreateTextureFromFileA( m_pD3D9, pSrcFile, &pD3D9), &pD3D9, ppTexture);
		}

		HRESULT CreateTextureFromFileInMemoryEx( LPCVOID pSrcData, UINT SrcDataSize, UINT Width, UINT Height, UINT MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, DWORD Filter, DWORD MipFilter, D3DCOLOR ColorKey, D3DXIMAGE_INFO *pSrcInfo, PALETTEENTRY *pPalette, LPTTEXTURE *ppTexture) override
		{
			IDirect3DTexture9 *pD3D9 = NULL;

			return Wrap<CTTextureD3D9>( ::D3DXCreateTextureFromFileInMemoryEx(
				m_pD3D9,
				pSrcData,
				SrcDataSize,
				Width,
				Height,
				MipLevels,
				Usage,
				Format,
				Pool,
				Filter,
				MipFilter,
				ColorKey,
				pSrcInfo,
				pPalette,
				&pD3D9), &pD3D9, ppTexture);
		}

		HRESULT LoadSurfaceFromSurface( LPTSURFACE pDestSurface, const PALETTEENTRY *pDestPalette, const RECT *pDestRect, LPTSURFACE pSrcSurface, const PALETTEENTRY *pSrcPalette, const RECT *pSrcRect, DWORD Filter, D3DCOLOR ColorKey) override
		{
			return ::D3DXLoadSurfaceFromSurface( D3D9(pDestSurface), pDestPalette, pDestRect, D3D9(pSrcSurface), pSrcPalette, pSrcRect, Filter, ColorKey);
		}

		HRESULT LoadSurfaceFromFileInMemory( LPTSURFACE pDestSurface, const PALETTEENTRY *pDestPalette, const RECT *pDestRect, LPCVOID pSrcData, UINT SrcDataSize, const RECT *pSrcRect, DWORD Filter, D3DCOLOR ColorKey, D3DXIMAGE_INFO *pSrcInfo) override
		{
			return ::D3DXLoadSurfaceFromFileInMemory( D3D9(pDestSurface), pDestPalette, pDestRect, pSrcData, SrcDataSize, pSrcRect, Filter, ColorKey, pSrcInfo);
		}

		HRESULT SaveSurfaceToFile( LPCSTR pDestFile, D3DXIMAGE_FILEFORMAT DestFormat, LPTSURFACE pSrcSurface, const PALETTEENTRY *pSrcPalette, const RECT *pSrcRect) override
		{
			return ::D3DXSaveSurfaceToFileA( pDestFile, DestFormat, D3D9(pSrcSurface), pSrcPalette, pSrcRect);
		}

		HRESULT SaveSurfaceToFileInMemory( LPD3DXBUFFER *ppDestBuf, D3DXIMAGE_FILEFORMAT DestFormat, LPTSURFACE pSrcSurface, const PALETTEENTRY *pSrcPalette, const RECT *pSrcRect) override
		{
			return ::D3DXSaveSurfaceToFileInMemory( ppDestBuf, DestFormat, D3D9(pSrcSurface), pSrcPalette, pSrcRect);
		}

		HRESULT SaveTextureToFile( LPCSTR pDestFile, D3DXIMAGE_FILEFORMAT DestFormat, LPTTEXTURE pSrcTexture, const PALETTEENTRY *pSrcPalette) override
		{
			return ::D3DXSaveTextureToFileA( pDestFile, DestFormat, D3D9(pSrcTexture), pSrcPalette);
		}

	private:
		LPDIRECT3DDEVICE9 m_pD3D9;
	};
}

LPTRENDERDEVICE CreateRenderDeviceD3D9( LPDIRECT3DDEVICE9 pD3D9)
{
	return new CTRenderDeviceD3D9(pD3D9);
}
