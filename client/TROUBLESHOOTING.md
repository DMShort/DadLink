# 🔧 Troubleshooting Guide - VoIP Client

## Application Won't Start / Exit Code 1

### Problem:
The `voip-client.exe` starts and immediately exits with code 1, no window appears.

### Root Cause:
Qt applications require platform plugins (DLLs) to initialize. Without them, Qt fails before `main()` even runs.

### ✅ Solution:
Run `windeployqt` to copy all required Qt dependencies:

```powershell
cd C:\dev\VoIP-System\client
& "C:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe" ".\build\Debug\voip-client.exe" --debug --no-translations
```

This will copy:
- Platform plugins (`platforms/qwindowsd.dll`)
- Style plugins (`styles/*.dll`)
- Image format plugins (`imageformats/*.dll`)  
- Icon engine plugins (`iconengines/*.dll`)
- Network plugins (`networkinformation/*.dll`, `tls/*.dll`)
- Required Qt DLLs (`Qt6Cored.dll`, `Qt6Widgetsd.dll`, etc.)

### 🔄 Automatic Deployment:
The CMakeLists.txt now includes a POST_BUILD command to run windeployqt automatically after each build.

Rebuild to activate:
```powershell
cmake --build build --config Debug --target voip-client
```

---

## Missing DLL Errors

### Qt6Cored.dll not found:
```powershell
windeployqt .\build\Debug\voip-client.exe --debug
```

### portaudio.dll or opus.dll not found:
These are already copied to `build/Debug/` by CMake. If missing, rebuild:
```powershell
cmake --build build --config Debug
```

---

## Application Crashes on Startup

### Check Console Output:
Run from command line to see error messages:
```powershell
.\build\Debug\voip-client.exe
```

Or use the provided batch file:
```powershell
.\run_client.bat
```

### Common Issues:

1. **Qt Platform Plugin Error:**
   ```
   qt.qpa.plugin: Could not find the Qt platform plugin "windows"
   ```
   **Fix:** Run `windeployqt` (see above)

2. **Resource File Not Found:**
   ```
   Cannot open resource file
   ```
   **Fix:** Rebuild to regenerate resources:
   ```powershell
   cmake --build build --config Debug
   ```

3. **Audio Device Error:**
   - Make sure your microphone/speakers are connected
   - Check Windows sound settings
   - Try different audio devices in settings (when implemented)

---

## Black Window / UI Not Loading

### Check Dark Theme:
The app loads `:/styles/dark_theme.qss` from resources. If this fails, UI will still load but won't be styled.

### Verify Resources:
```powershell
# Check if resources were compiled
Get-ChildItem .\build\ -Recurse -Filter "qrc_*.cpp"
```

Should show:  `voip-client_autogen/EWIEGA46WW/qrc_resources.cpp`

---

## Voice Session Won't Start

### PortAudio Initialization Failed:
- **Check audio devices:** Make sure mic/speakers are plugged in
- **Check permissions:** Windows may block mic access
- **Restart audio service:**
  ```powershell
  Restart-Service -Name "Audiosrv"
  ```

### UDP Socket Error:
- **Port already in use:** Another instance running?
  ```powershell
  Get-NetUDPEndpoint -LocalPort 9001
  ```
- **Firewall blocking:** Add exception for voip-client.exe

---

## Server Connection Issues

### WebSocket Connection Failed:
- **Server not running:**
  ```powershell
  cd C:\dev\VoIP-System\server
  cargo run
  ```
- **Wrong address:** Check login dialog server address (`127.0.0.1:9000`)
- **Firewall:** Allow TCP port 9000

### UDP Voice Not Working:
- **Server not receiving:** Check server console for UDP messages
- **Port blocked:** Allow UDP port 9001
- **Check server logs:**
  ```
  DEBUG voip_server::network::udp: Received packet from ...
  ```

---

## Debug Mode

### Enable Console Output:
Already enabled! The CMakeLists.txt sets `/SUBSYSTEM:CONSOLE`.

### Add More Logging:
Edit `ui_main.cpp` and add:
```cpp
#include <iostream>

int main(int argc, char *argv[]) {
    std::cout << "Starting..." << std::endl;
    // ... rest of code
}
```

### Qt Debug Messages:
Set environment variable:
```powershell
$env:QT_DEBUG_PLUGINS=1
.\build\Debug\voip-client.exe
```

---

## Build Issues

### CMake Configure Failed:
```powershell
# Clean and reconfigure
Remove-Item -Recurse -Force build
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.10.1/msvc2022_64"
cmake --build . --config Debug
```

### Qt6 Not Found:
Make sure Qt is installed at `C:\Qt\6.10.1\msvc2022_64` or update `CMAKE_PREFIX_PATH` in CMakeLists.txt.

### Opus/PortAudio Not Found:
Check vcpkg integration:
```powershell
vcpkg integrate install
vcpkg install opus:x64-windows portaudio:x64-windows
```

---

## Quick Fixes

### 1. Full Rebuild:
```powershell
cmake --build build --config Debug --target voip-client --clean-first
```

### 2. Deploy Qt Dependencies:
```powershell
windeployqt .\build\Debug\voip-client.exe --debug --no-translations
```

### 3. Check Process Running:
```powershell
Get-Process voip-client -ErrorAction SilentlyContinue
```

### 4. Kill Stuck Process:
```powershell
Stop-Process -Name voip-client -Force
```

---

## Still Not Working?

### Create Minimal Test:
```cpp
// test_qt.cpp
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QMessageBox::information(nullptr, "Test", "Qt Works!");
    return 0;
}
```

Build and test:
```powershell
# Build with CMake
cmake --build build --config Debug

# If Qt test works but voip-client doesn't, issue is in our code
# If Qt test doesn't work, Qt setup issue
```

---

## Contact / Report Issues

If none of these fixes work:
1. Check console output for error messages
2. Check Windows Event Viewer for crash details
3. Try running in VS Debugger to see crash point
4. Check antivirus/security software isn't blocking

---

## Success Checklist

✅ Qt installed at `C:\Qt\6.10.1\msvc2022_64`  
✅ `windeployqt` has been run  
✅ `build/Debug/platforms/qwindowsd.dll` exists  
✅ Server running (`cargo run` in server folder)  
✅ Audio devices connected  
✅ Firewall allows UDP 9001 and TCP 9000  
✅ No other VoIP client instance running  

If all checked → Should work! 🎉
