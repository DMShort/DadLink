# Multi-stage Dockerfile for VoIP Server
# Stage 1: Build the Rust application
# Stage 2: Create minimal runtime image

#############################################
# Stage 1: Builder
#############################################
FROM rust:slim AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    pkg-config \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Create app directory
WORKDIR /app

# Copy all source code
COPY . .

# Build the application with SQLx offline mode
# This skips compile-time SQL checking - queries are checked at runtime instead
ENV SQLX_OFFLINE=true
RUN cargo build --release --bin voip-server

#############################################
# Stage 2: Runtime
#############################################
FROM debian:trixie-slim

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    ca-certificates \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

# Create app user (non-root for security)
RUN useradd -m -u 1000 voipuser && \
    mkdir -p /app /app/certs /app/migrations && \
    chown -R voipuser:voipuser /app

# Set working directory
WORKDIR /app

# Copy binary from builder
COPY --from=builder --chown=voipuser:voipuser /app/target/release/voip-server /app/voip-server

# Copy migrations from builder
COPY --from=builder --chown=voipuser:voipuser /app/migrations /app/migrations

# Switch to non-root user
USER voipuser

# Expose ports
# 9000 - WebSocket control port (TCP)
# 9001 - Voice UDP port
EXPOSE 9000 9001/udp

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD [ "sh", "-c", "test -f /app/voip-server" ]

# Set default environment variables (can be overridden)
ENV VOIP__SERVER__BIND_ADDRESS=0.0.0.0 \
    VOIP__SERVER__CONTROL_PORT=9000 \
    VOIP__SERVER__VOICE_PORT=9001 \
    VOIP__DATABASE__MAX_CONNECTIONS=20 \
    VOIP__SECURITY__TLS_CERT=/app/certs/cert.pem \
    VOIP__SECURITY__TLS_KEY=/app/certs/key.pem \
    RUST_LOG=voip_server=info

# Run the server
CMD ["/app/voip-server"]
