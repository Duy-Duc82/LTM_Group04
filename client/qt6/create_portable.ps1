# create_portable.ps1
# Tạo portable package - thư mục có thể copy sang máy khác và chạy trực tiếp
# Đơn giản hơn so với standalone EXE, nhưng vẫn hoạt động tốt

Write-Host "=== Creating Portable Package ===" -ForegroundColor Cyan
Write-Host ""

# Kiểm tra đang ở đúng thư mục
if (-not (Test-Path "CMakeLists.txt")) {
    Write-Host "[ERROR] Please run this script from client/qt6 directory!" -ForegroundColor Red
    exit 1
}

# Tìm build directory
$buildDirs = @(
    "build\Desktop_Qt_6_10_1_MinGW_64_bit-Release",
    "build\Desktop_Qt_6_10_1_MinGW_64_bit-Debug",
    "build\Release",
    "build\Debug",
    "build"
)

$buildDir = $null
$exePath = $null

foreach ($dir in $buildDirs) {
    if (Test-Path $dir) {
        $possibleExe = Join-Path $dir "MillionaireClient.exe"
        if (Test-Path $possibleExe) {
            $buildDir = $dir
            $exePath = $possibleExe
            Write-Host "Found build directory: $buildDir" -ForegroundColor Green
            break
        }
        
        $subdirs = Get-ChildItem -Path $dir -Directory -ErrorAction SilentlyContinue
        foreach ($subdir in $subdirs) {
            $possibleExe = Join-Path $subdir.FullName "MillionaireClient.exe"
            if (Test-Path $possibleExe) {
                $buildDir = $subdir.FullName
                $exePath = $possibleExe
                Write-Host "Found build directory: $buildDir" -ForegroundColor Green
                break
            }
        }
        
        if ($null -ne $buildDir) {
            break
        }
    }
}

