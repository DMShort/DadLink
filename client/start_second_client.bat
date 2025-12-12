@echo off
echo ========================================
echo Starting Second VoIP Client
echo ========================================
echo.
echo This will open a second instance for testing
echo multi-user voice communication.
echo.
echo Instructions:
echo 1. First Client: Login as User1
echo 2. Second Client: Login as User2
echo 3. Both clients will be in Channel 1
echo 4. Speak in one - hear in the other!
echo.
echo Press any key to start second client...
pause >nul

start "VoIP Client 2" ".\build\Debug\voip-client.exe"

echo.
echo Second client started!
echo.
