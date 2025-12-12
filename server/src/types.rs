use serde::{Deserialize, Serialize};
use chrono::{DateTime, Utc};

// Type aliases
pub type UserId = u32;
pub type ChannelId = u32;
pub type OrgId = u32;
pub type RoleId = u32;
pub type SequenceNumber = u64;

// Voice packet constants
pub const VOICE_PACKET_MAGIC: u32 = 0x564F4950; // 'VOIP'
pub const VOICE_PACKET_HEADER_SIZE: usize = 28;
pub const AES_GCM_NONCE_SIZE: usize = 12;
pub const AES_GCM_TAG_SIZE: usize = 16;

/// Voice packet header (network format)
#[repr(C, packed)]
#[derive(Debug, Clone, Copy)]
pub struct VoicePacketHeader {
    pub magic: u32,
    pub sequence: u64,
    pub timestamp: u64,  // Microseconds
    pub channel_id: u32,
    pub user_id: u32,
}

/// Voice packet (after parsing)
#[derive(Debug, Clone)]
pub struct VoicePacket {
    pub header: VoicePacketHeader,
    pub encrypted_payload: Vec<u8>,
}

/// Organization
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Organization {
    pub id: OrgId,
    pub name: String,
    pub tag: String,
    pub owner_id: Option<UserId>,
    pub created_at: DateTime<Utc>,
    pub max_users: u32,
    pub max_channels: u32,
}

/// User
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct User {
    pub id: UserId,
    pub org_id: OrgId,
    pub username: String,
    pub email: Option<String>,
    pub password_hash: String,
    pub created_at: DateTime<Utc>,
}

/// Channel
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Channel {
    pub id: ChannelId,
    pub org_id: OrgId,
    pub parent_id: Option<ChannelId>,
    pub name: String,
    pub description: String,
    pub password_hash: Option<String>,
    pub max_users: u32,
    pub created_at: DateTime<Utc>,
}

bitflags::bitflags! {
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
    #[serde(transparent)]
    pub struct Permissions: u32 {
        const JOIN = 0b0001;
        const SPEAK = 0b0010;
        const WHISPER = 0b0100;
        const MANAGE = 0b1000;
        const KICK = 0b00010000;
        const BAN = 0b00100000;
    }
}

/// Role
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Role {
    pub id: RoleId,
    pub org_id: OrgId,
    pub name: String,
    pub permissions: Permissions,
    pub priority: u32,
}

/// ACL Entry
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ACLEntry {
    pub channel_id: ChannelId,
    pub role_id: RoleId,
    pub permissions: Permissions,
}

/// User session
#[derive(Debug, Clone)]
pub struct Session {
    pub user_id: UserId,
    pub org_id: OrgId,
    pub username: String,
    pub roles: Vec<RoleId>,
    pub channels: Vec<ChannelId>,
    pub last_sequence: SequenceNumber,
}

/// JWT Claims
#[derive(Debug, Serialize, Deserialize)]
pub struct Claims {
    pub sub: UserId,        // Subject (user ID)
    pub org: OrgId,         // Organization ID
    pub roles: Vec<RoleId>, // User roles
    pub exp: i64,           // Expiration
    pub iat: i64,           // Issued at
    pub jti: String,        // JWT ID
}

/// Control messages (WebSocket)
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum ControlMessage {
    // Client to server
    Hello {
        version: String,
        client: String,
    },
    Authenticate {
        method: String,
        username: Option<String>,
        password: Option<String>,
        token: Option<String>,
    },
    Register {
        username: String,
        password: String,
        email: Option<String>,
    },
    JoinChannel {
        channel_id: ChannelId,
        password: Option<String>,
    },
    LeaveChannel {
        channel_id: ChannelId,
    },
    SetTransmitChannel {
        channel_id: ChannelId,
    },
    KeyExchangeResponse {
        public_key: [u8; 32],
    },
    Ping {
        timestamp: i64,
    },

    // Admin/Management (Client to server)
    AssignRole {
        user_id: UserId,
        role_id: RoleId,
    },
    RemoveRole {
        user_id: UserId,
        role_id: RoleId,
    },
    ListRoles {
        org_id: OrgId,
    },
    GetUserRoles {
        user_id: UserId,
    },
    SetChannelAcl {
        channel_id: ChannelId,
        role_id: RoleId,
        permissions: u32,  // Permissions as bitflags
    },

    // Server to client
    Challenge {
        methods: Vec<String>,
        server_version: String,
    },
    AuthResult {
        success: bool,
        user_id: Option<UserId>,
        org_id: Option<OrgId>,
        permissions: Option<u32>,
        session_token: Option<String>,
        voice_key: Option<String>,
        message: Option<String>,
    },
    RegisterResult {
        success: bool,
        user_id: Option<UserId>,
        message: String,
    },
    KeyExchangeInit {
        public_key: [u8; 32],
    },
    ChannelJoined {
        channel_id: ChannelId,
        channel_name: String,
        users: Vec<UserInfo>,
    },
    ChannelState {
        channel_id: ChannelId,
        event: String,
        user: Option<UserInfo>,
    },
    UserSpeaking {
        channel_id: ChannelId,
        user_id: UserId,
        speaking: bool,
    },
    UserLeft {
        channel_id: ChannelId,
        user_id: UserId,
    },
    Pong {
        timestamp: i64,
        server_time: i64,
    },
    RoleOperationResult {
        success: bool,
        message: String,
    },
    RolesList {
        roles: Vec<Role>,
    },
    UserRolesList {
        user_id: UserId,
        roles: Vec<Role>,
    },
    Error {
        code: String,
        message: String,
    },
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct UserInfo {
    pub id: UserId,
    pub name: String,
    pub speaking: bool,
}
