# Admin Dashboard Development Plan

**Created:** November 29, 2024  
**Status:** Planning  
**Priority:** High (Post-MVP Feature)  
**Estimated Time:** 2-3 weeks

---

## 📋 Executive Summary

Create a comprehensive web-based admin dashboard for the VoIP system to manage organizations, users, channels, permissions, and monitor system health. The dashboard will leverage the existing Supabase PostgreSQL database and provide real-time insights into system operations.

**Technology Stack:**
- **Frontend:** React + TypeScript + Tailwind CSS + shadcn/ui
- **Backend:** Rust (Axum) REST API (reuse existing server infrastructure)
- **Database:** Supabase PostgreSQL (already integrated ✅)
- **Real-time:** WebSocket for live updates
- **Auth:** JWT tokens (already implemented ✅)
- **Deployment:** Vercel/Netlify (frontend) + existing VoIP server (API)

---

## 🎯 Core Features

### 1. Dashboard Overview (Landing Page)
**Priority:** High  
**Estimated Time:** 2-3 days

**Features:**
- **System Health:**
  - Server status (online/offline)
  - Active connections count
  - CPU/Memory usage
  - Network throughput
  - Database connection status

- **Quick Stats:**
  - Total organizations
  - Total users (active/inactive)
  - Total channels
  - Active voice sessions
  - Messages sent (last 24h)

- **Recent Activity Feed:**
  - User logins
  - Channel joins/leaves
  - Permission changes
  - System events

- **Charts & Graphs:**
  - User activity over time
  - Voice quality metrics
  - Peak usage hours
  - Error rate trends

**UI Components:**
- Stat cards with icons
- Line/bar charts (Chart.js or Recharts)
- Activity timeline
- Status indicators (green/yellow/red)

---

### 2. Organization Management
**Priority:** High  
**Estimated Time:** 2-3 days

**Features:**

#### Organization List
- Table view with sorting/filtering
- Columns: Name, Tag, Owner, Users, Channels, Created Date
- Search by name/tag
- Actions: View, Edit, Delete

#### Organization Details
- **Info Tab:**
  - Name, tag, owner
  - Creation date
  - User/channel limits
  - Current usage stats

- **Users Tab:**
  - All users in organization
  - Role assignments
  - Last login times
  - Add/remove users

- **Channels Tab:**
  - Channel tree/hierarchy
  - Channel permissions
  - Active users per channel
  - Create/edit/delete channels

- **Settings Tab:**
  - Update organization limits
  - Transfer ownership
  - Archive/delete organization
  - Audit log for org

**API Endpoints:**
```rust
GET    /api/admin/organizations
GET    /api/admin/organizations/:id
POST   /api/admin/organizations
PUT    /api/admin/organizations/:id
DELETE /api/admin/organizations/:id
GET    /api/admin/organizations/:id/users
GET    /api/admin/organizations/:id/channels
GET    /api/admin/organizations/:id/stats
```

---

### 3. User Management
**Priority:** High  
**Estimated Time:** 3-4 days

**Features:**

#### User List
- Paginated table (50 users per page)
- Columns: Username, Email, Organization, Roles, Last Login, Status
- Filters: Organization, Role, Status (active/inactive/banned)
- Search: Username or email
- Bulk actions: Ban, assign role, delete

#### User Details
- **Profile Tab:**
  - Username, email, organization
  - Created date, last login
  - Status (active/inactive/banned)
  - Edit profile button

- **Roles & Permissions Tab:**
  - Current roles
  - Permission matrix (visual)
  - Add/remove roles
  - Custom permission overrides

- **Activity Tab:**
  - Login history (last 30 days)
  - Channels joined
  - Voice session history
  - Messages sent
  - Audit trail

- **Sessions Tab:**
  - Active sessions
  - Session details (IP, device, duration)
  - Force logout button

**Actions:**
- Edit user details
- Reset password (send email)
- Ban/unban user
- Delete user (with confirmation)
- View impersonation (admin logs in as user for support)

**API Endpoints:**
```rust
GET    /api/admin/users
GET    /api/admin/users/:id
POST   /api/admin/users
PUT    /api/admin/users/:id
DELETE /api/admin/users/:id
POST   /api/admin/users/:id/ban
POST   /api/admin/users/:id/unban
POST   /api/admin/users/:id/reset-password
GET    /api/admin/users/:id/activity
GET    /api/admin/users/:id/sessions
DELETE /api/admin/users/:id/sessions/:session_id
```

