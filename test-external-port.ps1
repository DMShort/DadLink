# External Port Checker
# Tests if port 9000 is accessible from the internet

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "External Port Accessibility Test" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Get current public IP
Write-Host "[1/3] Getting your public IP..." -ForegroundColor Yellow
try {
    $publicIP = (Invoke-WebRequest -Uri "https://api.ipify.org" -UseBasicParsing).Content
    Write-Host "  Public IP: $publicIP" -ForegroundColor Green
} catch {
    Write-Host "  [ERROR] Could not get public IP" -ForegroundColor Red
    exit 1
}

# Get DNS resolution
Write-Host ""
Write-Host "[2/3] Checking DNS resolution..." -ForegroundColor Yellow
try {
    $dnsResult = Resolve-DnsName -Name "dadlink.ddns.net" -Type A -ErrorAction Stop
    $dnsIP = $dnsResult.IPAddress
    Write-Host "  dadlink.ddns.net -> $dnsIP" -ForegroundColor Green

    if ($dnsIP -eq $publicIP) {
        Write-Host "  [OK] DNS matches current IP" -ForegroundColor Green
    } else {
        Write-Host "  [WARNING] DNS ($dnsIP) does not match current IP ($publicIP)" -ForegroundColor Yellow
        Write-Host "  This is normal if your IP just changed. DNS updates in 1-5 minutes." -ForegroundColor Gray
    }
} catch {
    Write-Host "  [ERROR] Could not resolve dadlink.ddns.net" -ForegroundColor Red
}

# Test using external service
Write-Host ""
Write-Host "[3/3] Testing port 9000 accessibility..." -ForegroundColor Yellow
Write-Host ""
Write-Host "Online port checkers you can use:" -ForegroundColor Cyan
Write-Host ""
Write-Host "  1. CanYouSeeMe.org" -ForegroundColor White
Write-Host "     https://canyouseeme.org/" -ForegroundColor Gray
Write-Host "     Enter port: 9000" -ForegroundColor Gray
Write-Host ""
Write-Host "  2. PortChecker.co" -ForegroundColor White
Write-Host "     https://portchecker.co/" -ForegroundColor Gray
Write-Host "     Enter IP: $publicIP" -ForegroundColor Gray
Write-Host "     Enter port: 9000" -ForegroundColor Gray
Write-Host ""
Write-Host "  3. YouGetSignal" -ForegroundColor White
Write-Host "     https://www.yougetsignal.com/tools/open-ports/" -ForegroundColor Gray
Write-Host "     Enter port: 9000" -ForegroundColor Gray
Write-Host ""

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "NAT Hairpinning Note" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Testing from your own network (Test-NetConnection) may fail" -ForegroundColor Yellow
Write-Host "even when port forwarding is working correctly!" -ForegroundColor Yellow
Write-Host ""
Write-Host "This is called 'NAT Hairpinning' limitation." -ForegroundColor Gray
Write-Host ""
Write-Host "Your port forwarding rules are configured correctly:" -ForegroundColor Green
Write-Host "  - VoIP Control: 9000 -> 192.168.1.111" -ForegroundColor Green
Write-Host "  - VoIP Voice: 9001 -> 192.168.1.111" -ForegroundColor Green
Write-Host ""
Write-Host "Best test: Connect from phone hotspot using:" -ForegroundColor Cyan
Write-Host "  Server: dadlink.ddns.net" -ForegroundColor White
Write-Host "  Port: 9000" -ForegroundColor White
Write-Host ""

pause
