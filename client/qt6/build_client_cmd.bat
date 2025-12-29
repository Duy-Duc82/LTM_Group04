@echo off
REM Script tự động build client trên Windows CMD
REM Sử dụng: build_client_cmd.bat

echo === Building Millionaire Client ===
echo.

REM Chuyển đến thư mục client
cd /d "%~dp0"
cd ..

REM Tìm đường dẫn Qt
echo Finding Qt installation...
set QT_PATH=
if exist "C:\Qt\6.10.1\mingw_64\bin\qmake.exe" (
    set QT_PATH=C:\Qt\6.10.1\mingw_64
    echo [FOUND] %QT_PATH%
) else if exist "C:\Qt\6.10.0\mingw_64\bin\qmake.exe" (
    set QT_PATH=C:\Qt\6.10.0\mingw_64
    echo [FOUND] %QT_PATH%
) else if exist "C:\Qt\6.9.2\mingw_64\bin\qmake.exe" (
    set QT_PATH=C:\Qt\6.9.2\mingw_64
    echo [FOUND] %QT_PATH%
) else (
    echo [ERROR] Qt6 not found in common paths
    echo Please run find_qt_path.bat to find Qt installation
    pause
    exit /b 1
)

REM Thêm MinGW vào PATH
set MINGW_PATH=%QT_PATH%\bin
echo.
echo Adding MinGW to PATH: %MINGW_PATH%
set PATH=%MINGW_PATH%;%PATH%

REM Kiểm tra compiler
echo.
echo Checking compiler...
gcc --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] gcc not found! MinGW may not be installed correctly.
    echo Please check: %MINGW_PATH%\gcc.exe
    pause
    exit /b 1
)
gcc --version | findstr "gcc"
echo [OK] Compiler found

REM Tạo thư mục build
echo.
echo Creating build directory...
if not exist build mkdir build
cd build

REM Xóa cache CMake nếu có
if exist CMakeCache.txt (
    echo.
    echo Removing old CMake cache...
    del CMakeCache.txt
)
if exist CMakeFiles (
    echo Removing CMakeFiles directory...
    rmdir /s /q CMakeFiles
)

REM Cấu hình CMake
echo.
echo Configuring CMake...
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="%QT_PATH%"
if errorlevel 1 (
    echo [ERROR] CMake configuration failed!
    echo.
    echo Troubleshooting:
    echo 1. Check Qt path: %QT_PATH%
    echo 2. Check MinGW path: %MINGW_PATH%
    echo 3. Try: gcc --version
    pause
    exit /b 1
)

REM Build
echo.
echo Building project...
cmake --build . --config Release
if errorlevel 1 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo === Build Complete ===
echo.
echo Executable location:
if exist "Desktop_Qt_6_10_1_MinGW_64_bit-Release\MillionaireClient.exe" (
    echo   Desktop_Qt_6_10_1_MinGW_64_bit-Release\MillionaireClient.exe
) else if exist "Release\MillionaireClient.exe" (
    echo   Release\MillionaireClient.exe
) else (
    echo   (Check build directory)
)

echo.
echo Next steps:
echo 1. Run: powershell -ExecutionPolicy Bypass -File deploy_qt.ps1
echo 2. Run: MillionaireClient.exe
echo.
pause