---

### 4. Channel Management
**Priority:** Medium  
**Estimated Time:** 2-3 days

**Features:**

#### Channel Tree View
- Hierarchical visualization
- Drag-and-drop to reorganize
- Expand/collapse branches
- Icons for channel types
- Color coding by organization

#### Channel Details
- **Info Tab:**
  - Name, description
  - Parent channel
  - Max users, position
  - Created date
  - Current active users

- **Permissions Tab:**
  - ACL entries (role → permissions)
  - Visual permission matrix
  - Add/edit/delete ACL entries
  - Channel password (set/remove)

- **Users Tab:**
  - Currently connected users
  - Voice activity indicators
  - Kick user action
  - Move user to another channel

- **Statistics Tab:**
  - Total joins/leaves
  - Average session duration
  - Peak concurrent users
  - Voice quality metrics

**Actions:**
- Create subchannel
- Edit channel details
- Move channel (change parent)
- Delete channel (cascade warning)
- Lock/unlock channel
- Set temporary limits

**API Endpoints:**
```rust
GET    /api/admin/channels
GET    /api/admin/channels/:id
POST   /api/admin/channels
PUT    /api/admin/channels/:id
DELETE /api/admin/channels/:id
GET    /api/admin/channels/:id/users
POST   /api/admin/channels/:id/acl
PUT    /api/admin/channels/:id/acl/:role_id
DELETE /api/admin/channels/:id/acl/:role_id
POST   /api/admin/channels/:id/kick/:user_id
```

---

### 5. Role & Permission Management
**Priority:** Medium  
**Estimated Time:** 2-3 days

**Features:**

#### Role List
- Table view per organization
- Columns: Name, Priority, Permissions, Users
- Default roles highlighted
- Create new role button

#### Role Details
- **Info Tab:**
  - Name, organization
  - Priority (for conflict resolution)
  - Created date
  - Number of users with role

- **Permissions Tab:**
  - Visual permission editor
  - Checkboxes for each permission:
    - JOIN (join channels)
    - SPEAK (transmit voice)
    - WHISPER (private voice)
    - MANAGE (edit channels)
    - KICK (remove users)
    - ADMIN (full access)
  - Permission inheritance visualization

- **Users Tab:**
  - All users with this role
  - Add/remove users
  - Bulk operations

- **Channel ACLs Tab:**
  - All channels with ACL entries for this role
  - Edit channel-specific permissions

**Actions:**
- Create custom role
- Edit role permissions
- Delete role (reassign users warning)
- Clone role (template)
- Export/import role definitions

**API Endpoints:**
```rust
GET    /api/admin/roles
GET    /api/admin/roles/:id
POST   /api/admin/roles
PUT    /api/admin/roles/:id
DELETE /api/admin/roles/:id
GET    /api/admin/roles/:id/users
POST   /api/admin/roles/:id/users/:user_id
DELETE /api/admin/roles/:id/users/:user_id
```

---

### 6. Monitoring & Analytics
**Priority:** Medium  
**Estimated Time:** 3-4 days

**Features:**

#### System Metrics
- **Server Health:**
  - CPU usage (real-time graph)
  - Memory usage (real-time graph)
  - Network I/O
  - Database connections
  - WebSocket connections
  - UDP voice connections

- **Voice Quality:**
  - Average latency
  - Packet loss percentage
  - Jitter measurements
  - Opus codec stats
  - PLC (packet loss concealment) usage

- **Database Performance:**
  - Query count (per second)
  - Slow queries log
  - Connection pool status
  - Cache hit rate

#### User Analytics
- **Activity Metrics:**
  - Daily active users (DAU)
  - Monthly active users (MAU)
  - User retention rate
  - Average session duration
  - Peak concurrent users

- **Usage Patterns:**
  - Most active hours (heatmap)
  - Most popular channels
  - Organization activity comparison
  - Geographic distribution (if IP tracking)

#### Voice Analytics
- **Session Metrics:**
  - Total voice sessions (24h/7d/30d)
  - Average session length
  - Peak concurrent speakers
  - Channel usage distribution

- **Quality Metrics:**
  - Latency distribution (histogram)
  - Packet loss by user/channel
  - Audio quality scores
  - Error rate over time

