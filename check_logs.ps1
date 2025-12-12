# Quick script to check if all log files exist and show their status

Write-Host ""
Write-Host "=== VoIP System Log Files Status ===" -ForegroundColor Cyan
Write-Host ""

$serverLog = "server\server_log.txt"
$client1Log = "client\client_log.txt"
$client2Log = "client\client2_log.txt"

function Check-LogFile {
    param($path, $name)
    
    if (Test-Path $path) {
        $size = (Get-Item $path).Length
        $lastWrite = (Get-Item $path).LastWriteTime
        Write-Host "✅ $name" -ForegroundColor Green
        Write-Host "   Path: $path" -ForegroundColor Gray
        Write-Host "   Size: $([math]::Round($size/1KB, 2)) KB" -ForegroundColor Gray
        Write-Host "   Last updated: $lastWrite" -ForegroundColor Gray
    } else {
        Write-Host "❌ $name - NOT FOUND" -ForegroundColor Red
        Write-Host "   Expected: $path" -ForegroundColor Gray
        Write-Host "   Start the process with logging script to create it" -ForegroundColor Yellow
    }
    Write-Host ""
}

Check-LogFile $serverLog "Server Log"
Check-LogFile $client1Log "Client 1 Log"
Check-LogFile $client2Log "Client 2 Log"

Write-Host "=== Quick Commands ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "View server log:" -ForegroundColor Yellow
Write-Host "  Get-Content server\server_log.txt -Tail 50" -ForegroundColor White
Write-Host ""
Write-Host "View client 1 log:" -ForegroundColor Yellow
Write-Host "  Get-Content client\client_log.txt -Tail 50" -ForegroundColor White
Write-Host ""
Write-Host "View client 2 log:" -ForegroundColor Yellow
Write-Host "  Get-Content client\client2_log.txt -Tail 50" -ForegroundColor White
Write-Host ""
Write-Host "Search for errors in server log:" -ForegroundColor Yellow
Write-Host "  Select-String -Path server\server_log.txt -Pattern 'ERROR|WARN|alone'" -ForegroundColor White
Write-Host ""
Write-Host "Tell Cascade to analyze:" -ForegroundColor Yellow
Write-Host '  "analyze all log files for the intermittent issue"' -ForegroundColor White
Write-Host ""
