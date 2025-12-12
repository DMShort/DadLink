//! Repository Integration Tests
//!
//! Tests the repository layer with actual Supabase database operations
//! Run with: cargo test --test repository_tests -- --test-threads=1

mod common;

use voip_server::db::{UserRepository, ChannelRepository, OrganizationRepository, RoleRepository};
use chrono::Utc;

#[tokio::test]
async fn test_user_repository_create_and_authenticate() {
    let pool = common::setup_test_pool().await;
    let repo = UserRepository::new(pool.clone());
    
    println!("👤 Testing UserRepository::create_user and authenticate...");
    
    let username = format!("repo_test_{}", Utc::now().timestamp());
    let password = "test_password_123";
    
    // Create user
    let user_id = repo.create_user(1, &username, Some("test@repo.com"), password)
        .await
        .expect("Failed to create user");
    
    println!("✅ Created user ID: {}", user_id);
    
    // Authenticate with correct password
    let user = repo.authenticate(&username, password)
        .await
        .expect("Failed to authenticate");
    
    assert!(user.is_some(), "User should authenticate with correct password");
    let user = user.unwrap();
    assert_eq!(user.username, username);
    assert_eq!(user.id, user_id);
    println!("✅ Authentication successful");
    
    // Authenticate with wrong password
    let user = repo.authenticate(&username, "wrong_password")
        .await
        .expect("Failed to authenticate");
    
    assert!(user.is_none(), "User should not authenticate with wrong password");
    println!("✅ Wrong password rejected");
    
    // Clean up
    repo.delete_user(user_id).await.expect("Failed to delete user");
    pool.close().await;
}

#[tokio::test]
async fn test_user_repository_get_operations() {
    let pool = common::setup_test_pool().await;
    let repo = UserRepository::new(pool.clone());
    
    println!("👤 Testing UserRepository::get operations...");
    
    let username = format!("get_test_{}", Utc::now().timestamp());
    let user_id = repo.create_user(1, &username, None, "password")
        .await
        .expect("Failed to create user");
    
    // Get by ID
    let user = repo.get_user_by_id(user_id)
        .await
        .expect("Failed to get user by ID");
    
    assert!(user.is_some());
    assert_eq!(user.unwrap().username, username);
    println!("✅ get_user_by_id works");
    
    // Get by username
    let user = repo.get_user_by_username(&username)
        .await
        .expect("Failed to get user by username");
    
    assert!(user.is_some());
    assert_eq!(user.unwrap().id, user_id);
    println!("✅ get_user_by_username works");
    
    // Get users by org
    let users = repo.get_users_by_org(1)
        .await
        .expect("Failed to get users by org");
    
    assert!(users.len() > 0);
    assert!(users.iter().any(|u| u.id == user_id));
    println!("✅ get_users_by_org works - found {} users", users.len());
    
    // Count users
    let count = repo.count_users_in_org(1)
        .await
        .expect("Failed to count users");
    
    assert!(count > 0);
    println!("✅ count_users_in_org: {}", count);
    
    // Clean up
    repo.delete_user(user_id).await.expect("Failed to delete user");
    pool.close().await;
}

#[tokio::test]
async fn test_user_repository_update_operations() {
    let pool = common::setup_test_pool().await;
    let repo = UserRepository::new(pool.clone());
    
    println!("👤 Testing UserRepository::update operations...");
    
    let username = format!("update_test_{}", Utc::now().timestamp());
    let user_id = repo.create_user(1, &username, Some("old@example.com"), "password")
        .await
        .expect("Failed to create user");
    
    // Update email
    repo.update_email(user_id, Some("new@example.com"))
        .await
        .expect("Failed to update email");
    
    let user = repo.get_user_by_id(user_id)
        .await
        .expect("Failed to get user")
        .unwrap();
    
    assert_eq!(user.email, Some("new@example.com".to_string()));
    println!("✅ update_email works");
    
    // Update password
    let new_password = "new_password_456";
    repo.update_password(user_id, new_password)
        .await
        .expect("Failed to update password");
    
    let auth_result = repo.authenticate(&username, new_password)
        .await
        .expect("Failed to authenticate");
    
    assert!(auth_result.is_some(), "Should authenticate with new password");
    println!("✅ update_password works");
    
    // Clean up
    repo.delete_user(user_id).await.expect("Failed to delete user");
    pool.close().await;
}

