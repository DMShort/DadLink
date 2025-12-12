# Run client with logs saved to file
# Cascade can read the log file directly!

$logFile = "client_log.txt"

Write-Host "=== Starting VoIP Client with logging ===" -ForegroundColor Cyan
Write-Host "Logs will be saved to: $logFile" -ForegroundColor Yellow
Write-Host "Cascade can read this file directly!" -ForegroundColor Green
Write-Host ""
Write-Host "To run multiple clients:" -ForegroundColor Yellow
Write-Host "   Client 1: .\run_with_logs.ps1" -ForegroundColor White
Write-Host "   Client 2: .\run_client2.ps1 (saves to client2_log.txt)" -ForegroundColor White
Write-Host ""
Write-Host "Watch for these messages:" -ForegroundColor Cyan
Write-Host "   1. [OK] Voice session initialized (not started yet)" -ForegroundColor White
Write-Host "   2. [OK] Login SUCCESS! User ID: X" -ForegroundColor White
Write-Host "   3. [OK] Starting voice session now that channel is joined..." -ForegroundColor White
Write-Host "   4. [OK] Voice session started successfully!" -ForegroundColor Green
Write-Host ""

# Stop any existing client
Write-Host "Stopping any running client..." -ForegroundColor Yellow
Stop-Process -Name "voip-client" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

# Run client and tee output to both console and file
Write-Host "Starting client..." -ForegroundColor Green
Write-Host ""
cd build\Debug
.\voip-client.exe 2>&1 | Tee-Object -FilePath "..\..\$logFile"