**Visualization:**
- Real-time line charts
- Histograms
- Heatmaps
- Pie charts
- Geographic maps (optional)

**Export Features:**
- CSV export for all metrics
- PDF reports
- Scheduled email reports
- API access for external tools

---

### 7. Audit Log Viewer
**Priority:** Medium  
**Estimated Time:** 2 days

**Features:**

#### Log Table
- Paginated view (100 logs per page)
- Columns: Timestamp, User, Action, Resource, Details, IP
- Filters:
  - Date range
  - User
  - Action type
  - Organization
  - Resource type

#### Log Details
- Full event information
- Before/after states (for changes)
- Associated user details
- Stack trace (for errors)
- Related events timeline

#### Actions
- Search logs
- Export filtered logs (CSV/JSON)
- Set up alerts for specific events
- Archive old logs

**API Endpoints:**
```rust
GET    /api/admin/audit-log
GET    /api/admin/audit-log/:id
POST   /api/admin/audit-log/search
GET    /api/admin/audit-log/export
```

---

### 8. Configuration Management
**Priority:** Low  
**Estimated Time:** 2 days

**Features:**

#### Server Configuration
- **General Settings:**
  - Server name
  - Description
  - Max users (global)
  - Max channels (global)

- **Network Settings:**
  - Control port (WebSocket)
  - Voice port (UDP)
  - TLS certificate paths
  - CORS allowed origins

- **Security Settings:**
  - JWT secret rotation
  - Session timeout
  - Max login attempts
  - Password policy
  - 2FA enforcement

- **Audio Settings:**
  - Default codec settings
  - Bitrate limits
  - Buffer sizes
  - Quality presets

#### Feature Flags
- Toggle features on/off:
  - User registration
  - Channel creation
  - Voice encryption
  - Audit logging
  - Metrics collection

**API Endpoints:**
```rust
GET    /api/admin/config
PUT    /api/admin/config
GET    /api/admin/config/feature-flags
PUT    /api/admin/config/feature-flags/:flag
```

---

## 🏗️ Technical Architecture

### Frontend Structure

```
admin-dashboard/
├── src/
│   ├── components/
│   │   ├── common/
│   │   │   ├── Button.tsx
│   │   │   ├── Table.tsx
│   │   │   ├── Modal.tsx
│   │   │   ├── Card.tsx
│   │   │   └── Chart.tsx
│   │   ├── layout/
│   │   │   ├── Sidebar.tsx
│   │   │   ├── Header.tsx
│   │   │   ├── Footer.tsx
│   │   │   └── Layout.tsx
│   │   ├── dashboard/
│   │   │   ├── StatsCard.tsx
│   │   │   ├── ActivityFeed.tsx
│   │   │   └── QuickActions.tsx
│   │   ├── organizations/
│   │   │   ├── OrgList.tsx
│   │   │   ├── OrgDetails.tsx
│   │   │   └── OrgForm.tsx
│   │   ├── users/
│   │   │   ├── UserList.tsx
│   │   │   ├── UserDetails.tsx
│   │   │   └── UserForm.tsx
│   │   ├── channels/
│   │   │   ├── ChannelTree.tsx
│   │   │   ├── ChannelDetails.tsx
│   │   │   └── ChannelForm.tsx
│   │   ├── roles/
│   │   │   ├── RoleList.tsx
│   │   │   ├── RoleDetails.tsx
│   │   │   └── PermissionMatrix.tsx
│   │   └── monitoring/
│   │       ├── SystemMetrics.tsx
│   │       ├── UserAnalytics.tsx
│   │       └── VoiceQuality.tsx
│   ├── pages/
│   │   ├── Dashboard.tsx
│   │   ├── Organizations.tsx
│   │   ├── Users.tsx
│   │   ├── Channels.tsx
│   │   ├── Roles.tsx
│   │   ├── Monitoring.tsx
│   │   ├── AuditLog.tsx
│   │   ├── Settings.tsx
│   │   └── Login.tsx
│   ├── api/
│   │   ├── client.ts
│   │   ├── organizations.ts
│   │   ├── users.ts
│   │   ├── channels.ts
│   │   ├── roles.ts
│   │   └── metrics.ts
│   ├── hooks/
│   │   ├── useAuth.ts
│   │   ├── useOrganizations.ts
│   │   ├── useUsers.ts
│   │   ├── useChannels.ts
│   │   └── useWebSocket.ts
│   ├── utils/
│   │   ├── formatters.ts
│   │   ├── validators.ts
│   │   └── constants.ts
│   ├── types/
│   │   ├── api.ts
│   │   ├── models.ts
│   │   └── enums.ts
│   ├── App.tsx
│   └── main.tsx
├── public/
├── package.json
├── tsconfig.json
├── tailwind.config.js
└── vite.config.ts
```

