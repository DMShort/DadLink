#include <iostream>
#include "audio/audio_engine.h"
#include "audio/opus_codec.h"
#include "common/types.h"

using namespace voip;
using namespace voip::audio;

int main() {
    std::cout << "VoIP Client v0.1.0\n";
    std::cout << "==================\n\n";
    
    // Initialize audio engine
    AudioEngine engine;
    AudioConfig config;
    config.sample_rate = 48000;
    config.frame_size = 960;
    
    auto init_result = engine.initialize(config);
    if (!init_result.is_ok()) {
        std::cerr << "Failed to initialize audio engine: " 
                  << init_result.error().to_string() << "\n";
        return 1;
    }
    
    std::cout << "Audio engine initialized\n\n";
    
    // Enumerate devices
    std::cout << "Available input devices:\n";
    auto input_devices = engine.enumerate_input_devices();
    for (const auto& device : input_devices) {
        std::cout << "  [" << device.id << "] " << device.name;
        if (device.is_default) {
            std::cout << " (default)";
        }
        std::cout << "\n";
    }
    
    std::cout << "\nAvailable output devices:\n";
    auto output_devices = engine.enumerate_output_devices();
    for (const auto& device : output_devices) {
        std::cout << "  [" << device.id << "] " << device.name;
        if (device.is_default) {
            std::cout << " (default)";
        }
        std::cout << "\n";
    }
    
    // Create Opus encoder
    OpusConfig opus_config;
    opus_config.sample_rate = 48000;
    opus_config.bitrate = 32000;
    
    auto encoder_result = voip::audio::OpusEncoder::create(opus_config);
    if (!encoder_result.is_ok()) {
        std::cerr << "\nFailed to create Opus encoder: " 
                  << encoder_result.error().to_string() << "\n";
        return 1;
    }
    
    std::cout << "\nOpus encoder created (32 kbps)\n";
    
    // Create Opus decoder
    auto decoder_result = voip::audio::OpusDecoder::create(48000, 1);
    if (!decoder_result.is_ok()) {
        std::cerr << "Failed to create Opus decoder: " 
                  << decoder_result.error().to_string() << "\n";
        return 1;
    }
    
    std::cout << "Opus decoder created\n";
    
    std::cout << "\n================================\n";
    std::cout << "System ready for development!\n";
    std::cout << "================================\n\n";
    
    std::cout << "Next steps:\n";
    std::cout << "- Run tests: ctest --output-on-failure\n";
    std::cout << "- Start network implementation (Milestone 1.2)\n";
    std::cout << "- Build Qt UI (Milestone 1.4)\n";
    
    return 0;
}
