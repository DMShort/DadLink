@echo off
echo ========================================
echo VoIP Client - Build and Deploy
echo ========================================
echo.

cd /d "%~dp0"

echo [Step 1/3] Building project...
cmake --build build --config Debug --target voip-client
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo BUILD FAILED!
    pause
    exit /b 1
)

echo.
echo [Step 2/3] Deploying dependencies...
call deploy.bat

echo.
echo [Step 3/3] Verifying DLL versions...
powershell -ExecutionPolicy Bypass -File verify_dlls.ps1

echo.
echo ========================================
echo BUILD AND DEPLOY COMPLETE!
echo ========================================
echo.
echo To run the application:
echo   .\build\Debug\voip-client.exe
echo.
pause
