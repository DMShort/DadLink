use crate::error::Result;
use crate::types::{Role, RoleId, UserId, OrgId, ChannelId, Permissions, ACLEntry};
use sqlx::{PgPool, Row};
use tracing::{info, warn};

/// Role repository for database operations
pub struct RoleRepository {
    pool: PgPool,
}

impl RoleRepository {
    /// Create a new role repository
    pub fn new(pool: PgPool) -> Self {
        Self { pool }
    }

    /// Create a new role
    pub async fn create_role(
        &self,
        org_id: OrgId,
        name: &str,
        permissions: Permissions,
        priority: u32,
    ) -> Result<RoleId> {
        let row = sqlx::query(
            r#"
            INSERT INTO roles (org_id, name, permissions, priority)
            VALUES ($1, $2, $3, $4)
            RETURNING id
            "#
        )
        .bind(org_id as i32)
        .bind(name)
        .bind(permissions.bits() as i32)
        .bind(priority as i32)
        .fetch_one(&self.pool)
        .await?;

        let role_id: i32 = row.try_get("id")?;
        info!("Created role '{}' (ID: {}) for org {}", name, role_id, org_id);
        Ok(role_id as RoleId)
    }

    /// Get role by ID
    pub async fn get_role(&self, role_id: RoleId) -> Result<Option<Role>> {
        let row = sqlx::query(
            r#"
            SELECT id, org_id, name, permissions, priority
            FROM roles
            WHERE id = $1
            "#
        )
        .bind(role_id as i32)
        .fetch_optional(&self.pool)
        .await?;

        Ok(row.map(|r| -> Result<Role> {
            Ok(Role {
                id: r.try_get::<i32, _>("id")? as RoleId,
                org_id: r.try_get::<i32, _>("org_id")? as OrgId,
                name: r.try_get("name")?,
                permissions: Permissions::from_bits_truncate(r.try_get::<i32, _>("permissions")? as u32),
                priority: r.try_get::<i32, _>("priority")? as u32,
            })
        }).transpose()?)
    }

    /// Get all roles for an organization
    pub async fn get_roles_by_org(&self, org_id: OrgId) -> Result<Vec<Role>> {
        let rows = sqlx::query(
            r#"
            SELECT id, org_id, name, permissions, priority
            FROM roles
            WHERE org_id = $1
            ORDER BY priority DESC, name ASC
            "#
        )
        .bind(org_id as i32)
        .fetch_all(&self.pool)
        .await?;

        rows.into_iter().map(|r| -> Result<Role> {
            Ok(Role {
                id: r.try_get::<i32, _>("id")? as RoleId,
                org_id: r.try_get::<i32, _>("org_id")? as OrgId,
                name: r.try_get("name")?,
                permissions: Permissions::from_bits_truncate(r.try_get::<i32, _>("permissions")? as u32),
                priority: r.try_get::<i32, _>("priority")? as u32,
            })
        }).collect()
    }

    /// Get role by name in an organization
    pub async fn get_role_by_name(&self, org_id: OrgId, name: &str) -> Result<Option<Role>> {
        let row = sqlx::query(
            r#"
            SELECT id, org_id, name, permissions, priority
            FROM roles
            WHERE org_id = $1 AND name = $2
            "#
        )
        .bind(org_id as i32)
        .bind(name)
        .fetch_optional(&self.pool)
        .await?;

        Ok(row.map(|r| -> Result<Role> {
            Ok(Role {
                id: r.try_get::<i32, _>("id")? as RoleId,
                org_id: r.try_get::<i32, _>("org_id")? as OrgId,
                name: r.try_get("name")?,
                permissions: Permissions::from_bits_truncate(r.try_get::<i32, _>("permissions")? as u32),
                priority: r.try_get::<i32, _>("priority")? as u32,
            })
        }).transpose()?)
    }

    /// Get roles assigned to a user
    pub async fn get_user_roles(&self, user_id: UserId) -> Result<Vec<Role>> {
        let rows = sqlx::query(
            r#"
            SELECT r.id, r.org_id, r.name, r.permissions, r.priority
            FROM roles r
            INNER JOIN user_roles ur ON r.id = ur.role_id
            WHERE ur.user_id = $1
            ORDER BY r.priority DESC, r.name ASC
            "#
        )
        .bind(user_id as i32)
        .fetch_all(&self.pool)
        .await?;

        rows.into_iter().map(|r| -> Result<Role> {
            Ok(Role {
                id: r.try_get::<i32, _>("id")? as RoleId,
                org_id: r.try_get::<i32, _>("org_id")? as OrgId,
                name: r.try_get("name")?,
                permissions: Permissions::from_bits_truncate(r.try_get::<i32, _>("permissions")? as u32),
                priority: r.try_get::<i32, _>("priority")? as u32,
            })
        }).collect()
    }

