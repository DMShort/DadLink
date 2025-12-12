use crate::error::{Result, VoipError};
use crate::types::{UserId, OrgId, User};
use crate::auth;
use sqlx::{PgPool, Row};
use tracing::{info, warn, debug};

/// Repository for user database operations
pub struct UserRepository {
    pool: PgPool,
}

impl UserRepository {
    pub fn new(pool: PgPool) -> Self {
        Self { pool }
    }

    /// Register a new user
    pub async fn create_user(
        &self,
        org_id: OrgId,
        username: &str,
        email: Option<&str>,
        password: &str,
    ) -> Result<UserId> {
        // Hash the password
        let password_hash = auth::hash_password(password)?;

        // Insert user into database
        let result = sqlx::query(
            r#"
            INSERT INTO users (org_id, username, email, password_hash)
            VALUES ($1, $2, $3, $4)
            RETURNING id
            "#
        )
        .bind(org_id as i32)
        .bind(username)
        .bind(email)
        .bind(password_hash)
        .fetch_one(&self.pool)
        .await
        .map_err(|e| {
            if e.to_string().contains("unique") {
                VoipError::Other(format!("Username '{}' already exists", username))
            } else {
                VoipError::DatabaseError(e)
            }
        })?;

        let user_id: i32 = result.try_get("id")?;
        let user_id = user_id as UserId;
        info!("✅ User created: {} (ID: {}, Org: {})", username, user_id, org_id);

        Ok(user_id)
    }

    /// Authenticate a user by username and password
    pub async fn authenticate(
        &self,
        username: &str,
        password: &str,
    ) -> Result<Option<User>> {
        // Fetch user from database
        let result = sqlx::query(
            r#"
            SELECT id, org_id, username, email, password_hash, created_at
            FROM users
            WHERE username = $1
            "#
        )
        .bind(username)
        .fetch_optional(&self.pool)
        .await?;

        let Some(row) = result else {
            debug!("User not found: {}", username);
            return Ok(None);
        };

        // Verify password using Argon2
        let password_hash_str: String = row.try_get("password_hash")?;
        if auth::verify_password(password, &password_hash_str).is_err() {
            debug!("Invalid password for user: {}", username);
            return Ok(None);
        }

        // Update last login
        let row_id: i32 = row.try_get("id")?;
        let _ = sqlx::query(
            r#"
            UPDATE users
            SET last_login = NOW()
            WHERE id = $1
            "#
        )
        .bind(row_id)
        .execute(&self.pool)
        .await;

        let user = User {
            id: row_id as UserId,
            username: row.try_get("username")?,
            password_hash: String::new(), // Don't return password hash
            org_id: row.try_get::<i32, _>("org_id")? as OrgId,
            email: row.try_get("email")?,
            created_at: row.try_get("created_at")?,
        };

        info!("✅ User authenticated: {} (ID: {})", user.username, user.id);
        Ok(Some(user))
    }

    /// Get user by ID
    pub async fn get_user_by_id(&self, user_id: UserId) -> Result<Option<User>> {
        let result = sqlx::query(
            r#"
            SELECT id, org_id, username, email, password_hash, created_at
            FROM users
            WHERE id = $1
            "#
        )
        .bind(user_id as i32)
        .fetch_optional(&self.pool)
        .await?;

        Ok(result.map(|row| -> Result<User> {
            Ok(User {
                id: row.try_get::<i32, _>("id")? as UserId,
                username: row.try_get("username")?,
                password_hash: String::new(), // Don't return password hash
                org_id: row.try_get::<i32, _>("org_id")? as OrgId,
                email: row.try_get("email")?,
                created_at: row.try_get("created_at")?,
            })
        }).transpose()?)
    }

    /// Get user by username
    pub async fn get_user_by_username(
        &self,
        username: &str,
    ) -> Result<Option<User>> {
        let result = sqlx::query(
            r#"
            SELECT id, org_id, username, email, password_hash, created_at
            FROM users
            WHERE username = $1
            "#
        )
        .bind(username)
        .fetch_optional(&self.pool)
        .await?;

        Ok(result.map(|row| -> Result<User> {
            Ok(User {
                id: row.try_get::<i32, _>("id")? as UserId,
                username: row.try_get("username")?,
                password_hash: String::new(), // Don't return password hash
                org_id: row.try_get::<i32, _>("org_id")? as OrgId,
                email: row.try_get("email")?,
                created_at: row.try_get("created_at")?,
            })
        }).transpose()?)
    }

    /// Get all users in an organization
    pub async fn get_users_by_org(&self, org_id: OrgId) -> Result<Vec<User>> {
        let results = sqlx::query(
            r#"
            SELECT id, org_id, username, email, password_hash, created_at
            FROM users
            WHERE org_id = $1
            ORDER BY username
            "#
        )
        .bind(org_id as i32)
        .fetch_all(&self.pool)
        .await?;

        results
            .into_iter()
            .map(|row| -> Result<User> {
                Ok(User {
                    id: row.try_get::<i32, _>("id")? as UserId,
                    username: row.try_get("username")?,
                    password_hash: String::new(),
                    org_id: row.try_get::<i32, _>("org_id")? as OrgId,
                    email: row.try_get("email")?,
                    created_at: row.try_get("created_at")?,
                })
            })
            .collect()
    }

    /// Update user email
    pub async fn update_email(
        &self,
        user_id: UserId,
        email: Option<&str>,
    ) -> Result<()> {
        sqlx::query(
            r#"
            UPDATE users
            SET email = $1
            WHERE id = $2
            "#
        )
        .bind(email)
        .bind(user_id as i32)
        .execute(&self.pool)
        .await?;

        Ok(())
    }

    /// Update user password
    pub async fn update_password(
        &self,
        user_id: UserId,
        new_password: &str,
    ) -> Result<()> {
        let password_hash = auth::hash_password(new_password)?;

        sqlx::query(
            r#"
            UPDATE users
            SET password_hash = $1
            WHERE id = $2
            "#
        )
        .bind(password_hash)
        .bind(user_id as i32)
        .execute(&self.pool)
        .await?;

        info!("🔐 Password updated for user ID: {}", user_id);
        Ok(())
    }

    /// Delete a user
    pub async fn delete_user(&self, user_id: UserId) -> Result<()> {
        sqlx::query(
            r#"
            DELETE FROM users
            WHERE id = $1
            "#
        )
        .bind(user_id as i32)
        .execute(&self.pool)
        .await?;

        warn!("🗑️ User deleted: ID {}", user_id);
        Ok(())
    }

    /// Get user count for an organization
    pub async fn count_users_in_org(&self, org_id: OrgId) -> Result<i64> {
        let result = sqlx::query(
            r#"
            SELECT COUNT(*) as count
            FROM users
            WHERE org_id = $1
            "#
        )
        .bind(org_id as i32)
        .fetch_one(&self.pool)
        .await?;

        let count: Option<i64> = result.try_get("count")?;
        Ok(count.unwrap_or(0))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // Integration tests require running PostgreSQL instance
    // Run with: cargo test --features test-integration
}
