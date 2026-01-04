# =========================================================
# TEST SCRIPT: Friend Request Flow (PowerShell)
# Mục đích: Chạy test SQL và hiển thị kết quả trên Windows
# =========================================================

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "Testing Friend Request Flow" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

# Kiểm tra psql command
$psqlPath = Get-Command psql -ErrorAction SilentlyContinue
if (-not $psqlPath) {
    Write-Host "ERROR: psql command not found!" -ForegroundColor Red
    Write-Host "Please install PostgreSQL client tools" -ForegroundColor Red
    Write-Host "Or add PostgreSQL bin directory to PATH" -ForegroundColor Red
    exit 1
}

# Database config
$DB_NAME = if ($env:DB_NAME) { $env:DB_NAME } else { "ltm_group04" }
$DB_USER = if ($env:DB_USER) { $env:DB_USER } else { "postgres" }
$DB_HOST = if ($env:DB_HOST) { $env:DB_HOST } else { "localhost" }
$DB_PORT = if ($env:DB_PORT) { $env:DB_PORT } else { "5432" }

Write-Host "Database: $DB_NAME" -ForegroundColor Yellow
Write-Host "User: $DB_USER" -ForegroundColor Yellow
Write-Host "Host: $($DB_HOST):$($DB_PORT)" -ForegroundColor Yellow
Write-Host ""

# Kiểm tra file SQL tồn tại
$sqlFile = "test_friend_request_flow.sql"
if (-not (Test-Path $sqlFile)) {
    Write-Host "ERROR: SQL file not found: $sqlFile" -ForegroundColor Red
    exit 1
}

# Chạy SQL script
Write-Host "Running test script..." -ForegroundColor Green
Write-Host "----------------------------------------" -ForegroundColor Gray

$env:PGPASSWORD = if ($env:PGPASSWORD) { $env:PGPASSWORD } else { "" }

$result = & psql -h $DB_HOST -p $DB_PORT -U $DB_USER -d $DB_NAME -f $sqlFile 2>&1

Write-Host $result

Write-Host ""
Write-Host "----------------------------------------" -ForegroundColor Gray

# Kiểm tra kết quả
Write-Host "Verifying results..." -ForegroundColor Green

$verifyQuery = @"
SELECT 
    CASE 
        WHEN COUNT(*) = 2 THEN 'SUCCESS: Both relationships are ACCEPTED'
        ELSE 'FAILED: Relationships are not correct'
    END as test_result
FROM friend_relationships
WHERE ((user_id = (SELECT user_id FROM users WHERE username = 'test_user_a') 
        AND peer_user_id = (SELECT user_id FROM users WHERE username = 'test_user_b'))
    OR (user_id = (SELECT user_id FROM users WHERE username = 'test_user_b') 
        AND peer_user_id = (SELECT user_id FROM users WHERE username = 'test_user_a')))
AND status = 'ACCEPTED';
"@

$verifyResult = & psql -h $DB_HOST -p $DB_PORT -U $DB_USER -d $DB_NAME -c $verifyQuery 2>&1

if ($verifyResult -match "SUCCESS") {
    Write-Host "`n✅ $verifyResult" -ForegroundColor Green
} else {
    Write-Host "`n❌ $verifyResult" -ForegroundColor Red
}

Write-Host ""
Write-Host "Final state:" -ForegroundColor Yellow

$finalQuery = @"
SELECT 
    u1.username as from_user,
    u2.username as to_user,
    fr.status,
    fr.created_at,
    fr.responded_at
FROM friend_relationships fr
JOIN users u1 ON fr.user_id = u1.user_id
JOIN users u2 ON fr.peer_user_id = u2.user_id
WHERE u1.username IN ('test_user_a', 'test_user_b')
   OR u2.username IN ('test_user_a', 'test_user_b')
ORDER BY fr.created_at;
"@

& psql -h $DB_HOST -p $DB_PORT -U $DB_USER -d $DB_NAME -c $finalQuery

Write-Host ""
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "Test completed!" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

