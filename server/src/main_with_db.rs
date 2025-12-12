mod config;
mod error;
mod types;
mod network;
mod routing;
mod auth;
mod org;
mod channel;
mod channel_manager;
mod user_registry;
mod db;

use anyhow::Result;
use tracing::{info, error, warn};
use tracing_subscriber::{layer::SubscriberExt, util::SubscriberInitExt};

#[tokio::main]
async fn main() -> Result<()> {
    // Initialize logging
    tracing_subscriber::registry()
        .with(
            tracing_subscriber::EnvFilter::try_from_default_env()
                .unwrap_or_else(|_| "voip_server=debug,tower_http=debug,sqlx=warn".into()),
        )
        .with(tracing_subscriber::fmt::layer())
        .init();

    info!("🎤 VoIP Server v{}", env!("CARGO_PKG_VERSION"));

    // Load configuration
    let config = config::ServerConfig::load().unwrap_or_default();
    info!("⚙️  Configuration loaded");
    info!("   Control port: {}", config.server.control_port);
    info!("   Voice port: {}", config.server.voice_port);
    info!("   Database: {}", config.database.url.split('@').last().unwrap_or("unknown"));

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
    info!("✅ Repositories initialized");

    // Create channel manager (in-memory for real-time state)
    let channel_manager = std::sync::Arc::new(channel_manager::ChannelManager::new());
    info!("✅ Channel manager initialized");

    // Create voice router
    let router = std::sync::Arc::new(routing::VoiceRouter::new());
    info!("✅ Voice router initialized");

    // Create server state with database-backed repositories
    let state = std::sync::Arc::new(network::tls::ServerStateWithDb {
        jwt_secret: config.security.jwt_secret.clone(),
        channel_manager: channel_manager.clone(),
        user_repo: user_repo.clone(),
        channel_repo: channel_repo.clone(),
        org_repo: org_repo.clone(),
        voice_router: router.clone(),
        database: database.clone(),
    });

    // Start UDP voice server
    info!("🎙️  Starting UDP voice server on {}", config.voice_addr());
    let udp_server = std::sync::Arc::new(
        network::udp::UdpVoiceServer::bind(
            config.voice_addr(),
            router.clone(),
            channel_manager.clone()
        ).await?
    );
    let udp_handle = udp_server.start();
    info!("✅ UDP voice server started");

    // Start TLS control server (WebSocket)
    info!("🔐 Starting WebSocket control server on {}", control_addr);
    let app = network::tls::control_routes_with_db().with_state(state);
    let control_handle = tokio::spawn(async move {
        let listener = tokio::net::TcpListener::bind(control_addr)
            .await
            .expect("Failed to bind control port");
        info!("✅ WebSocket control server started");

        axum::serve(listener, app)
            .await
            .expect("Server failed");
    });

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
