@echo off
setlocal enabledelayedexpansion
REM Resolve preferred VS2022 (prefer Community, then Professional, Enterprise, then any)
set "VSROOT="
for /f "usebackq delims=" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products Microsoft.VisualStudio.Product.Community -property installationPath`) do set "VSROOT=%%I"
if not defined VSROOT for /f "usebackq delims=" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products Microsoft.VisualStudio.Product.Professional -property installationPath`) do set "VSROOT=%%I"
if not defined VSROOT for /f "usebackq delims=" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products Microsoft.VisualStudio.Product.Enterprise -property installationPath`) do set "VSROOT=%%I"
if not defined VSROOT for /f "usebackq delims=" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath`) do set "VSROOT=%%I"
if not defined VSROOT (
  echo [ERROR] Visual Studio 2022 not found.
  exit /b 9001
)

(
  call "%VSROOT%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
  echo [INFO] VS path: %VSROOT%
  ver
  echo [INFO] After VsDevCmd, trying to locate tools...
  where cl
  echo [INFO] VCToolsInstallDir=%VCToolsInstallDir%
  echo [INFO] VisualStudioVersion=%VisualStudioVersion%
  echo [INFO] WindowsSDKDir=%WindowsSDKDir%
  cd /d C:\Users\yuiseki\src\maplibre-native-slint

  REM Disable VS-bundled vcpkg and set overlay triplets per project
  set "VCPKG_ROOT="
  set "VCPKG_OVERLAY_TRIPLETS=%cd%\vendor\maplibre-native\platform\windows\vendor\vcpkg-custom-triplets"
  REM Constrain skia-bindings features to avoid Direct3D wrappers (vcpkg skia lacks D3D)
  set "SKIA_FEATURES=gl,skshaper,skparagraph,icu"

  REM Clear cross/toolchain vars that may leak from parent env (breaks MSVC detection)
  for %%V in (CC CXX CFLAGS CXXFLAGS CPPFLAGS LDFLAGS AR LD RC STRIP RANLIB OBJC OBJCXX) do set "%%V="

  REM Verify compilers and ninja
  where cl || (
    echo [ERROR] cl.exe not found. VsDevCmd may have failed.
    exit /b 9009
  )

  where ninja >nul 2>&1 || (
    set "NINJA_EXE=%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
    if exist "%NINJA_EXE%" set "CMAKE_MAKE_FLAG=-DCMAKE_MAKE_PROGRAM=%NINJA_EXE%"
  )

  REM Clean as instructed
  if exist build-ninja rmdir /s /q build-ninja

  REM Configure with Ninja (vcpkg toolchain specified)
  cmake -S . -B build-ninja -G "Ninja" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DMLNS_WITH_SLINT_GL=ON ^
    -DMLNS_FORCE_FETCH_SLINT=ON ^
    -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DVCPKG_HOST_TRIPLET=x64-windows ^
    %CMAKE_MAKE_FLAG%
  if errorlevel 1 exit /b !errorlevel!

  REM Build as instructed
  cmake --build build-ninja -v -j
  set "_ret=!errorlevel!"
  if not "%_ret%"=="0" exit /b %_ret%

  REM Auto-run the example with GL zero-copy if requested (default ON)
  if not defined MLNS_AUTO_RUN set "MLNS_AUTO_RUN=1"
  if "%MLNS_AUTO_RUN%"=="1" (
    pushd build-ninja
    echo [auto-run] Launching maplibre-slint-example with MLNS_USE_GL=1 (SLINT_RENDERER=gl)
    start "maplibre-slint-example" cmd /C "set MLNS_USE_GL=1& set SLINT_RENDERER=gl& maplibre-slint-example.exe"
    popd
  )
  exit /b 0
)
