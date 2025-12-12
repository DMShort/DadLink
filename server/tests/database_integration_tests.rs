//! Database Integration Tests for VoIP Server with Supabase
//!
//! These tests verify the database layer works correctly with Supabase PostgreSQL.
//! 
//! Run with: cargo test --test database_integration_tests -- --test-threads=1
//! (Single-threaded to avoid database conflicts)

use sqlx::postgres::PgPoolOptions;
use sqlx::Row;
use std::env;

mod common;

/// Test database connection to Supabase
#[tokio::test]
async fn test_database_connection() {
    let database_url = env::var("DATABASE_URL")
        .expect("DATABASE_URL must be set for integration tests");
    
    println!("🔌 Testing connection to Supabase PostgreSQL...");
    
    let pool = PgPoolOptions::new()
        .max_connections(5)
        .connect(&database_url)
        .await
        .expect("Failed to connect to database");
    
    // Simple health check
    let result = sqlx::query("SELECT 1 as num")
        .fetch_one(&pool)
        .await
        .expect("Failed to execute health check query");
    
    let num: i32 = result.try_get("num").expect("Failed to get result");
    assert_eq!(num, 1);
    
    println!("✅ Database connection successful!");
    pool.close().await;
}

/// Test that migrations have been applied
#[tokio::test]
async fn test_migrations_applied() {
    let pool = common::setup_test_pool().await;
    
    println!("🔄 Checking database migrations...");
    
    // Check that key tables exist
    let tables = vec!["organizations", "users", "channels", "roles", "user_roles", "channel_acl"];
    
    for table in tables {
        let result = sqlx::query(&format!(
            "SELECT EXISTS (
                SELECT FROM information_schema.tables 
                WHERE table_schema = 'public' 
                AND table_name = '{}'
            )"
            , table))
            .fetch_one(&pool)
            .await
            .expect(&format!("Failed to check for table: {}", table));
        
        let exists: bool = result.try_get(0).expect("Failed to get exists result");
        assert!(exists, "Table '{}' should exist after migrations", table);
        println!("✅ Table '{}' exists", table);
    }
    
    pool.close().await;
}

/// Test organization operations
#[tokio::test]
async fn test_organization_crud() {
    let pool = common::setup_test_pool().await;
    
    println!("🏢 Testing organization operations...");
    
    // Create organization
    let result = sqlx::query(
        "INSERT INTO organizations (name, tag, max_users, max_channels) 
         VALUES ($1, $2, $3, $4) 
         RETURNING id"
    )
    .bind("Test Org")
    .bind("TEST")
    .bind(100)
    .bind(50)
    .fetch_one(&pool)
    .await
    .expect("Failed to create organization");
    
    let org_id: i32 = result.try_get("id").expect("Failed to get org ID");
    println!("✅ Created organization with ID: {}", org_id);
    
    // Read organization
    let result = sqlx::query("SELECT name, tag FROM organizations WHERE id = $1")
        .bind(org_id)
        .fetch_one(&pool)
        .await
        .expect("Failed to fetch organization");
    
    let name: String = result.try_get("name").unwrap();
    let tag: String = result.try_get("tag").unwrap();
    assert_eq!(name, "Test Org");
    assert_eq!(tag, "TEST");
    println!("✅ Organization data verified");
    
    // Update organization
    sqlx::query("UPDATE organizations SET name = $1 WHERE id = $2")
        .bind("Updated Test Org")
        .bind(org_id)
        .execute(&pool)
        .await
        .expect("Failed to update organization");
    println!("✅ Organization updated");
    
    // Delete organization
    sqlx::query("DELETE FROM organizations WHERE id = $1")
        .bind(org_id)
        .execute(&pool)
        .await
        .expect("Failed to delete organization");
    println!("✅ Organization deleted");
    
    pool.close().await;
}

/// Test user operations
#[tokio::test]
async fn test_user_crud() {
    let pool = common::setup_test_pool().await;
    
    println!("👤 Testing user operations...");
    
    // Use the demo organization (ID: 1) which should exist from seed data
    let org_id = 1;
    
    // Create user
    let username = format!("test_user_{}", chrono::Utc::now().timestamp());
    let result = sqlx::query(
        "INSERT INTO users (org_id, username, email, password_hash) 
         VALUES ($1, $2, $3, $4) 
         RETURNING id"
    )
    .bind(org_id)
    .bind(&username)
    .bind("test@example.com")
    .bind("$argon2id$v=19$m=19456,t=2,p=1$test$testhash") // Dummy hash
    .fetch_one(&pool)
    .await
    .expect("Failed to create user");
    
    let user_id: i32 = result.try_get("id").expect("Failed to get user ID");
    println!("✅ Created user '{}' with ID: {}", username, user_id);
    
    // Read user
    let result = sqlx::query("SELECT username, email FROM users WHERE id = $1")
        .bind(user_id)
        .fetch_one(&pool)
        .await
        .expect("Failed to fetch user");
    
    let fetched_username: String = result.try_get("username").unwrap();
    assert_eq!(fetched_username, username);
    println!("✅ User data verified");
    
    // Update user email
    sqlx::query("UPDATE users SET email = $1 WHERE id = $2")
        .bind("updated@example.com")
        .bind(user_id)
        .execute(&pool)
        .await
        .expect("Failed to update user");
    println!("✅ User updated");
    
    // Delete user
    sqlx::query("DELETE FROM users WHERE id = $1")
        .bind(user_id)
        .execute(&pool)
        .await
        .expect("Failed to delete user");
    println!("✅ User deleted");
    
    pool.close().await;
}

