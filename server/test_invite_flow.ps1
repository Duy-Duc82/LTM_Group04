# Test invite flow (PowerShell version)

Write-Host "=== Building test_invite_flow ===" -ForegroundColor Cyan

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

# Compile using WSL gcc
wsl gcc -o build/test_invite_flow `
    src/test/test_invite_flow.c `
    -I include `
    -Wall -Wextra `
    -g

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host "Build successful!" -ForegroundColor Green
Write-Host ""
Write-Host "=== Running test ===" -ForegroundColor Cyan
Write-Host ""

# Check if server is running
$serverRunning = Test-NetConnection -ComputerName 127.0.0.1 -Port 9000 -InformationLevel Quiet -WarningAction SilentlyContinue

if (-not $serverRunning) {
    Write-Host "WARNING: Server is not running on port 9000" -ForegroundColor Yellow
    Write-Host "Please start the server first" -ForegroundColor Yellow
    Write-Host ""
    $continue = Read-Host "Continue anyway? (y/n)"
    if ($continue -ne 'y' -and $continue -ne 'Y') {
        exit 1
    }
}

# Run test in WSL
wsl ./build/test_invite_flow 127.0.0.1 9000
