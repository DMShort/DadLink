#include "ui/channel_roster_manager.h"
#include <algorithm>
#include <iostream>

namespace voip::ui {

ChannelRosterManager::ChannelRosterManager(QObject* parent)
    : QObject(parent) {
}

void ChannelRosterManager::updateAllRosters(const std::vector<protocol::ChannelRosterInfo>& rosters) {
    // Store channels that need signal emission
    std::vector<std::pair<ChannelId, int>> channelsToSignal;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Roster snapshots only ADD users, never REMOVE
        // This prevents race conditions where server snapshot is outdated
        // Join/leave notifications are the source of truth for removal

        for (const auto& roster : rosters) {
            auto& channel_users = rosters_[roster.channel_id];

            for (const auto& user_info : roster.users) {
                // Check if user already exists
                auto it = std::find_if(channel_users.begin(), channel_users.end(),
                                      [&](const ChannelUser& u) { return u.id == user_info.id; });

                if (it == channel_users.end()) {
                    // New user - add to channel
                    ChannelUser user;
                    user.id = user_info.id;
                    user.username = QString::fromStdString(user_info.username);
                    user.speaking = user_info.speaking;
                    user.listening = true;
                    channel_users.push_back(user);
                    std::cout << "📊 Roster snapshot added user " << user.username.toStdString()
                              << " to channel " << roster.channel_id << std::endl;
                } else {
                    // User exists - update their info
                    it->speaking = user_info.speaking;
                }
            }

            // Store channel info for signal emission after lock is released
            channelsToSignal.push_back({roster.channel_id, static_cast<int>(channel_users.size())});
        }

        std::cout << "📊 Updated rosters for " << rosters.size() << " channels" << std::endl;
    }

    // Emit signals OUTSIDE the lock to avoid deadlock
    for (const auto& [channelId, userCount] : channelsToSignal) {
        emit channelRosterChanged(channelId);
        emit userCountChanged(channelId, userCount);
    }
}

void ChannelRosterManager::addUserToChannel(ChannelId channelId, const ChannelUser& user) {
    int userCount = 0;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if user already exists
        auto& channel_users = rosters_[channelId];
        auto it = std::find_if(channel_users.begin(), channel_users.end(),
                              [&](const ChannelUser& u) { return u.id == user.id; });

        if (it != channel_users.end()) {
            // User already in channel - update their info
            std::cout << "🔄 Updating existing user " << user.username.toStdString()
                      << " (ID: " << user.id << ") in channel " << channelId << std::endl;
            *it = user;
        } else {
            // New user - add to channel
            channel_users.push_back(user);
            std::cout << "👤 Added NEW user " << user.username.toStdString()
                      << " (ID: " << user.id << ") to channel " << channelId
                      << " (now has " << channel_users.size() << " users)" << std::endl;
        }

        userCount = static_cast<int>(channel_users.size());
    }

    // Emit signals outside the lock
    emit channelRosterChanged(channelId);
    emit userCountChanged(channelId, userCount);
}

void ChannelRosterManager::removeUserFromChannel(ChannelId channelId, UserId userId) {
    bool userRemoved = false;
    int userCount = 0;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = rosters_.find(channelId);
        if (it == rosters_.end()) {
            return;  // Channel not found
        }

        auto& users = it->second;
        auto user_it = std::find_if(users.begin(), users.end(),
                                    [userId](const ChannelUser& u) { return u.id == userId; });

        if (user_it != users.end()) {
            std::cout << "👋 Removed user " << user_it->username.toStdString() << " from channel " << channelId << std::endl;
            users.erase(user_it);
            userRemoved = true;
            userCount = static_cast<int>(users.size());
        }
    }

    // Emit signals outside the lock
    if (userRemoved) {
        emit channelRosterChanged(channelId);
        emit userCountChanged(channelId, userCount);
    }
}

void ChannelRosterManager::updateUserSpeaking(ChannelId channelId, UserId userId, bool speaking) {
    bool userUpdated = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = rosters_.find(channelId);
        if (it == rosters_.end()) {
            return;  // Channel not found
        }

        auto& users = it->second;
        auto user_it = std::find_if(users.begin(), users.end(),
                                    [userId](const ChannelUser& u) { return u.id == userId; });

        if (user_it != users.end()) {
            user_it->speaking = speaking;
            userUpdated = true;
        }
    }

    // Emit signal outside the lock
    if (userUpdated) {
        emit channelRosterChanged(channelId);
    }
}

void ChannelRosterManager::updateUserListening(ChannelId channelId, UserId userId, bool listening) {
    bool userUpdated = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = rosters_.find(channelId);
        if (it == rosters_.end()) {
            return;  // Channel not found
        }

        auto& users = it->second;
        auto user_it = std::find_if(users.begin(), users.end(),
                                    [userId](const ChannelUser& u) { return u.id == userId; });

        if (user_it != users.end()) {
            user_it->listening = listening;
            userUpdated = true;
        }
    }

    // Emit signal outside the lock
    if (userUpdated) {
        emit channelRosterChanged(channelId);
    }
}

std::vector<ChannelRosterManager::ChannelUser> ChannelRosterManager::getChannelUsers(ChannelId channelId) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rosters_.find(channelId);
    if (it != rosters_.end()) {
        return it->second;  // Return copy
    }

    return {};  // Empty vector if channel not found
}

int ChannelRosterManager::getChannelUserCount(ChannelId channelId) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rosters_.find(channelId);
    if (it != rosters_.end()) {
        return static_cast<int>(it->second.size());
    }

    return 0;
}

bool ChannelRosterManager::isUserInChannel(ChannelId channelId, UserId userId) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rosters_.find(channelId);
    if (it == rosters_.end()) {
        return false;
    }

    const auto& users = it->second;
    return std::find_if(users.begin(), users.end(),
                       [userId](const ChannelUser& u) { return u.id == userId; }) != users.end();
}

void ChannelRosterManager::clearAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    rosters_.clear();
    std::cout << "🗑️ Cleared all channel rosters" << std::endl;
}

} // namespace voip::ui
