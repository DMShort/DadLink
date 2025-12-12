use crate::crypto::SrtpSession;
use crate::types::UserId;
use std::collections::HashMap;
use std::sync::Arc;
use tokio::sync::RwLock;
use tracing::info;

/// Manages SRTP encryption sessions for users
pub struct SrtpSessionManager {
    sessions: Arc<RwLock<HashMap<UserId, Arc<SrtpSession>>>>,
}

impl SrtpSessionManager {
    /// Create a new session manager
    pub fn new() -> Self {
        Self {
            sessions: Arc::new(RwLock::new(HashMap::new())),
        }
    }

    /// Add or update an SRTP session for a user
    pub async fn set_session(&self, user_id: UserId, session: SrtpSession) {
        let mut sessions = self.sessions.write().await;
        sessions.insert(user_id, Arc::new(session));
        info!("🔐 SRTP session established for user {}", user_id);
    }

    /// Get an SRTP session for a user
    pub async fn get_session(&self, user_id: UserId) -> Option<Arc<SrtpSession>> {
        let sessions = self.sessions.read().await;
        sessions.get(&user_id).cloned()
    }

    /// Remove a user's SRTP session (on disconnect)
    pub async fn remove_session(&self, user_id: UserId) {
        let mut sessions = self.sessions.write().await;
        if sessions.remove(&user_id).is_some() {
            info!("🔓 SRTP session removed for user {}", user_id);
        }
    }

    /// Check if a user has an active SRTP session
    pub async fn has_session(&self, user_id: UserId) -> bool {
        let sessions = self.sessions.read().await;
        sessions.contains_key(&user_id)
    }

    /// Get total number of active sessions
    pub async fn session_count(&self) -> usize {
        let sessions = self.sessions.read().await;
        sessions.len()
    }
}

impl Default for SrtpSessionManager {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[tokio::test]
    async fn test_add_and_get_session() {
        let manager = SrtpSessionManager::new();
        let (key, salt) = SrtpSession::generate_key_material();
        let session = SrtpSession::new(&key, &salt, 12345).unwrap();

        manager.set_session(1, session).await;

        assert!(manager.has_session(1).await);
        assert!(manager.get_session(1).await.is_some());
        assert_eq!(manager.session_count().await, 1);
    }

    #[tokio::test]
    async fn test_remove_session() {
        let manager = SrtpSessionManager::new();
        let (key, salt) = SrtpSession::generate_key_material();
        let session = SrtpSession::new(&key, &salt, 12345).unwrap();

        manager.set_session(1, session).await;
        assert!(manager.has_session(1).await);

        manager.remove_session(1).await;
        assert!(!manager.has_session(1).await);
        assert_eq!(manager.session_count().await, 0);
    }

    #[tokio::test]
    async fn test_multiple_sessions() {
        let manager = SrtpSessionManager::new();

        for user_id in 1..=5 {
            let (key, salt) = SrtpSession::generate_key_material();
            let session = SrtpSession::new(&key, &salt, user_id as u64).unwrap();
            manager.set_session(user_id, session).await;
        }

        assert_eq!(manager.session_count().await, 5);

        for user_id in 1..=5 {
            assert!(manager.has_session(user_id).await);
        }
    }
}
