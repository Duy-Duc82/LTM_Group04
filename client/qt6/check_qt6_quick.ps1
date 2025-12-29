# Script to check if Qt6 Quick is installed
Write-Host "=== Checking Qt6 Quick Installation ===" -ForegroundColor Cyan
Write-Host ""

# Common Qt6 installation paths
$qtPaths = @(
    "C:\Qt\6.10.1\mingw_64",
    "C:\Qt\6.10.0\mingw_64",
    "C:\Qt\6.9.2\mingw_64",
    "C:\Qt\6.10.1\msvc2022_64"
)

$foundQt = $false
foreach ($qtPath in $qtPaths) {
    if (Test-Path $qtPath) {
        Write-Host "Found Qt6 at: $qtPath" -ForegroundColor Green
        $foundQt = $true
        
        # Check for QML directory
        $qmlPath = Join-Path $qtPath "qml"
        if (Test-Path $qmlPath) {
            Write-Host "  ✓ QML directory exists: $qmlPath" -ForegroundColor Green
            
            # Check for QtQuick module
            $qtQuickPath = Join-Path $qmlPath "QtQuick"
            if (Test-Path $qtQuickPath) {
                Write-Host "  ✓ QtQuick module found" -ForegroundColor Green
                
                # List version directories
                $versions = Get-ChildItem $qtQuickPath -Directory | Where-Object { $_.Name -match '^\d+\.\d+$' }
                if ($versions) {
                    Write-Host "    Available versions:" -ForegroundColor Yellow
                    foreach ($ver in $versions) {
                        Write-Host "      - $($ver.Name)" -ForegroundColor White
                    }
                }
            } else {
                Write-Host "  ✗ QtQuick module NOT found" -ForegroundColor Red
                Write-Host "    Expected path: $qtQuickPath" -ForegroundColor Yellow
            }
            
            # Check for QtQuick.Controls
            $controlsPath = Join-Path $qmlPath "QtQuick\Controls"
            if (Test-Path $controlsPath) {
                Write-Host "  ✓ QtQuick.Controls module found" -ForegroundColor Green
            } else {
                Write-Host "  ✗ QtQuick.Controls module NOT found" -ForegroundColor Red
            }
            
            # Check for QtQuick.Layouts
            $layoutsPath = Join-Path $qmlPath "QtQuick\Layouts"
            if (Test-Path $layoutsPath) {
                Write-Host "  ✓ QtQuick.Layouts module found" -ForegroundColor Green
            } else {
                Write-Host "  ✗ QtQuick.Layouts module NOT found" -ForegroundColor Red
            }
        } else {
            Write-Host "  ✗ QML directory NOT found: $qmlPath" -ForegroundColor Red
        }
        
        Write-Host ""
    }
}

if (-not $foundQt) {
    Write-Host "✗ Qt6 installation NOT found in common paths" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please check:" -ForegroundColor Yellow
    Write-Host "  1. Qt6 is installed" -ForegroundColor White
    Write-Host "  2. Qt6 Quick modules are installed (via Qt Maintenance Tool)" -ForegroundColor White
    Write-Host ""
    Write-Host "To install Qt6 Quick:" -ForegroundColor Cyan
    Write-Host "  1. Open Qt Maintenance Tool" -ForegroundColor White
    Write-Host "  2. Select 'Add or remove components'" -ForegroundColor White
    Write-Host "  3. Find 'Qt 6.10.1' → MinGW 64-bit'" -ForegroundColor White
    Write-Host "  4. Check:" -ForegroundColor White
    Write-Host "     - Qt Quick" -ForegroundColor White
    Write-Host "     - Qt Quick Controls" -ForegroundColor White
    Write-Host "     - Qt Quick Layouts" -ForegroundColor White
    Write-Host "  5. Click 'Apply' to install" -ForegroundColor White
}

Write-Host ""
Write-Host "=== Check Complete ===" -ForegroundColor Cyan

