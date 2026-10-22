# MQTT Monitor Dashboard - Auto Start Script
# PowerShell Version

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  MQTT Monitor Dashboard" -ForegroundColor Cyan
Write-Host "  Starting Development Server..." -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if node_modules exists
if (-not (Test-Path "node_modules")) {
    Write-Host "[1/3] Installing dependencies..." -ForegroundColor Yellow
    npm install
    if ($LASTEXITCODE -ne 0) {
        Write-Host ""
        Write-Host "ERROR: Failed to install dependencies!" -ForegroundColor Red
        Read-Host "Press Enter to exit"
        exit 1
    }
    Write-Host ""
} else {
    Write-Host "[1/3] Dependencies already installed" -ForegroundColor Green
    Write-Host ""
}

# Kill any existing Vite processes
Get-Process node -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue

Write-Host "[2/3] Starting Vite development server..." -ForegroundColor Yellow

# Start Vite in a new window
Start-Process powershell -ArgumentList "-NoExit", "-Command", "npm run dev"

Write-Host "[3/3] Waiting for server to start..." -ForegroundColor Yellow
Start-Sleep -Seconds 3

Write-Host ""
Write-Host "Opening browser..." -ForegroundColor Green
Start-Process "http://localhost:5173"

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  Server started successfully!" -ForegroundColor Green
Write-Host "  URL: http://localhost:5173" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Press Ctrl+C in the server window to stop" -ForegroundColor Gray
Write-Host ""
Write-Host "Press any key to close this window..." -ForegroundColor Gray
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
