# Quick PTT Test Script
# This script starts the client and shows what to look for

Write-Host "🎤 VoIP Client PTT Test" -ForegroundColor Cyan
Write-Host ""
Write-Host "📋 What to watch for:" -ForegroundColor Yellow
Write-Host "  1. ✅ Voice session initialized (not started yet)" -ForegroundColor White
Write-Host "  2. ✅ Login SUCCESS! User ID: X" -ForegroundColor White
Write-Host "  3. 🔧 Updating VoiceSession user ID from hardcoded 42 to X" -ForegroundColor White
Write-Host "  4. 🆔 VoiceSession user ID updated to: X" -ForegroundColor White
Write-Host "  5. ✅ Join channel 1 request sent" -ForegroundColor White
Write-Host "  6. 🎤 Starting voice session now that channel is joined..." -ForegroundColor White
Write-Host "  7. ✅ Voice session started successfully!" -ForegroundColor White
Write-Host "  8. ✅ Voice session ready - you can now use PTT!" -ForegroundColor Green
Write-Host ""
Write-Host "🎮 PTT Controls:" -ForegroundColor Yellow
Write-Host "  F1 = PTT to Channel 1 (General)" -ForegroundColor White
Write-Host "  F2 = PTT to Channel 2 (Gaming)" -ForegroundColor White
Write-Host "  F3 = PTT to Channel 3 (Music)" -ForegroundColor White
Write-Host "  F4 = PTT to Channel 4 (AFK)" -ForegroundColor White
Write-Host ""
Write-Host "🧪 Test Procedure:" -ForegroundColor Yellow
Write-Host "  1. Start server in another terminal: cd ..\server; cargo run" -ForegroundColor White
Write-Host "  2. Start this client (press Enter)" -ForegroundColor White
Write-Host "  3. Login (e.g., dave / pass123)" -ForegroundColor White
Write-Host "  4. Wait for 'Voice session ready' message" -ForegroundColor White
Write-Host "  5. Press F1 and speak" -ForegroundColor White
Write-Host "  6. Check server logs for '🔊 Routed voice from user X'" -ForegroundColor White
Write-Host ""
Write-Host "⚠️  Multi-User Test:" -ForegroundColor Yellow
Write-Host "  1. Start second client in another terminal" -ForegroundColor White
Write-Host "  2. Login as different user (e.g., bob / pass123)" -ForegroundColor White
Write-Host "  3. Both should see 2 users in channel list" -ForegroundColor White
Write-Host "  4. When one presses F1, the other should HEAR audio!" -ForegroundColor Green
Write-Host ""
Write-Host "Press Enter to start client..." -ForegroundColor Cyan
Read-Host

cd build\Debug
.\voip-client.exe