#[tokio::test]
async fn test_channel_repository_operations() {
    let pool = common::setup_test_pool().await;
    let repo = ChannelRepository::new(pool.clone());
    
    println!("📢 Testing ChannelRepository operations...");
    
    let channel_name = format!("Test Channel {}", Utc::now().timestamp());
    
    // Create channel
    let channel_id = repo.create_channel(
        1,
        None,
        &channel_name,
        Some("Test description"),
        None,
        Some(25),
        99
    )
    .await
    .expect("Failed to create channel");
    
    println!("✅ Created channel ID: {}", channel_id);
    
    // Get channel
    let channel = repo.get_channel(channel_id)
        .await
        .expect("Failed to get channel");
    
    assert!(channel.is_some());
    let channel = channel.unwrap();
    assert_eq!(channel.name, channel_name);
    assert_eq!(channel.description, "Test description");
    assert_eq!(channel.max_users, 25);
    println!("✅ get_channel works");
    
    // Get channels by org
    let channels = repo.get_channels_by_org(1)
        .await
        .expect("Failed to get channels by org");
    
    assert!(channels.iter().any(|c| c.id == channel_id));
    println!("✅ get_channels_by_org works - found {} channels", channels.len());
    
    // Update channel name
    repo.update_channel_name(channel_id, "Updated Name")
        .await
        .expect("Failed to update channel name");
    
    let channel = repo.get_channel(channel_id)
        .await
        .expect("Failed to get channel")
        .unwrap();
    
    assert_eq!(channel.name, "Updated Name");
    println!("✅ update_channel_name works");
    
    // Update channel description
    repo.update_channel_description(channel_id, "Updated description")
        .await
        .expect("Failed to update channel description");
    
    let channel = repo.get_channel(channel_id)
        .await
        .expect("Failed to get channel")
        .unwrap();
    
    assert_eq!(channel.description, "Updated description");
    println!("✅ update_channel_description works");
    
    // Count channels
    let count = repo.count_channels_in_org(1)
        .await
        .expect("Failed to count channels");
    
    assert!(count > 0);
    println!("✅ count_channels_in_org: {}", count);
    
    // Clean up
    repo.delete_channel(channel_id).await.expect("Failed to delete channel");
    pool.close().await;
}

#[tokio::test]
async fn test_channel_repository_parent_child() {
    let pool = common::setup_test_pool().await;
    let repo = ChannelRepository::new(pool.clone());
    
    println!("📢 Testing parent-child channel relationships...");
    
    // Create parent channel
    let parent_name = format!("Parent {}", Utc::now().timestamp());
    let parent_id = repo.create_channel(1, None, &parent_name, None, None, None, 0)
        .await
        .expect("Failed to create parent channel");
    
    println!("✅ Created parent channel ID: {}", parent_id);
    
    // Create child channel
    let child_name = format!("Child {}", Utc::now().timestamp());
    let child_id = repo.create_channel(1, Some(parent_id), &child_name, None, None, None, 0)
        .await
        .expect("Failed to create child channel");
    
    println!("✅ Created child channel ID: {}", child_id);
    
    // Get child channels
    let children = repo.get_child_channels(parent_id)
        .await
        .expect("Failed to get child channels");
    
    assert!(children.iter().any(|c| c.id == child_id));
    println!("✅ get_child_channels works - found {} children", children.len());
    
    // Clean up (parent deletion should cascade to children)
    repo.delete_channel(parent_id).await.expect("Failed to delete parent channel");
    
    // Verify child was also deleted
    let child = repo.get_channel(child_id).await.expect("Failed to get child channel");
    assert!(child.is_none(), "Child channel should be deleted when parent is deleted");
    println!("✅ Cascade delete works");
    
    pool.close().await;
}

#[tokio::test]
async fn test_organization_repository_operations() {
    let pool = common::setup_test_pool().await;
    let repo = OrganizationRepository::new(pool.clone());
    
    println!("🏢 Testing OrganizationRepository operations...");
    
    let tag = format!("TEST{}", Utc::now().timestamp());
    
    // Create organization
    let org_id = repo.create_organization(&format!("Test Org {}", tag), &tag, None, Some(100), Some(50))
        .await
        .expect("Failed to create organization");
    
    println!("✅ Created organization ID: {}", org_id);
    
    // Get organization
    let org = repo.get_organization(org_id)
        .await
        .expect("Failed to get organization");
    
    assert!(org.is_some());
    let org = org.unwrap();
    assert_eq!(org.tag, tag);
    println!("✅ get_organization works");
    
    // Get by tag
    let org = repo.get_organization_by_tag(&tag)
        .await
        .expect("Failed to get organization by tag");
    
    assert!(org.is_some());
    assert_eq!(org.unwrap().id, org_id);
    println!("✅ get_organization_by_tag works");
    
    // Clean up
    repo.delete_organization(org_id).await.expect("Failed to delete organization");
    pool.close().await;
}

#[tokio::test]
async fn test_role_repository_operations() {
    let pool = common::setup_test_pool().await;
    let repo = RoleRepository::new(pool.clone());
    
    println!("🔐 Testing RoleRepository operations...");
    
    // Get roles for org
    let roles = repo.get_roles_by_org(1)
        .await
        .expect("Failed to get roles");
    
    assert!(roles.len() >= 4, "Should have at least 4 default roles");
    println!("✅ get_roles_by_org works - found {} roles", roles.len());
    
    // Get role by name
    let admin_role = repo.get_role_by_name(1, "Admin")
        .await
        .expect("Failed to get role by name");
    
    assert!(admin_role.is_some());
    let admin_role = admin_role.unwrap();
    assert_eq!(admin_role.permissions, 255);
    println!("✅ get_role_by_name works - Admin has permissions: {}", admin_role.permissions);
    
    // Get user roles
    let user_roles = repo.get_user_roles(1)
        .await
        .expect("Failed to get user roles");
    
    assert!(user_roles.len() > 0);
    println!("✅ get_user_roles works - user has {} roles", user_roles.len());
    
    pool.close().await;
}
