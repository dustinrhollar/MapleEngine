@echo off
if $%1==$rebuild (
    echo Rebuilding:
    call build.bat
    if %errorlevel% neq 0 (exit /b %errorlevel%)
)
if not exist bin\debug (
    echo Unable to find bin directory! Try building in debug mode first, or call debug with argument <rebuild>.
    exit /b 0
)
cl >nul 2>nul
if %errorlevel% neq 9009 goto :debug
echo Running VS build tool setup.
echo Initializing MS build tools...
call scripts\setup_cl.bat
cl >nul 2>nul
if %errorlevel% neq 9009 goto :debug
echo Unable to find build tools! Make sure that you have Microsoft Visual Studio 10 or above installed!
exit /b 1

:debug
pushd bin\debug
call devenv Reflection.exe
popd