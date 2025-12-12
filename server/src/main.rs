mod config;
mod error;
mod types;
mod network;
mod routing;
mod auth;
mod org;
mod channel;
mod channel_manager;
mod db;
mod permissions;
mod crypto;
mod api;

use anyhow::Result;
use tracing::{info, error, warn};
use tracing_subscriber::{layer::SubscriberExt, util::SubscriberInitExt};

#[tokio::main]
async fn main() -> Result<()> {
    // Load .env file
    dotenvy::dotenv().ok();
    
    // Initialize logging
    tracing_subscriber::registry()
        .with(
            tracing_subscriber::EnvFilter::try_from_default_env()
                .unwrap_or_else(|_| "voip_server=debug,tower_http=debug".into()),
        )
        .with(tracing_subscriber::fmt::layer())
        .init();

    info!("🎤 VoIP Server v{}", env!("CARGO_PKG_VERSION"));

    // Load configuration
    let config = config::ServerConfig::load().unwrap_or_default();
    info!("⚙️  Configuration loaded");
    info!("   Control port: {}", config.server.control_port);
    info!("   Voice port: {}", config.server.voice_port);
    info!("   Database: {}", sanitize_db_url(&config.database.url));

    // Save ports for later logging
    let voice_port = config.server.voice_port;
    let control_port = config.server.control_port;
    let control_addr = config.control_addr();

    // Initialize database
    info!("🔌 Initializing database...");
    let database = db::Database::new(&config.database).await?;

    // Run migrations
    database.migrate().await?;

    // Health check
    database.health_check().await?;
    info!("✅ Database ready");

    // Get database pool
    let db_pool = database.pool().clone();

    // Create repositories
    let user_repo = std::sync::Arc::new(db::UserRepository::new(db_pool.clone()));
    let channel_repo = std::sync::Arc::new(db::ChannelRepository::new(db_pool.clone()));
    let org_repo = std::sync::Arc::new(db::OrganizationRepository::new(db_pool.clone()));
    let role_repo = std::sync::Arc::new(db::RoleRepository::new(db_pool.clone()));
    info!("✅ Repositories initialized");

    // Create channel manager (in-memory for real-time state)
    let channel_manager = std::sync::Arc::new(channel_manager::ChannelManager::new());
    info!("✅ Channel manager initialized");

    // Create voice router
    let router = std::sync::Arc::new(routing::VoiceRouter::new());
    info!("✅ Voice router initialized");

    // Create permission checker
    let permission_checker = std::sync::Arc::new(permissions::PermissionChecker::new(
        user_repo.clone(),
        channel_repo.clone(),
        role_repo.clone(),
    ));
    info!("✅ Permission checker initialized");

    // Create SRTP session manager for voice encryption
    let srtp_sessions = std::sync::Arc::new(crypto::SrtpSessionManager::new());
    info!("✅ SRTP session manager initialized");

    // Create pending key exchanges storage
    let pending_key_exchanges = std::sync::Arc::new(tokio::sync::RwLock::new(std::collections::HashMap::new()));

    // Create server state with database-backed repositories
    let state = std::sync::Arc::new(network::tls::ServerState {
        jwt_secret: config.security.jwt_secret.clone(),
        channel_manager: channel_manager.clone(),
        user_repo: user_repo.clone(),
        channel_repo: channel_repo.clone(),
        org_repo: org_repo.clone(),
        role_repo: role_repo.clone(),
        voice_router: router.clone(),
        database: database.clone(),
        permission_checker: permission_checker.clone(),
        srtp_sessions: srtp_sessions.clone(),
        pending_key_exchanges: pending_key_exchanges.clone(),
    });

    // Start UDP voice server
    info!("Starting UDP voice server on {}", config.voice_addr());
    let udp_server = std::sync::Arc::new(
        network::udp::UdpVoiceServer::bind(
            config.voice_addr(),
            router.clone(),
            channel_manager.clone(),
            srtp_sessions.clone()
        ).await?
    );
    let udp_handle = udp_server.start();
    info!("✅ UDP voice server started");

    // Load TLS certificates
    let tls_config = match load_tls_config(&config.security.tls_cert, &config.security.tls_key) {
        Ok(cfg) => {
            info!("✅ TLS certificates loaded successfully");
            Some(cfg)
        }
        Err(e) => {
            warn!("⚠️  Failed to load TLS certificates: {}", e);
            warn!("   Starting server WITHOUT TLS (insecure mode)");
            warn!("   Generate certificates with: cd server && ./tools/generate_certs.sh");
            None
        }
    };

    // Start WebSocket control server (with TLS if available)
    let app = network::tls::control_routes().with_state(state);
    let control_handle = if let Some(tls) = tls_config {
        info!("Starting SECURE WebSocket server (WSS) on {}", control_addr);
        tokio::spawn(async move {
            axum_server::bind_rustls(control_addr, tls)
                .serve(app.into_make_service())
                .await
                .expect("Server failed");
        })
    } else {
        info!("Starting INSECURE WebSocket server (WS) on {}", control_addr);
        tokio::spawn(async move {
            let listener = tokio::net::TcpListener::bind(control_addr)
                .await
                .expect("Failed to bind control port");
            axum::serve(listener, app)
                .await
                .expect("Server failed");
        })
    };
    info!("✅ WebSocket control server started");

    info!("🚀 Server initialization complete");
    info!("");
    info!("📡 Voice: UDP port {}", voice_port);
    info!("🔐 Control: WebSocket port {}", control_port);
    info!("💾 Database: PostgreSQL connected");
    info!("");
    info!("Press Ctrl+C to shutdown");
    info!("");

    // Wait for shutdown signal
    tokio::select! {
        _ = tokio::signal::ctrl_c() => {
            info!("🛑 Shutdown signal received");
        }
        _ = udp_handle => {
            error!("❌ UDP server terminated unexpectedly");
        }
        _ = control_handle => {
            error!("❌ Control server terminated unexpectedly");
        }
    }

    info!("🔄 Server shutting down...");

    // Cleanup
    database.close().await;

    info!("✅ Server stopped successfully");
    Ok(())
}

/// Sanitize database URL for logging (hide password)
fn sanitize_db_url(url: &str) -> String {
    if let Some(at_pos) = url.rfind('@') {
        if let Some(colon_pos) = url[..at_pos].rfind(':') {
            let mut sanitized = url.to_string();
            sanitized.replace_range(colon_pos + 1..at_pos, "****");
            return sanitized;
        }
    }
    url.to_string()
}

/// Load TLS configuration from certificate and key files
fn load_tls_config(
    cert_path: &str,
    key_path: &str,
) -> Result<axum_server::tls_rustls::RustlsConfig> {
    use std::path::PathBuf;

    let cert_file = PathBuf::from(cert_path);
    let key_file = PathBuf::from(key_path);

    // Check if files exist
    if !cert_file.exists() {
        return Err(anyhow::anyhow!("Certificate file not found: {}", cert_path));
    }
    if !key_file.exists() {
        return Err(anyhow::anyhow!("Private key file not found: {}", key_path));
    }

    // Load certificate and key
    let config = axum_server::tls_rustls::RustlsConfig::from_pem_file(
        cert_file,
        key_file,
    );

    // This returns a Future, but we need to block here
    // Use tokio::runtime::Handle to block on it
    Ok(tokio::task::block_in_place(|| {
        tokio::runtime::Handle::current().block_on(config)
    })?)
}
