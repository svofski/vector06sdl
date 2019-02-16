set V06X=%~dp0
set S=%V06X%scripts
%V06X%v06x.exe --script %S%\rkload.chai --script %S%\musload.chai --scriptargs %V06X%\MSVec.rk --scriptargs %1 --rom %V06X%\micro_rk.rom