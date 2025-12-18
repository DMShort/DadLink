# DadLink Client Packaging Script
param(
    [string]$Version = "1.0.0",
    [string]$OutputDir = ".\release-package"
)

Write-Host "Package DadLink Client v$Version" -ForegroundColor Cyan

# Paths
$BuildDir = ".\build\Release"
$QtDir = "C:\Qt\6.10.1\msvc2022_64"
$PackageDir = "$OutputDir\DadLink-$Version"

# Create package directory
if (Test-Path $OutputDir) {
    Remove-Item $OutputDir -Recurse -Force
}
New-Item -ItemType Directory -Path $PackageDir -Force | Out-Null

Write-Host "Created package directory: $PackageDir"

# Copy executable
Write-Host "Copying executable..."
Copy-Item "$BuildDir\voip-client.exe" -Destination "$PackageDir\DadLink.exe"

# Copy required Qt DLLs
Write-Host "Copying Qt libraries..."
$QtDlls = @(
    "Qt6Core.dll",
    "Qt6Gui.dll",
    "Qt6Widgets.dll",
    "Qt6Network.dll",
    "Qt6WebSockets.dll"
)

foreach ($dll in $QtDlls) {
    $sourcePath = "$QtDir\bin\$dll"
    if (Test-Path $sourcePath) {
        Copy-Item $sourcePath -Destination $PackageDir
        Write-Host "  Copied $dll"
    }
}

# Copy Qt plugins
Write-Host "Copying Qt plugins..."
$PluginDirs = @("platforms", "styles", "imageformats", "tls")

foreach ($pluginDir in $PluginDirs) {
    $sourcePath = "$QtDir\plugins\$pluginDir"
    $destPath = "$PackageDir\$pluginDir"
    if (Test-Path $sourcePath) {
        Copy-Item $sourcePath -Destination $destPath -Recurse
        Write-Host "  Copied $pluginDir"
    }
}

# Copy vcpkg DLLs (OpenSSL, Opus, PortAudio)
Write-Host "Copying vcpkg libraries..."
$VcpkgDir = "C:\vcpkg\installed\x64-windows-static-runtime\bin"
$VcpkgDlls = @(
    "libcrypto-3-x64.dll",
    "libssl-3-x64.dll",
    "opus.dll",
    "portaudio.dll"
)
foreach ($dll in $VcpkgDlls) {
    $sourcePath = "$VcpkgDir\$dll"
    if (Test-Path $sourcePath) {
        Copy-Item $sourcePath -Destination $PackageDir
        Write-Host "  Copied $dll"
    } else {
        Write-Host "  Warning: $dll not found at $sourcePath" -ForegroundColor Yellow
    }
}

# Create README
Write-Host "Creating README..."
$readme = @"
DadLink - Voice Communication Client
Version: $Version

ABOUT
DadLink is a low-latency voice communication system designed for gaming
and team coordination. Use it to communicate with your team while playing!

FEATURES
- Global hotkeys (works while gaming!)
- Push-to-Talk on F1-F10 keys
- Multi-channel listening
- TLS encrypted connections
- Admin panel for management

QUICK START
1. Get connection details from your server administrator:
   - Server address
   - Port number (usually 9000)
   - Your username and password

2. Launch DadLink.exe

3. Connect to your server:
   - Enter the server address
   - Enter the port (default: 9000)
   - Enable "Use TLS" (recommended)
   - Click Connect

4. Login with your credentials

5. Join channels from the Voice tab

6. Use F1-F10 to talk (works even when minimized!)

SYSTEM REQUIREMENTS
- Windows 10/11 (64-bit)
- No additional software required! (fully portable)

TROUBLESHOOTING
- Hotkeys don't work: Try running as Administrator
- Can't connect: Verify server address/port with your admin
- Audio issues: Check Windows audio device settings

Support: https://github.com/DMShort/DadLink/issues
"@
$readme | Out-File -FilePath "$PackageDir\README.txt" -Encoding UTF8

# Create ZIP archive
Write-Host "Creating ZIP archive..."
$ZipPath = "$OutputDir\DadLink-$Version-Windows-x64.zip"
Compress-Archive -Path $PackageDir -DestinationPath $ZipPath -Force

Write-Host ""
Write-Host "Package created successfully!" -ForegroundColor Green
Write-Host "Location: $ZipPath" -ForegroundColor Cyan
$sizeInMB = [math]::Round((Get-Item $ZipPath).Length / 1MB, 2)
Write-Host "Size: $sizeInMB MB" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Test on a clean Windows machine"
Write-Host "  2. Create a GitHub Release"
Write-Host "  3. Upload ZIP as release asset"
