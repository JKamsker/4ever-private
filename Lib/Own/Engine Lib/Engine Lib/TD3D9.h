// TD3D9.h: the Direct3D 9 declarations of the engine: the DirectX SDK's d3d9.h and d3dx9.h, or
// with TCLIENT_NO_D3D9 (bgfx only, Linux) the engine's own TD3D9Types.h. The D3DX math is the
// engine's own either way (TD3DXMath.h).
//////////////////////////////////////////////////////////////////////

#pragma once

#ifdef TCLIENT_NO_D3D9
#include "TD3D9Types.h"
#include "TD3DXMath.h"
#else
#include <d3d9.h>
#include "TD3DXMath.h"
#include <d3dx9.h>
#endif
