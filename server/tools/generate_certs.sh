#!/bin/bash
# Generate self-signed TLS certificates for development
# For production, use Let's Encrypt or a proper CA

set -e

CERT_DIR="certs"
DOMAIN="localhost"
DAYS=365

echo "🔐 Generating TLS certificates for development..."
echo "   Domain: $DOMAIN"
echo "   Valid for: $DAYS days"
echo ""

# Create certs directory
mkdir -p "$CERT_DIR"

# Generate private key
echo "📝 Generating private key..."
openssl genrsa -out "$CERT_DIR/key.pem" 2048

# Generate certificate signing request
echo "📝 Generating certificate signing request..."
openssl req -new -key "$CERT_DIR/key.pem" -out "$CERT_DIR/csr.pem" \
    -subj "/C=US/ST=State/L=City/O=VoIP Server/OU=Development/CN=$DOMAIN"

# Generate self-signed certificate
echo "📝 Generating self-signed certificate..."
openssl x509 -req -days $DAYS \
    -in "$CERT_DIR/csr.pem" \
    -signkey "$CERT_DIR/key.pem" \
    -out "$CERT_DIR/cert.pem" \
    -extfile <(printf "subjectAltName=DNS:$DOMAIN,DNS:*.${DOMAIN},IP:127.0.0.1")

# Clean up CSR
rm "$CERT_DIR/csr.pem"

# Set permissions
chmod 600 "$CERT_DIR/key.pem"
chmod 644 "$CERT_DIR/cert.pem"

echo ""
echo "✅ Certificate generation complete!"
echo ""
echo "📁 Files created:"
echo "   Private key: $CERT_DIR/key.pem"
echo "   Certificate: $CERT_DIR/cert.pem"
echo ""
echo "⚠️  WARNING: These are self-signed certificates for DEVELOPMENT ONLY"
echo "   Clients will show security warnings when connecting"
echo "   For production, use Let's Encrypt or a proper certificate authority"
echo ""
echo "💡 To view certificate details:"
echo "   openssl x509 -in $CERT_DIR/cert.pem -text -noout"
echo ""
