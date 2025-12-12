# VoIP Client - Distribution Guide

## 📦 Quick Distribution

### Option 1: Portable ZIP (Recommended)

**Your ready-to-distribute file:**
```
C:\dev\VoIP-System\client\VoIP-Client-v0.1.0.zip (16 MB)
```

**How to distribute:**
1. Upload to file sharing service (Google Drive, Dropbox, OneDrive, etc.)
2. Share the download link
3. Recipients extract and run `voip-client.exe`

**No installation required!** Users just extract and run.

---

## 🔄 Rebuilding the Package

Whenever you make changes to the client:

```cmd
cd C:\dev\VoIP-System\client

REM 1. Build Release version
cmake --build build --config Release

REM 2. Run deployment script
deploy.bat

REM 3. ZIP is created automatically at:
REM    C:\dev\VoIP-System\client\VoIP-Client-v0.1.0.zip
```

---

## 📤 Distribution Methods

### 1. Direct Download (Simple)

**Best for:** Small groups, testing

Upload `VoIP-Client-v0.1.0.zip` to:
- **Google Drive**: Share link → Anyone with link can view
- **Dropbox**: Share link
- **OneDrive**: Share link
- **WeTransfer**: Send up to 2GB free
- **Your own web server**: Direct download link

### 2. GitHub Releases (Professional)

**Best for:** Open source projects, versioning

```bash
# 1. Create a GitHub repo (if not already)
git init
git add .
git commit -m "Initial commit"
git remote add origin https://github.com/yourusername/voip-client.git
git push -u origin main

# 2. Create a release
# Go to GitHub → Releases → Create new release
# - Tag: v0.1.0
# - Title: "VoIP Client v0.1.0 - Initial Release"
# - Upload: VoIP-Client-v0.1.0.zip
# - Add release notes
```

### 3. Self-Hosted Web Server

**Best for:** Full control

```bash
# Example: nginx configuration
location /downloads {
    alias /var/www/downloads;
    autoindex on;
}

# Upload ZIP and provide link:
# https://yourserver.com/downloads/VoIP-Client-v0.1.0.zip
```

---

## 🎯 Professional Installer (Optional)

### Create .exe Installer with Inno Setup

**1. Install Inno Setup:**
- Download: https://jrsoftware.org/isinfo.php
- Install (free)

**2. Create installer script** (`installer.iss`):

```iss
[Setup]
AppName=VoIP Client
AppVersion=0.1.0
DefaultDirName={pf}\VoIP Client
DefaultGroupName=VoIP Client
OutputBaseFilename=VoIP-Client-Setup-v0.1.0
OutputDir=.
Compression=lzma2
SolidCompression=yes

[Files]
Source: "deploy\VoIP-Client\*"; DestDir: "{app}"; Flags: recursesubdirs

[Icons]
Name: "{group}\VoIP Client"; Filename: "{app}\voip-client.exe"
Name: "{commondesktop}\VoIP Client"; Filename: "{app}\voip-client.exe"

[Run]
Filename: "{app}\voip-client.exe"; Description: "Launch VoIP Client"; Flags: postinstall nowait skipifsilent
```

**3. Compile:**
- Open `installer.iss` in Inno Setup
- Build → Compile
- Output: `VoIP-Client-Setup-v0.1.0.exe`

**Benefits:**
- Professional installation experience
- Creates Start Menu shortcuts
- Desktop icon
- Uninstaller included
- ~16MB installer size

---

## 📋 What Users Need

### System Requirements
- **OS**: Windows 10/11 (64-bit)
- **RAM**: 100 MB minimum
- **Disk**: 50 MB
- **Network**: Internet connection
- **Audio**: Microphone + speakers/headphones

### User Instructions

**Download & Extract:**
```
1. Download VoIP-Client-v0.1.0.zip
2. Extract to any folder
3. Run voip-client.exe
```

**First Time Setup:**
```
1. Enter server address (e.g., 122.150.216.145)
2. Port: 9000
3. ✅ Check "Use TLS"
4. Click "Register" to create account
5. Login with your credentials
```

**Firewall Prompt:**
- Windows may ask for firewall permission
- Click "Allow" for both Private and Public networks

---

## 🌐 Hosting Your Own Server

If users want to host their own server:

```bash
# Server requirements:
- Rust 1.70+ (Windows/Linux/macOS)
- PostgreSQL database
- Ports: 9000 (TCP), 9001 (UDP)

# Quick start:
cd C:\dev\VoIP-System\server
cargo run --release

# Configuration: server/config/server.toml
```

**Server distribution:**
- Share server binary: `target/release/voip_server.exe`
- Include: `config/`, `certs/`, `.env.example`
- Database setup instructions

---

## 🔐 Security Notes for Users

**Included in README.txt:**
- All voice is end-to-end encrypted (SRTP + AES-128-GCM)
- Perfect Forward Secrecy (ephemeral keys)
- Server cannot decrypt voice packets
- TLS for control channel

**Privacy:**
- No telemetry or analytics
- No data collection
- Open source (you can verify)

---

## 📊 Distribution Checklist

Before releasing to users:

- [ ] Test on clean Windows 10/11
- [ ] Verify all DLLs included
- [ ] Test server connection
- [ ] Test voice transmission
- [ ] Check README.txt is clear
- [ ] Verify file size reasonable (~16 MB)
- [ ] Scan with antivirus (optional)
- [ ] Code signing certificate (optional, for trust)

---

## 🚀 Advanced: Code Signing (Optional)

**Why:** Prevents "Unknown Publisher" warnings

**How:**
1. Get code signing certificate ($50-300/year)
   - DigiCert, Sectigo, Comodo
2. Sign executable:
   ```cmd
   signtool sign /f cert.pfx /p password /t http://timestamp.digicert.com deploy\VoIP-Client\voip-client.exe
   ```
3. Rebuild ZIP

---

## 📈 Version Management

**Semantic Versioning:**
- `v0.1.0` - Initial release
- `v0.2.0` - New features (SRTP added)
- `v0.2.1` - Bug fixes
- `v1.0.0` - Stable release

**Update process:**
1. Update version in code
2. Build → Deploy → ZIP
3. Create GitHub release
4. Notify users

---

## 💡 Distribution Tips

1. **Naming:** Use clear version numbers
   - ✅ `VoIP-Client-v0.1.0.zip`
   - ❌ `voip-final-FINAL-v2.zip`

2. **Changelog:** Include what's new
   ```
   v0.1.0 (2025-01-09)
   - Initial release
   - End-to-end encryption (SRTP)
   - Multi-channel support
   - Push-to-talk
   ```

3. **Support:** Provide contact method
   - GitHub Issues
   - Discord server
   - Email

4. **License:** Choose a license
   - MIT (permissive)
   - GPL (copyleft)
   - Proprietary (closed source)

---

## ✅ You're Ready!

**Current distribution file:**
```
📦 C:\dev\VoIP-System\client\VoIP-Client-v0.1.0.zip (16 MB)
```

**Share this file and users can:**
1. Extract
2. Run `voip-client.exe`
3. Connect to your server (122.150.216.145:9000)
4. Enjoy encrypted voice chat!

**No installation, no admin rights, no hassle.** 🎉
