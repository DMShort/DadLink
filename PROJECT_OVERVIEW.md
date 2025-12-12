# Gaming-Focused Multi-Channel VoIP System
## Project Overview & Vision

**Version**: 1.0  
**Last Updated**: November 2025  
**Status**: Planning Phase

---

## Executive Summary

### What We're Building

A specialized multi-channel VoIP system for large-scale organized multiplayer gaming operations, initially targeting Star Citizen organizations. Unlike traditional VoIP solutions, this system enables users to participate in multiple voice channels simultaneously with independent control over transmission and reception for each channel.

### Core Capabilities

- **Multi-Channel Listening**: Listen to several channels simultaneously
- **Selective Transmission**: Per-channel hotkeys for targeted communication
- **Simulcast Support**: Transmit to multiple channels when needed
- **Radio Stack UX**: Military/aviation-inspired interface
- **Flexible Deployment**: Self-hosted (Docker/binary) OR managed cloud hosting
- **Org Management**: Server-side organization structure, roles, and permissions

### Key Differentiators

Unlike Discord, TeamSpeak, or Mumble:
- Multi-channel operation is CORE, not a workaround
- Per-channel PTT controls for complex operational scenarios
- Audio ducking and prioritization for command hierarchy
- Operation presets for rapid configuration deployment
- Ultra-low latency optimization (<150ms target)

---

## Project Vision

**Mission Statement:**  
Create the definitive VoIP solution for organized multiplayer gaming operations, combining traditional voice server reliability with modern UX and the flexibility needed for complex multi-team coordination.

### Success Criteria by Phase

#### Phase 1 - MVP (Months 1-6)
- [ ] Stable 2-4 channel simultaneous operation
- [ ] <150ms end-to-end latency on good networks
- [ ] Basic org/channel/role management
- [ ] Windows desktop client with radio stack UI
- [ ] Self-hosted Docker deployment
- [ ] 50+ concurrent users per server
- [ ] Core testing framework operational

#### Phase 2 - Production (Months 7-12)
- [ ] 10+ simultaneous channels
- [ ] Advanced audio features (ducking, spatial audio concepts)
- [ ] In-game overlay for Windows
- [ ] Managed hosting platform (multi-tenant)
- [ ] 500+ concurrent users per server
- [ ] Operation presets and templates
- [ ] Mobile client beta (iOS/Android)

#### Phase 3 - Scale (Months 13-18)
- [ ] 1000+ concurrent users per server
- [ ] Cross-server channel linking
- [ ] Advanced analytics and monitoring dashboard
- [ ] Public API for third-party integrations
- [ ] WebRTC-based web client
- [ ] Linux client release

---

## Target Users & Use Cases

### Primary Personas

**1. Fleet Commander**
- **Needs**: Monitor 4-6 channels simultaneously (Command, Flight Wings, Ground, Logistics)
- **Usage Pattern**: Primarily listens, occasionally broadcasts to all channels
- **Requirements**: Channel priority, audio ducking, clear visual indication of all active speakers
- **Pain Point**: Current solutions require multiple apps or constant channel switching

**2. Squadron Leader**
- **Needs**: Active participation in 2-3 channels (overall Command + own squadron + cross-squadron coordination)
- **Usage Pattern**: Frequent PTT switching between channels, needs quick response capability
- **Requirements**: Easy PTT hotkey access, muscle-memory-friendly controls, minimal latency
- **Pain Point**: Fumbling between channels loses critical seconds in combat

**3. Pilot/Operator**
- **Needs**: Primary channel + emergency command channel monitoring
- **Usage Pattern**: Mostly in one channel, needs ability to hear priority broadcasts
- **Requirements**: Minimal UI distraction, reliable audio, clear indicators
- **Pain Point**: Missing critical commands due to channel isolation

**4. Organization Administrator**
- **Needs**: Manage org structure, roles, permissions, operation configurations
- **Usage Pattern**: Pre-operation setup, ongoing member management
- **Requirements**: Intuitive admin interface, bulk operations, audit logging
- **Pain Point**: Complex permission systems, time-consuming op setup

### Typical Operation Scenario

**Star Citizen Large-Scale Operation (100+ participants)**

**Channel Structure:**
```
Org: [Example Org]
├── Command (Fleet Commander + Squadron Leaders)
├── Flight Operations
│   ├── Alpha Wing (Fighter Squadron)
│   ├── Beta Wing (Bomber Squadron)
│   └── Gamma Wing (Support Squadron)
├── Ground Operations
│   ├── Marines Team 1
│   └── Marines Team 2
├── Logistics
│   └── Supply Coordination
└── Social (Off-duty / Non-op chat)
```

