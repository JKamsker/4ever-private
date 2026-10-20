#pragma once

// The render layer: every draw call, render state and resource of the engine
// goes through these interfaces instead of the D3D9 ones. They keep the D3D9
// methods, signatures and types, so the engine code did not change. Two
// implementations exist: TRenderD3D9.cpp forwards to Direct3D 9, and
// TRenderBGFX.cpp draws with bgfx (Settings/Renderer=BGFX in config.ini).

class CTSurface
{
public:
	virtual ULONG AddRef() = 0;
	virtual ULONG Release() = 0;

	virtual HRESULT GetDesc( D3DSURFACE_DESC *pDesc) = 0;
	virtual HRESULT LockRect( D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) = 0;
	virtual HRESULT UnlockRect() = 0;

protected:
	virtual ~CTSurface() {}
};

class CTTexture
{
public:
	virtual ULONG AddRef() = 0;
	virtual ULONG Release() = 0;

	virtual HRESULT GetLevelDesc( UINT Level, D3DSURFACE_DESC *pDesc) = 0;
	virtual HRESULT GetSurfaceLevel( UINT Level, CTSurface **ppSurface) = 0;
	virtual HRESULT LockRect( UINT Level, D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) = 0;
	virtual HRESULT UnlockRect( UINT Level) = 0;
	virtual void GenerateMipSubLevels() = 0;

protected:
	virtual ~CTTexture() {}
};

class CTVertexBuffer
{
public:
	virtual ULONG AddRef() = 0;
	virtual ULONG Release() = 0;

	virtual HRESULT GetDesc( D3DVERTEXBUFFER_DESC *pDesc) = 0;
	virtual HRESULT Lock( UINT OffsetToLock, UINT SizeToLock, void **ppbData, DWORD Flags) = 0;
	virtual HRESULT Unlock() = 0;

protected:
	virtual ~CTVertexBuffer() {}
};

class CTIndexBuffer
{
public:
	virtual ULONG AddRef() = 0;
	virtual ULONG Release() = 0;

	virtual HRESULT GetDesc( D3DINDEXBUFFER_DESC *pDesc) = 0;
	virtual HRESULT Lock( UINT OffsetToLock, UINT SizeToLock, void **ppbData, DWORD Flags) = 0;
	virtual HRESULT Unlock() = 0;

protected:
	virtual ~CTIndexBuffer() {}
};

typedef CTSurface *LPTSURFACE;
typedef CTTexture *LPTTEXTURE;
typedef CTVertexBuffer *LPTVERTEXBUFFER;
typedef CTIndexBuffer *LPTINDEXBUFFER;


class CTRenderDevice
{
public:
	// The device the D3DX helpers below without a device argument work with.
	static CTRenderDevice *m_pCURRENT;

	// Releases the device and deletes the layer (it has a single owner).
	virtual ULONG Release() = 0;

	virtual HRESULT TestCooperativeLevel() = 0;
	virtual UINT GetAvailableTextureMem() = 0;
	virtual HRESULT GetDeviceCaps( D3DCAPS9 *pCaps) = 0;
	virtual HRESULT Reset( D3DPRESENT_PARAMETERS *pPresentationParameters) = 0;
	virtual HRESULT Present( const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion) = 0;
	virtual void SetGammaRamp( UINT iSwapChain, DWORD Flags, const D3DGAMMARAMP *pRamp) = 0;

	virtual HRESULT CreateTexture( UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, LPTTEXTURE *ppTexture, HANDLE *pSharedHandle) = 0;
	virtual HRESULT CreateVertexBuffer( UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, LPTVERTEXBUFFER *ppVertexBuffer, HANDLE *pSharedHandle) = 0;
	virtual HRESULT CreateIndexBuffer( UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, LPTINDEXBUFFER *ppIndexBuffer, HANDLE *pSharedHandle) = 0;
	virtual HRESULT CreateDepthStencilSurface( UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, LPTSURFACE *ppSurface, HANDLE *pSharedHandle) = 0;
	virtual HRESULT CreateOffscreenPlainSurface( UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, LPTSURFACE *ppSurface, HANDLE *pSharedHandle) = 0;

	virtual HRESULT GetRenderTargetData( LPTSURFACE pRenderTarget, LPTSURFACE pDestSurface) = 0;
	virtual HRESULT SetRenderTarget( DWORD RenderTargetIndex, LPTSURFACE pRenderTarget) = 0;
	virtual HRESULT GetRenderTarget( DWORD RenderTargetIndex, LPTSURFACE *ppRenderTarget) = 0;
	virtual HRESULT SetDepthStencilSurface( LPTSURFACE pNewZStencil) = 0;
	virtual HRESULT GetDepthStencilSurface( LPTSURFACE *ppZStencilSurface) = 0;

