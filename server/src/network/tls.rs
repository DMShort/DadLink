use crate::error::{Result, VoipError};
use crate::types::{ControlMessage, Session, UserInfo, UserId, RoleId};
use crate::auth;
use crate::channel_manager::ChannelManager;
use crate::{
    db,
    permissions::PermissionChecker,
    routing,
    api::admin,
};
use axum::{
    extract::{
        ws::{WebSocket, Message as WsMessage},
        WebSocketUpgrade, State,
    },
    response::Response,
    routing::get,
    Router,
};
use std::collections::HashMap;
use std::sync::Arc;
use tokio::sync::{mpsc, RwLock};
use tracing::{debug, info, warn, error};
use serde_json;

/// Shared server state with database repositories
#[derive(Clone)]
pub struct ServerState {
    pub jwt_secret: String,
    pub channel_manager: Arc<ChannelManager>,
    pub user_repo: Arc<db::UserRepository>,
    pub channel_repo: Arc<db::ChannelRepository>,
    pub org_repo: Arc<db::OrganizationRepository>,
    pub role_repo: Arc<db::RoleRepository>,
    pub voice_router: Arc<routing::VoiceRouter>,
    pub database: db::Database,
    pub permission_checker: Arc<PermissionChecker>,
    pub srtp_sessions: Arc<crate::crypto::SrtpSessionManager>,
    pub pending_key_exchanges: Arc<RwLock<HashMap<UserId, crate::crypto::KeyExchange>>>,
}

/// WebSocket control handler with admin API routes
pub fn control_routes() -> Router<Arc<ServerState>> {
    use axum::routing::post;
    
    Router::new()
        .route("/control", get(websocket_handler))
        
        // Admin API - Organizations
        .route("/api/admin/organizations", get(admin::organizations::list_organizations))
        .route("/api/admin/organizations/:id", get(admin::organizations::get_organization))
        
        // Admin API - Users
        .route("/api/admin/users", get(admin::users::list_users)
            .post(admin::users::create_user))
        .route("/api/admin/users/:id", get(admin::users::get_user)
            .put(admin::users::update_user)
            .delete(admin::users::delete_user))
        .route("/api/admin/users/:id/ban", post(admin::users::ban_user))
        .route("/api/admin/users/:id/unban", post(admin::users::unban_user))
        .route("/api/admin/users/:id/reset-password", post(admin::users::reset_password))
        
        // Admin API - Metrics
        .route("/api/admin/metrics/system", get(admin::metrics::system_metrics))
        .route("/api/admin/metrics/users", get(admin::metrics::user_metrics))
        .route("/api/admin/metrics/voice", get(admin::metrics::voice_metrics))
        
        // Admin API - Audit Log
        .route("/api/admin/audit-log", get(admin::audit_log::list_audit_logs))
        .route("/api/admin/audit-log/search", post(admin::audit_log::search_audit_logs))
        
        // Admin API - Channels
        .route("/api/admin/channels", get(admin::channels::list_channels)
            .post(admin::channels::create_channel))
        .route("/api/admin/channels/:id", get(admin::channels::get_channel)
            .put(admin::channels::update_channel)
            .delete(admin::channels::delete_channel))
        
        // Admin API - Roles
        .route("/api/admin/roles", get(admin::roles::list_roles)
            .post(admin::roles::create_role))
        .route("/api/admin/roles/:id", get(admin::roles::get_role)
            .put(admin::roles::update_role)
            .delete(admin::roles::delete_role))
}

/// WebSocket upgrade handler
async fn websocket_handler(
    ws: WebSocketUpgrade,
    State(state): State<Arc<ServerState>>,
) -> Response {
    ws.on_upgrade(move |socket| handle_socket(socket, state))
}

