# DadLink v1.0.0 - Initial Release 🎉

## What is DadLink?

DadLink is a low-latency voice communication system designed for gaming and team coordination. It features **global hotkeys** that work even when you're playing games, allowing seamless push-to-talk functionality without alt-tabbing.

## Key Features

### 🎮 Global Hotkey System
- **Works while gaming!** Press F1-F10 to talk even when DadLink is minimized
- Uses Windows low-level keyboard hooks for system-wide hotkey capture
- Perfect for gaming, streaming, or any situation where you need background voice comms

### 🎤 Multi-Channel Voice
- Join multiple channels simultaneously
- Push-to-talk on individual channels (F1-F10)
- Listen to multiple channels at once
- Low-latency audio streaming with Opus codec

### 🔐 Security
- TLS encrypted connections
- SRTP encryption for voice packets
- Secure authentication system

### 👥 Admin Panel
- Full channel management (create, edit, delete)
- User management with role-based permissions
- Role creation with granular permission control
- Real-time updates when channels are modified

## Installation

### Windows 10/11 (64-bit)

1. **Download** `DadLink-1.0.0-Windows-x64.zip` from the Assets below
2. **Extract** the ZIP file to a folder (e.g., `C:\Program Files\DadLink`)
3. **Install** [Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe) if you don't have it
4. **Run** `DadLink.exe`

## Quick Start Guide

1. **Launch DadLink**
2. **Connect to your server:**
   - Enter your server address (e.g., `voip.yourserver.com`)
   - Port: `9000` (default)
   - Enable "Use TLS" for secure connection
3. **Login** with your credentials
4. **Join channels** from the Voice tab
5. **Use F1-F10 keys** to talk on different channels (works even when minimized!)

## Usage Tips

### Global Hotkeys
- Hotkeys work **system-wide** - you can press them while playing any game
- Default assignments: F1-F10 for channels 1-10
- The client must be running (can be minimized)

### Multi-Channel Setup
Example gaming setup:
- **F1**: General chat (listen always)
- **F2**: Squad Alpha (talk when in squad)
- **F3**: Squad Bravo
- **F4**: Command channel (admins only)

Press and hold the key to talk on that channel, release to stop.

## System Requirements

- **OS**: Windows 10 or Windows 11 (64-bit)
- **Memory**: 100 MB RAM
- **Disk**: 100 MB free space
- **Network**: Broadband internet connection
- **Prerequisites**: Visual C++ Redistributable 2015-2022

## Known Issues

- Voice loopback test app has linker errors (doesn't affect main client)
- Some games with anti-cheat might conflict with keyboard hooks (run DadLink as admin)

## Troubleshooting

**Application won't start:**
- Install the [VC++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe)

**Hotkeys not working:**
- Ensure DadLink is running (check system tray)
- Try running as Administrator
- Check if another program is using the same keys

**Cannot connect to server:**
- Verify server address and port
- Ensure TLS is enabled if required
- Check firewall settings

## Server Setup

To run your own DadLink server, see the [Server Documentation](https://github.com/DMShort/DadLink#server-setup).

## Changelog

### Features
- ✨ Global hotkey system using Windows keyboard hooks
- ✨ Multi-channel voice communication
- ✨ Admin panel with full CRUD for channels/users/roles
- ✨ TLS and SRTP encryption
- ✨ Voice tab auto-refresh when admin modifies channels
- ✨ F1-F10 hotkey support for up to 10 channels

### Bug Fixes
- 🐛 Fixed API response parsing for channel/user/role updates
- 🐛 Fixed voice tab not updating after admin changes
- 🐛 Fixed hotkeys only working when app has focus

## Credits

Built with:
- Qt 6.10.1 for the UI framework
- Opus codec for audio compression
- Windows Winsock for networking
- OpenSSL for TLS encryption

---

**Support**: [GitHub Issues](https://github.com/DMShort/DadLink/issues)
**Documentation**: [README](https://github.com/DMShort/DadLink)

🤖 Built with Claude Code
