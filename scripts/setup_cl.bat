@echo off

set "lib="

set vc=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build
if not defined lib (if exist "%vc%" (call "%vc%\vcvarsall.bat" x64 >nul))

set vc=C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC
if not defined lib (if exist "%vc%" (call "%vc%\vcvarsall.bat" x64 >nul))

set vc=C:\Program Files (x86)\Microsoft Visual Studio 13.0\VC
if not defined lib (if exist "%vc%" (call "%vc%\vcvarsall.bat" x64 >nul))

set vc=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC
if not defined lib (if exist "%vc%" (call "%vc%\vcvarsall.bat" x64 >nul))

set vc=C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC
if not defined lib (if exist "%vc%" (call "%vc%\vcvarsall.bat" x64 >nul))

set vc=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC
if not defined lib (if exist "%vc%" (call "%vc%\vcvarsall.bat" x64 >nul))