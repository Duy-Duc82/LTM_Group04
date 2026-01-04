# forward_wsl_port.ps1
# Forward port từ WSL2 ra Windows để client Windows có thể kết nối
# Sử dụng: .\forward_wsl_port.ps1 -Port 9000

param(
    [Parameter(Mandatory=$true)]
    [int]$Port = 9000
)

Write-Host "=== Forwarding WSL2 Port to Windows ===" -ForegroundColor Cyan
Write-Host ""

# Lấy IP của WSL2
Write-Host "Getting WSL2 IP address..." -ForegroundColor Cyan
$wslIp = (wsl hostname -I).Trim()
if ([string]::IsNullOrEmpty($wslIp)) {
    Write-Host "[ERROR] Could not get WSL2 IP address!" -ForegroundColor Red
    Write-Host "Make sure WSL2 is running." -ForegroundColor Yellow
    exit 1
}

Write-Host "WSL2 IP: $wslIp" -ForegroundColor Green
Write-Host ""

# Kiểm tra quyền admin
$currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
$isAdmin = $currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "[WARNING] This script requires Administrator privileges!" -ForegroundColor Yellow
    Write-Host "Please run PowerShell as Administrator." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "To forward port manually:" -ForegroundColor Cyan
    Write-Host "  netsh interface portproxy add v4tov4 listenport=$Port listenaddress=0.0.0.0 connectport=$Port connectaddress=$wslIp" -ForegroundColor White
    exit 1
}

# Xóa rule cũ nếu có
Write-Host "Removing existing port forwarding rule (if any)..." -ForegroundColor Cyan
netsh interface portproxy delete v4tov4 listenport=$Port listenaddress=0.0.0.0 2>$null

# Tạo rule mới
Write-Host "Creating port forwarding rule..." -ForegroundColor Cyan
netsh interface portproxy add v4tov4 listenport=$Port listenaddress=0.0.0.0 connectport=$Port connectaddress=$wslIp

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "✓ Port forwarding configured successfully!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Configuration:" -ForegroundColor Cyan
    Write-Host "  Windows listens on: 0.0.0.0:$Port" -ForegroundColor White
    Write-Host "  Forwards to WSL2: $wslIp`:$Port" -ForegroundColor White
    Write-Host ""
    Write-Host "Now Windows clients can connect to:" -ForegroundColor Cyan
    Write-Host "  - localhost:$Port" -ForegroundColor Yellow
    Write-Host "  - 127.0.0.1:$Port" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "To verify:" -ForegroundColor Cyan
    Write-Host "  netstat -an | findstr :$Port" -ForegroundColor Gray
} else {
    Write-Host "[ERROR] Failed to create port forwarding rule!" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Note: This forwarding persists until you remove it or restart Windows." -ForegroundColor Gray
Write-Host "To remove, run:" -ForegroundColor Gray
Write-Host "  netsh interface portproxy delete v4tov4 listenport=$Port listenaddress=0.0.0.0" -ForegroundColor Gray


