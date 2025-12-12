# Cascade AI Rules for VoIP Project

## Project Identity
**Project Name**: Gaming-Focused Multi-Channel VoIP System  
**Target Users**: Large organized multiplayer operations (initially Star Citizen orgs)  
**Core Value Proposition**: Specialized, modernized "Mumble/TeamSpeak for combat ops" with radio-stack UX

---

## Behavioral Rules

### 1. Request Processing Protocol
When receiving any design/implementation request:

1. **Restate** the request in my own words
2. **List** key requirements inferred from the request
3. **Identify** dependencies and compatibility concerns:
   - OS compatibility (Windows 10/11 primary, Linux secondary)
   - Client/server language and library compatibility
   - Codec and audio stack compatibility
4. **Propose** 1-2 realistic implementation paths with pros/cons
5. **Choose** one path clearly and justify the choice
6. **Design** before coding:
   - Outline modules and data flows
   - Specify data structures and message formats
   - Define public APIs
   - Only then write code examples

### 2. Code Quality Standards
All code must be:
- **Idiomatic** for the chosen language
- **Strongly typed** where applicable
- **Clearly separated** by concerns
- **Commented** where non-obvious
- **Immediately runnable** with all necessary imports and dependencies

### 3. Testing & Debugging Requirements
For ANY non-trivial feature, always include:
- **How we will test it** (specific test types and methods)
- **Typical failure modes** and mitigation strategies
- **Logging/instrumentation** to be added
- **Regression test plans**

### 4. Assumption Protocol
If requirements are missing:
- Make **sensible assumptions**
- **State them explicitly**
- Only ask follow-ups if absolutely necessary

---

## Technical Architecture Rules

### Client Architecture (Windows 10/11 Primary)

#### Stack Selection Criteria
Evaluate options based on:
1. **Latency characteristics** (target: <150ms end-to-end)
2. **Maintenance complexity**
3. **Library maturity**

#### Preferred Stack Options
**Option A - C++ Stack:**
- libopus for audio codec
- PortAudio for cross-platform audio I/O
- Qt for desktop UI

**Option B - Rust Core:**
- cpal for audio I/O
- Opus bindings
- Wrapped by C++ or UI layer

#### Required Client Modules
1. **Audio Engine**
   - Device enumeration and selection
   - Low-latency capture/playback
   - Sample rate conversion
   - NO blocking or heavy allocation in audio callbacks

2. **Codec Layer (Opus)**
   - Per-channel encoder instances
   - Per-stream decoder instances
   - Variable bitrates
   - Packet loss concealment (PLC)
   - Discontinuous Transmission (DTX) for silence

3. **Network Layer**
   - UDP for voice packets
   - TLS WebSocket/gRPC/TCP for signalling
   - Packet structure: ServerID, OrgID, ChannelID, SenderID, SeqNum, Timestamp
   - Jitter buffer implementation

4. **Mixer & Channel Router**
   - Per-channel volume/pan/mute/monitor settings
   - Multi-channel PCM mixing
   - Audio ducking rules (e.g., Command channel priority)

5. **Hotkey & Input Manager**
   - Global hotkey registration (OS-appropriate APIs)
   - Per-channel PTT mapping
   - Simulcast support
   - Conflict avoidance with game bindings

6. **Config & Profile Manager**
   - JSON/YAML config files
   - Org server configurations
   - Channel presets and operation templates
   - Keybindings and audio settings

7. **UI Layer**
   - Radio stack view (channels, active speakers)
   - Org/operation preset loader
   - Settings interface
   - Optional in-game overlay

### Server Architecture

#### Language Options (Choose and Justify)
- **Rust**: Tokio async, strong network ecosystem, good Opus support
- **Go**: Excellent for network daemons, easy concurrency
- **C++**: Mumble-like architecture

#### Core Server Responsibilities
1. Accept client connections
2. Authenticate users
3. Maintain org/role/channel hierarchies
4. Track user-channel memberships
5. **Route encoded Opus packets** (NO decoding/mixing on server)
6. Expose admin API (HTTP/REST or gRPC + WebSocket)

#### Required Server Modules
1. **Network Frontend**
   - TLS control connections
   - UDP voice packet handling
   - DDoS protection and rate limiting

2. **Org / Tenant Manager**
   - Multi-org per server process
   - Per-org: settings, user DB, roles, channel trees

3. **Channel & Role Manager**
   - Hierarchical channels with per-channel ACLs
   - Roles as permission bundles (join/speak/manage)
   - Operation presets (preconfigured channel trees)

4. **Routing Logic**
   - Validate sender, channel membership, permissions
   - Forward encoded packets to channel listeners
   - Support linked channels if needed
   - Pure forwarder (no decoding unless mixing feature designed)

5. **Admin API**
   - CRUD operations: orgs, channels, roles, presets
   - Queries: users, sessions, channel occupancy
   - Authentication & authorization

6. **Persistence Layer**
   - Choose: PostgreSQL, SQLite, or file-based JSON/YAML
   - Store: org configs, channels, roles, user profiles
   - Justify choice based on scale and deployment model

---

## Coding Practice Rules

### Separation of Concerns
- Keep audio I/O, encoding, networking, and UI in **separate modules**
- NO UI code in audio callbacks or network drivers

### Real-Time Audio Practices (CRITICAL)
- **NEVER** allocate memory in audio callbacks
- **NEVER** perform blocking I/O in audio callbacks
- Pre-allocate buffers and reuse them
- Use lock-free queues or ring buffers for cross-thread communication
- Keep CPU-heavy work off the real-time path

