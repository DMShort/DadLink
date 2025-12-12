# VoIP Client

C++ client for gaming-focused multi-channel VoIP system.

## Prerequisites

### Windows (MSVC)
- Visual Studio 2022 or later with C++20 support
- CMake 3.20+
- vcpkg (for dependencies)

### Linux (GCC/Clang)
- GCC 11+ or Clang 13+
- CMake 3.20+
- Development libraries

## Dependencies

Install via vcpkg (Windows) or system package manager (Linux):

- Qt6 (Core, Widgets, Network)
- Opus (libopus)
- PortAudio
- OpenSSL
- Google Test (for tests)

### Windows (vcpkg)
```powershell
vcpkg install qt6-base qt6-widgets opus portaudio openssl gtest
```

### Linux (Ubuntu/Debian)
```bash
sudo apt-get install \
    build-essential cmake \
    qt6-base-dev libqt6widgets6 \
    libopus-dev libportaudio2 libportaudiocpp0 \
    libssl-dev \
    libgtest-dev
```

## Building

### Configure
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

With vcpkg (Windows):
```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

### Build
```bash
cmake --build build --config Release
```

### Run Tests
```bash
cd build
ctest --output-on-failure
```

## Project Structure

```
client/
├── include/
│   ├── audio/           # Audio engine, Opus codec
│   ├── network/         # UDP/TLS networking
│   ├── session/         # Session management
│   ├── channel/         # Channel routing, mixer
│   ├── hotkey/          # Global hotkey manager
│   ├── config/          # Configuration
│   ├── ui/              # Qt UI components
│   └── common/          # Shared types, Result, queues
├── src/                 # Implementation files
├── tests/               # Unit tests
└── CMakeLists.txt
```

## Current Status

**Milestone 1.2 - CORE VOICE COMPLETE!** ✅

### ✅ Completed Features
- ✅ Project structure & CMake build system
- ✅ Audio engine (PortAudio wrapper with real-time safety)
- ✅ Opus encoder/decoder with FEC support
- ✅ Lock-free queues for RT-safe communication
- ✅ Result<T> type for error handling
- ✅ UDP voice socket with packet routing
- ✅ Jitter buffer with PLC (Packet Loss Concealment)
- ✅ VoiceSession integration (capture → encode → send → receive → decode → playback)
- ✅ Real-time audio level meters (input/output)
- ✅ Qt6 UI with dark theme
- ✅ Login dialog with proper layout
- ✅ Main window with voice controls
- ✅ **Multi-user voice communication WORKING!**
- ✅ WebSocket client connection
- ✅ DLL deployment scripts (Windows)

### ⏳ In Progress
- ⏳ WebSocket user list population (timing/parser issue)
- ⏳ Channel management UI
- ⏳ CI/CD setup

### 🎯 Next Priorities
1. Fix WebSocket message parser ("Unknown message type: 0")
2. Implement channel join/leave UI
3. Add mute/deafen functionality
4. Push-to-talk keybind system
5. Audio device selection
6. Multi-channel support

### 🔧 Key Technical Achievements
- **Struct packing fix**: `#pragma pack(push, 1)` on VoicePacketHeader prevents padding issues
- **Audio level gain**: 4x multiplier makes meters more responsive (RMS * 4.0)
- **DLL deployment**: Robust scripts prevent version conflicts
- **Dark theme**: Comprehensive QSS styling with proper label visibility  

## Running

```bash
./build/voip-client
```

Configuration file: `config.json` (create from `config.json.example`)

## Development

### Code Style
- C++20 standard
- Follow existing naming conventions
- Use Result<T> for error handling
- Document public APIs

### Real-Time Audio Rules (CRITICAL)
1. **NO** heap allocation in audio callbacks
2. **NO** blocking operations (mutex, I/O)
3. **NO** unbounded loops
4. Use lock-free queues for cross-thread communication
5. Pre-allocate all buffers

### Testing
- Write unit tests for new features
- Run tests before committing: `ctest`
- Aim for >80% code coverage

## License

[To be determined]
