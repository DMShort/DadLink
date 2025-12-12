@echo off
echo Setting up database schema...

REM Apply first migration
echo Applying 001_initial_schema.sql...
docker exec -i voip-postgres psql -U voip -d voip < migrations\001_initial_schema.sql

REM Apply second migration
echo Applying 002_seed_data.sql...
docker exec -i voip-postgres psql -U voip -d voip < migrations\002_seed_data.sql

echo.
echo Database setup complete! Now you can run: cargo check
