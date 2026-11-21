@echo off
setlocal DisableDelayedExpansion

rem End the batch context so CMD can finish after PowerShell deletes this file.
(goto) 2>nul & (
    cd /d "%SystemRoot%"
    "%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Uninstall.ps1" %*
    if errorlevel 1 (
        pause
        exit /b 1
    )
    pause
    exit /b 0
)
