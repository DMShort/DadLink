//! Admin API endpoints for audit log

use axum::{
    extract::{Query, State},
    Json,
};
use std::sync::Arc;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use sqlx::FromRow;
use crate::error::Result;
use crate::network::tls::ServerState;

#[derive(Debug, FromRow, Serialize)]
pub struct AuditLogEntry {
    id: i64,
    org_id: Option<i32>,
    user_id: Option<i32>,
    action: String,
    resource_type: Option<String>,
    resource_id: Option<i32>,
    details: Option<Value>,
    ip_address: Option<String>,
    created_at: chrono::DateTime<chrono::Utc>,
}

#[derive(Debug, Deserialize)]
pub struct AuditLogParams {
    #[serde(default = "default_limit")]
    limit: i64,
    #[serde(default)]
    offset: i64,
}

fn default_limit() -> i64 {
    50
}

#[derive(Debug, Deserialize)]
pub struct AuditLogSearchFilters {
    user_id: Option<i32>,
    action: Option<String>,
    resource_type: Option<String>,
    #[serde(default = "default_limit")]
    limit: i64,
    #[serde(default)]
    offset: i64,
}

/// GET /api/admin/audit-log
pub async fn list_audit_logs(
    State(state): State<Arc<ServerState>>,
    Query(params): Query<AuditLogParams>,
) -> Result<Json<Vec<AuditLogEntry>>> {
    let pool = state.database.pool();
    
    let logs = sqlx::query_as::<_, AuditLogEntry>(
        r#"
        SELECT id, org_id, user_id, action, resource_type, resource_id, 
               details, ip_address::text as ip_address, created_at
        FROM audit_log
        ORDER BY created_at DESC
        LIMIT $1 OFFSET $2
        "#
    )
    .bind(params.limit)
    .bind(params.offset)
    .fetch_all(pool)
    .await?;
    
    Ok(Json(logs))
}

/// POST /api/admin/audit-log/search
pub async fn search_audit_logs(
    State(state): State<Arc<ServerState>>,
    Json(filters): Json<AuditLogSearchFilters>,
) -> Result<Json<Vec<AuditLogEntry>>> {
    let pool = state.database.pool();
    
    let mut query = String::from(
        "SELECT id, org_id, user_id, action, resource_type, resource_id, \
         details, ip_address::text as ip_address, created_at \
         FROM audit_log WHERE 1=1"
    );
    
    if filters.user_id.is_some() {
        query.push_str(&format!(" AND user_id = {}", filters.user_id.unwrap()));
    }
    
    if let Some(ref action) = filters.action {
        query.push_str(&format!(" AND action ILIKE '%{}%'", action));
    }
    
    if let Some(ref resource_type) = filters.resource_type {
        query.push_str(&format!(" AND resource_type = '{}'", resource_type));
    }
    
    query.push_str(&format!(
        " ORDER BY created_at DESC LIMIT {} OFFSET {}",
        filters.limit, filters.offset
    ));
    
    let logs = sqlx::query_as::<_, AuditLogEntry>(&query)
        .fetch_all(pool)
        .await?;
    
    Ok(Json(logs))
}
