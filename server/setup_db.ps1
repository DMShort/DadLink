# Setup database script for VoIP Server
# This script applies migrations to the PostgreSQL database

$env:PGPASSWORD = "voip"

# Combine all migrations
$migration001 = Get-Content "migrations\001_initial_schema.sql" -Raw
$migration002 = Get-Content "migrations\002_seed_data.sql" -Raw

$allMigrations = @"
$migration001

$migration002
"@

# Save to temp file
$tempFile = "temp_migrations.sql"
$allMigrations | Out-File -FilePath $tempFile -Encoding UTF8

# Run migrations using docker exec
Write-Host "Applying database migrations..."
docker exec -i voip-postgres psql -U voip -d voip < $tempFile

# Cleanup
Remove-Item $tempFile

Write-Host "Database setup complete!"
"@