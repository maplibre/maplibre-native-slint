@echo off
setlocal

rem Resolve Visual Studio 2022 installation (prefer Community)
set "VSROOT="
for /f "usebackq delims=" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products Microsoft.VisualStudio.Product.Community -property installationPath`) do set "VSROOT=%%I"
if not defined VSROOT for /f "usebackq delims=" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath`) do set "VSROOT=%%I"
if not defined VSROOT (
  echo [ERROR] Visual Studio 2022 not found.
  exit /b 1
)

call "%VSROOT%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 || exit /b %errorlevel%
cd /d C:\Users\yuiseki\src\maplibre-native-slint || exit /b %errorlevel%

rem Align vcpkg env with project
set VCPKG_ROOT=
set VCPKG_OVERLAY_TRIPLETS=%cd%\vendor\maplibre-native\platform\windows\vendor\vcpkg-custom-triplets
set SKIA_FEATURES=gl,skshaper,skparagraph,icu

cmake -S . -B build-ninja -G "Ninja" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DMLNS_WITH_SLINT_GL=ON ^
  -DMLNS_FORCE_FETCH_SLINT=ON ^
  -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DVCPKG_HOST_TRIPLET=x64-windows || exit /b %errorlevel%

cmake --build build-ninja -v -j
exit /b %errorlevel%
