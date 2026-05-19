@echo off
REM WeMeet network simulation launcher
setlocal
set TOOLS_DIR=%~dp0tools
set CLUMSY=%TOOLS_DIR%\clumsy\clumsy.exe

if not exist "%CLUMSY%" (
    echo [ERROR] clumsy.exe not found at %CLUMSY%
    echo Download: https://github.com/jagt/clumsy/releases/download/0.2/clumsy-0.2-win64.zip
    echo Extract clumsy.exe to test\tools\clumsy\
    pause
    exit /b 1
)

powershell -ExecutionPolicy Bypass -File "%~dp0net_sim.ps1" -ClumsyPath "%CLUMSY%"