	virtual HRESULT BeginScene() = 0;
	virtual HRESULT EndScene() = 0;
	virtual HRESULT Clear( DWORD Count, const D3DRECT *pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) = 0;
	virtual HRESULT SetTransform( D3DTRANSFORMSTATETYPE State, const D3DMATRIX *pMatrix) = 0;
	virtual HRESULT SetViewport( const D3DVIEWPORT9 *pViewport) = 0;
	virtual HRESULT GetViewport( D3DVIEWPORT9 *pViewport) = 0;
	virtual HRESULT SetMaterial( const D3DMATERIAL9 *pMaterial) = 0;
	virtual HRESULT GetMaterial( D3DMATERIAL9 *pMaterial) = 0;
	virtual HRESULT SetLight( DWORD Index, const D3DLIGHT9 *pLight) = 0;
	virtual HRESULT GetLight( DWORD Index, D3DLIGHT9 *pLight) = 0;
	virtual HRESULT LightEnable( DWORD Index, BOOL Enable) = 0;
	virtual HRESULT GetLightEnable( DWORD Index, BOOL *pEnable) = 0;
	virtual HRESULT SetRenderState( D3DRENDERSTATETYPE State, DWORD Value) = 0;
	virtual HRESULT GetRenderState( D3DRENDERSTATETYPE State, DWORD *pValue) = 0;
	virtual HRESULT SetTexture( DWORD Stage, LPTTEXTURE pTexture) = 0;
	virtual HRESULT SetTextureStageState( DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) = 0;
	virtual HRESULT SetSamplerState( DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) = 0;
	virtual HRESULT SetSoftwareVertexProcessing( BOOL bSoftware) = 0;

	virtual HRESULT DrawPrimitive( D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) = 0;
	virtual HRESULT DrawIndexedPrimitive( D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) = 0;
	virtual HRESULT DrawPrimitiveUP( D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void *pVertexStreamZeroData, UINT VertexStreamZeroStride) = 0;

	virtual HRESULT CreateVertexDeclaration( const D3DVERTEXELEMENT9 *pVertexElements, IDirect3DVertexDeclaration9 **ppDecl) = 0;
	virtual HRESULT SetVertexDeclaration( IDirect3DVertexDeclaration9 *pDecl) = 0;
	virtual HRESULT SetFVF( DWORD FVF) = 0;
	virtual HRESULT CreateVertexShader( const DWORD *pFunction, IDirect3DVertexShader9 **ppShader) = 0;
	virtual HRESULT SetVertexShader( IDirect3DVertexShader9 *pShader) = 0;
	virtual HRESULT SetVertexShaderConstantF( UINT StartRegister, const float *pConstantData, UINT Vector4fCount) = 0;
	virtual HRESULT SetVertexShaderConstantI( UINT StartRegister, const int *pConstantData, UINT Vector4iCount) = 0;
	virtual HRESULT SetVertexShaderConstantB( UINT StartRegister, const BOOL *pConstantData, UINT BoolCount) = 0;
	virtual HRESULT SetStreamSource( UINT StreamNumber, LPTVERTEXBUFFER pStreamData, UINT OffsetInBytes, UINT Stride) = 0;
	virtual HRESULT SetIndices( LPTINDEXBUFFER pIndexData) = 0;
	virtual HRESULT CreatePixelShader( const DWORD *pFunction, IDirect3DPixelShader9 **ppShader) = 0;
	virtual HRESULT SetPixelShader( IDirect3DPixelShader9 *pShader) = 0;
	virtual HRESULT SetPixelShaderConstantF( UINT StartRegister, const float *pConstantData, UINT Vector4fCount) = 0;

	// The D3DX helpers used by the engine (see the inline functions below).
	virtual HRESULT CreateTextureD3DX( UINT Width, UINT Height, UINT MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, LPTTEXTURE *ppTexture) = 0;
	virtual HRESULT CreateTextureFromFile( LPCSTR pSrcFile, LPTTEXTURE *ppTexture) = 0;
	virtual HRESULT CreateTextureFromFileInMemoryEx( LPCVOID pSrcData, UINT SrcDataSize, UINT Width, UINT Height, UINT MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, DWORD Filter, DWORD MipFilter, D3DCOLOR ColorKey, D3DXIMAGE_INFO *pSrcInfo, PALETTEENTRY *pPalette, LPTTEXTURE *ppTexture) = 0;
	virtual HRESULT LoadSurfaceFromSurface( LPTSURFACE pDestSurface, const PALETTEENTRY *pDestPalette, const RECT *pDestRect, LPTSURFACE pSrcSurface, const PALETTEENTRY *pSrcPalette, const RECT *pSrcRect, DWORD Filter, D3DCOLOR ColorKey) = 0;
	virtual HRESULT LoadSurfaceFromFileInMemory( LPTSURFACE pDestSurface, const PALETTEENTRY *pDestPalette, const RECT *pDestRect, LPCVOID pSrcData, UINT SrcDataSize, const RECT *pSrcRect, DWORD Filter, D3DCOLOR ColorKey, D3DXIMAGE_INFO *pSrcInfo) = 0;
	virtual HRESULT SaveSurfaceToFile( LPCSTR pDestFile, D3DXIMAGE_FILEFORMAT DestFormat, LPTSURFACE pSrcSurface, const PALETTEENTRY *pSrcPalette, const RECT *pSrcRect) = 0;
	virtual HRESULT SaveSurfaceToFileInMemory( LPD3DXBUFFER *ppDestBuf, D3DXIMAGE_FILEFORMAT DestFormat, LPTSURFACE pSrcSurface, const PALETTEENTRY *pSrcPalette, const RECT *pSrcRect) = 0;
	virtual HRESULT SaveTextureToFile( LPCSTR pDestFile, D3DXIMAGE_FILEFORMAT DestFormat, LPTTEXTURE pSrcTexture, const PALETTEENTRY *pSrcPalette) = 0;

protected:
	CTRenderDevice() { m_pCURRENT = this; }
	virtual ~CTRenderDevice() { if( m_pCURRENT == this ) m_pCURRENT = NULL; }
};

