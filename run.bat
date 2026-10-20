@echo off
setlocal

:: Detect Qt bin path
if exist "C:\msys64\ucrt64\bin\qmake6.exe" (
    set "PATH=C:\msys64\ucrt64\bin;%PATH%"
) else if exist "C:\Qt\6.11.2\mingw_64\bin" (
    set "PATH=C:\Qt\6.11.2\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;%PATH%"
)

:find_exe
for %%F in (
    "%~dp0build\release\DijkstraMap.exe"
    "%~dp0build\debug\DijkstraMap.exe"
    "%~dp0build\Desktop_Qt_6_11_2_MinGW_64_bit_Debug\release\DijkstraMap.exe"
    "%~dp0build\Desktop_Qt_6_11_2_MinGW_64_bit_Debug\debug\DijkstraMap.exe"
    "%~dp0build\Desktop_Qt_6_11_2_MinGW_64_bit_Release\release\DijkstraMap.exe"
    "%~dp0release\DijkstraMap.exe"
    "%~dp0debug\DijkstraMap.exe"
) do (
    if exist "%%~fF" (
        echo [run.bat] Launching: %%~fF
        start "" "%%~fF"
        goto :eof
    )
)

echo [run.bat] ERROR: Khong tim thay DijkstraMap.exe
echo Hay chay build.bat truoc de bien dich!
pause
endlocal
