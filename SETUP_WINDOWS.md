# Windows Development Setup

## Prerequisites Installation

### 1. Install Rust (5 minutes)

**Download**: https://rustup.rs/

```powershell
# After installation, verify:
cargo --version
rustc --version
```

### 2. Install vcpkg Dependencies (2-4 hours - Qt6 is large)

vcpkg is already installed at `C:\vcpkg`

```powershell
# Update vcpkg
cd C:\vcpkg
git pull
.\bootstrap-vcpkg.bat

# Install dependencies (this will take 2-4 hours)
.\vcpkg.exe install `
    qtbase:x64-windows `
    qtwebsockets:x64-windows `
    opus:x64-windows `
    portaudio:x64-windows `
    openssl:x64-windows `
    gtest:x64-windows
```

---

## Quick Start (Test Server First - 2 minutes)

### Step 1: Test Server (Rust)

```powershell
cd server

# Build and test
cargo build
cargo test

# Expected output: All tests pass ✅
```

### Step 2: Build Client (After vcpkg install completes)

```powershell
cd client

# Configure
cmake -B build `
    -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" `
    -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build --config Debug

# Test
cd build
ctest --output-on-failure -C Debug
```

---

## Alternative: Skip Client Build for Now

You can work on the server first while dependencies install in the background:

```powershell
# Terminal 1: Install dependencies (background)
cd C:\vcpkg
.\vcpkg.exe install qtbase opus portaudio openssl gtest --triplet=x64-windows

# Terminal 2: Develop server
cd server
cargo build
cargo test
cargo run
```

---

## What to Install First?

**Choose based on what you want to work on:**

| Goal | Install This | Time | Priority |
|------|-------------|------|----------|
| Test server code | Rust only | 5 min | ⭐⭐⭐ Do first |
| Build audio engine | vcpkg deps | 2-4 hrs | ⭐⭐ Background |
| Full system | Both | 2-4 hrs | ⭐ Eventually |

---

## Recommended Order

1. **Install Rust** (5 minutes)
2. **Test server** to verify foundation works
3. **Start vcpkg install** in background
4. **Continue server development** while waiting
5. **Build client** once dependencies finish

---

## Verification Commands

### After Rust Install
```powershell
cargo --version
# Expected: cargo 1.70+ or newer
```

### After vcpkg Install
```powershell
C:\vcpkg\vcpkg.exe list
# Should show: qtbase, opus, portaudio, openssl, gtest
```

---

## Troubleshooting

### "cargo not found"
- Restart PowerShell after installing Rust
- Or add to PATH: `$env:Path += ";$env:USERPROFILE\.cargo\bin"`

### "Qt6 not found"
- Verify vcpkg install completed: `C:\vcpkg\vcpkg.exe list`
- Check toolchain file path in cmake command

### vcpkg install fails
- Update vcpkg: `cd C:\vcpkg; git pull; .\bootstrap-vcpkg.bat`
- Try installing packages one at a time

---

## Current Status

- ✅ Visual Studio 2022 (detected)
- ✅ vcpkg (installed at C:\vcpkg)
- ❌ Rust (need to install)
- ❌ vcpkg dependencies (need to install)

## Next Step

**Install Rust first** - it's the fastest way to see something working!

```powershell
# 1. Download from https://rustup.rs/
# 2. Run installer
# 3. Restart PowerShell
# 4. cd server && cargo test
```
