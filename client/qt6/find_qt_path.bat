@echo off
REM Script để tìm đường dẫn Qt6 trên Windows
REM Sử dụng: find_qt_path.bat

echo === Tìm đường dẫn Qt6 ===
echo.

REM Kiểm tra các đường dẫn phổ biến
set FOUND=0

echo Checking common Qt installation paths...
echo.

if exist "C:\Qt\6.10.1\mingw_64\bin\qmake.exe" (
    echo [FOUND] C:\Qt\6.10.1\mingw_64
    echo   Use this path: C:\Qt\6.10.1\mingw_64
    set FOUND=1
    goto :found
)

if exist "C:\Qt\6.10.0\mingw_64\bin\qmake.exe" (
    echo [FOUND] C:\Qt\6.10.0\mingw_64
    echo   Use this path: C:\Qt\6.10.0\mingw_64
    set FOUND=1
    goto :found
)

if exist "C:\Qt\6.9.2\mingw_64\bin\qmake.exe" (
    echo [FOUND] C:\Qt\6.9.2\mingw_64
    echo   Use this path: C:\Qt\6.9.2\mingw_64
    set FOUND=1
    goto :found
)

if exist "C:\Qt\6.10.1\msvc2022_64\bin\qmake.exe" (
    echo [FOUND] C:\Qt\6.10.1\msvc2022_64
    echo   Use this path: C:\Qt\6.10.1\msvc2022_64
    set FOUND=1
    goto :found
)

REM Kiểm tra trong Program Files
if exist "%ProgramFiles%\Qt\6.10.1\mingw_64\bin\qmake.exe" (
    echo [FOUND] %ProgramFiles%\Qt\6.10.1\mingw_64
    echo   Use this path: %ProgramFiles%\Qt\6.10.1\mingw_64
    set FOUND=1
    goto :found
)

:found
if %FOUND%==0 (
    echo [NOT FOUND] Qt6 not found in common paths
    echo.
    echo Please check:
    echo   1. Qt6 is installed
    echo   2. Qt6 is installed in a non-standard location
    echo.
    echo To find Qt6 manually:
    echo   dir C:\Qt\ /s /b | findstr qmake.exe
    echo.
    echo Or check Qt Maintenance Tool for installation path
) else (
    echo.
    echo === Usage ===
    echo When running cmake, use:
    echo   cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="<path_above>"
)

pause

