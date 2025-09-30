@echo off
chcp 65001 >nul
echo Running ListDevices_v2.exe...
ListDevices_v2.exe
echo Exit code: %errorlevel%
rem pause