/// Test channel operations
#[tokio::test]
async fn test_channel_crud() {
    let pool = common::setup_test_pool().await;
    
    println!("📢 Testing channel operations...");
    
    // Use the demo organization (ID: 1)
    let org_id = 1;
    
    // Create channel
    let channel_name = format!("Test Channel {}", chrono::Utc::now().timestamp());
    let result = sqlx::query(
        "INSERT INTO channels (org_id, name, description, max_users, position) 
         VALUES ($1, $2, $3, $4, $5) 
         RETURNING id"
    )
    .bind(org_id)
    .bind(&channel_name)
    .bind("Test channel description")
    .bind(25)
    .bind(99)
    .fetch_one(&pool)
    .await
    .expect("Failed to create channel");
    
    let channel_id: i32 = result.try_get("id").expect("Failed to get channel ID");
    println!("✅ Created channel '{}' with ID: {}", channel_name, channel_id);
    
    // Read channel
    let result = sqlx::query("SELECT name, description FROM channels WHERE id = $1")
        .bind(channel_id)
        .fetch_one(&pool)
        .await
        .expect("Failed to fetch channel");
    
    let fetched_name: String = result.try_get("name").unwrap();
    assert_eq!(fetched_name, channel_name);
    println!("✅ Channel data verified");
    
    // Get all channels in org
    let results = sqlx::query("SELECT COUNT(*) as count FROM channels WHERE org_id = $1")
        .bind(org_id)
        .fetch_one(&pool)
        .await
        .expect("Failed to count channels");
    
    let count: i64 = results.try_get("count").unwrap();
    println!("✅ Organization has {} channels", count);
    assert!(count > 0);
    
    // Delete channel
    sqlx::query("DELETE FROM channels WHERE id = $1")
        .bind(channel_id)
        .execute(&pool)
        .await
        .expect("Failed to delete channel");
    println!("✅ Channel deleted");
    
    pool.close().await;
}

/// Test role and permission operations
#[tokio::test]
async fn test_roles_and_permissions() {
    let pool = common::setup_test_pool().await;
    
    println!("🔐 Testing roles and permissions...");
    
    // Check that default roles exist
    let result = sqlx::query("SELECT COUNT(*) as count FROM roles WHERE org_id = 1")
        .fetch_one(&pool)
        .await
        .expect("Failed to count roles");
    
    let count: i64 = result.try_get("count").unwrap();
    println!("✅ Found {} default roles", count);
    assert!(count >= 4, "Should have at least 4 default roles (Admin, Officer, Member, Guest)");
    
    // Fetch specific role
    let result = sqlx::query("SELECT id, name, permissions, priority FROM roles WHERE org_id = 1 AND name = 'Admin'")
        .fetch_one(&pool)
        .await
        .expect("Failed to fetch Admin role");
    
    let role_name: String = result.try_get("name").unwrap();
    let permissions: i32 = result.try_get("permissions").unwrap();
    let priority: i32 = result.try_get("priority").unwrap();
    
    assert_eq!(role_name, "Admin");
    assert_eq!(permissions, 255); // All permissions
    assert_eq!(priority, 100);
    println!("✅ Admin role verified: permissions={}, priority={}", permissions, priority);
    
    pool.close().await;
}

