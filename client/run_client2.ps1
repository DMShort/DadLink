# Run second client instance with separate log file
# Useful for multi-user testing

$logFile = "client2_log.txt"

Write-Host "=== Starting VoIP Client #2 with logging ===" -ForegroundColor Magenta
Write-Host "Logs will be saved to: $logFile" -ForegroundColor Yellow
Write-Host "Cascade can read this file directly!" -ForegroundColor Green
Write-Host ""
Write-Host "This is CLIENT #2 - Login as a different user!" -ForegroundColor Cyan
Write-Host "   Suggestion: Login as 'bob' if Client #1 is 'dave'" -ForegroundColor White
Write-Host ""
Write-Host "Watch for these messages:" -ForegroundColor Cyan
Write-Host "   1. [OK] Voice session initialized (not started yet)" -ForegroundColor White
Write-Host "   2. [OK] Login SUCCESS! User ID: X" -ForegroundColor White
Write-Host "   3. [OK] Starting voice session now that channel is joined..." -ForegroundColor White
Write-Host "   4. [OK] Voice session started successfully!" -ForegroundColor Green
Write-Host ""

# Don't stop existing clients - we want multiple instances!

# Run client and tee output to both console and file
Write-Host "Starting client #2..." -ForegroundColor Magenta
Write-Host ""
cd build\Debug
.\voip-client.exe 2>&1 | Tee-Object -FilePath "..\..\$logFile"
