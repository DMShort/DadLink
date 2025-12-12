//! Admin API endpoints for role management

use axum::{extract::{Path, State}, http::StatusCode, Json};
use serde::{Deserialize, Serialize};
use std::sync::Arc;
use crate::error::Result;
use crate::network::tls::ServerState;

#[derive(Debug, Deserialize)]
pub struct CreateRoleRequest {
    pub org_id: i32,
    pub name: String,
    pub permissions: u32,
    pub priority: i32,
}

#[derive(Debug, Deserialize)]
pub struct UpdateRoleRequest {
    pub name: Option<String>,
    pub permissions: Option<u32>,
    pub priority: Option<i32>,
}

#[derive(Debug, Serialize)]
pub struct RoleResponse {
    pub id: i32,
    pub org_id: i32,
    pub name: String,
    pub permissions: u32,
    pub priority: i32,
}

/// GET /api/admin/roles
pub async fn list_roles(State(state): State<Arc<ServerState>>) -> Result<Json<Vec<RoleResponse>>> {
    let pool = state.database.pool();
    
    let roles = sqlx::query!(
        r#"
        SELECT id, org_id, name, permissions, priority
        FROM roles
        ORDER BY priority DESC
        "#
    )
    .fetch_all(pool)
    .await?;
    
    let response: Vec<RoleResponse> = roles.into_iter().map(|r| RoleResponse {
        id: r.id,
        org_id: r.org_id,
        name: r.name,
        permissions: r.permissions as u32,
        priority: r.priority,
    }).collect();
    
    Ok(Json(response))
}

/// GET /api/admin/roles/:id
pub async fn get_role(
    Path(role_id): Path<i32>,
    State(state): State<Arc<ServerState>>,
) -> Result<Json<RoleResponse>> {
    let pool = state.database.pool();
    
    let role = sqlx::query!(
        r#"
        SELECT id, org_id, name, permissions, priority
        FROM roles
        WHERE id = $1
        "#,
        role_id
    )
    .fetch_one(pool)
    .await?;
    
    Ok(Json(RoleResponse {
        id: role.id,
        org_id: role.org_id,
        name: role.name,
        permissions: role.permissions as u32,
        priority: role.priority,
    }))
}

/// POST /api/admin/roles
pub async fn create_role(
    State(state): State<Arc<ServerState>>,
    Json(req): Json<CreateRoleRequest>,
) -> Result<(StatusCode, Json<RoleResponse>)> {
    let pool = state.database.pool();
    
    let role = sqlx::query!(
        r#"
        INSERT INTO roles (org_id, name, permissions, priority)
        VALUES ($1, $2, $3, $4)
        RETURNING id, org_id, name, permissions, priority
        "#,
        req.org_id,
        req.name,
        req.permissions as i32,
        req.priority
    )
    .fetch_one(pool)
    .await?;
    
    Ok((StatusCode::CREATED, Json(RoleResponse {
        id: role.id,
        org_id: role.org_id,
        name: role.name,
        permissions: role.permissions as u32,
        priority: role.priority,
    })))
}

/// PUT /api/admin/roles/:id
pub async fn update_role(
    Path(role_id): Path<i32>,
    State(state): State<Arc<ServerState>>,
    Json(req): Json<UpdateRoleRequest>,
) -> Result<Json<RoleResponse>> {
    let pool = state.database.pool();
    
    let role = sqlx::query!(
        r#"
        UPDATE roles
        SET name = COALESCE($2, name),
            permissions = COALESCE($3, permissions),
            priority = COALESCE($4, priority)
        WHERE id = $1
        RETURNING id, org_id, name, permissions, priority
        "#,
        role_id,
        req.name,
        req.permissions.map(|p| p as i32),
        req.priority
    )
    .fetch_one(pool)
    .await?;
    
    Ok(Json(RoleResponse {
        id: role.id,
        org_id: role.org_id,
        name: role.name,
        permissions: role.permissions as u32,
        priority: role.priority,
    }))
}

/// DELETE /api/admin/roles/:id
pub async fn delete_role(
    Path(role_id): Path<i32>,
    State(state): State<Arc<ServerState>>,
) -> Result<Json<serde_json::Value>> {
    let pool = state.database.pool();
    
    sqlx::query!(
        r#"
        DELETE FROM roles
        WHERE id = $1
        "#,
        role_id
    )
    .execute(pool)
    .await?;
    
    Ok(Json(serde_json::json!({ "success": true })))
}
