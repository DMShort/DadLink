#include "common/result.h"

namespace voip {

std::string Error::to_string() const {
    std::string result = "Error(";
    
    switch (code_) {
        case ErrorCode::Success:
            result += "Success";
            break;
        case ErrorCode::AudioDeviceNotFound:
            result += "AudioDeviceNotFound";
            break;
        case ErrorCode::AudioInitFailed:
            result += "AudioInitFailed";
            break;
        case ErrorCode::AudioStreamFailed:
            result += "AudioStreamFailed";
            break;
        case ErrorCode::OpusEncodeFailed:
            result += "OpusEncodeFailed";
            break;
        case ErrorCode::OpusDecodeFailed:
            result += "OpusDecodeFailed";
            break;
        case ErrorCode::NetworkConnectionFailed:
            result += "NetworkConnectionFailed";
            break;
        case ErrorCode::NetworkSendFailed:
            result += "NetworkSendFailed";
            break;
        case ErrorCode::NetworkReceiveFailed:
            result += "NetworkReceiveFailed";
            break;
        case ErrorCode::InvalidPacket:
            result += "InvalidPacket";
            break;
        case ErrorCode::BufferFull:
            result += "BufferFull";
            break;
        case ErrorCode::BufferEmpty:
            result += "BufferEmpty";
            break;
        case ErrorCode::Timeout:
            result += "Timeout";
            break;
        case ErrorCode::AuthenticationFailed:
            result += "AuthenticationFailed";
            break;
        case ErrorCode::InvalidState:
            result += "InvalidState";
            break;
        case ErrorCode::Unknown:
            result += "Unknown";
            break;
    }
    
    if (!message_.empty()) {
        result += ": " + message_;
    }
    
    result += ")";
    return result;
}

} // namespace voip
