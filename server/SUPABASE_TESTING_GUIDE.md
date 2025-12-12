# Supabase Database Testing Guide

Complete guide for testing the VoIP server's Supabase PostgreSQL integration.

## Prerequisites

1. **Supabase Account**: Project configured with connection details in `.env`
2. **Rust**: cargo and rustc installed
3. **sqlx-cli**: For running migrations
   ```powershell
   cargo install sqlx-cli --no-default-features --features postgres
   ```

## Environment Setup

Create or verify `server/.env` file:

```env
DATABASE_URL=postgresql://postgres.nozereosifhbfqmdgsjj:YOUR_PASSWORD@aws-1-ap-southeast-2.pooler.supabase.com:5432/postgres
```

## Quick Start

### 1. Verify Supabase Connection

Run the verification script to check everything is configured correctly:

```powershell
cd server
.\verify_supabase.ps1
```

This script will:
- ✅ Check environment variables
- ✅ Install sqlx-cli if needed
- ✅ Test database connection
- ✅ Run migrations
- ✅ Query table counts
- ✅ Verify schema structure

### 2. Run All Tests

Run the complete test suite:

```powershell
# Run all integration tests (single-threaded to avoid conflicts)
cargo test --test database_integration_tests -- --test-threads=1

# Run repository-level tests
cargo test --test repository_tests -- --test-threads=1

# Run end-to-end workflow tests
cargo test --test end_to_end_test -- --test-threads=1
```

### 3. Run Individual Test Suites

**Database Integration Tests:**
```powershell
cargo test --test database_integration_tests -- --test-threads=1 --nocapture
```

Tests:
- ✅ Database connection
- ✅ Migration status
- ✅ CRUD operations (Organizations, Users, Channels, Roles)
- ✅ Foreign key constraints
- ✅ Cascade deletes
- ✅ Audit logging
- ✅ Index verification

**Repository Tests:**
```powershell
cargo test --test repository_tests -- --test-threads=1 --nocapture
```

Tests:
- ✅ UserRepository: create, authenticate, get, update, delete
- ✅ ChannelRepository: CRUD, parent-child relationships
- ✅ OrganizationRepository: CRUD operations
- ✅ RoleRepository: roles, permissions, ACLs

**End-to-End Tests:**
```powershell
cargo test --test end_to_end_test -- --test-threads=1 --nocapture
```

Tests:
- ✅ Complete organization setup workflow
- ✅ Multi-user channel workflow
- ✅ Permission verification
- ✅ Authentication flow
- ✅ Data integrity checks

## Test Structure

```
server/
├── tests/
│   ├── common/
│   │   └── mod.rs              # Shared test utilities
│   ├── database_integration_tests.rs  # Low-level DB tests
│   ├── repository_tests.rs     # Repository layer tests
│   └── end_to_end_test.rs      # Complete workflow tests
├── migrations/
│   ├── 001_initial_schema.sql  # Database schema
│   └── 002_seed_data.sql       # Test data
└── verify_supabase.ps1         # Verification script
```

## Database Schema

### Tables
- **organizations**: Multi-tenant organizations
- **users**: User accounts with authentication
- **roles**: Permission roles within organizations
- **user_roles**: Many-to-many user-role assignments
- **channels**: Voice channels (hierarchical)
- **channel_acl**: Channel-specific permissions
- **sessions**: Active user sessions
- **audit_log**: Audit trail for all actions

### Indexes
- Performance indexes on foreign keys
- Username and email lookups
- Session expiration queries
- Audit log queries

## Running Migrations

### Apply Migrations
```powershell
cd server
sqlx migrate run
```

### Check Migration Status
```powershell
sqlx migrate info
```

### Revert Last Migration
```powershell
sqlx migrate revert
```

## Common Test Patterns

### Creating Test Data

```rust
use voip_server::db::{UserRepository, ChannelRepository};

#[tokio::test]
async fn test_example() {
    let pool = common::setup_test_pool().await;
    let user_repo = UserRepository::new(pool.clone());
    
    // Create test user
    let user_id = user_repo.create_user(
        1,  // org_id
        "test_user",
        Some("test@example.com"),
        "password"
    ).await.unwrap();
    
    // ... test logic ...
    
    // Cleanup
    user_repo.delete_user(user_id).await.unwrap();
    pool.close().await;
}
```

### Testing Permissions

