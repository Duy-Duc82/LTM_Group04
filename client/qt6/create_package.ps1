# create_package.ps1
# Script tự động tạo package deployment cho MillionaireClient
# Sử dụng: .\create_package.ps1

Write-Host "=== Creating MillionaireClient Package ===" -ForegroundColor Cyan
Write-Host ""

# Kiểm tra đang ở đúng thư mục
if (-not (Test-Path "CMakeLists.txt")) {
    Write-Host "[ERROR] Please run this script from client/qt6 directory!" -ForegroundColor Red
    exit 1
}

# Tìm build directory
$buildDirs = @(
    "build\Desktop_Qt_6_10_1_MinGW_64_bit-Debug",
    "build\Desktop_Qt_6_10_1_MinGW_64_bit-Release",
    "build\Release",
    "build\Debug",
    "build"
)

$buildDir = $null
$exePath = $null

foreach ($dir in $buildDirs) {
    if (Test-Path $dir) {
        # Thử tìm exe trong thư mục
        $possibleExe = Join-Path $dir "MillionaireClient.exe"
        if (Test-Path $possibleExe) {
            $buildDir = $dir
            $exePath = $possibleExe
            Write-Host "Found build directory: $buildDir" -ForegroundColor Green
            break
        }
        
        # Thử tìm trong subdirectories
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
    Write-Host "[ERROR] Could not find build directory or MillionaireClient.exe!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please build the project first:" -ForegroundColor Yellow
    Write-Host "  cd build" -ForegroundColor Gray
    Write-Host "  cmake .. -G `"MinGW Makefiles`"" -ForegroundColor Gray
    Write-Host "  cmake --build ." -ForegroundColor Gray
    Write-Host "  cd .." -ForegroundColor Gray
    Write-Host "  .\deploy_qt.ps1" -ForegroundColor Gray
    exit 1
}

# Kiểm tra deployment (Qt DLLs)
Write-Host "Checking deployment..." -ForegroundColor Cyan
$requiredDlls = @("Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll", "Qt6Network.dll")
$missingDlls = @()

foreach ($dll in $requiredDlls) {
    $dllPath = Join-Path $buildDir $dll
    if (-not (Test-Path $dllPath)) {
        $missingDlls += $dll
    }
}

if ($missingDlls.Count -gt 0) {
    Write-Host "[WARNING] Missing Qt DLLs: $($missingDlls -join ', ')" -ForegroundColor Yellow
    Write-Host "Running deployment script..." -ForegroundColor Yellow
    Write-Host ""
    
    # Chạy deploy script
    if (Test-Path "deploy_qt.ps1") {
        & .\deploy_qt.ps1
        Write-Host ""
        
        # Kiểm tra lại
        $stillMissing = @()
        foreach ($dll in $missingDlls) {
            $dllPath = Join-Path $buildDir $dll
            if (-not (Test-Path $dllPath)) {
                $stillMissing += $dll
            }
        }
        
        if ($stillMissing.Count -gt 0) {
            Write-Host "[ERROR] Still missing DLLs after deployment: $($stillMissing -join ', ')" -ForegroundColor Red
            Write-Host "Please deploy manually or check Qt installation." -ForegroundColor Yellow
            exit 1
        }
    } else {
        Write-Host "[ERROR] deploy_qt.ps1 not found!" -ForegroundColor Red
        Write-Host "Please deploy Qt libraries manually before creating package." -ForegroundColor Yellow
        exit 1
    }
} else {
    Write-Host "✓ All required Qt DLLs found" -ForegroundColor Green
}

# Kiểm tra plugins (Qt plugins thường nằm trực tiếp trong build dir, không phải trong thư mục "plugins")
$pluginDirs = @("platforms", "imageformats", "styles", "iconengines", "networkinformation", "tls", "translations")
$foundPlugins = @()
$totalPluginFiles = 0

foreach ($pluginDir in $pluginDirs) {
    $pluginPath = Join-Path $buildDir $pluginDir
    if (Test-Path $pluginPath) {
        $foundPlugins += $pluginDir
        $fileCount = (Get-ChildItem -Path $pluginPath -Recurse -File -ErrorAction SilentlyContinue).Count
        $totalPluginFiles += $fileCount
    }
}

if ($foundPlugins.Count -gt 0) {
    Write-Host "✓ Qt plugins found: $($foundPlugins -join ', ') ($totalPluginFiles files)" -ForegroundColor Green
} else {
    Write-Host "[WARNING] No Qt plugins found (platforms, imageformats, etc.)" -ForegroundColor Yellow
    Write-Host "  Application may not work properly without platform plugins!" -ForegroundColor Yellow
}

# Tạo package
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$packageName = "MillionaireClient_Package_$timestamp.zip"
$packagePath = Join-Path "build" $packageName

Write-Host ""
Write-Host "Creating package..." -ForegroundColor Cyan
Write-Host "  Source: $buildDir" -ForegroundColor Gray
Write-Host "  Destination: $packagePath" -ForegroundColor Gray
Write-Host ""

# Tạo ZIP (bao gồm tất cả files và subdirectories)
try {
    Compress-Archive -Path "$buildDir\*" `
                      -DestinationPath $packagePath `
                      -CompressionLevel Optimal `
                      -Force
    
    if (Test-Path $packagePath) {
        $size = (Get-Item $packagePath).Length / 1MB
        Write-Host ""
        Write-Host "✓ Package created successfully!" -ForegroundColor Green
        Write-Host ""
        Write-Host "Package Details:" -ForegroundColor Cyan
        Write-Host "  - File: $packagePath" -ForegroundColor White
        Write-Host "  - Size: $([math]::Round($size, 2)) MB" -ForegroundColor White
        Write-Host ""
        Write-Host "Next steps:" -ForegroundColor Cyan
        Write-Host "  1. Copy $packageName to client machines" -ForegroundColor Yellow
        Write-Host "  2. Extract to C:\MillionaireClient on each client" -ForegroundColor Yellow
        Write-Host "  3. Run MillionaireClient.exe" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "Or use install_client.ps1 script on client machines." -ForegroundColor Gray
    } else {
        Write-Host "[ERROR] Package file was not created!" -ForegroundColor Red
        exit 1
    }
} catch {
    Write-Host "[ERROR] Failed to create package" -ForegroundColor Red
    exit 1
}


