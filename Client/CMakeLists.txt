# CMake build of the game client. It builds the same six projects as TClient.sln
# (TClient, Engine Lib, TChart, TCML, TComp, HwidLib). See BUILD-CMAKE.md.

cmake_minimum_required(VERSION 3.25)
project(TClient LANGUAGES C CXX RC)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "The client is 32-bit only for now: configure with an x86 toolchain (-A Win32 or an x86 developer shell).")
endif()

include(cmake/TClientCommon.cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# bgfx, bx and bimg for the bgfx render backend (Lib/3rdParty/bgfx.cmake/VERSION.txt).
# Only the libraries: the shader compiler is built on demand with TCLIENT_BUILD_SHADERC.
option(TCLIENT_BUILD_SHADERC "Also build bgfx's shaderc, to recompile the render shaders" OFF)
set(BGFX_BUILD_TOOLS ${TCLIENT_BUILD_SHADERC} CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_BIN2C OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_GEOMETRY OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_TEXTURE OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
set(BGFX_CUSTOM_TARGETS OFF CACHE BOOL "" FORCE)
set(BGFX_CONFIG_VIDEO OFF CACHE BOOL "" FORCE)
# The engine calls the render layer from its main thread and its loader thread itself.
set(BGFX_CONFIG_MULTITHREADED OFF CACHE BOOL "" FORCE)
add_subdirectory("${TCLIENT_LIB_3RD}/bgfx.cmake" bgfx EXCLUDE_FROM_ALL)
# bimg's ASTC encoder uses _mm_popcnt_u64 when clang defines __POPCNT__; it does not exist in
# 32-bit code.
target_compile_definitions(bimg PRIVATE ASTCENC_POPCNT=0)
# renderer_webgpu.cpp narrows 64-bit sizes to size_t, an error for clang in 32-bit code.
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(bgfx PRIVATE -Wno-c++11-narrowing)
endif()

add_subdirectory("${TCLIENT_LIB_OWN}/TCML/TCML" TCML)
add_subdirectory("${TCLIENT_LIB_OWN}/Engine Lib/Engine Lib" EngineLib)
add_subdirectory("${TCLIENT_LIB_OWN}/TComp/TComp" TComp)
add_subdirectory("${TCLIENT_LIB_OWN}/TChart/TChart" TChart)
add_subdirectory("${TCLIENT_LIB_OWN}/HwidLib/HwidLib" HwidLib)
add_subdirectory(TClient)

# Copy TClient.exe into Game/ after each build, like the MSBuild project (OutDir=..\..\Game\).
option(TCLIENT_DEPLOY_TO_GAME "Copy TClient.exe into the Game folder after building" OFF)
if(TCLIENT_DEPLOY_TO_GAME)
    add_custom_command(TARGET TClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:TClient>" "${TCLIENT_ROOT}/Game/TClient.exe"
        COMMENT "Deploying TClient.exe to Game/")
endif()
