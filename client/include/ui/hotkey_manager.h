#pragma once

#include <QObject>
#include <QKeySequence>
#include <QEvent>
#include <QKeyEvent>
#include <map>
#include <set>
#include "common/types.h"

namespace voip::ui {

/**
 * HotkeyManager - Global hotkey handler for PTT functionality
 *
 * Features:
 * - Registers hotkeys for channels SYSTEM-WIDE (works even when app is in background)
 * - Detects key press/release globally using Windows low-level keyboard hook
 * - Emits signals for PTT start/stop
 * - Supports simultaneous key presses
 *
 * Usage:
 *   hotkeyManager->registerHotkey(1, QKeySequence(Qt::Key_F1));
 *   connect(hotkeyManager, &HotkeyManager::hotkeyPressed, ...);
 */
class HotkeyManager : public QObject {
    Q_OBJECT
    
public:
    explicit HotkeyManager(QWidget* parent = nullptr);
    ~HotkeyManager() override;
    
    /**
     * Register a hotkey for a channel
     * Returns true if successful, false if key is already in use
     */
    bool registerHotkey(ChannelId channel_id, const QKeySequence& key);
    
    /**
     * Unregister hotkey for a channel
     */
    void unregisterHotkey(ChannelId channel_id);
    
    /**
     * Get the hotkey assigned to a channel
     * Returns empty QKeySequence if no hotkey assigned
     */
    QKeySequence getHotkey(ChannelId channel_id) const;
    
    /**
     * Get the channel assigned to a key
     * Returns 0 if no channel assigned
     */
    ChannelId getChannelForKey(Qt::Key key) const;
    
    /**
     * Check if a key is currently pressed
     */
    bool isKeyPressed(Qt::Key key) const;
    
    /**
     * Get all currently pressed keys
     */
    std::set<Qt::Key> getPressedKeys() const;
    
    /**
     * Get all registered hotkeys
     */
    std::map<ChannelId, QKeySequence> getAllHotkeys() const;
    
    /**
     * Clear all hotkeys
     */
    void clearAllHotkeys();

    /**
     * Handle key press from Windows hook (public for global callback)
     * Returns true if the key was a registered hotkey (consumed)
     */
    bool handleKeyPress(unsigned long vkCode);

    /**
     * Handle key release from Windows hook (public for global callback)
     * Returns true if the key was a registered hotkey (consumed)
     */
    bool handleKeyRelease(unsigned long vkCode);

signals:
    /**
     * Emitted when a registered hotkey is pressed
     */
    void hotkeyPressed(ChannelId channel_id);
    
    /**
     * Emitted when a registered hotkey is released
     */
    void hotkeyReleased(ChannelId channel_id);
    
    /**
     * Emitted when hotkey registration changes
     */
    void hotkeysChanged();
    
private:
    // Convert Qt::Key to Windows virtual key code
    int qtKeyToVirtualKey(Qt::Key key);

    // Map: ChannelId -> QKeySequence
    std::map<ChannelId, QKeySequence> channel_hotkeys_;

    // Map: Qt::Key -> ChannelId (for fast lookup)
    std::map<Qt::Key, ChannelId> key_to_channel_;

    // Map: Windows VK code -> ChannelId
    std::map<int, ChannelId> vk_to_channel_;

    // Set of currently pressed keys
    std::set<Qt::Key> pressed_keys_;

    // Parent widget (for getting window handle)
    QWidget* parent_widget_;

    // Counter for generating unique hotkey IDs
    int next_hotkey_id_;

#ifdef _WIN32
    // Windows keyboard hook handle (HHOOK - opaque pointer)
    void* keyboard_hook_;
#endif
};

} // namespace voip::ui
