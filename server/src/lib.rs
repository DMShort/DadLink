//! VoIP Server Library
//! 
//! This library provides the core functionality for the VoIP server,
//! including database access, networking, and voice routing.

pub mod config;
pub mod error;
pub mod types;
pub mod db;
pub mod permissions;
pub mod auth;
pub mod org;
pub mod channel;
pub mod channel_manager;
pub mod network;
pub mod routing;
pub mod crypto;
pub mod api;  // Admin REST API
mod user_registry;

// Re-export commonly used types
pub use error::{Result, VoipError};
pub use types::{UserId, ChannelId, OrgId, RoleId, Permissions};
