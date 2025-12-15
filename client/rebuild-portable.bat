@echo off
REM Rebuild vcpkg dependencies with static runtime for portable deployment

echo ========================================
echo Rebuilding vcpkg dependencies with static runtime (/MT)
echo This makes DadLink portable (no VC++ redistributable needed)
echo ========================================
echo.

cd C:\vcpkg

echo Removing old packages (dynamic runtime)...
vcpkg remove opus:x64-windows portaudio:x64-windows openssl:x64-windows --recurse
echo.

echo Installing packages with static runtime...
vcpkg install opus:x64-windows-static-runtime
vcpkg install portaudio:x64-windows-static-runtime
vcpkg install openssl:x64-windows-static-runtime
echo.

echo ========================================
echo Done! Dependencies now use static runtime (/MT)
echo.
echo Next steps:
echo 1. cd C:\dev\VoIP-System\client
echo 2. cmake --preset windows-release
echo 3. cmake --build build --config Release
echo 4. Run package-release.ps1
echo ========================================
pause
