# Deployment Guide
## Gaming-Focused Multi-Channel VoIP System

---

## Deployment Models

### 1. Self-Hosted (Docker)
For organizations that want full control over their infrastructure.

### 2. Self-Hosted (Standalone Binary)
For simpler deployments without container orchestration.

### 3. Managed Hosting (Multi-Tenant SaaS)
For organizations that prefer hosted solution.

---

## Self-Hosted Docker Deployment

### Prerequisites

- Docker 20.10+ and Docker Compose 2.0+
- 2 CPU cores minimum (4+ recommended)
- 4 GB RAM minimum (8 GB+ recommended)
- 20 GB disk space
- Open ports: 9000 (TLS), 9001 (UDP)

### Quick Start

**1. Clone Repository**
```bash
git clone https://github.com/yourorg/voip-system.git
cd voip-system
```

**2. Configure Environment**
```bash
cp .env.example .env
nano .env
```

**.env File:**
```bash
# Server Configuration
SERVER_BIND_ADDRESS=0.0.0.0
CONTROL_PORT=9000
VOICE_PORT=9001
MAX_CONNECTIONS=1000

# Database
POSTGRES_HOST=postgres
POSTGRES_PORT=5432
POSTGRES_USER=voip
POSTGRES_PASSWORD=change_me_in_production
POSTGRES_DB=voip

# Redis
REDIS_HOST=redis
REDIS_PORT=6379

# Security
JWT_SECRET=generate_random_secret_here
TLS_CERT_PATH=/certs/cert.pem
TLS_KEY_PATH=/certs/key.pem

# Audio
MAX_BITRATE=128000
DEFAULT_BITRATE=32000

# Limits
MAX_CHANNELS_PER_ORG=100
MAX_USERS_PER_ORG=1000
```

**3. Generate TLS Certificates**
```bash
# Self-signed for testing
openssl req -x509 -newkey rsa:4096 -nodes \
  -keyout certs/key.pem \
  -out certs/cert.pem \
  -days 365 \
  -subj "/CN=voip.example.com"

# Production: use Let's Encrypt
certbot certonly --standalone -d voip.example.com
```

**4. Start Services**
```bash
docker-compose up -d
```

### docker-compose.yml

```yaml
version: '3.8'

services:
  voip-server:
    image: voip-server:latest
    container_name: voip-server
    ports:
      - "${CONTROL_PORT}:9000"
      - "${VOICE_PORT}:9001/udp"
    environment:
      - SERVER_BIND_ADDRESS=${SERVER_BIND_ADDRESS}
      - CONTROL_PORT=${CONTROL_PORT}
      - VOICE_PORT=${VOICE_PORT}
      - DATABASE_URL=postgresql://${POSTGRES_USER}:${POSTGRES_PASSWORD}@postgres:5432/${POSTGRES_DB}
      - REDIS_URL=redis://redis:6379
      - JWT_SECRET=${JWT_SECRET}
    volumes:
      - ./certs:/certs:ro
      - ./config:/config:ro
    depends_on:
      - postgres
      - redis
    restart: unless-stopped
    networks:
      - voip-network

  postgres:
    image: postgres:15-alpine
    container_name: voip-postgres
    environment:
      - POSTGRES_USER=${POSTGRES_USER}
      - POSTGRES_PASSWORD=${POSTGRES_PASSWORD}
      - POSTGRES_DB=${POSTGRES_DB}
    volumes:
      - postgres-data:/var/lib/postgresql/data
      - ./schema.sql:/docker-entrypoint-initdb.d/schema.sql
    restart: unless-stopped
    networks:
      - voip-network

  redis:
    image: redis:7-alpine
    container_name: voip-redis
    command: redis-server --appendonly yes
    volumes:
      - redis-data:/data
    restart: unless-stopped
    networks:
      - voip-network

  # Optional: Admin Web UI
  admin-ui:
    image: voip-admin-ui:latest
    container_name: voip-admin-ui
    ports:
      - "8080:80"
    environment:
      - API_URL=http://voip-server:9000
    depends_on:
      - voip-server
    restart: unless-stopped
    networks:
      - voip-network

volumes:
  postgres-data:
  redis-data:

networks:
  voip-network:
    driver: bridge
```

### Database Initialization

**schema.sql:**
```sql
-- See SERVER_DESIGN.md for full schema
CREATE TABLE IF NOT EXISTS organizations (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    tag VARCHAR(16) UNIQUE NOT NULL,
    owner_id INTEGER,
    created_at TIMESTAMP DEFAULT NOW(),
    max_users INTEGER DEFAULT 100,
    max_channels INTEGER DEFAULT 50
);

-- Additional tables...
```

### Health Checks

```bash
# Check server status
curl https://voip.example.com:9000/health

# Expected response:
# {"status": "healthy", "version": "1.0.0"}

# Check database connectivity
docker exec voip-server /usr/local/bin/check-db

# Check logs
docker logs voip-server -f
```

---

## Self-Hosted Standalone Binary

### Prerequisites

