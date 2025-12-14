# VoIP Server Deployment Script for Windows
# Deploys DadLink to DigitalOcean VPS at 170.64.133.51

$VPS_IP = "170.64.133.51"
$VPS_USER = "root"

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "DadLink VoIP Server - VPS Deployment" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Target VPS: $VPS_IP" -ForegroundColor Yellow
Write-Host ""

# Transfer deployment script
Write-Host "==> Transferring deployment script to VPS..." -ForegroundColor Green
scp deploy-to-vps.sh ${VPS_USER}@${VPS_IP}:/root/deploy.sh

# Make it executable and run it
Write-Host "==> Running deployment on VPS..." -ForegroundColor Green
ssh ${VPS_USER}@${VPS_IP} "chmod +x /root/deploy.sh && /root/deploy.sh"

Write-Host ""
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "Deployment Complete!" -ForegroundColor Green
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Your VoIP server is now running at: $VPS_IP" -ForegroundColor Yellow
Write-Host ""
Write-Host "To test connection from Windows:" -ForegroundColor White
Write-Host "  Test-NetConnection -ComputerName $VPS_IP -Port 9000" -ForegroundColor Gray
Write-Host ""
Write-Host "To view server logs:" -ForegroundColor White
Write-Host "  ssh ${VPS_USER}@${VPS_IP} 'cd /root/DadLink && docker compose logs -f voip-server'" -ForegroundColor Gray
Write-Host ""
