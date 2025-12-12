use crate::error::{Result, VoipError};
use crate::types::{OrgId, UserId, Organization};
use sqlx::{PgPool, Row};
use tracing::{info, warn};

/// Repository for organization database operations
pub struct OrganizationRepository {
    pool: PgPool,
}

impl OrganizationRepository {
    pub fn new(pool: PgPool) -> Self {
        Self { pool }
    }

    /// Create a new organization
    pub async fn create_organization(
        &self,
        name: &str,
        tag: &str,
        owner_id: Option<UserId>,
        max_users: Option<u32>,
        max_channels: Option<u32>,
    ) -> Result<OrgId> {
        let result = sqlx::query(
            r#"
            INSERT INTO organizations (name, tag, owner_id, max_users, max_channels)
            VALUES ($1, $2, $3, $4, $5)
            RETURNING id
            "#
        )
        .bind(name)
        .bind(tag)
        .bind(owner_id.map(|id| id as i32))
        .bind(max_users.map(|m| m as i32))
        .bind(max_channels.map(|m| m as i32))
        .fetch_one(&self.pool)
        .await
        .map_err(|e| {
            if e.to_string().contains("unique") {
                VoipError::Other(format!("Organization tag '{}' already exists", tag))
            } else {
                VoipError::DatabaseError(e)
            }
        })?;

        let org_id: i32 = result.try_get("id")?;
        let org_id = org_id as OrgId;
        info!("✅ Organization created: {} [{}] (ID: {})", name, tag, org_id);

        Ok(org_id)
    }

    /// Get organization by ID
    pub async fn get_organization(&self, org_id: OrgId) -> Result<Option<Organization>> {
        let result = sqlx::query(
            r#"
            SELECT id, name, tag, owner_id, created_at, max_users, max_channels
            FROM organizations
            WHERE id = $1
            "#
        )
        .bind(org_id as i32)
        .fetch_optional(&self.pool)
        .await?;

        Ok(result.map(|row| -> Result<Organization> {
            Ok(Organization {
                id: row.try_get::<i32, _>("id")? as OrgId,
                name: row.try_get("name")?,
                tag: row.try_get("tag")?,
                owner_id: row.try_get::<Option<i32>, _>("owner_id")?.map(|id| id as UserId),
                created_at: row.try_get("created_at")?,
                max_users: row.try_get::<Option<i32>, _>("max_users")?.map(|m| m as u32).unwrap_or(1000),
                max_channels: row.try_get::<Option<i32>, _>("max_channels")?.map(|m| m as u32).unwrap_or(100),
            })
        }).transpose()?)
    }

    /// Get organization by tag
    pub async fn get_organization_by_tag(&self, tag: &str) -> Result<Option<Organization>> {
        let result = sqlx::query(
            r#"
            SELECT id, name, tag, owner_id, created_at, max_users, max_channels
            FROM organizations
            WHERE tag = $1
            "#
        )
        .bind(tag)
        .fetch_optional(&self.pool)
        .await?;

        Ok(result.map(|row| -> Result<Organization> {
            Ok(Organization {
                id: row.try_get::<i32, _>("id")? as OrgId,
                name: row.try_get("name")?,
                tag: row.try_get("tag")?,
                owner_id: row.try_get::<Option<i32>, _>("owner_id")?.map(|id| id as UserId),
                created_at: row.try_get("created_at")?,
                max_users: row.try_get::<Option<i32>, _>("max_users")?.map(|m| m as u32).unwrap_or(1000),
                max_channels: row.try_get::<Option<i32>, _>("max_channels")?.map(|m| m as u32).unwrap_or(100),
            })
        }).transpose()?)
    }

    /// Get all organizations
    pub async fn get_all_organizations(&self) -> Result<Vec<Organization>> {
        let results = sqlx::query(
            r#"
            SELECT id, name, tag, owner_id, created_at, max_users, max_channels
            FROM organizations
            ORDER BY name
            "#
        )
        .fetch_all(&self.pool)
        .await?;

        results
            .into_iter()
            .map(|row| -> Result<Organization> {
                Ok(Organization {
                    id: row.try_get::<i32, _>("id")? as OrgId,
                    name: row.try_get("name")?,
                    tag: row.try_get("tag")?,
                    owner_id: row.try_get::<Option<i32>, _>("owner_id")?.map(|id| id as UserId),
                    created_at: row.try_get("created_at")?,
                    max_users: row.try_get::<Option<i32>, _>("max_users")?.map(|m| m as u32).unwrap_or(1000),
                    max_channels: row.try_get::<Option<i32>, _>("max_channels")?.map(|m| m as u32).unwrap_or(100),
                })
            })
            .collect()
    }

    /// Update organization name
    pub async fn update_organization_name(
        &self,
        org_id: OrgId,
        name: &str,
    ) -> Result<()> {
        sqlx::query(
            r#"
            UPDATE organizations
            SET name = $1
            WHERE id = $2
            "#
        )
        .bind(name)
        .bind(org_id as i32)
        .execute(&self.pool)
        .await?;

        Ok(())
    }

    /// Update organization owner
    pub async fn update_organization_owner(
        &self,
        org_id: OrgId,
        owner_id: UserId,
    ) -> Result<()> {
        sqlx::query(
            r#"
            UPDATE organizations
            SET owner_id = $1
            WHERE id = $2
            "#
        )
        .bind(owner_id as i32)
        .bind(org_id as i32)
        .execute(&self.pool)
        .await?;

        Ok(())
    }

    /// Update organization limits
    pub async fn update_organization_limits(
        &self,
        org_id: OrgId,
        max_users: Option<u32>,
        max_channels: Option<u32>,
    ) -> Result<()> {
        sqlx::query(
            r#"
            UPDATE organizations
            SET max_users = $1, max_channels = $2
            WHERE id = $3
            "#
        )
        .bind(max_users.map(|m| m as i32))
        .bind(max_channels.map(|m| m as i32))
        .bind(org_id as i32)
        .execute(&self.pool)
        .await?;

        Ok(())
    }

    /// Delete an organization
    pub async fn delete_organization(&self, org_id: OrgId) -> Result<()> {
        sqlx::query(
            r#"
            DELETE FROM organizations
            WHERE id = $1
            "#
        )
        .bind(org_id as i32)
        .execute(&self.pool)
        .await?;

        warn!("🗑️ Organization deleted: ID {}", org_id);
        Ok(())
    }
}
