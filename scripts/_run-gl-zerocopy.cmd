@echo off
setlocal
set "REPO=C:\Users\yuiseki\src\maplibre-native-slint"
cd /d "%REPO%" || exit /b 1

REM Ensure app is built
if not exist build-ninja\maplibre-slint-example.exe (
  echo [build] Configuring + building (Ninja)
  call "%REPO%\_reconfigure-win.cmd" || exit /b %errorlevel%
)

echo [run] MLNS_USE_GL=1 SLINT_RENDERER=gl
set MLNS_USE_GL=1
set SLINT_RENDERER=gl
cd build-ninja || exit /b 1
start "maplibre-slint-example" maplibre-slint-example.exe
exit /b 0

