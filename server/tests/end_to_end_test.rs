//! End-to-End Database Tests
//!
//! These tests verify complete workflows from organization creation
//! through user management, channels, and permissions.
//! 
//! Run with: cargo test --test end_to_end_test -- --test-threads=1

mod common;

use voip_server::db::{UserRepository, ChannelRepository, OrganizationRepository, RoleRepository};
use voip_server::types::Permissions;
use chrono::Utc;

#[tokio::test]
async fn test_complete_organization_setup() {
    let pool = common::setup_test_pool().await;
    
    println!("\n🎯 Running complete organization setup workflow...\n");
    
    // Initialize repositories
    let org_repo = OrganizationRepository::new(pool.clone());
    let user_repo = UserRepository::new(pool.clone());
    let channel_repo = ChannelRepository::new(pool.clone());
    let role_repo = RoleRepository::new(pool.clone());
    
    let tag = format!("E2E{}", Utc::now().timestamp());
    
    // Step 1: Create organization
    println!("📋 Step 1: Creating organization...");
    let org_id = org_repo.create_organization(
        &format!("E2E Test Org {}", tag),
        &tag,
        None,
        Some(100),
        Some(50)
    )
    .await
    .expect("Failed to create organization");
    println!("   ✅ Organization created: ID {}", org_id);
    
    // Step 2: Create roles
    println!("\n📋 Step 2: Creating roles...");
    let admin_role = role_repo.create_role(
        org_id,
        "Admin",
        Permissions::all(),
        100
    )
    .await
    .expect("Failed to create admin role");
    println!("   ✅ Admin role created: ID {}", admin_role);
    
    let member_role = role_repo.create_role(
        org_id,
        "Member",
        Permissions::JOIN | Permissions::SPEAK,
        10
    )
    .await
    .expect("Failed to create member role");
    println!("   ✅ Member role created: ID {}", member_role);
    
    // Step 3: Create users
    println!("\n📋 Step 3: Creating users...");
    let admin_user = user_repo.create_user(
        org_id,
        &format!("admin_{}", tag),
        Some("admin@e2e.test"),
        "admin_password"
    )
    .await
    .expect("Failed to create admin user");
    println!("   ✅ Admin user created: ID {}", admin_user);
    
    let member_user = user_repo.create_user(
        org_id,
        &format!("member_{}", tag),
        Some("member@e2e.test"),
        "member_password"
    )
    .await
    .expect("Failed to create member user");
    println!("   ✅ Member user created: ID {}", member_user);
    
    // Step 4: Assign roles
    println!("\n📋 Step 4: Assigning roles...");
    role_repo.assign_role_to_user(admin_user, admin_role)
        .await
        .expect("Failed to assign admin role");
    println!("   ✅ Admin role assigned to admin user");
    
    role_repo.assign_role_to_user(member_user, member_role)
        .await
        .expect("Failed to assign member role");
    println!("   ✅ Member role assigned to member user");
    
    // Step 5: Create channels
    println!("\n📋 Step 5: Creating channels...");
    let general_channel = channel_repo.create_channel(
        org_id,
        None,
        "General",
        Some("General discussion"),
        None,
        Some(50),
        0
    )
    .await
    .expect("Failed to create general channel");
    println!("   ✅ General channel created: ID {}", general_channel);
    
    let admin_channel = channel_repo.create_channel(
        org_id,
        None,
        "Admin Only",
        Some("Admin-only channel"),
        None,
        Some(10),
        1
    )
    .await
    .expect("Failed to create admin channel");
    println!("   ✅ Admin channel created: ID {}", admin_channel);
    
    // Step 6: Set channel permissions
    println!("\n📋 Step 6: Setting channel permissions...");
    
    // General channel: All roles can join and speak
    role_repo.set_channel_acl(
        general_channel,
        admin_role,
        Permissions::all()
    )
    .await
    .expect("Failed to set admin ACL for general channel");
    
    role_repo.set_channel_acl(
        general_channel,
        member_role,
        Permissions::JOIN | Permissions::SPEAK
    )
    .await
    .expect("Failed to set member ACL for general channel");
    println!("   ✅ General channel permissions set");
    
    // Admin channel: Only admin role can access
    role_repo.set_channel_acl(
        admin_channel,
        admin_role,
        Permissions::all()
    )
    .await
    .expect("Failed to set admin ACL for admin channel");
    println!("   ✅ Admin channel permissions set");
    
    // Step 7: Verify permissions
    println!("\n📋 Step 7: Verifying permissions...");
    
    let admin_perms_general = role_repo.get_user_channel_permissions(admin_user, general_channel)
        .await
        .expect("Failed to get admin permissions for general channel");
    println!("   ✅ Admin permissions in General: {:?}", admin_perms_general);
    assert!(admin_perms_general.contains(Permissions::JOIN));
    assert!(admin_perms_general.contains(Permissions::SPEAK));
    
    let member_perms_general = role_repo.get_user_channel_permissions(member_user, general_channel)
        .await
        .expect("Failed to get member permissions for general channel");
    println!("   ✅ Member permissions in General: {:?}", member_perms_general);
    assert!(member_perms_general.contains(Permissions::JOIN));
    assert!(member_perms_general.contains(Permissions::SPEAK));
    
    let admin_perms_admin = role_repo.get_user_channel_permissions(admin_user, admin_channel)
        .await
        .expect("Failed to get admin permissions for admin channel");
    println!("   ✅ Admin permissions in Admin channel: {:?}", admin_perms_admin);
    assert!(admin_perms_admin.contains(Permissions::JOIN));
    
    let member_perms_admin = role_repo.get_user_channel_permissions(member_user, admin_channel)
        .await
        .expect("Failed to get member permissions for admin channel");
    println!("   ✅ Member permissions in Admin channel: {:?}", member_perms_admin);
    assert!(member_perms_admin.is_empty(), "Member should have no permissions in admin channel");
    
    // Step 8: Test authentication
    println!("\n📋 Step 8: Testing authentication...");
    
    let auth_admin = user_repo.authenticate(&format!("admin_{}", tag), "admin_password")
        .await
        .expect("Failed to authenticate admin");
    assert!(auth_admin.is_some());
    println!("   ✅ Admin authentication successful");
    
    let auth_member = user_repo.authenticate(&format!("member_{}", tag), "member_password")
        .await
        .expect("Failed to authenticate member");
    assert!(auth_member.is_some());
    println!("   ✅ Member authentication successful");
    
    // Step 9: Verify data integrity
    println!("\n📋 Step 9: Verifying data integrity...");
    
    let org = org_repo.get_organization(org_id)
        .await
        .expect("Failed to get organization")
        .expect("Organization not found");
    assert_eq!(org.tag, tag);
    println!("   ✅ Organization data verified");
    
    let users = user_repo.get_users_by_org(org_id)
        .await
        .expect("Failed to get users");
    assert_eq!(users.len(), 2);
    println!("   ✅ User count verified: {}", users.len());
    
    let channels = channel_repo.get_channels_by_org(org_id)
        .await
        .expect("Failed to get channels");
    assert_eq!(channels.len(), 2);
    println!("   ✅ Channel count verified: {}", channels.len());
    
    let roles = role_repo.get_roles_by_org(org_id)
        .await
        .expect("Failed to get roles");
    assert_eq!(roles.len(), 2);
    println!("   ✅ Role count verified: {}", roles.len());
    
    // Step 10: Cleanup (cascade delete)
    println!("\n📋 Step 10: Cleanup...");
    org_repo.delete_organization(org_id)
        .await
        .expect("Failed to delete organization");
    println!("   ✅ Organization deleted (cascading to all related data)");
    
    // Verify cascade delete
    let org_check = org_repo.get_organization(org_id).await.expect("Failed to check org");
    assert!(org_check.is_none(), "Organization should be deleted");
    
    let users_check = user_repo.get_users_by_org(org_id).await.expect("Failed to check users");
    assert_eq!(users_check.len(), 0, "Users should be cascade deleted");
    
    let channels_check = channel_repo.get_channels_by_org(org_id).await.expect("Failed to check channels");
    assert_eq!(channels_check.len(), 0, "Channels should be cascade deleted");
    
    let roles_check = role_repo.get_roles_by_org(org_id).await.expect("Failed to check roles");
    assert_eq!(roles_check.len(), 0, "Roles should be cascade deleted");
    
    println!("   ✅ Cascade delete verified\n");
    
    println!("🎉 Complete organization setup workflow passed!\n");
    
    pool.close().await;
}