typedef CTRenderDevice *LPTRENDERDEVICE;

// The two backends. CreateRenderDeviceBGFX exists only in the CMake build (TCLIENT_BGFX).
#ifndef TCLIENT_NO_D3D9
LPTRENDERDEVICE CreateRenderDeviceD3D9( LPDIRECT3DDEVICE9 pD3D9);
#endif
LPTRENDERDEVICE CreateRenderDeviceBGFX( HWND hWnd, const D3DPRESENT_PARAMETERS *pPresent, const D3DCAPS9 *pCaps);
// bgfx's renderer: Auto (default), D3D11, D3D12, Vulkan or OpenGL (Settings/BGFXRenderer).
void SetRenderDeviceBGFXType( LPCSTR lpszRenderer);


// The D3DX helpers used by the engine, on the layer types, so their call sites do not change.

inline HRESULT D3DXCreateTexture( LPTRENDERDEVICE pDevice, UINT Width, UINT Height, UINT MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, LPTTEXTURE *ppTexture)
{
	return pDevice->CreateTextureD3DX( Width, Height, MipLevels, Usage, Format, Pool, ppTexture);
}

inline HRESULT D3DXCreateTextureFromFileA( LPTRENDERDEVICE pDevice, LPCSTR pSrcFile, LPTTEXTURE *ppTexture)
{
	return pDevice->CreateTextureFromFile( pSrcFile, ppTexture);
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
	return pDevice->CreateTextureFromFileInMemoryEx(
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
		ppTexture);
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
	return CTRenderDevice::m_pCURRENT->LoadSurfaceFromSurface(
		pDestSurface,
		pDestPalette,
		pDestRect,
		pSrcSurface,
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
	return CTRenderDevice::m_pCURRENT->LoadSurfaceFromFileInMemory(
		pDestSurface,
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
	return CTRenderDevice::m_pCURRENT->SaveSurfaceToFile( pDestFile, DestFormat, pSrcSurface, pSrcPalette, pSrcRect);
}

inline HRESULT D3DXSaveSurfaceToFileInMemory( LPD3DXBUFFER *ppDestBuf, D3DXIMAGE_FILEFORMAT DestFormat, LPTSURFACE pSrcSurface, const PALETTEENTRY *pSrcPalette, const RECT *pSrcRect)
{
	return CTRenderDevice::m_pCURRENT->SaveSurfaceToFileInMemory( ppDestBuf, DestFormat, pSrcSurface, pSrcPalette, pSrcRect);
}

inline HRESULT D3DXSaveTextureToFileA( LPCSTR pDestFile, D3DXIMAGE_FILEFORMAT DestFormat, LPTTEXTURE pSrcTexture, const PALETTEENTRY *pSrcPalette)
{
	return CTRenderDevice::m_pCURRENT->SaveTextureToFile( pDestFile, DestFormat, pSrcTexture, pSrcPalette);
}

#ifdef TCLIENT_NO_D3D9
// Without D3DX: the header reader of TImageInfo.h, and d3dx9tex.h's names for the functions above.
#include "TImageInfo.h"

inline HRESULT D3DXGetImageInfoFromFileInMemory( LPCVOID pSrcData, UINT SrcDataSize, D3DXIMAGE_INFO *pSrcInfo)
{
	return TImageInfo::Get( pSrcData, SrcDataSize, pSrcInfo);
}

#define D3DXCreateTextureFromFile	D3DXCreateTextureFromFileA
#define D3DXSaveSurfaceToFile		D3DXSaveSurfaceToFileA
#define D3DXSaveTextureToFile		D3DXSaveTextureToFileA
#endif
