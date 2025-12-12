/**
 * Voice Loopback Demo
 * 
 * Full end-to-end voice transmission test:
 * Microphone → Encode → Network → Decode → Speakers
 * 
 * Usage: voice_loopback_demo.exe [server_ip] [port]
 * Example: voice_loopback_demo.exe 127.0.0.1 9001
 */

#include "session/voice_session.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

using namespace voip;
using namespace voip::session;
using namespace std::chrono_literals;

// Global flag for clean shutdown
std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\n\n🛑 Stopping...\n";
        g_running = false;
    }
}

void print_stats(const VoiceSession::Stats& stats) {
    std::cout << "\r";
    std::cout << "📊 Stats: ";
    std::cout << "Cap:" << stats.frames_captured << " ";
    std::cout << "Enc:" << stats.frames_encoded << " ";
    std::cout << "Sent:" << stats.packets_sent << " ";
    std::cout << "Recv:" << stats.packets_received << " ";
    std::cout << "Dec:" << stats.frames_decoded << " ";
    std::cout << "Play:" << stats.frames_played << " ";
    std::cout << "Lat:" << static_cast<int>(stats.estimated_latency_ms) << "ms ";
    std::cout << "Jit:" << stats.jitter_ms << "ms ";
    
    if (stats.plc_frames > 0) {
        std::cout << "PLC:" << stats.plc_frames << " ";
    }
    if (stats.jitter_buffer_underruns > 0) {
        std::cout << "⚠️Underruns:" << stats.jitter_buffer_underruns << " ";
    }
    if (stats.encode_errors > 0 || stats.decode_errors > 0) {
        std::cout << "❌Errors:" << (stats.encode_errors + stats.decode_errors) << " ";
    }
    
    std::cout << std::flush;
}

