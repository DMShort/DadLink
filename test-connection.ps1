# VoIP Server - Connection Test Script

param(
    [string]$ServerIP = "127.0.0.1",
    [int]$Port = 9000
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "VoIP Server - Connection Test" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Testing connection to: $ServerIP`:$Port" -ForegroundColor Yellow
Write-Host ""

# Test 1: TCP Port
Write-Host "[1/5] Testing TCP port $Port..." -ForegroundColor Yellow
try {
    $tcpClient = New-Object System.Net.Sockets.TcpClient
    $connection = $tcpClient.BeginConnect($ServerIP, $Port, $null, $null)
    $wait = $connection.AsyncWaitHandle.WaitOne(3000, $false)
    
    if ($wait) {
        $tcpClient.EndConnect($connection)
        $tcpClient.Close()
        Write-Host "  [OK] TCP Port $Port is accessible" -ForegroundColor Green
    } else {
        Write-Host "  [X] TCP Port $Port timeout" -ForegroundColor Red
    }
} catch {
    Write-Host "  [X] TCP Port $Port is NOT accessible" -ForegroundColor Red
}

# Test 2: Check server process
Write-Host ""
Write-Host "[2/5] Checking if server is running..." -ForegroundColor Yellow
$serverProcess = Get-Process | Where-Object {$_.ProcessName -like "*voip*" -or $_.ProcessName -eq "cargo"}
if ($serverProcess) {
    Write-Host "  [OK] Server process found" -ForegroundColor Green
} else {
    Write-Host "  [X] No server process found" -ForegroundColor Red
}

# Test 3: Check firewall rules
Write-Host ""
Write-Host "[3/5] Checking firewall rules..." -ForegroundColor Yellow
$tcpRule = Get-NetFirewallRule -DisplayName "VoIP Server - WebSocket Control" -ErrorAction SilentlyContinue
$udpRule = Get-NetFirewallRule -DisplayName "VoIP Server - Voice UDP" -ErrorAction SilentlyContinue

if ($tcpRule) {
    Write-Host "  [OK] TCP firewall rule exists" -ForegroundColor Green
} else {
    Write-Host "  [X] TCP firewall rule missing" -ForegroundColor Red
}

if ($udpRule) {
    Write-Host "  [OK] UDP firewall rule exists" -ForegroundColor Green
} else {
    Write-Host "  [X] UDP firewall rule missing" -ForegroundColor Red
}

# Test 4: Check if port is listening
Write-Host ""
Write-Host "[4/5] Checking if port is listening..." -ForegroundColor Yellow
$listening = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue
if ($listening) {
    Write-Host "  [OK] Server is listening on port $Port" -ForegroundColor Green
} else {
    Write-Host "  [X] Nothing is listening on port $Port" -ForegroundColor Red
}

# Test 5: Network info
Write-Host ""
Write-Host "[5/5] Network Information..." -ForegroundColor Yellow

$localIP = (Get-NetIPAddress -AddressFamily IPv4 | Where-Object {$_.IPAddress -like "192.168.*" -or $_.IPAddress -like "10.*"} | Select-Object -First 1).IPAddress
if ($localIP) {
    Write-Host "  Local IP: $localIP" -ForegroundColor Cyan
}

try {
    $publicIP = (Invoke-WebRequest -Uri "https://api.ipify.org" -UseBasicParsing -TimeoutSec 5).Content
    Write-Host "  Public IP: $publicIP" -ForegroundColor Cyan
} catch {
    Write-Host "  Public IP: (could not determine)" -ForegroundColor Gray
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

if ($tcpClient -and $tcpRule -and $listening) {
    Write-Host "Status: [OK] Server should be accessible" -ForegroundColor Green
    Write-Host ""
    if ($publicIP) {
        Write-Host "Users can connect with:" -ForegroundColor Cyan
        Write-Host "  Server: $publicIP" -ForegroundColor White
        Write-Host "  Port: $Port" -ForegroundColor White
        Write-Host "  Use TLS: Yes" -ForegroundColor White
    }
} else {
    Write-Host "Status: [X] Server is NOT accessible" -ForegroundColor Red
    Write-Host ""
    Write-Host "Common fixes:" -ForegroundColor Yellow
    Write-Host "  1. Start the server" -ForegroundColor Gray
    Write-Host "  2. Run fix-firewall-clean.ps1 as Administrator" -ForegroundColor Gray
    Write-Host "  3. Configure router port forwarding" -ForegroundColor Gray
}

Write-Host ""
pause
