# VoIP Client Resources

This directory contains all embedded resources for the VoIP client application.

## Directory Structure

```
resources/
├── icons/          # Application and UI icons
├── images/         # Logos, backgrounds, graphics
├── styles/         # Qt stylesheets (.qss)
├── sounds/         # Audio effects
└── resources.qrc   # Qt Resource Collection file
```

## Adding Your Own Assets

### 1. Add Files to Appropriate Folder

```
resources/icons/your_icon.png
resources/images/your_logo.svg
```

### 2. Register in resources.qrc

```xml
<qresource prefix="/icons">
    <file>icons/your_icon.png</file>
</qresource>
```

### 3. Use in Code

```cpp
// Set window icon
setWindowIcon(QIcon(":/icons/app_icon.png"));

// Set button icon
button->setIcon(QIcon(":/icons/mic_on.png"));

// Load image
QPixmap logo(":/images/logo_large.png");

// Apply stylesheet
QFile styleFile(":/styles/dark_theme.qss");
styleFile.open(QFile::ReadOnly);
qApp->setStyleSheet(styleFile.readAll());
```

## Supported Formats

### Icons
- **PNG** - Recommended for most icons
- **SVG** - Scalable vector graphics (best for high-DPI)
- **ICO** - Windows icon format

### Images
- **PNG** - Best for logos with transparency
- **JPG** - Backgrounds, photos
- **SVG** - Scalable graphics

### Audio
- **WAV** - Uncompressed (best quality, larger size)
- **MP3** - Compressed (smaller size)

## Icon Sizes Recommended

- **App Icon**: 256x256, 128x128, 64x64, 32x32, 16x16 (provide multiple sizes)
- **Toolbar Icons**: 24x24 or 32x32
- **List Icons**: 16x16 or 20x20
- **User Avatars**: 48x48 or 64x64
- **Logo**: 256x256 or larger (SVG preferred)

## Creating Placeholder Icons

If you don't have custom icons yet, you can:

1. **Use emoji** - Qt supports emoji: 🎤 🔇 🔊 ⚙️
2. **Use icon fonts** - Font Awesome, Material Icons
3. **Generate online** - flaticon.com, iconfinder.com
4. **AI generate** - DALL-E, Midjourney for custom designs

## Theme Colors (Dark Theme)

```
Background:  #2b2d31
Foreground:  #dcddde
Accent:      #5865f2 (Discord blue)
Success:     #23a55a (Green)
Warning:     #f0b232 (Yellow)
Error:       #da373c (Red)
```

## Example: Adding a Custom Logo

```bash
# 1. Save your logo as PNG or SVG
cp my_logo.png resources/images/app_logo.png

# 2. Add to resources.qrc
# (Already done in the template above)

# 3. Use in login_dialog.cpp
QPixmap logo(":/images/app_logo.png");
QLabel* logoLabel = new QLabel();
logoLabel->setPixmap(logo.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
```

## Next Steps

1. Create or download your custom icons
2. Place them in the appropriate folders
3. Rebuild the project (`cmake --build build`)
4. Icons will be embedded in the executable!