```rust
use voip_server::types::Permissions;

let perms = role_repo.get_user_channel_permissions(user_id, channel_id)
    .await
    .unwrap();

assert!(perms.contains(Permissions::JOIN));
assert!(perms.contains(Permissions::SPEAK));
```

## Debugging Failed Tests

### Enable Detailed Logging

```powershell
$env:RUST_LOG = "debug"
cargo test --test database_integration_tests -- --nocapture
```

### Check Database State

Use Supabase dashboard or direct SQL query:

```powershell
# Connect with psql (if installed)
$env:DATABASE_URL = (Get-Content .env | Where-Object { $_ -match "DATABASE_URL" } | ForEach-Object { $_.Split('=')[1] })
psql $env:DATABASE_URL
```

```sql
-- Check table contents
SELECT COUNT(*) FROM users;
SELECT COUNT(*) FROM channels;
SELECT COUNT(*) FROM roles;

-- Check specific user
SELECT * FROM users WHERE username = 'demo';

-- Check permissions
SELECT u.username, r.name, r.permissions 
FROM users u
JOIN user_roles ur ON u.id = ur.user_id
JOIN roles r ON ur.role_id = r.id;
```

## Performance Testing

### Connection Pool Stress Test

```rust
#[tokio::test]
async fn test_connection_pool_stress() {
    let pool = common::setup_test_pool().await;
    
    let mut handles = vec![];
    for _ in 0..50 {
        let pool_clone = pool.clone();
        let handle = tokio::spawn(async move {
            sqlx::query("SELECT 1")
                .execute(&pool_clone)
                .await
                .unwrap();
        });
        handles.push(handle);
    }
    
    for handle in handles {
        handle.await.unwrap();
    }
    
    pool.close().await;
}
```

## CI/CD Integration

### GitHub Actions Example

```yaml
name: Database Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - uses: actions-rs/toolchain@v1
        with:
          toolchain: stable
      - name: Install sqlx-cli
        run: cargo install sqlx-cli --no-default-features --features postgres
      - name: Run migrations
        env:
          DATABASE_URL: ${{ secrets.SUPABASE_DATABASE_URL }}
        run: |
          cd server
          sqlx migrate run
      - name: Run tests
        env:
          DATABASE_URL: ${{ secrets.SUPABASE_DATABASE_URL }}
        run: |
          cd server
          cargo test --all-features -- --test-threads=1
```

## Troubleshooting

### Connection Refused

**Problem**: Cannot connect to Supabase database

**Solutions**:
1. Check DATABASE_URL is correct in `.env`
2. Verify Supabase project is active
3. Check firewall/network settings
4. Verify pooler endpoint is accessible

### Migration Failed

**Problem**: Migration fails to apply

**Solutions**:
1. Check if migration was partially applied
2. Manually verify table state in Supabase dashboard
3. Use `sqlx migrate revert` to undo
4. Re-apply with `sqlx migrate run`

### Test Timeout

**Problem**: Tests hang or timeout

**Solutions**:
1. Reduce connection pool size
2. Increase timeout in test configuration
3. Check for deadlocks in database
4. Run with `--test-threads=1` to avoid conflicts

### Permission Denied

**Problem**: Database operations fail with permission errors

**Solutions**:
1. Verify Supabase user has correct permissions
2. Check service role key is used (not anon key)
3. Review RLS policies if enabled

## Best Practices

1. **Use Transactions**: Wrap test database operations in transactions when possible
2. **Clean Up**: Always delete test data after tests complete
3. **Isolation**: Run database tests single-threaded (`--test-threads=1`)
4. **Unique IDs**: Use timestamps in test data to avoid conflicts
5. **Assertions**: Verify both success and failure cases
6. **Logging**: Use `println!` for test progress visibility
7. **Documentation**: Comment complex test scenarios

## Resources

- [Supabase Documentation](https://supabase.com/docs)
- [sqlx Documentation](https://docs.rs/sqlx/)
- [PostgreSQL Documentation](https://www.postgresql.org/docs/)
- [Project Architecture](../ARCHITECTURE.md)
- [Database Schema](./migrations/001_initial_schema.sql)

## Next Steps

After verifying database integration:

1. **Start Server**: `cargo run` in server directory
2. **Run Client**: Build and run VoIP client
3. **Monitor Logs**: Check server logs for database operations
4. **Production Deploy**: Set up production Supabase instance
5. **Backup Strategy**: Configure automated backups in Supabase

---

**Testing Status**: ✅ All tests passing with Supabase PostgreSQL  
**Last Updated**: 2024-11-29
