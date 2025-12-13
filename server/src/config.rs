use serde::Deserialize;
use std::net::SocketAddr;

#[derive(Debug, Deserialize, Clone)]
pub struct ServerConfig {
    pub server: NetworkConfig,
    pub database: DatabaseConfig,
    pub redis: RedisConfig,
    pub security: SecurityConfig,
    pub audio: AudioConfig,
    pub limits: LimitsConfig,
}

#[derive(Debug, Deserialize, Clone)]
pub struct NetworkConfig {
    pub bind_address: String,
    pub control_port: u16,
    pub voice_port: u16,
    pub max_connections: usize,
}

#[derive(Debug, Deserialize, Clone)]
pub struct DatabaseConfig {
    pub url: String,
    pub max_connections: u32,
}

#[derive(Debug, Deserialize, Clone)]
pub struct RedisConfig {
    pub url: String,
}

#[derive(Debug, Deserialize, Clone)]
pub struct SecurityConfig {
    pub tls_cert: String,
    pub tls_key: String,
    pub jwt_secret: String,
}

#[derive(Debug, Deserialize, Clone)]
pub struct AudioConfig {
    pub max_bitrate: u32,
    pub default_bitrate: u32,
}

#[derive(Debug, Deserialize, Clone)]
pub struct LimitsConfig {
    pub max_channels_per_org: u32,
    pub max_users_per_org: u32,
    pub max_users_per_channel: u32,
}

impl ServerConfig {
    pub fn load() -> anyhow::Result<Self> {
        let config = config::Config::builder()
            .add_source(config::File::with_name("config/server").required(false))
            .add_source(config::Environment::with_prefix("VOIP").separator("__"))
            .build()?;

        let mut server_config: ServerConfig = config.try_deserialize()?;

        // Override database URL from environment if set
        if let Ok(db_url) = std::env::var("VOIP__DATABASE__URL") {
            server_config.database.url = db_url;
        }

        Ok(server_config)
    }
    
    pub fn control_addr(&self) -> SocketAddr {
        format!("{}:{}", self.server.bind_address, self.server.control_port)
            .parse()
            .expect("Invalid control address")
    }
    
    pub fn voice_addr(&self) -> SocketAddr {
        format!("{}:{}", self.server.bind_address, self.server.voice_port)
            .parse()
            .expect("Invalid voice address")
    }
}

impl Default for ServerConfig {
    fn default() -> Self {
        Self {
            server: NetworkConfig {
                bind_address: "0.0.0.0".to_string(),
                control_port: 9000,
                voice_port: 9001,
                max_connections: 1000,
            },
            database: DatabaseConfig {
                url: "postgresql://voip:voip@postgres/voip".to_string(),
                max_connections: 20,
            },
            redis: RedisConfig {
                url: "redis://redis:6379".to_string(),
            },
            security: SecurityConfig {
                tls_cert: "certs/cert.pem".to_string(),
                tls_key: "certs/key.pem".to_string(),
                jwt_secret: "CHANGE_ME_IN_PRODUCTION".to_string(),
            },
            audio: AudioConfig {
                max_bitrate: 128000,
                default_bitrate: 32000,
            },
            limits: LimitsConfig {
                max_channels_per_org: 100,
                max_users_per_org: 1000,
                max_users_per_channel: 50,
            },
        }
    }
}
