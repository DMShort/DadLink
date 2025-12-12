use crate::config::DatabaseConfig;
use crate::error::Result;
use sqlx::postgres::{PgPool, PgPoolOptions};
use sqlx::migrate::MigrateDatabase;
use tracing::info;

/// Database connection pool
#[derive(Clone)]
pub struct Database {
    pool: PgPool,
}

impl Database {
    /// Create a new database connection pool
    pub async fn new(config: &DatabaseConfig) -> Result<Self> {
        info!("🔌 Connecting to database: {}", sanitize_db_url(&config.url));

        // Check if database exists, create if not
        if !sqlx::Postgres::database_exists(&config.url).await.unwrap_or(false) {
            info!("📦 Database does not exist, creating...");
            sqlx::Postgres::create_database(&config.url)
                .await?;
            info!("✅ Database created successfully");
        }

        // Create connection pool
        let pool = PgPoolOptions::new()
            .max_connections(config.max_connections)
            .connect(&config.url)
            .await?;

        info!("✅ Database connection pool created ({} max connections)", config.max_connections);

        Ok(Self { pool })
    }

    /// Run database migrations
    pub async fn migrate(&self) -> Result<()> {
        info!("🔄 Running database migrations...");

        sqlx::migrate!("./migrations")
            .run(&self.pool)
            .await?;

        info!("✅ Database migrations completed successfully");
        Ok(())
    }

    /// Get a reference to the connection pool
    pub fn pool(&self) -> &PgPool {
        &self.pool
    }

    /// Check database health
    pub async fn health_check(&self) -> Result<()> {
        sqlx::query("SELECT 1")
            .execute(&self.pool)
            .await?;

        Ok(())
    }

    /// Get database statistics
    pub async fn stats(&self) -> DatabaseStats {
        DatabaseStats {
            pool_size: self.pool.size() as u32,
            idle_connections: self.pool.num_idle() as u32,
        }
    }

    /// Close the database connection pool
    pub async fn close(&self) {
        self.pool.close().await;
        info!("🔌 Database connection pool closed");
    }
}

/// Database statistics
#[derive(Debug, Clone)]
pub struct DatabaseStats {
    pub pool_size: u32,
    pub idle_connections: u32,
}

/// Sanitize database URL for logging (hide password)
fn sanitize_db_url(url: &str) -> String {
    if let Some(at_pos) = url.rfind('@') {
        if let Some(colon_pos) = url[..at_pos].rfind(':') {
            let mut sanitized = url.to_string();
            sanitized.replace_range(colon_pos + 1..at_pos, "****");
            return sanitized;
        }
    }
    url.to_string()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_sanitize_db_url() {
        let url = "postgresql://voip:secret_password@localhost/voip";
        let sanitized = sanitize_db_url(url);
        assert!(!sanitized.contains("secret_password"));
        assert!(sanitized.contains("****"));
        assert!(sanitized.contains("postgresql://voip:"));
        assert!(sanitized.contains("@localhost/voip"));
    }
}
