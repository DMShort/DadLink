#include "ui/hotkey_manager.h"
#include <QApplication>
#include <QWidget>
#include <iostream>

namespace voip::ui {

HotkeyManager::HotkeyManager(QWidget* parent)
    : QObject(qobject_cast<QObject*>(parent)), parent_widget_(parent) {
    
    // Install event filter on application to capture all key events
    if (parent_widget_) {
        parent_widget_->installEventFilter(this);
    } else {
        // If no parent, install on QApplication for truly global capture
        QApplication::instance()->installEventFilter(this);
    }
    
    std::cout << "✅ HotkeyManager initialized\n";
}

HotkeyManager::~HotkeyManager() {
    // Event filter is automatically removed when QObject is destroyed
}

bool HotkeyManager::registerHotkey(ChannelId channel_id, const QKeySequence& key) {
    if (key.isEmpty()) {
        std::cerr << "❌ Cannot register empty hotkey for channel " << channel_id << "\n";
        return false;
    }
    
    // Extract Qt::Key from QKeySequence
    Qt::Key qtKey = static_cast<Qt::Key>(key[0].key());
    
    // Check if this key is already assigned to another channel
    auto it = key_to_channel_.find(qtKey);
    if (it != key_to_channel_.end() && it->second != channel_id) {
        std::cerr << "❌ Key already assigned to channel " << it->second << "\n";
        return false;
    }
    
    // Unregister old hotkey if channel had one
    auto old_it = channel_hotkeys_.find(channel_id);
    if (old_it != channel_hotkeys_.end()) {
        Qt::Key old_key = static_cast<Qt::Key>(old_it->second[0].key());
        key_to_channel_.erase(old_key);
    }
    
    // Register new hotkey
    channel_hotkeys_[channel_id] = key;
    key_to_channel_[qtKey] = channel_id;
    
    std::cout << "⌨️ Registered hotkey " << key.toString().toStdString() 
              << " for channel " << channel_id << "\n";
    
    emit hotkeysChanged();
    return true;
}

void HotkeyManager::unregisterHotkey(ChannelId channel_id) {
    auto it = channel_hotkeys_.find(channel_id);
    if (it == channel_hotkeys_.end()) {
        return;  // Not registered
    }
    
    // Remove from key_to_channel map
    Qt::Key qtKey = static_cast<Qt::Key>(it->second[0].key());
    key_to_channel_.erase(qtKey);
    
    // Remove from channel_hotkeys map
    channel_hotkeys_.erase(it);
    
    std::cout << "⌨️ Unregistered hotkey for channel " << channel_id << "\n";
    
    emit hotkeysChanged();
}

QKeySequence HotkeyManager::getHotkey(ChannelId channel_id) const {
    auto it = channel_hotkeys_.find(channel_id);
    if (it != channel_hotkeys_.end()) {
        return it->second;
    }
    return QKeySequence();
}

ChannelId HotkeyManager::getChannelForKey(Qt::Key key) const {
    auto it = key_to_channel_.find(key);
    if (it != key_to_channel_.end()) {
        return it->second;
    }
    return 0;
}

bool HotkeyManager::isKeyPressed(Qt::Key key) const {
    return pressed_keys_.count(key) > 0;
}

std::set<Qt::Key> HotkeyManager::getPressedKeys() const {
    return pressed_keys_;
}

std::map<ChannelId, QKeySequence> HotkeyManager::getAllHotkeys() const {
    return channel_hotkeys_;
}

void HotkeyManager::clearAllHotkeys() {
    channel_hotkeys_.clear();
    key_to_channel_.clear();
    pressed_keys_.clear();
    
    std::cout << "⌨️ Cleared all hotkeys\n";
    
    emit hotkeysChanged();
}

bool HotkeyManager::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        
        // Ignore auto-repeat
        if (keyEvent->isAutoRepeat()) {
            return false;
        }
        
        Qt::Key key = static_cast<Qt::Key>(keyEvent->key());
        
        // Check if this key is mapped to a channel
        auto it = key_to_channel_.find(key);
        if (it != key_to_channel_.end()) {
            ChannelId channel_id = it->second;
            
            // Check if this is a new press (not already pressed)
            if (pressed_keys_.count(key) == 0) {
                pressed_keys_.insert(key);
                
                std::cout << "🎤 Hotkey pressed: Channel " << channel_id 
                          << " (key: " << keyEvent->key() << ")\n";
                
                emit hotkeyPressed(channel_id);
                
                // Consume the event so it doesn't propagate
                return true;
            }
        }
    } else if (event->type() == QEvent::KeyRelease) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        
        // Ignore auto-repeat
        if (keyEvent->isAutoRepeat()) {
            return false;
        }
        
        Qt::Key key = static_cast<Qt::Key>(keyEvent->key());
        
        // Check if this key is mapped to a channel
        auto it = key_to_channel_.find(key);
        if (it != key_to_channel_.end()) {
            ChannelId channel_id = it->second;
            
            // Remove from pressed keys
            pressed_keys_.erase(key);
            
            std::cout << "🔇 Hotkey released: Channel " << channel_id 
                      << " (key: " << keyEvent->key() << ")\n";
            
            emit hotkeyReleased(channel_id);
            
            // Consume the event
            return true;
        }
    }
    
    // Pass event to parent
    return QObject::eventFilter(obj, event);
}

} // namespace voip::ui
