# Getting Started with VoIP System Development

Welcome! This guide will help you get started with developing the gaming-focused multi-channel VoIP system.

## 📚 Start Here

1. **Read Project Overview**
   - [`PROJECT_OVERVIEW.md`](PROJECT_OVERVIEW.md) - Understand what we're building and why

2. **Review Architecture**
   - [`ARCHITECTURE.md`](ARCHITECTURE.md) - System design and component relationships

3. **Understand Development Rules**
   - [`CASCADE_RULES.md`](CASCADE_RULES.md) - Critical guidelines and best practices

## 🚀 Quick Start

### Option 1: Client Development (C++)

```bash
cd client

# Install dependencies (Windows with vcpkg)
vcpkg install qt6-base opus portaudio openssl gtest

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run tests
cd build && ctest
```

See [`client/README.md`](client/README.md) for detailed instructions.

### Option 2: Server Development (Rust)

```bash
cd server

# Build (dependencies auto-installed by cargo)
cargo build

# Run tests
cargo test

# Run server
cargo run
```

See [`server/README.md`](server/README.md) for detailed instructions.

## 📖 Essential Reading

### For All Developers
- `PROJECT_OVERVIEW.md` - Vision and goals
- `ARCHITECTURE.md` - System design
- `CASCADE_RULES.md` - **CRITICAL**: Development standards
- `DEVELOPMENT.md` - Workflow and guidelines

### For Client Developers
- `CLIENT_DESIGN.md` - Client architecture
- `AUDIO_ENGINEERING.md` - Audio pipeline details
- `client/README.md` - Build instructions

### For Server Developers
- `SERVER_DESIGN.md` - Server architecture
- `NETWORK_PROTOCOL.md` - Protocol specifications
- `server/README.md` - Build instructions

### For DevOps
- `DEPLOYMENT_GUIDE.md` - Deployment options
- `SECURITY_ARCHITECTURE.md` - Security requirements

## 🎯 Current Milestone

**Milestone 1.1: Foundation (Months 1-2)**

### Completed ✅
- Project structure (client + server)
- Build configuration
- Audio engine framework
- Opus codec integration
- Authentication system (server)
- Basic unit tests
- Documentation

### In Progress ⏳
- Audio loopback integration test
- Network layer (UDP/TLS)
- Database integration
- CI/CD pipeline completion

### What You Can Work On

**Client Tasks:**
1. Implement jitter buffer (`client/src/audio/jitter_buffer.cpp`)
2. Add audio mixer (`client/src/audio/mixer.cpp`)
3. Create integration tests
4. Improve error handling

**Server Tasks:**
1. Implement UDP voice listener (`server/src/network/udp.rs`)
2. Implement TLS control handler (`server/src/network/tls.rs`)
3. Add database schema and migrations
4. Create routing engine foundation

## 🛠️ Development Environment

### Required Tools

**Client:**
- CMake 3.20+
- C++20 compiler (MSVC 2022+, GCC 11+, Clang 13+)
- vcpkg (Windows) or system package manager

**Server:**
- Rust 1.70+ via [rustup](https://rustup.rs/)
- PostgreSQL 12+
- Redis 6+

### IDE Recommendations

- **Visual Studio Code** with extensions:
  - C/C++ (Microsoft)
  - rust-analyzer
  - CMake Tools
  
- **Visual Studio 2022** (Windows client dev)
- **CLion** (Cross-platform)

## ⚠️ Critical Guidelines

### Real-Time Audio Safety (NEVER VIOLATE)

When working on audio code:
1. **NO** heap allocation in audio callbacks
2. **NO** blocking operations (mutex, I/O, sleep)
3. **NO** unbounded loops
4. Use lock-free queues for cross-thread communication
5. Pre-allocate ALL buffers

See `AUDIO_ENGINEERING.md` for details.

### Testing Requirements

For every feature:
- Write unit tests
- Consider integration tests
- Test error cases
- Document test scenarios

Target: >80% code coverage

### Code Quality

**Client (C++):**
- Use `Result<T>` for error handling
- Follow RAII principles
- Document public APIs
- Run tests before committing

**Server (Rust):**
- Use `cargo fmt` before committing
- Run `cargo clippy` and fix warnings
- Use idiomatic Rust patterns
- Comprehensive error handling with `Result`

## 🧪 Testing Your Changes

### Run All Tests

```bash
# Client
cd client/build && ctest --output-on-failure

# Server
cd server && cargo test
```

### Run Specific Tests

```bash
# Client
./voip-client-tests --gtest_filter=OpusCodecTest.*

# Server
cargo test test_password_hash -- --nocapture
```

## 📊 Project Status

- **Phase**: MVP Development (Phase 1)
- **Milestone**: 1.1 - Foundation
- **Target**: Month 2 completion
- **Next Milestone**: 1.2 - Basic Networking

See `DEVELOPMENT_ROADMAP.md` for complete timeline.

## 🤝 Contributing

1. Create feature branch: `git checkout -b feature/your-feature`
2. Make changes following guidelines
3. Write/update tests
4. Run tests locally
5. Commit with conventional format: `feat(client): add feature`
6. Push and create pull request

## 📞 Getting Help

1. **Documentation**: Check relevant `.md` files in root directory
2. **Code Comments**: Many implementations have detailed comments
3. **Examples**: See `tests/` directories for usage examples
4. **Team**: Ask questions in development channels

## 🎓 Learning Resources

### Audio Programming
- [PortAudio Documentation](http://www.portaudio.com/docs/)
- [Opus Codec](https://opus-codec.org/)
- Real-Time Audio Programming (Ross Bencina)

### Network Programming
- [Tokio Tutorial](https://tokio.rs/tokio/tutorial) (Rust async)
- [WebSocket Protocol](https://datatracker.ietf.org/doc/html/rfc6455)

### VoIP Concepts
- [Mumble Protocol](https://mumble-protocol.readthedocs.io/)
- [WebRTC Standards](https://webrtc.org/)

## 🚦 Next Steps

1. Choose client or server development
2. Set up development environment
3. Read relevant design documents
4. Pick a task from current milestone
5. Start coding!

**Good luck and happy coding! 🎉**