### Backend API Structure

```rust
server/
├── src/
│   ├── api/
│   │   ├── mod.rs
│   │   ├── admin/
│   │   │   ├── mod.rs
│   │   │   ├── organizations.rs
│   │   │   ├── users.rs
│   │   │   ├── channels.rs
│   │   │   ├── roles.rs
│   │   │   ├── metrics.rs
│   │   │   ├── audit_log.rs
│   │   │   └── config.rs
│   │   └── auth.rs
│   ├── middleware/
│   │   ├── admin_auth.rs
│   │   └── rate_limit.rs
│   └── ...
```

---

## 🔐 Security Considerations

### Authentication & Authorization

1. **Admin Access Control:**
   - Separate admin role with full permissions
   - JWT tokens with admin claim
   - Short token expiration (15 minutes)
   - Refresh token rotation

2. **API Security:**
   - All endpoints require admin authentication
   - Rate limiting (100 requests/minute per admin)
   - CORS configuration (whitelist admin domain)
   - CSRF protection

3. **Audit Trail:**
   - Log all admin actions
   - IP address tracking
   - Session management
   - Suspicious activity alerts

### Data Protection

1. **Sensitive Data:**
   - Never display password hashes
   - Mask partial email addresses
   - Hide JWT secrets
   - Sanitize error messages

2. **API Keys:**
   - Environment variables only
   - Never commit to git
   - Rotation capability
   - Secure storage (secrets manager)

---

## 📊 Database Schema Extensions

### Metrics Tables

```sql
-- Real-time server metrics
CREATE TABLE server_metrics (
    id BIGSERIAL PRIMARY KEY,
    timestamp TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    cpu_percent DECIMAL(5,2),
    memory_mb INTEGER,
    active_connections INTEGER,
    voice_sessions INTEGER,
    messages_per_second INTEGER,
    avg_latency_ms INTEGER
);

-- Voice session quality
CREATE TABLE voice_quality_metrics (
    id BIGSERIAL PRIMARY KEY,
    session_id UUID,
    user_id INTEGER REFERENCES users(id),
    channel_id INTEGER REFERENCES channels(id),
    timestamp TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    latency_ms INTEGER,
    packet_loss_percent DECIMAL(5,2),
    jitter_ms INTEGER,
    quality_score INTEGER CHECK (quality_score BETWEEN 1 AND 5)
);

-- User activity tracking
CREATE TABLE user_activity (
    id BIGSERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id),
    activity_type VARCHAR(50),
    timestamp TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    details JSONB
);

-- Feature flags
CREATE TABLE feature_flags (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) UNIQUE NOT NULL,
    enabled BOOLEAN DEFAULT FALSE,
    description TEXT,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Admin settings
CREATE TABLE admin_settings (
    key VARCHAR(100) PRIMARY KEY,
    value JSONB NOT NULL,
    description TEXT,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_by INTEGER REFERENCES users(id)
);
```

---

## 🧪 Testing Strategy

### Frontend Tests
- **Unit Tests:** React components (Jest + React Testing Library)
- **Integration Tests:** API calls and data flow
- **E2E Tests:** Critical user flows (Playwright)
- **Visual Regression:** Component screenshots (Chromatic)

### Backend Tests
- **Unit Tests:** API handlers and business logic
- **Integration Tests:** Database queries
- **Load Tests:** API performance under load
- **Security Tests:** Authentication and authorization

### Manual Testing
- Cross-browser compatibility (Chrome, Firefox, Safari, Edge)
- Responsive design (desktop, tablet, mobile)
- Accessibility (WCAG 2.1 AA)
- User acceptance testing

---

## 📅 Development Timeline

### Week 1: Foundation (5 days)
- [ ] Set up React + TypeScript project
- [ ] Create API routes in Rust server
- [ ] Implement authentication system
- [ ] Build layout and navigation
- [ ] Create common UI components

