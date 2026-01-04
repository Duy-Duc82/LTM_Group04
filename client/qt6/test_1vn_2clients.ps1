# Script to test 1vN mode with 2 clients on same machine
# Run from PowerShell: .\test_1vn_2clients.ps1

Write-Host "=== Test 1vN Mode with 2 Clients ===" -ForegroundColor Cyan
Write-Host ""

$exePath = ".\build\Desktop_Qt_6_10_1_MinGW_64_bit-Debug\MillionaireClient.exe"
$buildDir = ".\build\Desktop_Qt_6_10_1_MinGW_64_bit-Debug"

# Check if exe exists
if (-not (Test-Path $exePath)) {
    Write-Host "[ERROR] File not found: $exePath" -ForegroundColor Red
    Write-Host "Please build the project first." -ForegroundColor Yellow
    exit 1
}

# Check if Qt DLLs are deployed (required for QML)
$qtDlls = @("Qt6Qml.dll", "Qt6Quick.dll", "Qt6Core.dll", "Qt6Gui.dll", "Qt6Network.dll")
$missingDlls = @()
foreach ($dll in $qtDlls) {
    $dllPath = Join-Path $buildDir $dll
    if (-not (Test-Path $dllPath)) {
        $missingDlls += $dll
    }
}

# Only deploy if DLLs are actually missing
if ($missingDlls.Count -gt 0) {
    Write-Host "[WARNING] Missing Qt DLLs detected!" -ForegroundColor Yellow
    Write-Host "Missing: $($missingDlls -join ', ')" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Qt DLLs are required for QML applications." -ForegroundColor Cyan
    Write-Host "Attempting to deploy Qt libraries..." -ForegroundColor Cyan
    Write-Host ""
    
    # Try to find and run deploy script
    $deployScript = ".\deploy_qt.ps1"
    if (Test-Path $deployScript) {
        Write-Host "Running deployment script..." -ForegroundColor Cyan
        & $deployScript
        if ($LASTEXITCODE -ne 0) {
            Write-Host ""
            Write-Host "[ERROR] Deployment failed!" -ForegroundColor Red
            Write-Host "Please run manually: .\deploy_qt.ps1" -ForegroundColor Yellow
            exit 1
        }
        Write-Host ""
    } else {
        Write-Host "[ERROR] Deployment script not found: $deployScript" -ForegroundColor Red
        Write-Host ""
        Write-Host "Please run deployment manually:" -ForegroundColor Yellow
        Write-Host "  1. Run: .\deploy_qt.ps1" -ForegroundColor White
        Write-Host "  2. Or add Qt bin to PATH: C:\Qt\6.10.1\mingw_64\bin" -ForegroundColor White
        exit 1
    }
} else {
    Write-Host "[INFO] Qt DLLs are already deployed" -ForegroundColor Green
    Write-Host ""
}

# Find Qt bin directory to add to PATH
$qtBinPaths = @(
    "C:\Qt\6.10.1\mingw_64\bin",
    "C:\Qt\6.10.0\mingw_64\bin",
    "C:\Qt\6.9.2\mingw_64\bin"
)

$qtBinPath = $null
foreach ($path in $qtBinPaths) {
    if (Test-Path $path) {
        $qtBinPath = $path
        break
    }
}

Write-Host "Testing Instructions:" -ForegroundColor Yellow
Write-Host "1. Two client windows will open" -ForegroundColor White
Write-Host "2. Window 1 - Login with: testuser1 / testpass1" -ForegroundColor White
Write-Host "3. Window 2 - Login with: testuser2 / testpass2" -ForegroundColor White
Write-Host "4. Client 1 creates room and selects question count" -ForegroundColor White
Write-Host "5. Client 2 joins room (enter room ID from client 1)" -ForegroundColor White
Write-Host "6. When both are in room, client 1 clicks Start Game" -ForegroundColor White
Write-Host ""
Write-Host "Note: Server must be running on WSL (localhost:9000)" -ForegroundColor Green
Write-Host ""

# Wait for user to read instructions
Read-Host "Press Enter to start..."

Write-Host ""
Write-Host "Starting Client 1..." -ForegroundColor Cyan

# Set environment with Qt bin in PATH for this process
$env:Path = if ($qtBinPath) { "$qtBinPath;$env:Path" } else { $env:Path }
Start-Process -FilePath $exePath -WorkingDirectory $buildDir

# Wait 2 seconds for client 1 to start
Start-Sleep -Seconds 2

Write-Host "Starting Client 2..." -ForegroundColor Cyan
Start-Process -FilePath $exePath -WorkingDirectory $buildDir

Write-Host ""
Write-Host "Successfully launched 2 clients!" -ForegroundColor Green
Write-Host "Login accounts:" -ForegroundColor Yellow
Write-Host ""
Write-Host "  Client 1: testuser1 / testpass1" -ForegroundColor White
Write-Host "  Client 2: testuser2 / testpass2" -ForegroundColor White
Write-Host ""
Write-Host "For 3+ players, run the exe again:" -ForegroundColor Cyan
Write-Host "  $exePath" -ForegroundColor Gray
Write-Host "  Login with: testuser3 / testpass3" -ForegroundColor Gray
Write-Host ""
