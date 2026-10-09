#pragma once

// The render layer: every draw call, render state and resource of the engine
// goes through these classes instead of the D3D9 interfaces, so the bgfx port
// only has to provide another implementation of them. For now they forward to
// D3D9 with the same methods and signatures.

#include <map>
#include <mutex>

// A D3D9 object behind the layer. There is exactly one wrapper per D3D9
// object (D3D9 hands out the same surface pointer twice, and the engine
// compares them), and the wrapper holds one D3D9 reference for all of its own.
template< class T, class RAW > class TRenderResource
{
public:
	// The D3D9 object, for the D3D9 backend code only.
	RAW *GetD3D9() { return m_pD3D9; }

	ULONG AddRef()
	{
		std::lock_guard<std::mutex> lock(Mutex());
		return ++m_nRef;
	}

	ULONG Release()
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
			delete static_cast<T *>(this);
		}

		return nRef;
	}

	// Takes over one reference of pD3D9 and returns its wrapper.
	static T *Wrap( RAW *pD3D9)
	{
		if(!pD3D9)
			return NULL;

		{
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
	}

	static RAW *D3D9( T *pResource)
	{
		return pResource ? pResource->m_pD3D9 : NULL;
	}

protected:
	TRenderResource( RAW *pD3D9) : m_pD3D9(pD3D9), m_nRef(1) {}
	~TRenderResource() {}

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


class CTSurface : public TRenderResource< CTSurface, IDirect3DSurface9>
{
	friend class TRenderResource< CTSurface, IDirect3DSurface9>;
	CTSurface( IDirect3DSurface9 *pD3D9) : TRenderResource(pD3D9) {}

public:
	HRESULT GetDesc( D3DSURFACE_DESC *pDesc) { return m_pD3D9->GetDesc(pDesc); }
	HRESULT LockRect( D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) { return m_pD3D9->LockRect( pLockedRect, pRect, Flags); }
	HRESULT UnlockRect() { return m_pD3D9->UnlockRect(); }
};

class CTTexture : public TRenderResource< CTTexture, IDirect3DTexture9>
{
	friend class TRenderResource< CTTexture, IDirect3DTexture9>;
	CTTexture( IDirect3DTexture9 *pD3D9) : TRenderResource(pD3D9) {}

public:
	HRESULT GetLevelDesc( UINT Level, D3DSURFACE_DESC *pDesc) { return m_pD3D9->GetLevelDesc( Level, pDesc); }
	HRESULT LockRect( UINT Level, D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) { return m_pD3D9->LockRect( Level, pLockedRect, pRect, Flags); }
	HRESULT UnlockRect( UINT Level) { return m_pD3D9->UnlockRect(Level); }
	void GenerateMipSubLevels() { m_pD3D9->GenerateMipSubLevels(); }

	HRESULT GetSurfaceLevel( UINT Level, CTSurface **ppSurface)
	{
		IDirect3DSurface9 *pSurface = NULL;
		HRESULT hr = m_pD3D9->GetSurfaceLevel( Level, &pSurface);

		if(SUCCEEDED(hr))
			*ppSurface = CTSurface::Wrap(pSurface);

		return hr;
	}
};

class CTVertexBuffer : public TRenderResource< CTVertexBuffer, IDirect3DVertexBuffer9>
{
	friend class TRenderResource< CTVertexBuffer, IDirect3DVertexBuffer9>;
	CTVertexBuffer( IDirect3DVertexBuffer9 *pD3D9) : TRenderResource(pD3D9) {}

public:
	HRESULT GetDesc( D3DVERTEXBUFFER_DESC *pDesc) { return m_pD3D9->GetDesc(pDesc); }
	HRESULT Lock( UINT OffsetToLock, UINT SizeToLock, void **ppbData, DWORD Flags) { return m_pD3D9->Lock( OffsetToLock, SizeToLock, ppbData, Flags); }
	HRESULT Unlock() { return m_pD3D9->Unlock(); }
};

class CTIndexBuffer : public TRenderResource< CTIndexBuffer, IDirect3DIndexBuffer9>
{
	friend class TRenderResource< CTIndexBuffer, IDirect3DIndexBuffer9>;
	CTIndexBuffer( IDirect3DIndexBuffer9 *pD3D9) : TRenderResource(pD3D9) {}

public:
	HRESULT GetDesc( D3DINDEXBUFFER_DESC *pDesc) { return m_pD3D9->GetDesc(pDesc); }
	HRESULT Lock( UINT OffsetToLock, UINT SizeToLock, void **ppbData, DWORD Flags) { return m_pD3D9->Lock( OffsetToLock, SizeToLock, ppbData, Flags); }
	HRESULT Unlock() { return m_pD3D9->Unlock(); }
};

typedef CTSurface *LPTSURFACE;
typedef CTTexture *LPTTEXTURE;
typedef CTVertexBuffer *LPTVERTEXBUFFER;
typedef CTIndexBuffer *LPTINDEXBUFFER;


// Stores the wrapper of the D3D9 object in *ppD3D9 through ppOut when hr succeeded.
// ppD3D9 is read only here, after the call that fills it (the order in which
// function arguments are evaluated is unspecified).
template< class T, class RAW > inline HRESULT TRenderWrap( HRESULT hr, RAW **ppD3D9, T **ppOut)
{
	if(SUCCEEDED(hr))
		*ppOut = T::Wrap(*ppD3D9);

	return hr;
}


class CTRenderDevice
{
public:
	CTRenderDevice( LPDIRECT3DDEVICE9 pD3D9) : m_pD3D9(pD3D9) {}

	// The D3D9 device behind the layer, for the D3D9 backend code only.
	LPDIRECT3DDEVICE9 GetD3D9() { return m_pD3D9; }

	// Releases the D3D9 device and deletes the layer (it has a single owner).
	ULONG Release() { ULONG nCount = m_pD3D9->Release(); delete this; return nCount; }

	HRESULT TestCooperativeLevel() { return m_pD3D9->TestCooperativeLevel(); }
	UINT GetAvailableTextureMem() { return m_pD3D9->GetAvailableTextureMem(); }
	HRESULT GetDeviceCaps( D3DCAPS9* pCaps) { return m_pD3D9->GetDeviceCaps( pCaps); }
	HRESULT Reset( D3DPRESENT_PARAMETERS* pPresentationParameters) { return m_pD3D9->Reset( pPresentationParameters); }
	HRESULT Present( const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) { return m_pD3D9->Present( pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion); }
	void SetGammaRamp( UINT iSwapChain, DWORD Flags, const D3DGAMMARAMP* pRamp) { m_pD3D9->SetGammaRamp( iSwapChain, Flags, pRamp); }

	HRESULT CreateTexture( UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, LPTTEXTURE* ppTexture, HANDLE* pSharedHandle)
	{
		IDirect3DTexture9 *pD3D9 = NULL;
		return TRenderWrap( m_pD3D9->CreateTexture( Width, Height, Levels, Usage, Format, Pool, &pD3D9, pSharedHandle), &pD3D9, ppTexture);
	}

	HRESULT CreateVertexBuffer( UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, LPTVERTEXBUFFER* ppVertexBuffer, HANDLE* pSharedHandle)
	{
		IDirect3DVertexBuffer9 *pD3D9 = NULL;
		return TRenderWrap( m_pD3D9->CreateVertexBuffer( Length, Usage, FVF, Pool, &pD3D9, pSharedHandle), &pD3D9, ppVertexBuffer);
	}

	HRESULT CreateIndexBuffer( UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, LPTINDEXBUFFER* ppIndexBuffer, HANDLE* pSharedHandle)
	{
		IDirect3DIndexBuffer9 *pD3D9 = NULL;
		return TRenderWrap( m_pD3D9->CreateIndexBuffer( Length, Usage, Format, Pool, &pD3D9, pSharedHandle), &pD3D9, ppIndexBuffer);
	}

	HRESULT CreateDepthStencilSurface( UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, LPTSURFACE* ppSurface, HANDLE* pSharedHandle)
	{
		IDirect3DSurface9 *pD3D9 = NULL;
		return TRenderWrap( m_pD3D9->CreateDepthStencilSurface( Width, Height, Format, MultiSample, MultisampleQuality, Discard, &pD3D9, pSharedHandle), &pD3D9, ppSurface);
	}

	HRESULT CreateOffscreenPlainSurface( UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, LPTSURFACE* ppSurface, HANDLE* pSharedHandle)
	{
		IDirect3DSurface9 *pD3D9 = NULL;
		return TRenderWrap( m_pD3D9->CreateOffscreenPlainSurface( Width, Height, Format, Pool, &pD3D9, pSharedHandle), &pD3D9, ppSurface);
	}

	HRESULT GetRenderTargetData( LPTSURFACE pRenderTarget, LPTSURFACE pDestSurface) { return m_pD3D9->GetRenderTargetData( CTSurface::D3D9(pRenderTarget), CTSurface::D3D9(pDestSurface)); }
	HRESULT SetRenderTarget( DWORD RenderTargetIndex, LPTSURFACE pRenderTarget) { return m_pD3D9->SetRenderTarget( RenderTargetIndex, CTSurface::D3D9(pRenderTarget)); }
	HRESULT SetDepthStencilSurface( LPTSURFACE pNewZStencil) { return m_pD3D9->SetDepthStencilSurface( CTSurface::D3D9(pNewZStencil)); }

	HRESULT GetRenderTarget( DWORD RenderTargetIndex, LPTSURFACE* ppRenderTarget)
	{
		IDirect3DSurface9 *pD3D9 = NULL;
		return TRenderWrap( m_pD3D9->GetRenderTarget( RenderTargetIndex, &pD3D9), &pD3D9, ppRenderTarget);
	}

	HRESULT GetDepthStencilSurface( LPTSURFACE* ppZStencilSurface)
	{
		IDirect3DSurface9 *pD3D9 = NULL;
		return TRenderWrap( m_pD3D9->GetDepthStencilSurface( &pD3D9), &pD3D9, ppZStencilSurface);
	}

	HRESULT BeginScene() { return m_pD3D9->BeginScene(); }
	HRESULT EndScene() { return m_pD3D9->EndScene(); }
	HRESULT Clear( DWORD Count, const D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) { return m_pD3D9->Clear( Count, pRects, Flags, Color, Z, Stencil); }
	HRESULT SetTransform( D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix) { return m_pD3D9->SetTransform( State, pMatrix); }
	HRESULT SetViewport( const D3DVIEWPORT9* pViewport) { return m_pD3D9->SetViewport( pViewport); }
	HRESULT GetViewport( D3DVIEWPORT9* pViewport) { return m_pD3D9->GetViewport( pViewport); }
	HRESULT SetMaterial( const D3DMATERIAL9* pMaterial) { return m_pD3D9->SetMaterial( pMaterial); }
	HRESULT GetMaterial( D3DMATERIAL9* pMaterial) { return m_pD3D9->GetMaterial( pMaterial); }
	HRESULT SetLight( DWORD Index, const D3DLIGHT9* pLight) { return m_pD3D9->SetLight( Index, pLight); }
	HRESULT GetLight( DWORD Index, D3DLIGHT9* pLight) { return m_pD3D9->GetLight( Index, pLight); }
	HRESULT LightEnable( DWORD Index, BOOL Enable) { return m_pD3D9->LightEnable( Index, Enable); }
	HRESULT GetLightEnable( DWORD Index, BOOL* pEnable) { return m_pD3D9->GetLightEnable( Index, pEnable); }
	HRESULT SetRenderState( D3DRENDERSTATETYPE State, DWORD Value) { return m_pD3D9->SetRenderState( State, Value); }
	HRESULT GetRenderState( D3DRENDERSTATETYPE State, DWORD* pValue) { return m_pD3D9->GetRenderState( State, pValue); }
	HRESULT SetTexture( DWORD Stage, LPTTEXTURE pTexture) { return m_pD3D9->SetTexture( Stage, CTTexture::D3D9(pTexture)); }
	HRESULT SetTextureStageState( DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) { return m_pD3D9->SetTextureStageState( Stage, Type, Value); }
	HRESULT SetSamplerState( DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) { return m_pD3D9->SetSamplerState( Sampler, Type, Value); }
	HRESULT SetSoftwareVertexProcessing( BOOL bSoftware) { return m_pD3D9->SetSoftwareVertexProcessing( bSoftware); }
	HRESULT DrawPrimitive( D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) { return m_pD3D9->DrawPrimitive( PrimitiveType, StartVertex, PrimitiveCount); }
	HRESULT DrawIndexedPrimitive( D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) { return m_pD3D9->DrawIndexedPrimitive( Type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount); }
	HRESULT DrawPrimitiveUP( D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) { return m_pD3D9->DrawPrimitiveUP( PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride); }
	HRESULT CreateVertexDeclaration( const D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl) { return m_pD3D9->CreateVertexDeclaration( pVertexElements, ppDecl); }
	HRESULT SetVertexDeclaration( IDirect3DVertexDeclaration9* pDecl) { return m_pD3D9->SetVertexDeclaration( pDecl); }
	HRESULT SetFVF( DWORD FVF) { return m_pD3D9->SetFVF( FVF); }
	HRESULT CreateVertexShader( const DWORD* pFunction, IDirect3DVertexShader9** ppShader) { return m_pD3D9->CreateVertexShader( pFunction, ppShader); }
	HRESULT SetVertexShader( IDirect3DVertexShader9* pShader) { return m_pD3D9->SetVertexShader( pShader); }
	HRESULT SetVertexShaderConstantF( UINT StartRegister, const float* pConstantData, UINT Vector4fCount) { return m_pD3D9->SetVertexShaderConstantF( StartRegister, pConstantData, Vector4fCount); }
	HRESULT SetVertexShaderConstantI( UINT StartRegister, const int* pConstantData, UINT Vector4iCount) { return m_pD3D9->SetVertexShaderConstantI( StartRegister, pConstantData, Vector4iCount); }
	HRESULT SetVertexShaderConstantB( UINT StartRegister, const BOOL* pConstantData, UINT BoolCount) { return m_pD3D9->SetVertexShaderConstantB( StartRegister, pConstantData, BoolCount); }
	HRESULT SetStreamSource( UINT StreamNumber, LPTVERTEXBUFFER pStreamData, UINT OffsetInBytes, UINT Stride) { return m_pD3D9->SetStreamSource( StreamNumber, CTVertexBuffer::D3D9(pStreamData), OffsetInBytes, Stride); }
	HRESULT SetIndices( LPTINDEXBUFFER pIndexData) { return m_pD3D9->SetIndices( CTIndexBuffer::D3D9(pIndexData)); }
	HRESULT CreatePixelShader( const DWORD* pFunction, IDirect3DPixelShader9** ppShader) { return m_pD3D9->CreatePixelShader( pFunction, ppShader); }
	HRESULT SetPixelShader( IDirect3DPixelShader9* pShader) { return m_pD3D9->SetPixelShader( pShader); }
	HRESULT SetPixelShaderConstantF( UINT StartRegister, const float* pConstantData, UINT Vector4fCount) { return m_pD3D9->SetPixelShaderConstantF( StartRegister, pConstantData, Vector4fCount); }

private:
	LPDIRECT3DDEVICE9 m_pD3D9;
};

typedef CTRenderDevice *LPTRENDERDEVICE;


// The D3DX helpers used by the engine, on the layer types, so their call sites do not change.

inline HRESULT D3DXCreateTexture( LPTRENDERDEVICE pDevice, UINT Width, UINT Height, UINT MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, LPTTEXTURE *ppTexture)
{
	IDirect3DTexture9 *pD3D9 = NULL;
	return TRenderWrap( ::D3DXCreateTexture( pDevice->GetD3D9(), Width, Height, MipLevels, Usage, Format, Pool, &pD3D9), &pD3D9, ppTexture);
}

inline HRESULT D3DXCreateTextureFromFileA( LPTRENDERDEVICE pDevice, LPCSTR pSrcFile, LPTTEXTURE *ppTexture)
{
	IDirect3DTexture9 *pD3D9 = NULL;
	return TRenderWrap( ::D3DXCreateTextureFromFileA( pDevice->GetD3D9(), pSrcFile, &pD3D9), &pD3D9, ppTexture);
}

inline HRESULT D3DXCreateTextureFromFileInMemoryEx(
	LPTRENDERDEVICE pDevice,
	LPCVOID pSrcData,
	UINT SrcDataSize,
	UINT Width,
	UINT Height,
	UINT MipLevels,
	DWORD Usage,
	D3DFORMAT Format,
	D3DPOOL Pool,
	DWORD Filter,
	DWORD MipFilter,
	D3DCOLOR ColorKey,
	D3DXIMAGE_INFO *pSrcInfo,
	PALETTEENTRY *pPalette,
	LPTTEXTURE *ppTexture)
{
	IDirect3DTexture9 *pD3D9 = NULL;

	return TRenderWrap( ::D3DXCreateTextureFromFileInMemoryEx(
		pDevice->GetD3D9(),
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

inline HRESULT D3DXLoadSurfaceFromSurface(
	LPTSURFACE pDestSurface,
	const PALETTEENTRY *pDestPalette,
	const RECT *pDestRect,
	LPTSURFACE pSrcSurface,
	const PALETTEENTRY *pSrcPalette,
	const RECT *pSrcRect,
	DWORD Filter,
	D3DCOLOR ColorKey)
{
	return ::D3DXLoadSurfaceFromSurface(
		CTSurface::D3D9(pDestSurface),
		pDestPalette,
		pDestRect,
		CTSurface::D3D9(pSrcSurface),
		pSrcPalette,
		pSrcRect,
		Filter,
		ColorKey);
}

inline HRESULT D3DXLoadSurfaceFromFileInMemory(
	LPTSURFACE pDestSurface,
	const PALETTEENTRY *pDestPalette,
	const RECT *pDestRect,
	LPCVOID pSrcData,
	UINT SrcDataSize,
	const RECT *pSrcRect,
	DWORD Filter,
	D3DCOLOR ColorKey,
	D3DXIMAGE_INFO *pSrcInfo)
{
	return ::D3DXLoadSurfaceFromFileInMemory(
		CTSurface::D3D9(pDestSurface),
		pDestPalette,
		pDestRect,
		pSrcData,
		SrcDataSize,
		pSrcRect,
		Filter,
		ColorKey,
		pSrcInfo);
}

inline HRESULT D3DXSaveSurfaceToFileA( LPCSTR pDestFile, D3DXIMAGE_FILEFORMAT DestFormat, LPTSURFACE pSrcSurface, const PALETTEENTRY *pSrcPalette, const RECT *pSrcRect)
{
	return ::D3DXSaveSurfaceToFileA( pDestFile, DestFormat, CTSurface::D3D9(pSrcSurface), pSrcPalette, pSrcRect);
}

inline HRESULT D3DXSaveSurfaceToFileInMemory( LPD3DXBUFFER *ppDestBuf, D3DXIMAGE_FILEFORMAT DestFormat, LPTSURFACE pSrcSurface, const PALETTEENTRY *pSrcPalette, const RECT *pSrcRect)
{
	return ::D3DXSaveSurfaceToFileInMemory( ppDestBuf, DestFormat, CTSurface::D3D9(pSrcSurface), pSrcPalette, pSrcRect);
}

inline HRESULT D3DXSaveTextureToFileA( LPCSTR pDestFile, D3DXIMAGE_FILEFORMAT DestFormat, LPTTEXTURE pSrcTexture, const PALETTEENTRY *pSrcPalette)
{
	return ::D3DXSaveTextureToFileA( pDestFile, DestFormat, CTTexture::D3D9(pSrcTexture), pSrcPalette);
}
