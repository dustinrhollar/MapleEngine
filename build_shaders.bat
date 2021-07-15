@echo off
setlocal EnableDelayedExpansion

pushd data\shaders
call build.bat
popd

SET mode=debug
IF /i $%1 equ $release (set mode=release)
IF %mode% equ debug (
	SET flags=%common_flags% %input_main% %output_main% %debug_flags%
) else (
	SET flags=%common_flags% %input_main% %output_main% %release_flags%
)

REM Copy shaders, if they have changed.
echo.     -Copying Shaders:
if not exist bin\%mode%\shaders mkdir bin\%mode%\shaders
robocopy data\shaders bin\%mode%\shaders *.cso /s /purge /ns /nc /ndl /np /njh /njs /mt
del data\shaders\*.cso

REM Copy textures, if they have changed.
echo.     -Copying Textures:
if not exist bin\%mode%\textures mkdir bin\%mode%\textures
robocopy data\textures bin\%mode%\textures *.* /s /purge /ns /nc /ndl /np /njh /njs /mt
