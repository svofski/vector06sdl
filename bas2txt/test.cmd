@echo off
del/q testwork\*.* 2>nul
rmdir testwork 2>nul
mkdir testwork 2>nul

call :runtest bochka
call :runtest reklama
call :runtest blokigr1
call :runtest kalah
call :runtest uglimen

goto :eof

:runtest
setlocal
bas2asc.py testdata\%1.bas testwork\%1.asc testwork\%1.bas
fc /b testdata\%1.bas testwork\%1.bas >testwork\%1.diff
if ERRORLEVEL 1 (
	echo %1 ERROR
	exit /b %errorlevel%
)
if ERRORLEVEL 0 (
	echo %1 PASS
)
endlocal

:eof