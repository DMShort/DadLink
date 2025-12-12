# VoIP Server

Rust server for gaming-focused multi-channel VoIP system.

## Prerequisites

- Rust 1.70+ (install via [rustup](https://rustup.rs/))
- PostgreSQL 12+
- Redis 6+

## Building

```bash
cargo build --release
```

## Running

```bash
# Development
cargo run

# Production (release build)
./target/release/voip-server
```

## Configuration

Create `config/server.yaml`:

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
  tls_cert: "certs/cert.pem"
  tls_key: "certs/key.pem"
  jwt_secret: "CHANGE_ME_IN_PRODUCTION"

audio:
  max_bitrate: 128000
  default_bitrate: 32000

limits:
  max_channels_per_org: 100
  max_users_per_org: 1000
  max_users_per_channel: 50
```

Or use environment variables (prefix with `VOIP_`):
```bash
export VOIP_DATABASE__URL="postgresql://..."
export VOIP_SECURITY__JWT_SECRET="..."
```

## Testing

```bash
cargo test
```

## Project Structure

```
server/
├── src/
│   ├── main.rs          # Entry point
│   ├── config.rs        # Configuration loading
│   ├── error.rs         # Error types
│   ├── types.rs         # Shared types and messages
│   ├── auth.rs          # Authentication (Argon2, JWT)
│   ├── network/         # UDP voice + TLS control
│   ├── routing.rs       # Voice packet routing
│   ├── org.rs           # Organization management
│   └── channel.rs       # Channel management
├── migrations/          # Database migrations
└── Cargo.toml
```

## Current Status

**Milestone 1.1 - Foundation** (In Progress)

✅ Project structure  
✅ Configuration system  
✅ Error handling  
✅ Type definitions  
✅ Authentication (Argon2 + JWT)  

⏳ Database integration  
⏳ Network layer (UDP/TLS)  
⏳ Routing engine  

## Development

### Code Style
- Follow Rust standard conventions
- Use `cargo fmt` before committing
- Run `cargo clippy` to catch issues

### Testing
```bash
cargo test
cargo test -- --nocapture  # See output
```

### Database Migrations

```bash
# Install sqlx-cli
cargo install sqlx-cli --features postgres

# Run migrations
sqlx migrate run

# Create new migration
sqlx migrate add <name>
```

## License

[To be determined]