### Week 2: Core Features (5 days)
- [ ] Dashboard overview page
- [ ] Organization management
- [ ] User management (list + details)
- [ ] Basic role management

### Week 3: Advanced Features (5 days)
- [ ] Channel management (tree view)
- [ ] Permission matrix UI
- [ ] Monitoring & analytics
- [ ] Audit log viewer

### Week 4: Polish & Deploy (5 days)
- [ ] Configuration management
- [ ] Real-time WebSocket updates
- [ ] Testing and bug fixes
- [ ] Documentation
- [ ] Deployment to production

**Total: 20 working days (~4 weeks)**

---

## 🚀 Deployment Strategy

### Frontend Deployment
- **Platform:** Vercel or Netlify
- **Domain:** `admin.yourdomain.com`
- **SSL:** Automatic (Let's Encrypt)
- **CDN:** Global edge network
- **CI/CD:** Auto-deploy on push to main branch

### Backend API
- **Integration:** Add routes to existing VoIP server
- **Port:** Same as control port (9000)
- **Path:** `/api/admin/*`
- **Auth:** JWT middleware on all routes

### Database Migrations
- **Versioned migrations** in `server/migrations/`
- **Backward compatible** changes
- **Automated deployment** with server startup

---

## 📖 Documentation Requirements

1. **Admin User Guide:**
   - How to access dashboard
   - Feature walkthrough
   - Common tasks
   - Troubleshooting

2. **API Documentation:**
   - OpenAPI/Swagger spec
   - Endpoint descriptions
   - Request/response examples
   - Authentication flow

3. **Development Guide:**
   - Setup instructions
   - Architecture overview
   - Contributing guidelines
   - Testing procedures

---

## 🎯 Success Criteria

### Functionality
- [ ] All CRUD operations working for orgs/users/channels/roles
- [ ] Real-time metrics display
- [ ] Responsive UI (mobile-friendly)
- [ ] Fast page loads (<1 second)

### Security
- [ ] Admin authentication required
- [ ] All actions logged to audit log
- [ ] No security vulnerabilities (tested)
- [ ] Rate limiting working

### Performance
- [ ] API response time <200ms (95th percentile)
- [ ] Dashboard loads in <2 seconds
- [ ] Support 10+ concurrent admins
- [ ] No memory leaks

### Quality
- [ ] 80%+ test coverage
- [ ] Zero critical bugs
- [ ] Accessibility WCAG 2.1 AA
- [ ] Documentation complete

---

## 💡 Future Enhancements

### Phase 2 Features
- **Advanced Analytics:**
  - Custom report builder
  - Predictive analytics
  - Anomaly detection
  - Cost tracking

- **Automation:**
  - Scheduled tasks
  - Auto-scaling rules
  - Backup management
  - Alert escalation

- **Integrations:**
  - Discord webhooks
  - Slack notifications
  - Email campaigns
  - Third-party SSO

- **Mobile App:**
  - React Native admin app
  - Push notifications
  - Quick actions
  - Offline mode

---

## 🛠️ Technology Stack Details

### Frontend
```json
{
  "dependencies": {
    "react": "^18.2.0",
    "react-dom": "^18.2.0",
    "react-router-dom": "^6.20.0",
    "typescript": "^5.3.0",
    "@tanstack/react-query": "^5.0.0",
    "axios": "^1.6.0",
    "tailwindcss": "^3.3.0",
    "@radix-ui/react-*": "latest",
    "recharts": "^2.10.0",
    "date-fns": "^2.30.0",
    "zod": "^3.22.0",
    "react-hook-form": "^7.48.0"
  }
}
```

### Backend
```toml
[dependencies]
# Existing dependencies +
serde = { version = "1.0", features = ["derive"] }
chrono = "0.4"
sysinfo = "0.30"  # System metrics
prometheus = "0.13"  # Metrics collection (optional)
```

---

## 📞 Next Steps

1. **Review this plan** and provide feedback
2. **Prioritize features** (MVP vs Phase 2)
3. **Set timeline** based on availability
4. **Create project repo** for admin dashboard
5. **Begin implementation** starting with Week 1 tasks

---

**Ready to build a world-class admin dashboard?** 🚀

This dashboard will give you complete control over your VoIP system with real-time visibility and powerful management tools!
