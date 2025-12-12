# Run server with logs saved to file
# Cascade can read the log file directly!

$logFile = "server_log.txt"

Write-Host "=== Starting VoIP Server with logging ===" -ForegroundColor Cyan
Write-Host "Logs will be saved to: $logFile" -ForegroundColor Yellow
Write-Host "Cascade can read this file directly!" -ForegroundColor Green
Write-Host ""

# Stop any existing server
Write-Host "Stopping any existing server..." -ForegroundColor Yellow
Stop-Process -Name "voip_server" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2

# Clear old log file
if (Test-Path $logFile) {
    Remove-Item $logFile
    Write-Host "Cleared old log file" -ForegroundColor Yellow
}

# Run server (use release build for better performance)
Write-Host "Starting server (release build)..." -ForegroundColor Green
Write-Host "Press Ctrl+C to stop. Logs saving to: $logFile" -ForegroundColor Yellow
Write-Host ""
cargo run --release 2>&1 | Tee-Object -FilePath $logFile
