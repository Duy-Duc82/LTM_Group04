# check_server_setup.ps1
# Script kiểm tra setup server WSL → Windows LAN
# Sử dụng: .\check_server_setup.ps1

Write-Host "=== Kiểm Tra Server Setup ===" -ForegroundColor Cyan
Write-Host ""

# 1. Kiểm tra WSL đang chạy
Write-Host "1. Kiểm tra WSL..." -ForegroundColor Yellow
$wslRunning = wsl --list --running 2>$null
if ($wslRunning) {
    Write-Host "   ✓ WSL đang chạy" -ForegroundColor Green
} else {
    Write-Host "   ✗ WSL chưa chạy!" -ForegroundColor Red
    Write-Host "   Khởi động WSL: wsl" -ForegroundColor Yellow
    exit 1
}

# 2. Lấy IP WSL
Write-Host ""
Write-Host "2. Lấy IP WSL..." -ForegroundColor Yellow
$wslIp = (wsl hostname -I).Trim()
if ($wslIp) {
    Write-Host "   ✓ WSL IP: $wslIp" -ForegroundColor Green
} else {
    Write-Host "   ✗ Không lấy được IP WSL!" -ForegroundColor Red
    exit 1
}

# 3. Kiểm tra server đang chạy trong WSL
Write-Host ""
Write-Host "3. Kiểm tra server trong WSL..." -ForegroundColor Yellow
$serverRunning = wsl -e bash -c "netstat -tuln 2>/dev/null | grep :9000" 2>$null
if ($serverRunning) {
    Write-Host "   ✓ Server đang lắng nghe port 9000 trong WSL" -ForegroundColor Green
    Write-Host "   $serverRunning" -ForegroundColor Gray
} else {
    Write-Host "   ✗ Server chưa chạy hoặc chưa lắng nghe port 9000!" -ForegroundColor Red
    Write-Host "   Chạy server trong WSL:" -ForegroundColor Yellow
    Write-Host "     cd server" -ForegroundColor Gray
    Write-Host "     export DB_CONN='host=localhost port=5432 dbname=ltm_group04 user=postgres password=1'" -ForegroundColor Gray
    Write-Host "     export SERVER_MODE='1'" -ForegroundColor Gray
    Write-Host "     export SERVER_PORT='9000'" -ForegroundColor Gray
    Write-Host "     ./build/server" -ForegroundColor Gray
}

# 4. Kiểm tra port forwarding
Write-Host ""
Write-Host "4. Kiểm tra port forwarding..." -ForegroundColor Yellow
$portForward = netsh interface portproxy show all 2>$null | Select-String "9000"
if ($portForward) {
    Write-Host "   ✓ Port forwarding đã được cấu hình:" -ForegroundColor Green
    Write-Host "   $portForward" -ForegroundColor Gray
    
    # Kiểm tra IP có đúng không
    if ($portForward -match $wslIp) {
        Write-Host "   ✓ IP forwarding đúng ($wslIp)" -ForegroundColor Green
    } else {
        Write-Host "   ⚠ IP forwarding có thể sai! WSL IP: $wslIp" -ForegroundColor Yellow
        Write-Host "   Chạy lại: .\forward_wsl_port.ps1 -Port 9000" -ForegroundColor Yellow
    }
} else {
    Write-Host "   ✗ Port forwarding chưa được cấu hình!" -ForegroundColor Red
    Write-Host "   Chạy: .\forward_wsl_port.ps1 -Port 9000 (as Administrator)" -ForegroundColor Yellow
}

# 5. Kiểm tra Windows lắng nghe port 9000
Write-Host ""
Write-Host "5. Kiểm tra Windows lắng nghe port 9000..." -ForegroundColor Yellow
$windowsListen = netstat -an | Select-String ":9000.*LISTENING"
if ($windowsListen) {
    Write-Host "   ✓ Windows đang lắng nghe port 9000:" -ForegroundColor Green
    Write-Host "   $windowsListen" -ForegroundColor Gray
} else {
    Write-Host "   ✗ Windows chưa lắng nghe port 9000!" -ForegroundColor Red
    Write-Host "   Cần setup port forwarding!" -ForegroundColor Yellow
}

# 6. Kiểm tra firewall
Write-Host ""
Write-Host "6. Kiểm tra firewall..." -ForegroundColor Yellow
$firewallRule = Get-NetFirewallRule | Where-Object {
    ($_.DisplayName -like "*9000*" -or $_.DisplayName -like "*Millionaire*") -and
    $_.Enabled -eq $true
}
if ($firewallRule) {
    Write-Host "   ✓ Firewall rule đã được tạo:" -ForegroundColor Green
    $firewallRule | ForEach-Object {
        Write-Host "     - $($_.DisplayName) (Enabled: $($_.Enabled))" -ForegroundColor Gray
    }
} else {
    Write-Host "   ✗ Firewall rule chưa được tạo!" -ForegroundColor Red
    Write-Host "   Tạo rule (as Administrator):" -ForegroundColor Yellow
    Write-Host "     New-NetFirewallRule -DisplayName 'Millionaire Server Port 9000' -Direction Inbound -LocalPort 9000 -Protocol TCP -Action Allow" -ForegroundColor Gray
}

# 7. Lấy IP Windows
Write-Host ""
Write-Host "7. IP Windows (cho client kết nối)..." -ForegroundColor Yellow
$winIp = (Get-NetIPAddress -AddressFamily IPv4 | Where-Object {
    $_.IPAddress -notlike "127.*" -and
    $_.IPAddress -notlike "169.254.*" -and
    $_.InterfaceAlias -like "*Wi-Fi*"
}).IPAddress | Select-Object -First 1

if ($winIp) {
    Write-Host "   ✓ Windows IP: $winIp" -ForegroundColor Green
    Write-Host "   → Client nên dùng IP này để kết nối" -ForegroundColor Cyan
} else {
    Write-Host "   ⚠ Không tìm thấy IP Wi-Fi" -ForegroundColor Yellow
    Write-Host "   Chạy: ipconfig để xem IP" -ForegroundColor Yellow
}

# 8. Test kết nối
Write-Host ""
Write-Host "8. Test kết nối..." -ForegroundColor Yellow
if ($winIp) {
    $test = Test-NetConnection -ComputerName $winIp -Port 9000 -WarningAction SilentlyContinue
    if ($test.TcpTestSucceeded) {
        Write-Host "   ✓ Kết nối TCP thành công!" -ForegroundColor Green
    } else {
        Write-Host "   ✗ Kết nối TCP thất bại!" -ForegroundColor Red
        Write-Host "   Kiểm tra lại các bước trên." -ForegroundColor Yellow
    }
} else {
    Write-Host "   ⚠ Không thể test (không có IP)" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=== Kết Thúc Kiểm Tra ===" -ForegroundColor Cyan