/// Handle individual WebSocket connection
async fn handle_socket(mut socket: WebSocket, state: Arc<ServerState>) {
    info!("New WebSocket connection");
    
    // Create channel for outgoing messages (for broadcasting)
    let (tx, mut rx) = mpsc::unbounded_channel::<ControlMessage>();
    
    // Send challenge
    let challenge = ControlMessage::Challenge {
        methods: vec!["password".to_string(), "token".to_string()],
        server_version: env!("CARGO_PKG_VERSION").to_string(),
    };
    
    if let Err(e) = send_message(&mut socket, &challenge).await {
        warn!("Failed to send challenge: {:?}", e);
        return;
    }
    
    // Main message loop
    let mut authenticated = false;
    let mut session: Option<Session> = None;
    
    loop {
        tokio::select! {
            // Handle incoming messages from client
            msg = socket.recv() => {
                match msg {
                    Some(Ok(WsMessage::Text(text))) => {
                        match serde_json::from_str::<ControlMessage>(&text) {
                            Ok(control_msg) => {
                                match handle_control_message(
                                    control_msg,
                                    &mut socket,
                                    &state,
                                    &mut authenticated,
                                    &mut session,
                                    &tx,
                                ).await {
                                    Ok(should_continue) => {
                                        if !should_continue {
                                            break;
                                        }
                                    }
                                    Err(e) => {
                                        error!("Error handling message: {:?}", e);
                                        let error_msg = ControlMessage::Error {
                                            code: "internal_error".to_string(),
                                            message: e.to_string(),
                                        };
                                        let _ = send_message(&mut socket, &error_msg).await;
                                    }
                                }
                            }
                            Err(e) => {
                                warn!("Invalid JSON: {:?}", e);
                                let error_msg = ControlMessage::Error {
                                    code: "invalid_json".to_string(),
                                    message: "Failed to parse message".to_string(),
                                };
                                let _ = send_message(&mut socket, &error_msg).await;
                            }
                        }
                    }
                    Some(Ok(WsMessage::Close(_))) => {
                        info!("Client closed connection");
                        break;
                    }
                    Some(Ok(WsMessage::Ping(data))) => {
                        let _ = socket.send(WsMessage::Pong(data)).await;
                    }
                    Some(Ok(_)) => {
                        // Ignore binary messages, pongs, etc.
                    }
                    Some(Err(e)) => {
                        error!("WebSocket error: {:?}", e);
                        break;
                    }
                    None => {
                        info!("WebSocket connection closed by client");
                        break;
                    }
                }
            }
            
            // Handle outgoing broadcast messages
            Some(msg) = rx.recv() => {
                if let Err(e) = send_message(&mut socket, &msg).await {
                    warn!("Failed to send broadcast message: {:?}", e);
                    break;
                }
            }
        }
    }
    
    // Cleanup on disconnect
    if let Some(sess) = &session {
        info!("🔌 User {} (ID: {}) disconnected", sess.username, sess.user_id);

        // Unregister from channel manager
        state.channel_manager.unregister_user(sess.user_id).await;

        // Remove SRTP session (cleanup encryption keys)
        state.srtp_sessions.remove_session(sess.user_id).await;

        // Remove pending key exchange if any
        {
            let mut pending = state.pending_key_exchanges.write().await;
            pending.remove(&sess.user_id);
        }

        // Notify all channels the user was in
        for channel_id in &sess.channels {
            let msg = ControlMessage::UserLeft {
                channel_id: *channel_id,
                user_id: sess.user_id,
            };
            state.channel_manager.broadcast_to_channel(*channel_id, &msg, None).await;
        }
    }
    
    info!("WebSocket connection closed");
}

