use crate::error::{Result, VoipError};
use crate::types::{Permissions, UserId, ChannelId, OrgId};
use crate::db::{UserRepository, ChannelRepository, RoleRepository};
use std::sync::Arc;

/// Permission checking service
pub struct PermissionChecker {
    user_repo: Arc<UserRepository>,
    channel_repo: Arc<ChannelRepository>,
    role_repo: Arc<RoleRepository>,
}

impl PermissionChecker {
    /// Create a new permission checker
    pub fn new(
        user_repo: Arc<UserRepository>,
        channel_repo: Arc<ChannelRepository>,
        role_repo: Arc<RoleRepository>,
    ) -> Self {
        Self {
            user_repo,
            channel_repo,
            role_repo,
        }
    }

    /// Check if a user has permission to join a channel
    pub async fn can_join_channel(
        &self,
        user_id: UserId,
        org_id: OrgId,
        channel_id: ChannelId,
    ) -> Result<bool> {
        self.check_channel_permission(user_id, org_id, channel_id, Permissions::JOIN).await
    }

    /// Check if a user has permission to speak in a channel
    pub async fn can_speak_in_channel(
        &self,
        user_id: UserId,
        org_id: OrgId,
        channel_id: ChannelId,
    ) -> Result<bool> {
        self.check_channel_permission(user_id, org_id, channel_id, Permissions::SPEAK).await
    }

    /// Check if a user has permission to whisper
    pub async fn can_whisper(
        &self,
        user_id: UserId,
        org_id: OrgId,
        channel_id: ChannelId,
    ) -> Result<bool> {
        self.check_channel_permission(user_id, org_id, channel_id, Permissions::WHISPER).await
    }

    /// Check if a user has permission to manage a channel
    pub async fn can_manage_channel(
        &self,
        user_id: UserId,
        org_id: OrgId,
        channel_id: ChannelId,
    ) -> Result<bool> {
        self.check_channel_permission(user_id, org_id, channel_id, Permissions::MANAGE).await
    }

    /// Check if a user has permission to kick other users
    pub async fn can_kick_users(
        &self,
        user_id: UserId,
        org_id: OrgId,
        channel_id: ChannelId,
    ) -> Result<bool> {
        self.check_channel_permission(user_id, org_id, channel_id, Permissions::KICK).await
    }

    /// Verify channel password if one is set
    pub async fn verify_channel_password(
        &self,
        channel_id: ChannelId,
        password: Option<&str>,
    ) -> Result<bool> {
        // Get channel from repository
        let channel = self.channel_repo.get_channel(channel_id).await?
            .ok_or_else(|| VoipError::Other(format!("Channel {} not found", channel_id)))?;

        // If channel has no password, allow access
        if channel.password_hash.is_none() {
            return Ok(true);
        }

        // If channel has password but none provided, deny access
        let provided_password = match password {
            Some(p) => p,
            None => return Ok(false),
        };

        // Verify password using argon2
        match &channel.password_hash {
            Some(hash) => crate::auth::verify_password(provided_password, hash),
            None => Ok(true),
        }
    }

    /// Core permission checking logic
    async fn check_channel_permission(
        &self,
        user_id: UserId,
        org_id: OrgId,
        channel_id: ChannelId,
        required_permission: Permissions,
    ) -> Result<bool> {
        // Get user from database
        let user = self.user_repo.get_user_by_id(user_id).await?
            .ok_or_else(|| VoipError::Other(format!("User {} not found", user_id)))?;

        // Verify user belongs to the organization
        if user.org_id != org_id {
            tracing::warn!("User {} does not belong to organization {}", user_id, org_id);
            return Ok(false);
        }

        // Get effective permissions for user in this channel
        let effective_permissions = self.role_repo
            .get_user_channel_permissions(user_id, channel_id)
            .await?;

        // Check if user has the required permission
        let has_permission = effective_permissions.contains(required_permission);

        if !has_permission {
            tracing::debug!(
                "User {} lacks {:?} permission in channel {} (has: {:?})",
                user_id,
                required_permission,
                channel_id,
                effective_permissions
            );
        }

        Ok(has_permission)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // Note: These tests would require a test database
    // For now, we'll add integration tests later

    #[test]
    fn test_permission_flags() {
        let all_perms = Permissions::JOIN | Permissions::SPEAK | Permissions::WHISPER;
        assert!(all_perms.contains(Permissions::JOIN));
        assert!(all_perms.contains(Permissions::SPEAK));
        assert!(all_perms.contains(Permissions::WHISPER));
        assert!(!all_perms.contains(Permissions::MANAGE));
    }

    #[test]
    fn test_permission_combinations() {
        let member_perms = Permissions::JOIN | Permissions::SPEAK;
        let admin_perms = Permissions::JOIN | Permissions::SPEAK | Permissions::MANAGE | Permissions::KICK;

        assert!(member_perms.contains(Permissions::JOIN));
        assert!(!member_perms.contains(Permissions::MANAGE));

        assert!(admin_perms.contains(Permissions::MANAGE));
        assert!(admin_perms.contains(Permissions::KICK));
    }
}
