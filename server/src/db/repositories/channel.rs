use crate::error::Result;
use crate::types::{ChannelId, OrgId, Channel};
use sqlx::{PgPool, Row};
use tracing::{info, warn};

/// Repository for channel database operations
pub struct ChannelRepository {
    pool: PgPool,
}

impl ChannelRepository {
    pub fn new(pool: PgPool) -> Self {
        Self { pool }
    }

    /// Create a new channel
    pub async fn create_channel(
        &self,
        org_id: OrgId,
        parent_id: Option<ChannelId>,
        name: &str,
        description: Option<&str>,
        password: Option<&str>,
        max_users: Option<u32>,
        position: i32,
    ) -> Result<ChannelId> {
        let password_hash = if let Some(pwd) = password {
            Some(crate::auth::hash_password(pwd)?)
        } else {
            None
        };

        let result = sqlx::query(
            r#"
            INSERT INTO channels (org_id, parent_id, name, description, password_hash, max_users, position)
            VALUES ($1, $2, $3, $4, $5, $6, $7)
            RETURNING id
            "#
        )
        .bind(org_id as i32)
        .bind(parent_id.map(|id| id as i32))
        .bind(name)
        .bind(description)
        .bind(password_hash)
        .bind(max_users.map(|m| m as i32))
        .bind(position)
        .fetch_one(&self.pool)
        .await?;

        let channel_id: i32 = result.try_get("id")?;
        let channel_id = channel_id as ChannelId;
        info!("✅ Channel created: {} (ID: {}, Org: {})", name, channel_id, org_id);

        Ok(channel_id)
    }

    /// Get channel by ID
    pub async fn get_channel(&self, channel_id: ChannelId) -> Result<Option<Channel>> {
        let result = sqlx::query(
            r#"
            SELECT id, org_id, parent_id, name, description, password_hash, max_users, created_at
            FROM channels
            WHERE id = $1
            "#
        )
        .bind(channel_id as i32)
        .fetch_optional(&self.pool)
        .await?;

        Ok(result.map(|row| -> Result<Channel> {
            Ok(Channel {
                id: row.try_get::<i32, _>("id")? as ChannelId,
                org_id: row.try_get::<i32, _>("org_id")? as OrgId,
                parent_id: row.try_get::<Option<i32>, _>("parent_id")?.map(|id| id as ChannelId),
                name: row.try_get("name")?,
                description: row.try_get::<Option<String>, _>("description")?.unwrap_or_default(),
                password_hash: row.try_get("password_hash")?,
                max_users: row.try_get::<Option<i32>, _>("max_users")?.unwrap_or(50) as u32,
                created_at: row.try_get("created_at")?,
            })
        }).transpose()?)
    }

    /// Get all channels for an organization
    pub async fn get_channels_by_org(&self, org_id: OrgId) -> Result<Vec<Channel>> {
        let results = sqlx::query(
            r#"
            SELECT id, org_id, parent_id, name, description, password_hash, max_users, created_at
            FROM channels
            WHERE org_id = $1
            ORDER BY position, name
            "#
        )
        .bind(org_id as i32)
        .fetch_all(&self.pool)
        .await?;

        results
            .into_iter()
            .map(|row| -> Result<Channel> {
                Ok(Channel {
                    id: row.try_get::<i32, _>("id")? as ChannelId,
                    org_id: row.try_get::<i32, _>("org_id")? as OrgId,
                    parent_id: row.try_get::<Option<i32>, _>("parent_id")?.map(|id| id as ChannelId),
                    name: row.try_get("name")?,
                    description: row.try_get::<Option<String>, _>("description")?.unwrap_or_default(),
                    password_hash: row.try_get("password_hash")?,
                    max_users: row.try_get::<Option<i32>, _>("max_users")?.unwrap_or(50) as u32,
                    created_at: row.try_get("created_at")?,
                })
            })
            .collect()
    }

    /// Get child channels
    pub async fn get_child_channels(&self, parent_id: ChannelId) -> Result<Vec<Channel>> {
        let results = sqlx::query(
            r#"
            SELECT id, org_id, parent_id, name, description, password_hash, max_users, created_at
            FROM channels
            WHERE parent_id = $1
            ORDER BY position, name
            "#
        )
        .bind(parent_id as i32)
        .fetch_all(&self.pool)
        .await?;

        results
            .into_iter()
            .map(|row| -> Result<Channel> {
                Ok(Channel {
                    id: row.try_get::<i32, _>("id")? as ChannelId,
                    org_id: row.try_get::<i32, _>("org_id")? as OrgId,
                    parent_id: row.try_get::<Option<i32>, _>("parent_id")?.map(|id| id as ChannelId),
                    name: row.try_get("name")?,
                    description: row.try_get::<Option<String>, _>("description")?.unwrap_or_default(),
                    password_hash: row.try_get("password_hash")?,
                    max_users: row.try_get::<Option<i32>, _>("max_users")?.unwrap_or(50) as u32,
                    created_at: row.try_get("created_at")?,
                })
            })
            .collect()
    }

    /// Update channel name
    pub async fn update_channel_name(
        &self,
        channel_id: ChannelId,
        name: &str,
    ) -> Result<()> {
        sqlx::query(
            r#"
            UPDATE channels
            SET name = $1
            WHERE id = $2
            "#
        )
        .bind(name)
        .bind(channel_id as i32)
        .execute(&self.pool)
        .await?;

        Ok(())
    }

    /// Update channel description
    pub async fn update_channel_description(
        &self,
        channel_id: ChannelId,
        description: &str,
    ) -> Result<()> {
        sqlx::query(
            r#"
            UPDATE channels
            SET description = $1
            WHERE id = $2
            "#
        )
        .bind(description)
        .bind(channel_id as i32)
        .execute(&self.pool)
        .await?;

        Ok(())
    }

    /// Delete a channel
    pub async fn delete_channel(&self, channel_id: ChannelId) -> Result<()> {
        sqlx::query(
            r#"
            DELETE FROM channels
            WHERE id = $1
            "#
        )
        .bind(channel_id as i32)
        .execute(&self.pool)
        .await?;

        warn!("🗑️ Channel deleted: ID {}", channel_id);
        Ok(())
    }

    /// Count channels in organization
    pub async fn count_channels_in_org(&self, org_id: OrgId) -> Result<i64> {
        let result = sqlx::query(
            r#"
            SELECT COUNT(*) as count
            FROM channels
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
