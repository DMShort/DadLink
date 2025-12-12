# Installing Qt6 with WebSockets for VoIP Client

## Download Qt6 Installer

1. Go to: https://www.qt.io/download-qt-installer
2. Download: `qt-unified-windows-x64-online.exe`
3. Run the installer

## Installation Steps

### 1. Create Qt Account (Free)
- You'll need to create a free Qt account to proceed

### 2. Select Installation Type
- Choose **"Custom Installation"**

### 3. Select Components (CRITICAL!)

**Qt 6.8.x (or latest):**
```
✅ MSVC 2019 64-bit (or MSVC 2022 64-bit)
✅ Qt WebSockets ← MUST SELECT THIS!
✅ Qt Core
✅ Qt Widgets
✅ Qt Network
```

**Optional but recommended:**
```
✅ CMake
✅ Ninja
✅ Qt Design Studio (for UI design)
```

### 4. Installation Path
Default is fine:
```
C:\Qt\6.8.0\msvc2019_64
```

**Size:** ~5-10 GB
**Time:** 15-30 minutes depending on internet speed

---

## After Installation

### Update Your Path (Optional)
Add Qt to your PATH environment variable:
```
C:\Qt\6.8.0\msvc2019_64\bin
```

### Verify Installation
Check that Qt is installed:
```powershell
dir C:\Qt\6.8.0\msvc2019_64\lib\cmake\Qt6WebSockets
```

You should see Qt6WebSocketsConfig.cmake

---

## Configure CMake with Qt6

### Option A: Set CMAKE_PREFIX_PATH (Recommended)

Already done in `CMakeLists.txt`:
```cmake
set(CMAKE_PREFIX_PATH "C:/Qt/6.8.0/msvc2019_64" ${CMAKE_PREFIX_PATH})
```

### Option B: Use Environment Variable

Set environment variable:
```powershell
$env:CMAKE_PREFIX_PATH = "C:\Qt\6.8.0\msvc2019_64"
```

---

## Build After Qt6 Installation

```powershell
cd C:\dev\VoIP-System\client

# Clean previous build
Remove-Item -Recurse -Force build

# Configure with new Qt6
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=OFF

# Build
cmake --build build --config Debug
```

**Note:** Don't use vcpkg toolchain anymore - Qt6 is now system-installed!

---

## Verify WebSockets Module

After CMake configure, check the output for:
```
-- Found Qt6WebSockets
```

If you see that, you're good! ✅

---

## Troubleshooting

### "Could not find Qt6WebSockets"

**Check version number matches:**
```powershell
dir C:\Qt
```

Update CMakeLists.txt to match your installed version:
```cmake
set(CMAKE_PREFIX_PATH "C:/Qt/YOUR_VERSION/msvc2019_64" ${CMAKE_PREFIX_PATH})
```

### "Qt6 not found"

Make sure you selected MSVC 2019 64-bit (not MinGW) during installation.

### Wrong compiler

CMake might pick the wrong compiler. Force it:
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug
```

---

## Alternative: Qt6 via vcpkg (Future)

If vcpkg adds WebSockets support:
```powershell
vcpkg install qt6-base[websockets]:x64-windows
```

But for now, **full Qt SDK is the way to go!**

---

## What You'll Get

After successful installation and build:
✅ WebSocket client compiles
✅ Full authentication support
✅ Real-time channel switching
✅ User notifications
✅ Complete multi-user voice system

---

## Installation Time

- **Download:** 5-10 minutes
- **Install:** 15-30 minutes
- **Configure + Build:** 5 minutes

**Total:** ~30-45 minutes

---

## While Qt is Installing...

You can still test multi-user voice! The server routing doesn't require WebSocket client.

Want me to update the server voice router to enable multi-user now? Then when Qt finishes installing, you'll have both working! 🚀