- Linux (Ubuntu 20.04+) or Windows Server 2019+
- PostgreSQL 12+
- Redis 6+
- Systemd (Linux) or Windows Service

### Installation (Linux)

**1. Download Binary**
```bash
wget https://releases.voip-system.com/v1.0.0/voip-server-linux-x64.tar.gz
tar -xzf voip-server-linux-x64.tar.gz
sudo mv voip-server /usr/local/bin/
sudo chmod +x /usr/local/bin/voip-server
```

**2. Create Configuration**
```bash
sudo mkdir -p /etc/voip
sudo nano /etc/voip/config.yaml
```

**/etc/voip/config.yaml:**
```yaml
server:
  bind_address: "0.0.0.0"
  control_port: 9000
  voice_port: 9001
  max_connections: 1000

database:
  url: "postgresql://voip:password@localhost/voip"
  max_connections: 20

redis:
  url: "redis://localhost:6379"

security:
  tls_cert: "/etc/voip/certs/cert.pem"
  tls_key: "/etc/voip/certs/key.pem"
  jwt_secret: "generate_random_secret"

audio:
  max_bitrate: 128000
  default_bitrate: 32000

limits:
  max_channels_per_org: 100
  max_users_per_org: 1000
```

**3. Create Systemd Service**
```bash
sudo nano /etc/systemd/system/voip-server.service
```

**/etc/systemd/system/voip-server.service:**
```ini
[Unit]
Description=VoIP Server
After=network.target postgresql.service redis.service

[Service]
Type=simple
User=voip
Group=voip
ExecStart=/usr/local/bin/voip-server --config /etc/voip/config.yaml
Restart=on-failure
RestartSec=10
StandardOutput=journal
StandardError=journal

# Security hardening
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/var/lib/voip

[Install]
WantedBy=multi-user.target
```

**4. Create User & Directories**
```bash
sudo useradd -r -s /bin/false voip
sudo mkdir -p /var/lib/voip
sudo chown voip:voip /var/lib/voip
```

**5. Start Service**
```bash
sudo systemctl daemon-reload
sudo systemctl enable voip-server
sudo systemctl start voip-server
sudo systemctl status voip-server
```

### Installation (Windows)

**1. Download and Extract**
```powershell
Invoke-WebRequest -Uri https://releases.voip-system.com/v1.0.0/voip-server-windows-x64.zip -OutFile voip-server.zip
Expand-Archive voip-server.zip -DestinationPath C:\VoIP
```

**2. Install as Windows Service**
```powershell
# Using NSSM (Non-Sucking Service Manager)
nssm install VoIPServer C:\VoIP\voip-server.exe
nssm set VoIPServer AppParameters "--config C:\VoIP\config.yaml"
nssm set VoIPServer AppDirectory C:\VoIP
nssm set VoIPServer DisplayName "VoIP Server"
nssm set VoIPServer Start SERVICE_AUTO_START

# Start service
Start-Service VoIPServer
```

---

## Managed Hosting (Multi-Tenant)

### Architecture

```
                ┌─────────────┐
                │   CDN       │
                │ (Static)    │
                └──────┬──────┘
                       │
                ┌──────┴──────┐
Internet ──────→│Load Balancer│
                │  (HAProxy)  │
                └──────┬──────┘
                       │
        ┌──────────────┼──────────────┐
        ↓              ↓              ↓
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│ VoIP Server  │ │ VoIP Server  │ │ VoIP Server  │
│  Instance 1  │ │  Instance 2  │ │  Instance 3  │
└──────┬───────┘ └──────┬───────┘ └──────┬───────┘
       │                │                │
       └────────────────┼────────────────┘
                        │
        ┌───────────────┴───────────────┐
        ↓                               ↓
┌──────────────┐                ┌──────────────┐
│ PostgreSQL   │                │   Redis      │
│  (Primary +  │                │  Cluster     │
│   Replicas)  │                │              │
└──────────────┘                └──────────────┘
```

### Kubernetes Deployment

**voip-deployment.yaml:**
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: voip-server
  namespace: voip
spec:
  replicas: 3
  selector:
    matchLabels:
      app: voip-server
  template:
    metadata:
      labels:
        app: voip-server
    spec:
      containers:
      - name: voip-server
        image: voip-server:1.0.0
        ports:
        - containerPort: 9000
          name: control
          protocol: TCP
        - containerPort: 9001
          name: voice
          protocol: UDP
        env:
        - name: DATABASE_URL
          valueFrom:
            secretKeyRef:
              name: voip-secrets
              key: database-url
        - name: REDIS_URL
          valueFrom:
            secretKeyRef:
              name: voip-secrets
              key: redis-url
        - name: JWT_SECRET
          valueFrom:
            secretKeyRef:
              name: voip-secrets
              key: jwt-secret
        resources:
          requests:
            memory: "2Gi"
            cpu: "1000m"
          limits:
            memory: "4Gi"
            cpu: "2000m"
        livenessProbe:
          httpGet:
            path: /health
            port: 9000
            scheme: HTTPS
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /ready
            port: 9000
            scheme: HTTPS
          initialDelaySeconds: 5
          periodSeconds: 5
