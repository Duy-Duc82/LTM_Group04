# install_client.ps1
# Script tự động cài đặt MillionaireClient từ package ZIP
# Sử dụng: .\install_client.ps1 [-PackagePath <path>] [-InstallDir <dir>]

param(
    [Parameter(Mandatory=$false)]
    [string]$PackagePath = "",
    
    [Parameter(Mandatory=$false)]
    [string]$InstallDir = "C:\MillionaireClient"
)

Write-Host "=== MillionaireClient Installation ===" -ForegroundColor Cyan
Write-Host ""

# Tìm package nếu không chỉ định
if ([string]::IsNullOrEmpty($PackagePath)) {
    Write-Host "Searching for package file..." -ForegroundColor Cyan
    
    $searchPaths = @(
        "$PSScriptRoot\MillionaireClient_Package_*.zip",
        "$env:USERPROFILE\Desktop\MillionaireClient_Package_*.zip",
        "$env:USERPROFILE\Downloads\MillionaireClient_Package_*.zip",
        "$env:USERPROFILE\Documents\MillionaireClient_Package_*.zip",
        "D:\MillionaireClient_Package_*.zip",
        "E:\MillionaireClient_Package_*.zip",
        "F:\MillionaireClient_Package_*.zip"
    )
    
    foreach ($pattern in $searchPaths) {
        $found = Get-ChildItem -Path $pattern -ErrorAction SilentlyContinue | 
                 Sort-Object LastWriteTime -Descending | 
                 Select-Object -First 1
        if ($found) {
            $PackagePath = $found.FullName
            Write-Host "Found package: $PackagePath" -ForegroundColor Green
            break
        }
    }
}

# Kiểm tra package tồn tại
if ([string]::IsNullOrEmpty($PackagePath) -or -not (Test-Path $PackagePath)) {
    Write-Host "[ERROR] Package not found!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Usage:" -ForegroundColor Yellow
    Write-Host "  .\install_client.ps1 -PackagePath <path_to_zip>" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Or place MillionaireClient_Package_*.zip in one of these locations:" -ForegroundColor Yellow
    Write-Host "  - Same directory as this script" -ForegroundColor Gray
    Write-Host "  - Desktop" -ForegroundColor Gray
    Write-Host "  - Downloads" -ForegroundColor Gray
    Write-Host "  - Documents" -ForegroundColor Gray
    Write-Host "  - USB drive (D:\, E:\, F:\...)" -ForegroundColor Gray
    exit 1
}

# Kiểm tra file hợp lệ
try {
    $zip = [System.IO.Compression.ZipFile]::OpenRead($PackagePath)
    $zip.Dispose()
} catch {
    Write-Host "[ERROR] Invalid or corrupted ZIP file: $PackagePath" -ForegroundColor Red
    exit 1
}

# Kiểm tra quyền admin nếu cài vào C:\
if ($InstallDir -like "C:\*") {
    $currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
    $isAdmin = $currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
    
    if (-not $isAdmin) {
        Write-Host "[WARNING] Installing to C:\ requires administrator privileges." -ForegroundColor Yellow
        Write-Host "You may need to run PowerShell as Administrator." -ForegroundColor Yellow
        Write-Host ""
        $response = Read-Host "Continue anyway? (Y/N)"
        if ($response -ne "Y" -and $response -ne "y") {
            Write-Host "Installation cancelled." -ForegroundColor Yellow
            exit 0
        }
    }
}

# Tạo thư mục cài đặt
Write-Host "Installation directory: $InstallDir" -ForegroundColor Cyan

if (Test-Path $InstallDir) {
    Write-Host "[WARNING] Directory already exists: $InstallDir" -ForegroundColor Yellow
    $response = Read-Host "Overwrite existing installation? (Y/N)"
    if ($response -eq "Y" -or $response -eq "y") {
        Write-Host "Removing existing installation..." -ForegroundColor Yellow
        Remove-Item -Path $InstallDir -Recurse -Force -ErrorAction SilentlyContinue
    } else {
        Write-Host "Installation cancelled." -ForegroundColor Yellow
        exit 0
    }
}

