use thiserror::Error;
use axum::{
    http::StatusCode,
    response::{IntoResponse, Response},
    Json,
};
use serde_json::json;

#[derive(Error, Debug)]
pub enum VoipError {
    #[error("Authentication failed: {0}")]
    AuthenticationFailed(String),
    
    #[error("Permission denied: {0}")]
    PermissionDenied(String),
    
    #[error("Channel not found: {0}")]
    ChannelNotFound(u32),
    
    #[error("User not found: {0}")]
    UserNotFound(u32),
    
    #[error("Organization not found: {0}")]
    OrgNotFound(u32),
    
    #[error("Invalid packet: {0}")]
    InvalidPacket(String),
    
    #[error("Invalid state: {0}")]
    InvalidState(String),
    
    #[error("Database error: {0}")]
    DatabaseError(#[from] sqlx::Error),

    #[error("Migration error: {0}")]
    MigrationError(#[from] sqlx::migrate::MigrateError),

    #[error("Redis error: {0}")]
    RedisError(#[from] redis::RedisError),
    
    #[error("JWT error: {0}")]
    JwtError(#[from] jsonwebtoken::errors::Error),
    
    #[error("Encryption error: {0}")]
    EncryptionError(String),
    
    #[error("Configuration error: {0}")]
    ConfigError(#[from] config::ConfigError),
    
    #[error("IO error: {0}")]
    IoError(#[from] std::io::Error),
    
    #[error("{0}")]
    Other(String),
}

pub type Result<T> = std::result::Result<T, VoipError>;

// Implement IntoResponse for VoipError so it can be used in Axum handlers
impl IntoResponse for VoipError {
    fn into_response(self) -> Response {
        let (status, message) = match self {
            VoipError::AuthenticationFailed(msg) => (StatusCode::UNAUTHORIZED, msg),
            VoipError::PermissionDenied(msg) => (StatusCode::FORBIDDEN, msg),
            VoipError::ChannelNotFound(id) => (StatusCode::NOT_FOUND, format!("Channel {} not found", id)),
            VoipError::UserNotFound(id) => (StatusCode::NOT_FOUND, format!("User {} not found", id)),
            VoipError::OrgNotFound(id) => (StatusCode::NOT_FOUND, format!("Organization {} not found", id)),
            VoipError::InvalidPacket(msg) => (StatusCode::BAD_REQUEST, msg),
            VoipError::InvalidState(msg) => (StatusCode::BAD_REQUEST, msg),
            VoipError::DatabaseError(e) => (StatusCode::INTERNAL_SERVER_ERROR, format!("Database error: {}", e)),
            VoipError::MigrationError(e) => (StatusCode::INTERNAL_SERVER_ERROR, format!("Migration error: {}", e)),
            VoipError::RedisError(e) => (StatusCode::INTERNAL_SERVER_ERROR, format!("Redis error: {}", e)),
            VoipError::JwtError(e) => (StatusCode::UNAUTHORIZED, format!("JWT error: {}", e)),
            VoipError::EncryptionError(msg) => (StatusCode::INTERNAL_SERVER_ERROR, msg),
            VoipError::ConfigError(e) => (StatusCode::INTERNAL_SERVER_ERROR, format!("Config error: {}", e)),
            VoipError::IoError(e) => (StatusCode::INTERNAL_SERVER_ERROR, format!("IO error: {}", e)),
            VoipError::Other(msg) => (StatusCode::INTERNAL_SERVER_ERROR, msg),
        };

        let body = Json(json!({
            "error": message,
            "status": status.as_u16()
        }));

        (status, body).into_response()
    }
}
