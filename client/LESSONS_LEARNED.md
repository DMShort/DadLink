# Lessons Learned - VoIP Client Development

## 🐛 Critical Fixes & Solutions

### 1. Struct Packing for Network Packets
**Problem**: Voice packets were corrupted due to struct padding  
**Solution**: Use `#pragma pack(push, 1)` and `#pragma pack(pop)`

```cpp
#pragma pack(push, 1)
struct VoicePacketHeader {
    uint32_t magic;         // offset 0, 4 bytes
    SequenceNumber sequence; // offset 4, 8 bytes  
    uint64_t timestamp;     // offset 12, 8 bytes
    ChannelId channel_id;   // offset 20, 4 bytes
    UserId user_id;         // offset 24, 4 bytes
};                          // Total: 28 bytes (no padding!)
#pragma pack(pop)
```

**Location**: `client/include/common/types.h`  
**Impact**: Prevents 4-byte padding that would corrupt network transmission

---

### 2. Audio Level Meter Responsiveness
**Problem**: RMS values are naturally low (0.1-0.3 for speech), meters barely moved  
**Solution**: Apply 4x gain multiplier for visualization

```cpp
float AudioEngine::calculate_rms(const float* pcm, size_t count) const noexcept {
    float sum_squares = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        sum_squares += pcm[i] * pcm[i];
    }
    float rms = std::sqrt(sum_squares / static_cast<float>(count));
    
    // Apply 4x gain for better visualization
    rms *= 4.0f;
    
    // Clamp to 0.0-1.0
    return std::min(rms, 1.0f);
}
```

**Location**: `client/src/audio/audio_engine.cpp`  
**Result**: Meters now show 40-80% during normal speech instead of 10-20%

---

### 3. Qt Stylesheet Conflicts
**Problem**: Labels invisible due to conflicting stylesheets  
**Solution**: Use brighter color in global stylesheet + transparent background

```cpp
// In ui_main.cpp fallback stylesheet
QLabel {
    color: #dcddde;        // Brighter gray (was #b5bac1)
    background: transparent;  // Prevent background override
}
```

**Location**: `client/src/ui_main.cpp`  
**Impact**: Labels now visible across all dialogs and windows

---

### 4. DLL Version Conflicts
**Problem**: Old Qt DLLs from vcpkg conflicted with deployment  
**Solution**: Manual deployment scripts with version verification

```batch
REM Clean old DLLs before windeployqt
del /Q build\Debug\Qt*.dll 2>nul
windeployqt build\Debug\voip-client.exe --debug --no-translations
```

**Location**: `client/deploy.bat`  
**Tools**: `build_and_deploy.bat`, `verify_dlls.ps1`

---

### 5. UI Layout - Label Visibility
**Problem**: Labels cut off horizontally and vertically  
**Solution**: Fixed width + minimum height

```cpp
auto* usernameLabel = new QLabel("Username:", credentialsGroup);
usernameLabel->setFixedWidth(110);      // Prevent horizontal cutoff
usernameLabel->setMinimumHeight(30);    // Prevent vertical cutoff
usernameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
```

**Location**: `client/src/ui/login_dialog.cpp`

---

## 🎯 Best Practices Established

### Real-Time Audio Safety
1. ✅ NO heap allocation in audio callbacks
2. ✅ NO mutex locks in audio callbacks  
3. ✅ NO I/O operations in audio callbacks
4. ✅ Use lock-free queues for cross-thread communication
5. ✅ Pre-allocate all buffers before starting audio

### Error Handling
- Use `Result<T>` for all operations that can fail
- Log errors with context (file, function, line)
- Check `Result.is_ok()` before using `.value()`

### Network Programming
- Always use `#pragma pack(push, 1)` for network structs
- Validate packet magic number
- Check sequence numbers for packet loss
- Handle UDP unreliability with jitter buffer + PLC

### Qt UI Development
- Apply styles at dialog level, not widget level
- Use fixed widths for labels in forms
- Set minimum heights to prevent text cutoff
- Test with different DPI scaling
- Always check global stylesheet conflicts

