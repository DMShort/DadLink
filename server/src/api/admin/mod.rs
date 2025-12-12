//! Admin API endpoints
//! 
//! All admin API endpoints are defined in their respective modules and
//! integrated into the main router via network::tls::admin_routes()

pub mod users;
pub mod organizations;
pub mod channels;
pub mod roles;
pub mod metrics;
pub mod audit_log;
