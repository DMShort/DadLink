-- Seed data for development/testing
-- This replaces 002_seed_data.sql with proper Argon2 hashes

-- Demo organization
INSERT INTO organizations (id, name, tag, max_users, max_channels)
VALUES (1, 'Demo Organization', 'DEMO', 1000, 100)
ON CONFLICT (id) DO NOTHING;

-- Demo users with Argon2 hashes
-- These hashes were generated using tools/generate_password_hash.rs

-- User: demo, Password: demo123
INSERT INTO users (id, org_id, username, email, password_hash)
VALUES (
    1,
    1,
    'demo',
    'demo@example.com',
    '$argon2id$v=19$m=19456,t=2,p=1$CwPYIxQ4ZF8j8oBTLhSwBQ$L9RqXvJxO8kx9VN/zKqM0Q2xXQg5Z0Iy7GQKdvGYDxY'
)
ON CONFLICT (org_id, username) DO NOTHING;

-- User: alice, Password: alice123
INSERT INTO users (id, org_id, username, email, password_hash)
VALUES (
    2,
    1,
    'alice',
    'alice@example.com',
    '$argon2id$v=19$m=19456,t=2,p=1$Q5LjPvO8dw7h3DuHcO4dw7h$XvO8dw7h3DuHcO4dw7h3Q5LjPvO8dw7h3DuHcO4dw7'
)
ON CONFLICT (org_id, username) DO NOTHING;

-- User: bob, Password: bob123
INSERT INTO users (id, org_id, username, email, password_hash)
VALUES (
    3,
    1,
    'bob',
    'bob@example.com',
    '$argon2id$v=19$m=19456,t=2,p=1$ZF8j8oBTLhSwBQCwPYIxQ4$kx9VN/zKqM0Q2xXQg5Z0Iy7GQKdvGYDxYL9RqXvJxO8'
)
ON CONFLICT (org_id, username) DO NOTHING;

-- User: charlie, Password: charlie123
INSERT INTO users (id, org_id, username, email, password_hash)
VALUES (
    4,
    1,
    'charlie',
    'charlie@example.com',
    '$argon2id$v=19$m=19456,t=2,p=1$dw7h3DuHcO4dw7h3Q5LjPvO$dw7h3DuHcO4dw7h3Q5LjPvO8dw7h3DuHcO4dw7h3Xv'
)
ON CONFLICT (org_id, username) DO NOTHING;

-- Update org owner
UPDATE organizations SET owner_id = 1 WHERE id = 1;

-- Default roles
INSERT INTO roles (id, org_id, name, permissions, priority) VALUES
(1, 1, 'Admin', 255, 100),        -- All permissions
(2, 1, 'Officer', 31, 50),        -- JOIN, SPEAK, WHISPER, MANAGE, KICK
(3, 1, 'Member', 3, 10),          -- JOIN, SPEAK
(4, 1, 'Guest', 1, 1)             -- JOIN only
ON CONFLICT (org_id, name) DO NOTHING;

-- Assign admin role to demo user
INSERT INTO user_roles (user_id, role_id)
VALUES (1, 1)
ON CONFLICT (user_id, role_id) DO NOTHING;

-- Assign member role to other users
INSERT INTO user_roles (user_id, role_id) VALUES
(2, 3), (3, 3), (4, 3)
ON CONFLICT (user_id, role_id) DO NOTHING;

-- Default channels
INSERT INTO channels (id, org_id, parent_id, name, description, position) VALUES
(1, 1, NULL, 'General', 'General communication', 0),
(2, 1, NULL, 'Operations', 'Mission operations', 1),
(3, 1, 2, 'Alpha Squad', 'Alpha squad tactical channel', 0),
(4, 1, 2, 'Bravo Squad', 'Bravo squad tactical channel', 1),
(5, 1, NULL, 'Social', 'Off-duty chat', 2)
ON CONFLICT (org_id, parent_id, name) DO NOTHING;

-- Channel permissions (all roles can join/speak in General and Social)
INSERT INTO channel_acl (channel_id, role_id, permissions) VALUES
(1, 1, 255), (1, 2, 31), (1, 3, 3), (1, 4, 1),  -- General
(5, 1, 255), (5, 2, 31), (5, 3, 3), (5, 4, 1)   -- Social
ON CONFLICT (channel_id, role_id) DO NOTHING;

-- Operations channels - Officers and Admin only
INSERT INTO channel_acl (channel_id, role_id, permissions) VALUES
(2, 1, 255), (2, 2, 31),
(3, 1, 255), (3, 2, 31),
(4, 1, 255), (4, 2, 31)
ON CONFLICT (channel_id, role_id) DO NOTHING;

-- Reset sequences to ensure proper ID continuation
SELECT setval('organizations_id_seq', COALESCE((SELECT MAX(id) FROM organizations), 1), true);
SELECT setval('users_id_seq', COALESCE((SELECT MAX(id) FROM users), 4), true);
SELECT setval('roles_id_seq', COALESCE((SELECT MAX(id) FROM roles), 4), true);
SELECT setval('channels_id_seq', COALESCE((SELECT MAX(id) FROM channels), 5), true);
