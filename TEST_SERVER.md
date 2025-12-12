# Test the Server

## Step 1: Restart PowerShell
```powershell
# Close and reopen PowerShell to get Rust in PATH
```

## Step 2: Build and Test
```powershell
cd server

# Build the project
cargo build

# Run tests
cargo test

# Run the server
cargo run
```

## Expected Output

### Tests:
```
running 4 tests
test auth::tests::test_password_hash_and_verify ... ok
test auth::tests::test_jwt_create_and_verify ... ok
test auth::tests::test_jwt_invalid_secret ... ok
test network::udp::tests::test_parse_valid_packet ... ok

test result: ok. 4 passed; 0 failed
```

### Server Running:
```
VoIP Server v0.1.0
Configuration loaded
Control port: 9000
Voice port: 9001
Starting UDP voice server on 0.0.0.0:9001
✅ UDP voice server started
Starting WebSocket control server on 0.0.0.0:9000
✅ WebSocket control server started
🚀 Server initialization complete
📡 Voice: UDP port 9001
🔐 Control: WebSocket port 9000

Press Ctrl+C to shutdown
```

## What You Can Do Now

### 1. Test with curl (WebSocket won't work, but connection test)
```powershell
# This will fail but shows server is listening
curl http://localhost:9000/control
```

### 2. View the Code
- `src/network/udp.rs` - UDP voice packet handler
- `src/network/tls.rs` - WebSocket control handler
- `src/auth.rs` - Authentication system (with tests!)

### 3. Explore the Database Schema
- `migrations/001_initial_schema.sql` - Complete database structure
- `migrations/002_seed_data.sql` - Demo data

## Next Steps While Qt6 Installs

1. **Database Setup** (optional)
   ```powershell
   # Install PostgreSQL if not already
   # Create database
   # Run migrations
   ```

2. **Add More Tests**
   - Network protocol tests
   - WebSocket message handling
   - Packet routing logic

3. **Review Documentation**
   - Read `NETWORK_PROTOCOL.md`
   - Study `SERVER_DESIGN.md`
   - Check `SECURITY_ARCHITECTURE.md`

## Troubleshooting

### "cargo not found"
→ Restart PowerShell after Rust install

### Port already in use
→ Change ports in code or kill existing process

### Tests fail
→ Check error messages, likely dependency issue

---

**Status**: Server code complete, ready to test! 🚀