    /// Assign role to user
    pub async fn assign_role_to_user(&self, user_id: UserId, role_id: RoleId) -> Result<()> {
        sqlx::query(
            r#"
            INSERT INTO user_roles (user_id, role_id)
            VALUES ($1, $2)
            ON CONFLICT (user_id, role_id) DO NOTHING
            "#
        )
        .bind(user_id as i32)
        .bind(role_id as i32)
        .execute(&self.pool)
        .await?;

        info!("Assigned role {} to user {}", role_id, user_id);
        Ok(())
    }

    /// Remove role from user
    pub async fn remove_role_from_user(&self, user_id: UserId, role_id: RoleId) -> Result<()> {
        sqlx::query(
            r#"
            DELETE FROM user_roles
            WHERE user_id = $1 AND role_id = $2
            "#
        )
        .bind(user_id as i32)
        .bind(role_id as i32)
        .execute(&self.pool)
        .await?;

        info!("Removed role {} from user {}", role_id, user_id);
        Ok(())
    }

    /// Update role permissions
    pub async fn update_role_permissions(&self, role_id: RoleId, permissions: Permissions) -> Result<()> {
        sqlx::query(
            r#"
            UPDATE roles
            SET permissions = $1
            WHERE id = $2
            "#
        )
        .bind(permissions.bits() as i32)
        .bind(role_id as i32)
        .execute(&self.pool)
        .await?;

        info!("Updated permissions for role {}", role_id);
        Ok(())
    }

    /// Delete role
    pub async fn delete_role(&self, role_id: RoleId) -> Result<()> {
        sqlx::query(
            r#"
            DELETE FROM roles
            WHERE id = $1
            "#
        )
        .bind(role_id as i32)
        .execute(&self.pool)
        .await?;

        info!("Deleted role {}", role_id);
        Ok(())
    }

    /// Get channel ACL entries
    pub async fn get_channel_acl(&self, channel_id: ChannelId) -> Result<Vec<ACLEntry>> {
        let rows = sqlx::query(
            r#"
            SELECT channel_id, role_id, permissions
            FROM channel_acl
            WHERE channel_id = $1
            "#
        )
        .bind(channel_id as i32)
        .fetch_all(&self.pool)
        .await?;

        rows.into_iter().map(|r| -> Result<ACLEntry> {
            Ok(ACLEntry {
                channel_id: r.try_get::<i32, _>("channel_id")? as ChannelId,
                role_id: r.try_get::<i32, _>("role_id")? as RoleId,
                permissions: Permissions::from_bits_truncate(r.try_get::<i32, _>("permissions")? as u32),
            })
        }).collect()
    }

    /// Set channel ACL for a role
    pub async fn set_channel_acl(
        &self,
        channel_id: ChannelId,
        role_id: RoleId,
        permissions: Permissions,
    ) -> Result<()> {
        sqlx::query(
            r#"
            INSERT INTO channel_acl (channel_id, role_id, permissions)
            VALUES ($1, $2, $3)
            ON CONFLICT (channel_id, role_id)
            DO UPDATE SET permissions = $3
            "#
        )
        .bind(channel_id as i32)
        .bind(role_id as i32)
        .bind(permissions.bits() as i32)
        .execute(&self.pool)
        .await?;

        info!("Set ACL for channel {} role {} to {:?}", channel_id, role_id, permissions);
        Ok(())
    }

    /// Remove channel ACL entry
    pub async fn remove_channel_acl(&self, channel_id: ChannelId, role_id: RoleId) -> Result<()> {
        sqlx::query(
            r#"
            DELETE FROM channel_acl
            WHERE channel_id = $1 AND role_id = $2
            "#
        )
        .bind(channel_id as i32)
        .bind(role_id as i32)
        .execute(&self.pool)
        .await?;

        info!("Removed ACL for channel {} role {}", channel_id, role_id);
        Ok(())
    }

    /// Get effective permissions for a user in a channel
    /// This combines role permissions and channel-specific ACL
    pub async fn get_user_channel_permissions(
        &self,
        user_id: UserId,
        channel_id: ChannelId,
    ) -> Result<Permissions> {
        // Get user's roles
        let user_roles = self.get_user_roles(user_id).await?;

        if user_roles.is_empty() {
            warn!("User {} has no roles assigned", user_id);
            return Ok(Permissions::empty());
        }

        // Get channel ACL
        let channel_acl = self.get_channel_acl(channel_id).await?;

        // Combine permissions
        let mut effective_permissions = Permissions::empty();

        for role in user_roles {
            // Check if there's a channel-specific ACL for this role
            if let Some(acl) = channel_acl.iter().find(|a| a.role_id == role.id) {
                // Use channel-specific permissions
                effective_permissions |= acl.permissions;
            } else {
                // Use organization-level role permissions
                effective_permissions |= role.permissions;
            }
        }

        Ok(effective_permissions)
    }
}
