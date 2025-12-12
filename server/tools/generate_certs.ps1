# Generate self-signed TLS certificates for development (Windows PowerShell)
# For production, use Let's Encrypt or a proper CA

param(
    [string]$CertDir = "certs",
    [string]$Domain = "localhost",
    [int]$Days = 365
)

Write-Host "🔐 Generating TLS certificates for development..." -ForegroundColor Cyan
Write-Host "   Domain: $Domain" -ForegroundColor Gray
Write-Host "   Valid for: $Days days" -ForegroundColor Gray
Write-Host ""

# Create certs directory
if (-not (Test-Path $CertDir)) {
    New-Item -ItemType Directory -Path $CertDir | Out-Null
}

$certPath = Join-Path $CertDir "cert.pem"
$keyPath = Join-Path $CertDir "key.pem"

# Check if OpenSSL is available
$opensslPath = Get-Command openssl -ErrorAction SilentlyContinue

if ($opensslPath) {
    Write-Host "📝 Using OpenSSL to generate certificates..." -ForegroundColor Green

    # Generate private key
    Write-Host "   Generating private key..." -ForegroundColor Gray
    & openssl genrsa -out $keyPath 2048 2>&1 | Out-Null

    # Generate certificate
    Write-Host "   Generating self-signed certificate..." -ForegroundColor Gray
    & openssl req -new -x509 -key $keyPath -out $certPath -days $Days `
        -subj "/C=US/ST=State/L=City/O=VoIP Server/OU=Development/CN=$Domain" `
        -addext "subjectAltName=DNS:$Domain,DNS:*.$Domain,IP:127.0.0.1" 2>&1 | Out-Null

    Write-Host ""
    Write-Host "✅ Certificate generation complete!" -ForegroundColor Green
}
else {
    Write-Host "⚠️  OpenSSL not found. Using .NET certificate generation..." -ForegroundColor Yellow
    Write-Host ""

    # Use .NET to generate self-signed certificate
    $cert = New-SelfSignedCertificate `
        -DnsName $Domain, "127.0.0.1" `
        -CertStoreLocation "Cert:\CurrentUser\My" `
        -NotAfter (Get-Date).AddDays($Days) `
        -KeyAlgorithm RSA `
        -KeyLength 2048 `
        -KeyExportPolicy Exportable `
        -KeyUsage DigitalSignature, KeyEncipherment `
        -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.1")

    # Export certificate
    Write-Host "   Exporting certificate to PEM format..." -ForegroundColor Gray
    $certBytes = $cert.Export([System.Security.Cryptography.X509Certificates.X509ContentType]::Cert)
    $certPem = "-----BEGIN CERTIFICATE-----`n"
    $certPem += [System.Convert]::ToBase64String($certBytes, [System.Base64FormattingOptions]::InsertLineBreaks)
    $certPem += "`n-----END CERTIFICATE-----`n"
    Set-Content -Path $certPath -Value $certPem -Encoding ASCII

    # Export private key (requires password for security)
    Write-Host "   Exporting private key..." -ForegroundColor Gray
    $password = ConvertTo-SecureString -String "temp" -Force -AsPlainText
    $pfxBytes = $cert.Export([System.Security.Cryptography.X509Certificates.X509ContentType]::Pfx, $password)

    # Convert PFX to PEM using OpenSSL (if available) or save as PFX
    $pfxPath = Join-Path $CertDir "cert.pfx"
    [System.IO.File]::WriteAllBytes($pfxPath, $pfxBytes)

    Write-Host ""
    Write-Host "✅ Certificate generation complete!" -ForegroundColor Green
    Write-Host ""
    Write-Host "⚠️  Note: Private key is in PFX format: $pfxPath" -ForegroundColor Yellow
    Write-Host "   To convert to PEM, install OpenSSL and run:" -ForegroundColor Yellow
    Write-Host "   openssl pkcs12 -in cert.pfx -nocerts -out key.pem -nodes -password pass:temp" -ForegroundColor Gray

    # Clean up from cert store
    Remove-Item -Path "Cert:\CurrentUser\My\$($cert.Thumbprint)" -Force
}

Write-Host ""
Write-Host "📁 Files created:" -ForegroundColor Cyan
if (Test-Path $keyPath) {
    Write-Host "   Private key: $keyPath" -ForegroundColor Gray
}
Write-Host "   Certificate: $certPath" -ForegroundColor Gray
Write-Host ""
Write-Host "⚠️  WARNING: These are self-signed certificates for DEVELOPMENT ONLY" -ForegroundColor Red
Write-Host "   Clients will show security warnings when connecting" -ForegroundColor Yellow
Write-Host "   For production, use Let's Encrypt or a proper certificate authority" -ForegroundColor Yellow
Write-Host ""
Write-Host "💡 Installation instructions:" -ForegroundColor Cyan
Write-Host "   1. OpenSSL: https://slproweb.com/products/Win32OpenSSL.html" -ForegroundColor Gray
Write-Host "   2. Or use Chocolatey: choco install openssl" -ForegroundColor Gray
Write-Host ""
