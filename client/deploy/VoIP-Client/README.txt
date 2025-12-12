VoIP Client - Encrypted Voice Communication
============================================

QUICK START
-----------
1. Run voip-client.exe
2. Enter server address (IP or domain)
3. Enter port (default: 9000)
4. Check "Use TLS" for encrypted connection
5. Login or Register

FEATURES
--------
- End-to-end encrypted voice (SRTP with AES-128-GCM)
- Perfect Forward Secrecy (ephemeral X25519 keys)
- Multi-channel voice chat
- Push-to-talk with customizable hotkeys
- Low latency (Opus codec, ~20ms)
- Jitter buffer for network stability

SYSTEM REQUIREMENTS
-------------------
- Windows 10/11 (64-bit)
- Audio input device (microphone)
- Audio output device (speakers/headphones)
- Internet connection
- 500 KB/s recommended bandwidth

CONTROLS
--------
- Space: Push-to-talk (default, customizable)
- M: Toggle mute
- D: Toggle deafen

SERVER CONNECTION
-----------------
The server uses two ports:
- TCP 9000: Control channel (WebSocket over TLS)
- UDP 9001: Voice packets (encrypted with SRTP)

Example server: 122.150.216.145:9000

TROUBLESHOOTING
---------------
Connection Failed:
  - Verify server address and port
  - Check firewall settings
  - Ensure server is running

No Audio:
  - Check microphone permissions
  - Verify audio device in Settings
  - Check volume levels

High Latency:
  - Check network connection
  - Reduce bandwidth usage
  - Server may be distant

SECURITY NOTES
--------------
- All voice packets are encrypted end-to-end
- Key exchange uses X25519 elliptic curve
- Encryption uses AES-128-GCM authenticated encryption
- Replay protection prevents packet replay attacks
- Each session uses unique ephemeral keys

For more information: https://github.com/yourusername/voip-system
