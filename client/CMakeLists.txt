cmake_minimum_required(VERSION 3.20)
project(voip-client VERSION 1.0.3 LANGUAGES CXX)

# Static link MSVC runtime for portable deployment (no VC++ redistributable needed)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    # Uses /MT for Release (static runtime) instead of /MD (dynamic runtime)
    # This embeds the C++ runtime into the executable (~800KB larger)
    # Result: Fully portable, no external VC++ installation required
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_OVERLAY "Build in-game overlay" OFF)

# Set Qt6 path explicitly (before vcpkg tries to find it)
set(Qt6_DIR "C:/Qt/6.10.1/msvc2022_64/lib/cmake/Qt6" CACHE PATH "Qt6 directory")

# Prevent vcpkg from auto-copying DLLs (we use windeployqt instead)
set(VCPKG_APPLOCAL_DEPS OFF CACHE BOOL "Disable vcpkg auto DLL copy" FORCE)

# Enable Qt MOC (Meta-Object Compiler) for signals/slots
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find dependencies
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network WebSockets)
find_package(Opus REQUIRED)
find_package(PortAudio REQUIRED)
find_package(OpenSSL REQUIRED)

# Qt Resources (icons, images, stylesheets)
set(VOIP_CLIENT_RESOURCES
    resources/resources.qrc
)

# Source files for GUI application
set(VOIP_CLIENT_SOURCES
    src/ui_main.cpp
    src/ui/login_dialog.cpp
    src/ui/main_window.cpp
    src/ui/settings_dialog.cpp
    src/ui/hotkey_manager.cpp
    src/ui/hotkey_input_dialog.cpp
    src/ui/channel_widget.cpp
    src/ui/channel_roster_manager.cpp
    src/audio/audio_engine.cpp
    src/audio/opus_codec.cpp
    src/audio/jitter_buffer.cpp
    src/audio/audio_mixer.cpp
    src/network/udp_socket.cpp
    src/network/websocket_client.cpp
    src/session/voice_session.cpp
    src/common/result.cpp
    # Crypto components
    src/crypto/key_exchange.cpp
    src/crypto/srtp_session.cpp
    # Admin panel components
    src/ui/admin/admin_panel.cpp
    src/ui/admin/dashboard_widget.cpp
    src/ui/admin/user_manager.cpp
    src/ui/admin/create_user_dialog.cpp
    src/ui/admin/edit_user_dialog.cpp
    src/ui/admin/channel_manager.cpp
    src/ui/admin/create_channel_dialog.cpp
    src/ui/admin/edit_channel_dialog.cpp
    src/ui/admin/role_manager.cpp
    src/ui/admin/create_role_dialog.cpp
    src/ui/admin/edit_role_dialog.cpp
    src/api/admin_api_client.cpp
    ${VOIP_CLIENT_RESOURCES}
)

set(VOIP_CLIENT_HEADERS
    include/ui/login_dialog.h
    include/ui/main_window.h
    include/ui/settings_dialog.h
    include/ui/hotkey_manager.h
    include/ui/hotkey_input_dialog.h
    include/ui/channel_widget.h
    include/ui/channel_roster_manager.h
    include/audio/audio_engine.h
    include/audio/opus_codec.h
    include/audio/jitter_buffer.h
    include/audio/audio_mixer.h
    include/network/udp_socket.h
    include/network/websocket_client.h
    include/protocol/control_messages.h
    include/session/voice_session.h
    include/common/types.h
    include/common/result.h
    include/common/lock_free_queue.h
    # Crypto headers
    include/crypto/key_exchange.h
    include/crypto/srtp_session.h
    # Admin panel headers
    include/ui/admin/admin_panel.h
    include/ui/admin/dashboard_widget.h
    include/ui/admin/user_manager.h
    include/ui/admin/create_user_dialog.h
    include/ui/admin/edit_user_dialog.h
    include/ui/admin/channel_manager.h
    include/ui/admin/create_channel_dialog.h
    include/ui/admin/edit_channel_dialog.h
    include/ui/admin/role_manager.h
    include/ui/admin/create_role_dialog.h
    include/ui/admin/edit_role_dialog.h
    include/api/admin_api_client.h
)

# Create executable (with console for debugging)
if(WIN32)
    add_executable(voip-client ${VOIP_CLIENT_SOURCES} ${VOIP_CLIENT_HEADERS})
    # Enable console window for debugging
    set_target_properties(voip-client PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
    
    # Manual deployment - POST_BUILD disabled to prevent DLL version conflicts
    # Use deploy.bat script instead for reliable deployment
    message(STATUS "========================================")
    message(STATUS "NOTE: Run deploy.bat after building to deploy Qt dependencies")
    message(STATUS "This ensures correct DLL versions are copied")
    message(STATUS "========================================")
else()
    add_executable(voip-client ${VOIP_CLIENT_SOURCES} ${VOIP_CLIENT_HEADERS})
endif()

target_include_directories(voip-client
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OPUS_INCLUDE_DIRS}
        ${PORTAUDIO_INCLUDE_DIRS}
)

target_link_libraries(voip-client
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::WebSockets
        Opus::opus
        portaudio
        OpenSSL::SSL
        OpenSSL::Crypto
)

# Compiler warnings
if(MSVC)
    target_compile_options(voip-client PRIVATE /W4 /FS)
else()
    target_compile_options(voip-client PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Voice loopback demo (optional example)
add_executable(voice_loopback_demo 
    examples/voice_loopback_demo.cpp
    src/audio/audio_engine.cpp
    src/audio/opus_codec.cpp
    src/audio/jitter_buffer.cpp
    src/network/udp_socket.cpp
    src/session/voice_session.cpp
    src/common/result.cpp
)

target_include_directories(voice_loopback_demo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OPUS_INCLUDE_DIRS}
        ${PORTAUDIO_INCLUDE_DIRS}
)

target_link_libraries(voice_loopback_demo
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Opus::opus
        portaudio
        OpenSSL::SSL
        OpenSSL::Crypto
)

# Unit tests
if(BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    
    add_executable(voip-client-tests
        tests/audio/test_opus_codec.cpp
        tests/audio/test_jitter_buffer.cpp
        tests/integration/test_audio_loopback.cpp
        # Add corresponding source files (without main.cpp)
        src/audio/audio_engine.cpp
        src/audio/opus_codec.cpp
        src/audio/jitter_buffer.cpp
        src/common/result.cpp
    )
    
    target_include_directories(voip-client-tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${OPUS_INCLUDE_DIRS}
            ${PORTAUDIO_INCLUDE_DIRS}
    )
    
    target_link_libraries(voip-client-tests
        PRIVATE
            GTest::gtest_main
            Qt6::Core
            ${OPUS_LIBRARIES}
            ${PORTAUDIO_LIBRARIES}
    )
    
    include(GoogleTest)
    gtest_discover_tests(voip-client-tests)
endif()

# Installation
install(TARGETS voip-client
    RUNTIME DESTINATION bin
)

install(FILES config.json.example
    DESTINATION share/voip-client
    RENAME config.json
)
