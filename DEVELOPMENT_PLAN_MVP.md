# 🚀 MVP Development Plan - Path to Production

**Created:** November 27, 2025
**Target Completion:** 6-8 weeks
**Current Status:** Phase 1 Starting

---

## 📋 Executive Summary

This plan takes the VoIP system from 95% MVP completion to production-ready status. We'll systematically address remaining issues, implement missing features, and thoroughly test the system.

**Total Estimated Time:** 14-22 days of active development + 14-21 days testing
**Risk Level:** Low (core system works, polish and infrastructure remaining)

---

## 🎯 Development Phases

### Phase 1: WebSocket Message Parsing Fix ⚡ HIGH PRIORITY
**Estimated Time:** 1-2 days
**Objective:** Fix user list population and message type parsing

**Tasks:**
1. ✅ Analyze WebSocket message flow
2. Debug "Unknown message type: 0" error
3. Fix message deserialization in client
4. Verify all ControlMessage types parse correctly
5. Test user list population in UI
6. Test multi-user join/leave notifications

**Testing:**
- Unit test: Message serialization/deserialization
- Integration test: 2-3 users joining channels
- UI test: User list updates in real-time

**Success Criteria:**
- ✅ No "Unknown message type" errors
- ✅ User list populates correctly
- ✅ Join/leave notifications work
- ✅ All WebSocket messages handled

---

### Phase 2: Database Integration 🗄️ CRITICAL
**Estimated Time:** 3-5 days
**Objective:** Migrate from in-memory/file storage to PostgreSQL

**Tasks:**
1. Set up PostgreSQL locally (Docker recommended)
2. Run database migrations (001_initial_schema.sql, 002_seed_data.sql)
3. Implement SQLx connection pool in server
4. Migrate UserRegistry to use database queries
5. Implement Channel persistence (CRUD operations)
6. Implement Organization management (basic CRUD)
7. Update server startup to initialize DB connection
8. Add database health checks

**Testing:**
- Unit test: Each database query function
- Integration test: User registration → DB → login
- Integration test: Channel creation → persistence → retrieval
- Load test: 100 concurrent user registrations
- Failure test: Database connection loss handling

**Success Criteria:**
- ✅ All user data persisted in PostgreSQL
- ✅ Server restart preserves data
- ✅ Channel hierarchy stored and retrieved
- ✅ Organizations support multi-tenancy
- ✅ No data loss on crashes

---

### Phase 3: TLS/HTTPS Support 🔒 HIGH PRIORITY
**Estimated Time:** 2-3 days
**Objective:** Secure control channel with TLS 1.3

