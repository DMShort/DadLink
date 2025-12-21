# DadLink v1.0.3 - Enhanced UI & UX Update ✨

## What's New

### Channel Roster Improvements
- ✨ **Visual Channel Hierarchy** - Sub-channels are now visually indented, making it easy to see which channels belong together
  - Example: "Operations" channel with "Alpha Squad" and "Bravo Squad" as sub-channels
  - Clear visual organization helps you understand team structure at a glance
- ✨ **Expand/Collapse Controls** - Discord-style expand arrows for managing channel visibility
  - Clean, professional button styling with hover effects
  - Intuitive Discord-blue accent on hover
- ✨ **User Count Badges** - See how many users are in each channel at a glance
  - Compact badge design that doesn't clutter the interface
  - Hidden when no users are present

### Improved Hotkey System
- 🎯 **Better Hotkey Capture** - More reliable hotkey input dialog
  - Clear visual feedback when capturing a new hotkey
  - Prevents conflicts with system keys
- 🎯 **Enhanced Hotkey Manager** - Improved internal hotkey management for better reliability

### Server Compatibility
- 🔧 Updated server to v1.0.3 for full compatibility with new client features
- 🔧 Enhanced TLS handling for more reliable secure connections

## Upgrading from 1.0.2

Download the new version and replace your existing DadLink folder. All settings and channel configurations remain fully compatible.

---

# DadLink v1.0.2 - Portable Release 🎉

## What's New

### Fully Portable - No VC++ Required!
- ✨ **True portability** - No Visual C++ Redistributable installation needed
- Application now uses static MSVC runtime (/MT)
- Extract ZIP and run - that's it!
- Perfect for USB drives, network shares, or any portable use case
- Same 70MB package size

### Includes All Previous Fixes
- Registration TLS bug fix from v1.0.1
- All features from v1.0.0

## Upgrading from 1.0.0 or 1.0.1

Download the new version and replace your existing DadLink folder. All settings remain compatible.

---

# DadLink v1.0.1 - Bug Fix Release

## What's Fixed

### Registration Bug
- 🐛 **Fixed registration freeze** - Registration now properly respects the "Use TLS" checkbox
- Previously, registration always attempted plain WebSocket (ws://) connection regardless of TLS setting
- This caused freezes when connecting to servers that require TLS
- Registration now works correctly with both secure (wss://) and insecure (ws://) servers

## Upgrading from 1.0.0

Simply download the new version and replace your existing DadLink.exe. All settings and data remain compatible.

---

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

1. **Get your server details** from your DadLink server administrator:
   - Server address
   - Port number (usually 9000)
   - Your username and password

2. **Launch DadLink**

3. **Connect to your server:**
   - Enter the server address provided by your admin
   - Enter the port number (default: 9000)
   - Enable "Use TLS" (recommended)
   - Click Connect

4. **Login** with your credentials

5. **Join channels** from the Voice tab

6. **Use F1-F10 keys** to talk on different channels (works even when minimized!)

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
- **Prerequisites**: None! Fully portable (v1.0.2+)

## Known Issues

- Voice loopback test app has linker errors (doesn't affect main client)
- Some games with anti-cheat might conflict with keyboard hooks (run DadLink as admin)

## Troubleshooting

**Application won't start:**
- Check Windows Defender/antivirus (may need to add exception)
- Try running as Administrator
- For v1.0.0-1.0.1: Install the [VC++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe)

**Hotkeys not working:**
- Ensure DadLink is running (check system tray)
- Try running as Administrator
- Check if another program is using the same keys

**Cannot connect to server:**
- Verify server address and port
- Ensure TLS is enabled if required
- Check firewall settings
- Contact your server administrator for connection details

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
