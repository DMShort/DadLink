# DLL Version Verification Script
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "VoIP Client - DLL Version Checker" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$buildDir = Join-Path $PSScriptRoot "build\Debug"

# Expected DLL sizes for Qt 6.10.1 Debug
$expectedSizes = @{
    "Qt6Cored.dll" = 21895392
    "Qt6Guid.dll" = 27584736
    "Qt6Networkd.dll" = 5387488
    "Qt6WebSocketsd.dll" = 742624
    "Qt6Widgetsd.dll" = 17137376
    "opus.dll" = 907264
    "portaudio.dll" = 522240
}

$allCorrect = $true

foreach ($dll in $expectedSizes.Keys) {
    $path = Join-Path $buildDir $dll
    
    if (Test-Path $path) {
        $actual = (Get-Item $path).Length
        $expected = $expectedSizes[$dll]
        
        if ($actual -eq $expected) {
            Write-Host "[OK] $dll" -ForegroundColor Green -NoNewline
            Write-Host " ($actual bytes)"
        } else {
            Write-Host "[WRONG VERSION] $dll" -ForegroundColor Red
            Write-Host "  Expected: $expected bytes" -ForegroundColor Yellow
            Write-Host "  Actual:   $actual bytes" -ForegroundColor Yellow
            $allCorrect = $false
        }
    } else {
        Write-Host "[MISSING] $dll" -ForegroundColor Red
        $allCorrect = $false
    }
}

# Check platform plugin
$platformPlugin = Join-Path $buildDir "platforms\qwindowsd.dll"
if (Test-Path $platformPlugin) {
    Write-Host "[OK] platforms\qwindowsd.dll" -ForegroundColor Green
} else {
    Write-Host "[MISSING] platforms\qwindowsd.dll" -ForegroundColor Red
    $allCorrect = $false
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan

if ($allCorrect) {
    Write-Host "ALL DLLS CORRECT!" -ForegroundColor Green
    Write-Host "You can run: .\build\Debug\voip-client.exe" -ForegroundColor Green
} else {
    Write-Host "DLLS NEED FIXING!" -ForegroundColor Red
    Write-Host "Run: .\deploy.bat" -ForegroundColor Yellow
}

Write-Host "========================================" -ForegroundColor Cyan
