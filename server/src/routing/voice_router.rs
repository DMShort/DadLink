use crate::error::{Result, VoipError};
use crate::types::{ChannelId, UserId};
use std::collections::{HashMap, HashSet};
use std::net::SocketAddr;
use std::sync::Arc;
use tokio::sync::RwLock;
use tracing::{info, debug};

/// User session in a channel
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct UserSession {
    pub user_id: UserId,
    pub addr: SocketAddr,
}

/// Voice packet router - manages channel membership and packet routing
/// MULTI-CHANNEL SUPPORT: Users can listen to multiple channels simultaneously
pub struct VoiceRouter {
    /// Map: ChannelId -> Set of UserSessions (users listening to this channel)
    channels: Arc<RwLock<HashMap<ChannelId, HashSet<UserSession>>>>,
    
    /// Map: SocketAddr -> UserId (basic user lookup)
    addr_to_user: Arc<RwLock<HashMap<SocketAddr, UserId>>>,
    
    /// Map: SocketAddr -> Set of ChannelIds (all channels user is listening to)
    user_channels: Arc<RwLock<HashMap<SocketAddr, HashSet<ChannelId>>>>,
    
    /// Map: SocketAddr -> ChannelId (which channel user transmits to)
    user_transmit_channel: Arc<RwLock<HashMap<SocketAddr, ChannelId>>>,
}

impl VoiceRouter {
    /// Create a new voice router
    pub fn new() -> Self {
        Self {
            channels: Arc::new(RwLock::new(HashMap::new())),
            addr_to_user: Arc::new(RwLock::new(HashMap::new())),
            user_channels: Arc::new(RwLock::new(HashMap::new())),
            user_transmit_channel: Arc::new(RwLock::new(HashMap::new())),
        }
    }
    
    /// Add a user to a channel (MULTI-CHANNEL: doesn't leave other channels)
    /// This is the NEW multi-channel method
    pub async fn join_channel(
        &self,
        channel_id: ChannelId,
        user_id: UserId,
        addr: SocketAddr,
    ) -> Result<()> {
        let session = UserSession { user_id, addr };
        
        // Add to channel's user list
        {
            let mut channels = self.channels.write().await;
            channels
                .entry(channel_id)
                .or_insert_with(HashSet::new)
                .insert(session.clone());
        }
        
        // Track user ID
        {
            let mut addr_map = self.addr_to_user.write().await;
            addr_map.insert(addr, user_id);
        }
        
        // Add to user's listening channels
        {
            let mut user_chans = self.user_channels.write().await;
            user_chans
                .entry(addr)
                .or_insert_with(HashSet::new)
                .insert(channel_id);
        }
        
        // If this is user's first channel, set it as transmit channel
        {
            let transmit_chans = self.user_transmit_channel.read().await;
            if !transmit_chans.contains_key(&addr) {
                drop(transmit_chans);  // Release read lock
                let mut transmit_chans = self.user_transmit_channel.write().await;
                transmit_chans.insert(addr, channel_id);
                info!(
                    "👤 User {} joined channel {} from {} (set as transmit channel)",
                    user_id, channel_id, addr
                );
            } else {
                info!(
                    "👤 User {} joined channel {} from {} (listening only)",
                    user_id, channel_id, addr
                );
            }
        }
        
        Ok(())
    }
    
