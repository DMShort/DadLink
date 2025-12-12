# Rebuild and restart server with PTT fix

Write-Host "🛑 Stopping any running server..." -ForegroundColor Yellow
Stop-Process -Name "voip-server" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host "🔨 Rebuilding server..." -ForegroundColor Cyan
cargo build
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host "✅ Build successful!" -ForegroundColor Green
Write-Host ""
Write-Host "📋 Changes applied:" -ForegroundColor Yellow
Write-Host "  - Disabled auto-join logic in UDP packet handler" -ForegroundColor White
Write-Host "  - PTT packets no longer add users to listening channels" -ForegroundColor White
Write-Host "  - Users must join channels via WebSocket for listening" -ForegroundColor White
Write-Host ""
Write-Host "🚀 Starting server..." -ForegroundColor Cyan
Write-Host ""

cargo run