**Tasks:**
1. Generate self-signed certificates for testing
2. Integrate rustls with Axum WebSocket server
3. Update server to load TLS certificates from config
4. Update client to use wss:// (WebSocket Secure)
5. Implement certificate validation in client
6. Add option for self-signed cert acceptance (dev mode)
7. Document production certificate setup (Let's Encrypt)

**Testing:**
- Unit test: Certificate loading
- Integration test: Client connects via wss://
- Security test: Verify TLS 1.3 negotiation
- Failure test: Invalid certificate rejection
- Cross-browser test: WebSocket secure from browser (future)

**Success Criteria:**
- ✅ Control channel encrypted with TLS 1.3
- ✅ Certificate validation working
- ✅ Self-signed certs work in dev mode
- ✅ Production cert instructions documented

---

### Phase 4: Password Security Fix 🔐 CRITICAL
**Estimated Time:** 1 day
**Objective:** Use Argon2 verification instead of plaintext

**Tasks:**
1. Update UserRegistry::authenticate() to use argon2::verify_password()
2. Migrate demo users to hashed passwords
3. Update registration to hash passwords before storage
4. Remove all plaintext password storage
5. Add password strength validation
6. Document password policy

**Testing:**
- Unit test: Password hashing and verification
- Unit test: Invalid password rejection
- Integration test: Login with hashed passwords
- Security test: Verify no plaintext passwords in DB/files
- Migration test: Old users can still login

**Success Criteria:**
- ✅ All passwords stored as Argon2 hashes
- ✅ Login verification uses argon2::verify_password()
- ✅ No plaintext passwords anywhere
- ✅ Password strength requirements enforced

---

### Phase 5: Permission/ACL Enforcement 👮 MEDIUM PRIORITY
**Estimated Time:** 2-3 days
**Objective:** Enforce role-based access control

**Tasks:**
1. Implement ACL checking in channel join handler
2. Implement speak permission verification
3. Add role assignment API
4. Implement channel password validation
5. Add permission error responses
6. Create default role templates (Admin, Member, Guest)
7. Test role inheritance in channel hierarchy

**Testing:**
- Unit test: Permission bitflag operations
- Unit test: ACL evaluation logic
- Integration test: User denied channel access
- Integration test: User can't speak without permission
- Integration test: Admin can override permissions
- Scenario test: Military operation with Command/Flight/Ground roles

**Success Criteria:**
- ✅ Users can't join channels without JOIN permission
- ✅ Users can't speak without SPEAK permission
- ✅ Channel passwords validated
- ✅ Role-based access working end-to-end

---

### Phase 6: Load Testing & Optimization 📊 CRITICAL
**Estimated Time:** 3-5 days
**Objective:** Verify system performance at scale

**Tasks:**
1. Create load test harness (simulate 50-100 users)
2. Measure baseline performance (CPU, memory, latency)
3. Test scenarios:
   - 50 users in single channel
   - 100 users across 10 channels
   - 10 users in 20 channels each (multi-channel load)
   - Network stress (packet loss, jitter, high latency)
4. Profile server routing performance
5. Profile client audio pipeline
6. Identify and fix bottlenecks
7. Optimize database queries (indexes, caching)
8. Implement connection pooling optimizations

**Testing:**
- Load test: 50 concurrent users, 30 min session
- Load test: 100 concurrent users, 10 min session
- Stress test: 200 users (expected to degrade gracefully)
- Network test: 10% packet loss, 200ms jitter
- Latency test: Measure end-to-end under load
- Memory test: Check for leaks over 1 hour

**Success Criteria:**
- ✅ 50 users: <150ms latency, <50% CPU
- ✅ 100 users: <200ms latency, <70% CPU
- ✅ No memory leaks
- ✅ Graceful degradation beyond limits
- ✅ Network resilience (5% packet loss acceptable)

---

## 🧪 Testing Strategy

### Test Levels

**1. Unit Tests (Per Phase)**
- Test individual functions and modules
- Run after every code change
- Target: 80% code coverage

**2. Integration Tests (After Each Phase)**
- Test component interactions
- Verify data flows end-to-end
- Run before moving to next phase

**3. System Tests (Phase 6)**
- Full system under realistic load
- Multi-user scenarios
- Network condition simulation

**4. Acceptance Tests (Final)**
- Real users in Star Citizen org scenario
- Command/Flight/Ground/Logistics channels
- 30+ minute operation simulation

### Test Automation

**Server Tests:**
```bash
cd server
cargo test                    # Unit tests
cargo test --test integration # Integration tests
cargo bench                   # Benchmarks
```

**Client Tests:**
```bash
cd client
cmake --build build --target test
./build/tests/test_opus_codec
./build/tests/test_jitter_buffer
```

**Manual Tests:**
- Multi-client test script
- Network condition simulator
- Audio quality assessment

---

## 📊 Progress Tracking

### Phase Checklist

- [ ] **Phase 1:** WebSocket Parsing (1-2 days)
- [ ] **Phase 2:** Database Integration (3-5 days)
- [ ] **Phase 3:** TLS Support (2-3 days)
- [ ] **Phase 4:** Password Security (1 day)
- [ ] **Phase 5:** Permissions (2-3 days)
- [ ] **Phase 6:** Load Testing (3-5 days)

**Total:** 12-19 days + testing buffer

### Milestone Gates

Each phase must pass these gates before proceeding:

1. ✅ All unit tests pass
2. ✅ Integration tests pass
3. ✅ No regressions in previous features
4. ✅ Documentation updated
5. ✅ Code reviewed (if team available)

---

## 🚨 Risk Mitigation

### Identified Risks

**Technical Risks:**

1. **Database Migration Complexity**
   - Risk: Data migration breaks existing users
   - Mitigation: Test with backup, rollback plan

2. **TLS Certificate Issues**
   - Risk: Certificate validation blocks legitimate clients
   - Mitigation: Dev mode with self-signed certs, clear error messages

3. **Performance Degradation**
   - Risk: Database queries slow down system
   - Mitigation: Profiling, connection pooling, caching

4. **Permission Logic Bugs**
   - Risk: Users locked out or unauthorized access
   - Mitigation: Thorough testing, default-deny policy

**Schedule Risks:**

1. **Scope Creep**
   - Risk: Adding features delays MVP
   - Mitigation: Strict phase boundaries, defer non-critical features

2. **Testing Time Underestimate**
   - Risk: Issues found late require rework
   - Mitigation: Test early and often, buffer time allocated

---

## 📈 Success Metrics

### MVP Success Criteria

**Functionality:**
- ✅ 100 concurrent users supported
- ✅ <150ms average latency
- ✅ Zero data loss
- ✅ All core features working

**Reliability:**
- ✅ 99% uptime over 24 hours
- ✅ Graceful error handling
- ✅ Automatic reconnection
- ✅ No memory leaks

**Security:**
- ✅ All traffic encrypted
- ✅ Strong authentication
- ✅ Permission enforcement
- ✅ No plaintext passwords

**Quality:**
- ✅ 80% test coverage
- ✅ Zero critical bugs
- ✅ <5 known minor bugs
- ✅ Documentation complete

---

## 🎯 Post-MVP Roadmap

### Phase 7: Beta Testing (2-3 weeks)
- Recruit 20-30 beta testers
- Real-world operations testing
- Bug fixes and polish
- Performance tuning

### Phase 8: Production Deployment (1-2 weeks)
- Set up production infrastructure
- Deploy to cloud (AWS/GCP/DigitalOcean)
- Configure monitoring (Prometheus, Grafana)
- Set up alerting and logging
- Create backup/recovery procedures

### Phase 9: Additional Features (Ongoing)
- In-game overlay
- Mobile client
- Web client
- Advanced audio processing (noise suppression, echo cancellation)
- Recording and playback
- Text chat
- File sharing

---

## 📝 Daily Development Workflow

### Each Development Day:

**Morning:**
1. Review previous day's progress
2. Run full test suite
3. Pick next task from current phase
4. Create feature branch (git)

**Development:**
1. Implement feature/fix
2. Write unit tests
3. Test locally
4. Document changes

**Evening:**
5. Run full test suite again
6. Integration test with existing features
7. Commit changes with descriptive message
8. Update progress in this document
9. Note any blockers or issues

---

## 🔧 Development Environment Setup

### Prerequisites

**Server:**
- Rust 1.75+ with Cargo
- PostgreSQL 15+
- Redis 7+ (optional, for later)
- OpenSSL development libraries

**Client:**
- C++20 compiler (MSVC 2022 or GCC 11+)
- Qt 6.10.1
- CMake 3.20+
- PortAudio, Opus, OpenSSL libraries

**Tools:**
- Docker & Docker Compose (for PostgreSQL)
- Git (version control)
- VS Code or CLion (recommended IDEs)

### Quick Setup

```bash
# Clone repository
git clone <repo-url>
cd VoIP-System

# Start PostgreSQL
docker-compose up -d postgres

# Build server
cd server
cargo build --release
cargo test

# Build client
cd ../client
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Run tests
cd build && ctest
```

---

## 📞 Communication & Reporting

### Daily Updates
- Update this document with progress
- Log issues in GitHub Issues
- Document decisions in ADRs (Architecture Decision Records)

### Blocking Issues
- Document in BLOCKERS.md
- Include steps to reproduce
- Proposed solutions
- Priority level

### Code Review
- Self-review checklist before commit
- Pair programming for complex features
- Test coverage verification

---

## 🎊 Definition of Done

A phase is "done" when:

1. ✅ All tasks completed
2. ✅ All tests passing (unit + integration)
3. ✅ No regressions in existing features
4. ✅ Documentation updated
5. ✅ Code committed and pushed
6. ✅ Demo prepared (if applicable)
7. ✅ Success criteria met

---

## 📅 Timeline

**Week 1-2:** Phases 1-4 (Critical fixes and infrastructure)
**Week 3-4:** Phases 5-6 (Permissions and load testing)
**Week 5-6:** Beta testing and polish
**Week 7-8:** Production deployment preparation

**Target MVP Completion:** End of Week 4
**Target Production Release:** End of Week 8

---

**Let's build an amazing VoIP system! 🚀**
