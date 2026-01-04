@echo off
REM Script để xóa CMakeCache và CMakeFiles (CMD)
REM Sử dụng: clear_cmake_cache.bat

echo === Clearing CMake Cache ===
echo.

if not exist build (
    echo [INFO] Build directory does not exist: build
    pause
    exit /b 0
)

cd build

REM Xóa CMakeCache.txt
if exist CMakeCache.txt (
    del CMakeCache.txt
    echo [OK] Removed CMakeCache.txt
) else (
    echo [-] CMakeCache.txt not found
)

REM Xóa CMakeFiles directory
if exist CMakeFiles (
    rmdir /s /q CMakeFiles
    echo [OK] Removed CMakeFiles directory
) else (
    echo [-] CMakeFiles directory not found
)

echo.
echo === Done ===
pause