---

## 🔧 Build & Deployment

### Windows Build Process
```powershell
# 1. Stop any running instances
Stop-Process -Name "voip-client" -Force -ErrorAction SilentlyContinue

# 2. Build with CMake
cmake --build build --config Debug --target voip-client

# 3. Deploy DLLs
.\deploy.bat

# 4. Verify DLLs
.\verify_dlls.ps1
```

### Common Build Issues
| Issue | Solution |
|---|---|
| LNK1168: Cannot open .exe | Stop running process first |
| Missing Qt DLLs | Run `deploy.bat` |
| Wrong Qt version | Run `verify_dlls.ps1` |
| Missing opus/portaudio | Copy from vcpkg manually |

---

## 📊 Performance Metrics

### Audio Pipeline Latency
- **Capture**: ~20ms (960 samples @ 48kHz)
- **Encoding**: <5ms (Opus optimized)
- **Network**: Variable (typically 10-50ms)
- **Jitter Buffer**: ~100ms (5 frames)
- **Decoding**: <5ms
- **Playback**: ~20ms
- **Total**: 150-200ms (acceptable for voice)

### Audio Quality Settings
```cpp
VoiceSession::Config config;
config.sample_rate = 48000;      // 48kHz (standard for VoIP)
config.frame_size = 960;         // 20ms frames
config.channels = 1;             // Mono
config.bitrate = 32000;          // 32 kbps (good quality)
config.enable_fec = true;        // Forward Error Correction
config.enable_dtx = false;       // No silence suppression (for gaming)
config.jitter_buffer_frames = 5; // ~100ms buffer
```

---

## 🚨 Known Issues

### WebSocket User List
**Issue**: User list shows only yourself  
**Cause**: WebSocket message parser issue ("Unknown message type: 0")  
**Impact**: LOW - Voice communication works perfectly  
**Workaround**: None needed, users can communicate without seeing list  
**Fix**: Move WebSocket init after Qt event loop or fix message parser

### Console Output Noise
**Issue**: PortAudio logs verbose device enumeration  
**Impact**: Cosmetic only  
**Fix**: Filter PortAudio logs or redirect to file

---

## 🎓 Key Learnings

### What Worked Well
1. **Lock-free design**: Audio thread never blocks
2. **Opus codec**: Excellent quality at 32kbps
3. **Jitter buffer**: Smooths out network jitter effectively
4. **Qt6**: Beautiful UI with minimal code
5. **Result<T>**: Clean error handling without exceptions

### What Could Be Improved
1. **WebSocket integration**: Should initialize after event loop
2. **Error messages**: Need more context in logs
3. **Testing**: Need automated audio pipeline tests
4. **Configuration**: Hardcoded values should be in config file
5. **Documentation**: Inline comments for complex logic

---

## 📝 Code Patterns to Follow

### Resource Management
```cpp
class AudioEngine {
    // Use RAII - acquire in constructor, release in destructor
    AudioEngine();
    ~AudioEngine() { shutdown(); }
    
    // Disable copy, enable move
    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;
    
    // Explicit initialization
    Result<void> initialize(const Config& config);
    void shutdown();
};
```

### Thread-Safe State
```cpp
class VoiceSession {
private:
    // Use atomics for simple state
    std::atomic<bool> active_{false};
    std::atomic<uint64_t> frames_captured_{0};
    
    // Use lock-free queues for data
    LockFreeQueue<AudioBuffer> audio_queue_;
};
```

### Callbacks from Audio Thread
```cpp
// GOOD: Lock-free, no allocation
void on_audio_captured(const float* pcm, size_t frames) {
    audio_queue_.push({pcm, frames});  // Lock-free queue
}

// BAD: Mutex lock in audio thread!
void on_audio_captured(const float* pcm, size_t frames) {
    std::lock_guard lock(mutex_);  // BLOCKS! Don't do this!
    buffer_.push_back({pcm, frames});
}
```

---

**Last Updated**: November 23, 2025  
**Status**: Core voice system fully operational, WebSocket user list pending
