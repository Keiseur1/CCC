@echo off
setlocal

:: Detect Qt and MinGW path
if exist "C:\msys64\ucrt64\bin\qmake6.exe" (
    set "PATH=C:\msys64\ucrt64\bin;%PATH%"
    set "QMAKE_CMD=qmake6"
) else if exist "C:\Qt\6.11.2\mingw_64\bin\qmake.exe" (
    set "PATH=C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.11.2\mingw_64\bin;%PATH%"
    set "QMAKE_CMD=qmake"
) else (
    set "QMAKE_CMD=qmake"
)

set "BUILD_DIR=%~dp0build"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%" || exit /b 1

echo [build.bat] Running %QMAKE_CMD%...
%QMAKE_CMD% "%~dp0DijkstraMap.pro" -spec win32-g++ || exit /b 1

echo [build.bat] Compiling...
mingw32-make -j4
if errorlevel 1 (
    echo [build.bat] Build failed!
    exit /b 1
)

echo [build.bat] Done!
endlocal
