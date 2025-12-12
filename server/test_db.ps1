# Simple Database Test Runner
param([string]$TestName = "")

Write-Host "🧪 Database Integration Tests" -ForegroundColor Cyan
Write-Host ""

# Load .env
if (Test-Path ".env") {
    Get-Content ".env" | ForEach-Object {
        if ($_ -match '^\s*([^#][^=]+)=(.*)$') {
            $name = $matches[1].Trim()
            $value = $matches[2].Trim()
            [Environment]::SetEnvironmentVariable($name, $value, "Process")
        }
    }
    Write-Host "✅ Loaded .env file" -ForegroundColor Green
} else {
    Write-Host "❌ .env file not found" -ForegroundColor Red
    exit 1
}

if (-not $env:DATABASE_URL) {
    Write-Host "❌ DATABASE_URL not set" -ForegroundColor Red
    exit 1
}

Write-Host ""

# Run test
if ($TestName) {
    Write-Host "Running test: $TestName" -ForegroundColor Yellow
    cargo test --test database_integration_tests $TestName -- --test-threads=1 --nocapture
} else {
    Write-Host "Running all database integration tests..." -ForegroundColor Yellow
    cargo test --test database_integration_tests -- --test-threads=1 --nocapture
}

exit $LASTEXITCODE
