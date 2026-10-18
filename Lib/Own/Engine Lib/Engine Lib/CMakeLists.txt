# Tachyon 3D engine (Engine Lib.vcxproj)

set(EngineLib_SOURCES
    CryptographyExt.cpp
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
    TDynamicBillboard.cpp
    TMath.cpp
    TMiniAudio.cpp
    TModuleProtector.cpp
    TNICEventSink.cpp
    TProfile.cpp
    TRenderD3D9.cpp
    TShow.cpp
)

# The bgfx backend of the render layer. It is its own library because bx needs C++20 and a
# conforming preprocessor, while the engine keeps the compiler's default standard.
add_library(TRenderBGFX STATIC TRenderBGFX.cpp)
target_compile_features(TRenderBGFX PRIVATE cxx_std_20)
target_compile_definitions(TRenderBGFX PRIVATE WIN32 NDEBUG _WINDOWS)
target_compile_options(TRenderBGFX PRIVATE /Zc:__cplusplus /Zc:preprocessor)
target_include_directories(TRenderBGFX PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(TRenderBGFX
    PUBLIC
        tclient_dxsdk
    PRIVATE
        tclient_options
        bgfx
        bimg_decode
        bx
        gdiplus
)

add_library(EngineLib STATIC ${EngineLib_SOURCES})
target_compile_definitions(EngineLib PRIVATE WIN32 NDEBUG _WINDOWS TCLIENT_BGFX)
target_compile_options(EngineLib PRIVATE /GS-)
target_include_directories(EngineLib
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${TCLIENT_LIB_3RD}/zlib/include"
    PRIVATE
        "${TCLIENT_LIB_OWN}/GameConfig"
        "${TCLIENT_LIB_3RD}/miniaudio"
        # TComp and Engine Lib include each other's headers.
        "${TCLIENT_LIB_OWN}/TComp/TComp"
)
target_link_directories(EngineLib PUBLIC "${TCLIENT_LIB_3RD}/zlib/lib/${TCLIENT_ARCH}")
target_link_libraries(EngineLib
    PUBLIC
        tclient_dxsdk
        TCML
        zlibstatic
        SDL3::SDL3-static
    PRIVATE
        tclient_options
        TRenderBGFX
)
