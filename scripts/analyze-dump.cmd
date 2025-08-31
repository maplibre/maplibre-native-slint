@echo off
setlocal ENABLEDELAYEDEXPANSION

REM Analyze a crash dump with cdb and write a clean log (no copy/paste hassles).
REM Usage:
REM   scripts\analyze-dump.cmd                      (fast local-symbols, analyzes latest dumps\*.dmp)
REM   scripts\analyze-dump.cmd <dump_path>          (fast local-symbols)
REM   scripts\analyze-dump.cmd --full [dump_path]   (full symbols via MS server + build-ninja)
REM   scripts\analyze-dump.cmd --mini [dump_path]   (very quick scan, minimal output)

REM Resolve repository root (this script is under scripts/)
set "REPO=%~dp0.."
for %%G in ("%REPO%") do set "REPO=%%~fG"

REM Prefer the 64-bit Console Debugger (cdb)
set "CDB="
for %%P in (
  "%ProgramFiles(x86)%\Windows Kits\11\Debuggers\x64\cdb.exe"
  "%ProgramFiles%\Windows Kits\11\Debuggers\x64\cdb.exe"
  "%ProgramFiles(x86)%\Windows Kits\10\Debuggers\x64\cdb.exe"
  "%ProgramFiles%\Windows Kits\10\Debuggers\x64\cdb.exe"
) do (
  if exist %%~P (
    set "CDB=%%~P"
    goto :cdb_found
  )
)
for /f "delims=" %%F in ('where cdb 2^>NUL') do if not defined CDB set "CDB=%%~F"

:cdb_found
if not defined CDB (
  echo [error] cdb.exe not found. Install "Debugging Tools for Windows" via VS Installer.
  exit /b 1
)

REM Parse args (optional --full)
set "FULL=0"
set "MINI=0"
if /I "%~1"=="--full" (
  set "FULL=1"
  shift
)
if /I "%~1"=="--mini" (
  set "MINI=1"
  shift
)

REM Pick dump file
set "DUMP=%~1"
if not defined DUMP (
  if exist "%REPO%\dumps\*.dmp" (
    for /f "delims=" %%F in ('dir /b /a:-d /o:-d "%REPO%\dumps\*.dmp"') do (
      set "DUMP=%REPO%\dumps\%%F"
      goto :dump_ok
    )
  )
)
:dump_ok
if not defined DUMP (
  echo [error] No .dmp specified and no dumps found under %REPO%\dumps
  exit /b 2
)
if not exist "%DUMP%" (
  echo [error] Dump not found: %DUMP%
  exit /b 3
)

REM Timestamp for log file
for /f %%t in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd_HHmmss"') do set "TS=%%t"
if not defined TS set "TS=now"
if not exist "%REPO%\dumps" mkdir "%REPO%\dumps" >NUL 2>&1
set "LOG=%REPO%\dumps\analyze_%TS%.txt"

REM Ensure symbol cache folder exists
if not exist "C:\symbols" mkdir "C:\symbols" >NUL 2>&1

REM Compose debugger command script (write to file to avoid '!' expansion issues)
set "CMDS=%REPO%\dumps\dbgcmds_%TS%.txt"
if "%MINI%"=="1" (
  (
    echo .symfix
    echo .sympath srv*c:\symbols
    echo .reload
    echo .exr -1
    echo .ecxr
    echo r
    echo ln @rip
    echo u @rip-32 L80
    echo kv 40
    echo lm m opengl32
    echo lm m nv*gl*
    echo q
  ) > "%CMDS%"
) else if "%FULL%"=="1" (
  (
    echo .symfix
    echo .sympath+ srv*c:\symbols*https://msdl.microsoft.com/download/symbols
    echo .sympath+ "%REPO%\build-ninja"
    echo .reload /f
    echo .exr -1
    echo .ecxr
    echo r
    echo ln @rip
    echo u @rip-40 L100
    echo kv 200
    echo !analyze -v
    echo .echo [loaded GL modules]
    echo lm m opengl32
    echo lm m nv*gl*
    echo lm m ig9icd64
    echo lm m amd*ogl*
    echo .echo [suspect exports]
    echo x opengl32!*Swap*
    echo x opengl32!*wgl*
    echo .echo [threads top15]
    echo ~* kP 15
    echo .echo [end]
    echo q
  ) > "%CMDS%"
) else (
  REM Fast path: local symbols only, targeted module reloads to avoid stalls
  (
    echo .symfix
    echo .sympath srv*c:\symbols
    echo .reload /f opengl32
    echo .reload /f ntdll
    echo .reload /f kernel32
    echo .reload /f user32
    echo .exr -1
    echo .ecxr
    echo r
    echo ln @rip
    echo u @rip-40 L100
    echo kv 200
    echo !analyze -v
    echo .echo [loaded GL modules]
    echo lm m opengl32
    echo lm m nv*gl*
    echo lm m ig9icd64
    echo lm m amd*ogl*
    echo .echo [suspect exports]
    echo x opengl32!*Swap*
    echo x opengl32!*wgl*
    echo .echo [end]
    echo q
  ) > "%CMDS%"
)

echo [info] CDB:   "%CDB%"
echo [info] Dump:  "%DUMP%"
echo [info] Log :  "%LOG%"
echo [info] Cmds:  "%CMDS%"

"%CDB%" -z "%DUMP%" -logo "%LOG%" -cf "%CMDS%"
set "ERR=%ERRORLEVEL%"
if not "%ERR%"=="0" (
  echo [warn] cdb exited with code %ERR%. Review the log for details.
)
echo.
echo [done] Wrote analysis to: "%LOG%"
exit /b %ERR%
