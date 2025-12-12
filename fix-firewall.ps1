# VoIP Server - Firewall Configuration Script
# Run this as Administrator to allow incoming connections

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "VoIP Server - Firewall Configuration" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if running as administrator
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "ERROR: This script must be run as Administrator!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Right-click PowerShell and select Run as Administrator" -ForegroundColor Yellow
    Write-Host ""
    pause
    exit 1
}

Write-Host "[1/4] Checking existing firewall rules..." -ForegroundColor Yellow
$existingTCP = Get-NetFirewallRule -DisplayName "VoIP Server - WebSocket Control" -ErrorAction SilentlyContinue
$existingUDP = Get-NetFirewallRule -DisplayName "VoIP Server - Voice UDP" -ErrorAction SilentlyContinue

if ($existingTCP) {
    Write-Host "  Removing old TCP rule..." -ForegroundColor Gray
    Remove-NetFirewallRule -DisplayName "VoIP Server - WebSocket Control"
}

if ($existingUDP) {
    Write-Host "  Removing old UDP rule..." -ForegroundColor Gray
    Remove-NetFirewallRule -DisplayName "VoIP Server - Voice UDP"
}

Write-Host "[2/4] Creating TCP firewall rule (Port 9000)..." -ForegroundColor Yellow
try {
    New-NetFirewallRule -DisplayName "VoIP Server - WebSocket Control" -Direction Inbound -LocalPort 9000 -Protocol TCP -Action Allow -Profile Any -Enabled True | Out-Null
    Write-Host "  [OK] TCP Port 9000 allowed" -ForegroundColor Green
} catch {
    Write-Host "  [ERROR] Failed to create TCP rule" -ForegroundColor Red
    exit 1
}

Write-Host "[3/4] Creating UDP firewall rule (Port 9001)..." -ForegroundColor Yellow
try {
    New-NetFirewallRule -DisplayName "VoIP Server - Voice UDP" -Direction Inbound -LocalPort 9001 -Protocol UDP -Action Allow -Profile Any -Enabled True | Out-Null
    Write-Host "  [OK] UDP Port 9001 allowed" -ForegroundColor Green
} catch {
    Write-Host "  [ERROR] Failed to create UDP rule" -ForegroundColor Red
    exit 1
}

Write-Host "[4/4] Verifying rules..." -ForegroundColor Yellow
$tcpRule = Get-NetFirewallRule -DisplayName "VoIP Server - WebSocket Control" -ErrorAction SilentlyContinue
$udpRule = Get-NetFirewallRule -DisplayName "VoIP Server - Voice UDP" -ErrorAction SilentlyContinue

if ($tcpRule -and $udpRule) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "Firewall Configuration Complete!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Firewall rules created:" -ForegroundColor Cyan
    Write-Host "  [OK] TCP Port 9000 (WebSocket Control)" -ForegroundColor Green
    Write-Host "  [OK] UDP Port 9001 (Voice Packets)" -ForegroundColor Green
    Write-Host ""
} else {
    Write-Host "ERROR: Rules were not created successfully" -ForegroundColor Red
    exit 1
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Next Steps" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "1. Windows Firewall: [OK] Configured" -ForegroundColor Green
Write-Host ""
Write-Host "2. Router Port Forwarding:" -ForegroundColor Yellow
Write-Host "   Forward these ports to your computer:" -ForegroundColor Gray
Write-Host "   - TCP 9000" -ForegroundColor Gray
Write-Host "   - UDP 9001" -ForegroundColor Gray
Write-Host ""

pause
