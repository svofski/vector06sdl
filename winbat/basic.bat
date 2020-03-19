rem Launch a .BAS file using scripted hooks: basic.bat program.bas
rem Should be located in the bin directory of v06x.exe
set S=%~dp0
set D=%S%\..\scripts
set B=%S%\..\boot
%S%\v06x.exe --script %D%\bas25hook.chai --script %D%\robotnik.chai --script %D%\basload.chai --bootrom %B%\boot.bin --scriptargs %1
