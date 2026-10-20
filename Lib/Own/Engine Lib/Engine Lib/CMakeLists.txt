# Tachyon 3D engine (Engine Lib.vcxproj)

set(EngineLib_SOURCES
    D3DCamera.cpp
    D3DDevice.cpp
    D3DFont.cpp
    D3DImage.cpp
    D3DLight.cpp
    D3DSound.cpp
    FrameGroup.cpp
    Packet.cpp
    StdAfx.cpp
    T3DMusic.cpp
    T3DTexture.cpp
    TachyonAnimation.cpp
    TachyonApp.cpp
    TachyonBSPMAP.cpp
    TachyonCompressor.cpp
    TachyonEXTMAP.cpp
    TachyonHUGEMAP.cpp
    TachyonMAP.cpp
    TachyonMedia.cpp
    TachyonInput.cpp
    TWindow.cpp
    TachyonMesh.cpp
    TachyonObject.cpp
    TachyonRes.cpp
    TachyonSession.cpp
    TachyonSFX.cpp
    TachyonSlashSFX.cpp
    TachyonWnd.cpp
    TBSPNode.cpp
    TBSPPortal.cpp
    TConsole.cpp
    TCrypt.cpp
    TFile.cpp
    TTextRaster.cpp
    TDynamicBillboard.cpp
    TMath.cpp
    TMiniAudio.cpp
    TModuleProtector.cpp
    TNICEventSink.cpp
    TProfile.cpp
    TShow.cpp
)

# The bgfx backend of the render layer. It is its own library because bx needs C++20 and a
# conforming preprocessor, while the engine keeps the compiler's default standard.
add_library(TRenderBGFX STATIC TRenderBGFX.cpp)
target_compile_features(TRenderBGFX PRIVATE cxx_std_20)
target_compile_definitions(TRenderBGFX PRIVATE ${TCLIENT_WIN32_DEFS} NDEBUG)
if(MSVC)
    target_compile_options(TRenderBGFX PRIVATE /Zc:__cplusplus /Zc:preprocessor)
endif()
target_include_directories(TRenderBGFX PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${TCLIENT_LIB_3RD}/stb_image_write")
target_link_libraries(TRenderBGFX
    PUBLIC
        tclient_dxsdk
    PRIVATE
        tclient_options
        bgfx
        bimg_decode
        bx
        $<$<PLATFORM_ID:Windows>:gdiplus>
        # The native window of the SDL window, elsewhere.
        $<$<NOT:$<PLATFORM_ID:Windows>>:SDL3::SDL3-static>
)

# The Direct3D 9 backend of the render layer.
if(NOT TCLIENT_NO_D3D9)
    list(APPEND EngineLib_SOURCES TRenderD3D9.cpp)
endif()

add_library(EngineLib STATIC ${EngineLib_SOURCES})
target_compile_definitions(EngineLib PRIVATE ${TCLIENT_WIN32_DEFS} NDEBUG TCLIENT_BGFX)
target_compile_options(EngineLib PRIVATE ${TCLIENT_NO_GS})
target_include_directories(EngineLib
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
    PRIVATE
        "${TCLIENT_LIB_OWN}/GameConfig"
        "${TCLIENT_LIB_3RD}/miniaudio"
        # TComp and Engine Lib include each other's headers.
        "${TCLIENT_LIB_OWN}/TComp/TComp"
        # stb_truetype, for the text drawn from font files.
        "${TCLIENT_LIB_3RD}/bgfx.cmake/bgfx/3rdparty"
)
# zlib: the prebuilt library on Windows, the system's on Linux.
if(WIN32)
    target_include_directories(EngineLib PUBLIC "${TCLIENT_LIB_3RD}/zlib/include")
    target_link_directories(EngineLib PUBLIC "${TCLIENT_LIB_3RD}/zlib/lib/${TCLIENT_ARCH}")
    target_link_libraries(EngineLib PUBLIC zlibstatic)
else()
    target_link_libraries(EngineLib PUBLIC ZLIB::ZLIB)
endif()
target_link_libraries(EngineLib
    PUBLIC
        tclient_dxsdk
        TCML
        SDL3::SDL3-static
    PRIVATE
        tclient_options
        TRenderBGFX
)
