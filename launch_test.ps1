$env:Path = "D:\dev\Qt\5.15.2\mingw81_64\bin;D:\dev\Qt\5.15.2\mingw81_64\lib;D:\dev\Qt\Tools\mingw810_64\bin;" + $env:Path
$p = Start-Process "D:\dev\projects\cpp\qt\demo\WeMeetClient\build\Desktop_Qt_5_15_2_MinGW_64_bit-Debug\debug\WeMeetClient.exe" -WorkingDirectory "D:\dev\projects\cpp\qt\demo\WeMeetClient\build\Desktop_Qt_5_15_2_MinGW_64_bit-Debug\debug" -PassThru -Wait -NoNewWindow -RedirectStandardError "D:\dev\projects\cpp\qt\demo\WeMeetClient\err.txt" -RedirectStandardOutput "D:\dev\projects\cpp\qt\demo\WeMeetClient\out.txt"
$p.ExitCode
