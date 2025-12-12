//! Admin API endpoints for organization management

use axum::{
    extract::{Path, State},
    http::StatusCode,
    Json,
};
use serde::Serialize;
use std::sync::Arc;
use crate::error::Result;
use crate::network::tls::ServerState;

#[derive(Debug, Serialize)]
pub struct OrgResponse {
    pub id: i32,
    pub name: String,
    pub tag: String,
    pub owner_id: Option<i32>,
}

/// GET /api/admin/organizations
pub async fn list_organizations(State(state): State<Arc<ServerState>>) -> Result<Json<Vec<OrgResponse>>> {
    let pool = state.database.pool();
    let orgs = sqlx::query_as!(
        OrgResponse,
        "SELECT id, name, tag, owner_id FROM organizations ORDER BY name"
    )
    .fetch_all(pool)
    .await?;
    
    Ok(Json(orgs))
}

/// GET /api/admin/organizations/:id
pub async fn get_organization(
    Path(id): Path<i32>,
    State(state): State<Arc<ServerState>>,
) -> Result<Json<OrgResponse>> {
    let pool = state.database.pool();
    let org = sqlx::query_as!(
        OrgResponse,
        "SELECT id, name, tag, owner_id FROM organizations WHERE id = $1",
        id
    )
    .fetch_one(pool)
    .await?;
    
    Ok(Json(org))
}

/// POST /api/admin/organizations
pub async fn create_organization(
    State(_state): State<Arc<ServerState>>,
    Json(_org): Json<serde_json::Value>,
) -> Result<(StatusCode, Json<serde_json::Value>)> {
    // TODO: Implement org creation
    Ok((StatusCode::CREATED, Json(serde_json::json!({ "id": 1 }))))
}

/// PUT /api/admin/organizations/:id
pub async fn update_organization(
    Path(_id): Path<i32>,
    State(_state): State<Arc<ServerState>>,
    Json(_org): Json<serde_json::Value>,
) -> Result<Json<serde_json::Value>> {
    Ok(Json(serde_json::json!({ "success": true })))
}

/// DELETE /api/admin/organizations/:id
pub async fn delete_organization(
    Path(_id): Path<i32>,
    State(_state): State<Arc<ServerState>>,
) -> Result<Json<serde_json::Value>> {
    Ok(Json(serde_json::json!({ "success": true })))
}