int main(int argc, char* argv[]) {
    std::cout << "═══════════════════════════════════════════════\n";
    std::cout << "  🎤 VoIP Voice Loopback Demo 🔊\n";
    std::cout << "═══════════════════════════════════════════════\n\n";
    
    // Parse arguments
    std::string server = "127.0.0.1";
    uint16_t port = 9001;
    
    if (argc >= 2) {
        server = argv[1];
    }
    if (argc >= 3) {
        port = static_cast<uint16_t>(std::atoi(argv[2]));
    }
    
    std::cout << "Configuration:\n";
    std::cout << "  Server: " << server << ":" << port << "\n";
    std::cout << "  Sample Rate: 48000 Hz\n";
    std::cout << "  Frame Size: 960 samples (20ms)\n";
    std::cout << "  Bitrate: 32 kbps\n";
    std::cout << "  Codec: Opus (FEC enabled)\n\n";
    
    // Install signal handler for clean shutdown
    std::signal(SIGINT, signal_handler);
    
    // Create voice session
    VoiceSession session;
    
    // Configure session
    VoiceSession::Config config;
    config.server_address = server;
    config.server_port = port;
    config.sample_rate = 48000;
    config.channels = 1;
    config.frame_size = 960;  // 20ms
    config.bitrate = 32000;
    config.enable_fec = true;
    config.enable_dtx = false;
    config.channel_id = 1;
    config.user_id = 42;
    config.jitter_buffer_frames = 5;  // 100ms buffer
    
    // Initialize
    std::cout << "Initializing session...\n";
    auto init_result = session.initialize(config);
    if (!init_result.is_ok()) {
        std::cerr << "\n❌ Failed to initialize session:\n";
        std::cerr << "   " << init_result.error().to_string() << "\n\n";
        std::cerr << "💡 Tips:\n";
        std::cerr << "   - Make sure the server is running: cd server && cargo run\n";
        std::cerr << "   - Check your audio devices are available\n";
        std::cerr << "   - Verify the server address and port\n\n";
        return 1;
    }
    
    std::cout << "✅ Session initialized successfully!\n\n";
    
    // Start voice transmission
    std::cout << "Starting voice transmission...\n";
    auto start_result = session.start();
    if (!start_result.is_ok()) {
        std::cerr << "\n❌ Failed to start session:\n";
        std::cerr << "   " << start_result.error().to_string() << "\n\n";
        return 1;
    }
    
    std::cout << "\n╔═══════════════════════════════════════════════╗\n";
    std::cout << "║  🎤 VOICE SESSION ACTIVE 🔊                   ║\n";
    std::cout << "╚═══════════════════════════════════════════════╝\n\n";
    std::cout << "💬 Speak into your microphone!\n";
    std::cout << "   You should hear your voice after ~100-150ms\n";
    std::cout << "   (Processing through: Mic → Encode → Network → Decode → Speakers)\n\n";
    std::cout << "📊 Live statistics below (updates every second):\n";
    std::cout << "   Press Ctrl+C to stop\n\n";
    
    // Main loop - print statistics
    auto last_print = std::chrono::steady_clock::now();
    
    while (g_running && session.is_active()) {
        std::this_thread::sleep_for(100ms);
        
        // Print stats every second
        auto now = std::chrono::steady_clock::now();
        if (now - last_print >= 1s) {
            auto stats = session.get_stats();
            print_stats(stats);
            last_print = now;
        }
    }
    
    // Final statistics
    std::cout << "\n\n";
    std::cout << "═══════════════════════════════════════════════\n";
    std::cout << "  📊 Final Statistics\n";
    std::cout << "═══════════════════════════════════════════════\n\n";
    
    auto final_stats = session.get_stats();
    
    std::cout << "Audio:\n";
    std::cout << "  Frames captured:    " << final_stats.frames_captured << "\n";
    std::cout << "  Frames encoded:     " << final_stats.frames_encoded << "\n";
    std::cout << "  Frames decoded:     " << final_stats.frames_decoded << "\n";
    std::cout << "  Frames played:      " << final_stats.frames_played << "\n";
    std::cout << "  PLC frames:         " << final_stats.plc_frames << "\n";
    
    std::cout << "\nNetwork:\n";
    std::cout << "  Packets sent:       " << final_stats.packets_sent << "\n";
    std::cout << "  Packets received:   " << final_stats.packets_received << "\n";
    std::cout << "  Network errors:     " << final_stats.network_errors << "\n";
    
    std::cout << "\nQuality:\n";
    std::cout << "  Encode errors:      " << final_stats.encode_errors << "\n";
    std::cout << "  Decode errors:      " << final_stats.decode_errors << "\n";
    std::cout << "  Buffer underruns:   " << final_stats.jitter_buffer_underruns << "\n";
    std::cout << "  Jitter:             " << final_stats.jitter_ms << " ms\n";
    std::cout << "  Est. latency:       " << final_stats.estimated_latency_ms << " ms\n";
    
    // Calculate packet loss
    if (final_stats.packets_sent > 0) {
        float loss_rate = 0.0f;
        if (final_stats.packets_sent > final_stats.packets_received) {
            loss_rate = 100.0f * (1.0f - static_cast<float>(final_stats.packets_received) / 
                                          static_cast<float>(final_stats.packets_sent));
        }
        std::cout << "  Packet loss:        " << loss_rate << " %\n";
    }
    
    // Quality assessment
    std::cout << "\n";
    if (final_stats.estimated_latency_ms < 150.0f && 
        final_stats.jitter_buffer_underruns == 0 &&
        final_stats.encode_errors == 0 && 
        final_stats.decode_errors == 0) {
        std::cout << "✅ Excellent quality! System is performing well.\n";
    } else if (final_stats.estimated_latency_ms < 200.0f &&
               final_stats.jitter_buffer_underruns < 10) {
        std::cout << "✅ Good quality. Minor issues detected but acceptable.\n";
    } else {
        std::cout << "⚠️  Quality issues detected. Check network and system load.\n";
    }
    
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════\n";
    std::cout << "  👋 Session ended successfully!\n";
    std::cout << "═══════════════════════════════════════════════\n\n";
    
    // Shutdown
    session.shutdown();
    
    return 0;
}
