# Check DLL dependencies for voip-client.exe
$exePath = ".\build\Debug\voip-client.exe"

Write-Host "=== Checking voip-client.exe Dependencies ===" -ForegroundColor Cyan
Write-Host ""

# Check if exe exists
if (-not (Test-Path $exePath)) {
    Write-Host "ERROR: $exePath not found!" -ForegroundColor Red
    exit 1
}

Write-Host "✓ Executable found: $exePath" -ForegroundColor Green
Write-Host ""

# List all DLLs in Debug folder
Write-Host "=== DLLs in Debug folder: ===" -ForegroundColor Yellow
Get-ChildItem ".\build\Debug\*.dll" | Select-Object Name, @{Name="Size (KB)";Expression={[math]::Round($_.Length/1KB,2)}} | Format-Table -AutoSize

Write-Host ""
Write-Host "=== Checking Qt DLL versions: ===" -ForegroundColor Yellow

$qtDlls = Get-ChildItem ".\build\Debug\Qt*.dll"
foreach ($dll in $qtDlls) {
    try {
        $version = (Get-Item $dll.FullName).VersionInfo.FileVersion
        $productVersion = (Get-Item $dll.FullName).VersionInfo.ProductVersion
        Write-Host "$($dll.Name): $version (Product: $productVersion)"
    } catch {
        Write-Host "$($dll.Name): [No version info]" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "=== Checking VC++ Runtime: ===" -ForegroundColor Yellow

# Check if debug runtime DLLs are accessible
$vcRuntimeDlls = @("vcruntime140d.dll", "msvcp140d.dll", "ucrtbased.dll")
foreach ($dll in $vcRuntimeDlls) {
    $found = Get-Command $dll -ErrorAction SilentlyContinue
    if ($found) {
        Write-Host "✓ $dll found in PATH" -ForegroundColor Green
    } else {
        Write-Host "✗ $dll NOT found in PATH" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "=== Checking specific DLLs: ===" -ForegroundColor Yellow

$requiredDlls = @(
    "Qt6Cored.dll",
    "Qt6Guid.dll", 
    "Qt6Widgetsd.dll",
    "Qt6Networkd.dll",
    "Qt6WebSocketsd.dll",
    "opus.dll",
    "portaudio.dll",
    "libssl-3-x64.dll",
    "libcrypto-3-x64.dll"
)

foreach ($dll in $requiredDlls) {
    $path = ".\build\Debug\$dll"
    if (Test-Path $path) {
        $size = [math]::Round((Get-Item $path).Length / 1MB, 2)
        Write-Host "✓ $dll ($size MB)" -ForegroundColor Green
    } else {
        Write-Host "✗ $dll MISSING!" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "=== Checking Qt Platform Plugin: ===" -ForegroundColor Yellow

$platformPlugin = ".\build\Debug\platforms\qwindowsd.dll"
if (Test-Path $platformPlugin) {
    Write-Host "✓ qwindowsd.dll found in platforms folder" -ForegroundColor Green
} else {
    Write-Host "✗ qwindowsd.dll MISSING from platforms folder!" -ForegroundColor Red
}

Write-Host ""
Write-Host "=== Summary ===" -ForegroundColor Cyan
Write-Host "If all DLLs are present and correct versions, the crash is likely:"
Write-Host "  1. DLL initialization failure (use Visual Studio debugger)"
Write-Host "  2. Qt version mismatch (check all Qt DLLs are 6.10.1)"
Write-Host "  3. Debug/Release runtime mismatch"
Write-Host ""
Write-Host "Next step: Run with Visual Studio debugger (F5) and enable ALL exception breaking!"
