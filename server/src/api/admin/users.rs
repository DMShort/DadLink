//! Admin API endpoints for user management

use axum::{
    extract::{Path, State},
    http::StatusCode,
    Json,
};
use serde::{Deserialize, Serialize};
use std::sync::Arc;
use crate::error::Result;
use crate::network::tls::ServerState;

#[derive(Debug, Deserialize)]
pub struct CreateUserRequest {
    pub username: String,
    pub password: String,
    pub email: Option<String>,
    pub org_id: i32,
}

#[derive(Debug, Deserialize)]
pub struct UpdateUserRequest {
    pub username: Option<String>,
    pub email: Option<String>,
}

#[derive(Debug, Deserialize)]
pub struct ResetPasswordRequest {
    pub password: String,
}

#[derive(Debug, Serialize)]
pub struct UserResponse {
    pub id: i32,
    pub username: String,
    pub email: Option<String>,
    pub org_id: i32,
    pub is_active: bool,
    pub is_banned: bool,
    pub last_login: Option<String>,
}

/// GET /api/admin/users - List all users
pub async fn list_users(
    State(state): State<Arc<ServerState>>,
) -> Result<Json<Vec<UserResponse>>> {
    let pool = state.database.pool();
    
    let users = sqlx::query!(
        r#"
        SELECT id, username, email, org_id, created_at
        FROM users
        ORDER BY created_at DESC
        "#
    )
    .fetch_all(pool)
    .await?;
    
    let response: Vec<UserResponse> = users.into_iter().map(|u| UserResponse {
        id: u.id,
        username: u.username,
        email: u.email,
        org_id: u.org_id,
        is_active: true,
        is_banned: false,
        last_login: None,
    }).collect();
    
    Ok(Json(response))
}

/// GET /api/admin/users/:id - Get specific user
pub async fn get_user(
    Path(user_id): Path<i32>,
    State(state): State<Arc<ServerState>>,
) -> Result<Json<UserResponse>> {
    let pool = state.database.pool();
    
    let user = sqlx::query!(
        r#"
        SELECT id, username, email, org_id, created_at
        FROM users
        WHERE id = $1
        "#,
        user_id
    )
    .fetch_one(pool)
    .await?;
    
    Ok(Json(UserResponse {
        id: user.id,
        username: user.username,
        email: user.email,
        org_id: user.org_id,
        is_active: true,
        is_banned: false,
        last_login: None,
    }))
}

/// POST /api/admin/users - Create new user
pub async fn create_user(
    State(state): State<Arc<ServerState>>,
    Json(req): Json<CreateUserRequest>,
) -> Result<(StatusCode, Json<serde_json::Value>)> {
    let pool = state.database.pool();
    
    // Hash password
    let password_hash = crate::auth::hash_password(&req.password)?;
    
    let result = sqlx::query!(
        r#"
        INSERT INTO users (username, password_hash, email, org_id)
        VALUES ($1, $2, $3, $4)
        RETURNING id
        "#,
        req.username,
        password_hash,
        req.email,
        req.org_id
    )
    .fetch_one(pool)
    .await?;
    
    Ok((
        StatusCode::CREATED,
        Json(serde_json::json!({ "id": result.id }))
    ))
}

/// PUT /api/admin/users/:id - Update user
pub async fn update_user(
    Path(user_id): Path<i32>,
    State(state): State<Arc<ServerState>>,
    Json(req): Json<UpdateUserRequest>,
) -> Result<Json<serde_json::Value>> {
    let pool = state.database.pool();
    
    // Build dynamic update query based on provided fields
    if let Some(username) = req.username {
        sqlx::query!(
            "UPDATE users SET username = $1 WHERE id = $2",
            username,
            user_id
        )
        .execute(pool)
        .await?;
    }
    
    if let Some(email) = req.email {
        sqlx::query!(
            "UPDATE users SET email = $1 WHERE id = $2",
            email,
            user_id
        )
        .execute(pool)
        .await?;
    }
    
    Ok(Json(serde_json::json!({ "success": true })))
}

/// DELETE /api/admin/users/:id - Delete user
pub async fn delete_user(
    Path(user_id): Path<i32>,
    State(state): State<Arc<ServerState>>,
) -> Result<Json<serde_json::Value>> {
    let pool = state.database.pool();
    
    sqlx::query!(
        "DELETE FROM users WHERE id = $1",
        user_id
    )
    .execute(pool)
    .await?;
    
    Ok(Json(serde_json::json!({ "success": true })))
}

/// POST /api/admin/users/:id/ban - Ban user
pub async fn ban_user(
    Path(_user_id): Path<i32>,
    State(_state): State<Arc<ServerState>>,
) -> Result<Json<serde_json::Value>> {
    // TODO: Add is_banned column to users table
    // For now, just return success
    Ok(Json(serde_json::json!({ "success": true, "message": "Ban feature pending database schema" })))
}

/// POST /api/admin/users/:id/unban - Unban user
pub async fn unban_user(
    Path(_user_id): Path<i32>,
    State(_state): State<Arc<ServerState>>,
) -> Result<Json<serde_json::Value>> {
    // TODO: Add is_banned column to users table
    Ok(Json(serde_json::json!({ "success": true, "message": "Unban feature pending database schema" })))
}

/// POST /api/admin/users/:id/reset-password - Reset user password
pub async fn reset_password(
    Path(user_id): Path<i32>,
    State(state): State<Arc<ServerState>>,
    Json(req): Json<ResetPasswordRequest>,
) -> Result<Json<serde_json::Value>> {
    let pool = state.database.pool();
    let password_hash = crate::auth::hash_password(&req.password)?;
    
    sqlx::query!(
        "UPDATE users SET password_hash = $1 WHERE id = $2",
        password_hash,
        user_id
    )
    .execute(pool)
    .await?;
    
    Ok(Json(serde_json::json!({ "success": true })))
}