/// Handle individual control message
async fn handle_control_message(
    msg: ControlMessage,
    socket: &mut WebSocket,
    state: &Arc<ServerState>,
    authenticated: &mut bool,
    session: &mut Option<Session>,
    tx: &mpsc::UnboundedSender<ControlMessage>,
) -> Result<bool> {
    match msg {
        ControlMessage::Hello { version, client } => {
            debug!("Client hello: {} v{}", client, version);
            Ok(true)
        }
        
        ControlMessage::Register { username, password, email } => {
            info!("Registration attempt for username: {}", username);

            // Use database repository for registration
            match state.user_repo.create_user(1, &username, email.as_deref(), &password).await {
                Ok(user_id) => {
                    info!("✅ User {} registered successfully (ID: {})", username, user_id);

                    let response = ControlMessage::RegisterResult {
                        success: true,
                        user_id: Some(user_id),
                        message: format!("Registration successful! You can now login as '{}'", username),
                    };
                    send_message(socket, &response).await?;
                }
                Err(e) => {
                    warn!("Registration failed for {}: {}", username, e);
                    let response = ControlMessage::RegisterResult {
                        success: false,
                        user_id: None,
                        message: format!("Registration failed: {}", e),
                    };
                    send_message(socket, &response).await?;
                }
            }
            Ok(true)
        }
        
        ControlMessage::Authenticate { method, username, password, token } => {
            if *authenticated {
                let response = ControlMessage::Error {
                    code: "already_authenticated".to_string(),
                    message: "Already authenticated".to_string(),
                };
                send_message(socket, &response).await?;
                return Ok(true);
            }
            
            // Handle authentication
            match method.as_str() {
                "password" => {
                    if let (Some(user), Some(pass)) = (username, password) {
                        // Authenticate against user repository
                        match state.user_repo.authenticate(&user, &pass).await {
                            Ok(Some(authenticated_user)) => {
                                // Get user roles and calculate permissions
                                let user_roles = state.role_repo.get_user_roles(authenticated_user.id).await
                                    .unwrap_or_else(|_| vec![]);
                                let role_ids: Vec<RoleId> = user_roles.iter().map(|r| r.id).collect();
                                
                                // Calculate aggregate permissions (bitwise OR of all role permissions)
                                let mut total_permissions: u32 = 0;
                                for role in &user_roles {
                                    total_permissions |= role.permissions.bits();
                                }
                                
                                let token = auth::create_jwt(
                                    authenticated_user.id,
                                    authenticated_user.org_id,
                                    role_ids.clone(),
                                    &state.jwt_secret,
                                    1, // 1 hour
                                )?;

                                *authenticated = true;
                                *session = Some(Session {
                                    user_id: authenticated_user.id,
                                    org_id: authenticated_user.org_id,
                                    username: authenticated_user.username.clone(),
                                    roles: role_ids,
                                    channels: vec![],
                                    last_sequence: 0,
                                });

                                // Register user with channel manager for broadcasts
                                state.channel_manager.register_user(
                                    authenticated_user.id,
                                    authenticated_user.username.clone(),
                                    tx.clone()
                                ).await;

                                info!("✅ User {} (ID: {}) authenticated with permissions: 0x{:x}",
                                      authenticated_user.username, authenticated_user.id, total_permissions);

                                let response = ControlMessage::AuthResult {
                                    success: true,
                                    user_id: Some(authenticated_user.id),
                                    org_id: Some(authenticated_user.org_id),
                                    permissions: Some(total_permissions),
                                    session_token: Some(token),
                                    voice_key: Some("demo_voice_key".to_string()),
                                    message: Some("Authentication successful".to_string()),
                                };
                                send_message(socket, &response).await?;

                                // Initiate key exchange for voice encryption
                                let kx = crate::crypto::KeyExchange::new();
                                let server_public_key = kx.public_key_bytes();

                                // Store key exchange object for when client responds
                                {
                                    let mut pending = state.pending_key_exchanges.write().await;
                                    pending.insert(authenticated_user.id, kx);
                                }

                                let key_exchange_msg = ControlMessage::KeyExchangeInit {
                                    public_key: server_public_key,
                                };
                                send_message(socket, &key_exchange_msg).await?;
                                info!("🔑 Sent key exchange init to user {}", authenticated_user.id);
                            }
                            Ok(None) => {
                                warn!("Failed login attempt for user: {}", user);
                                let response = ControlMessage::AuthResult {
                                    success: false,
                                    user_id: None,
                                    org_id: None,
                                    permissions: None,
                                    session_token: None,
                                    voice_key: None,
                                    message: Some("Invalid credentials".to_string()),
                                };
                                send_message(socket, &response).await?;
                            }
                            Err(e) => {
                                error!("Authentication error for user {}: {}", user, e);
                                let response = ControlMessage::AuthResult {
                                    success: false,
                                    user_id: None,
                                    org_id: None,
                                    permissions: None,
                                    session_token: None,
                                    voice_key: None,
                                    message: Some("Authentication error".to_string()),
                                };
                                send_message(socket, &response).await?;
                            }
                        }
                    } else {
                        let response = ControlMessage::Error {
                            code: "missing_credentials".to_string(),
                            message: "Username and password required".to_string(),
                        };
                        send_message(socket, &response).await?;
                    }
                }
                "token" => {
                    if let Some(tok) = token {
                        match auth::verify_jwt(&tok, &state.jwt_secret) {
                            Ok(claims) => {
                                // Look up user by ID from repository
                                let username = match state.user_repo.get_user_by_id(claims.sub).await {
                                    Ok(Some(user)) => user.username,
                                    _ => format!("user_{}", claims.sub),
                                };

                                // Calculate permissions from roles
                                let mut total_permissions: u32 = 0;
                                for role_id in &claims.roles {
                                    if let Ok(Some(role)) = state.role_repo.get_role(*role_id).await {
                                        total_permissions |= role.permissions.bits();
                                    }
                                }

                                *authenticated = true;
                                *session = Some(Session {
                                    user_id: claims.sub,
                                    org_id: claims.org,
                                    username: username.clone(),
                                    roles: claims.roles.clone(),
                                    channels: vec![],
                                    last_sequence: 0,
                                });

                                // Register user with channel manager for broadcasts
                                state.channel_manager.register_user(claims.sub, username.clone(), tx.clone()).await;
                                info!("✅ User {} (ID: {}) authenticated via token with permissions: 0x{:x}", 
                                      username, claims.sub, total_permissions);

                                let response = ControlMessage::AuthResult {
                                    success: true,
                                    user_id: Some(claims.sub),
                                    org_id: Some(claims.org),
                                    permissions: Some(total_permissions),
                                    session_token: Some(tok),
                                    voice_key: Some("demo_voice_key".to_string()),
                                    message: Some("Token validated".to_string()),
                                };
                                send_message(socket, &response).await?;

                                // Initiate key exchange for voice encryption
                                let kx = crate::crypto::KeyExchange::new();
                                let server_public_key = kx.public_key_bytes();

                                // Store key exchange object for when client responds
                                {
                                    let mut pending = state.pending_key_exchanges.write().await;
                                    pending.insert(claims.sub, kx);
                                }

                                let key_exchange_msg = ControlMessage::KeyExchangeInit {
                                    public_key: server_public_key,
                                };
                                send_message(socket, &key_exchange_msg).await?;
                                info!("🔑 Sent key exchange init to user {}", claims.sub);
                            }
                            Err(_) => {
                                let response = ControlMessage::AuthResult {
                                    success: false,
                                    user_id: None,
                                    org_id: None,
                                    permissions: None,
                                    session_token: None,
                                    voice_key: None,
                                    message: Some("Invalid token".to_string()),
                                };
                                send_message(socket, &response).await?;
                            }
                        }
                    }
                }
                _ => {
                    let response = ControlMessage::Error {
                        code: "unsupported_method".to_string(),
                        message: format!("Unsupported auth method: {}", method),
                    };
                    send_message(socket, &response).await?;
                }
            }
            
            Ok(true)
        }
        
        ControlMessage::JoinChannel { channel_id, password } => {
            if !*authenticated {
                let response = ControlMessage::Error {
                    code: "not_authenticated".to_string(),
                    message: "Must authenticate first".to_string(),
                };
                send_message(socket, &response).await?;
                return Ok(true);
            }

            // Get session info
            let sess = match session {
                Some(ref mut s) => s,
                None => {
                    let response = ControlMessage::Error {
                        code: "no_session".to_string(),
                        message: "No session found".to_string(),
                    };
                    send_message(socket, &response).await?;
                    return Ok(true);
                }
            };

            // Check channel password if provided
            match state.permission_checker.verify_channel_password(channel_id, password.as_deref()).await {
                Ok(true) => {
                    // Password correct or no password required
                }
                Ok(false) => {
                    warn!("User {} (ID: {}) provided incorrect password for channel {}",
                          sess.username, sess.user_id, channel_id);
                    let response = ControlMessage::Error {
                        code: "invalid_password".to_string(),
                        message: "Incorrect channel password".to_string(),
                    };
                    send_message(socket, &response).await?;
                    return Ok(true);
                }
                Err(e) => {
                    error!("Error verifying channel password: {:?}", e);
                    let response = ControlMessage::Error {
                        code: "internal_error".to_string(),
                        message: "Failed to verify channel password".to_string(),
                    };
                    send_message(socket, &response).await?;
                    return Ok(true);
                }
            }

            // Check JOIN permission
            match state.permission_checker.can_join_channel(sess.user_id, sess.org_id, channel_id).await {
                Ok(true) => {
                    // Permission granted
                }
                Ok(false) => {
                    warn!("User {} (ID: {}) denied permission to join channel {}",
                          sess.username, sess.user_id, channel_id);
                    let response = ControlMessage::Error {
                        code: "permission_denied".to_string(),
                        message: "You don't have permission to join this channel".to_string(),
                    };
                    send_message(socket, &response).await?;
                    return Ok(true);
                }
                Err(e) => {
                    error!("Error checking join permission: {:?}", e);
                    let response = ControlMessage::Error {
                        code: "internal_error".to_string(),
                        message: "Failed to check permissions".to_string(),
                    };
                    send_message(socket, &response).await?;
                    return Ok(true);
                }
            }

            info!("User {} (ID: {}) joining channel {}", sess.username, sess.user_id, channel_id);
            
            // Create user info
            let user_info = UserInfo {
                id: sess.user_id,
                name: sess.username.clone(),
                speaking: false,
            };
            
            // Add user to channel and get user list
            let users = state.channel_manager.join_channel(channel_id, user_info.clone()).await;
            let channel_name = state.channel_manager.get_channel_name(channel_id).await;
            
            // Update session
            if !sess.channels.contains(&channel_id) {
                sess.channels.push(channel_id);
            }
            
            // Send response to joining user with current user list
            let response = ControlMessage::ChannelJoined {
                channel_id,
                channel_name: channel_name.clone(),
                users: users.clone(),
            };
            send_message(socket, &response).await?;
            info!("✅ User {} joined channel {} (now has {} users)", 
                  sess.username, channel_id, users.len());
            
            // Broadcast to OTHER users in the channel
            let notification = ControlMessage::ChannelState {
                channel_id,
                event: "user_joined".to_string(),
                user: Some(user_info),
            };
            state.channel_manager.broadcast_to_channel(
                channel_id,
                &notification,
                Some(sess.user_id),  // Exclude the joining user
            ).await;
            
            Ok(true)
        }
        
        ControlMessage::LeaveChannel { channel_id } => {
            if let Some(ref mut sess) = session {
                info!("User {} (ID: {}) leaving channel {}", sess.username, sess.user_id, channel_id);

                // Remove from channel manager
                state.channel_manager.leave_channel(channel_id, sess.user_id).await;

                // Update session
                sess.channels.retain(|&c| c != channel_id);

                // Broadcast to other users
                let notification = ControlMessage::UserLeft {
                    channel_id,
                    user_id: sess.user_id,
                };
                state.channel_manager.broadcast_to_channel(channel_id, &notification, None).await;
            }
            Ok(true)
        }

        ControlMessage::KeyExchangeResponse { public_key } => {
            if let Some(ref sess) = session {
                info!("🔑 Received key exchange response from user {}", sess.user_id);

                // Retrieve pending key exchange
                let kx = {
                    let mut pending = state.pending_key_exchanges.write().await;
                    pending.remove(&sess.user_id)
                };

                match kx {
                    Some(key_exchange) => {
                        // Complete key exchange and derive SRTP keys
                        match key_exchange.derive_keys(&public_key) {
                            Ok(key_material) => {
                                // Create SRTP session with derived keys
                                match crate::crypto::SrtpSession::new(
                                    &key_material.master_key,
                                    &key_material.salt,
                                    sess.user_id as u64,
                                ) {
                                    Ok(srtp_session) => {
                                        // Store SRTP session
                                        state.srtp_sessions.set_session(sess.user_id, srtp_session).await;
                                        info!("✅ SRTP session established for user {}", sess.user_id);
                                    }
                                    Err(e) => {
                                        error!("Failed to create SRTP session for user {}: {}", sess.user_id, e);
                                        let response = ControlMessage::Error {
                                            code: "crypto_error".to_string(),
                                            message: "Failed to create encryption session".to_string(),
                                        };
                                        send_message(socket, &response).await?;
                                    }
                                }
                            }
                            Err(e) => {
                                error!("Key derivation failed for user {}: {}", sess.user_id, e);
                                let response = ControlMessage::Error {
                                    code: "crypto_error".to_string(),
                                    message: "Key exchange failed".to_string(),
                                };
                                send_message(socket, &response).await?;
                            }
                        }
                    }
                    None => {
                        warn!("No pending key exchange found for user {}", sess.user_id);
                        let response = ControlMessage::Error {
                            code: "invalid_state".to_string(),
                            message: "No pending key exchange".to_string(),
                        };
                        send_message(socket, &response).await?;
                    }
                }
            } else {
                let response = ControlMessage::Error {
                    code: "not_authenticated".to_string(),
                    message: "Must authenticate first".to_string(),
                };
                send_message(socket, &response).await?;
            }
            Ok(true)
        }

        ControlMessage::SetTransmitChannel { channel_id } => {
            if let Some(ref sess) = session {
                info!("User {} (ID: {}) setting transmit channel to {}", 
                      sess.username, sess.user_id, channel_id);
                
                // Verify user is in the channel
                if !sess.channels.contains(&channel_id) {
                    let response = ControlMessage::Error {
                        code: "not_in_channel".to_string(),
                        message: format!("You must join channel {} before transmitting to it", channel_id),
                    };
                    send_message(socket, &response).await?;
                    return Ok(true);
                }
                
                // TODO: Set transmit channel in VoiceRouter
                // For now, just confirm
                info!("✅ User {} set transmit channel to {}", sess.user_id, channel_id);
                
                // Send confirmation (optional - could add a new response type)
                let response = ControlMessage::Pong {
                    timestamp: chrono::Utc::now().timestamp(),
                    server_time: chrono::Utc::now().timestamp(),
                };
                send_message(socket, &response).await?;
            } else {
                let response = ControlMessage::Error {
                    code: "not_authenticated".to_string(),
                    message: "Must authenticate first".to_string(),
                };
                send_message(socket, &response).await?;
            }
            Ok(true)
        }
        
        ControlMessage::Ping { timestamp } => {
            let response = ControlMessage::Pong {
                timestamp,
                server_time: chrono::Utc::now().timestamp(),
            };
            send_message(socket, &response).await?;
            Ok(true)
        }

        // Role Management
        ControlMessage::AssignRole { user_id, role_id } => {
            if let Some(ref sess) = session {
                // Check if user has MANAGE permission (admin only)
                // For now, allow all authenticated users (TODO: add admin check)
                match state.role_repo.assign_role_to_user(user_id, role_id).await {
                    Ok(_) => {
                        info!("User {} assigned role {} to user {}", sess.user_id, role_id, user_id);
                        let response = ControlMessage::RoleOperationResult {
                            success: true,
                            message: format!("Role {} assigned to user {}", role_id, user_id),
                        };
                        send_message(socket, &response).await?;
                    }
                    Err(e) => {
                        error!("Failed to assign role: {:?}", e);
                        let response = ControlMessage::RoleOperationResult {
                            success: false,
                            message: format!("Failed to assign role: {}", e),
                        };
                        send_message(socket, &response).await?;
                    }
                }
            } else {
                let response = ControlMessage::Error {
                    code: "not_authenticated".to_string(),
                    message: "Must authenticate first".to_string(),
                };
                send_message(socket, &response).await?;
            }
            Ok(true)
        }

        ControlMessage::RemoveRole { user_id, role_id } => {
            if let Some(ref sess) = session {
                match state.role_repo.remove_role_from_user(user_id, role_id).await {
                    Ok(_) => {
                        info!("User {} removed role {} from user {}", sess.user_id, role_id, user_id);
                        let response = ControlMessage::RoleOperationResult {
                            success: true,
                            message: format!("Role {} removed from user {}", role_id, user_id),
                        };
                        send_message(socket, &response).await?;
                    }
                    Err(e) => {
                        error!("Failed to remove role: {:?}", e);
                        let response = ControlMessage::RoleOperationResult {
                            success: false,
                            message: format!("Failed to remove role: {}", e),
                        };
                        send_message(socket, &response).await?;
                    }
                }
            } else {
                let response = ControlMessage::Error {
                    code: "not_authenticated".to_string(),
                    message: "Must authenticate first".to_string(),
                };
                send_message(socket, &response).await?;
            }
            Ok(true)
        }

        ControlMessage::ListRoles { org_id } => {
            if let Some(_sess) = session {
                match state.role_repo.get_roles_by_org(org_id).await {
                    Ok(roles) => {
                        let response = ControlMessage::RolesList { roles };
                        send_message(socket, &response).await?;
                    }
                    Err(e) => {
                        error!("Failed to list roles: {:?}", e);
                        let response = ControlMessage::Error {
                            code: "internal_error".to_string(),
                            message: "Failed to list roles".to_string(),
                        };
                        send_message(socket, &response).await?;
                    }
                }
            } else {
                let response = ControlMessage::Error {
                    code: "not_authenticated".to_string(),
                    message: "Must authenticate first".to_string(),
                };
                send_message(socket, &response).await?;
            }
            Ok(true)
        }

        ControlMessage::GetUserRoles { user_id } => {
            if let Some(_sess) = session {
                match state.role_repo.get_user_roles(user_id).await {
                    Ok(roles) => {
                        let response = ControlMessage::UserRolesList { user_id, roles };
                        send_message(socket, &response).await?;
                    }
                    Err(e) => {
                        error!("Failed to get user roles: {:?}", e);
                        let response = ControlMessage::Error {
                            code: "internal_error".to_string(),
                            message: "Failed to get user roles".to_string(),
                        };
                        send_message(socket, &response).await?;
                    }
                }
            } else {
                let response = ControlMessage::Error {
                    code: "not_authenticated".to_string(),
                    message: "Must authenticate first".to_string(),
                };
                send_message(socket, &response).await?;
            }
            Ok(true)
        }

        ControlMessage::SetChannelAcl { channel_id, role_id, permissions } => {
            if let Some(ref sess) = session {
                let perms = crate::types::Permissions::from_bits_truncate(permissions);
                match state.role_repo.set_channel_acl(channel_id, role_id, perms).await {
                    Ok(_) => {
                        info!("User {} set ACL for channel {} role {} to {:?}",
                              sess.user_id, channel_id, role_id, perms);
                        let response = ControlMessage::RoleOperationResult {
                            success: true,
                            message: format!("ACL set for channel {} role {}", channel_id, role_id),
                        };
                        send_message(socket, &response).await?;
                    }
                    Err(e) => {
                        error!("Failed to set channel ACL: {:?}", e);
                        let response = ControlMessage::RoleOperationResult {
                            success: false,
                            message: format!("Failed to set ACL: {}", e),
                        };
                        send_message(socket, &response).await?;
                    }
                }
            } else {
                let response = ControlMessage::Error {
                    code: "not_authenticated".to_string(),
                    message: "Must authenticate first".to_string(),
                };
                send_message(socket, &response).await?;
            }
            Ok(true)
        }

        _ => {
            // Server-to-client messages shouldn't be received
            let response = ControlMessage::Error {
                code: "invalid_message".to_string(),
                message: "Unexpected message type".to_string(),
            };
            send_message(socket, &response).await?;
            Ok(true)
        }
    }
}

/// Send control message
async fn send_message(socket: &mut WebSocket, msg: &ControlMessage) -> Result<()> {
    let json = serde_json::to_string(msg)
        .map_err(|e| VoipError::Other(format!("JSON serialization failed: {}", e)))?;
    
    socket.send(WsMessage::Text(json))
        .await
        .map_err(|e| VoipError::Other(format!("WebSocket send failed: {}", e)))?;
    
    Ok(())
}
