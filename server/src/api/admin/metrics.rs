//! Admin API endpoints for system metrics

use axum::{
    extract::State,
    Json,
};
use std::sync::Arc;
use serde_json::{json, Value};
use crate::error::Result;
use crate::network::tls::ServerState;

/// GET /api/admin/metrics/system - Get system metrics
pub async fn system_metrics(
    State(state): State<Arc<ServerState>>,
) -> Result<Json<Value>> {
    let pool = state.database.pool();
    
    // Get user counts
    let total_users = sqlx::query_scalar::<_, i64>("SELECT COUNT(*) FROM users")
        .fetch_one(pool)
        .await?;
    
    let total_channels = sqlx::query_scalar::<_, i64>("SELECT COUNT(*) FROM channels")
        .fetch_one(pool)
        .await?;
    
    let total_orgs = sqlx::query_scalar::<_, i64>("SELECT COUNT(*) FROM organizations")
        .fetch_one(pool)
        .await?;
    
    // TODO: Get real-time metrics from server state
    let metrics = json!({
        "server_status": "online",
        "total_users": total_users,
        "active_users": 0,  // TODO: Track active connections
        "total_channels": total_channels,
        "total_organizations": total_orgs,
        "active_sessions": 0,  // TODO: Track active voice sessions
        "avg_latency_ms": 0,   // TODO: Calculate from voice metrics
        "cpu_usage_percent": 0.0,  // TODO: Use sysinfo crate
        "memory_usage_mb": 0,      // TODO: Use sysinfo crate
        "timestamp": chrono::Utc::now().to_rfc3339(),
    });
    
    Ok(Json(metrics))
}

/// GET /api/admin/metrics/users - Get user metrics
pub async fn user_metrics(
    State(state): State<Arc<ServerState>>,
) -> Result<Json<Value>> {
    let pool = state.database.pool();
    
    let total_users = sqlx::query_scalar::<_, i64>("SELECT COUNT(*) FROM users")
        .fetch_one(pool)
        .await?;
    
    // Users created in last 7 days
    let new_this_week = sqlx::query_scalar::<_, i64>(
        "SELECT COUNT(*) FROM users WHERE created_at >= NOW() - INTERVAL '7 days'"
    )
    .fetch_one(pool)
    .await?;
    
    // Users active in last 24 hours (based on last_activity)
    let active_today = sqlx::query_scalar::<_, i64>(
        "SELECT COUNT(*) FROM users WHERE last_activity >= NOW() - INTERVAL '24 hours'"
    )
    .fetch_one(pool)
    .await?;
    
    let metrics = json!({
        "total_users": total_users,
        "active_today": active_today,
        "new_this_week": new_this_week,
        "timestamp": chrono::Utc::now().to_rfc3339(),
    });
    
    Ok(Json(metrics))
}

/// GET /api/admin/metrics/voice - Get voice quality metrics
pub async fn voice_metrics(
    State(_state): State<Arc<ServerState>>,
) -> Result<Json<Value>> {
    // TODO: Implement voice quality tracking
    let metrics = json!({
        "active_sessions": 0,
        "avg_latency_ms": 0,
        "avg_packet_loss": 0.0,
        "avg_jitter_ms": 0,
        "timestamp": chrono::Utc::now().to_rfc3339(),
    });
    
    Ok(Json(metrics))
}
