@echo off
set mode=debug
if /i $%1 equ $release set mode=release
if not exist bin\%mode% exit /b 0
pushd bin\%mode%
call Sapling.exe
popd