---
apiVersion: v1
kind: Service
metadata:
  name: voip-server-control
  namespace: voip
spec:
  type: LoadBalancer
  ports:
  - port: 9000
    targetPort: 9000
    protocol: TCP
    name: control
  selector:
    app: voip-server
---
apiVersion: v1
kind: Service
metadata:
  name: voip-server-voice
  namespace: voip
spec:
  type: LoadBalancer
  ports:
  - port: 9001
    targetPort: 9001
    protocol: UDP
    name: voice
  selector:
    app: voip-server
```

---

## Client Distribution

### Windows Installer

**Build with NSIS or WiX:**
```nsis
; voip-client-installer.nsi
OutFile "VoIP-Client-Setup.exe"
InstallDir "$PROGRAMFILES64\VoIP Client"

Section "Install"
    SetOutPath "$INSTDIR"
    File "voip-client.exe"
    File "*.dll"
    File "config.json"
    
    CreateDirectory "$SMPROGRAMS\VoIP Client"
    CreateShortcut "$SMPROGRAMS\VoIP Client\VoIP Client.lnk" "$INSTDIR\voip-client.exe"
    CreateShortcut "$DESKTOP\VoIP Client.lnk" "$INSTDIR\voip-client.exe"
    
    WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd
```

### Auto-Update

**Client-side update checker:**
```cpp
class UpdateChecker {
public:
    void check_for_updates() {
        // Fetch latest version from server
        auto latest = fetch_latest_version("https://updates.voip-system.com/latest.json");
        
        if (latest.version > current_version) {
            // Prompt user to download update
            show_update_notification(latest);
        }
    }
};
```

---

## Monitoring & Alerting

### Prometheus Metrics

**Server exposes metrics on `/metrics` endpoint:**
```
# Active connections
voip_active_connections 142

# Packets per second
voip_packets_forwarded_total 15420

# Routing latency (histogram)
voip_routing_latency_seconds{quantile="0.5"} 0.003
voip_routing_latency_seconds{quantile="0.95"} 0.008
voip_routing_latency_seconds{quantile="0.99"} 0.015

# Per-org metrics
voip_org_users{org="testorg"} 25
voip_org_channels{org="testorg"} 12
```

### Grafana Dashboard

**Key Panels:**
- Active connections over time
- Packets/second
- Routing latency (p50, p95, p99)
- CPU & memory usage
- Database query time
- Per-org bandwidth usage

### Alerting Rules

```yaml
groups:
- name: voip_alerts
  rules:
  - alert: HighRoutingLatency
    expr: voip_routing_latency_seconds{quantile="0.95"} > 0.010
    for: 5m
    annotations:
      summary: "High routing latency detected"
  
  - alert: HighConnectionCount
    expr: voip_active_connections > 900
    for: 5m
    annotations:
      summary: "Approaching connection limit"
  
  - alert: ServerDown
    expr: up{job="voip-server"} == 0
    for: 1m
    annotations:
      summary: "VoIP server is down"
```

---

## Backup & Recovery

### Database Backups

```bash
# Daily backup script
#!/bin/bash
BACKUP_DIR="/var/backups/voip"
DATE=$(date +%Y%m%d_%H%M%S)

pg_dump -h localhost -U voip voip | gzip > "$BACKUP_DIR/voip_$DATE.sql.gz"

# Keep only last 7 days
find "$BACKUP_DIR" -name "voip_*.sql.gz" -mtime +7 -delete
```

### Configuration Backups

```bash
# Backup config and certs
tar -czf config-backup-$(date +%Y%m%d).tar.gz \
    /etc/voip/config.yaml \
    /etc/voip/certs/ \
    /var/lib/voip/
```

### Disaster Recovery

**Recovery Time Objective (RTO)**: <1 hour  
**Recovery Point Objective (RPO)**: <24 hours

**Recovery Steps:**
1. Provision new server
2. Restore database from latest backup
3. Restore configuration files
4. Start services
5. Verify health checks
6. Update DNS if needed

---

## Security Hardening

### Firewall Rules

```bash
# UFW (Ubuntu)
sudo ufw allow 9000/tcp   # TLS control
sudo ufw allow 9001/udp   # Voice
sudo ufw enable

# iptables
iptables -A INPUT -p tcp --dport 9000 -j ACCEPT
iptables -A INPUT -p udp --dport 9001 -j ACCEPT
```

### Fail2Ban

**/etc/fail2ban/jail.d/voip.conf:**
```ini
[voip]
enabled = true
port = 9000
filter = voip
logpath = /var/log/voip/server.log
maxretry = 5
bantime = 3600
```

### SSL/TLS Best Practices

- Use TLS 1.3 only
- Strong cipher suites
- HSTS headers
- Certificate pinning (optional)

---

See `ARCHITECTURE.md` for system overview and `TESTING_STRATEGY.md` for validation procedures.
