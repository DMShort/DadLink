# Development Guide

## Getting Started

### Prerequisites

**Client Development:**
- Windows 10/11 with Visual Studio 2022+ OR Linux with GCC 11+
- CMake 3.20+
- vcpkg (Windows) or system package manager (Linux)

**Server Development:**
- Rust 1.70+ ([Install](https://rustup.rs/))
- PostgreSQL 12+
- Redis 6+

### Initial Setup

1. **Clone Repository**
```bash
git clone <repository-url>
cd voip-system
```

2. **Client Setup**
```bash
cd client

# Windows (vcpkg)
vcpkg install qt6-base opus portaudio openssl gtest

# Linux (Ubuntu)
sudo apt-get install qt6-base-dev libopus-dev libportaudio2 libssl-dev libgtest-dev

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

3. **Server Setup**
```bash
cd server

# Install dependencies (handled by cargo)
cargo build

# Setup database
createdb voip
sqlx migrate run

# Run server
cargo run
```

## Development Workflow

### Milestone 1.1: Foundation (Current)

**Client Tasks:**
- [x] Audio engine implementation
- [x] Opus codec wrapper
- [x] Lock-free queues
- [x] Basic unit tests
- [ ] Audio loopback integration test
- [ ] Network packet structures

**Server Tasks:**
- [x] Project structure
- [x] Configuration system
- [x] Authentication (Argon2 + JWT)
- [x] Type definitions
- [ ] Database schema
- [ ] UDP voice listener
- [ ] TLS control listener

### Branch Strategy

- `main` - Production-ready code
- `develop` - Integration branch
- `feature/*` - Feature branches
- `fix/*` - Bug fix branches

### Commit Convention

```
<type>(<scope>): <subject>

<body>
```

Types: `feat`, `fix`, `docs`, `test`, `refactor`, `perf`, `chore`

Examples:
```
feat(client): implement jitter buffer
fix(server): correct packet routing logic
test(audio): add opus encode/decode tests
```

## Testing

### Client Tests
```bash
cd client/build
ctest --output-on-failure
```

Run specific test:
```bash
./voip-client-tests --gtest_filter=OpusCodecTest.*
```

### Server Tests
```bash
cd server
cargo test
cargo test -- --nocapture  # See output
```

Run specific test:
```bash
cargo test test_password_hash
```

## Code Quality

### Client (C++)
```bash
# Format code (if clang-format configured)
clang-format -i src/**/*.cpp include/**/*.h

# Static analysis
# Configure in CMake with: -DCMAKE_CXX_CLANG_TIDY=clang-tidy
```

### Server (Rust)
```bash
# Format code
cargo fmt

# Linting
cargo clippy -- -D warnings

# Check without building
cargo check
```

## Real-Time Audio Guidelines (CRITICAL)

When working on audio code, **ALWAYS** follow these rules:

1. **NO** heap allocation in audio callbacks
   - Pre-allocate all buffers during initialization
   - Use fixed-size arrays or pre-allocated vectors
   
2. **NO** blocking operations
   - No mutexes (use atomics or lock-free structures)
   - No I/O operations
   - No sleeps or waits

3. **NO** unbounded loops
   - All loops must have fixed iteration counts
   - No searching or dynamic algorithms

4. **Use lock-free queues**
   - For passing data between RT and non-RT threads
   - See `lock_free_queue.h` for implementation

5. **Keep execution time minimal**
   - Target: <1ms for audio callback
   - Profile with timing instrumentation

### Example of GOOD audio callback code:
```cpp
int audio_callback(const float* input, float* output, size_t frames) {
    // GOOD: Lock-free queue push (wait-free)
    if (!queue.try_push(input, frames)) {
        stats.queue_full++;  // Atomic increment
    }
    
    // GOOD: Fixed-size copy
    std::memcpy(output, prebuffer, frames * sizeof(float));
    
    return 0;
}
```

### Example of BAD audio callback code:
```cpp
int audio_callback(const float* input, float* output, size_t frames) {
    // BAD: Heap allocation
    auto buffer = new float[frames];  // ❌ NEVER!
    
    // BAD: Mutex lock (blocking)
    std::lock_guard<std::mutex> lock(mutex);  // ❌ NEVER!
    
    // BAD: I/O operation
    file.write(input, frames);  // ❌ NEVER!
    
    // BAD: Unbounded loop
    while (!queue.empty()) { }  // ❌ NEVER!
    
    return 0;
}
```

## Debugging

### Client Debugging
```bash
# Windows (Visual Studio)
# Open build/voip-client.sln

# Linux (GDB)
gdb ./build/voip-client
```

### Server Debugging
```bash
# Enable debug logging
export RUST_LOG=voip_server=trace

# Run with debugger
rust-gdb target/debug/voip-server
```

### Network Debugging

Use Wireshark with custom dissector (coming in Milestone 1.2):
```bash
wireshark -i lo -f "udp port 9001"
```

## Performance Profiling

### Client Profiling
```bash
# Windows (Visual Studio Profiler)
# Use built-in profiler

# Linux (perf)
perf record -g ./build/voip-client
perf report
```

### Server Profiling
```bash
# CPU profiling
cargo install flamegraph
cargo flamegraph

# Memory profiling
cargo install heaptrack
heaptrack target/release/voip-server
```

## Documentation

- Update README when adding features
- Document all public APIs with comments
- Keep design docs in `/docs` in sync with code
- Add examples for non-obvious functionality

## Getting Help

- Review documentation in `/docs`
- Check `CASCADE_RULES.md` for project standards
- Ask questions in team chat
- Create issues for bugs or feature requests

## Next Steps

See `DEVELOPMENT_ROADMAP.md` for upcoming milestones and features.