**User Channel Subscriptions:**
- **Fleet Commander**: Listens to ALL, transmits primarily to Command
- **Alpha Wing Leader**: Listens to Command + Alpha Wing, transmits to both
- **Alpha Pilot**: Listens to Command (monitor only) + Alpha Wing (active)
- **Logistics Lead**: Listens to Command + Logistics, transmits to both

**During Operation:**
1. Fleet Commander issues orders on Command → all leaders hear
2. Leaders relay/coordinate on their specific channels
3. Critical alerts (enemy contact) can simulcast up the chain
4. Command channel automatically ducks other channels for all users
5. Quick channel switching via hotkeys (no menu navigation)

---

## High-Level Architecture

### System Components

```
┌────────────────────────────────────────────────────────┐
│                    CLIENT LAYER                        │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐│
│  │ Desktop  │ │  Mobile  │ │   Web    │ │ Overlay  ││
│  │ (Win/Lin)│ │(iOS/And) │ │(WebRTC)  │ │(In-game) ││
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘│
└───────┼────────────┼─────────────┼────────────┼──────┘
        └────────────┴─────────────┴────────────┘
                     │
        ┌────────────┴────────────┐
        │  NETWORK TRANSPORT      │
        │  ┌────────┐ ┌─────────┐│
        │  │UDP Voice│ │TLS Ctrl ││
        │  └────────┘ └─────────┘│
        └────────────┬────────────┘
                     │
┌────────────────────┴───────────────────────────────────┐
│                   SERVER LAYER                         │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ │
│  │  Network     │ │   Routing    │ │    Admin     │ │
│  │  Frontend    │ │   Engine     │ │     API      │ │
│  └──────┬───────┘ └──────┬───────┘ └──────┬───────┘ │
│         └──────────────┬──────────────────┘          │
│                        │                              │
│            ┌───────────┴──────────┐                  │
│            │  Org/Tenant Manager  │                  │
│            │  ┌─────┐ ┌─────────┐ │                  │
│            │  │Orgs │ │Channels │ │                  │
│            │  └─────┘ └─────────┘ │                  │
│            └──────────┬───────────┘                  │
│                       │                              │
│               ┌───────┴──────┐                       │
│               │ PostgreSQL   │                       │
│               └──────────────┘                       │
└────────────────────────────────────────────────────────┘
```

### Technology Stack Summary

**Client (Windows Primary):**
- **Language**: C++ (with Rust alternative for audio core)
- **Audio I/O**: PortAudio
- **Codec**: libopus
- **UI**: Qt 6
- **Networking**: Asio or native sockets
- **Crypto**: OpenSSL

**Server:**
- **Language**: Rust (with Go alternative)
- **Runtime**: Tokio (async)
- **Database**: PostgreSQL
- **Web Framework**: Axum
- **Protocols**: Custom UDP + WebSocket/TLS

**Protocol:**
- **Voice**: UDP with AES-256-GCM encryption, Opus codec
- **Control**: WebSocket over TLS 1.3, JSON messages
- **Auth**: JWT tokens

---

## Development Principles

### 1. Real-Time Audio First
- NO memory allocation in audio callbacks
- NO blocking operations in real-time paths
- Pre-allocated buffers and lock-free queues
- Target: <150ms end-to-end latency

### 2. Testing as Core Discipline
- Unit tests for all logic
- Integration tests for component interactions
- Network condition simulation (jitter, loss, reordering)
- Load testing (100+ concurrent users)
- End-to-end scenario testing

### 3. Design Before Code
- Architecture and data flow diagrams first
- API specifications before implementation
- Identify failure modes upfront
- Plan instrumentation and logging

### 4. Security by Design
- TLS for all control traffic
- Encrypted voice packets
- Proper authentication and authorization
- No hardcoded credentials
- Audit logging for admin actions

### 5. Deployment Flexibility
- Docker containers for easy self-hosting
- Standalone binaries as alternative
- Multi-tenant architecture for managed hosting
- Clear resource isolation between orgs

---

## Next Steps

1. **Review and finalize technology stack decisions** (Client: C++ vs Rust core, Server: Rust vs Go)
2. **Create detailed technical specifications** for each major module
3. **Set up development environment** and CI/CD pipeline
4. **Implement MVP core components** in priority order:
   - Audio engine with single-channel operation
   - Basic server routing
   - Simple client UI
   - Network protocol implementation
5. **Establish testing framework** early
6. **Iterate toward Phase 1 MVP**

---

## Related Documents

- `CASCADE_RULES.md` - AI assistant behavior rules for this project
- `ARCHITECTURE_DETAILED.md` - Deep dive into system architecture
- `CLIENT_DESIGN.md` - Client implementation specifications
- `SERVER_DESIGN.md` - Server implementation specifications
- `NETWORK_PROTOCOL.md` - Protocol specifications
- `TESTING_STRATEGY.md` - Comprehensive testing approach
- `DEPLOYMENT_GUIDE.md` - Deployment models and instructions
