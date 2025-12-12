@echo off
echo Killing all voip-client.exe processes...
taskkill /F /IM voip-client.exe 2>nul
if %errorlevel% EQU 0 (
    echo Client processes terminated.
) else (
    echo No client processes found.
)
timeout /t 1 >nul
