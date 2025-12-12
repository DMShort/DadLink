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
 * - Registers hotkeys for channels
 * - Detects key press/release globally
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
    
protected:
    /**
     * Event filter to capture key events globally
     */
    bool eventFilter(QObject* obj, QEvent* event) override;
    
private:
    // Map: ChannelId -> QKeySequence
    std::map<ChannelId, QKeySequence> channel_hotkeys_;
    
    // Map: Qt::Key -> ChannelId (for fast lookup)
    std::map<Qt::Key, ChannelId> key_to_channel_;
    
    // Set of currently pressed keys
    std::set<Qt::Key> pressed_keys_;
    
    // Parent widget (to install event filter)
    QWidget* parent_widget_;
};

} // namespace voip::ui
