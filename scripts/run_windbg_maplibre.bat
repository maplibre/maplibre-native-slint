@echo off
setlocal ENABLEDELAYEDEXPANSION
REM Run maplibre-slint-example under WinDbg/WinDbgX and auto-capture crash info (Windows 10/11)

REM Resolve repository root (this script may be invoked from scripts/)
set "REPO=%~dp0.."
for %%# in ("%REPO%") do set "REPO=%%~f#"
pushd "%REPO%" >NUL
REM Target app under repo root
set "APP_EXE=%REPO%\build-ninja\maplibre-slint-example.exe"
set "DBGEXE="
for %%P in (
  "%ProgramFiles%\Windows Kits\11\Debuggers\x64\windbgx.exe"
  "%ProgramFiles(x86)%\Windows Kits\11\Debuggers\x64\windbgx.exe"
  "%ProgramFiles%\Windows Kits\10\Debuggers\x64\windbgx.exe"
  "%ProgramFiles(x86)%\Windows Kits\10\Debuggers\x64\windbgx.exe"
  "%ProgramFiles%\Windows Kits\11\Debuggers\x64\windbg.exe"
  "%ProgramFiles(x86)%\Windows Kits\11\Debuggers\x64\windbg.exe"
  "%ProgramFiles%\Windows Kits\10\Debuggers\x64\windbg.exe"
  "%ProgramFiles(x86)%\Windows Kits\10\Debuggers\x64\windbg.exe"
  "%ProgramFiles%\Windows Kits\11\Debuggers\x64\cdb.exe"
  "%ProgramFiles(x86)%\Windows Kits\11\Debuggers\x64\cdb.exe"
  "%ProgramFiles%\Windows Kits\10\Debuggers\x64\cdb.exe"
  "%ProgramFiles(x86)%\Windows Kits\10\Debuggers\x64\cdb.exe"
) do (
  if exist %%~P (
    set "DBGEXE=%%~P"
    goto :dbg_found
  )
)

REM Fallback: try locating via WHERE (might be slower)
for %%D in (windbgx.exe windbg.exe cdb.exe) do (
  for /f "delims=" %%F in ('where %%D 2^>NUL') do (
    if not defined DBGEXE set "DBGEXE=%%~F"
  )
)

:dbg_found
if not defined DBGEXE (
  echo [error] Debugger not found. Install "Windows SDK - Debugging Tools for Windows".
  echo Tips: In Visual Studio Installer, add "Windows 11/10 SDK" and "Debugging Tools".
  exit /b 1
)

if not exist "%APP_EXE%" (
  echo [error] App not found: %APP_EXE%
  echo Build first (e.g. scripts\_reconfigure-win.cmd or scripts\_build-ninja-win.cmd)
  popd >NUL
  exit /b 2
)

REM Environment for reproducible crash (tune if needed)
set "SLINT_RENDERER=gl"
set "MLNS_FORCE_DESKTOP_GL=1"
REM Guard + isolate mode from args
REM   usage: run_windbg_maplibre.bat [iso|noiso]
set "_ISO=%1"
if /i "%_ISO%"=="iso" (
  set "MLNS_GL_ISOLATE_CONTEXT=1"
) else (
  set "MLNS_GL_ISOLATE_CONTEXT=0"
)
REM Use minimal safe-mode to keep renderer functional, and enable vptr guard
set "MLNS_GL_SAFE_MODE=1"
set "MLNS_GL_VPTR_GUARD=1"

REM WinDbg startup commands:
REM  - set symbol path, open timestamped log
REM  - break on AV (0xC0000005) and 0xC000041D with an analysis macro, then quit
set "WDBGCMD=.symfix; .sympath+ srv*c:\symbols*https://msdl.microsoft.com/download/symbols; .sympath+ %REPO%\build-ninja; .reload;"
set "WDBGCMD=%WDBGCMD% .logopen /t %REPO%\windbg_maplibre.log; sxd gp;"
set "WDBGCMD=%WDBGCMD% sxe -c \".printf --- EX AV ---\n; .time; !gle; .exr -1; .ecxr; kv 200; !analyze -v; .dump /ma %REPO%\windbg_maplibre.dmp; .logclose; q\" av;"
set "WDBGCMD=%WDBGCMD% sxe -c \".printf --- EX 0xC000041D ---\n; .time; !gle; .exr -1; .ecxr; kv 200; !analyze -v; .dump /ma %REPO%\windbg_maplibre.dmp; .logclose; q\" 041D;"
set "WDBGCMD=%WDBGCMD% g"

echo [info] Launching debugger: "%DBGEXE%"
echo [info] App: "%APP_EXE%"
echo [info] Commands: %WDBGCMD%

"%DBGEXE%" -o -g -G -c "%WDBGCMD%" "%APP_EXE%"

popd >NUL

endlocal
