#include "ui/hotkey_manager.h"
#include <QApplication>
#include <QWidget>
#include <iostream>

#ifdef _WIN32
#include <windows.h>

// Static pointer to the HotkeyManager instance (for hook callback)
static voip::ui::HotkeyManager* g_hotkeyManager = nullptr;

// Low-level keyboard hook callback
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && g_hotkeyManager) {
        KBDLLHOOKSTRUCT* kb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            // Key pressed
            if (g_hotkeyManager->handleKeyPress(kb->vkCode)) {
                // Consume the event if it's a registered hotkey
                return 1;
            }
        } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            // Key released
            if (g_hotkeyManager->handleKeyRelease(kb->vkCode)) {
                // Consume the event if it's a registered hotkey
                return 1;
            }
        }
    }

    // Pass to next hook in chain
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
#endif

namespace voip::ui {

HotkeyManager::HotkeyManager(QWidget* parent)
    : QObject(qobject_cast<QObject*>(parent))
    , parent_widget_(parent)
    , next_hotkey_id_(1) {

#ifdef _WIN32
    // Set global pointer
    g_hotkeyManager = this;

    // Install low-level keyboard hook
    HHOOK hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(nullptr), 0);
    keyboard_hook_ = static_cast<void*>(hook);

    if (keyboard_hook_) {
        std::cout << "✅ HotkeyManager initialized with Windows global keyboard hook\n";
    } else {
        DWORD error = GetLastError();
        std::cerr << "❌ Failed to install keyboard hook, error: " << error << "\n";
    }
#else
    std::cerr << "❌ Global hotkeys only supported on Windows\n";
#endif
}

HotkeyManager::~HotkeyManager() {
#ifdef _WIN32
    // Unhook keyboard
    if (keyboard_hook_) {
        UnhookWindowsHookEx(static_cast<HHOOK>(keyboard_hook_));
        std::cout << "🔻 Keyboard hook removed\n";
    }

    // Clear global pointer
    g_hotkeyManager = nullptr;
#endif
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

        // Remove from VK code map
        int oldVk = qtKeyToVirtualKey(old_key);
        if (oldVk != 0) {
            vk_to_channel_.erase(oldVk);
        }
    }

    // Convert to VK code
    int vkCode = qtKeyToVirtualKey(qtKey);
    if (vkCode == 0) {
        std::cerr << "❌ Cannot map Qt key to Windows VK code\n";
        return false;
    }

    // Store mappings
    channel_hotkeys_[channel_id] = key;
    key_to_channel_[qtKey] = channel_id;
    vk_to_channel_[vkCode] = channel_id;

    std::cout << "⌨️ Registered GLOBAL hotkey " << key.toString().toStdString()
              << " for channel " << channel_id << " (VK: " << vkCode << ")\n";

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

    // Remove from VK code map
    int vkCode = qtKeyToVirtualKey(qtKey);
    if (vkCode != 0) {
        vk_to_channel_.erase(vkCode);
    }

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
    vk_to_channel_.clear();
    pressed_keys_.clear();

    std::cout << "⌨️ Cleared all hotkeys\n";

    emit hotkeysChanged();
}

#ifdef _WIN32
bool HotkeyManager::handleKeyPress(DWORD vkCode) {
    // Check if this VK code is mapped to a channel
    auto it = vk_to_channel_.find(static_cast<int>(vkCode));
    if (it != vk_to_channel_.end()) {
        ChannelId channel_id = it->second;

        // Check if this is a new press (not already pressed)
        if (pressed_keys_.count(static_cast<Qt::Key>(vkCode)) == 0) {
            pressed_keys_.insert(static_cast<Qt::Key>(vkCode));

            std::cout << "🎤 Global hotkey pressed: Channel " << channel_id
                      << " (VK: " << vkCode << ")\n";

            emit hotkeyPressed(channel_id);
        }

        return true;  // Event handled (hotkey consumed)
    }

    return false;  // Not a registered hotkey
}

bool HotkeyManager::handleKeyRelease(DWORD vkCode) {
    // Check if this VK code is mapped to a channel
    auto it = vk_to_channel_.find(static_cast<int>(vkCode));
    if (it != vk_to_channel_.end()) {
        ChannelId channel_id = it->second;

        // Remove from pressed keys
        pressed_keys_.erase(static_cast<Qt::Key>(vkCode));

        std::cout << "🔇 Global hotkey released: Channel " << channel_id
                  << " (VK: " << vkCode << ")\n";

        emit hotkeyReleased(channel_id);

        return true;  // Event handled (hotkey consumed)
    }

    return false;  // Not a registered hotkey
}

int HotkeyManager::qtKeyToVirtualKey(Qt::Key key) {
    // Map Qt keys to Windows virtual key codes
    // F1-F12 keys
    if (key >= Qt::Key_F1 && key <= Qt::Key_F12) {
        return VK_F1 + (key - Qt::Key_F1);
    }

    // Number keys (0-9)
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        return '0' + (key - Qt::Key_0);
    }

    // Letter keys (A-Z)
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        return 'A' + (key - Qt::Key_A);
    }

    // Add more mappings as needed
    switch (key) {
        case Qt::Key_Space: return VK_SPACE;
        case Qt::Key_Return: return VK_RETURN;
        case Qt::Key_Escape: return VK_ESCAPE;
        case Qt::Key_Tab: return VK_TAB;
        case Qt::Key_Backspace: return VK_BACK;
        case Qt::Key_Insert: return VK_INSERT;
        case Qt::Key_Delete: return VK_DELETE;
        case Qt::Key_Home: return VK_HOME;
        case Qt::Key_End: return VK_END;
        case Qt::Key_PageUp: return VK_PRIOR;
        case Qt::Key_PageDown: return VK_NEXT;
        case Qt::Key_Left: return VK_LEFT;
        case Qt::Key_Right: return VK_RIGHT;
        case Qt::Key_Up: return VK_UP;
        case Qt::Key_Down: return VK_DOWN;
        // Numpad keys
        case Qt::Key_0 + Qt::KeypadModifier: return VK_NUMPAD0;
        case Qt::Key_1 + Qt::KeypadModifier: return VK_NUMPAD1;
        case Qt::Key_2 + Qt::KeypadModifier: return VK_NUMPAD2;
        case Qt::Key_3 + Qt::KeypadModifier: return VK_NUMPAD3;
        case Qt::Key_4 + Qt::KeypadModifier: return VK_NUMPAD4;
        case Qt::Key_5 + Qt::KeypadModifier: return VK_NUMPAD5;
        case Qt::Key_6 + Qt::KeypadModifier: return VK_NUMPAD6;
        case Qt::Key_7 + Qt::KeypadModifier: return VK_NUMPAD7;
        case Qt::Key_8 + Qt::KeypadModifier: return VK_NUMPAD8;
        case Qt::Key_9 + Qt::KeypadModifier: return VK_NUMPAD9;
        default:
            // std::cerr << "⚠️ Unmapped Qt key: " << key << "\n";
            return 0;
    }
}

#else
// Stub implementations for non-Windows platforms
bool HotkeyManager::handleKeyPress(DWORD vkCode) {
    Q_UNUSED(vkCode);
    return false;
}

bool HotkeyManager::handleKeyRelease(DWORD vkCode) {
    Q_UNUSED(vkCode);
    return false;
}

int HotkeyManager::qtKeyToVirtualKey(Qt::Key key) {
    Q_UNUSED(key);
    return 0;
}
#endif

} // namespace voip::ui
