@echo off
echo Starting VoIP Client...
cd /d "%~dp0"
.\build\Debug\voip-client.exe
echo.
echo Exit code: %ERRORLEVEL%
pause
