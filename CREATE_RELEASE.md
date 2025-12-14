# How to Create a GitHub Release for DadLink

## Automated Option (if you have GitHub CLI):

Install GitHub CLI from: https://cli.github.com/

Then run:
```bash
cd C:\dev\VoIP-System
gh release create v1.0.0 ^
  .\client\release-package\DadLink-1.0.0-Windows-x64.zip ^
  --title "DadLink v1.0.0 - Initial Release" ^
  --notes-file RELEASE_NOTES.md ^
  --latest
```

## Manual Option (via GitHub Web):

### Step 1: Go to GitHub Releases
1. Open https://github.com/DMShort/DadLink/releases
2. Click "Draft a new release"

### Step 2: Tag the Release
1. In "Choose a tag" field, type: `v1.0.0`
2. Click "Create new tag: v1.0.0 on publish"

### Step 3: Set Release Title
- Title: `DadLink v1.0.0 - Initial Release`

### Step 4: Add Release Notes
Copy and paste the content from `RELEASE_NOTES.md` into the description field

### Step 5: Upload the Client Package
1. Click "Attach binaries by dropping them here or selecting them"
2. Select the file: `client\release-package\DadLink-1.0.0-Windows-x64.zip`
3. Wait for upload to complete (70 MB)

### Step 6: Publish
1. Check "Set as the latest release"
2. Click "Publish release"

## After Publishing

Users can download the client from:
https://github.com/DMShort/DadLink/releases/latest

The download URL will be:
https://github.com/DMShort/DadLink/releases/download/v1.0.0/DadLink-1.0.0-Windows-x64.zip

## Testing the Release

Before sharing widely:
1. Download the ZIP from GitHub (test the download link works)
2. Extract on a clean Windows machine (without Qt installed)
3. Verify DadLink.exe launches and connects to your server
4. Test the global hotkeys work while using another application

## Sharing with Users

Once tested, you can share:
- Direct link: https://github.com/DMShort/DadLink/releases/latest
- Or embed download button on your website
- Or post to Discord/forums with installation instructions

## Files Created

✅ `client\release-package\DadLink-1.0.0-Windows-x64.zip` (70 MB)
   - Contains: DadLink.exe + all Qt DLLs + OpenSSL + README.txt

✅ `RELEASE_NOTES.md`
   - Release notes with features, installation guide, troubleshooting

✅ `client\package-release.ps1`
   - Script to package future releases