#[tokio::test]
async fn test_multi_user_channel_workflow() {
    let pool = common::setup_test_pool().await;
    
    println!("\n🎯 Running multi-user channel workflow...\n");
    
    let user_repo = UserRepository::new(pool.clone());
    let channel_repo = ChannelRepository::new(pool.clone());
    let role_repo = RoleRepository::new(pool.clone());
    
    // Use demo organization
    let org_id = 1;
    
    println!("📋 Creating test users...");
    let mut user_ids = Vec::new();
    for i in 1..=5 {
        let username = format!("multi_user_test_{}_{}", i, Utc::now().timestamp());
        let user_id = user_repo.create_user(
            org_id,
            &username,
            Some(&format!("user{}@multi.test", i)),
            "password"
        )
        .await
        .expect("Failed to create user");
        user_ids.push(user_id);
        println!("   ✅ Created user {}: ID {}", i, user_id);
    }
    
    println!("\n📋 Assigning member role to all users...");
    let member_role = role_repo.get_role_by_name(org_id, "Member")
        .await
        .expect("Failed to get member role")
        .expect("Member role not found");
    
    for user_id in &user_ids {
        role_repo.assign_role_to_user(*user_id, member_role.id)
            .await
            .expect("Failed to assign role");
    }
    println!("   ✅ All users assigned member role");
    
    println!("\n📋 Creating test channel...");
    let channel_name = format!("Multi User Test {}", Utc::now().timestamp());
    let channel_id = channel_repo.create_channel(
        org_id,
        None,
        &channel_name,
        Some("Test channel for multiple users"),
        None,
        Some(10),
        99
    )
    .await
    .expect("Failed to create channel");
    println!("   ✅ Channel created: ID {}", channel_id);
    
    println!("\n📋 Setting channel permissions...");
    role_repo.set_channel_acl(
        channel_id,
        member_role.id,
        Permissions::JOIN | Permissions::SPEAK
    )
    .await
    .expect("Failed to set channel ACL");
    println!("   ✅ Channel permissions set");
    
    println!("\n📋 Verifying all users can access channel...");
    for (i, user_id) in user_ids.iter().enumerate() {
        let perms = role_repo.get_user_channel_permissions(*user_id, channel_id)
            .await
            .expect("Failed to get permissions");
        assert!(perms.contains(Permissions::JOIN), "User {} should have JOIN permission", i + 1);
        assert!(perms.contains(Permissions::SPEAK), "User {} should have SPEAK permission", i + 1);
        println!("   ✅ User {} can join and speak", i + 1);
    }
    
    println!("\n📋 Cleanup...");
    channel_repo.delete_channel(channel_id).await.expect("Failed to delete channel");
    for user_id in user_ids {
        user_repo.delete_user(user_id).await.expect("Failed to delete user");
    }
    println!("   ✅ Cleanup complete\n");
    
    println!("🎉 Multi-user channel workflow passed!\n");
    
    pool.close().await;
}