if ($null -eq $buildDir -or $null -eq $exePath) {
    Write-Host "[ERROR] Could not find MillionaireClient.exe!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please build the project first:" -ForegroundColor Yellow
    Write-Host "  cd build" -ForegroundColor Gray
    Write-Host "  cmake .. -G `"MinGW Makefiles`" -DCMAKE_BUILD_TYPE=Release" -ForegroundColor Gray
    Write-Host "  cmake --build . --config Release" -ForegroundColor Gray
    Write-Host "  cd .." -ForegroundColor Gray
    Write-Host "  .\deploy_qt.ps1" -ForegroundColor Gray
    exit 1
}

# Kiểm tra deployment
Write-Host "Checking deployment..." -ForegroundColor Cyan
$requiredDlls = @("Qt6Core.dll", "Qt6Gui.dll", "Qt6Network.dll")
$missingDlls = @()

foreach ($dll in $requiredDlls) {
    $dllPath = Join-Path $buildDir $dll
    if (-not (Test-Path $dllPath)) {
        $missingDlls += $dll
    }
}

if ($missingDlls.Count -gt 0) {
    Write-Host "[WARNING] Missing Qt DLLs. Running deployment..." -ForegroundColor Yellow
    if (Test-Path "deploy_qt.ps1") {
        & .\deploy_qt.ps1
    } else {
        Write-Host "[ERROR] deploy_qt.ps1 not found!" -ForegroundColor Red
        exit 1
    }
}

# Tạo thư mục portable
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$portableDir = "build\MillionaireClient_Portable_$timestamp"
$portableName = "MillionaireClient_Portable_$timestamp"

Write-Host ""
Write-Host "Creating portable package..." -ForegroundColor Cyan
Write-Host "  Source: $buildDir" -ForegroundColor Gray
Write-Host "  Destination: $portableDir" -ForegroundColor Gray
Write-Host ""

# Tạo thư mục
if (Test-Path $portableDir) {
    Remove-Item -Path $portableDir -Recurse -Force
}
New-Item -ItemType Directory -Path $portableDir -Force | Out-Null

# Copy tất cả file từ buildDir
Write-Host "Copying files..." -ForegroundColor Cyan
Copy-Item -Path "$buildDir\*" -Destination $portableDir -Recurse -Force

# Tạo file .bat để chạy
$batContent = @"
@echo off
REM Millionaire Client Launcher
REM This batch file sets the working directory and runs the application

cd /d "%~dp0"
start "" "MillionaireClient.exe"
"@

$batFile = Join-Path $portableDir "Run_MillionaireClient.bat"
$batContent | Out-File -FilePath $batFile -Encoding ASCII

Write-Host "✓ Files copied" -ForegroundColor Green
Write-Host "✓ Launcher batch file created" -ForegroundColor Green

# Tạo file README
$readmeContent = @"
Millionaire Client - Portable Package
=====================================

This is a portable version of Millionaire Client.
You can copy this entire folder to any Windows machine and run it directly.

HOW TO RUN:
-----------
1. Double-click "Run_MillionaireClient.bat"
   OR
2. Double-click "MillionaireClient.exe"

REQUIREMENTS:
-------------
- Windows 10/11 (64-bit)
- No additional software required (Qt6 libraries included)

CONFIGURATION:
--------------
When you first run the client, you may need to configure the server connection:
- Host: <server_ip_address>
- Port: 9000

TROUBLESHOOTING:
----------------
If the application doesn't start:
1. Make sure all files are in the same folder
2. Check Windows Defender/Antivirus isn't blocking it
3. Try running as Administrator

For more information, see FEATURES.md

Created: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
"@

$readmeFile = Join-Path $portableDir "README.txt"
$readmeContent | Out-File -FilePath $readmeFile -Encoding UTF8

Write-Host "✓ README file created" -ForegroundColor Green

# Tính kích thước
$totalSize = (Get-ChildItem -Path $portableDir -Recurse -File | Measure-Object -Property Length -Sum).Sum / 1MB

Write-Host ""
Write-Host "=== Portable Package Created ===" -ForegroundColor Green
Write-Host ""
Write-Host "Location: $portableDir" -ForegroundColor White
Write-Host "Size: $([math]::Round($totalSize, 2)) MB" -ForegroundColor White
Write-Host ""
Write-Host "Files included:" -ForegroundColor Cyan
Write-Host "  - MillionaireClient.exe" -ForegroundColor Gray
Write-Host "  - Qt6 DLLs and plugins" -ForegroundColor Gray
Write-Host "  - Run_MillionaireClient.bat (launcher)" -ForegroundColor Gray
Write-Host "  - README.txt" -ForegroundColor Gray
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. Test the package: cd `"$portableDir`" && .\Run_MillionaireClient.bat" -ForegroundColor Yellow
Write-Host "  2. Copy the entire folder to other machines" -ForegroundColor Yellow
Write-Host "  3. Run from any location (portable)" -ForegroundColor Yellow
Write-Host ""
Write-Host "Optional: Create ZIP archive:" -ForegroundColor Gray
Write-Host "  Compress-Archive -Path `"$portableDir\*`" -DestinationPath `"$portableDir.zip`" -Force" -ForegroundColor Gray
Write-Host ""

# Hỏi có muốn tạo ZIP không
$createZip = Read-Host "Create ZIP archive? (Y/N)"
if ($createZip -eq "Y" -or $createZip -eq "y") {
    $zipFile = "$portableDir.zip"
    Write-Host "Creating ZIP archive..." -ForegroundColor Cyan
    Compress-Archive -Path "$portableDir\*" -DestinationPath $zipFile -CompressionLevel Optimal -Force
    
    if (Test-Path $zipFile) {
        $zipSize = (Get-Item $zipFile).Length / 1MB
        Write-Host "✓ ZIP archive created: $zipFile" -ForegroundColor Green
        Write-Host "  Size: $([math]::Round($zipSize, 2)) MB" -ForegroundColor White
    }
}

Write-Host ""
Write-Host "=== Done ===" -ForegroundColor Green

