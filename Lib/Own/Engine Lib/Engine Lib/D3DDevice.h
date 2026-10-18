// D3DDevice.h: interface for the CD3DDevice class.
//
//////////////////////////////////////////////////////////////////////

#if !defined ___D3DDEVICE_H
#define ___D3DDEVICE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


typedef enum PSTYPE
{
	PS_STAGE1 = 0,
	PS_MODULATE,
	PS_MODULATE2X,
	PS_MODULATE4X,
	PS_ADD,
	PS_ADDSIGNED,
	PS_ADDSIGNED2X,
	PS_SUBTRACT,
	PS_ADDSMOOTH,
	PS_BLENDDIFFUSEALPHA,
	PS_BLENDTEXTUREALPHA,
	PS_BLENDFACTORALPHA,
	PS_BLENDTEXTUREALPHAPM,
	PS_BLENDCURRENTALPHA,
	PS_MODULATEALPHA_ADDCOLOR,
	PS_MODULATECOLOR_ADDALPHA,
	PS_MODULATEINVALPHA_ADDCOLOR,
	PS_MODULATEINVCOLOR_ADDALPHA,
	PS_DOTPRODUCT3,
	PS_MULTIPLYADD,
	PS_LERP,
	PS_SHADER,
	PS_DETAILMAP,
	PS_MAP,
	PS_COUNT
} *LPPSTYPE;


typedef enum VSTYPE
{
	VS_WMESHVERTEX = 0,
	VS_MESHVERTEX,
	VS_LVERTEX,
	VS_COUNT
} *LPVSTYPE;


typedef enum PSCONSTANT
{
	PC_COMMON = 0,
	PC_ARG,
	PC_TFACTOR,
	PC_COUNT
} *LPPSCONSTANT;


typedef enum VSCONSTANT
{
	VC_COMMON = 0,
	VC_WORLD,
	VC_PROJ,
	VC_TEXTRAN0,
	VC_TEXTRAN1,
	VC_MTRLAMBIENT,
	VC_MTRLDIFFUSE,
	VC_AMBIENT,
	VC_CAMPOS,
	VC_LIGHTCOUNT,
	VC_LIGHTAMBIENT,
	VC_LIGHTDIFFUSE,
	VC_LIGHTDIR,
	VC_SKINNING,
	VC_COUNT
} *LPVSCONSTANT;

typedef enum TEXTURE_DETAIL_LEVEL
{
	TEXTURE_DETAIL_LOW = 0,
	TEXTURE_DETAIL_MED,
	TEXTURE_DETAIL_HI,
	TEXTURE_DETAIL_COUNT
} *LPTEXTURE_DETAIL;

typedef struct tagGAMEOPTION
{
	DWORD m_dwPresentationInterval;
	DWORD m_dwSwapEffect;
	DWORD m_dwBehavior;
	DWORD m_dwScreenX;
	DWORD m_dwScreenY;

	BYTE m_bWindowedMode;
	BYTE m_bUseSHADER;
	BYTE m_bAniso;					// anisotropic texture filtering on world surfaces
	BYTE m_bBGFX;					// draw with the bgfx backend of the render layer (Settings/Renderer)

	TEXTURE_DETAIL_LEVEL m_nTextureDetail;

	tagGAMEOPTION()
	{
		m_dwPresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		m_dwSwapEffect = D3DSWAPEFFECT_COPY;
		m_dwBehavior = D3DCREATE_MIXED_VERTEXPROCESSING;
		m_dwScreenX = 1024;
		m_dwScreenY = 768;

		m_bWindowedMode = TRUE;
		m_bUseSHADER = TRUE;
		m_bAniso = TRUE;
		m_bBGFX = FALSE;

		m_nTextureDetail = TEXTURE_DETAIL_COUNT;
	}
} GAMEOPTION, *LPGAMEOPTION;


class CD3DDevice
{
protected:
	D3DFORMAT GetBestZFormat(
		UINT nAdapter,
		D3DDEVTYPE nType,
		D3DFORMAT nFormat);

	// The desktop's mode and the display's modes: from Direct3D 9, or without it from SDL.
	BOOL GetDesktopMode( D3DDISPLAYMODE *pMODE);
	void GetDisplayModes( D3DFORMAT nFormat, std::vector<D3DDISPLAYMODE>& vMODES);
	void ReleaseD3D9();

#ifdef TCLIENT_NO_D3D9
	static void GetDefaultCaps( D3DCAPS9 *pCaps);
#endif

