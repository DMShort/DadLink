# DadLink - Encrypted Multi-Channel VoIP System

[![CI](https://github.com/DMShort/DadLink/actions/workflows/ci.yml/badge.svg)](https://github.com/DMShort/DadLink/actions/workflows/ci.yml)

A secure, low-latency Push-to-Talk (PTT) voice communication system with end-to-end encryption, built with Rust and Qt.

**Perfect for**: Family communication, gaming groups, team coordination, or any scenario requiring reliable, encrypted voice chat.

---

## Features

### Core Voice Communication
- **Multi-Channel PTT** - Multiple voice channels per organization
- **Low Latency** - UDP-based voice transmission with jitter buffering
- **End-to-End Encryption** - SRTP encryption (X25519 key exchange + AES-128-GCM)
- **High-Quality Audio** - Opus codec with configurable bitrate

### Client Features
- **Modern Qt GUI** - Clean, intuitive interface
- **Hotkey Support** - Customizable Push-to-Talk keybinds
- **Multi-User** - See who's online, talking, muted, or deafened
- **Multiple Channels** - Switch between channels seamlessly
- **Admin Dashboard** - Built-in admin panel for server management

### Server Features
- **PostgreSQL Backend** - Robust user and permission management
- **JWT Authentication** - Secure token-based auth
- **WebSocket Control** - Real-time state synchronization
- **Docker Deployment** - Easy VPS deployment
- **Multi-Organization** - Support for multiple isolated groups

---

## Quick Start

### For Users

1. **Download** the latest client release
2. **Configure** server address in settings
3. **Login** with credentials
4. **Join** a channel
5. **Hold** Push-to-Talk key to speak

### For Server Admins

**Docker Deployment (Recommended)**:

```bash
# 1. Clone repository
git clone https://github.com/DMShort/DadLink.git
cd DadLink

# 2. Configure environment
cp .env.example .env
# Edit .env with secure passwords

# 3. Deploy with Docker Compose
docker-compose up -d

# 4. Configure firewall
sudo ufw allow 9000/tcp  # WebSocket control
sudo ufw allow 9001/udp  # Voice packets
```

**Full deployment guide**: [VPS_DEPLOYMENT_GUIDE.md](./VPS_DEPLOYMENT_GUIDE.md)

---

## Architecture

```
┌─────────────────────────────────────────────┐
│  Qt Client (Windows)                        │
│  ├─ WebSocket (Control - Port 9000/TCP)    │
│  └─ UDP (Voice - Port 9001/UDP)             │
└─────────────────┬───────────────────────────┘
                  │
                  │ TLS + SRTP Encrypted
                  │
┌─────────────────▼───────────────────────────┐
│  Rust Server                                │
│  ├─ Axum (WebSocket)                        │
│  ├─ UDP Voice Router                        │
│  ├─ SRTP Session Manager                    │
│  └─ PostgreSQL Database                     │
└─────────────────────────────────────────────┘
```

### Technology Stack

**Client**:
- Qt 6.8 (GUI framework)
- OpenSSL (TLS, crypto)
- Opus (audio codec)
- WebSocket++ (control channel)

**Server**:
- Rust (async with Tokio)
- Axum (WebSocket server)
- PostgreSQL (user/channel storage)
- SQLx (database ORM)

---

## Security

- **End-to-End Voice Encryption**: X25519 ECDH key exchange → AES-128-GCM
- **TLS WebSocket**: Encrypted control channel (optional but recommended)
- **JWT Tokens**: Secure, stateless authentication
- **Password Hashing**: Argon2 for stored credentials
- **No Plaintext Secrets**: Environment-based configuration

**Security documentation**: [SECURITY_ARCHITECTURE.md](./SECURITY_ARCHITECTURE.md)

---

## Documentation

### Getting Started
- **[QUICK_START.md](./QUICK_START.md)** - 5-minute getting started guide
- **[SETUP_WINDOWS.md](./SETUP_WINDOWS.md)** - Windows client build guide
- **[VPS_DEPLOYMENT_GUIDE.md](./VPS_DEPLOYMENT_GUIDE.md)** - Server deployment

### Docker Deployment
- **[DOCKER_README.md](./DOCKER_README.md)** - Docker overview
- **[DOCKER_DEPLOYMENT.md](./DOCKER_DEPLOYMENT.md)** - Complete Docker reference
- **[TESTING_DOCKER.md](./TESTING_DOCKER.md)** - Local Docker testing
- **[VPS_COMPARISON_2024.md](./VPS_COMPARISON_2024.md)** - VPS provider comparison

### Architecture & Design
- **[ARCHITECTURE.md](./ARCHITECTURE.md)** - System architecture
- **[NETWORK_PROTOCOL.md](./NETWORK_PROTOCOL.md)** - Network protocol details
- **[SECURITY_ARCHITECTURE.md](./SECURITY_ARCHITECTURE.md)** - Security design

### Administration
- **[ADMIN_PANEL_DEVELOPMENT_COMPLETE.md](./ADMIN_PANEL_DEVELOPMENT_COMPLETE.md)** - Admin features
- **[TROUBLESHOOTING.md](./TROUBLESHOOTING.md)** - Common issues

---

## Deployment Options

### Home Hosting (Basic)
- **Cost**: Free (port forwarding required)
- **Difficulty**: Medium
- **Limitations**: CGNAT may block

See: [HOME_HOSTING_GUIDE.md](./HOME_HOSTING_GUIDE.md)

### VPS Hosting (Recommended)
- **Cost**: $4.50-6/month
- **Difficulty**: Easy with Docker
- **Reliability**: 99.9% uptime

**Recommended providers**:
- DigitalOcean ($6/mo, $200 free credit)
- Hetzner ($4.50/mo, best value)

See: [VPS_DEPLOYMENT_GUIDE.md](./VPS_DEPLOYMENT_GUIDE.md)

---

## Project Status

✅ **Core Features Complete**
- Multi-channel PTT system
- SRTP voice encryption
- WebSocket control channel
- User authentication & permissions
- Admin dashboard

✅ **Docker Deployment Ready**
- Multi-stage Dockerfile
- docker-compose orchestration
- Complete deployment documentation

⚠️ **Known Issues**
- Local Docker build requires Rust dependency update (works fine on VPS)
- See [DOCKER_STATUS.md](./DOCKER_STATUS.md) for details

---

## Development

### Building from Source

**Client** (Windows):
```powershell
# Install Qt 6.8
# Install Visual Studio 2019+

cd client
cmake -G "Visual Studio 16 2019" -A x64 -B build
cmake --build build --config Release
```

**Server** (any OS):
```bash
cd server
cargo build --release
```

### Testing

```bash
# Server tests
cd server
cargo test

# Admin integration tests
.\test_admin_integration.ps1
```

---

## Roadmap

### v1.0 (Current)
- ✅ Multi-channel PTT
- ✅ SRTP encryption
- ✅ Admin dashboard
- ✅ Docker deployment

### v1.1 (Planned)
- [ ] Mobile clients (iOS/Android)
- [ ] Screen sharing
- [ ] File transfers
- [ ] Voice activity detection (VAD)

### v2.0 (Future)
- [ ] Video calling
- [ ] Recording/playback
- [ ] Integrations (Discord, Slack)
- [ ] Cloud-based relay servers

---

## Contributing

Contributions welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

See [DEVELOPMENT.md](./DEVELOPMENT.md) for development guidelines.

---

## License

[Choose your license - MIT, GPL, etc.]

---

## Support

**Issues**: https://github.com/DMShort/DadLink/issues

**Documentation**: See `/docs` directory

**Deployment Help**: [VPS_DEPLOYMENT_GUIDE.md](./VPS_DEPLOYMENT_GUIDE.md)

---

## Credits

Built with:
- [Rust](https://www.rust-lang.org/) - Server backend
- [Qt](https://www.qt.io/) - Client GUI framework
- [Opus](https://opus-codec.org/) - Audio codec
- [libsodium](https://libsodium.org/) - Cryptography

---

## Quick Links

- 🚀 **[Quick Start](./QUICK_START.md)**
- 📘 **[Full Documentation](./ARCHITECTURE.md)**
- 🐳 **[Docker Deployment](./DOCKER_DEPLOYMENT.md)**
- 🔧 **[Troubleshooting](./TROUBLESHOOTING.md)**
- 🏥 **[VPS Guide](./VPS_DEPLOYMENT_GUIDE.md)**

---

**Ready to deploy?** See [VPS_DEPLOYMENT_GUIDE.md](./VPS_DEPLOYMENT_GUIDE.md) for a 30-minute setup guide!
