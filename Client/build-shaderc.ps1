# Builds bgfx's shader compiler (shaderc) from the vendored bgfx.cmake into build\shaderc, then
# optionally recompiles the bgfx backend's shaders into Lib\Own\Engine Lib\Engine Lib\TRenderShaders.h.
#   .\Client\build-shaderc.ps1              # build shaderc only
#   .\Client\build-shaderc.ps1 -Compile     # build shaderc and recompile the shaders
# shaderc is a 64-bit host tool, independent of the 32-bit game build.
param(
    [switch]$Compile
)
$ErrorActionPreference = "Stop"

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw "Visual Studio with the C++ x86/x64 tools was not found." }

$env:PATH = "$(Split-Path $vswhere);$env:PATH"
Import-Module "$vs\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath $vs -SkipAutomaticLocation -DevCmdArguments "-arch=x64 -host_arch=x64" | Out-Null

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$src = Join-Path $root "Lib\3rdParty\bgfx.cmake"
$bin = Join-Path $root "build\shaderc"

cmake -S $src -B $bin -G Ninja -DCMAKE_BUILD_TYPE=Release `
    -DBGFX_BUILD_EXAMPLES=OFF -DBGFX_BUILD_TOOLS=ON -DBGFX_BUILD_TOOLS_SHADER=ON `
    -DBGFX_BUILD_TOOLS_GEOMETRY=OFF -DBGFX_BUILD_TOOLS_TEXTURE=OFF -DBGFX_BUILD_TOOLS_BIN2C=OFF `
    -DBGFX_INSTALL=OFF -DBGFX_CUSTOM_TARGETS=OFF
if ($LASTEXITCODE) { throw "CMake configure failed." }

# The post-build step then copies dxcompiler.dll from bgfx/tools/bin, which the vendored copy
# leaves out: it is only needed for D3D12 DXIL shaders, so its failure is not an error.
cmake --build $bin --target shaderc
$shaderc = Get-ChildItem $bin -Recurse -Filter shaderc.exe | Select-Object -First 1
if (-not $shaderc) { throw "shaderc build failed." }
Write-Host "shaderc: $($shaderc.FullName)"

if ($Compile) {
    cmake "-DSHADERC=$($shaderc.FullName)" -P (Join-Path $root "Lib\Own\Engine Lib\Engine Lib\shaders\CompileShaders.cmake")
    if ($LASTEXITCODE) { throw "Shader compilation failed." }
}
