# Rebuild client with user ID fix

Write-Host "🛑 Stopping any running client..." -ForegroundColor Yellow
Stop-Process -Name "voip-client" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host "🔨 Rebuilding client..." -ForegroundColor Cyan
cmake --build build --config Debug
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host "✅ Build successful!" -ForegroundColor Green
Write-Host ""
Write-Host "📋 Changes applied:" -ForegroundColor Yellow
Write-Host "  1. Added VoiceSession::set_user_id() method" -ForegroundColor White
Write-Host "  2. Login callback now updates user ID from server response" -ForegroundColor White
Write-Host "  3. Voice packets will use authenticated user ID (not hardcoded 42)" -ForegroundColor White
Write-Host ""
Write-Host "🧪 Testing Instructions:" -ForegroundColor Cyan
Write-Host "  1. Start server: cd ..\server; cargo run" -ForegroundColor White
Write-Host "  2. Start client: cd build\Debug; .\voip-client.exe" -ForegroundColor White
Write-Host "  3. Login as 'dave' (user ID 7)" -ForegroundColor White
Write-Host "  4. Press F1 and speak" -ForegroundColor White
Write-Host "  5. Check server logs - should show 'user=7' not 'user=42'" -ForegroundColor White
Write-Host ""
Write-Host "Expected server logs:" -ForegroundColor Yellow
Write-Host "  📦 Voice packet: seq=0, ch=1, user=7, payload=59B" -ForegroundColor Green
Write-Host "  (NOT user=42!)" -ForegroundColor Green
Write-Host ""