### Networking Practices
- Implement sequence numbers and timestamps for:
  - Out-of-order packet detection
  - Jitter buffering support
  - Packet loss concealment
- Add configurable jitter buffer:
  - Adjustable playout delay
  - Trade-off: latency vs packet loss tolerance
- Recommend DSCP/QoS markings for VoIP traffic
- Target: <150ms one-way end-to-end delay

### Security Requirements
- Control channel over **TLS** (mutual TLS or token-based)
- Encrypted voice transport (**SRTP** or custom AES over UDP)
- Proper authentication and authorization on admin endpoints
- No hardcoded credentials

### Compatibility & Integration
- Design protocol/APIs for interoperability:
  - Different client implementations (desktop, future mobile)
  - Future WebRTC-based clients possible
- Avoid engine-specific dependencies (UE5 can consume API, not core VoIP)
- Versioned config formats
- Provide defaults with override capability
- Embed hooks for:
  - Custom auth backends
  - External telemetry

---

## Testing & Debugging Protocol (MANDATORY)

### Common VoIP Problems to Anticipate
Must design for and handle:

1. **Jitter** - Variable packet arrival times
   - Mitigation: Jitter buffer with configurable playout delay
   - Test: Simulate variable network delays

2. **Packet Loss** - Missing audio frames
   - Mitigation: Opus PLC
   - Test: Drop N% of packets randomly

3. **Latency** - High end-to-end delay
   - Mitigation: Low-latency codec settings, minimal buffering
   - Test: Measure end-to-end timing under load

4. **Echo** - Acoustic feedback
   - Mitigation: Echo cancellation, encourage headsets
   - Test: Speaker + mic feedback scenarios

5. **Broken/Garbled Audio** - Buffering or codec issues
   - Mitigation: Proper sample rate handling, buffer management
   - Test: Loopback with various configurations

6. **Dropped Sessions** - Network hiccups, NAT changes
   - Mitigation: Reconnection logic, keepalives
   - Test: Simulate connection drops

### Required Testing Layers

#### 1. Unit Tests
- Pure functions (packet parsers, config readers)
- Jitter buffer algorithms
- Permission checks and ACL evaluation
- Timing logic

#### 2. Component / Integration Tests
- Audio engine with mock/loopback devices
- Encode/decode cycle across bitrates and frame sizes
- Network routing with simulated clients

#### 3. Network Condition Tests
- Simulate jitter, packet loss, reordering
- Test harness that:
  - Drops N% of packets
  - Delays packets by random intervals
  - Reorders packet sequences
- Verify: jitter buffer behavior, audio quality, stability

#### 4. Load / Stress Tests
- Simulate many concurrent users (e.g., 100 clients)
- Measure: CPU usage, memory, packet routing latency
- Ensure no O(N²) pathological behavior

#### 5. End-to-End Scenario Tests
- Scripted operations (Command, Flight, Ground, Logistics channels)
- Test: role-based membership, ACL enforcement, UX displays

#### 6. Cross-Platform Tests
- Verify per OS: device enumeration, latency, hotkeys, overlay

### Bug Triage & Fix Workflow

When debugging or fixing issues, follow this workflow:

1. **Clarify the Symptom**
   - Restate: expected vs observed behavior
   - Note logs and error messages

2. **Locate Likely Layer**
   - Audio I/O, codec, network, jitter buffer, routing, ACL, UI, or hotkeys

3. **Propose Additional Instrumentation**
   - Specific log statements
   - Metrics and debug views
   - Sequence numbers, timestamps, buffer levels

4. **Hypothesis & Fix**
   - Form specific hypothesis about root cause
   - Propose code/config change
   - Explain why it addresses the root cause

5. **Regression and Edge Case Testing**
   - List unit tests to add/update
   - Integration tests to rerun
   - Edge cases to verify

6. **Document**
   - Describe bug, root cause, fix
   - Document how to avoid reintroduction

---

## Deliverable Types

Depending on the request, produce:
- **Architecture diagrams** (text descriptions if visual not possible)
- **API designs** (JSON, protobuf schemas)
- **Concrete code** (runnable with minimal adjustments)
- **Test plans** and test harness pseudo-code
- **Admin UI flows** and data models

### Code Deliverable Standards
- Make it **runnable in principle**
- Show function signatures, data types, sufficient context
- Include build/run notes where appropriate
- NO pseudo-APIs unless clearly labelled

---

## Style and Precision

### Design Philosophy
- **Clarity and correctness** over cleverness
- **Simple, robust designs** over premature optimization
- Explicitly call out **trade-offs**
- Example: "This gives lower latency at the cost of higher CPU"

### Library References
When referencing third-party libraries, specify:
1. Its **role** in the system
2. How it **integrates** with our stack
3. Known **caveats/limitations** to plan around

### Communication Style
- Be **precise and technical**
- State **assumptions explicitly**
- Provide **rationale** for decisions
- Include **concrete examples** where helpful

---

## Summary

These rules ensure that all work on the VoIP project:
1. Follows rigorous design-before-code methodology
2. Maintains high code quality and real-time audio standards
3. Includes comprehensive testing and debugging considerations
4. Addresses compatibility, security, and scalability requirements
5. Produces runnable, well-documented deliverables

**Apply these rules to every design, implementation, and debugging task for this project.**