	void InitBACK( D3DDISPLAYMODE *pMODE);
	void ReleaseBACK();
	void InitCAPS();
	void InitAnisoCaps();	// derive aniso state from caps + option and (re)apply MAXANISOTROPY
	void ApplyGammaRamp();	// build + set the fullscreen gamma ramp from the color controls

public:
	LPTTEXTURE LoadTexture(
		LPBYTE pResData,
		UINT nLength,
		UINT nMipLevels);

	BOOL InitDevices( CTWindow *pWnd);
	BOOL ResetHost();
	BOOL Reset();

	void BeginGLOWScene();
	void EndGLOWScene(
		CD3DCamera *pCAMERA,
		FLOAT fGlowRange);
	void ReleaseDevice();

public:
	void SetResetFlag( BYTE bRESET);
	BYTE GetResetFlag();

public:
	CTCriticalSection m_cs;

	BYTE m_bRESET;

public:
	static DWORD m_dwPolyCount;

	// Anisotropic filtering (Item 1). m_dwWorldAniso is 0 when off/unsupported, else the
	// MaxAnisotropy to use; m_WorldMinFilter is ANISOTROPIC when enabled else LINEAR. World
	// render paths read m_WorldMinFilter for their MINFILTER sampler state. Statics so the
	// LPTRENDERDEVICE-only paths (e.g. water) can reach them without a CD3DDevice handle.
	static DWORD m_dwWorldAniso;
	static D3DTEXTUREFILTERTYPE m_WorldMinFilter;

	// Color controls (Item 4). All default to 1.0 = no change. m_fAmbientScale brightens the
	// world ambient light (works windowed; applied to the variable scene-ambient setters via
	// ScaleAmbient). m_fBrightness/m_fContrast/m_fGamma drive a fullscreen gamma ramp (D3D9
	// gamma ramps only take effect in fullscreen mode).
	static FLOAT m_fAmbientScale;
	static FLOAT m_fBrightness;
	static FLOAT m_fContrast;
	static FLOAT m_fGamma;
	static DWORD ScaleAmbient( DWORD dwAmbient);	// scale the RGB of an ambient color by m_fAmbientScale

	LPTRENDERDEVICE m_pDevice;
#ifndef TCLIENT_NO_D3D9
	LPDIRECT3D9 m_pD3D;
#endif

	LPDIRECT3DVERTEXSHADER9 m_pVertexShader[VS_COUNT];
	LPDIRECT3DPIXELSHADER9 m_pPixelShader[PS_COUNT];
	LPDIRECT3DVERTEXDECLARATION9 m_pDECL[VS_COUNT];

	// GPU skinning (ported from 4retro): bones uploaded to a float texture and read
	// in a vs_3_0 shader via vertex-texture-fetch, so skinning runs in hardware
	// instead of the legacy software-VP constant-palette path. m_bGPUSkinReady is
	// TRUE only when all three were created successfully.
	static const int BONES_RING = 64;	// cycle bones textures per object to avoid per-frame lock stalls
	LPDIRECT3DVERTEXSHADER9 m_pSkinnedVS;
	LPDIRECT3DVERTEXDECLARATION9 m_pSkinnedDECL;
	LPTTEXTURE m_pBonesTexture[BONES_RING];
	DWORD m_dwBonesIndex;
	BYTE m_bGPUSkinReady;

	CString m_strResourceType;
	BYTE m_bEnableSHADER;

	DWORD m_dwVertexShader[VS_COUNT];
	DWORD m_dwPixelShader[PS_COUNT];

	WORD m_vConstantVS[VC_COUNT];
	WORD m_vConstantPS[PC_COUNT];

	D3DPRESENT_PARAMETERS m_vPRESENT;
	GAMEOPTION m_option;
	D3DCAPS9 m_vCAPS;

	ULONGLONG m_lVIDEOMEM;
	ULONGLONG m_lSYSMEM;

	LPTVERTEXBUFFER m_pBACKVB;
	LPTTEXTURE m_pGRAYTEX;
	LPTTEXTURE m_pGLOWTEX;
	LPTTEXTURE m_pBACKTEX;
	LPTTEXTURE m_pBACKTEX2;
	LPTSURFACE m_pGRAYBUF;
	LPTSURFACE m_pGLOWBUF;
	LPTSURFACE m_pBACKBUF;
	LPTSURFACE m_pBACKBUF2;
	LPTSURFACE m_pRTARGET;
	LPTSURFACE m_pZBUF;

protected:
	LPTVERTEXBUFFER m_pEraserVB;
	LPTINDEXBUFFER m_pEraserIB;

	CTWindow* m_pMainWnd;

public:
	CD3DDevice();
	virtual ~CD3DDevice();
};


#endif // !defined ___D3DDEVICE_H
