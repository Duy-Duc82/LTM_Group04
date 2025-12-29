# PowerShell script to set environment variables
# Usage: . .\setup_env.ps1

# Default PostgreSQL connection string
$env:DB_CONN = "host=localhost port=5432 dbname=ltm_group04 user=postgres password=1"

# Optional: Server mode
$env:SERVER_MODE = "0"  # Set to "1" to run as server
$env:SERVER_PORT = "9000"

Write-Host "Environment variables set:"
Write-Host "  DB_CONN=$env:DB_CONN"
Write-Host "  SERVER_MODE=$env:SERVER_MODE"
Write-Host "  SERVER_PORT=$env:SERVER_PORT"
Write-Host ""
Write-Host "Note: These variables are only set for the current PowerShell session"
Write-Host "To make permanent, add to your PowerShell profile or use System Properties"

