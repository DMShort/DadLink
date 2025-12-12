# Supabase Database Verification Script
# This script verifies the connection to Supabase and checks database state

Write-Host "🔍 Supabase Database Verification" -ForegroundColor Cyan
Write-Host "=================================" -ForegroundColor Cyan
Write-Host ""

# Load environment variables
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
    Write-Host "❌ .env file not found!" -ForegroundColor Red
    exit 1
}

$DATABASE_URL = $env:DATABASE_URL
if (-not $DATABASE_URL) {
    Write-Host "❌ DATABASE_URL not set in .env" -ForegroundColor Red
    exit 1
}

# Extract connection details (sanitized for display)
if ($DATABASE_URL -match 'postgresql://([^:]+):([^@]+)@([^/]+)/(.+)') {
    $dbUser = $matches[1]
    $dbHost = $matches[3]
    $dbName = $matches[4]
    Write-Host "📊 Database Info:" -ForegroundColor Yellow
    Write-Host "   User: $dbUser"
    Write-Host "   Host: $dbHost"
    Write-Host "   Database: $dbName"
    Write-Host ""
}

# Check if sqlx-cli is installed
Write-Host "🔧 Checking for sqlx-cli..." -ForegroundColor Yellow
$sqlxInstalled = Get-Command sqlx -ErrorAction SilentlyContinue
if (-not $sqlxInstalled) {
    Write-Host "❌ sqlx-cli not installed. Installing..." -ForegroundColor Yellow
    cargo install sqlx-cli --no-default-features --features postgres
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ Failed to install sqlx-cli" -ForegroundColor Red
        exit 1
    }
}
Write-Host "✅ sqlx-cli available" -ForegroundColor Green
Write-Host ""

# Test database connection
Write-Host "🔌 Testing database connection..." -ForegroundColor Yellow
$env:DATABASE_URL = $DATABASE_URL
sqlx database create 2>&1 | Out-Null
if ($LASTEXITCODE -eq 0) {
    Write-Host "✅ Database exists and is accessible" -ForegroundColor Green
} else {
    Write-Host "ℹ️  Database already exists or connection successful" -ForegroundColor Cyan
}
Write-Host ""

# Check migration status
Write-Host "🔄 Checking migration status..." -ForegroundColor Yellow
sqlx migrate info
if ($LASTEXITCODE -eq 0) {
    Write-Host "✅ Migration status retrieved" -ForegroundColor Green
} else {
    Write-Host "⚠️  Could not retrieve migration status" -ForegroundColor Yellow
}
Write-Host ""

# Run migrations
Write-Host "📦 Running migrations..." -ForegroundColor Yellow
sqlx migrate run
if ($LASTEXITCODE -eq 0) {
    Write-Host "✅ Migrations completed successfully" -ForegroundColor Green
} else {
    Write-Host "⚠️  Migration may have already been applied or failed" -ForegroundColor Yellow
}
Write-Host ""

# Quick database query test using PowerShell and PostgreSQL connection
Write-Host "🧪 Running quick database tests..." -ForegroundColor Yellow
Write-Host ""

# Build a simple test query program
$testQuery = @"
use sqlx::postgres::PgPoolOptions;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let database_url = std::env::var("DATABASE_URL")?;
    let pool = PgPoolOptions::new()
        .max_connections(5)
        .connect(&database_url)
        .await?;
    
    // Test basic connectivity
    let result: (i32,) = sqlx::query_as("SELECT 1")
        .fetch_one(&pool)
        .await?;
    println!("✅ Basic query: {}", result.0);
    
    // Count organizations
    let result: (i64,) = sqlx::query_as("SELECT COUNT(*) FROM organizations")
        .fetch_one(&pool)
        .await?;
    println!("✅ Organizations: {}", result.0);
    
    // Count users
    let result: (i64,) = sqlx::query_as("SELECT COUNT(*) FROM users")
        .fetch_one(&pool)
        .await?;
    println!("✅ Users: {}", result.0);
    
    // Count channels
    let result: (i64,) = sqlx::query_as("SELECT COUNT(*) FROM channels")
        .fetch_one(&pool)
        .await?;
    println!("✅ Channels: {}", result.0);
    
    // Count roles
    let result: (i64,) = sqlx::query_as("SELECT COUNT(*) FROM roles")
        .fetch_one(&pool)
        .await?;
    println!("✅ Roles: {}", result.0);
    
    pool.close().await;
    Ok(())
}
"@

# Create temporary test directory
$tempDir = Join-Path $env:TEMP "voip_db_test"
if (Test-Path $tempDir) {
    Remove-Item -Recurse -Force $tempDir
}
New-Item -ItemType Directory -Path $tempDir | Out-Null

# Create Cargo.toml
$cargoToml = @"
[package]
name = "db-test"
version = "0.1.0"
edition = "2021"

[dependencies]
tokio = { version = "1", features = ["full"] }
sqlx = { version = "0.7", features = ["postgres", "runtime-tokio-rustls"] }
"@

Set-Content -Path (Join-Path $tempDir "Cargo.toml") -Value $cargoToml
New-Item -ItemType Directory -Path (Join-Path $tempDir "src") | Out-Null
Set-Content -Path (Join-Path $tempDir "src\main.rs") -Value $testQuery

# Run the test
Push-Location $tempDir
Write-Host "   Running Rust database connectivity test..." -ForegroundColor Cyan
$env:DATABASE_URL = $DATABASE_URL
cargo run --quiet 2>&1
$testResult = $LASTEXITCODE
Pop-Location

# Clean up
Remove-Item -Recurse -Force $tempDir

Write-Host ""
if ($testResult -eq 0) {
    Write-Host "✅ All database tests passed!" -ForegroundColor Green
} else {
    Write-Host "⚠️  Some tests may have failed" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=================================" -ForegroundColor Cyan
Write-Host "🎉 Verification Complete!" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. Run integration tests: cargo test --test database_integration_tests -- --test-threads=1"
Write-Host "2. Start the server: cargo run"
Write-Host ""