    /// Remove user from a specific channel (MULTI-CHANNEL)
    pub async fn leave_channel(
        &self,
        channel_id: ChannelId,
        addr: SocketAddr,
    ) -> Result<()> {
        let user_id = {
            let addr_map = self.addr_to_user.read().await;
            addr_map.get(&addr).copied()
        };
        
        // Remove from channel's user list
        {
            let mut channels = self.channels.write().await;
            if let Some(users) = channels.get_mut(&channel_id) {
                users.retain(|s| s.addr != addr);
                
                // Remove empty channels
                if users.is_empty() {
                    channels.remove(&channel_id);
                }
            }
        }
        
        // Remove from user's listening channels
        {
            let mut user_chans = self.user_channels.write().await;
            if let Some(channels) = user_chans.get_mut(&addr) {
                channels.remove(&channel_id);
                
                // If user has no channels left, clean up
                if channels.is_empty() {
                    user_chans.remove(&addr);
                }
            }
        }
        
        // If this was the transmit channel, switch to another channel or clear
        {
            let mut transmit_chans = self.user_transmit_channel.write().await;
            if transmit_chans.get(&addr) == Some(&channel_id) {
                // Try to find another channel to transmit to
                let user_chans = self.user_channels.read().await;
                if let Some(channels) = user_chans.get(&addr) {
                    if let Some(&new_channel) = channels.iter().next() {
                        transmit_chans.insert(addr, new_channel);
                        info!(
                            "User {:?} switched transmit to channel {}",
                            user_id, new_channel
                        );
                    } else {
                        transmit_chans.remove(&addr);
                    }
                } else {
                    transmit_chans.remove(&addr);
                }
            }
        }
        
        if let Some(uid) = user_id {
            info!(
                "👋 User {} left channel {} from {}",
                uid, channel_id, addr
            );
        }
        
        Ok(())
    }
    
    /// Remove a user from all channels (cleanup on disconnect)
    pub async fn leave_all_channels(&self, addr: SocketAddr) -> Result<()> {
        // Get all channels user is in
        let channels_to_leave = {
            let user_chans = self.user_channels.read().await;
            user_chans
                .get(&addr)
                .map(|channels| channels.iter().copied().collect::<Vec<_>>())
                .unwrap_or_default()
        };
        
        // Leave each channel
        for channel_id in channels_to_leave {
            self.leave_channel(channel_id, addr).await?;
        }
        
        // Clean up user tracking
        {
            let mut addr_map = self.addr_to_user.write().await;
            addr_map.remove(&addr);
        }
        
        {
            let mut user_chans = self.user_channels.write().await;
            user_chans.remove(&addr);
        }
        
        {
            let mut transmit_chans = self.user_transmit_channel.write().await;
            transmit_chans.remove(&addr);
        }
        
        Ok(())
    }
    
    /// Get all users in a channel (except the sender)
    pub async fn get_channel_users(
        &self,
        channel_id: ChannelId,
        exclude_addr: SocketAddr,
    ) -> Vec<SocketAddr> {
        let channels = self.channels.read().await;
        
        channels
            .get(&channel_id)
            .map(|users| {
                users
                    .iter()
                    .filter(|s| s.addr != exclude_addr)
                    .map(|s| s.addr)
                    .collect()
            })
            .unwrap_or_default()
    }
    
    /// Register UDP address for a user ID (without joining channels)
    /// This allows mapping user IDs from packets to UDP addresses for routing
    pub async fn register_udp_address(&self, user_id: UserId, addr: SocketAddr) {
        let mut addr_map = self.addr_to_user.write().await;
        addr_map.insert(addr, user_id);
    }
    
    /// Get UDP address for a user ID
    pub async fn get_udp_address(&self, user_id: UserId) -> Option<SocketAddr> {
        let addr_map = self.addr_to_user.read().await;
        addr_map.iter()
            .find(|(_, &uid)| uid == user_id)
            .map(|(addr, _)| *addr)
    }
    
    /// Get the channel ID where user transmits (active transmit channel)
    pub async fn get_transmit_channel(&self, addr: SocketAddr) -> Option<ChannelId> {
        let transmit_chans = self.user_transmit_channel.read().await;
        transmit_chans.get(&addr).copied()
    }
    
    /// Get all channels the user is listening to
    pub async fn get_listening_channels(&self, addr: SocketAddr) -> Vec<ChannelId> {
        let user_chans = self.user_channels.read().await;
        user_chans
            .get(&addr)
            .map(|channels| channels.iter().copied().collect())
            .unwrap_or_default()
    }
    
    /// Set which channel the user transmits to
    /// User must already be listening to this channel
    pub async fn set_transmit_channel(
        &self,
        addr: SocketAddr,
        channel_id: ChannelId,
    ) -> Result<()> {
        // Verify user is listening to this channel
        let listening = {
            let user_chans = self.user_channels.read().await;
            user_chans
                .get(&addr)
                .map(|channels| channels.contains(&channel_id))
                .unwrap_or(false)
        };
        
        if !listening {
            return Err(VoipError::InvalidState(
                "User is not listening to target channel".to_string(),
            ));
        }
        
        // Set transmit channel
        {
            let mut transmit_chans = self.user_transmit_channel.write().await;
            transmit_chans.insert(addr, channel_id);
        }
        
        let user_id = {
            let addr_map = self.addr_to_user.read().await;
            addr_map.get(&addr).copied()
        };
        
        info!(
            "🎤 User {:?} set transmit channel to {}",
            user_id, channel_id
        );
        
        Ok(())
    }
    
