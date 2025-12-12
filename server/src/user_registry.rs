use crate::types::UserId;
use std::collections::HashMap;
use std::sync::atomic::{AtomicU32, Ordering};
use tokio::sync::RwLock;
use tracing::{info, warn, error};
use serde::{Serialize, Deserialize};
use std::path::Path;

/// Simple user information
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct User {
    pub id: UserId,
    pub username: String,
    pub password: String,  // In production, this would be a hashed password
    pub org_id: u32,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub email: Option<String>,
}

/// User registry for authentication and user management
pub struct UserRegistry {
    users: RwLock<HashMap<String, User>>,  // username -> User
    next_user_id: AtomicU32,
}

impl UserRegistry {
    pub fn new() -> Self {
        Self {
            users: RwLock::new(HashMap::new()),
            next_user_id: AtomicU32::new(1),
        }
    }
    
    /// Load users from file, or initialize demo users if file doesn't exist
    pub async fn load_or_init(&self, path: &str) -> std::io::Result<()> {
        if Path::new(path).exists() {
            // Load from file
            match self.load_from_file(path).await {
                Ok(count) => {
                    info!("✅ Loaded {} users from {}", count, path);
                }
                Err(e) => {
                    error!("Failed to load users from {}: {}", path, e);
                    warn!("Initializing with demo users instead");
                    self.init_demo_users().await;
                }
            }
        } else {
            // Initialize with demo users
            info!("User file not found, creating demo users");
            self.init_demo_users().await;
            // Save demo users
            if let Err(e) = self.save_to_file(path).await {
                error!("Failed to save initial users: {}", e);
            }
        }
        Ok(())
    }
    
    /// Initialize demo users (call this after creating the registry)
    async fn init_demo_users(&self) {
        let demo_users = vec![
            ("demo", "demo123"),
            ("alice", "alice123"),
            ("bob", "bob123"),
            ("charlie", "charlie123"),
            ("test1", "123"),
            ("test2", "123"),
        ];
        
        for (username, password) in demo_users {
            let _ = self.register_user(username.to_string(), password.to_string(), 1).await;
        }
    }
    
    /// Register a new user
    pub async fn register_user(&self, username: String, password: String, org_id: u32) -> Result<UserId, String> {
        let mut users = self.users.write().await;
        
        // Check if user already exists
        if users.contains_key(&username) {
            return Err(format!("Username '{}' already exists", username));
        }
        
        // Validate username (alphanumeric, 3-20 chars)
        if username.len() < 3 || username.len() > 20 {
            return Err("Username must be 3-20 characters".to_string());
        }
        if !username.chars().all(|c| c.is_alphanumeric() || c == '_') {
            return Err("Username can only contain letters, numbers, and underscores".to_string());
        }
        
        // Validate password (min 3 chars for testing, would be more strict in production)
        if password.len() < 3 {
            return Err("Password must be at least 3 characters".to_string());
        }
        
        // Generate new user ID
        let user_id = self.next_user_id.fetch_add(1, Ordering::SeqCst);
        
        let user = User {
            id: user_id,
            username: username.clone(),
            password: password.clone(),
            org_id,
            email: None,
        };
        
        let username_clone = username.clone();
        users.insert(username, user);
        info!("✅ New user registered: {} (ID: {})", username_clone, user_id);
        Ok(user_id)
    }
    
    /// Authenticate a user with username/password
    pub async fn authenticate(&self, username: &str, password: &str) -> Option<User> {
        let users = self.users.read().await;
        
        if let Some(user) = users.get(username) {
            // In production, use proper password hashing (bcrypt, argon2, etc.)
            if user.password == password {
                return Some(user.clone());
            }
        }
        
        None
    }
    
    /// Get user by ID
    pub async fn get_user_by_id(&self, user_id: UserId) -> Option<User> {
        let users = self.users.read().await;
        
        users.values().find(|u| u.id == user_id).cloned()
    }
    
    /// Get user by username
    pub async fn get_user_by_username(&self, username: &str) -> Option<User> {
        let users = self.users.read().await;
        
        users.get(username).cloned()
    }
    
    /// Get all users (for admin purposes)
    pub async fn get_all_users(&self) -> Vec<User> {
        let users = self.users.read().await;
        
        users.values().cloned().collect()
    }
    
    /// Check if username exists
    pub async fn username_exists(&self, username: &str) -> bool {
        let users = self.users.read().await;
        users.contains_key(username)
    }
    
    /// Save users to JSON file
    pub async fn save_to_file(&self, path: &str) -> std::io::Result<()> {
        let users = self.users.read().await;
        let user_list: Vec<User> = users.values().cloned().collect();
        
        let json = serde_json::to_string_pretty(&user_list)
            .map_err(|e| std::io::Error::new(std::io::ErrorKind::Other, e))?;
        
        tokio::fs::write(path, json).await?;
        info!("💾 Saved {} users to {}", user_list.len(), path);
        Ok(())
    }
    
    /// Load users from JSON file
    pub async fn load_from_file(&self, path: &str) -> std::io::Result<usize> {
        let json = tokio::fs::read_to_string(path).await?;
        let user_list: Vec<User> = serde_json::from_str(&json)
            .map_err(|e| std::io::Error::new(std::io::ErrorKind::InvalidData, e))?;
        
        let mut users = self.users.write().await;
        users.clear();
        
        let mut max_id = 0;
        for user in user_list {
            if user.id > max_id {
                max_id = user.id;
            }
            users.insert(user.username.clone(), user);
        }
        
        // Set next_user_id to max_id + 1
        self.next_user_id.store(max_id + 1, Ordering::SeqCst);
        
        Ok(users.len())
    }
}

impl Default for UserRegistry {
    fn default() -> Self {
        Self::new()
    }
}
