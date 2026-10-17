#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#define _WIN32_DCOM 

// Global Include
#include "..\global\globalinc.h"

#include <TAfx.h>			// MFC's core classes without MFC
#include <direct.h>
#include <dbghelp.h>

#include <assert.h>
#include <fstream>
#include <iostream>
#include <tlhelp32.h>
#include <dxdiag.h>
#include <strsafe.h>
#include <winsvc.h>

//#define USE_HWID_AUTH // Toggle HWID Auth support, comment-out to disable. 

#ifdef TEST_MODE
#include <conio.h>
#endif

#ifdef USE_XTRAP
#include <XTrap4Client.h>
#endif

#ifdef USE_GG
#include "NPGameLib.h"
#endif

#include <T3D.h>
#include <CSProtocol.h>
#include <NetCode.h>
#include <TClientID.h>

#include <TChartType.h>
#include "TClientType.h"

#include "constant.hpp"
#include "helper.hpp"

// Explicit STL includes (newer MSVC no longer pulls these in transitively)
#include <algorithm>
#include <functional>
#include <numeric>
#include <iterator>

#pragma comment(lib, "Rpcrt4.lib")
