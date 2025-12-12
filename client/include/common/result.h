#pragma once

#include <variant>
#include <string>
#include <stdexcept>
#include <optional>

namespace voip {

// Error types
enum class ErrorCode {
    Success = 0,
    AudioDeviceNotFound,
    AudioInitFailed,
    AudioStreamFailed,
    OpusEncodeFailed,
    OpusDecodeFailed,
    NetworkConnectionFailed,
    NetworkSendFailed,
    NetworkReceiveFailed,
    InvalidPacket,
    BufferFull,
    BufferEmpty,
    Timeout,
    AuthenticationFailed,
    InvalidState,
    Unknown,
};

// Error class
class Error {
public:
    explicit Error(ErrorCode code, std::string message = "")
        : code_(code), message_(std::move(message)) {}
    
    [[nodiscard]] ErrorCode code() const noexcept { return code_; }
    [[nodiscard]] const std::string& message() const noexcept { return message_; }
    [[nodiscard]] std::string to_string() const;
    
private:
    ErrorCode code_;
    std::string message_;
};

// Result type (like Rust's Result<T, E>)
template<typename T>
class [[nodiscard]] Result {
public:
    // Constructors
    Result(T value) : data_(std::move(value)) {}
    Result(Error error) : data_(std::move(error)) {}
    
    // Check if result is ok
    [[nodiscard]] bool is_ok() const noexcept {
        return std::holds_alternative<T>(data_);
    }
    
    [[nodiscard]] bool is_err() const noexcept {
        return std::holds_alternative<Error>(data_);
    }
    
    // Get value (throws if error)
    [[nodiscard]] T& value() {
        if (is_err()) {
            throw std::runtime_error("Called value() on error Result: " + error().to_string());
        }
        return std::get<T>(data_);
    }
    
    [[nodiscard]] const T& value() const {
        if (is_err()) {
            throw std::runtime_error("Called value() on error Result: " + error().to_string());
        }
        return std::get<T>(data_);
    }
    
    // Get error (throws if ok)
    [[nodiscard]] Error& error() {
        if (is_ok()) {
            throw std::runtime_error("Called error() on ok Result");
        }
        return std::get<Error>(data_);
    }
    
    [[nodiscard]] const Error& error() const {
        if (is_ok()) {
            throw std::runtime_error("Called error() on ok Result");
        }
        return std::get<Error>(data_);
    }
    
    // Unwrap (throws if error)
    [[nodiscard]] T unwrap() {
        return std::move(value());
    }
    
    // Unwrap or default
    [[nodiscard]] T value_or(T default_value) {
        return is_ok() ? std::move(value()) : std::move(default_value);
    }
    
private:
    std::variant<T, Error> data_;
};

// Specialization for void
template<>
class [[nodiscard]] Result<void> {
public:
    Result() : error_(std::nullopt) {}
    Result(Error error) : error_(std::move(error)) {}
    
    [[nodiscard]] bool is_ok() const noexcept { return !error_.has_value(); }
    [[nodiscard]] bool is_err() const noexcept { return error_.has_value(); }
    
    [[nodiscard]] Error& error() {
        if (!error_) {
            throw std::runtime_error("Called error() on ok Result");
        }
        return *error_;
    }
    
    [[nodiscard]] const Error& error() const {
        if (!error_) {
            throw std::runtime_error("Called error() on ok Result");
        }
        return *error_;
    }
    
    void unwrap() {
        if (error_) {
            throw std::runtime_error("Called unwrap() on error Result: " + error_->to_string());
        }
    }
    
private:
    std::optional<Error> error_;
};

// Helper functions for creating Results
template<typename T>
inline Result<T> Ok(T value) {
    return Result<T>(std::move(value));
}

inline Result<void> Ok() {
    return Result<void>();
}

template<typename T>
inline Result<T> Err(ErrorCode code, std::string message = "") {
    return Result<T>(Error(code, std::move(message)));
}

} // namespace voip
