# Script để xóa CMakeCache và CMakeFiles (PowerShell)
# Sử dụng: .\clear_cmake_cache.ps1

Write-Host "=== Clearing CMake Cache ===" -ForegroundColor Cyan
Write-Host ""

$buildDir = "build"

if (-not (Test-Path $buildDir)) {
    Write-Host "[INFO] Build directory does not exist: $buildDir" -ForegroundColor Yellow
    exit 0
}

cd $buildDir

# Xóa CMakeCache.txt
if (Test-Path "CMakeCache.txt") {
    Remove-Item "CMakeCache.txt" -Force
    Write-Host "✓ Removed CMakeCache.txt" -ForegroundColor Green
} else {
    Write-Host "- CMakeCache.txt not found" -ForegroundColor Gray
}

# Xóa CMakeFiles directory
if (Test-Path "CMakeFiles") {
    Remove-Item "CMakeFiles" -Recurse -Force
    Write-Host "✓ Removed CMakeFiles directory" -ForegroundColor Green
} else {
    Write-Host "- CMakeFiles directory not found" -ForegroundColor Gray
}

Write-Host ""
Write-Host "=== Done ===" -ForegroundColor Green

