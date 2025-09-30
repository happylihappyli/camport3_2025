@echo off
chcp 65001 >nul
dir /b *.exe
echo.
echo Attempting to run ListDevices_v2.exe...
ListDevices_v2.exe
echo Exit code: %errorlevel%