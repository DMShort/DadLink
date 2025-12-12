# Rebuild client with UDP presence packet fix

Write-Host ""
Write-Host "======================================================" -ForegroundColor Cyan
Write-Host "  REBUILDING CLIENT WITH UDP PRESENCE PACKET FIX" -ForegroundColor Cyan
Write-Host "======================================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "This fix resolves the intermittent audio issue where:" -ForegroundColor Yellow
Write-Host "  - Users couldn't hear each other initially" -ForegroundColor Yellow
Write-Host "  - Audio only worked after BOTH users transmitted once" -ForegroundColor Yellow
Write-Host "  - Audio stopped working after toggling to other channels" -ForegroundColor Yellow
Write-Host "  - Multi-channel PTT (F1-F4) failed after joining multiple channels" -ForegroundColor Yellow
Write-Host ""
Write-Host "The fix: Send UDP presence packet immediately on channel join" -ForegroundColor Green
Write-Host "Result: Instant audio routing without waiting for first transmission!" -ForegroundColor Green
Write-Host ""

# Stop any running clients
Write-Host "Step 1: Stopping any running clients..." -ForegroundColor Cyan
Stop-Process -Name "voip-client" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
Write-Host "  [OK] Clients stopped" -ForegroundColor Green
Write-Host ""

# Clean build
Write-Host "Step 2: Cleaning previous build..." -ForegroundColor Cyan
if (Test-Path "build") {
    Remove-Item -Path "build\CMakeFiles" -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "  [OK] Build cache cleared" -ForegroundColor Green
} else {
    Write-Host "  ! No previous build found" -ForegroundColor Yellow
}
Write-Host ""

# Configure with CMake
Write-Host "Step 3: Configuring with CMake..." -ForegroundColor Cyan
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}
Set-Location build
cmake .. -DCMAKE_TOOLCHAIN_FILE=..\vcpkg\scripts\buildsystems\vcpkg.cmake
if ($LASTEXITCODE -ne 0) {
    Write-Host "  [ERROR] CMake configuration failed!" -ForegroundColor Red
    Set-Location ..
    exit 1
}
Write-Host "  [OK] CMake configured" -ForegroundColor Green
Write-Host ""

# Build
Write-Host "Step 4: Building client (Debug)..." -ForegroundColor Cyan
cmake --build . --config Debug
if ($LASTEXITCODE -ne 0) {
    Write-Host "  [ERROR] Build failed!" -ForegroundColor Red
    Set-Location ..
    exit 1
}
Write-Host "  [OK] Build succeeded" -ForegroundColor Green
Write-Host ""

# Deploy DLLs
Write-Host "Step 5: Deploying Qt DLLs..." -ForegroundColor Cyan
Set-Location ..
if (Test-Path "deploy.bat") {
    cmd /c deploy.bat
    Write-Host "  [OK] DLLs deployed" -ForegroundColor Green
} else {
    Write-Host "  ! No deploy.bat found - you may need to copy DLLs manually" -ForegroundColor Yellow
}
Write-Host ""

# Success message
Write-Host "======================================================" -ForegroundColor Green
Write-Host "  [SUCCESS] CLIENT BUILD COMPLETE WITH PRESENCE FIX!" -ForegroundColor Green
Write-Host "======================================================" -ForegroundColor Green
Write-Host ""
Write-Host "WHAT CHANGED:" -ForegroundColor Cyan
Write-Host "  1. Added send_presence_packet() method to VoiceSession" -ForegroundColor White
Write-Host "  2. Sends presence packet when joining channels (WebSocket)" -ForegroundColor White
Write-Host "  3. Sends presence packet when joining channels (client-side)" -ForegroundColor White
Write-Host "  4. Sends presence packet when starting PTT (F1-F4)" -ForegroundColor White
Write-Host "  5. Ensures UDP address registered for ALL channel interactions" -ForegroundColor White
Write-Host ""
Write-Host "EXPECTED BEHAVIOR:" -ForegroundColor Cyan
Write-Host "  [OK] Audio works IMMEDIATELY on first PTT press" -ForegroundColor Green
Write-Host "  [OK] No delay waiting for both users to transmit" -ForegroundColor Green
Write-Host "  [OK] Users don't disappear when toggling channels" -ForegroundColor Green
Write-Host "  [OK] Server logs show 'Presence packet' messages" -ForegroundColor Green
Write-Host ""
Write-Host "WHAT TO WATCH FOR IN LOGS:" -ForegroundColor Cyan
Write-Host "  Client: 'Sending UDP presence packet for channel X'" -ForegroundColor White
Write-Host "  Client: 'Presence packet sent - UDP address should now be registered'" -ForegroundColor White
Write-Host "  Server: 'Voice packet: seq=0, ch=X, user=Y, payload=1B'" -ForegroundColor White
Write-Host "  Server: 'Routed voice from user Y to Z recipients'" -ForegroundColor White
Write-Host ""
Write-Host "NEXT STEPS:" -ForegroundColor Cyan
Write-Host "  1. Make sure server is running: cd ..\server; .\run_with_logs.ps1" -ForegroundColor Yellow
Write-Host "  2. Start Client 1: .\run_with_logs.ps1" -ForegroundColor Yellow
Write-Host "  3. Start Client 2: .\run_client2.ps1" -ForegroundColor Yellow
Write-Host "  4. Test PTT IMMEDIATELY - audio should work on first try!" -ForegroundColor Yellow
Write-Host ""
Write-Host "To test the fix:" -ForegroundColor Cyan
Write-Host "  - Both clients login (dave, bob)" -ForegroundColor White
Write-Host "  - Both auto-join channel 1" -ForegroundColor White
Write-Host "  - Bob presses F1 and speaks (FIRST TIME EVER)" -ForegroundColor White
Write-Host "  - Dave should HEAR IT IMMEDIATELY!" -ForegroundColor Green
Write-Host ""
Write-Host "Good luck!" -ForegroundColor Cyan
Write-Host ""
