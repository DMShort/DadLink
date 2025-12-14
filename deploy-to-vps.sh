#!/bin/bash
# VoIP Server Deployment Script for DigitalOcean VPS
# Run this on your VPS after SSH login

set -e  # Exit on error

echo "==> VoIP Server Deployment Script"
echo "==> VPS: 170.64.133.51"
echo ""

# Clone repository if not exists
if [ ! -d "/root/DadLink" ]; then
    echo "==> Cloning repository from GitHub..."
    cd /root
    git clone https://github.com/DMShort/DadLink.git
else
    echo "==> Repository already exists, pulling latest changes..."
    cd /root/DadLink
    git pull
fi

cd /root/DadLink

# Generate secure secrets
echo "==> Generating secure secrets..."
POSTGRES_PASSWORD=$(openssl rand -base64 32)
JWT_SECRET=$(openssl rand -base64 48)

# Create .env file
echo "==> Creating .env file..."
cat > .env <<EOF
# PostgreSQL Database Password
POSTGRES_PASSWORD=${POSTGRES_PASSWORD}

# JWT Secret for authentication tokens
JWT_SECRET=${JWT_SECRET}
EOF

echo "==> Environment file created"
echo "==> PostgreSQL Password: ${POSTGRES_PASSWORD}"
echo "==> JWT Secret: ${JWT_SECRET}"
echo ""
echo "==> IMPORTANT: Save these credentials somewhere safe!"
echo ""

# Build and start containers
echo "==> Building Docker images (this may take 5-10 minutes)..."
docker compose build

echo "==> Starting containers..."
docker compose up -d

# Wait for containers to start
echo "==> Waiting for containers to initialize..."
sleep 10

# Check status
echo "==> Container status:"
docker compose ps

echo ""
echo "==> Checking logs..."
docker compose logs --tail=50 voip-server

echo ""
echo "======================================"
echo "==> Deployment Complete!"
echo "======================================"
echo ""
echo "Your VoIP server is running at: 170.64.133.51"
echo "WebSocket Port: 9000"
echo "Voice Port: 9001 (UDP)"
echo ""
echo "Next steps:"
echo "1. Update your client to connect to: 170.64.133.51:9000"
echo "2. Test connection from your Windows machine"
echo "3. Monitor logs: docker compose logs -f voip-server"
echo ""
echo "To stop the server: docker compose down"
echo "To restart: docker compose restart"
echo "To view logs: docker compose logs -f"
echo ""
