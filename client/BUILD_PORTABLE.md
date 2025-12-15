# Building DadLink as a Fully Portable Application

This guide explains how to build DadLink without requiring users to install Visual C++ Redistributable.

## What Changed

**Before:** Application required VC++ Redistributable 2015-2022 (dynamic runtime `/MD`)
**After:** Application is fully portable with embedded runtime (static runtime `/MT`)

**Result:**
- ✅ Zero external dependencies (except packaged DLLs)
- ✅ No installer needed
- ✅ No admin rights required
- ✅ Extract and run anywhere
- ⚠️ Executable ~800KB larger (negligible for 70MB package)

---

## Build Process

### Step 1: Rebuild vcpkg Dependencies with Static Runtime

Dependencies must use the same runtime library as the application. Run:

```powershell
cd C:\dev\VoIP-System\client
.\rebuild-portable.bat
```

This script:
1. Removes old packages built with `/MD` (dynamic runtime)
2. Installs packages with `/MT` (static runtime) using custom triplet
3. Ensures compatibility with statically linked application

**Packages rebuilt:**
- `opus:x64-windows-static-runtime`
- `portaudio:x64-windows-static-runtime`
- `openssl:x64-windows-static-runtime`

**Time:** ~10-15 minutes

---

### Step 2: Clean Previous Build

Remove any existing build directory to prevent conflicts:

```powershell
cd C:\dev\VoIP-System\client
rm -r build -Force -ErrorAction SilentlyContinue
```

---

### Step 3: Configure with CMake

Use the new preset that specifies static runtime:

```powershell
cmake --preset windows-release
```

This configures the build with:
- `CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded` (static runtime)
- `VCPKG_TARGET_TRIPLET=x64-windows-static-runtime`
- Release optimizations
- Correct Qt and vcpkg paths

---

### Step 4: Build the Application

```powershell
cmake --build build --config Release
```

**Output:** `build\Release\voip-client.exe` with embedded runtime

---

### Step 5: Package for Distribution

```powershell
.\package-release.ps1 -Version "1.0.2"
```

This creates:
- `release-package\DadLink-1.0.2-Windows-x64.zip`
- Includes all Qt DLLs and OpenSSL libraries
- **No VC++ redistributable needed!**

---

## Technical Details

### CMakeLists.txt Changes

Added static runtime linking (lines 4-10):

```cmake
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    # Uses /MT for Release (static) instead of /MD (dynamic)
endif()
```

### Custom vcpkg Triplet

Created `C:\dev\vcpkg\triplets\x64-windows-static-runtime.cmake`:

```cmake
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_CXX_FLAGS "/MT")
set(VCPKG_CXX_FLAGS_DEBUG "/MTd")
```

**Key points:**
- `VCPKG_CRT_LINKAGE static` - Static C runtime
- `VCPKG_LIBRARY_LINKAGE dynamic` - Dynamic libraries (DLLs), not static libs
- `/MT` flag - Use MultiThreaded static runtime

---

## Verification

### Confirm Static Runtime Linking

Use `dumpbin` to verify the executable doesn't depend on MSVC runtime DLLs:

```powershell
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe" /DEPENDENTS build\Release\voip-client.exe
```

**Should NOT see:**
- `MSVCP140.dll`
- `VCRUNTIME140.dll`
- `VCRUNTIME140_1.dll`

**Should see (these are fine, we package them):**
- `Qt6Core.dll`
- `Qt6Widgets.dll`
- `libssl-3-x64.dll`
- `libcrypto-3-x64.dll`
- Standard Windows DLLs (KERNEL32.dll, etc.)

---

## Troubleshooting

### Error: LNK2038: mismatch detected for 'RuntimeLibrary'

**Cause:** Some dependencies still use `/MD` (dynamic runtime)

**Solution:**
1. Delete `C:\dev\vcpkg\installed\x64-windows\` folder
2. Run `rebuild-portable.bat` again
3. Rebuild client

### Error: Cannot find Qt6Core.dll

**Cause:** Qt DLLs not deployed

**Solution:** Run `package-release.ps1` which runs `windeployqt`

### Application crashes on startup on clean machine

**Possible causes:**
1. Missing DLLs - check package has all Qt DLLs
2. OpenSSL DLLs missing - ensure `libssl-3-x64.dll` and `libcrypto-3-x64.dll` are included
3. Audio device issues - check Windows audio settings

---

## Size Comparison

| Component | Dynamic Runtime | Static Runtime | Difference |
|-----------|----------------|----------------|------------|
| voip-client.exe | ~2.5 MB | ~3.3 MB | +800 KB |
| Total Package | 70.67 MB | 70.67 MB | 0 MB |

The size increase is embedded in the .exe and doesn't affect total package size since we no longer include VC++ redistributable installer.

---

## Distribution Checklist

Before releasing:

- [ ] Rebuild vcpkg dependencies with static runtime
- [ ] Clean build directory
- [ ] Configure with `windows-release` preset
- [ ] Build in Release mode
- [ ] Run `package-release.ps1`
- [ ] Test ZIP on clean Windows machine without VC++ installed
- [ ] Verify no runtime errors
- [ ] Verify hotkeys work
- [ ] Verify audio works
- [ ] Create GitHub release
- [ ] Upload ZIP as asset

---

## Reverting to Dynamic Runtime (if needed)

If you need to go back to dynamic runtime (requires VC++ redistributable):

1. **Remove static runtime line from CMakeLists.txt:**
   ```cmake
   # Comment out or remove:
   # set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
   ```

2. **Rebuild dependencies with standard triplet:**
   ```powershell
   vcpkg install opus:x64-windows portaudio:x64-windows openssl:x64-windows
   ```

3. **Reconfigure and rebuild:**
   ```powershell
   cmake -B build -DVCPKG_TARGET_TRIPLET=x64-windows
   cmake --build build --config Release
   ```

---

## Summary

✅ **Fully portable deployment** - No installers, no admin rights
✅ **Better user experience** - Extract and run
✅ **Same package size** - Runtime embedded in .exe
✅ **No version conflicts** - Self-contained runtime

For questions or issues, see: https://github.com/DMShort/DadLink/issues
