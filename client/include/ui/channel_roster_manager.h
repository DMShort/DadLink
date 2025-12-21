#pragma once

#include <QObject>
#include <QString>
#include <vector>
#include <map>
#include <mutex>
#include "common/types.h"
#include "protocol/control_messages.h"

namespace voip::ui {

/**
 * ChannelRosterManager - Manages per-channel user lists
 *
 * This class tracks which users are in each channel and their states
 * (listening, speaking, etc.). It provides thread-safe access to roster
 * data and emits signals when rosters change.
 *
 * Features:
 * - Tracks users per channel
 * - Thread-safe roster updates
 * - Qt signals for real-time UI updates
 * - Supports multiple concurrent channels
 */
class ChannelRosterManager : public QObject {
    Q_OBJECT

public:
    /**
     * Channel user information
     */
    struct ChannelUser {
        UserId id;
        QString username;
        bool speaking = false;
        bool listening = false;
    };

    explicit ChannelRosterManager(QObject* parent = nullptr);
    ~ChannelRosterManager() override = default;

    /**
     * Update all channel rosters (from server broadcast)
     * @param rosters Vector of channel rosters from server
     */
    void updateAllRosters(const std::vector<protocol::ChannelRosterInfo>& rosters);

    /**
     * Add user to a channel
     * @param channelId Channel ID
     * @param user User to add
     */
    void addUserToChannel(ChannelId channelId, const ChannelUser& user);

    /**
     * Remove user from a channel
     * @param channelId Channel ID
     * @param userId User ID to remove
     */
    void removeUserFromChannel(ChannelId channelId, UserId userId);

    /**
     * Update user's speaking state
     * @param channelId Channel ID
     * @param userId User ID
     * @param speaking True if user is speaking
     */
    void updateUserSpeaking(ChannelId channelId, UserId userId, bool speaking);

    /**
     * Update user's listening state
     * @param channelId Channel ID
     * @param userId User ID
     * @param listening True if user is listening
     */
    void updateUserListening(ChannelId channelId, UserId userId, bool listening);

    /**
     * Get users in a channel
     * @param channelId Channel ID
     * @return Vector of users in the channel
     */
    std::vector<ChannelUser> getChannelUsers(ChannelId channelId) const;

    /**
     * Get number of users in a channel
     * @param channelId Channel ID
     * @return Number of users
     */
    int getChannelUserCount(ChannelId channelId) const;

    /**
     * Check if a user is in a channel
     * @param channelId Channel ID
     * @param userId User ID
     * @return True if user is in channel
     */
    bool isUserInChannel(ChannelId channelId, UserId userId) const;

    /**
     * Clear all rosters
     */
    void clearAll();

signals:
    /**
     * Emitted when a channel's roster changes
     * @param channelId Channel whose roster changed
     */
    void channelRosterChanged(ChannelId channelId);

    /**
     * Emitted when a channel's user count changes
     * @param channelId Channel ID
     * @param count New user count
     */
    void userCountChanged(ChannelId channelId, int count);

private:
    // Map: ChannelId -> Vector of users
    std::map<ChannelId, std::vector<ChannelUser>> rosters_;

    // Thread safety
    mutable std::mutex mutex_;

    // Helper: Find user in channel
    std::vector<ChannelUser>::iterator findUser(ChannelId channelId, UserId userId);
    std::vector<ChannelUser>::const_iterator findUser(ChannelId channelId, UserId userId) const;
};

} // namespace voip::ui
