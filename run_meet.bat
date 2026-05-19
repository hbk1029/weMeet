@echo off
set "QT_DIR=D:\dev\Qt\5.15.2\mingw81_64"
set "TOOLS_DIR=D:\dev\Qt\Tools\mingw810_64\bin"
set "EXE_DIR=D:\Projects\dev-projects\cpp\qt\demo\WeMeetClient\build\debug"
set PATH=%QT_DIR%\bin;%TOOLS_DIR%;%EXE_DIR%;%PATH%
cd /d "%EXE_DIR%"
start "" "%EXE_DIR%\WeMeetClient.exe"
