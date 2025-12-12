use crate::types::{ChannelId, UserId, UserInfo, ControlMessage};
use std::collections::HashMap;
use tokio::sync::{RwLock, mpsc};
use tracing::{debug, info, warn};

/// WebSocket sender for a user
pub type WsSender = mpsc::UnboundedSender<ControlMessage>;

/// Channel state tracking users
#[derive(Debug, Clone)]
pub struct ChannelState {
    pub id: ChannelId,
    pub name: String,
    pub users: Vec<UserInfo>,
}

impl ChannelState {
    pub fn new(id: ChannelId, name: String) -> Self {
        Self {
            id,
            name,
            users: Vec::new(),
        }
    }
}

/// Manages channel membership and WebSocket broadcasting
pub struct ChannelManager {
    /// Map of channel_id -> ChannelState
    channels: RwLock<HashMap<ChannelId, ChannelState>>,
    
    /// Map of user_id -> WebSocket sender (for broadcasting)
    user_sockets: RwLock<HashMap<UserId, WsSender>>,
    
    /// Map of user_id -> username
    user_names: RwLock<HashMap<UserId, String>>,
}

impl ChannelManager {
    pub fn new() -> Self {
        Self {
            channels: RwLock::new(HashMap::new()),
            user_sockets: RwLock::new(HashMap::new()),
            user_names: RwLock::new(HashMap::new()),
        }
    }
    
    /// Register a user's WebSocket connection
    pub async fn register_user(&self, user_id: UserId, username: String, sender: WsSender) {
        let mut sockets = self.user_sockets.write().await;
        let mut names = self.user_names.write().await;
        
        sockets.insert(user_id, sender);
        names.insert(user_id, username.clone());
        
        info!("✅ Registered user {} (ID: {}) for WebSocket broadcasts", username, user_id);
    }
    
    /// Unregister a user's WebSocket connection
    pub async fn unregister_user(&self, user_id: UserId) {
        let mut sockets = self.user_sockets.write().await;
        let names = self.user_names.read().await;
        
        if let Some(username) = names.get(&user_id) {
            info!("🔌 Unregistering user {} (ID: {})", username, user_id);
        }
        
        sockets.remove(&user_id);
        
        // Remove user from all channels
        let mut channels = self.channels.write().await;
        for channel in channels.values_mut() {
            channel.users.retain(|u| u.id != user_id);
        }
    }
    
    /// Add a user to a channel
    pub async fn join_channel(&self, channel_id: ChannelId, user: UserInfo) -> Vec<UserInfo> {
        let mut channels = self.channels.write().await;
        
        // Get or create channel
        let channel = channels.entry(channel_id).or_insert_with(|| {
            ChannelState::new(channel_id, format!("Channel {}", channel_id))
        });
        
        // Add user if not already present
        if !channel.users.iter().any(|u| u.id == user.id) {
            info!("👤 User {} (ID: {}) joining channel {}", user.name, user.id, channel_id);
            channel.users.push(user.clone());
        }
        
        // Return current user list
        channel.users.clone()
    }
    
    /// Remove a user from a channel
    pub async fn leave_channel(&self, channel_id: ChannelId, user_id: UserId) {
        let mut channels = self.channels.write().await;
        
        if let Some(channel) = channels.get_mut(&channel_id) {
            channel.users.retain(|u| u.id != user_id);
            info!("👋 User ID {} left channel {}", user_id, channel_id);
        }
    }
    
    /// Get all users in a channel
    pub async fn get_channel_users(&self, channel_id: ChannelId) -> Vec<UserInfo> {
        let channels = self.channels.read().await;
        
        channels
            .get(&channel_id)
            .map(|ch| ch.users.clone())
            .unwrap_or_default()
    }
    
    /// Get channel name
    pub async fn get_channel_name(&self, channel_id: ChannelId) -> String {
        let channels = self.channels.read().await;
        
        channels
            .get(&channel_id)
            .map(|ch| ch.name.clone())
            .unwrap_or_else(|| format!("Channel {}", channel_id))
    }
    
    /// Broadcast a message to all users in a channel (optionally excluding one user)
    pub async fn broadcast_to_channel(
        &self,
        channel_id: ChannelId,
        message: &ControlMessage,
        exclude_user: Option<UserId>,
    ) {
        let channels = self.channels.read().await;
        let sockets = self.user_sockets.read().await;
        
        if let Some(channel) = channels.get(&channel_id) {
            debug!("📢 Broadcasting to channel {} ({} users)", channel_id, channel.users.len());
            
            for user in &channel.users {
                // Skip excluded user (e.g., the sender)
                if Some(user.id) == exclude_user {
                    continue;
                }
                
                // Send message if user has active WebSocket
                if let Some(sender) = sockets.get(&user.id) {
                    if let Err(e) = sender.send(message.clone()) {
                        warn!("Failed to send message to user {}: {:?}", user.id, e);
                    }
                } else {
                    debug!("User {} not connected (no WebSocket sender)", user.id);
                }
            }
        }
    }
    
    /// Send a message to a specific user
    pub async fn send_to_user(&self, user_id: UserId, message: &ControlMessage) {
        let sockets = self.user_sockets.read().await;
        
        if let Some(sender) = sockets.get(&user_id) {
            if let Err(e) = sender.send(message.clone()) {
                warn!("Failed to send message to user {}: {:?}", user_id, e);
            }
        }
    }
    
    /// Get all channels and their user counts (for listing)
    pub async fn get_all_channels(&self) -> Vec<(ChannelId, String, usize)> {
        let channels = self.channels.read().await;
        
        channels
            .iter()
            .map(|(id, state)| (*id, state.name.clone(), state.users.len()))
            .collect()
    }
}

impl Default for ChannelManager {
    fn default() -> Self {
        Self::new()
    }
}
