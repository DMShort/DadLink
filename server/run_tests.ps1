# VoIP Server Database Test Runner
# Loads environment variables and runs integration tests

Write-Host "🧪 VoIP Server Database Test Runner" -ForegroundColor Cyan
Write-Host "====================================" -ForegroundColor Cyan
Write-Host ""

# Load environment variables from .env
if (Test-Path ".env") {
    Write-Host "📄 Loading environment from .env..." -ForegroundColor Yellow
    Get-Content ".env" | ForEach-Object {
        if ($_ -match '^\s*([^#][^=]+)=(.*)$') {
            $name = $matches[1].Trim()
            $value = $matches[2].Trim()
            [Environment]::SetEnvironmentVariable($name, $value, "Process")
            Write-Host "   ✅ Set $name" -ForegroundColor Green
        }
    }
    Write-Host ""
} else {
    Write-Host "❌ .env file not found!" -ForegroundColor Red
    Write-Host "   Create server/.env with DATABASE_URL" -ForegroundColor Yellow
    exit 1
}

# Verify DATABASE_URL is set
if (-not $env:DATABASE_URL) {
    Write-Host "❌ DATABASE_URL not set in environment" -ForegroundColor Red
    exit 1
}

Write-Host "✅ Environment configured" -ForegroundColor Green
Write-Host ""

# Parse test arguments
if ($args.Count -eq 0) {
    Write-Host "🔬 Running all database integration tests..." -ForegroundColor Cyan
    $testCommand = "cargo", "test", "--test", "database_integration_tests", "--", "--test-threads=1", "--nocapture"
} else {
    Write-Host "🔬 Running specific tests: $($args -join ' ')" -ForegroundColor Cyan
    $testCommand = @("cargo", "test", "--test", "database_integration_tests") + $args + @("--", "--test-threads=1", "--nocapture")
}

Write-Host ""
Write-Host "Command: $($testCommand -join ' ')" -ForegroundColor Gray
Write-Host ""

# Run tests
& $testCommand[0] @($testCommand[1..($testCommand.Length-1)])

$exitCode = $LASTEXITCODE

Write-Host ""
if ($exitCode -eq 0) {
    Write-Host "✅ All tests passed!" -ForegroundColor Green
} else {
    Write-Host "❌ Tests failed (exit code: $exitCode)" -ForegroundColor Red
}

Write-Host ""
exit $exitCode
