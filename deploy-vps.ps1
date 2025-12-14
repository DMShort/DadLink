# VPS Deployment Script
# Deploys the VoIP server to DigitalOcean VPS

$VPS_IP = "170.64.133.51"
$VPS_USER = "root"

Write-Host "🚀 Deploying VoIP Server to VPS at $VPS_IP" -ForegroundColor Cyan

# Step 1: Copy docker-compose file to VPS
Write-Host "`n📦 Step 1: Copying docker-compose configuration..." -ForegroundColor Yellow
scp docker-compose.vps.yml "${VPS_USER}@${VPS_IP}:/root/docker-compose.yml"

if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Failed to copy docker-compose file" -ForegroundColor Red
    exit 1
}

# Step 2: Pull latest image and start containers
Write-Host "`n🐳 Step 2: Deploying containers on VPS..." -ForegroundColor Yellow
ssh "${VPS_USER}@${VPS_IP}" @'
# Login to GitHub Container Registry (public image, but login may be needed)
echo "Pulling latest VoIP server image from GitHub Container Registry..."
docker pull ghcr.io/dmshort/dadlink/voip-server:latest

# Stop existing containers if running
echo "Stopping existing containers..."
docker-compose down 2>/dev/null || true

# Start all services
echo "Starting services..."
docker-compose up -d

# Wait for services to be healthy
echo "Waiting for services to start..."
sleep 10

# Show status
echo -e "\n=== Service Status ==="
docker-compose ps

# Show server logs
echo -e "\n=== VoIP Server Logs (last 20 lines) ==="
docker-compose logs --tail=20 voip-server
'@

if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Deployment failed" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Deployment complete!" -ForegroundColor Green
Write-Host ""
Write-Host "Server is running at:" -ForegroundColor Cyan
Write-Host "  WebSocket: ws://$VPS_IP:9000" -ForegroundColor White
Write-Host "  Voice UDP: $VPS_IP:9001" -ForegroundColor White
