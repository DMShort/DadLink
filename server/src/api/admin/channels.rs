//! Admin API endpoints for channel management

use axum::{extract::{Path, State}, http::StatusCode, Json};
use serde::{Deserialize, Serialize};
use std::sync::Arc;
use crate::error::Result;
use crate::network::tls::ServerState;

#[derive(Debug, Deserialize)]
pub struct CreateChannelRequest {
    pub org_id: i32,
    pub parent_id: Option<i32>,
    pub name: String,
    pub description: String,
    pub max_users: Option<i32>,
}

#[derive(Debug, Deserialize)]
pub struct UpdateChannelRequest {
    pub name: Option<String>,
    pub description: Option<String>,
    pub max_users: Option<i32>,
}

#[derive(Debug, Serialize)]
pub struct ChannelResponse {
    pub id: i32,
    pub org_id: i32,
    pub parent_id: Option<i32>,
    pub name: String,
    pub description: String,
    pub max_users: i32,
    pub current_users: i32,
}

/// GET /api/admin/channels
pub async fn list_channels(State(state): State<Arc<ServerState>>) -> Result<Json<Vec<ChannelResponse>>> {
    let pool = state.database.pool();
    
    let channels = sqlx::query!(
        r#"
        SELECT id, org_id, parent_id, name, description, max_users
        FROM channels
        ORDER BY position ASC
        "#
    )
    .fetch_all(pool)
    .await?;
    
    let response: Vec<ChannelResponse> = channels.into_iter().map(|c| ChannelResponse {
        id: c.id,
        org_id: c.org_id,
        parent_id: c.parent_id,
        name: c.name,
        description: c.description.unwrap_or_default(),
        max_users: c.max_users.unwrap_or(50),
        current_users: 0, // TODO: Query active user count
    }).collect();
    
    Ok(Json(response))
}

/// GET /api/admin/channels/:id
pub async fn get_channel(
    Path(channel_id): Path<i32>,
    State(state): State<Arc<ServerState>>,
) -> Result<Json<ChannelResponse>> {
    let pool = state.database.pool();
    
    let channel = sqlx::query!(
        r#"
        SELECT id, org_id, parent_id, name, description, max_users
        FROM channels
        WHERE id = $1
        "#,
        channel_id
    )
    .fetch_one(pool)
    .await?;
    
    Ok(Json(ChannelResponse {
        id: channel.id,
        org_id: channel.org_id,
        parent_id: channel.parent_id,
        name: channel.name,
        description: channel.description.unwrap_or_default(),
        max_users: channel.max_users.unwrap_or(50),
        current_users: 0, // TODO: Query active user count
    }))
}

/// POST /api/admin/channels
pub async fn create_channel(
    State(state): State<Arc<ServerState>>,
    Json(req): Json<CreateChannelRequest>,
) -> Result<(StatusCode, Json<ChannelResponse>)> {
    let pool = state.database.pool();
    
    let channel = sqlx::query!(
        r#"
        INSERT INTO channels (org_id, parent_id, name, description, max_users, position)
        VALUES ($1, $2, $3, $4, $5, COALESCE((SELECT MAX(position) + 1 FROM channels WHERE org_id = $1), 0))
        RETURNING id, org_id, parent_id, name, description, max_users
        "#,
        req.org_id,
        req.parent_id,
        req.name,
        req.description,
        req.max_users.unwrap_or(50)
    )
    .fetch_one(pool)
    .await?;
    
    Ok((StatusCode::CREATED, Json(ChannelResponse {
        id: channel.id,
        org_id: channel.org_id,
        parent_id: channel.parent_id,
        name: channel.name,
        description: channel.description.unwrap_or_default(),
        max_users: channel.max_users.unwrap_or(50),
        current_users: 0,
    })))
}

/// PUT /api/admin/channels/:id
pub async fn update_channel(
    Path(channel_id): Path<i32>,
    State(state): State<Arc<ServerState>>,
    Json(req): Json<UpdateChannelRequest>,
) -> Result<Json<ChannelResponse>> {
    let pool = state.database.pool();
    
    // Build dynamic update query
    let mut updates = vec![];
    if req.name.is_some() { updates.push("name = COALESCE($2, name)"); }
    if req.description.is_some() { updates.push("description = COALESCE($3, description)"); }
    if req.max_users.is_some() { updates.push("max_users = COALESCE($4, max_users)"); }
    
    let channel = sqlx::query!(
        r#"
        UPDATE channels
        SET name = COALESCE($2, name),
            description = COALESCE($3, description),
            max_users = COALESCE($4, max_users)
        WHERE id = $1
        RETURNING id, org_id, parent_id, name, description, max_users
        "#,
        channel_id,
        req.name,
        req.description,
        req.max_users
    )
    .fetch_one(pool)
    .await?;
    
    Ok(Json(ChannelResponse {
        id: channel.id,
        org_id: channel.org_id,
        parent_id: channel.parent_id,
        name: channel.name,
        description: channel.description.unwrap_or_default(),
        max_users: channel.max_users.unwrap_or(50),
        current_users: 0,
    }))
}

/// DELETE /api/admin/channels/:id
pub async fn delete_channel(
    Path(channel_id): Path<i32>,
    State(state): State<Arc<ServerState>>,
) -> Result<Json<serde_json::Value>> {
    let pool = state.database.pool();
    
    sqlx::query!(
        r#"
        DELETE FROM channels
        WHERE id = $1
        "#,
        channel_id
    )
    .execute(pool)
    .await?;
    
    Ok(Json(serde_json::json!({ "success": true })))
}
