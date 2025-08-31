#!/usr/bin/env bash
set -euo pipefail

# Build helper to invoke VS2022 x64 Native Tools from WSL2 and run
# `cmake --build build-ninja -j` in the Windows repo path.
#
# Usage (run inside this repo under WSL2):
#   scripts/build-win-from-wsl.sh [--verbose]
#
# Notes:
# - This does not reconfigure CMake. Ensure `build-ninja` already exists.
# - It locates VS2022 via vswhere, calls VsDevCmd, then builds.

verbose=false
if [[ "${1:-}" == "--verbose" ]]; then
  verbose=true
  shift || true
fi

# Resolve repository root and convert to a Windows path
REPO_WSL_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
REPO_WIN_ROOT="$(wslpath -w "$REPO_WSL_ROOT")"

# Compose the command that runs under a single cmd.exe session so that
# VsDevCmd environment persists into the build step.
read -r -d '' CMD_BODY <<'PSCMDS' || true
$vs = $null
try {
  $vs = & "$Env:ProgramFiles(x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath
} catch {}
if (-not $vs) {
  $candidates = @(
    "$Env:ProgramFiles\Microsoft Visual Studio\2022\Community",
    "$Env:ProgramFiles\Microsoft Visual Studio\2022\Professional",
    "$Env:ProgramFiles\Microsoft Visual Studio\2022\Enterprise"
  )
  foreach ($c in $candidates) { if (Test-Path $c) { $vs = $c; break } }
}
if (-not $vs) { Write-Error "Visual Studio 2022 not found (vswhere/default paths)"; exit 9001 }
$bat = '"' + $vs + '\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64'
$cd  = 'cd /d "${REPO_WIN_ROOT}"'
$log = '"${REPO_WIN_ROOT}\\build-ninja\\wsl_build.log"'
$cm  = 'cmake --build build-ninja -j 1 > ' + $log + ' 2>&1'
if ('${VERBOSE}' -eq '1') { $cm = 'cmake --build build-ninja -v -j 1 > ' + $log + ' 2>&1' }
$line = 'call ' + $bat + ' && ' + $cd + ' && ' + $cm
cmd.exe /c $line
Write-Output "`n--- tail: build-ninja\\wsl_build.log (last 200 lines) ---"
Get-Content -Tail 200 -Path "${REPO_WIN_ROOT}\build-ninja\wsl_build.log"
exit $LASTEXITCODE
PSCMDS

# Inject variables and invoke PowerShell
pwsh_args=(
  powershell.exe -NoProfile -ExecutionPolicy Bypass -Command
)

VERBOSE_FLAG="0"
"$verbose" && VERBOSE_FLAG="1"

# Replace placeholders in the PS script
PS_SCRIPT="${CMD_BODY//\$\{REPO_WIN_ROOT\}/$REPO_WIN_ROOT}"
PS_SCRIPT="${PS_SCRIPT//\$\{VERBOSE\}/$VERBOSE_FLAG}"

"${pwsh_args[@]}" "$PS_SCRIPT"
