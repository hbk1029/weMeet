@echo off
set PATH=D:\dev\Qt\5.15.2\mingw81_64\bin;D:\dev\Qt\5.15.2\mingw81_64\lib;D:\dev\Qt\Tools\mingw810_64\bin;%PATH%
D:\dev\projects\cpp\qt\demo\WeMeetClient\build\Desktop_Qt_5_15_2_MinGW_64_bit-Debug\debug\WeMeetClient.exe > D:\dev\projects\cpp\qt\demo\WeMeetClient\stdout.txt 2> D:\dev\projects\cpp\qt\demo\WeMeetClient\stderr.txt
echo EXIT CODE: %ERRORLEVEL%