/// Test foreign key constraints
#[tokio::test]
async fn test_foreign_key_constraints() {
    let pool = common::setup_test_pool().await;
    
    println!("🔗 Testing foreign key constraints...");
    
    // Try to create a user with non-existent org_id (should fail)
    let result = sqlx::query(
        "INSERT INTO users (org_id, username, email, password_hash) 
         VALUES ($1, $2, $3, $4)"
    )
    .bind(99999) // Non-existent org
    .bind("invalid_user")
    .bind("invalid@example.com")
    .bind("hash")
    .execute(&pool)
    .await;
    
    assert!(result.is_err(), "Should fail to create user with invalid org_id");
    println!("✅ Foreign key constraint enforced (user -> organization)");
    
    // Try to create a channel with non-existent parent_id (should fail)
    let result = sqlx::query(
        "INSERT INTO channels (org_id, parent_id, name, position) 
         VALUES ($1, $2, $3, $4)"
    )
    .bind(1)
    .bind(99999) // Non-existent parent channel
    .bind("invalid_channel")
    .bind(0)
    .execute(&pool)
    .await;
    
    assert!(result.is_err(), "Should fail to create channel with invalid parent_id");
    println!("✅ Foreign key constraint enforced (channel -> parent channel)");
    
    pool.close().await;
}

/// Test cascade delete operations
#[tokio::test]
async fn test_cascade_deletes() {
    let pool = common::setup_test_pool().await;
    
    println!("🗑️ Testing cascade delete operations...");
    
    // Create a test organization
    let result = sqlx::query(
        "INSERT INTO organizations (name, tag) VALUES ($1, $2) RETURNING id"
    )
    .bind("Delete Test Org")
    .bind("DELTEST")
    .fetch_one(&pool)
    .await
    .expect("Failed to create test organization");
    
    let org_id: i32 = result.try_get("id").unwrap();
    
    // Create a user in that organization
    let result = sqlx::query(
        "INSERT INTO users (org_id, username, password_hash) 
         VALUES ($1, $2, $3) RETURNING id"
    )
    .bind(org_id)
    .bind("delete_test_user")
    .bind("hash")
    .fetch_one(&pool)
    .await
    .expect("Failed to create test user");
    
    let user_id: i32 = result.try_get("id").unwrap();
    println!("✅ Created test user ID: {}", user_id);
    
    // Delete the organization (should cascade to users)
    sqlx::query("DELETE FROM organizations WHERE id = $1")
        .bind(org_id)
        .execute(&pool)
        .await
        .expect("Failed to delete organization");
    
    // Verify user was also deleted
    let result = sqlx::query("SELECT id FROM users WHERE id = $1")
        .bind(user_id)
        .fetch_optional(&pool)
        .await
        .expect("Failed to check for user");
    
    assert!(result.is_none(), "User should be deleted when organization is deleted");
    println!("✅ Cascade delete verified (organization -> users)");
    
    pool.close().await;
}

/// Test audit logging
#[tokio::test]
async fn test_audit_logging() {
    let pool = common::setup_test_pool().await;
    
    println!("📝 Testing audit log functionality...");
    
    // Create an audit log entry
    let result = sqlx::query(
        "INSERT INTO audit_log (org_id, user_id, action, resource_type, resource_id, details) 
         VALUES ($1, $2, $3, $4, $5, $6) 
         RETURNING id"
    )
    .bind(1)
    .bind(1)
    .bind("test_action")
    .bind("test_resource")
    .bind(123)
    .bind(sqlx::types::Json(serde_json::json!({
        "test": "data",
        "timestamp": chrono::Utc::now().to_rfc3339()
    })))
    .fetch_one(&pool)
    .await
    .expect("Failed to create audit log entry");
    
    let log_id: i64 = result.try_get("id").unwrap();
    println!("✅ Created audit log entry ID: {}", log_id);
    
    // Read audit log entry
    let result = sqlx::query("SELECT action, resource_type, details FROM audit_log WHERE id = $1")
        .bind(log_id)
        .fetch_one(&pool)
        .await
        .expect("Failed to fetch audit log");
    
    let action: String = result.try_get("action").unwrap();
    assert_eq!(action, "test_action");
    println!("✅ Audit log entry verified");
    
    pool.close().await;
}

/// Test database indexes exist
#[tokio::test]
async fn test_database_indexes() {
    let pool = common::setup_test_pool().await;
    
    println!("📇 Testing database indexes...");
    
    let expected_indexes = vec![
        "idx_users_org",
        "idx_users_username",
        "idx_roles_org",
        "idx_channels_org",
        "idx_channels_parent",
        "idx_sessions_user",
        "idx_sessions_expires",
        "idx_audit_log_org",
        "idx_audit_log_user",
        "idx_audit_log_created",
    ];
    
    for index_name in expected_indexes {
        let result = sqlx::query(
            "SELECT EXISTS (
                SELECT FROM pg_indexes 
                WHERE schemaname = 'public' 
                AND indexname = $1
            )"
        )
        .bind(index_name)
        .fetch_one(&pool)
        .await
        .expect(&format!("Failed to check for index: {}", index_name));
        
        let exists: bool = result.try_get(0).unwrap();
        assert!(exists, "Index '{}' should exist", index_name);
        println!("✅ Index '{}' exists", index_name);
    }
    
    pool.close().await;
}