try {
    New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
    Write-Host "✓ Created installation directory" -ForegroundColor Green
} catch {
    Write-Host "[ERROR] Failed to create directory: $_" -ForegroundColor Red
    exit 1
}

# Giải nén package
Write-Host ""
Write-Host "Extracting package..." -ForegroundColor Cyan
Write-Host "  From: $PackagePath" -ForegroundColor Gray
Write-Host "  To: $InstallDir" -ForegroundColor Gray
Write-Host ""

try {
    Expand-Archive -Path $PackagePath `
                   -DestinationPath $InstallDir `
                   -Force
    
    Write-Host "✓ Extraction completed" -ForegroundColor Green
} catch {
    Write-Host "[ERROR] Failed to extract package: $_" -ForegroundColor Red
    exit 1
}

# Kiểm tra installation
Write-Host ""
Write-Host "Verifying installation..." -ForegroundColor Cyan

$exePath = Join-Path $InstallDir "MillionaireClient.exe"
if (-not (Test-Path $exePath)) {
    Write-Host "[ERROR] MillionaireClient.exe not found after extraction!" -ForegroundColor Red
    Write-Host "Package may be corrupted or incomplete." -ForegroundColor Yellow
    exit 1
}

Write-Host "✓ MillionaireClient.exe found" -ForegroundColor Green

# Kiểm tra Qt DLLs
$requiredDlls = @("Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll", "Qt6Network.dll")
$foundDlls = 0

foreach ($dll in $requiredDlls) {
    $dllPath = Join-Path $InstallDir $dll
    if (Test-Path $dllPath) {
        $foundDlls++
    }
}

if ($foundDlls -eq $requiredDlls.Count) {
    Write-Host "✓ All Qt DLLs found ($foundDlls/$($requiredDlls.Count))" -ForegroundColor Green
} else {
    Write-Host "[WARNING] Some Qt DLLs missing ($foundDlls/$($requiredDlls.Count))" -ForegroundColor Yellow
    Write-Host "Client may not run properly." -ForegroundColor Yellow
}

# Kiểm tra plugins (Qt plugins thường nằm trực tiếp trong install dir, không phải trong thư mục "plugins")
$pluginDirs = @("platforms", "imageformats", "styles", "iconengines", "networkinformation", "tls", "translations")
$foundPlugins = @()
$totalPluginFiles = 0

foreach ($pluginDir in $pluginDirs) {
    $pluginPath = Join-Path $InstallDir $pluginDir
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

# Tạo shortcut trên Desktop (optional)
Write-Host ""
$createShortcut = Read-Host "Create desktop shortcut? (Y/N)"
if ($createShortcut -eq "Y" -or $createShortcut -eq "y") {
    try {
        $WshShell = New-Object -ComObject WScript.Shell
        $Shortcut = $WshShell.CreateShortcut("$env:USERPROFILE\Desktop\MillionaireClient.lnk")
        $Shortcut.TargetPath = $exePath
        $Shortcut.WorkingDirectory = $InstallDir
        $Shortcut.Description = "Millionaire Game Client"
        $Shortcut.IconLocation = $exePath
        $Shortcut.Save()
        Write-Host "✓ Desktop shortcut created" -ForegroundColor Green
    } catch {
        Write-Host "[WARNING] Failed to create shortcut: $_" -ForegroundColor Yellow
    }
}

# Tóm tắt
Write-Host ""
Write-Host "=== Installation Completed ===" -ForegroundColor Green
Write-Host ""
Write-Host "Client installed at: $InstallDir" -ForegroundColor White
Write-Host ""
Write-Host "To run the client:" -ForegroundColor Cyan
Write-Host "  cd $InstallDir" -ForegroundColor Yellow
Write-Host "  .\MillionaireClient.exe" -ForegroundColor Yellow
Write-Host ""
Write-Host "Or double-click the desktop shortcut (if created)." -ForegroundColor Gray
Write-Host ""
Write-Host "Next step: Configure server connection in the client:" -ForegroundColor Cyan
Write-Host "  - Host: <server_ip_address>" -ForegroundColor Yellow
Write-Host "  - Port: 9000" -ForegroundColor Yellow
Write-Host ""

