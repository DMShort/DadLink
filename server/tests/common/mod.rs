//! Common test utilities for database integration tests

use sqlx::postgres::{PgPool, PgPoolOptions};
use std::env;

/// Setup a test database pool
pub async fn setup_test_pool() -> PgPool {
    dotenvy::dotenv().ok();
    
    let database_url = env::var("DATABASE_URL")
        .expect("DATABASE_URL must be set for integration tests. Set it in server/.env file");
    
    PgPoolOptions::new()
        .max_connections(5)
        .connect(&database_url)
        .await
        .expect("Failed to connect to test database")
}

/// Clean up test data (use with caution!)
#[allow(dead_code)]
pub async fn cleanup_test_data(pool: &PgPool, org_tag: &str) {
    // Delete test organization and cascading data
    let _ = sqlx::query("DELETE FROM organizations WHERE tag = $1")
        .bind(org_tag)
        .execute(pool)
        .await;
}
