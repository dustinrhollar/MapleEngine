@echo off
setlocal EnableDelayedExpansion

:: Project directory
SET HOST_DIR=%~dp0
SET HOST_DIR=%HOST_DIR:~0,-1%

:: General Flags and whatnot

SET debug_flags=/Od /Z7 /MTd /D_DEBUG
SET release_flags=/O2 /GL /MT /analyze- /D NDEBUG 
SET linker_flags=/INCREMENTAL:no /NOLOGO /SUBSYSTEM:WINDOWS user32.lib shell32.lib Winmm.lib ole32.lib Gdi32.lib
SET common_flags=/W3 /Gm- /EHsc /nologo /I..\..\Editor\Ext

SET imgui_main=..\..\Editor\Ext\imgui\single_file\imgui_single_file.cpp
SET imnodes_main=..\..\Editor\Ext\imgui-node-editor\single_file\imnodes_single_file.cpp

SET imgui_obj=imgui_single_file.obj
SET imnodes_obj=imnodes_single_file.obj

SET output_main=/OUT:imgui.lib

REM Run the build tools, but only if they aren't set up already.

cl >nul 2>nul
if %errorlevel% neq 9009 goto :build
echo Running VS build tool setup.
echo Initializing MS build tools...
call scripts\setup_cl.bat
cl >nul 2>nul
if %errorlevel% neq 9009 goto :build
echo Unable to find build tools! Make sure that you have Microsoft Visual Studio 10 or above installed!
exit /b 1

:build

SET mode=debug
IF /i $%1 equ $release (set mode=release)
IF %mode% equ debug (
	SET flags=%common_flags% %debug_flags%
) else (
	SET flags=%common_flags% %release_flags%
)

echo Building in %mode% mode.

IF NOT EXIST ..\bin\%mode% mkdir bin\%mode%

pushd ..\bin\%mode%

echo.     -Compiling ImGui Library:
	call cl /D_IMGUI_IMPLEMENTATION /c %flags% %imgui_main% /link %linker_flags% /DLL
	if %errorlevel% neq 0 (
		echo Error during compilation!
		popd
		goto :fail
	)
	
	call cl /D_IMGUI_NODE_IMPLEMENTATION /c %flags% %imnodes_main% /link %linker_flags% /DLL
	if %errorlevel% neq 0 (
		echo Error during compilation!
		popd
		goto :fail
	)
	
	call lib %imgui_obj% %imnodes_obj% %output_main%
	if %errorlevel% neq 0 (
		echo Error during compilation!
		popd
		goto :fail
	)
popd

:complete
echo Build complete!
exit /b 0

:fail
echo Build failed!
exit /b %errorlevel%