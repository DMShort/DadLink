@echo off
REM VoIP Client - Windows Distribution Package Creator
REM Creates a portable package ready for distribution

echo ========================================
echo VoIP Client - Distribution Builder
echo ========================================
echo.

cd /d "%~dp0"

REM Configuration
set BUILD_CONFIG=Release
set BUILD_DIR=build\%BUILD_CONFIG%
set DEPLOY_DIR=deploy\VoIP-Client
set EXE_NAME=voip-client.exe

REM Check if Release build exists
if not exist "%BUILD_DIR%\%EXE_NAME%" (
    echo [ERROR] Release build not found!
    echo Please build first: cmake --build build --config Release
    pause
    exit /b 1
)

echo [1/7] Cleaning previous deployment...
if exist deploy rmdir /s /q deploy
mkdir "%DEPLOY_DIR%"

echo [2/7] Copying executable...
copy /Y "%BUILD_DIR%\%EXE_NAME%" "%DEPLOY_DIR%\" >nul

echo [3/7] Deploying Qt dependencies...
C:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe ^
    "%DEPLOY_DIR%\%EXE_NAME%" ^
    --release ^
    --no-translations ^
    --no-compiler-runtime ^
    --no-system-d3d-compiler ^
    --no-opengl-sw ^
    --force

echo [4/7] Copying audio libraries (Opus, PortAudio)...
copy /Y C:\vcpkg\installed\x64-windows-static-runtime\bin\opus.dll "%DEPLOY_DIR%\" >nul
copy /Y C:\vcpkg\installed\x64-windows-static-runtime\bin\portaudio.dll "%DEPLOY_DIR%\" >nul

echo [5/7] Copying OpenSSL libraries (for SRTP encryption)...
copy /Y C:\vcpkg\installed\x64-windows-static-runtime\bin\libcrypto-3-x64.dll "%DEPLOY_DIR%\" >nul
copy /Y C:\vcpkg\installed\x64-windows-static-runtime\bin\libssl-3-x64.dll "%DEPLOY_DIR%\" >nul

echo [6/7] Creating README and documentation...
(
echo VoIP Client - Encrypted Voice Communication
echo ============================================
echo.
echo QUICK START
echo -----------
echo 1. Run voip-client.exe
echo 2. Enter server address ^(IP or domain^)
echo 3. Enter port ^(default: 9000^)
echo 4. Check "Use TLS" for encrypted connection
echo 5. Login or Register
echo.
echo FEATURES
echo --------
echo - End-to-end encrypted voice ^(SRTP with AES-128-GCM^)
echo - Perfect Forward Secrecy ^(ephemeral X25519 keys^)
echo - Multi-channel voice chat
echo - Push-to-talk with customizable hotkeys
echo - Low latency ^(Opus codec, ~20ms^)
echo - Jitter buffer for network stability
echo.
echo SYSTEM REQUIREMENTS
echo -------------------
echo - Windows 10/11 ^(64-bit^)
echo - Audio input device ^(microphone^)
echo - Audio output device ^(speakers/headphones^)
echo - Internet connection
echo - 500 KB/s recommended bandwidth
echo.
echo CONTROLS
echo --------
echo - Space: Push-to-talk ^(default, customizable^)
echo - M: Toggle mute
echo - D: Toggle deafen
echo.
echo SERVER CONNECTION
echo -----------------
echo The server uses two ports:
echo - TCP 9000: Control channel ^(WebSocket over TLS^)
echo - UDP 9001: Voice packets ^(encrypted with SRTP^)
echo.
echo Example server: 122.150.216.145:9000
echo.
echo TROUBLESHOOTING
echo ---------------
echo Connection Failed:
echo   - Verify server address and port
echo   - Check firewall settings
echo   - Ensure server is running
echo.
echo No Audio:
echo   - Check microphone permissions
echo   - Verify audio device in Settings
echo   - Check volume levels
echo.
echo High Latency:
echo   - Check network connection
echo   - Reduce bandwidth usage
echo   - Server may be distant
echo.
echo SECURITY NOTES
echo --------------
echo - All voice packets are encrypted end-to-end
echo - Key exchange uses X25519 elliptic curve
echo - Encryption uses AES-128-GCM authenticated encryption
echo - Replay protection prevents packet replay attacks
echo - Each session uses unique ephemeral keys
echo.
echo For more information: https://github.com/yourusername/voip-system
) > "%DEPLOY_DIR%\README.txt"

echo [7/7] Creating version info...
(
echo VoIP Client v0.1.0
echo Build Date: %DATE% %TIME%
echo.
echo Components:
echo - Qt 6.10.1
echo - Opus Codec
echo - PortAudio
echo - OpenSSL 3.x
echo.
echo Encryption: SRTP with AES-128-GCM
echo Key Exchange: X25519 + HKDF-SHA256
) > "%DEPLOY_DIR%\VERSION.txt"

echo.
echo ========================================
echo Deployment Complete!
echo ========================================
echo.
echo Package: %DEPLOY_DIR%
echo.
dir "%DEPLOY_DIR%\*.exe" | find "voip-client.exe"
echo.
echo Next steps:
echo   1. Test locally:
echo      cd %DEPLOY_DIR% ^&^& voip-client.exe
echo.
echo   2. Create ZIP for distribution:
echo      powershell Compress-Archive -Path %DEPLOY_DIR% -DestinationPath VoIP-Client.zip
echo.
echo   3. Or create installer (optional):
echo      Use Inno Setup or NSIS to create installer.exe
echo.
pause
