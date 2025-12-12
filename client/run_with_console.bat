@echo off
cd build\Debug
echo Starting VoIP Client...
voip-client.exe
if errorlevel 1 echo Error code: %errorlevel%
pause
