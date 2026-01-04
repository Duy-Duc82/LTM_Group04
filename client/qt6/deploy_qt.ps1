# Deploy Qt6 libraries for the built executable
# This script uses windeployqt to copy all necessary DLLs and plugins

Write-Host "=== Qt6 Deployment ===" -ForegroundColor Cyan
Write-Host ""

# Find Qt installation
$qtPaths = @(
    "C:\Qt\6.10.1\mingw_64\bin\windeployqt.exe",
    "C:\Qt\6.10.0\mingw_64\bin\windeployqt.exe",
    "C:\Qt\6.9.2\mingw_64\bin\windeployqt.exe",
    "C:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe"
)

$windeployqt = $null
foreach ($path in $qtPaths) {
    if (Test-Path $path) {
        $windeployqt = $path
        Write-Host "Found windeployqt at: $path" -ForegroundColor Green
        break
    }
}

if ($null -eq $windeployqt) {
    Write-Host "[ERROR] Could not find windeployqt.exe" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please install Qt6 or check your Qt installation path." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Installed Qt paths to check:" -ForegroundColor Cyan
    foreach ($path in $qtPaths) {
        Write-Host "  - $path" -ForegroundColor Gray
    }
    exit 1
}

$exePath = ".\build\Desktop_Qt_6_10_1_MinGW_64_bit-Debug\MillionaireClient.exe"
$deployDir = ".\build\Desktop_Qt_6_10_1_MinGW_64_bit-Debug"

Write-Host ""
Write-Host "Deploying Qt libraries..." -ForegroundColor Cyan

# Run windeployqt with QML support
# --qmldir specifies the QML source directory for QML applications
$qmlDir = ".\qml"
if (Test-Path $qmlDir) {
    Write-Host "Deploying with QML support (QML directory: $qmlDir)" -ForegroundColor Cyan
    & $windeployqt --release --qmldir $qmlDir $exePath
} else {
    Write-Host "Deploying without QML directory (using default)" -ForegroundColor Yellow
    & $windeployqt --release $exePath
}

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "Deployment completed successfully!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Qt libraries have been deployed to:" -ForegroundColor Yellow
    Write-Host "  $deployDir" -ForegroundColor White
    Write-Host ""
    Write-Host "You can now run the executable:" -ForegroundColor Cyan
    Write-Host "  .\test_1vn_2clients.ps1" -ForegroundColor White
} else {
    Write-Host ""
    Write-Host "Deployment failed with exit code: $LASTEXITCODE" -ForegroundColor Red
    exit 1
}
