# create_single_exe.ps1
# Script tạo file .exe độc lập (standalone) cho MillionaireClient
# Sử dụng Enigma Virtual Box để đóng gói tất cả dependencies vào 1 file .exe
# 
# Cách 1: Sử dụng Enigma Virtual Box (khuyến nghị)
# Cách 2: Sử dụng UPX (nén file, nhưng vẫn cần DLLs)
# Cách 3: Static linking (phức tạp, cần build Qt static)

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet("Enigma", "UPX", "Static")]
    [string]$Method = "Enigma"
)

Write-Host "=== Creating Standalone EXE for MillionaireClient ===" -ForegroundColor Cyan
Write-Host ""

# Kiểm tra đang ở đúng thư mục
if (-not (Test-Path "CMakeLists.txt")) {
    Write-Host "[ERROR] Please run this script from client/qt6 directory!" -ForegroundColor Red
    exit 1
}

# Tìm build directory và exe
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
        
        # Tìm trong subdirectories
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
$requiredDlls = @("Qt6Core.dll", "Qt6Gui.dll", "Qt6Network.dll", "Qt6Qml.dll", "Qt6Quick.dll")
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

# ============================================
# METHOD 1: Enigma Virtual Box (Khuyến nghị)
# ============================================
if ($Method -eq "Enigma") {
    Write-Host ""
    Write-Host "=== Method: Enigma Virtual Box ===" -ForegroundColor Cyan
    Write-Host ""
    
    # Tìm Enigma Virtual Box
    $enigmaPaths = @(
        "$env:ProgramFiles\Enigma Virtual Box\enigmavb.exe",
        "$env:ProgramFiles(x86)\Enigma Virtual Box\enigmavb.exe",
        "C:\Program Files\Enigma Virtual Box\enigmavb.exe",
        "C:\Program Files (x86)\Enigma Virtual Box\enigmavb.exe"
    )
    
    $enigmaExe = $null
    foreach ($path in $enigmaPaths) {
        if (Test-Path $path) {
            $enigmaExe = $path
            Write-Host "Found Enigma Virtual Box: $path" -ForegroundColor Green
            break
        }
    }
    
    if ($null -eq $enigmaExe) {
        Write-Host "[ERROR] Enigma Virtual Box not found!" -ForegroundColor Red
        Write-Host ""
        Write-Host "Please install Enigma Virtual Box:" -ForegroundColor Yellow
        Write-Host "  Download from: https://enigmaprotector.com/en/downloads.html" -ForegroundColor Cyan
        Write-Host "  Install to default location: C:\Program Files\Enigma Virtual Box" -ForegroundColor Gray
        Write-Host ""
        Write-Host "Alternative: Use UPX method (simpler but less effective)" -ForegroundColor Yellow
        Write-Host "  .\create_single_exe.ps1 -Method UPX" -ForegroundColor Gray
        exit 1
    }
    
    # Tạo file .evb (Enigma Virtual Box project file)
    Write-Host "Creating Enigma Virtual Box project..." -ForegroundColor Cyan
    
    $evbFile = Join-Path $buildDir "MillionaireClient.evb"
    $outputExe = Join-Path $buildDir "MillionaireClient_Standalone.exe"
    
    # Tạo nội dung file .evb
    $evbContent = @"
<Files>
"@
    
    # Thêm tất cả file cần thiết
    $filesToInclude = @(
        "MillionaireClient.exe",
        "Qt6Core.dll",
        "Qt6Gui.dll",
        "Qt6Network.dll",
        "Qt6Qml.dll",
        "Qt6Quick.dll",
        "Qt6QuickControls2.dll",
        "Qt6QuickTemplates2.dll"
    )
    
    # Thêm các file DLL khác trong buildDir
    $allDlls = Get-ChildItem -Path $buildDir -Filter "*.dll" -File
    foreach ($dll in $allDlls) {
        if ($filesToInclude -notcontains $dll.Name) {
            $filesToInclude += $dll.Name
        }
    }
    
    # Thêm plugins
    $pluginDirs = @("platforms", "imageformats", "styles", "iconengines", "networkinformation", "tls", "translations", "qml")
    foreach ($pluginDir in $pluginDirs) {
        $pluginPath = Join-Path $buildDir $pluginDir
        if (Test-Path $pluginPath) {
            $pluginFiles = Get-ChildItem -Path $pluginPath -Recurse -File
            foreach ($file in $pluginFiles) {
                $relativePath = $file.FullName.Replace($buildDir + "\", "")
                $evbContent += @"
  <File>
    <Source>$($file.FullName)</Source>
    <Target>$relativePath</Target>
  </File>
"@
            }
        }
    }
    
    # Thêm các file DLL
    foreach ($file in $filesToInclude) {
        $filePath = Join-Path $buildDir $file
        if (Test-Path $filePath) {
            $evbContent += @"
  <File>
    <Source>$filePath</Source>
    <Target>$file</Target>
  </File>
"@
        }
    }
    
    $evbContent += @"
</Files>
"@
    
    # Lưu file .evb
    $evbContent | Out-File -FilePath $evbFile -Encoding UTF8
    
    Write-Host "EVB project file created: $evbFile" -ForegroundColor Green
    
    # Hướng dẫn sử dụng Enigma Virtual Box GUI
    Write-Host ""
    Write-Host "=== Enigma Virtual Box Project Created ===" -ForegroundColor Green
    Write-Host ""
    Write-Host "EVB project file: $evbFile" -ForegroundColor White
    Write-Host ""
    Write-Host "Next steps (Manual):" -ForegroundColor Cyan
    Write-Host "  1. Open Enigma Virtual Box GUI" -ForegroundColor Yellow
    Write-Host "  2. File → Open → Select: $evbFile" -ForegroundColor Yellow
    Write-Host "  3. Click 'Process' button" -ForegroundColor Yellow
    Write-Host "  4. Output file: $outputExe" -ForegroundColor Yellow
    Write-Host "  5. Click 'OK' to create standalone EXE" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Alternative: Use the GUI method described in BUILD_STANDALONE_EXE.md" -ForegroundColor Gray
    Write-Host ""
    
    # Thử mở Enigma Virtual Box GUI
    $openGUI = Read-Host "Open Enigma Virtual Box GUI now? (Y/N)"
    if ($openGUI -eq "Y" -or $openGUI -eq "y") {
        Start-Process $enigmaExe -ArgumentList $evbFile
        Write-Host "Enigma Virtual Box opened with project file." -ForegroundColor Green
    }
    
    # Không tự động tạo file vì Enigma command line có thể không hoạt động
    $outputExe = Join-Path $buildDir "MillionaireClient_Standalone.exe"
    Write-Host ""
    Write-Host "After processing in Enigma Virtual Box, standalone EXE will be:" -ForegroundColor Cyan
    Write-Host "  $outputExe" -ForegroundColor White
    
    if ($LASTEXITCODE -eq 0 -and (Test-Path $outputExe)) {
        $size = (Get-Item $outputExe).Length / 1MB
        Write-Host ""
        Write-Host "✓ Standalone EXE created successfully!" -ForegroundColor Green
        Write-Host ""
        Write-Host "File: $outputExe" -ForegroundColor White
        Write-Host "Size: $([math]::Round($size, 2)) MB" -ForegroundColor White
        Write-Host ""
        Write-Host "This file can be run on any Windows machine without Qt6 installed!" -ForegroundColor Cyan
    } else {
        Write-Host "[ERROR] Failed to create standalone EXE" -ForegroundColor Red
        Write-Host "Exit code: $LASTEXITCODE" -ForegroundColor Red
        exit 1
    }
}

# ============================================
# METHOD 2: UPX (Compression only)
# ============================================
elseif ($Method -eq "UPX") {
    Write-Host ""
    Write-Host "=== Method: UPX Compression ===" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "[NOTE] UPX only compresses the EXE, but DLLs are still required." -ForegroundColor Yellow
    Write-Host "This method creates a smaller EXE but you still need to distribute DLLs." -ForegroundColor Yellow
    Write-Host ""
    
    # Tìm UPX
    $upxPaths = @(
        "upx.exe",
        "$env:ProgramFiles\upx\upx.exe",
        "C:\upx\upx.exe"
    )
    
    $upxExe = $null
    foreach ($path in $upxPaths) {
        if (Test-Path $path) {
            $upxExe = $path
            break
        }
    }
    
    # Check if upx is in PATH
    if ($null -eq $upxExe) {
        $upxCheck = Get-Command upx -ErrorAction SilentlyContinue
        if ($upxCheck) {
            $upxExe = "upx"
        }
    }
    
    if ($null -eq $upxExe) {
        Write-Host "[ERROR] UPX not found!" -ForegroundColor Red
        Write-Host ""
        Write-Host "Please install UPX:" -ForegroundColor Yellow
        Write-Host "  Download from: https://upx.github.io/" -ForegroundColor Cyan
        Write-Host "  Or use: choco install upx" -ForegroundColor Gray
        exit 1
    }
    
    $compressedExe = Join-Path $buildDir "MillionaireClient_Compressed.exe"
    
    # Copy exe first
    Copy-Item -Path $exePath -Destination $compressedExe -Force
    
    Write-Host "Compressing EXE with UPX..." -ForegroundColor Cyan
    & $upxExe --best --lzma $compressedExe
    
    if ($LASTEXITCODE -eq 0) {
        $originalSize = (Get-Item $exePath).Length / 1MB
        $compressedSize = (Get-Item $compressedExe).Length / 1MB
        $saved = $originalSize - $compressedSize
        
        Write-Host ""
        Write-Host "✓ EXE compressed successfully!" -ForegroundColor Green
        Write-Host "  Original: $([math]::Round($originalSize, 2)) MB" -ForegroundColor White
        Write-Host "  Compressed: $([math]::Round($compressedSize, 2)) MB" -ForegroundColor White
        Write-Host "  Saved: $([math]::Round($saved, 2)) MB" -ForegroundColor Green
        Write-Host ""
        Write-Host "[WARNING] You still need to distribute Qt DLLs with this EXE!" -ForegroundColor Yellow
        Write-Host "  Use create_package.ps1 to create a complete package." -ForegroundColor Gray
    } else {
        Write-Host "[ERROR] UPX compression failed" -ForegroundColor Red
        exit 1
    }
}

# ============================================
# METHOD 3: Static Linking (Advanced)
# ============================================
elseif ($Method -eq "Static") {
    Write-Host ""
    Write-Host "=== Method: Static Linking ===" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "[NOTE] Static linking requires building Qt6 with static libraries." -ForegroundColor Yellow
    Write-Host "This is complex and time-consuming. Not recommended for most users." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Steps for static linking:" -ForegroundColor Cyan
    Write-Host "  1. Build Qt6 with static libraries (takes hours)" -ForegroundColor Gray
    Write-Host "  2. Configure CMake to use static Qt6" -ForegroundColor Gray
    Write-Host "  3. Rebuild the project" -ForegroundColor Gray
    Write-Host ""
    Write-Host "For more information, see:" -ForegroundColor Cyan
    Write-Host "  https://doc.qt.io/qt-6/windows-deployment.html" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Recommended: Use Enigma Virtual Box method instead." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=== Done ===" -ForegroundColor Green