    /// DEPRECATED: Use get_transmit_channel() instead
    /// Kept for backwards compatibility
    pub async fn get_user_channel(&self, addr: SocketAddr) -> Option<ChannelId> {
        self.get_transmit_channel(addr).await
    }
    
    /// Auto-join user to channel based on packet
    /// This allows users to join just by sending a voice packet
    pub async fn auto_join_from_packet(
        &self,
        channel_id: ChannelId,
        user_id: UserId,
        addr: SocketAddr,
    ) -> Result<()> {
        // Check if user is already in a channel
        let current_channel = self.get_user_channel(addr).await;
        
        match current_channel {
            Some(current) if current == channel_id => {
                // Already in the correct channel, do nothing
                Ok(())
            }
            Some(current) => {
                // In a different channel, switch
                debug!(
                    "User {} switching from channel {} to {}",
                    user_id, current, channel_id
                );
                self.join_channel(channel_id, user_id, addr).await
            }
            None => {
                // Not in any channel, join
                self.join_channel(channel_id, user_id, addr).await
            }
        }
    }
    
    /// Get statistics
    pub async fn get_stats(&self) -> RouterStats {
        let channels = self.channels.read().await;
        
        let total_channels = channels.len();
        let total_users: usize = channels.values().map(|users| users.len()).sum();
        
        let channels_info: Vec<_> = channels
            .iter()
            .map(|(id, users)| (*id, users.len()))
            .collect();
        
        RouterStats {
            total_channels,
            total_users,
            channels: channels_info,
        }
    }
}

/// Router statistics
#[derive(Debug, Clone)]
pub struct RouterStats {
    pub total_channels: usize,
    pub total_users: usize,
    pub channels: Vec<(ChannelId, usize)>,
}

impl Default for VoiceRouter {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::net::{IpAddr, Ipv4Addr};
    
    fn test_addr(port: u16) -> SocketAddr {
        SocketAddr::new(IpAddr::V4(Ipv4Addr::new(127, 0, 0, 1)), port)
    }
    
    #[tokio::test]
    async fn test_join_channel() {
        let router = VoiceRouter::new();
        let addr = test_addr(1234);
        
        router.join_channel(1, 42, addr).await.unwrap();
        
        let channel = router.get_user_channel(addr).await;
        assert_eq!(channel, Some(1));
    }
    
    #[tokio::test]
    async fn test_multiple_users_same_channel() {
        let router = VoiceRouter::new();
        let addr1 = test_addr(1234);
        let addr2 = test_addr(1235);
        
        router.join_channel(1, 42, addr1).await.unwrap();
        router.join_channel(1, 43, addr2).await.unwrap();
        
        let users = router.get_channel_users(1, addr1).await;
        assert_eq!(users.len(), 1);
        assert_eq!(users[0], addr2);
    }
    
    #[tokio::test]
    async fn test_leave_channel() {
        let router = VoiceRouter::new();
        let addr = test_addr(1234);
        
        router.join_channel(1, 42, addr).await.unwrap();
        router.leave_all_channels(addr).await.unwrap();
        
        let channel = router.get_user_channel(addr).await;
        assert_eq!(channel, None);
    }
    
    #[tokio::test]
    async fn test_channel_isolation() {
        let router = VoiceRouter::new();
        let addr1 = test_addr(1234);
        let addr2 = test_addr(1235);
        
        router.join_channel(1, 42, addr1).await.unwrap();
        router.join_channel(2, 43, addr2).await.unwrap();
        
        let users_ch1 = router.get_channel_users(1, addr1).await;
        let users_ch2 = router.get_channel_users(2, addr2).await;
        
        assert_eq!(users_ch1.len(), 0); // Only sender in channel 1
        assert_eq!(users_ch2.len(), 0); // Only sender in channel 2
    }
}
